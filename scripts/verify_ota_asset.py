"""Verify the packed splash against its original checksum; emit host decoder fixtures."""
import hashlib
import pathlib
import re
import struct
from compress_lvgl_asset import decode

root = pathlib.Path(__file__).resolve().parents[1]
source = (root / 'src/ui/images/fluidnc_logo.c').read_text()
array = re.search(r'uint8_t\s+\w+\[\]\s*=\s*\{(.*?)\};', source, re.S)[1]
data = bytes(int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{2})', array))
method, size, raw_size = struct.unpack_from('<III', data)
assert method == 1 and size == len(data)-12
raw = decode(data[12:])
assert len(raw) == raw_size
expected = re.search(r'original SHA-256: ([0-9a-f]{64})', source)[1]
assert hashlib.sha256(raw).hexdigest() == expected
output = root / '.pio/ota-deps'
output.mkdir(parents=True, exist_ok=True)
(output / 'splash.rle').write_bytes(data[12:])
(output / 'splash.raw').write_bytes(raw)
print(f'Splash verified: {size + 12:,} compressed bytes -> {raw_size:,} identical bytes')
