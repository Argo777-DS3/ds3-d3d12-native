@echo off
rem I-0072 (D-0172): starts the native D3D12 version after removing the pipeline cache files of the builds before D-0241 (every
rem start compiles the listed pipelines again while the game boots: the "reload shaders" entry).
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12.ps1" -ReloadShaders
