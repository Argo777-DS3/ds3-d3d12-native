@echo off
rem I-0100 (Thomas, 2026-10-27): the TEST version for observation sessions: enemies without AI (test-no-ai.flag, they never
rem take the player as their target), popin-observe.flag and crossing-observe, as Launch-DS3-DX12-Popin.cmd writes them, in the
rem test journal (%LOCALAPPDATA%\DS3-DX12-Test\test-*.jsonl). F4 invincibility as the other test versions. The saves are a
rem sandbox, put back at the end. Other options follow (e.g. -Shebang -Borderless).
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12-Test.ps1" -NoAi -PopinObserve -DiagnosticFlags crossing-observe %*
