"""Bounded original-PE RIP-LEA references to the two observed UI owner tables.

Read-only constructor discovery when the existing Ghidra xref index is empty.
This does not infer a function boundary or qualify initialization/ownership.
"""
from pathlib import Path
import hashlib
import json
import struct
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--game-factory', action='store_true', help='Only callers of the qualified E5B250 game UI factory')
args = parser.parse_args()

root = Path(__file__).resolve().parents[2]
image_path = Path(r'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe')
raw = image_path.read_bytes()
digest = hashlib.sha256(raw).hexdigest().upper()
assert digest == 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
pe = struct.unpack_from('<I', raw, 60)[0]
assert raw[pe:pe+4] == b'PE\0\0'
base = struct.unpack_from('<Q', raw, pe+48)[0]
count = struct.unpack_from('<H', raw, pe+6)[0]
optional = struct.unpack_from('<H', raw, pe+20)[0]
targets = {base+0x3B4DD40: 'observed HAL', base+0x3B4E570: 'embedded mesh cache'}
constructors = {base+0x1F72680: 'observed HAL constructor', base+0x1F7C8E0: 'mesh-cache constructor'}
if args.game_factory:
    targets = {}
    constructors = {base+0xE5B250: 'game UI factory'}
references = []
for section in range(count):
    at = pe+24+optional+section*40
    name = raw[at:at+8].rstrip(b'\0').decode('ascii')
    rva, size, offset = struct.unpack_from('<III', raw, at+12)
    flags = struct.unpack_from('<I', raw, at+36)[0]
    if not flags & 0x20000000:
        continue
    data = raw[offset:offset+size]
    for prefix in (b'\x48\x8D', b'\x4C\x8D'):
        position = -1
        while True:
            position = data.find(prefix, position+1)
            if position < 0:
                break
            if position+7 > len(data) or data[position+2] & 0xC7 != 5:
                continue
            address = base+rva+position
            target = address+7+struct.unpack_from('<i', data, position+3)[0]
            if target in targets:
                references.append(dict(section=name, rva=address-base, target_rva=target-base,
                    responsibility=targets[target], instruction=data[position:position+7].hex()))
    position = -1
    while True:
        position = data.find(b'\xE8', position+1)
        if position < 0:
            break
        if position+5 > len(data):
            continue
        address = base+rva+position
        target = address+5+struct.unpack_from('<i', data, position+1)[0]
        if target in constructors:
            references.append(dict(section=name, rva=address-base, target_rva=target-base,
                responsibility=constructors[target], instruction=data[position:position+5].hex(),
                instruction_boundary_qualified=False))
out = root/'evidence/I-0066/artifacts'/('ui-game-factory-callers' if args.game_factory else 'ui-owner-constructor-references')
out.mkdir(parents=True, exist_ok=True)
(out/'original-pe-references.json').write_text(json.dumps(dict(image=str(image_path),
    image_sha256=digest, read_only=True, targets=[hex(value-base) for value in targets],call_targets=[hex(value-base) for value in constructors],
    references=references), indent=2)+'\n', encoding='utf-8')
for reference in references:
    print(f"{reference['rva']:X} -> {reference['target_rva']:X} {reference['responsibility']}")
