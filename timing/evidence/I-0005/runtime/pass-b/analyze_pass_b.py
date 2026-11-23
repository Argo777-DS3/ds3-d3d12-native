#!/usr/bin/env python3
"""Deterministic per-session analysis for I-0005 runtime Pass B."""

from __future__ import annotations

import csv
import json
import math
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path


def percentile(values: list[float], q: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    position = (len(ordered) - 1) * q
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    return ordered[lower] * (upper - position) + ordered[upper] * (position - lower)


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
        "stdev": statistics.pstdev(values),
    }


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, rows: list[dict], fields: list[str]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def read_info(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            result[key] = value
    return result


def rate(qpcs: list[int], frequency: int) -> dict[str, float | int | None]:
    if len(qpcs) < 2:
        return {"count": len(qpcs), "span_s": 0.0, "rate_hz": None}
    span = (qpcs[-1] - qpcs[0]) / frequency
    return {"count": len(qpcs), "span_s": span,
            "rate_hz": (len(qpcs) - 1) / span if span > 0 else None}


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: analyze_pass_b.py <session-directory>")
    session = Path(sys.argv[1]).resolve()
    derived = session / "derived"
    derived.mkdir(exist_ok=True)

    info = read_info(session / "session-info.txt")
    frequency = int(info["qpc_frequency"])
    overrides = read_csv(session / "override-transitions.csv")
    applies = [row for row in overrides if row["action"] == "apply" and row["success"] == "1"]
    restores = [row for row in overrides if row["action"] == "restore" and row["success"] == "1"]
    if not applies or not restores:
        raise SystemExit("session lacks a successful apply/restore pair")
    active_start = int(applies[0]["qpc"])
    active_end = int(restores[-1]["qpc"])

    call_rows = read_csv(session / "call-sequence.csv")
    active_calls = [row for row in call_rows if active_start <= int(row["qpc"]) <= active_end]
    by_site = {site: [row for row in active_calls if row["site"] == site]
               for site in ("D57900", "023410", "0499C0")}
    updates = by_site["D57900"]
    update_qpcs = [int(row["qpc"]) for row in updates]
    deltas = [float(row["delta"]) for row in updates]

    triples = 0
    exact_triples = 0
    triple_latencies: list[float] = []
    for index in range(len(active_calls) - 2):
        left, middle, right = active_calls[index:index + 3]
        if [left["site"], middle["site"], right["site"]] == ["D57900", "023410", "0499C0"]:
            triples += 1
            values = [float(left["delta"]), float(middle["delta"]), float(right["delta"])]
            if values[0] == values[1] == values[2]:
                exact_triples += 1
            triple_latencies.append((int(right["qpc"]) - int(left["qpc"])) / frequency)

    presents_all = read_csv(session / "present.csv")
    presents = [row for row in presents_all if active_start <= int(row["qpc_before"]) <= active_end]
    present_qpcs = [int(row["qpc_before"]) for row in presents]
    present_intervals = [(right - left) / frequency for left, right in zip(present_qpcs, present_qpcs[1:])]
    present_durations = [(int(row["qpc_after"]) - int(row["qpc_before"])) / frequency
                         for row in presents]

    pacing_all = read_csv(session / "presentation-pacing.csv")
    pacing = [row for row in pacing_all if active_start <= int(row["qpc_before"]) <= active_end]
    pacing_modes = Counter((row["requested"], row["active"], row["mode"],
                            row["original_sync_interval"], row["returned_sync_interval"],
                            row["forced_zero"]) for row in pacing)

    central_all = read_csv(session / "central-state.csv")
    central = [row for row in central_all
               if active_start <= int(row["qpc"]) <= active_end and
               row["site"] == "EDAAE0" and row["phase"] == "after"]

    relation_rows: list[dict] = []
    mismatches: list[dict] = []
    central_index = 0
    aligned: list[tuple[dict[str, str], dict[str, str]]] = []
    for update in updates:
        uqpc = int(update["qpc"])
        while central_index + 1 < len(central) and int(central[central_index + 1]["qpc"]) < uqpc:
            central_index += 1
        if central and int(central[central_index]["qpc"]) < uqpc:
            aligned.append((update, central[central_index]))

    for field in ("f264", "f268", "f26c", "f2c4", "f2c8"):
        pairs = [(float(update["delta"]), float(snapshot[field])) for update, snapshot in aligned]
        differences = [left - right for left, right in pairs]
        exact = sum(abs(value) <= 1e-7 for value in differences)
        relation_rows.append({
            "field": field,
            "pairs": len(pairs),
            "exact_1e-7": exact,
            "exact_fraction": exact / len(pairs) if pairs else None,
            "mae": statistics.fmean(abs(value) for value in differences) if differences else None,
            "max_abs_error": max((abs(value) for value in differences), default=None),
        })
        if field == "f268":
            for update, snapshot in aligned:
                delta = float(update["delta"])
                value = float(snapshot[field])
                difference = delta - value
                if abs(difference) > 1e-7:
                    mismatches.append({
                        "update_seq": update["seq"],
                        "update_qpc": update["qpc"],
                        "central_seq": snapshot["seq"],
                        "central_qpc": snapshot["qpc"],
                        "delta": delta,
                        "f268": value,
                        "difference": difference,
                    })

    write_csv(derived / "central-relations.csv", relation_rows,
              ["field", "pairs", "exact_1e-7", "exact_fraction", "mae", "max_abs_error"])
    write_csv(derived / "f268-mismatches.csv", mismatches,
              ["update_seq", "update_qpc", "central_seq", "central_qpc", "delta", "f268", "difference"])

    selectors_all = read_csv(session / "selector.csv")
    selectors = [row for row in selectors_all if active_start <= int(row["qpc_before"]) <= active_end]
    selector_modes = Counter(row["selector"] for row in selectors)
    selector_exact = sum(abs(float(row["input"]) - float(row["after"])) <= 1e-7 for row in selectors)

    callbacks_path = session / "callbacks.csv"
    callback_rows = read_csv(callbacks_path) if callbacks_path.exists() else []
    callback_rows = [row for row in callback_rows if active_start <= int(row["qpc"]) <= active_end]
    target_occurrences: Counter[str] = Counter()
    target_rows: Counter[str] = Counter()
    target_objects: dict[str, set[str]] = defaultdict(set)
    target_first: dict[str, int] = {}
    target_last: dict[str, int] = {}
    for row in callback_rows:
        seen: set[str] = set()
        for entry in row["entries_index_object_target_rva"].split(";") if row["entries_index_object_target_rva"] else []:
            parts = entry.split(":")
            if len(parts) != 4 or parts[1] == "INVALID":
                continue
            _, object_address, _, target_rva = parts
            key = target_rva.lower()
            target_occurrences[key] += 1
            target_objects[key].add(object_address.lower())
            qpc = int(row["qpc"])
            target_first.setdefault(key, qpc)
            target_last[key] = qpc
            seen.add(key)
        for key in seen:
            target_rows[key] += 1

    callback_targets = []
    for target, count in target_occurrences.most_common():
        callback_targets.append({
            "target_rva": target,
            "stable_id": f"DS3-1152-F-{int(target, 16):08X}",
            "occurrences": count,
            "rows_present": target_rows[target],
            "unique_objects": len(target_objects[target]),
            "span_s": (target_last[target] - target_first[target]) / frequency,
            "rate_per_s": count / ((target_last[target] - target_first[target]) / frequency)
            if target_last[target] > target_first[target] else None,
        })
    write_csv(derived / "callback-targets.csv", callback_targets,
              ["target_rva", "stable_id", "occurrences", "rows_present", "unique_objects", "span_s", "rate_per_s"])

    consumer_path = session / "consumer-summary.csv"
    consumer_rows = read_csv(consumer_path) if consumer_path.exists() else []
    consumer_sites: dict[str, dict] = {}
    for site in ("EFDA0", "F0D40", "21CC20"):
        rows = [row for row in consumer_rows if row["site"] == site]
        calls = sum(int(row["calls"]) for row in rows)
        active_calls = sum(int(row["active_calls"]) for row in rows)
        increment_matches = sum(int(row["increment_matches"]) for row in rows)
        resets = sum(int(row["resets"]) for row in rows)
        sum_delta = sum(float(row["sum_delta"]) for row in rows)
        sum_elapsed_increment = sum(float(row["sum_elapsed_increment"]) for row in rows)
        moving_objects = sum(float(row["xyz_distance"]) > 1e-5 for row in rows)
        consumer_sites[site] = {
            "objects": len(rows),
            "calls": calls,
            "active_calls": active_calls,
            "active_fraction": active_calls / calls if calls else None,
            "sum_delta_s": sum_delta,
            "sum_elapsed_increment": sum_elapsed_increment,
            "increment_matches": increment_matches,
            "increment_match_fraction": increment_matches / calls if calls else None,
            "resets": resets,
            "moving_objects": moving_objects,
            "max_xyz_distance": max((float(row["xyz_distance"]) for row in rows), default=None),
        }

    summary = {
        "schema": "I-0005-pass-b-analysis-v1",
        "session": session.name,
        "condition": info.get("condition"),
        "configuration": {
            "target_fps": float(info["target_fps"]),
            "enable_timing_override": int(info["enable_timing_override"]),
            "force_present_zero": int(info["force_present_zero"]),
            "callback_detail": int(info["callback_detail"]),
        },
        "override": {
            "active_start_qpc": active_start,
            "active_end_qpc": active_end,
            "duration_s": (active_end - active_start) / frequency,
            "transitions": overrides,
        },
        "broad_update": {
            "cadence": rate(update_qpcs, frequency),
            "delta": describe(deltas),
            "chain_counts": {site: len(rows) for site, rows in by_site.items()},
            "ordered_triples": triples,
            "exact_delta_triples": exact_triples,
            "triple_latency_s": describe(triple_latencies),
        },
        "presentation": {
            "cadence": rate(present_qpcs, frequency),
            "interval_s": describe(present_intervals),
            "call_duration_s": describe(present_durations),
            "sync_intervals": dict(Counter(row["sync_interval"] for row in presents)),
            "results": dict(Counter(row["result"] for row in presents)),
        },
        "pacing": {
            "count": len(pacing),
            "modes": [
                {"requested": key[0], "active": key[1], "mode": key[2],
                 "original_sync_interval": key[3], "returned_sync_interval": key[4],
                 "forced_zero": key[5], "count": count}
                for key, count in pacing_modes.most_common()
            ],
        },
        "central": {
            "after_snapshots": len(central),
            "fps354": describe([float(row["fps354"]) for row in central]),
            "flag358": dict(Counter(row["flag358"] for row in central)),
            "relations": relation_rows,
            "f268_mismatch_count": len(mismatches),
        },
        "selector": {
            "count": len(selectors),
            "modes": dict(selector_modes),
            "input_output_exact_1e-7": selector_exact,
            "input": describe([float(row["input"]) for row in selectors]),
            "output": describe([float(row["after"]) for row in selectors]),
        },
        "callbacks": {
            "detail_rows": len(callback_rows),
            "detail_enabled": int(info["callback_detail"]),
            "unique_targets": len(callback_targets),
            "targets": callback_targets,
        },
        "consumers": {
            "aggregate_rows": len(consumer_rows),
            "sites": consumer_sites,
        },
    }

    (derived / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
