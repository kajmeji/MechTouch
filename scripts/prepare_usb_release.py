"""Package a Basic USB migration without merging across the NVS settings area."""
import hashlib
import json
import pathlib
import zipfile

from prepare_ota_release import validate_image

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    build = ROOT / '.pio/build/elecrow-crowpanel-7-basic'
    image = (build / 'firmware.bin').read_bytes()
    table = (build / 'partitions.bin').read_bytes()
    version, board = validate_image(image, table)
    if board != 'basic':
        raise ValueError('Expected Basic firmware')
    metadata = json.loads((build / 'idedata.json').read_text(encoding='utf-8'))
    boot_data = next(pathlib.Path(item['path']) for item in metadata['extra']['flash_images']
                     if int(item['offset'], 16) == 0xe000)
    parts = [(0, 'bootloader.bin', (build / 'bootloader.bin').read_bytes()),
             (0x8000, 'partitions.bin', table),
             (0xe000, 'boot_app0.bin', boot_data.read_bytes()),
             (0x10000, 'firmware.bin', image)]
    previous_end = 0
    for address, name, data in parts:
        end = address + len(data)
        if address < previous_end or end > 0x200000:
            raise ValueError(f'Invalid USB segment: {name}')
        # Include erase-sector rounding in the NVS overlap check.
        erase_start, erase_end = address & ~4095, (end + 4095) & ~4095
        if erase_start < 0xe000 and erase_end > 0x9000:
            raise ValueError(f'USB segment would erase NVS: {name}')
        previous_end = end
    manifest = dict(board=board, version=version, parts=[
        dict(file=name, offset=hex(address), size=len(data), sha256=hashlib.sha256(data).hexdigest())
        for address, name, data in parts])
    instructions = f'''MechTouch {version} - Basic 4 MiB - initial USB migration

Export panel settings before installation. Only use this package on the Basic model.
These are USB files, not OTA assets. Keep them as separate files: a merged image
would overwrite the NVS settings gap. Do not use erase-all.

Preferred method from the matching source checkout:
  pio run -e elecrow-crowpanel-7-basic -t upload

Alternatively, extract this archive, install/use esptool 5.x, and replace COMx
with the panel's confirmed serial port:
  esptool --chip esp32s3 --port COMx write-flash --flash-size 4MB 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin

Check serial output after boot: next=2031616 otadata=1
Check display, touch, saved settings and FluidNC connection before testing OTA.
OTA requires internet access, a public GitHub Release, and a connected idle machine.
See docs/OTA.md in the source checkout for the complete validation procedure.
'''
    output = ROOT / f'dist/usb/mechtouch-basic-{version}-usb.zip'
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for _, name, data in parts:
            archive.writestr(name, data)
        archive.writestr('flash.json', json.dumps(manifest, indent=2) + '\n')
        archive.writestr('INSTALL.txt', instructions)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        for _, name, data in parts:
            assert archive.read(name) == data
    print(f'{output.name}: {output.stat().st_size:,} bytes; all segments verified; NVS excluded')


if __name__ == '__main__':
    main()
