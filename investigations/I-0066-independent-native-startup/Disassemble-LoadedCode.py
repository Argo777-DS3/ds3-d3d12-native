"""Disassemble bounded acquired CPU code with the installed MSVC tool.

The acquired journal/JSON remains authoritative. A temporary derived PE only
provides the original image address to dumpbin and is removed after use.
"""
import argparse, hashlib, json, re, struct, subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("input", type=Path)
parser.add_argument("dumpbin", type=Path)
args = parser.parse_args()
source = json.loads(args.input.read_text(encoding="utf-8-sig"))
for record in source["records"]:
    code = bytes.fromhex(record["hex"])
    assert 16 <= len(code) <= 512 and len(code) == record["bytes"]
    assert hashlib.sha256(code).hexdigest().upper() == record["sha256"]
    rva = record["rva"]
    page = rva & ~4095
    offset = rva - page
    raw_size = (offset + len(code) + 511) & ~511
    image = bytearray(512 + raw_size)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 60, 128)
    image[128:132] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", image, 132, 0x8664, 1, 0, 0, 0, 240, 0x2022)
    optional = 152
    struct.pack_into("<H", image, optional, 0x20B)
    struct.pack_into("<I", image, optional + 16, rva)
    struct.pack_into("<Q", image, optional + 24, 0x140000000)
    struct.pack_into("<II", image, optional + 32, 4096, 512)
    struct.pack_into("<II", image, optional + 56, page + 4096, 512)
    struct.pack_into("<H", image, optional + 68, 3)
    struct.pack_into("<I", image, optional + 108, 16)
    section = optional + 240
    image[section:section + 8] = b".text\0\0\0"
    struct.pack_into("<IIII", image, section + 8, offset + len(code), page, raw_size, 512)
    struct.pack_into("<I", image, section + 36, 0x60000020)
    # Single-byte INT3 padding prevents a preceding synthetic zero instruction
    # from consuming the first acquired byte at an unaligned continuation.
    image[512:512 + offset] = b"\xcc" * offset
    image[512 + offset:512 + offset + len(code)] = code
    temporary = args.input.with_name(f"loaded-{rva:x}-disassembly.dll")
    if temporary.exists():
        raise RuntimeError("Never overwrite an existing disassembly input")
    try:
        temporary.write_bytes(image)
        result = subprocess.check_output([str(args.dumpbin), "/nologo", "/disasm:nobytes", str(temporary)], text=True)
        lines = []
        for line in result.splitlines():
            match = re.match(r"\s*([0-9A-Fa-f]+):", line)
            if match and 0x140000000 + rva <= int(match[1], 16) < 0x140000000 + rva + len(code):
                lines.append(line)
        if not lines:
            raise RuntimeError("No acquired instructions disassembled")
        output = args.input.with_name(f"loaded-{rva:x}.asm.txt")
        if output.exists():
            raise RuntimeError("Never overwrite disassembly evidence")
        output.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(output)
    finally:
        temporary.unlink(missing_ok=True)
