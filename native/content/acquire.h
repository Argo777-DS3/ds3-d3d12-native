#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include "../renderer/content.h"
namespace ds3content {
// The FLVER model's vtable (DS3 1.15.2), checked by the model selection and the scene geometry before they read a model.
constexpr uintptr_t model_vtable_rva=0x2951018;
bool Read(uintptr_t address,void* out,size_t bytes) noexcept;
// I-0066 C7-0: Read copies through this function, ReadProcessMemory on the current process by default. A host running
// inside the game (the native bootstrap) installs its in-process guarded copy once, before its first read (one system
// call per field read otherwise; K-COLLECT C7 study).
using ProcessRead=bool(*)(uintptr_t address,void* out,size_t bytes) noexcept;
void SetProcessRead(ProcessRead read) noexcept;
bool Sha256(const void* data,size_t bytes,char (&hex)[65]) noexcept;
}
