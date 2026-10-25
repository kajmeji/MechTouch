"""Package application-only OTA assets; never upload or flash anything."""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import struct

BOARDS = {"basic": "elecrow-crowpanel-7-basic",
          "advance-v1.2": "elecrow-crowpanel-7-advance-v12",
          "advance-v1.3": "elecrow-crowpanel-7-advance-v13"}


def validate_image(image, table):
    if len(image) < 288 or image[0] != 0xE9 or struct.unpack_from("<H", image, 12)[0] != 9:
        raise ValueError("Expected ESP32-S3 application image")
    if struct.unpack_from("<I", image, 32)[0] != 0xABCD5432:
        raise ValueError("Application descriptor missing (do not use merged/bootloader images)")
    slots, metadata = [], False
    for offset in range(0, len(table) - 31, 32):
        magic, kind, subtype, address, size = struct.unpack_from("<HBBII", table, offset)
        if magic != 0x50AA:
            break
        if kind == 0 and subtype in (0x10, 0x11):
            slots.append(size)
        if kind == 1 and subtype == 0 and size == 8192:
            metadata = True
    if len(slots) != 2 or not metadata or len(image) > min(slots):
        raise ValueError("Firmware does not fit a dual-slot OTA partition table")
    identities = re.findall(rb"MechTouch-OTA:([0-9]+\.[0-9]+\.[0-9]+):(basic|advance-v1\.[23])\x00", image)
    if len(identities) != 1:
        raise ValueError("Expected exactly one embedded MechTouch OTA identity")
    return tuple(part.decode("ascii") for part in identities[0])


def package(root, output, version, boards):
    if not re.fullmatch(r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)", version):
        raise ValueError("Stable release version must be X.Y.Z")
    if any(int(n) > 65535 for n in version.split('.')):
        raise ValueError("Version component exceeds device limit")
    config = (root / "include/config.h").read_text(encoding="utf-8")
    if f'#define FLUIDTOUCH_VERSION "{version}"' not in config:
        raise ValueError("config.h version must match release")
    output.mkdir(parents=True, exist_ok=True)
    for board in boards:
        build = root / ".pio/build" / BOARDS[board]
        image = (build / "firmware.bin").read_bytes()
        embedded = validate_image(image, (build / "partitions.bin").read_bytes())
        if embedded != (version, board):
            raise ValueError(f"Wrong/stale build: embedded identity {embedded!r}, expected {(version, board)!r}")
        filename = f"mechtouch-{board}-{version}.bin"
        shutil.copyfile(build / "firmware.bin", output / filename)
        manifest = dict(schema=1, board=board, version=version, size=len(image),
                        sha256=hashlib.sha256(image).hexdigest(),
                        url=f"https://github.com/kajmeji/MechTouch/releases/download/v{version}/{filename}")
        (output / f"ota-{board}.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        print(f"{board}: {len(image):,} bytes; manifest and firmware ready")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version")
    parser.add_argument("--board", choices=BOARDS, action="append")
    parser.add_argument("--output", type=pathlib.Path, default=pathlib.Path("dist/ota"))
    args = parser.parse_args()
    package(pathlib.Path(__file__).resolve().parents[1], args.output, args.version, args.board or list(BOARDS))
