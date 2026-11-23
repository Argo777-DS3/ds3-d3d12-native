#pragma once
// I-0082 (DESIGN 3): the minidump and the stack report of a native fault. The controller (game_bootstrap_ctl) calls them on
// the game process from outside it; the module calls WriteDump on itself only when no controller answers (the fallback,
// from a thread of its own). dbghelp is not thread-safe: one caller at a time in a process.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <string>

namespace ds3native::crash {
// A minidump of `process` at `path`: thread info, unloaded modules, full memory info, the memory the stacks point to, and the
// data segments of the module named `keep_module` (file name) alone. `exception`: the EXCEPTION_POINTERS address of the fault
// in `process` (0 when none), for thread `thread`. False when the file could not be written (the partial file removed).
// with_process_data: the PEB and TEBs (MiniDumpWithProcessThreadData: the environment block among them); false for a play session's
// dump, which a player may share (security audit M2, 2026-11-23). The user's name stays in it all the same: in the modules' paths,
// in the kept data segments (fallback_dump.path) and in the memory the stacks point to.
bool WriteDump(HANDLE process,DWORD pid,DWORD thread,uint64_t exception,const wchar_t* path,const wchar_t* keep_module,bool with_process_data=true) noexcept;
// The stack of `thread` in `process`, innermost frame first, one line each: module+offset, then the function and line from
// the module's symbols (the PDB of game_bootstrap.dll), FUN_<address> for DarkSoulsIII.exe (its .pdata function start, the
// Ghidra name). From the exception's context when `exception` is set, else from the thread, suspended while it is read.
// `process` must be another process than the caller's, or the caller's own with a `thread` that is not the calling thread.
// `symbol_path`: the folders searched for PDBs (the module's own folder); no symbol server, no _NT_SYMBOL_PATH.
std::string StackReport(HANDLE process,DWORD thread,uint64_t exception,const wchar_t* symbol_path,unsigned max_frames=64);
// The exception record's code and address (read from `process`), for the report's head; false when unreadable.
bool ExceptionHead(HANDLE process,uint64_t exception,uint32_t& code,uint64_t& address) noexcept;
}
