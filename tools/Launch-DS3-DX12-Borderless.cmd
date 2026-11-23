@echo off
rem I-0076 (D-0170): starts the native D3D12 version with the borderless window: "full screen" in the graphics options becomes
rem a borderless window over the monitor (DS3's own screen mode 2) instead of exclusive full screen.
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12.ps1" -Borderless
