#include "post_glare_programs.h"
#include <cstring>
#include "post_glare_table.h"   // written by Build-GameBootstrap.ps1 with the fxc outputs it includes: post_glare_rows[]

namespace ds3native {
bool PostGlareProgram(const std::string& technique,GlareBlobs& out) {
    for(const auto& row:post_glare_rows)if(technique==row.technique){
        out.vertex.assign(row.vertex,row.vertex+row.vertex_bytes);out.pixel.assign(row.pixel,row.pixel+row.pixel_bytes);return true;}
    return false;
}
std::shared_ptr<const GlarePrograms> PostGlarePrograms() {
    static const std::shared_ptr<const GlarePrograms> all=[]{auto map=std::make_shared<GlarePrograms>();
        for(const auto& row:post_glare_rows){auto& blobs=(*map)[row.technique];
            blobs.vertex.assign(row.vertex,row.vertex+row.vertex_bytes);blobs.pixel.assign(row.pixel,row.pixel+row.pixel_bytes);}
        return std::shared_ptr<const GlarePrograms>(std::move(map));}();
    return all;
}
}
