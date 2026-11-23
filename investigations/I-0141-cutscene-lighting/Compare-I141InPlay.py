"""I-0141 tranche 15: the effect entities in play (after the ceremony) in both modes.

Reads two run journals (startup.jsonl) of Run-I141SmokePix.ps1 -Dump -Light -Life (one native, one original) and prints:
  - for each second s of the scene clock from --from, the row ffx_entity_life_count the module writes at the second's first offer
    (at or after s*1000 ms): the entities offered in the 100 ms before that offer, by kind and first mask, as (min, median, max)
    over the seconds both journals have a row for (D-0299: unequal seconds not compared; a second without any offer has no row;
    the writer makes at most one row a second, so a journal holding two, a reset clock or two runs, is refused), a row without the
    key counted 0: the entities present together. A row whose gap_max (the longest gap between two offers since the row before) is
    over 100 ms may undercount: those seconds are named (a row below that may still miss an entity offered early in a long walk);
  - only when both journals hold their close (ffx_entity_life_end; the lives still held at the close are written only then, end
    « open », so a journal without it lacks them and the table is left out, said): the lives whose offers [first, last] overlap
    at least one compared second, grouped by their world position rounded to --grid units and their kind, with the emitter nodes'
    particle counts (the last and the largest of each life, summed): lives that follow one another in a place add up, so these are
    not amounts present at once. It warns when the life rows ran out (rows_left 0). Lives whose position was not read (0, 0, 0)
    are counted apart.
Each journal's rows' mode tag is checked (native, then reference: the tag says whether the native effect draw ran, so a Native run
with no-ffx-draw.flag is tagged reference), and a journal without any of these rows is refused. Rows that do not parse are counted.
Usage: python -I Compare-I141InPlay.py <native startup.jsonl> <original startup.jsonl> [--from 60000] [--grid 5]
"""
import argparse
import json
import sys
from collections import defaultdict

CLOSE_MS = 90000.0


def read(path):
    lives, counts, end, modes, bad = [], [], None, set(), 0
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if '"ffx_entity_life' not in line:
                continue
            try:
                r = json.loads(line)
            except json.JSONDecodeError:
                bad += 1
                continue
            modes.add(r.get("mode"))
            e = r.get("event")
            if e == "ffx_entity_life":
                lives.append(r)
            elif e == "ffx_entity_life_count":
                counts.append(r)
            elif e == "ffx_entity_life_end":
                end = r
    return lives, counts, end, modes, bad


def place(r, grid):
    return tuple(int(round(v / grid)) * grid for v in r["pos"])


def main():
    a = argparse.ArgumentParser()
    a.add_argument("native")
    a.add_argument("original")
    a.add_argument("--from", dest="start", type=float, default=60000.0)
    a.add_argument("--grid", type=float, default=5.0)
    o = a.parse_args()
    if o.start >= CLOSE_MS:
        sys.exit(f"--from {o.start:.0f} is at or after the module's close ({CLOSE_MS:.0f} ms of the scene clock): no row there")
    data, windows, closed = {}, {}, {}
    for name, path, mode in (("native", o.native, "native"), ("original", o.original, "reference")):
        lives, counts, end, modes, bad = read(path)
        if not lives and not counts:
            sys.exit(f"{path}: no ffx_entity_life rows (a run without ffx-entity-life.flag?)")
        if modes - {mode}:
            sys.exit(f"{path}: rows tagged {sorted(repr(m) for m in modes)} where {mode!r} is expected (arguments swapped, or the"
                     f" native effect draw off?)")
        print(f"{name}: life rows {len(lives)}, count rows {len(counts)}, malformed {bad}")
        if end is None:
            print(f"  WARNING: no close (ffx_entity_life_end): the lives still held at the journal's end are not in the lives table")
        elif end.get("rows_left", 1) <= 0:
            print(f"  WARNING: the life rows ran out (rows_left {end.get('rows_left')}): the lives table may be incomplete")
        data[name] = lives
        closed[name] = end is not None
        w = {}
        for c in counts:
            if c.get("scene_ms", -1) >= o.start:
                w.setdefault(int(c["scene_ms"] // 1000), []).append(c)
        twice = sorted(s for s, rows in w.items() if len(rows) > 1)
        if twice:
            sys.exit(f"{path}: seconds with more than one count row {twice} (the writer makes one a second: a reset scene clock or"
                     f" two runs in one journal)")
        windows[name] = {s: rows[0] for s, rows in w.items()}
    common = sorted(set(windows["native"]) & set(windows["original"]))
    if not common:
        sys.exit(f"no second of the scene clock from {o.start:.0f} ms has a count row in both journals")
    keys = {(kind, mask) for n in windows for s in common for kind, mask, _ in windows[n][s].get("alive", [])}
    per_second = {}
    for name in windows:
        gaps = [s for s in common if windows[name][s].get("gap_max", 0) > 100]
        if gaps:
            print(f"  {name}: {len(gaps)} of the compared seconds' rows had a gap over 100 ms between two offers since the row"
                  f" before (may undercount): {gaps}")
        per_second[name] = {k: [{(kd, m): v for kd, m, v in windows[name][s].get("alive", [])}.get(k, 0) for s in common]
                            for k in keys}

    def stat(v):
        v = sorted(v)
        return f"({v[0]}, {v[len(v) // 2]}, {v[-1]})"

    print(f"\nentities present together, each second both journals have from {o.start:.0f} ms, by kind and first mask (min, median,"
          f" max): the {len(common)} seconds {common[0]} to {common[-1]} s of the scene clock (from --from, the native journal has"
          f" {len(windows['native'])} seconds, the original {len(windows['original'])}): native | original")
    for k in sorted(keys):
        print(f"  kind {k[0]} mask {k[1]}: {stat(per_second['native'][k])} | {stat(per_second['original'][k])}")
    if not all(closed.values()):
        print("\nlives table left out: a journal without its close lacks the lives still held at its end, so the two sides would not"
              " cover the same lives (D-0299)")
        return
    groups = {}
    for name, lives in data.items():
        late = [r for r in lives if any(r.get("first", 1e12) < (s + 1) * 1000.0 and r.get("last", -1) >= s * 1000.0 for s in common)]
        g = defaultdict(lambda: [0, 0, 0, 0])   # lives, emitter nodes, summed last counts, summed largest counts
        unplaced = 0
        for r in late:
            if all(v == 0 for v in r["pos"]):
                unplaced += 1
                continue
            k = (place(r, o.grid), r["kind"])
            g[k][0] += 1
            node = r.get("node")
            if node:
                g[k][1] += 1
                g[k][2] += node[1]
                g[k][3] += node[2]
        print(f"{name}: lives offered in the compared seconds {len(late)}"
              + (f", without a read position (0, 0, 0), left out of the table: {unplaced}" if unplaced else ""))
        groups[name] = g
    nat, org = groups["native"], groups["original"]
    keys = sorted(set(nat) | set(org), key=lambda k: (k in nat and k in org, k))
    print(f"\nlives offered in the compared seconds (their offers overlap at least one of them), by place"
          f" (grid {o.grid:g}) and kind: native [lives, emitter nodes, summed last particles, summed largest] | original [...]"
          f" (not amounts present at once)")
    for k in keys:
        mark = "both" if k in nat and k in org else ("native only" if k in nat else "original only")
        print(f"  {k[0]} kind {k[1]}: {nat.get(k, '-')} | {org.get(k, '-')}  {mark}")
    tot = {n: [sum(v[i] for v in groups[n].values()) for i in range(4)] for n in groups}
    print(f"\ntotals: native {tot['native']} | original {tot['original']}")

if __name__ == "__main__":
    main()
