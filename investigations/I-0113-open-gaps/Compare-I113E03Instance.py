"""I-0113 tranche 16 (the crystals' hue): a model particle's whole cbInstanceData block, native against original.

Reads native_e03_instance (ffx-record.flag) and reference_e03_instance (ffx-draw-observe.flag) rows with their « all » words
(0x44 floats) from two run journals, and prints per model and per 16-byte row the words' range on each side, marking the rows
whose ranges do not meet. Usage: python -I Compare-I113E03Instance.py <native startup.jsonl> <original startup.jsonl> [model prefix]
"""
import json
import sys

ROWS = {6: "envData/dynamicLightDisable", 7: "mulCol", 8: "aSh0", 9: "aSh1", 10: "aSh2", 11: "ShLightMask",
        13: "env blend", 14: "env scale 0", 15: "env scale 1", 16: "ditherFadeAlpha"}


def load(path, event, prefix):
    blocks = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if event not in line or '"all"' not in line:
                continue
            try:
                row = json.loads(line)
            except ValueError:
                continue
            if row.get("event") != event or not row.get("model", "").startswith(prefix):
                continue
            blocks.setdefault(row["model"], []).append(row["all"])
    return blocks


def ranges(blocks, word):
    values = [b[word] for b in blocks if word < len(b) and b[word] is not None]
    return (min(values), max(values)) if values else None


def main():
    native = load(sys.argv[1], "native_e03_instance", sys.argv[3] if len(sys.argv) > 3 else "")
    original = load(sys.argv[2], "reference_e03_instance", sys.argv[3] if len(sys.argv) > 3 else "")
    for model in sorted(set(native) | set(original)):
        n, o = native.get(model, []), original.get(model, [])
        print(f"== {model}: native {len(n)} blocks, original {len(o)} blocks")
        for r in range(0x44 // 4):
            cells, differs = [], False
            for w in range(4 * r, 4 * r + 4):
                a, b = ranges(n, w), ranges(o, w)
                if a and b and (a[1] < b[0] - 1e-4 or b[1] < a[0] - 1e-4):
                    differs = True
                fmt = lambda x: "-" if not x else (f"{x[0]:.4g}" if abs(x[1] - x[0]) < 1e-6 else f"{x[0]:.4g}..{x[1]:.4g}")
                cells.append(f"{fmt(a)} | {fmt(b)}")
            print(f"{'*' if differs else ' '} row {r:2d} {ROWS.get(r, ''):28s} " + "  ;  ".join(cells))


if __name__ == "__main__":
    main()
