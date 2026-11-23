#pragma once
#include <cstdio>
#include <share.h>

// The legacy _wfopen_s write mode denies external reads even after fflush.
// Keep the writer exclusive against other writers, but permit read-only analysis.
inline FILE* OpenSharedCaptureLog(const wchar_t* path) {
    return _wfsopen(path, L"wb", _SH_DENYWR);
}
