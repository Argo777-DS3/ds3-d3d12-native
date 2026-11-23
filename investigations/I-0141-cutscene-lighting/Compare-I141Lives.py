"""I-0141 tranche 12: the ceremony's effect entities in both modes on the scene's own clock.

Reads two run journals (startup.jsonl) made by Run-I141SmokePix.ps1 -Dump -Light -ArmMs <ms> -Life, one native and one in the
original (Reference), and prints:
  - the scene clock's start (the first creation of FXR 732035) and the press made at -ArmMs on that clock;
  - the entities offered at the press (rows ffx_entity_offered), distinct, by kind and first mask;
  - each second of the scene, the entities offered within the last 100 ms by kind and first mask (ffx_entity_life_count);
  - the lives (ffx_entity_life): births a 5 s window and the length of each life (last offer - first offer), by kind and first
    mask, for the lives born after the clock's first second (the ones alive at its start have no known birth);
  - the emitter nodes' state (rows with "node", I-0141 tranche 12): groups by birth window, offers, counts and objects, and the
    latest-ending lives alive at the clock's start.
Usage: python -I Compare-I141Lives.py <native startup.jsonl> <original startup.jsonl> [kind mask ...]
"""
import json
import statistics
import sys
from collections import Counter, defaultdict


def rows(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if '"event":"ffx_' not in line:
                continue
            try:
                yield json.loads(line)
            except json.JSONDecodeError:
                pass


def load(path):
    out = defaultdict(list)
    for r in rows(path):
        e = r.get("event", "")
        if e in ("ffx_scene_clock", "ffx_scene_press", "ffx_entity_offered", "ffx_entity_life", "ffx_entity_life_count", "ffx_entity_life_end", "ffx_instance_flag"):
            out[e].append(r)
    return out


def quartiles(values):
    if not values:
        return "-"
    v = sorted(values)
    q = lambda p: v[min(len(v) - 1, int(p * (len(v) - 1) + 0.5))]
    return f"n {len(v)}, min {v[0]:.0f}, q1 {q(0.25):.0f}, median {q(0.5):.0f}, q3 {q(0.75):.0f}, max {v[-1]:.0f} ms"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    runs = {"native": load(sys.argv[1]), "original": load(sys.argv[2])}
    focus = [(int(sys.argv[i]), sys.argv[i + 1].upper()) for i in range(3, len(sys.argv) - 1, 2)] or [(3, "4"), (6, "4"), (3, "8"), (3, "FFFFFFFF")]
    for name, r in runs.items():
        clock = r["ffx_scene_clock"][:1]
        press = r["ffx_scene_press"][:1]
        end = r["ffx_entity_life_end"][:1]
        print(f"{name}: clock {clock[0]['ms'] if clock else 'none'}; press {press[0]['scene_ms'] if press else 'none'} ms; "
              f"end {end[0]['scene_ms'] if end else 'none'} ms, rows left {end[0]['rows_left'] if end else '-'}; "
              f"life rows {len(r['ffx_entity_life'])}, count rows {len(r['ffx_entity_life_count'])}")
        if end and end[0]["rows_left"] <= 0 or not end and len(r["ffx_entity_life"]) >= 30000:
            print(f"  WARNING {name}: the life rows' cap reached: its late lives are missing, births not comparable past its last row")
        reborn = sum(1 for x in r["ffx_entity_life"] if x.get("end") == "reborn")
        gaps = [c.get("gap_max", 0) for c in r["ffx_entity_life_count"]]
        print(f"  {name}: lives ended as reborn (an address reused, or a node's new particle array) {reborn}; the longest gap between offers {max(gaps) if gaps else 0:.0f} ms")
    print("\nAt the press: distinct entities offered (and offers) by kind and first mask")
    keys = set()
    tallies = {}
    for name, r in runs.items():
        ent = defaultdict(set)
        offers = Counter()
        for o in r["ffx_entity_offered"]:
            k = (o["kind"], o["masks"][0])
            ent[k].add(o.get("entity"))
            offers[k] += 1
        tallies[name] = (ent, offers)
        keys |= set(ent)
    for k in sorted(keys):
        cells = [f"{len(tallies[n][0].get(k, ()))} ({tallies[n][1].get(k, 0)})" for n in runs]
        print(f"  kind {k[0]} mask {k[1]}: native {cells[0]}, original {cells[1]}")
    print("\nAlive (offered in the last 100 ms) each second of the scene clock")
    series = {}
    for name, r in runs.items():
        s = {}
        for c in r["ffx_entity_life_count"]:
            sec = int(c["scene_ms"] // 1000)
            if sec in s:
                continue
            s[sec] = {(a[0], a[1]): a[2] for a in c["alive"]}
        series[name] = s
    head = "  s   " + "  ".join(f"k{k} m{m}: nat/orig" for k, m in focus)
    print(head)
    for sec in range(0, 91):
        if sec not in series["native"] and sec not in series["original"]:
            continue
        cells = []
        for k in focus:
            a = series["native"].get(sec, {}).get(k)
            b = series["original"].get(sec, {}).get(k)
            cells.append(f"{'-' if a is None else a:>4}/{'-' if b is None else b:<4}")
        print(f"  {sec:>3} " + "  ".join(f"{c:>17}" for c in cells))
    print("\nLives born after the clock's first second, by kind and first mask")
    for k in focus:
        print(f"  kind {k[0]} mask {k[1]}:")
        for name, r in runs.items():
            lives = [x for x in r["ffx_entity_life"] if (x["kind"], x["masks"][0]) == k and x["first"] >= 1000]
            closed = [x["last"] - x["first"] for x in lives if not x["open"]]
            births = Counter(int(x["first"] // 5000) * 5 for x in lives)
            print(f"    {name}: lives {len(lives)} (open at the end {sum(1 for x in lives if x['open'])}); length {quartiles(closed)}")
            print(f"      births a 5 s window: " + ", ".join(f"{w}s {births[w]}" for w in sorted(births)))
    print("\nEmitter nodes' state (rows with \"node\": count at birth, last, max, capacity, bits at birth, last;"
          " bits 1 renderer, 2 +0x98, 4 +0xA0, 8 particle array)")
    for k in focus:
        print(f"  kind {k[0]} mask {k[1]}:")
        for name, r in runs.items():
            lives = [x for x in r["ffx_entity_life"] if (x["kind"], x["masks"][0]) == k and "node" in x]
            if not lives:
                print(f"    {name}: no node rows")
                continue
            groups = Counter()
            for x in lives:
                n = x["node"]
                groups[("born before 1 s" if x["first"] < 1000 else "born after", "one offer" if x["offers"] == 1 else "several",
                        "count 0 at first" if n[0] == 0 else "count>0 at first", "last 0" if n[1] == 0 else "last>0",
                        "max 0" if n[2] == 0 else "max>0", f"bits {n[4]}->{n[5]}")] += 1
            print(f"    {name}: {len(lives)} node lives")
            for g, c in groups.most_common(12):
                print(f"      {c:>5}  " + ", ".join(g))
            early = [x for x in lives if x["first"] < 1000]
            for x in sorted(early, key=lambda x: x["last"])[-8:]:
                print(f"      alive at the clock's start: last {x['last']:.0f} ms, offers {x['offers']}, node {x['node']}, "
                      f"owner {x.get('owner', '-')}, pos {x['pos']}")
            heads = [x for x in lives if "owner" in x]
            if heads:
                # owner = [state at the first read, state at the last offer, age, the age the end counts from, flags]
                ends = Counter((x["owner"][1], x["end"]) for x in heads)
                print(f"      heads {len(heads)}; the owner's state at the last offer and the life's end: "
                      + ", ".join(f"state {s} {e} {c}" for (s, e), c in sorted(ends.items())))
    print("\nInstance stops (ffx_instance_flag: stop = bit 4, every object deleted at the next update; bit8 = bit 8, a soft stop only from 1A87C60), by FXR id and callers")
    for name, r in runs.items():
        rows_ = r["ffx_instance_flag"]
        print(f"  {name}: {len(rows_)} rows")
        by = defaultdict(list)
        for x in rows_:
            by[(x["what"], x["fxr"], tuple(x["callers"]))].append(x)
        for (what, fxr, callers), xs in sorted(by.items(), key=lambda kv: kv[1][0]["scene_ms"]):
            times = ", ".join(f"{x['scene_ms'] / 1000:.2f}" for x in xs[:10])
            print(f"    {what} fxr {fxr}: {len(xs)} at s {times}{' ...' if len(xs) > 10 else ''}; callers {'/'.join(callers)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
