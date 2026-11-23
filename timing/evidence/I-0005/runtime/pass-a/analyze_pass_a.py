#!/usr/bin/env python3
"""Deterministic analysis for the I-0005 stock-60 observation pass."""

from __future__ import annotations

import argparse
import bisect
import csv
import json
import math
import statistics
from collections import Counter, defaultdict
from pathlib import Path


def percentile(values: list[float], probability: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    position = (len(ordered) - 1) * probability
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    weight = position - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def describe(values: list[float]) -> dict[str, float | int | None]:
    if not values:
        return {"count": 0, "min": None, "p01": None, "p50": None, "p99": None,
                "max": None, "mean": None, "stdev": None}
    return {
        "count": len(values),
        "min": min(values),
        "p01": percentile(values, 0.01),
        "p50": percentile(values, 0.50),
        "p99": percentile(values, 0.99),
        "max": max(values),
        "mean": statistics.fmean(values),
        "stdev": statistics.pstdev(values) if len(values) > 1 else 0.0,
    }


def correlation(left: list[float], right: list[float]) -> float | None:
    if len(left) != len(right) or len(left) < 2:
        return None
    mean_left = statistics.fmean(left)
    mean_right = statistics.fmean(right)
    numerator = sum((a - mean_left) * (b - mean_right) for a, b in zip(left, right))
    denominator = math.sqrt(
        sum((a - mean_left) ** 2 for a in left) * sum((b - mean_right) ** 2 for b in right)
    )
    return numerator / denominator if denominator else None


def read_frequency(path: Path) -> int:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return int(values["qpc_frequency"])


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, rows: list[dict], fieldnames: list[str]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("session", type=Path)
    args = parser.parse_args()
    session = args.session.resolve()
    derived = session / "derived"
    derived.mkdir(exist_ok=True)

    frequency = read_frequency(session / "session-info.txt")

    calls = load_csv(session / "call-sequence.csv")
    call_by_site: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in calls:
        call_by_site[row["site"]].append(row)

    call_summary: list[dict] = []
    for site, rows in sorted(call_by_site.items()):
        qpcs = [int(row["qpc"]) for row in rows]
        deltas = [float(row["delta"]) for row in rows]
        duration = (max(qpcs) - min(qpcs)) / frequency if len(qpcs) > 1 else 0.0
        stats = describe(deltas)
        call_summary.append({
            "site": site,
            "count": len(rows),
            "threads": len({row["tid"] for row in rows}),
            "first_qpc": min(qpcs),
            "last_qpc": max(qpcs),
            "duration_s": duration,
            "rate_hz": (len(rows) - 1) / duration if duration > 0 else None,
            **{f"delta_{key}": value for key, value in stats.items() if key != "count"},
        })
    write_csv(derived / "call-site-summary.csv", call_summary, list(call_summary[0].keys()))

    triples = 0
    exact_triples = 0
    triple_latencies: list[float] = []
    for index in range(len(calls) - 2):
        first, second, third = calls[index:index + 3]
        if [first["site"], second["site"], third["site"]] != ["D57900", "023410", "0499C0"]:
            continue
        if not (first["tid"] == second["tid"] == third["tid"]):
            continue
        triples += 1
        values = [float(first["delta"]), float(second["delta"]), float(third["delta"])]
        if values[0] == values[1] == values[2]:
            exact_triples += 1
        triple_latencies.append((int(third["qpc"]) - int(first["qpc"])) / frequency)

    update_rows = call_by_site.get("D57900", [])
    update_qpcs = [int(row["qpc"]) for row in update_rows]
    update_deltas = [float(row["delta"]) for row in update_rows]
    active_start = min(update_qpcs)
    active_end = max(update_qpcs)
    active_duration = (active_end - active_start) / frequency

    presents = load_csv(session / "present.csv")
    present_qpcs = [int(row["qpc_before"]) for row in presents]
    active_presents = [row for row in presents if active_start <= int(row["qpc_before"]) <= active_end]
    active_present_qpcs = [int(row["qpc_before"]) for row in active_presents]
    present_intervals = [
        (right - left) / frequency for left, right in zip(active_present_qpcs, active_present_qpcs[1:])
    ]
    present_durations = [
        (int(row["qpc_after"]) - int(row["qpc_before"])) / frequency for row in active_presents
    ]
    sync_intervals = Counter(row["sync_interval"] for row in active_presents)
    present_results = Counter(row["result"] for row in active_presents)

    pacing = load_csv(session / "presentation-pacing.csv")
    active_pacing = [row for row in pacing if active_start <= int(row["qpc_before"]) <= active_end]
    pacing_durations = [
        (int(row["qpc_after"]) - int(row["qpc_before"])) / frequency for row in active_pacing
    ]
    pacing_modes = Counter(
        (row["requested"], row["active"], row["mode"], row["returned_sync_interval"])
        for row in active_pacing
    )

    central = load_csv(session / "central-state.csv")
    central_after = [
        row for row in central
        if row["phase"] == "after" and active_start <= int(row["qpc"]) <= active_end
    ]
    central_after.sort(key=lambda row: int(row["qpc"]))
    central_qpcs = [int(row["qpc"]) for row in central_after]
    float_fields = [name for name in central[0] if name.startswith("f") and len(name) in (4, 5)]

    aligned_deltas: list[float] = []
    aligned_fields: dict[str, list[float]] = {field: [] for field in float_fields}
    alignment_lag: list[float] = []
    for update in update_rows:
        qpc = int(update["qpc"])
        index = bisect.bisect_right(central_qpcs, qpc) - 1
        if index < 0:
            continue
        state = central_after[index]
        aligned_deltas.append(float(update["delta"]))
        alignment_lag.append((qpc - central_qpcs[index]) / frequency)
        for field in float_fields:
            aligned_fields[field].append(float(state[field]))

    relation_rows: list[dict] = []
    for field, values in aligned_fields.items():
        differences = [abs(a - b) for a, b in zip(aligned_deltas, values)]
        ratios = [a / b for a, b in zip(aligned_deltas, values) if abs(b) > 1e-12]
        relation_rows.append({
            "field": field,
            "pairs": len(values),
            "pearson": correlation(aligned_deltas, values),
            "mae": statistics.fmean(differences) if differences else None,
            "max_abs_error": max(differences) if differences else None,
            "exact_1e-7": sum(diff <= 1e-7 for diff in differences),
            "median_delta_over_field": percentile(ratios, 0.5),
        })
    relation_rows.sort(key=lambda row: (math.inf if row["mae"] is None else row["mae"]))
    write_csv(derived / "central-delta-relations.csv", relation_rows, list(relation_rows[0].keys()))

    central_field_rows: list[dict] = []
    for field in float_fields:
        values = [float(row[field]) for row in central_after]
        stats = describe(values)
        central_field_rows.append({"field": field, **stats})
    write_csv(derived / "central-field-summary.csv", central_field_rows, list(central_field_rows[0].keys()))

    selectors = load_csv(session / "selector.csv")
    active_selectors = [row for row in selectors if active_start <= int(row["qpc_before"]) <= active_end]
    selector_groups: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in active_selectors:
        selector_groups[(row["selector"], row["object"])].append(row)
    selector_rows: list[dict] = []
    for (selector, object_address), rows in sorted(selector_groups.items()):
        inputs = [float(row["input"]) for row in rows]
        outputs = [float(row["after"]) for row in rows]
        errors = [abs(left - right) for left, right in zip(inputs, outputs)]
        selector_rows.append({
            "selector": selector,
            "object": object_address,
            "count": len(rows),
            "input_min": min(inputs),
            "input_p50": percentile(inputs, 0.5),
            "input_max": max(inputs),
            "output_min": min(outputs),
            "output_p50": percentile(outputs, 0.5),
            "output_max": max(outputs),
            "input_output_exact_1e-7": sum(error <= 1e-7 for error in errors),
        })
    write_csv(derived / "selector-summary.csv", selector_rows, list(selector_rows[0].keys()))

    callbacks_path = session / "callbacks.csv"
    callback_rows = 0
    callback_nonempty_rows = 0
    callback_nonempty_qpcs: list[int] = []
    callback_nonempty_deltas: list[float] = []
    callback_count_distribution: Counter[int] = Counter()
    target_occurrences: Counter[str] = Counter()
    target_rows: Counter[str] = Counter()
    target_objects: dict[str, set[str]] = defaultdict(set)
    target_first_qpc: dict[str, int] = {}
    target_last_qpc: dict[str, int] = {}
    with callbacks_path.open("r", encoding="utf-8-sig", newline="") as stream:
        for row in csv.DictReader(stream):
            qpc = int(row["qpc"])
            if not (active_start <= qpc <= active_end):
                continue
            callback_rows += 1
            count = int(row["count"])
            callback_count_distribution[count] += 1
            if count:
                callback_nonempty_rows += 1
                callback_nonempty_qpcs.append(qpc)
                callback_nonempty_deltas.append(float(row["delta"]))
            seen_in_row: set[str] = set()
            entries = row["entries_index_object_target_rva"]
            for entry in entries.split(";") if entries else []:
                parts = entry.split(":")
                if len(parts) != 4 or parts[1] == "INVALID":
                    continue
                _, object_address, target_address, target_rva = parts
                key = target_rva.lower()
                target_occurrences[key] += 1
                target_objects[key].add(object_address.lower())
                target_first_qpc.setdefault(key, qpc)
                target_last_qpc[key] = qpc
                seen_in_row.add(key)
            for key in seen_in_row:
                target_rows[key] += 1

    callback_target_rows: list[dict] = []
    for target, count in target_occurrences.most_common():
        span = (target_last_qpc[target] - target_first_qpc[target]) / frequency
        callback_target_rows.append({
            "target_rva": target,
            "stable_id": f"DS3-1152-F-{int(target, 16):08X}" if int(target, 16) <= 0xFFFFFFFF else "outside_main_image",
            "occurrences": count,
            "rows_present": target_rows[target],
            "share_of_update_rows": target_rows[target] / callback_rows if callback_rows else None,
            "unique_objects": len(target_objects[target]),
            "first_qpc": target_first_qpc[target],
            "last_qpc": target_last_qpc[target],
            "observed_span_s": span,
        })
    callback_fields = [
        "target_rva", "stable_id", "occurrences", "rows_present", "share_of_update_rows",
        "unique_objects", "first_qpc", "last_qpc", "observed_span_s",
    ]
    write_csv(derived / "callback-targets.csv", callback_target_rows, callback_fields)

    callback_count_rows = [
        {"callbacks_per_update": count, "update_rows": rows}
        for count, rows in sorted(callback_count_distribution.items())
    ]
    write_csv(
        derived / "callback-count-distribution.csv",
        callback_count_rows,
        ["callbacks_per_update", "update_rows"],
    )

    gameplay_start = min(callback_nonempty_qpcs)
    gameplay_end = max(callback_nonempty_qpcs)
    gameplay_duration = (gameplay_end - gameplay_start) / frequency
    gameplay_presents = [
        row for row in presents if gameplay_start <= int(row["qpc_before"]) <= gameplay_end
    ]
    gameplay_present_qpcs = [int(row["qpc_before"]) for row in gameplay_presents]
    gameplay_present_intervals = [
        (right - left) / frequency
        for left, right in zip(gameplay_present_qpcs, gameplay_present_qpcs[1:])
    ]

    selector_qpcs = [int(row["qpc_before"]) for row in active_selectors]
    selector_span = (max(selector_qpcs) - min(selector_qpcs)) / frequency if len(selector_qpcs) > 1 else 0.0

    cadence_rows = [
        {
            "stream": "D57900 broad update",
            "count": len(update_rows),
            "window_s": active_duration,
            "rate_hz": (len(update_rows) - 1) / active_duration,
        },
        {
            "stream": "0192CF00 Present in broad-update window",
            "count": len(active_presents),
            "window_s": active_duration,
            "rate_hz": (len(active_presents) - 1) / active_duration,
        },
        {
            "stream": "central after snapshots in broad-update window",
            "count": len(central_after),
            "window_s": active_duration,
            "rate_hz": (len(central_after) - 1) / active_duration,
        },
        {
            "stream": "DD4B60 selector in broad-update window",
            "count": len(active_selectors),
            "window_s": active_duration,
            "rate_hz": (len(active_selectors) - 1) / active_duration,
        },
        {
            "stream": "nonempty callback-vector updates in own span",
            "count": callback_nonempty_rows,
            "window_s": gameplay_duration,
            "rate_hz": (callback_nonempty_rows - 1) / gameplay_duration,
        },
        {
            "stream": "0192CF00 Present in nonempty-callback span",
            "count": len(gameplay_presents),
            "window_s": gameplay_duration,
            "rate_hz": (len(gameplay_presents) - 1) / gameplay_duration,
        },
        {
            "stream": "DD4B60 selector in own span",
            "count": len(active_selectors),
            "window_s": selector_span,
            "rate_hz": (len(active_selectors) - 1) / selector_span if selector_span > 0 else None,
        },
    ]
    write_csv(derived / "cadence-summary.csv", cadence_rows, list(cadence_rows[0].keys()))

    modules_start = (session / "modules-start.csv").read_text(encoding="utf-8-sig").lower()
    modules_stop = (session / "modules-stop.csv").read_text(encoding="utf-8-sig").lower()
    forbidden_terms = ["reshade", "nue-", "rtss", "rivatuner", "specialk", "gamebar", "nvidia overlay"]

    summary = {
        "schema": "I-0005-pass-a-analysis-v1",
        "session": session.name,
        "qpc_frequency": frequency,
        "active_window": {
            "first_qpc": active_start,
            "last_qpc": active_end,
            "duration_s": active_duration,
        },
        "broad_update": {
            "count": len(update_rows),
            "delta": describe(update_deltas),
            "chain_triples": triples,
            "chain_exact_delta_triples": exact_triples,
            "chain_latency_s": describe(triple_latencies),
        },
        "presentation": {
            "total_session_count": len(presents),
            "active_window_count": len(active_presents),
            "active_window_interval_s": describe(present_intervals),
            "active_window_call_duration_s": describe(present_durations),
            "sync_intervals": dict(sync_intervals),
            "results": dict(present_results),
        },
        "pacing": {
            "active_window_count": len(active_pacing),
            "call_duration_s": describe(pacing_durations),
            "modes": [
                {
                    "requested": key[0], "active": key[1], "mode": key[2],
                    "returned_sync_interval": key[3], "count": value,
                }
                for key, value in pacing_modes.most_common()
            ],
        },
        "central": {
            "after_snapshot_count": len(central_after),
            "alignment_lag_s": describe(alignment_lag),
            "best_delta_relations": relation_rows[:10],
        },
        "selector": {
            "active_window_count": len(active_selectors),
            "selectors": dict(Counter(row["selector"] for row in active_selectors)),
            "objects": len({row["object"] for row in active_selectors}),
        },
        "callbacks": {
            "update_rows": callback_rows,
            "nonempty_update_rows": callback_nonempty_rows,
            "count_distribution": dict(sorted(callback_count_distribution.items())),
            "unique_targets": len(callback_target_rows),
            "top_targets": callback_target_rows[:20],
        },
        "gameplay_candidate_window": {
            "definition": "first through last broad-update row with a nonempty virtual callback vector",
            "first_qpc": gameplay_start,
            "last_qpc": gameplay_end,
            "duration_s": gameplay_duration,
            "update_count": callback_nonempty_rows,
            "delta": describe(callback_nonempty_deltas),
            "present_count": len(gameplay_presents),
            "present_interval_s": describe(gameplay_present_intervals),
        },
        "modules": {
            "forbidden_terms": forbidden_terms,
            "matches_at_start": [term for term in forbidden_terms if term in modules_start],
            "matches_at_stop": [term for term in forbidden_terms if term in modules_stop],
            "steam_overlay_at_start": "gameoverlayrenderer64.dll" in modules_start,
            "steam_overlay_at_stop": "gameoverlayrenderer64.dll" in modules_stop,
            "probe_at_start": "i0005passa.dll" in modules_start,
            "probe_at_stop": "i0005passa.dll" in modules_stop,
        },
    }
    (derived / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
