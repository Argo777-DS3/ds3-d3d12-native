#include "game_primitive_geometry.h"
#include <limits>
#include <stdexcept>

namespace ds3native::bootstrap {
void PrimitiveGeometryStorage::Write::CheckWriter() const {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("primitive geometry writer thread changed");
    if(sealed_)throw std::runtime_error("primitive geometry writer already sealed");
}
uint8_t* PrimitiveGeometryStorage::Write::Vertices() {CheckWriter();return vertices_.data();}
uint8_t* PrimitiveGeometryStorage::Write::Indices() {CheckWriter();return indices_.data();}
PrimitiveGeometryStorage::Writer PrimitiveGeometryStorage::Allocate(uint64_t key,uint32_t stride,uint32_t vertices,uint32_t indices) {
    // Source1CF8750 consumes unsigned16 indices and rebases each mesh into one
    // vertex stream. Bounds refuse unsupported transactions before publication.
    constexpr uint64_t max_bytes=16ull*1024*1024;
    const auto vertex_bytes=uint64_t{stride}*vertices,index_bytes=uint64_t{2}*indices;
    if(!key || !stride || !vertices || vertices>65536 || !indices || vertex_bytes>max_bytes || index_bytes>max_bytes)
        throw std::runtime_error("invalid primitive geometry extent");
    auto write=std::make_shared<Write>();write->key_=key;write->stride_=stride;
    write->vertex_count_=vertices;write->index_count_=indices;write->thread_=std::this_thread::get_id();
    write->vertices_.resize(static_cast<size_t>(vertex_bytes));write->indices_.resize(static_cast<size_t>(index_bytes));
    std::lock_guard<std::mutex> hold(lock_);
    if(incarnation_==std::numeric_limits<uint64_t>::max()/2)throw std::runtime_error("primitive geometry incarnation exhausted");
    write->incarnation_=++incarnation_;
    entries_.insert_or_assign(key,Entry{write,{}});return write;
}
std::shared_ptr<const PrimitiveGeometry> PrimitiveGeometryStorage::Seal(const Writer& writer) {
    if(!writer)throw std::runtime_error("missing primitive geometry writer");
    writer->CheckWriter();
    std::lock_guard<std::mutex> hold(lock_);
    const auto entry=entries_.find(writer->key_);
    if(entry==entries_.end() || entry->second.writer!=writer)throw std::runtime_error("withdrawn primitive geometry writer");
    // Copy isolates already borrowed writer addresses. A prepared draw lease
    // remains immutable even if its CPU cache slot is recycled or withdrawn.
    auto geometry=std::make_shared<PrimitiveGeometry>();geometry->incarnation=writer->incarnation_;
    geometry->stride=writer->stride_;geometry->vertex_count=writer->vertex_count_;geometry->index_count=writer->index_count_;
    geometry->vertices={writer->incarnation_*2-1,writer->vertices_};
    geometry->indices={writer->incarnation_*2,writer->indices_};
    // Index validity is assessed after actual CPU writes, before admission.
    for(size_t index=0;index<geometry->indices.bytes.size();index+=2) {
        const uint32_t value=uint32_t{geometry->indices.bytes[index]} | (uint32_t{geometry->indices.bytes[index+1]}<<8);
        if(value>=geometry->vertex_count)throw std::runtime_error("primitive geometry index outside vertices");
    }
    writer->sealed_=true;entry->second.geometry=geometry;return geometry;
}
std::shared_ptr<const PrimitiveGeometry> PrimitiveGeometryStorage::Acquire(uint64_t key) const {
    std::lock_guard<std::mutex> hold(lock_);const auto entry=entries_.find(key);
    return entry==entries_.end()?nullptr:entry->second.geometry;
}
void PrimitiveGeometryStorage::Withdraw(uint64_t key) {std::lock_guard<std::mutex> hold(lock_);entries_.erase(key);}
}
