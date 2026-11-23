@echo off
rem I-0100 (Thomas, 2026-10-26): starts the native D3D12 version with popin-observe.flag: every object the native draws, when it
rem appears and disappears, with its position and distance, is written to the play journal (%LOCALAPPDATA%\DS3-DX12\play-*.jsonl),
rem with crossing-observe (the collects whose admissions took 3 ms or more).
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12.ps1" -PopinObserve -DiagnosticFlags crossing-observe %*
