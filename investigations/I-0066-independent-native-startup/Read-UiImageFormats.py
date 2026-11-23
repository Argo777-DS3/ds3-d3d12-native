"""Read the installed UI CPU format descriptors; no GPU capability inference."""
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

def read(rva, size):
    matches = [offset+rva-start for start, extent, offset in sections if start <= rva and rva+size <= start+extent]
    assert len(matches) == 1
    return raw[matches[0]:matches[0]+size]

rows = []
for ordinal in range(32):
    source, dxgi, component_bytes, minimum_feature_level, copy, inverse = struct.unpack('<IIIIQQ', read(0x46F8EB0+ordinal*32, 32))
    if not source:
        break
    rows.append(dict(ordinal=ordinal, engine_format=source, dxgi_format=dxgi,
        component_bytes=component_bytes, original_minimum_feature_level=minimum_feature_level,
        cpu_copy_rva=copy-base if copy else None, cpu_inverse_rva=inverse-base if inverse else None))
else:
    raise ValueError('unterminated format table')
out = root/'evidence/I-0066/artifacts/ui-image-physical-ingress/cpu-formats.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(dict(image=str(image), image_sha256=digest, read_only=True,
    table_rva=0x46F8EB0, rows=rows, native_gpu_support_claimed=False), indent=2)+'\n', encoding='utf-8')
print('Recorded15 original CPU image format descriptors; no original GPU probes invoked')
