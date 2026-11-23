#pragma once
#include "glare_stage.h"
#include <memory>
#include <string>

namespace ds3native {
// I-0138 tranche 2 (ROADMAP workstream 11): the glare family's programmes of our own (shaders/post_glare.hlsl, compiled by the
// build into headers and listed in its table post_glare_table.h), in place of YEBIS's read from the game's image. They are named
// by the technique the plan's draw names (yebis::GlareDraw::technique: the bloom's taps, bright pass and composite, the light
// shafts' mask, radial blurs and add, Alpha_R), since the plan is unchanged in this tranche. I-0138 tranche 4 (part 4a): the
// depth of field's techniques (the depth factor, the front blur's blend factor, the composite's forty counts, the taps at 16, 20
// and 25) and the motion blur's (the camera velocity, the blur at 2 to 8 taps) too.
// Compared with the originals' listings and values, all (investigations/I-0138-modern-post-chain/Compare-Programmes.py); drawn
// by a test, those a plan of the glare, the shafts, the depth of field or the motion blur names, the composite's and the blur's
// every count (dof_stage_test, mb_stage_test). The others (tech_MadT1_S2, S6, S8, S10, S12, S14, tech_MadLumGammaT2, and the
// vertex programmes for two and six offsets) are the family's same shapes at other counts, compared by the reader only.
// One technique's pair; false when it is not one of the family's.
bool PostGlareProgram(const std::string& technique,GlareBlobs& out);
// Every pair of the family by technique, built once (the frames share it: a plan's draws find theirs in it).
std::shared_ptr<const GlarePrograms> PostGlarePrograms();
}
