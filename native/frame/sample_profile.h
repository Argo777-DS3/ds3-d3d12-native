#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <span>
#include <string>

namespace ds3native::sampling {
// m45 frame cost (sample-profile.flag, a measurement aid for announced runs): an in-process sampling profiler of named
// threads. Every millisecond while `active` reads non-zero, each thread is suspended only long enough to copy its context
// and its used stack, then resumed; the copy is unwound afterwards (RtlVirtualUnwind on the copy, pointers into the
// original stack moved to the copy), so no lock the sampled thread may hold is taken while it is suspended. After `limit`
// samples per thread, the samples and the loaded modules are written to `output` (format in sample_profile.cpp) and the
// sampler ends; Tools: native/frame/Get-SampleProfile.ps1 (sample_symbolize.exe) names the frames from the module's PDB.
struct Thread {DWORD id{};const char* name{};};
// Starts the sampler thread once; false when a thread cannot be opened or the buffers cannot be reserved. I-0074: with a
// `mark` (the engine thread's frame clock), each sample also records the mark's value when it was taken, so a report can
// keep only the samples of chosen frames (sample_symbolize frames:FILE). I-0079: sampling also ends (and the samples are
// written) once the mark reaches `end_mark`, so a short run need not reach `limit`.
bool Start(const std::wstring& output,std::span<const Thread> threads,const std::atomic<uint64_t>& active,uint32_t limit,
    const std::atomic<uint64_t>* mark=nullptr,uint64_t end_mark=~0ull) noexcept;
// I-0074: one more thread (a loader thread met at a texture or model admission), sampled from the next tick until it has
// at most `limit` samples while the threads Start named run (the end is theirs); at most 32 added threads, a thread already sampled or added is ignored. Callable from any
// thread, before or after Start (false when the table is full).
bool AddThread(DWORD id,const char* name) noexcept;
// I-0082: the sampler's own thread, whose stack copies and unwinds handle their faults (the crash handler leaves them).
bool OnSamplerThread() noexcept;
}
