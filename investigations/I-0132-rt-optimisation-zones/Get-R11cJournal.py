"""Summarise DS3 native-renderer run journals. Usage: python -I Get-R11cJournal.py <run-folder> [...]
Mechanical counts and row excerpts only."""
import json, os, sys

FAULT = ("fault", "fatal", "refused", "latched", "crash", "exception", "failed", "error", "failure", "refusal")
SKIP = ("skip", "unsupported", "unknown", "missing")
SERVED = ("served", "unserved", "cause")
SERVED_EVENTS = ("native_rt_fail_magenta", "native_rt_last_scene", "native_rt_reuse_stopped")

def cut(line, n):
    line = line.strip()
    return line if len(line) <= n else line[:n] + "..."

def summarise(folder):
    name = os.path.basename(os.path.normpath(folder))
    out = []
    flags = "(baseline.json missing)"
    try:
        with open(os.path.join(folder, "baseline.json"), encoding="utf-8-sig") as f:
            b = json.load(f)
        flags = b.get("effect_flags", "(no effect_flags field)")
    except Exception as e:
        flags = "(baseline.json unreadable: %s)" % e
    rows = []; bad = 0; total = 0
    try:
        with open(os.path.join(folder, "startup.jsonl"), encoding="utf-8", errors="replace") as f:
            for line in f:
                if not line.strip():
                    continue
                total += 1
                try:
                    o = json.loads(line)
                    if not isinstance(o, dict): raise ValueError
                except Exception:
                    bad += 1; continue
                rows.append((str(o.get("event", "")), o, line))
    except OSError as e:
        out.append("run %s: startup.jsonl unreadable: %s" % (name, e)); return out
    out.append("== run %s | effect_flags=%s | lines=%d unparsable=%d" % (name, flags, total, bad))
    def group(match):
        d = {}
        for ev, o, line in rows:
            if any(k in ev for k in match):
                c = d.setdefault(ev, [0, line]); c[0] += 1
        return d
    def last(ev):
        r = None
        for e, o, line in rows:
            if e == ev: r = (o, line)
        return r
    d = group(FAULT)
    out.append("faults/closes: " + ("none" if not d else ""))
    for ev, (c, first) in sorted(d.items()):
        out.append("  %s x%d first: %s" % (ev, c, cut(first, 300)))
    # I-0132 R11c (the final review's L8): a traced pass turned off says so in its own row (`"on":false` and its `why`), not in its name.
    off = {}
    for ev, o, line in rows:
        if ev.startswith("native_rt_") and o.get("on") is False: off[ev] = line
    out.append("passes off: " + ("none" if not off else ""))
    for ev, line in sorted(off.items()):
        out.append("  %s last: %s" % (ev, cut(line, 300)))
    d = group(SKIP)
    out.append("skipped: " + ("none" if not d else ""))
    for ev, (c, first) in sorted(d.items()):
        out.append("  %s x%d first: %s" % (ev, c, cut(first, 300)))
    r = last("native_rt_blas")
    if r:
        o = r[0]
        out.append("rt_blas last: " + " ".join("%s=%s" % (k, o.get(k, "n/a")) for k in
            ("models", "triangles", "built_mb", "compacted_mb", "live_placed_mb", "tlas_mb", "failed", "pending")))
    else:
        out.append("rt_blas last: no row")
    arr = [l for e, o, l in rows if e == "native_rt_arrival"]
    out.append("rt_arrival rows: %d" % len(arr))
    for l in arr: out.append("  " + cut(l, 400))
    out.append("rt_gates rows: %d" % sum(1 for e, o, l in rows if e == "native_rt_gates"))
    vals = {}
    for e, o, l in rows:
        if e == "native_rt_gates" or e in SERVED_EVENTS:
            for k, v in o.items():
                if any(s in k for s in SERVED):
                    vals.setdefault((e, k), set()).add(json.dumps(v))
    if vals:
        for (e, k), s in sorted(vals.items()):
            out.append("  %s.%s distinct: %s" % (e, k, cut(" ".join(sorted(s)), 200)))
    else:
        out.append("  served/unserved/cause fields: none found")
    for e in SERVED_EVENTS:
        out.append("  %s rows: %d" % (e, sum(1 for x, o, l in rows if x == e)))
    for ev, n in (("native_rt_materials", 600), ("native_rt_family_names", 600), ("native_rt_lights", 300),
                  ("native_taa_frames", 300), ("native_dlss_mode", 300), ("native_rt_level", 300), ("native_auto_exposure", 300)):
        r = last(ev)
        out.append("%s last: %s" % (ev, cut(r[1], n) if r else "no row"))
    mx = None
    for e, o, l in rows:
        v = o.get("frame")
        if isinstance(v, (int, float)) and not isinstance(v, bool) and (mx is None or v > mx): mx = v
    out.append("max frame: %s | native_rt_walk_failed rows: %d" % (mx, sum(1 for e, o, l in rows if e == "native_rt_walk_failed")))
    return out

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    for p in sys.argv[1:]:
        print("\n".join(summarise(p))); print()
