// Lossless recompression of LVGL 9.5 fonts. Install lv_font_conv@1.5.3 in
// .pio/ota-font-tools first; generated C files need no Node dependency to build.
const fs = require('fs');
const path = require('path');
const {createRequire} = require('module');
const req = createRequire(path.resolve('.pio/ota-font-tools/node_modules/lv_font_conv/package.json'));
const compress = req('./lib/font/compress');
const {BitStream} = req('bit-buffer');
const sizes = [12,14,16,18,20,22,24,26,32];
let saved = 0;
for (const size of sizes) {
  const source = `.pio/libdeps/elecrow-crowpanel-7-basic/lvgl/src/font/lv_font_montserrat_${size}.c`;
  let text = fs.readFileSync(source, 'utf8');
  const match = text.match(/(static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap\[\] = \{)([\s\S]*?)(\n\};)/);
  const raw = Buffer.from(match[2].replace(/\/\*[\s\S]*?\*\//g, '').match(/0x[\da-f]+/gi).map(Number));
  const bpp = Number(text.match(/\.bpp = (\d+)/)[1]);
  const chunks = [];
  let offset = 0;
  text = text.replace(/\.bitmap_index = (\d+)([^}]*?)\.box_w = (\d+), \.box_h = (\d+)/g,
    (all, index, middle, width, height) => {
      const pixels = Array.from({length: Number(width)*Number(height)}, (_, i) => {
        const bit = Number(index)*8+i*bpp;
        return (raw[bit>>3] >> (8-bpp-(bit%8))) & ((1<<bpp)-1);
      });
      // XOR consecutive rows before RLE, exactly as lv_font_conv does.
      const filtered = pixels.map((p,i) => i < Number(width) ? p : p ^ pixels[i-Number(width)]);
      const buffer = Buffer.alloc(100+pixels.length*4);
      const stream = new BitStream(buffer); stream.bigEndian = true;
      compress(stream, filtered, {bpp});
      const encoded = buffer.subarray(0, Math.ceil(stream.index/8));
      // Independently decode the LVGL state machine and verify every pixel.
      let pos=0, prev=0, state=0, count=0;
      const read = n => {let v=0; while(n--) {v=(v<<1)|((buffer[pos>>3]>>(7-pos%8))&1); pos++;} return v;};
      const decoded=[];
      for(let i=0;i<pixels.length;i++) {
        let value;
        if(state===0) {value=read(bpp); if(pos!==bpp && value===prev) {count=0;state=1;} prev=value;}
        else if(state===1) {
          const bit=read(1); count++;
          if(bit) {value=prev; if(count===11) {count=read(6); if(count) state=2; else {value=prev=read(bpp);state=0;}}}
          else {value=prev=read(bpp);state=0;}
        } else {value=prev; if(--count===0) {value=prev=read(bpp);state=0;}}
        decoded.push(i < Number(width) ? value : value ^ decoded[i-Number(width)]);
        if(decoded[i]!==pixels[i]) throw Error(`Pixel mismatch ${source}:${index}:${i}`);
      }
      const next = `.bitmap_index = ${offset}${middle}.box_w = ${width}, .box_h = ${height}`;
      chunks.push(encoded); offset+=encoded.length;
      return next;
    });
  // Decoder reads a second byte at byte boundaries; include a final padding byte.
  const packed = Buffer.concat([...chunks, Buffer.from([0])]);
  const lines=[];
  for(let i=0;i<packed.length;i+=16) lines.push('    '+[...packed.subarray(i,i+16)].map(v=>'0x'+v.toString(16).padStart(2,'0')).join(', ')+',');
  text = text.replace(match[0], match[1]+'\n'+lines.join('\n')+match[3]);
  text = text.replace('.bitmap_format = 0', '.bitmap_format = 1');
  text = text.replace('#include "../../lvgl.h"', '#include "lvgl.h"');
  text = text.replace(`#if LV_FONT_MONTSERRAT_${size}`, '#if 1');
  text = '// Generated losslessly by scripts/compress_fonts.cjs from LVGL 9.5.0 (MIT).\n'+text;
  fs.writeFileSync(`src/ui/fonts/ota_montserrat_${size}.c`,text);
  saved+=raw.length-packed.length;
  console.log(`${size}px: ${raw.length} -> ${packed.length}; all pixels verified`);
}
console.log(`Saved ${saved} bytes`);
