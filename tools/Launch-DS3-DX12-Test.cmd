@echo off
rem I-0089: the TEST version (invincibility on F4, no boss revived). The saves are a sandbox, put back at the end.
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12-Test.ps1" %*
