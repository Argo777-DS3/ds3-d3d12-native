#pragma once
#include "primitive_work.h"
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace ds3native::bootstrap {
// Storage for the CPU writer identified at1CF8750. A write destination is not
// a legacy buffer object. Original cache offsets do not enter native storage.
struct PrimitiveGeometry {
    uint64_t incarnation{};
    uint32_t stride{},vertex_count{},index_count{};
    frame::PrimitiveBuffer vertices,indices;
};
class PrimitiveGeometryStorage {
public:
    class Write {
    public:
        uint8_t* Vertices();
        uint8_t* Indices();
        uint64_t Incarnation() const noexcept {return incarnation_;}
    private:
        friend class PrimitiveGeometryStorage;
        void CheckWriter() const;
        uint64_t key_{},incarnation_{};
        uint32_t stride_{},vertex_count_{},index_count_{};
        std::thread::id thread_;
        std::vector<uint8_t> vertices_,indices_;
        bool sealed_{};
    };
    using Writer=std::shared_ptr<Write>;
    Writer Allocate(uint64_t key,uint32_t stride,uint32_t vertices,uint32_t indices);
    std::shared_ptr<const PrimitiveGeometry> Seal(const Writer& writer);
    std::shared_ptr<const PrimitiveGeometry> Acquire(uint64_t key) const;
    void Withdraw(uint64_t key);
private:
    struct Entry {Writer writer;std::shared_ptr<const PrimitiveGeometry> geometry;};
    mutable std::mutex lock_;
    std::unordered_map<uint64_t,Entry> entries_;
    uint64_t incarnation_{};
};
}
