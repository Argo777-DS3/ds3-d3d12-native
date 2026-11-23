@echo off
rem I-0100 tranche 2 (2026-10-26): starts Dark Souls III on the original D3D11 renderer with popin-observe.flag: what the original
rem draws (its main-view draws and its shadow casters) is written to the play journal (%LOCALAPPDATA%\DS3-DX12\play-*.jsonl), to compare
rem with a native play at the same places (investigations\I-0100-popin\Get-PopinJournal.ps1 -Reference).
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12.ps1" -Original -PopinObserve %*
