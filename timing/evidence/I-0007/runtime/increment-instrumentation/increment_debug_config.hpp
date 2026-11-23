#pragma once
// Shared by the probe and the offline Windows debug-register test.
inline DWORD64 ConfigureIncrementDr7(DWORD64 original) {
    DWORD64 value = original;
    for (unsigned int slot = 0; slot < 3; ++slot) {
        value &= ~(3ull << (slot * 2));
        value &= ~(0xFull << (16 + slot * 4));
        value |= 1ull << (slot * 2);
    }
    // Slots 0/1: execute, one-byte length encoding. Slot 2: write, four bytes.
    value |= 1ull << 24;
    value |= 3ull << 26;
    return value;
}
