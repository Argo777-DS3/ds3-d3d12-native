// I-0132 (Thomas, 2026-11-12 18:18: « filmez l'écran titre avec son capturé »): the sound Windows plays on its default output,
// recorded by WASAPI's loopback into a WAV file, for a film whose audio ffmpeg's gdigrab cannot take. A test tool, never in the
// module. Usage: loopback_record.exe <out.wav> <max seconds>; stops at the bound or when a line « q » (or end of input) arrives on
// its standard input, as the films' ffmpeg. The mix format is written as it comes (usually 32-bit float, 48 kHz, stereo); the
// stretches where nothing plays (loopback then delivers no packet) are filled with silence from the clock, so the file's length is
// the recording's wall time and stays aligned with a film started at the same moment. Prints « started <ISO UTC> <rate> <channels> ».
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>
#pragma comment(lib, "ole32.lib")

static std::atomic<bool> stop_requested{false};

static void WatchInput() {
    char line[64];
    while (fgets(line, sizeof(line), stdin)) { if (line[0] == 'q') break; }
    stop_requested.store(true);
}

static void WriteHeader(FILE* f, const WAVEFORMATEX* wf, uint32_t data_bytes) {
    const uint32_t fmt_size = sizeof(WAVEFORMATEX) + wf->cbSize;
    const uint32_t riff = 4 + 8 + fmt_size + 8 + data_bytes;
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); fwrite(&fmt_size, 4, 1, f); fwrite(wf, 1, fmt_size, f);
    fwrite("data", 1, 4, f); fwrite(&data_bytes, 4, 1, f);
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) { fprintf(stderr, "usage: loopback_record <out.wav> <max seconds>\n"); return 2; }
    const double max_seconds = _wtof(argv[2]);
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) { fprintf(stderr, "CoInitializeEx failed\n"); return 1; }
    IMMDeviceEnumerator* en = nullptr; IMMDevice* dev = nullptr; IAudioClient* client = nullptr; IAudioCaptureClient* cap = nullptr;
    WAVEFORMATEX* wf = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en);
    if (SUCCEEDED(hr)) hr = en->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
    if (SUCCEEDED(hr)) hr = dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&client);
    if (SUCCEEDED(hr)) hr = client->GetMixFormat(&wf);
    if (SUCCEEDED(hr)) hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, wf, nullptr);
    if (SUCCEEDED(hr)) hr = client->GetService(__uuidof(IAudioCaptureClient), (void**)&cap);
    if (FAILED(hr)) { fprintf(stderr, "WASAPI loopback setup failed hr=0x%08lX\n", (unsigned long)hr); return 1; }
    FILE* f = nullptr;
    if (_wfopen_s(&f, argv[1], L"wb") != 0 || !f) { fprintf(stderr, "cannot open the output\n"); return 1; }
    WriteHeader(f, wf, 0);
    const uint32_t block = wf->nBlockAlign, rate = wf->nSamplesPerSec;
    std::thread(WatchInput).detach();
    LARGE_INTEGER freq, t0, now; QueryPerformanceFrequency(&freq);
    hr = client->Start();
    if (FAILED(hr)) { fprintf(stderr, "Start failed hr=0x%08lX\n", (unsigned long)hr); return 1; }
    QueryPerformanceCounter(&t0);
    SYSTEMTIME st; GetSystemTime(&st);
    printf("started %04u-%02u-%02uT%02u:%02u:%02u.%03uZ %u %u\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
        st.wMilliseconds, rate, wf->nChannels);
    fflush(stdout);
    uint64_t frames = 0, packets = 0, padded = 0;
    HRESULT capture_failed = S_OK;   // a failed capture call ends the recording with a non-zero exit (a review's Low)
    std::vector<uint8_t> zeros;
    while (!stop_requested.load()) {
        QueryPerformanceCounter(&now);
        const double elapsed = double(now.QuadPart - t0.QuadPart) / double(freq.QuadPart);
        if (elapsed >= max_seconds) break;
        UINT32 next = 0; bool got = false;
        for (;;) {
            const HRESULT size_hr = cap->GetNextPacketSize(&next);
            if (FAILED(size_hr)) { capture_failed = size_hr; break; }
            if (next == 0) break;
            BYTE* data = nullptr; UINT32 n = 0; DWORD flags = 0;
            const HRESULT buffer_hr = cap->GetBuffer(&data, &n, &flags, nullptr, nullptr);
            if (FAILED(buffer_hr)) { capture_failed = buffer_hr; break; }
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) { zeros.assign(size_t(n) * block, 0); fwrite(zeros.data(), block, n, f); }
            else fwrite(data, block, n, f);
            cap->ReleaseBuffer(n); frames += n; ++packets; got = true;
        }
        if (FAILED(capture_failed)) break;
        // Nothing plays: loopback sends nothing, so the clock fills the gap (beyond 100 ms behind, to leave real packets their place).
        const uint64_t expected = uint64_t(elapsed * rate);
        if (!got && expected > frames + rate / 10) {
            const uint64_t gap = expected - frames - rate / 20;
            zeros.assign(size_t(gap) * block, 0); fwrite(zeros.data(), block, size_t(gap), f); frames += gap; padded += gap;
        }
        Sleep(10);
    }
    client->Stop();
    const uint64_t data_bytes = frames * block;
    WriteHeader(f, wf, uint32_t(data_bytes > 0xFFFFFF00ull ? 0xFFFFFF00ull : data_bytes));
    fclose(f);
    printf("stopped frames=%llu seconds=%.3f packets=%llu padded_seconds=%.3f\n", (unsigned long long)frames, double(frames) / rate,
        (unsigned long long)packets, double(padded) / rate);
    CoTaskMemFree(wf); cap->Release(); client->Release(); dev->Release(); en->Release(); CoUninitialize();
    if (FAILED(capture_failed)) { fprintf(stderr, "capture failed hr=0x%08lX\n", (unsigned long)capture_failed); return 1; }
    return 0;
}
