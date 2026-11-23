# I-0005 runtime Pass A evidence

This directory preserves the authorized stock-60, offline, observation-only checkpoint performed on 2026-09-19.

Start with:

1. `PREFLIGHT.md` for executable/environment identity and the non-invasive boundary;
2. `PASS-A-RESULT.md` for findings and epistemic limits;
3. `sessions/20260919T083240Z-pid12832/derived/summary.json` for machine-readable metrics;
4. raw CSV files only when auditing a derived statement.

`analyze_pass_a.py` uses only the Python standard library. Re-run it with the session directory as its only argument. It deterministically replaces the small tables under `derived/`; it does not modify raw capture files.

`instrumentation/` contains the exact source, pinned dependency revision, build script, and binaries used. It is not a game mod and was never copied into the Dark Souls III directory.

`static-callback-targets/` contains only bounded read-only exports needed to interpret addresses that were actually observed at runtime.

`SHA256SUMS.txt` covers every retained file under this directory except itself, ignored object files, and PDBs.
