"""Generate the flash-resident translation catalog. Run from any directory."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / 'include/ui/i18n/tr.json').read_text(encoding='utf-8'))
tokens = re.compile(r'%(?:[-+ #0]*\d*(?:\.\d+)?[hlzjtL]*[diuoxXfFeEgGaAcsp]|%)')
for en, tr in list(data.items()):
    assert tokens.findall(en) == tokens.findall(tr), (en, tr)
    data.setdefault(en.strip(), tr.strip())
    # Adjacent C string literals often form multiline dialog messages.
    if len(en.splitlines()) == len(tr.splitlines()):
        for a, b in zip(en.splitlines(), tr.splitlines()):
            if a.strip() and tokens.findall(a) == tokens.findall(b):
                data.setdefault(a, b)
                data.setdefault(a.strip(), b.strip())
path = ROOT / 'src/ui/ui_language.cpp'
source = path.read_text(encoding='utf-8')
start = source.index('const Entry catalog[] = {')
end = source.index('\n};', start) + 3
catalog = 'const Entry catalog[] = {\n' + '\n'.join(
    '    {' + json.dumps(en, ensure_ascii=False) + ', ' + json.dumps(tr, ensure_ascii=False) + '},'
    for en, tr in sorted(data.items())) + '\n};'
path.write_text(source[:start] + catalog + source[end:], encoding='utf-8')
print(f'Generated {len(data)} translations; format arguments verified.')
