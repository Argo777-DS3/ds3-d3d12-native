@echo off
rem I-0089: the TEST version with the Shebang save installed and every defeated boss revived (F4 invincibility, F5 revives again).
rem The saves are a sandbox, put back at the end. For some bosses only: pwsh -File Launch-DS3-DX12-Test.ps1 -Shebang -Bosses vordt,iudex (list: -ListBosses); this .cmd already passes -Bosses all, so add other options here, never -Bosses.
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch-DS3-DX12-Test.ps1" -Shebang -Bosses all %*
