#include "content.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace ds3native {
namespace {
constexpr size_t kMaximumSourceBytes=32u*1024u*1024u;

bool Fits(uint32_t offset,uint64_t bytes,size_t size) noexcept {
    return static_cast<uint64_t>(offset)<=size && bytes<=static_cast<uint64_t>(size)-offset;
}
}

bool ValidContent(std::span<const uint8_t> source,std::span<const MeshSlice> meshes) noexcept {
    if (source.empty() || source.size()>kMaximumSourceBytes ||
        meshes.empty() || meshes.size()>kMaximumContentMeshes) return false;

    float minimum[3]={std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::infinity()};
    float maximum[3]={-std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity()};
    for (const auto& mesh:meshes) {
        if (mesh.vertex_count==0 || mesh.stride<12 || mesh.stride>256 || mesh.stride%4 || mesh.vertex_offset%4 ||
            (mesh.index_width!=2 && mesh.index_width!=4) || mesh.index_offset%mesh.index_width ||
            mesh.index_count==0 || mesh.index_count%3!=0) return false;
        const uint64_t vertex_bytes=static_cast<uint64_t>(mesh.vertex_count)*mesh.stride;
        const uint64_t index_bytes=static_cast<uint64_t>(mesh.index_count)*mesh.index_width;
        if (!Fits(mesh.vertex_offset,vertex_bytes,source.size()) ||
            !Fits(mesh.index_offset,index_bytes,source.size())) return false;
        if (mesh.stride2 && (mesh.stride2<4 || mesh.stride2>256 || mesh.stride2%4 || mesh.vertex_offset2%4 ||
            !Fits(mesh.vertex_offset2,static_cast<uint64_t>(mesh.vertex_count)*mesh.stride2,source.size()))) return false;

        for (uint32_t i=0;i<mesh.vertex_count;++i) {
            float position[3]{};
            const size_t offset=static_cast<size_t>(mesh.vertex_offset)+
                                static_cast<size_t>(i)*mesh.stride;
            std::memcpy(position,source.data()+offset,sizeof(position));
            for (unsigned axis=0;axis<3;++axis) {
                if (!std::isfinite(position[axis]) || std::abs(position[axis])>1.0e6f)
                    return false;
                minimum[axis]=std::min(minimum[axis],position[axis]);
                maximum[axis]=std::max(maximum[axis],position[axis]);
            }
        }
        for (uint32_t i=0;i<mesh.index_count;++i) {
            uint32_t index{};
            const size_t offset=static_cast<size_t>(mesh.index_offset)+
                                static_cast<size_t>(i)*mesh.index_width;
            std::memcpy(&index,source.data()+offset,mesh.index_width);
            if (index>=mesh.vertex_count) return false;
        }
    }
    const double dx=static_cast<double>(maximum[0])-minimum[0];
    const double dy=static_cast<double>(maximum[1])-minimum[1];
    const double dz=static_cast<double>(maximum[2])-minimum[2];
    const double diagonal_squared=dx*dx+dy*dy+dz*dz;
    return std::isfinite(diagonal_squared) && diagonal_squared>1.0e-12;
}

} // namespace ds3native
