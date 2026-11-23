#pragma once
#include <cstdint>

namespace ds3native::bootstrap {
// W3 G0-2 (GAMMA-COLOUR-PATH-READY 1, 3.2): the original's gamma-pass shaders, DXBC blobs in the image's .data (created by
// FUN_141933200 from these addresses and sizes). The SHA-256 values are fixed here, not derived from the executable:
// Build-GameBootstrap.ps1 extracts the bytes at the file offsets and fails if they differ, and the module checks the loaded
// bytes against them before the stage is opened (a mismatch leaves the stage closed: the frame draws straight to the
// back buffer as before).
struct DisplayBlobPin {uint32_t rva,size_rva,file_offset,bytes;const char* sha256;};
constexpr DisplayBlobPin display_vertex_blob{0x46BD950,0x46BDC08,0x46BB950,696,"ECAC101EB24BB344101632D54B7756AEF943CD7C2A9ED435B56A7E788E1B923D"};
constexpr DisplayBlobPin display_pixel_blob{0x46BDC10,0x46BE030,0x46BBC10,1056,"429D7400C0B35DAB096D18FED9ADFFA5D7D752FC4FB76526A4EA3C2AC42F68F8"};
// The engine's curve generator FUN_1400C83D0(out, srcR, srcG, srcB, dstR, dstG, dstB): 1025 interleaved RGB floats.
constexpr uint32_t display_curve_generator_rva=0xC83D0;
}
