#pragma once
#include <cmath>

namespace ds3native::frame {
// D57900/126EA0: preserve the supplied float tick's units and D6 one-call gate.
// This is the simulation accumulator, separate from the original QPC clock.
// Suppression discards even a nonfinite supplied delta, as XORPS does.
inline bool AdvanceSimulationTime(float previous,float delta,bool suppressed,
                                  float& output) noexcept {
    if(!std::isfinite(previous) || (!suppressed && !std::isfinite(delta)))return false;
    const float next=previous+(suppressed?0.0f:delta);
    if(!std::isfinite(next))return false;
    output=next;return true;
}
}
