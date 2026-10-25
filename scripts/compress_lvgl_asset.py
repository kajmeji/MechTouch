"""Losslessly encode an existing RGB565A8 C asset using LVGL 9's RLE format."""
import hashlib
import pathlib
import re
import struct
import sys


def encode(raw, block=2):
    assert len(raw) % block == 0
    pixels = [raw[i:i+block] for i in range(0, len(raw), block)]
    output = bytearray()
    i = 0
    while i < len(pixels):
        end = i + 1
        while end < len(pixels) and pixels[end] == pixels[i] and end-i < 127:
            end += 1
        if end-i >= 2:
            output.append(end-i); output.extend(pixels[i]); i = end
        else:
            end = i + 1
            while end < len(pixels) and end-i < 127:
                if end+1 < len(pixels) and pixels[end] == pixels[end+1]: break
                end += 1
            output.append(0x80 | (end-i)); output.extend(b''.join(pixels[i:end])); i = end
    return bytes(output)


def decode(packed, block=2):
    out = bytearray(); i = 0
    while i < len(packed):
        control = packed[i]; i += 1
        count = control & 127
        if control & 128:
            out.extend(packed[i:i+count*block]); i += count*block
        else:
            out.extend(packed[i:i+block]*count); i += block
    assert i == len(packed)
    return bytes(out)


def compress(path):
    source = path.read_text(encoding='utf-8')
    if 'LV_IMAGE_FLAGS_COMPRESSED' in source: raise ValueError('Already compressed')
    match = re.search(r'(uint8_t\s+\w+\[.*?\]\s*=\s*\{)(.*?)(\};)', source, re.S)
    if not match: raise ValueError('C image array not found')
    raw = bytes(int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{2})', match[2]))
    packed = encode(raw)
    assert decode(packed) == raw
    data = struct.pack('<III', 1, len(packed), len(raw)) + packed
    rows = ['  ' + ', '.join(f'0x{v:02x}' for v in data[i:i+16]) + ',' for i in range(0, len(data), 16)]
    source = source[:match.start(2)] + '\n' + '\n'.join(rows) + '\n' + source[match.end(2):]
    source = source.replace('.header.cf = LV_COLOR_FORMAT_RGB565A8,', '.header.cf = LV_COLOR_FORMAT_RGB565A8,\n  .header.flags = LV_IMAGE_FLAGS_COMPRESSED,')
    source = re.sub(r'\.data_size = [^,]+,', f'.data_size = {len(data)},', source)
    source = f'// Lossless LVGL RLE; original SHA-256: {hashlib.sha256(raw).hexdigest()}\n' + source
    path.write_text(source, encoding='utf-8')
    print(f'{path}: {len(raw):,} -> {len(data):,} bytes; exact round-trip verified')


if __name__ == '__main__': compress(pathlib.Path(sys.argv[1]))
