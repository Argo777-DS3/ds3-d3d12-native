#pragma once
// Shared by the dedicated source mirror and its offline Windows test.
inline DWORD64 ConfigureVelocityDr7(DWORD64 original) {
    DWORD64 value = original;
    for (unsigned int slot=0; slot<4; ++slot) {
        value &= ~(3ull << (slot*2));
        value &= ~(0xFull << (16+slot*4));
        value |= 1ull << (slot*2);
    }
    // Slots 0/1/2 execute; slot 3 watches one four-byte field write.
    value |= 1ull << 28;
    value |= 3ull << 30;
    return value;
}
