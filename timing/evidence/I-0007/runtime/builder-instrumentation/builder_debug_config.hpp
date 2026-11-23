#pragma once
// Shared by the bounded builder mirror and the Windows restoration test.
inline DWORD64 ConfigureBuilderDr7(DWORD64 original) {
    DWORD64 value = original;
    for (unsigned int slot = 0; slot < 4; ++slot) {
        value &= ~(3ull << (slot * 2));
        value &= ~(0xFull << (16 + slot * 4));
        value |= 1ull << (slot * 2);
    }
    return value;
}
