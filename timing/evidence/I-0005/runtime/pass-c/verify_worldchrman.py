#!/usr/bin/env python3
"""Re-identify the WorldChrMan singleton global in the exact C1 PE image."""

from __future__ import annotations

import hashlib
import struct
import sys
from pathlib import Path

EXPECTED_SHA256 = "EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C"
EXPECTED_INSTRUCTION_RVA = 0x004C283A
EXPECTED_GLOBAL_RVA = 0x0477FDB8


def sections(data: bytes) -> list[tuple[int, int, int]]:
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    result = []
    for index in range(count):
        offset = pe + 24 + optional_size + index * 40
        _, virtual_address, raw_size, raw_pointer = struct.unpack_from("<IIII", data, offset + 8)
        result.append((virtual_address, raw_pointer, raw_size))
    return result


def file_offset_to_rva(offset: int, table: list[tuple[int, int, int]]) -> int:
    for virtual_address, raw_pointer, raw_size in table:
        if raw_pointer <= offset < raw_pointer + raw_size:
            return virtual_address + offset - raw_pointer
    raise ValueError(f"file offset 0x{offset:X} is not in a PE section")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_worldchrman.py <DarkSoulsIII.exe>", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest().upper()
    matches = []
    table = sections(data)
    for offset in range(len(data) - 28):
        if data[offset : offset + 3] != b"\x48\x8B\x1D":
            continue
        if data[offset + 7 : offset + 17] != b"\x48\x8B\xF9\x48\x85\xDB\x74\x40\x8B\x11":
            continue
        instruction_rva = file_offset_to_rva(offset, table)
        displacement = struct.unpack_from("<i", data, offset + 3)[0]
        global_rva = instruction_rva + 7 + displacement
        matches.append((instruction_rva, global_rva, data[offset : offset + 28].hex(" ").upper()))
    print(f"sha256={digest}")
    for instruction_rva, global_rva, encoded in matches:
        print(f"instruction_rva=0x{instruction_rva:08X} global_rva=0x{global_rva:08X} bytes={encoded}")
    accepted = (
        digest == EXPECTED_SHA256
        and matches == [(EXPECTED_INSTRUCTION_RVA, EXPECTED_GLOBAL_RVA, matches[0][2])]
    ) if matches else False
    print(f"accepted={int(accepted)}")
    return 0 if accepted else 1


if __name__ == "__main__":
    raise SystemExit(main())
