"""I-0141 tranche 13: the type-0x898 effect objects' state 3 (1AD71A0) in both modes, on the scene's own clock.

Reads two run journals (startup.jsonl) made by Run-I141SmokePix.ps1 -Dump -Light -ArmMs <ms> -Life with the ffx_state3 rows, one
native and one in the original (Reference), and prints for each mode:
  - the rows (exit: the object left state 3, its state returned; open: still in state 3 at its last call when the rows closed
    at 45 s of the scene clock, or when another object was born at its address), and the window: from the first ceremony FXR
    (ffx_scene_pre) to the close (ffx_state3_end: the lives open then, the rows left, the rows the cap refused);
  - for the FXR ids given (all by default), the exits by the cause the inputs point to:
      duration  the duration evaluated at the last call <= the age before it,
      move      the FXR state machine moved in that call (the inputs and the duration are those before the move),
      flag      the flag 0x400 set and the flag 2 set, or an end asked (instance flags bit 3, or the machine's index >= its count)
                with +0x18 null (« flag? » when +0x18 is set: its type, 2000 or 2001 needed, is not in the row),
      spawner   neither (the spawner +0xC0 null or its vt+0x18 false),
    and the open objects with their last age and duration (a duration not finite never ends the state by (a));
  - the objects first seen before the scene clock's first second (before the clock included: the first instances), one line each.
Usage: python -I Compare-I141State3.py <native startup.jsonl> <original startup.jsonl> [fxr ...]
"""
import json
import math
import sys
from collections import Counter, defaultdict


def rows(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if '"event":"ffx_state3' not in line and '"event":"ffx_scene_' not in line:
                continue
            try:
                yield json.loads(line)
            except json.JSONDecodeError:
                pass


def num(v):
    if v is None:
        return None
    if isinstance(v, str):
        return {"inf": math.inf, "-inf": -math.inf}.get(v, math.nan)
    return float(v)


def cause(x):
    age = num(x["age"][1])
    dur = num(x["duration"][1])
    flags = int(x["flags"][1], 16)
    inst = x["instance_flags"][1]
    machine = x["machine"]
    asked = (inst is not None and inst & 8) or (machine[0] and machine[2] >= machine[4])
    if x.get("moves", [0, 0])[1]:
        return "move"
    if dur is not None and not math.isnan(dur) and dur <= age:
        return "duration"
    if flags & 0x400 and flags & 2:
        return "flag"
    if flags & 0x400 and asked:
        # also needs +0x18 null or of type 2000 or 2001, which the row does not carry (its vtable only)
        return "flag" if not x["o18"][1] else "flag?"
    if not x["oc0"][1]:
        return "spawner null"
    return "spawner"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    focus = {int(a) for a in sys.argv[3:]}
    for name, path in (("native", sys.argv[1]), ("original", sys.argv[2])):
        allrows = list(rows(path))
        clock = [x["ms"] for x in allrows if x["event"] == "ffx_scene_clock"][:1]
        pre = [x["ms"] for x in allrows if x["event"] == "ffx_scene_pre"][:1]
        end = [x for x in allrows if x["event"] == "ffx_state3_end"][:1]
        if not clock:
            n = sum(1 for x in allrows if x["event"] == "ffx_state3")
            print(f"{name}: no scene clock row ({'ffx_scene_pre at ' + str(pre[0]) + ' ms, ' if pre else 'no ffx_scene_pre, '}{n} ffx_state3 rows): "
                  f"the window opened but never closed" if pre else f"{name}: no scene clock row and no window")
            continue
        zero = clock[0]
        every = [x for x in allrows if x["event"] == "ffx_state3"]
        for x in every:   # QPC milliseconds to the scene clock
            x["first"] -= zero
            x["last"] -= zero
        closed = (f"at {round((end[0]['ms'] - zero) / 1000, 1)} s, open {end[0].get('open', '?')}, rows left {end[0]['rows_left']}, "
                  f"refused by the cap {end[0].get('dropped', '?')}") if end else "no close row (no call after 45 s, or the run ended before: the lives open then unwritten)"
        print(f"{name}: rows from {(pre[0] - zero) / 1000 if pre else 'none'} s on the scene clock; closed {closed}")
        xs = [x for x in every if not focus or x["fxr"] in focus]
        what = Counter(x["what"] for x in xs)
        print(f"{name}: {len(xs)} rows ({', '.join(f'{k} {v}' for k, v in sorted(what.items()))})")
        if len(every) >= 20000 or (end and end[0].get("dropped")):
            print(f"  WARNING {name}: the rows' cap reached")
        if end and "open" in end[0]:
            print(f"  open rows {sum(1 for x in every if x['what'] == 'open')} (at the close {end[0]['open']} lives; " + ("the cap refused rows: the split is not known)" if end[0].get("dropped") else "the others at a reused address)"))
        by = defaultdict(Counter)
        for x in xs:
            if x["what"] == "exit":
                by[x["fxr"]][(cause(x), x["result"])] += 1
            else:
                dur = num(x["duration"][1])
                by[x["fxr"]][("open, duration " + ("none" if dur is None else "not finite" if not math.isfinite(dur) else "finite"), 3)] += 1
        for fxr in sorted(by):
            print(f"  fxr {fxr}: " + ", ".join(f"{c} (state {r}) {n}" for (c, r), n in sorted(by[fxr].items())))
        early = sorted((x for x in xs if x["first"] < 1000), key=lambda x: (x["fxr"], x["last"]))
        print(f"  first seen before the clock's first second: {len(early)}")
        for x in early[:40]:
            print(f"    fxr {x['fxr']} {x['what']} first {x['first']:.0f} last {x['last']:.0f} ms state {x['result']} ({cause(x) if x['what'] == 'exit' else '-'}); age {x['age']},"
                  f" duration {x['duration']}, flags {x['flags']}, instance {x['instance_flags']}, machine {x['machine']}, b8 {x['b8']},"
                  f" o18 {x['o18']}, oc0 {x['oc0']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
