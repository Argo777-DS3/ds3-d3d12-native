#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ds3content {
enum class ShaderStage : uint16_t { Pixel, Vertex, Geometry, Hull, Domain, Compute };
struct CpuShaderProgram {
    int32_t id{};
    std::string name;
    ShaderStage stage{};
    uint8_t model_major{},model_minor{};
    std::vector<uint8_t> bytecode;
};
struct CpuShaderBank {
    std::string resource_key;
    std::vector<CpuShaderProgram> programs;
};
}
