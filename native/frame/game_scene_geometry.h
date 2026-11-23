#pragma once
#include "../renderer/content.h"
#include <cstddef>
#include <cstdint>

namespace ds3native::ingress {
// Genuine CPU FLVER tables joined to one owned data-region allocation. Native
// initial selection admits unflagged LOD0/R16 triangle lists with one stream.
// Other topology/stream/storage families remain unsupported, never inferred.
bool CopyModelGeometry(uintptr_t model,uintptr_t image,uintptr_t storage,
    uint32_t index_buffer_count,size_t source_bytes,std::vector<MeshSlice>& output,int* refusal=nullptr) noexcept;
}
