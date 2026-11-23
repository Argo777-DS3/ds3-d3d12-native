#pragma once
inline bool ValidAccumulatorCaptureDuration(float seconds) { return seconds == 12.0f; }
// Shared by the dedicated source mirror and its offline Windows test.
inline DWORD64 ConfigureVelocityDr7(DWORD64 original) {
    DWORD64 value = original;
    for (unsigned int slot=0; slot<4; ++slot) {
        value &= ~(3ull << (slot*2));
        value &= ~(0xFull << (16+slot*4));
        value |= 1ull << (slot*2);
    }
    // Four execution observations; none writes game memory.
    return value;
}
