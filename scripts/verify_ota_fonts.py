"""Compare compressed fonts with pinned LVGL originals, emit decoder fixtures."""
import pathlib
import re
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]
SIZES = (12, 14, 16, 18, 20, 22, 24, 26, 32)


def read_font(path):
    text = path.read_text(encoding='utf-8')
    bitmap = re.search(r'const uint8_t glyph_bitmap\[\] = \{(.*?)\n\};', text, re.S).group(1)
    bitmap = re.sub(r'/\*.*?\*/', '', bitmap, flags=re.S)
    data = bytes(int(v, 16) for v in re.findall(r'0x([\da-f]+)', bitmap))
    glyphs = re.findall(r'\{\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), \.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}', text)
    return data, [tuple(map(int, g)) for g in glyphs]


def main():
    output = bytearray()
    count = 0
    for size in SIZES:
        raw, originals = read_font(ROOT / f'.pio/libdeps/elecrow-crowpanel-7-basic/lvgl/src/font/lv_font_montserrat_{size}.c')
        packed, glyphs = read_font(ROOT / f'src/ui/fonts/ota_montserrat_{size}.c')
        assert len(originals) == len(glyphs) and len(glyphs) > 100
        for i, (old, new) in enumerate(zip(originals, glyphs)):
            assert old[1:] == new[1:], 'Glyph metrics changed'
            _, _, w, h, _, _ = old
            if not w*h:
                continue
            end = glyphs[i+1][0] if i+1 < len(glyphs) else len(packed)
            encoded = packed[new[0]:end] + b'\0'
            expected = bytes(((raw[old[0]+p//2] >> (4 if p%2 == 0 else 0)) & 15)*17 for p in range(w*h))
            output.extend(struct.pack('<III', w, h, len(encoded)))
            output.extend(encoded)
            output.extend(expected)
            count += 1
    target = ROOT / '.pio/ota-deps/fonts.bin'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(output)
    print(f'{count} glyph fixtures; all metrics unchanged')


if __name__ == '__main__':
    main()
