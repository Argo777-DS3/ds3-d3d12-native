#pragma once
#include "shader_bank.h"
#include <memory>

namespace ds3content {
struct ShaderResourceDeclaration {
    std::string name;
    uint32_t input_type{},return_type{},dimension{},samples{},shader_register{},count{},flags{};
};
struct ShaderVariableDeclaration {
    std::string name;
    uint32_t byte_offset{},bytes{},flags{};
    // Original RDEF type fields. Availability of bytes is independent of any
    // recovered engine meaning for the variable or its source values.
    uint16_t value_class{},scalar_type{},rows{},columns{},elements{},members{};
};
struct ShaderConstantDeclaration {
    std::string name;
    uint32_t declared_bytes{},flags{},buffer_type{};
    std::vector<ShaderVariableDeclaration> variables;
};
struct ShaderSignatureDeclaration {
    std::string semantic;
    uint32_t semantic_index{},system_value{},component_type{},shader_register{};
    uint8_t mask{},usage_mask{};
};
struct ShaderInterface {
    ShaderStage stage{};
    std::vector<ShaderResourceDeclaration> resources;
    std::vector<ShaderConstantDeclaration> constants;
    std::vector<ShaderSignatureDeclaration> inputs,outputs;
};
// Owned declarations from original SM5.0 RDEF/ISGN/OSGN, with bounded container
// and table validation. No compiler, graphics device, old backend or file I/O.
// Declarations authenticate a byte layout, not the source/lifetime of values.
// Other signature/container versions require a separate decoder.
ShaderInterface InspectShaderInterface(const CpuShaderProgram& program);
// The same declarations, decoded once per distinct (stage, model, bytecode)
// and shared: a hit compares the whole bytecode, so the result is exactly
// InspectShaderInterface(program). A refused program is never retained.
std::shared_ptr<const ShaderInterface> InspectShaderInterfaceShared(const CpuShaderProgram& program);
}
