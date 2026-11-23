"""Exact original CPU dispatch rows for the two observed UI vertex conversions."""
from pathlib import Path
import hashlib
import json
import struct

root = Path(__file__).resolve().parents[2]
image = Path(r'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe')
raw = image.read_bytes()
digest = hashlib.sha256(raw).hexdigest().upper()
assert digest == 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
pe = struct.unpack_from('<I', raw, 60)[0]
count = struct.unpack_from('<H', raw, pe+6)[0]
optional = struct.unpack_from('<H', raw, pe+20)[0]
base = struct.unpack_from('<Q', raw, pe+48)[0]
sections = [struct.unpack_from('<III', raw, pe+24+optional+i*40+12) for i in range(count)]
def read(address, size):
    rva = address-base
    matches = [offset+rva-start for start, extent, offset in sections if start <= rva and rva+size <= start+extent]
    assert len(matches) == 1
    offset = matches[0]
    assert offset+size <= len(raw)
    return raw[offset:offset+size]

rows = []
for role in (1, 2):
    table = struct.unpack('<Q', read(base+0x46EA4B8+role*8, 8))[0]
    for ordinal in range(16):
        source, destination, mask, padding, function = struct.unpack('<IIIIQ', read(table+ordinal*24, 24))
        if not destination:
            break
        rows.append(dict(role=role, ordinal=ordinal, table_rva=table-base,
                         source=source, destination=destination, mask=mask, function_rva=function-base))
    else:
        raise ValueError('unterminated CPU conversion table')
out = root/'evidence/I-0066/artifacts/ui-staged-vertex-conversion/cpu-dispatch.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(dict(image=str(image), image_sha256=digest, read_only=True,
    type_bytes=list(struct.unpack('<8I', read(base+0x46EA3B8, 32))),
    rows=rows, instruction_boundaries_qualified_by='141cf2170.asm.txt'), indent=2)+'\n', encoding='utf-8')
print('Recorded only observed position/color CPU dispatch roles1,2')
