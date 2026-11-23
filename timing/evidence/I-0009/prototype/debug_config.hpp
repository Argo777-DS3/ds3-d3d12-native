#pragma once

#include <windows.h>

inline DWORD64 ConfigurePrototypeDr7(DWORD64 original) {
    DWORD64 value = original;
    for (unsigned int slot = 0; slot < 4; ++slot) {
        value &= ~(3ull << (slot * 2));
        value &= ~(0xFull << (16 + slot * 4));
        if (slot < 3) value |= 1ull << (slot * 2);
    }
    value |= 1ull << 24;  // DR2 write-only.
    value |= 3ull << 26;  // DR2 watches four bytes.
    return value;
}

inline bool SameDebugState(const CONTEXT& left, const CONTEXT& right) {
    return left.Dr0 == right.Dr0 && left.Dr1 == right.Dr1 &&
           left.Dr2 == right.Dr2 && left.Dr3 == right.Dr3 &&
           left.Dr6 == right.Dr6 && left.Dr7 == right.Dr7;
}
