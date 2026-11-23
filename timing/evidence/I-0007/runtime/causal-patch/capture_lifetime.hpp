#pragma once
struct PostMeasureCapture {
    bool active = false;
    double elapsed = 0.0;
    void Begin() { active = true; elapsed = 0.0; }
    // Retain the same correction after the measured window while input is held.
    // A bounded safety timeout stops the entire probe, never silently just the patch.
    bool Advance(bool held, float delta) {
        elapsed += static_cast<double>(delta);
        active = held && elapsed < 10.0;
        return active;
    }
};
