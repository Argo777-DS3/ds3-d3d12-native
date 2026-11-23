#include "engine_window.h"

#include <limits>

namespace ds3native::frame::window {
namespace {

struct Sample {
    uintptr_t service{};
    uintptr_t child{};
    uintptr_t hwnd{};
    uint32_t width{};
    uint32_t height{};
};

bool AddOffset(uintptr_t base, uintptr_t offset, uintptr_t& result) noexcept {
    if (base > (std::numeric_limits<uintptr_t>::max)() - offset) return false;
    result = base + offset;
    return true;
}

bool ReadCurrentProcess(uintptr_t address, void* destination, size_t bytes,
                        void*) noexcept {
    SIZE_T transferred{};
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
                             destination, bytes, &transferred) != FALSE &&
           transferred == bytes;
}

bool ReadAt(ReadBytes reader, void* context, uintptr_t address,
            void* destination, size_t bytes) noexcept {
    return reader(address, destination, bytes, context);
}

bool Capture(const void* image, uintptr_t expected_service, ReadBytes reader,
             void* context, Sample& sample) noexcept {
    const auto base = reinterpret_cast<uintptr_t>(image);
    uintptr_t service_address{}, width_address{}, height_address{};
    if (!image || !AddOffset(base, kWindowServiceRva, service_address) ||
        !AddOffset(base, kWidthRva, width_address) ||
        !AddOffset(base, kHeightRva, height_address)) return false;

    Sample captured{};
    if (!ReadAt(reader, context, service_address, &captured.service,
                sizeof(captured.service)) ||
        captured.service == 0 ||
        (expected_service != 0 && captured.service != expected_service)) return false;

    uintptr_t hwnd_address{}, child_address{};
    if (!AddOffset(captured.service, kServiceHwndOffset, hwnd_address) ||
        !AddOffset(captured.service, kServiceChildOffset, child_address) ||
        !ReadAt(reader, context, hwnd_address, &captured.hwnd, sizeof(captured.hwnd)) ||
        !ReadAt(reader, context, child_address, &captured.child, sizeof(captured.child)) ||
        !ReadAt(reader, context, width_address, &captured.width, sizeof(captured.width)) ||
        !ReadAt(reader, context, height_address, &captured.height, sizeof(captured.height)))
        return false;

    if (captured.hwnd == 0 || captured.width == 0 || captured.height == 0 ||
        captured.width > 16384 || captured.height > 16384) return false;
    sample = captured;
    return true;
}

bool SameSample(const Sample& a, const Sample& b) noexcept {
    return a.service == b.service && a.child == b.child && a.hwnd == b.hwnd &&
           a.width == b.width && a.height == b.height;
}

bool BoundedDimension(LONG value, uint32_t& dimension) noexcept {
    if (value <= 0 || value > 16384) return false;
    dimension = static_cast<uint32_t>(value);
    return true;
}

} // namespace

bool SnapshotWindow(const void* image, uintptr_t expected_service,
                    WindowSnapshot& output, ReadBytes reader,
                    void* reader_context) noexcept {
    if (!reader) reader = &ReadCurrentProcess;

    Sample first{}, second{};
    if (!Capture(image, expected_service, reader, reader_context, first) ||
        !Capture(image, expected_service, reader, reader_context, second) ||
        !SameSample(first, second)) return false;

    const auto hwnd = reinterpret_cast<HWND>(first.hwnd);
    if (!IsWindow(hwnd)) return false;

    DWORD process_id{};
    const DWORD window_thread_id = GetWindowThreadProcessId(hwnd, &process_id);
    const DWORD snapshot_thread_id = GetCurrentThreadId();
    if (window_thread_id == 0 || process_id != GetCurrentProcessId()) return false;

    RECT client{};
    if (!GetClientRect(hwnd, &client)) return false;
    uint32_t client_width{}, client_height{};
    if (!BoundedDimension(client.right - client.left, client_width) ||
        !BoundedDimension(client.bottom - client.top, client_height)) return false;

    WindowSnapshot snapshot{};
    snapshot.service = first.service;
    snapshot.child = first.child;
    snapshot.hwnd = hwnd;
    snapshot.window_thread_id = window_thread_id;
    snapshot.snapshot_thread_id = snapshot_thread_id;
    snapshot.configured_width = first.width;
    snapshot.configured_height = first.height;
    snapshot.client_width = client_width;
    snapshot.client_height = client_height;
    output = snapshot;
    return true;
}

} // namespace ds3native::frame::window
