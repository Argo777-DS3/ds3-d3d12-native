"""I-0141 tranche 13b: the FXR state machines of the ceremony's effects (141ACA640) in both modes, on the scene's own clock.

Reads two run journals (startup.jsonl) made by Run-I141SmokePix.ps1 -Dump -Light -ArmMs <ms> -Life with the ffx_machine rows, one
native and one in the original (Reference), and prints for each mode and FXR id (all of 732000..732999 and 4040 by default):
  - the window (ffx_scene_pre, the state-3 rows' close); the machines seen, those that never moved, and every move (the scene time,
    the index before -> after);
  - for each distinct state (FXR, index, its conditions' shape), the conditions once: operator, next index, both operands (kind:
    FFFC constant, FFFD external value, FFFE camera distance, FFFF state time; the constant or the key; the value evaluated) at the
    machine's first step, at its last row and at each move, and the instance's external values (key, value) at those rows;
  - the external-value tables put on those instances (ffx_ext_set, 141AB1A60): per FXR, table, creation or not and caller chain, the
    scene times, and the close (ffx_ext_set_end: the rows the cap refused). A caller reaching 141A87DF0 by a tail jump through the
    thunk 140244A50 (1404D2AC0, the handle stop) never shows: the frame after 1A87xxx is then its own caller. A « creation » row
    is a call inside the creation of that FXR (frame 0 1ACCC05). A table shown « truncated » has more entries than the row holds.
Usage: python -I Compare-I141Machine.py <native startup.jsonl> <original startup.jsonl> [fxr ...]
"""
import json
import sys
from collections import Counter, defaultdict

KIND = {"FFFC": "const", "FFFD": "ext", "FFFE": "camdist", "FFFF": "time"}
OPS = {0: "==", 1: "!=", 2: "<", 3: "<=", 4: "==", 5: "!=", 6: "<", 7: "<=", 8: "==", 9: "!=", 10: "<", 11: "<="}


def rows(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if '"event":"ffx_machine"' not in line and '"event":"ffx_ext_set' not in line and '"event":"ffx_scene_' not in line and '"event":"ffx_state3_end"' not in line:
                continue
            try:
                yield json.loads(line)
            except json.JSONDecodeError:
                pass


def operand(o):
    kind, key, bits, value = o
    k = KIND.get(kind, kind)
    what = f"key {int(key, 16)}" if k == "ext" else f"{value:g}" if k == "const" else ""
    return f"{k}({what})={value:g}|{bits}" if what else f"{k}={value:g}|{bits}"


def show(x):
    conds = "; ".join(f"[{c['op']}] {operand(c['left'])} {OPS.get(c['op'], '?')} {operand(c['right'])} -> {c['next']}" for c in x.get("conditions", []))
    ext = ", ".join(f"{k}:{v:g}|{b}" for k, b, v in x.get("external", []))
    cut = f" (6 of {x['n']} conditions)" if x.get("n", 0) > 6 else ""
    cut += f" (8 of {x['external_n']} external values)" if x.get("external_n", 0) > 8 else ""
    return f"index {x['index'][0]}->{x['index'][1]} of {x.get('count', '?')}{cut}, time {x['time']:.3f}, dt {x['dt']:.4f}; {conds or 'no condition'}; external {ext or 'none'}"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    focus = {int(a) for a in sys.argv[3:]}
    for name, path in (("native", sys.argv[1]), ("original", sys.argv[2])):
        allrows = list(rows(path))
        clock = [x["ms"] for x in allrows if x["event"] == "ffx_scene_clock"][:1]
        zero = clock[0] if clock else 0.0
        pre = [x["ms"] for x in allrows if x["event"] == "ffx_scene_pre"][:1]
        end = [x["ms"] for x in allrows if x["event"] == "ffx_state3_end"][:1]
        if not clock:
            print(f"{name}: NO scene clock (732035 never spawned): the machine rows were never closed and their times below are QPC seconds")
        else:
            print(f"{name}: window from {round((pre[0] - zero) / 1000, 2) if pre else 'NONE (no ffx_scene_pre)'} s to 45 s of the clock (the machine "
                  f"rows' own close; the state-3 rows closed {'at ' + str(round((end[0] - zero) / 1000, 1)) + ' s' if end else 'never'})")
        xs = [x for x in allrows if x["event"] == "ffx_machine" and (not focus or x["fxr"] in focus)]
        print(f"{name}: {len(xs)} ffx_machine rows{'' if clock else ' (no scene clock: QPC ms)'}; why {dict(Counter(x['why'] for x in xs))}")
        if sum(1 for x in allrows if x["event"] == "ffx_machine") >= 6000:
            print(f"  WARNING {name}: the rows' cap reached")
        by = defaultdict(list)
        for x in xs:
            by[(x["fxr"], x["machine"], x["instance"])].append(x)
        per_fxr = defaultdict(lambda: [0, 0])
        for (fxr, _, _), life in by.items():
            per_fxr[fxr][0] += 1
            per_fxr[fxr][1] += any(r["index"][0] != r["index"][1] for r in life)   # a « first » row may carry a move too
        moves = defaultdict(list)
        for (fxr, _, _), life in by.items():
            moves[fxr] += [(round((r["ms"] - zero) / 1000, 2), r["index"][0], r["index"][1]) for r in life if r["index"][0] != r["index"][1]]
        for fxr in sorted(per_fxr):
            m = sorted(moves[fxr])
            print(f"  fxr {fxr}: machines {per_fxr[fxr][0]}, moved {per_fxr[fxr][1]}, never moved {per_fxr[fxr][0] - per_fxr[fxr][1]}; moves "
                  f"(s, from, to): {', '.join(f'{t} {a}->{b}' for t, a, b in m)}")
        shown = set()
        for (fxr, machine, instance), life in sorted(by.items(), key=lambda kv: (kv[0][0], kv[1][0]["ms"])):
            for x in life:
                if x["why"] == "every" and x is not life[-1]:
                    continue
                shape = (fxr, x["index"][0], x["index"][1], x["why"], tuple((c["op"], c["left"][0], c["left"][1], c["right"][0], c["right"][1], c["next"]) for c in x.get("conditions", [])))
                if shape in shown:   # one line per state shape, move and reason (every move's time is in the per-FXR list above)
                    continue
                shown.add(shape)
                print(f"    fxr {fxr} {x['why']:5} {(x['ms'] - zero) / 1000:8.3f} s machine {machine}: {show(x)}")
        sets = defaultdict(list)
        for x in allrows:
            if x["event"] == "ffx_ext_set" and (not focus or x["fxr"] in focus):
                n = x.get("entries_n", 0)
                entries = ("null" if x.get("table") == "0" else "unreadable") if n < 0 else tuple(tuple(e[:2]) for e in x["entries"]) + ((("truncated", n),) if len(x["entries"]) != n else ())
                sets[(x["fxr"], entries, x.get("creation", 0),
                      tuple(x["callers"]))].append(round((x["ms"] - zero) / 1000, 2))
        close = [x for x in allrows if x["event"] == "ffx_ext_set_end"][:1]
        print(f"  ffx_ext_set: {sum(len(v) for v in sets.values())} rows; " + (f"closed at {round((close[0]['ms'] - zero) / 1000, 1) if 'ms' in close[0] else '?'} s, rows left {max(0, close[0]['rows_left'])}, refused by the cap {close[0]['dropped']}" if close else "no close row"))
        if (close and close[0]["dropped"]) or sum(1 for x in allrows if x["event"] == "ffx_ext_set") >= 2000:
            print(f"  WARNING {name}: the ffx_ext_set cap refused rows")
        for (fxr, entries, creation, callers), times in sorted(sets.items(), key=lambda kv: (kv[0][0], kv[1][0])):
            print(f"    fxr {fxr}{' creation' if creation else ''} table {entries if entries == 'unreadable' else list(entries)} callers {' < '.join(callers)}: {len(times)} at {times[:30]}{' ...' if len(times) > 30 else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
