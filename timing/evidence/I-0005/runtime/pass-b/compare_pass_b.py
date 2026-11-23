#!/usr/bin/env python3
"""Build deterministic cross-condition tables for I-0005 Pass B."""

from __future__ import annotations

import csv
import json
import sys
from pathlib import Path


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def write_csv(path: Path, rows: list[dict], fields: list[str]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def relation(summary: dict, field: str) -> dict:
    return next(row for row in summary["central"]["relations"] if row["field"] == field)


def main() -> None:
    if len(sys.argv) < 3:
        raise SystemExit("usage: compare_pass_b.py <output-directory> <summary.json> [...]")
    output = Path(sys.argv[1]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    summaries = [load(Path(argument).resolve()) for argument in sys.argv[2:]]
    summaries.sort(key=lambda item: float(item["configuration"]["target_fps"]))

    conditions: list[dict] = []
    consumers: list[dict] = []
    for summary in summaries:
        target = float(summary["configuration"]["target_fps"])
        update = summary["broad_update"]
        presentation = summary["presentation"]
        pacing_modes = summary["pacing"]["modes"]
        sync_zero = int(presentation["sync_intervals"].get("0", 0))
        sync_total = sum(int(value) for value in presentation["sync_intervals"].values())
        manual_wait = sum(int(row["count"]) for row in pacing_modes if row["active"] != "0")
        conditions.append({
            "target_fps": target,
            "session": summary["session"],
            "duration_s": summary["override"]["duration_s"],
            "update_count": update["cadence"]["count"],
            "update_rate_hz": update["cadence"]["rate_hz"],
            "update_target_fraction": update["cadence"]["rate_hz"] / target,
            "delta_median_s": update["delta"]["p50"],
            "delta_expected_s": 1.0 / target,
            "delta_median_error_s": update["delta"]["p50"] - 1.0 / target,
            "present_count": presentation["cadence"]["count"],
            "present_rate_hz": presentation["cadence"]["rate_hz"],
            "present_target_fraction": presentation["cadence"]["rate_hz"] / target,
            "present_interval_median_s": presentation["interval_s"]["p50"],
            "sync_zero_fraction": sync_zero / sync_total if sync_total else None,
            "manual_qpc_wait_calls": manual_wait,
            "selector_calls": summary["selector"]["count"],
            "selector_modes": json.dumps(summary["selector"]["modes"], sort_keys=True),
            "selector_exact_fraction": summary["selector"]["input_output_exact_1e-7"] /
                                       summary["selector"]["count"] if summary["selector"]["count"] else None,
            "f268_exact_fraction": relation(summary, "f268")["exact_fraction"],
            "triple_latency_median_us": update["triple_latency_s"]["p50"] * 1_000_000,
            "present_call_median_us": presentation["call_duration_s"]["p50"] * 1_000_000,
        })

        updates = int(update["cadence"]["count"])
        duration = float(summary["override"]["duration_s"])
        for site, values in summary["consumers"]["sites"].items():
            calls = int(values["calls"])
            active_calls = int(values["active_calls"])
            matches = int(values["increment_matches"])
            resets = int(values["resets"])
            consumers.append({
                "target_fps": target,
                "session": summary["session"],
                "site": site,
                "objects": values["objects"],
                "calls": calls,
                "calls_per_update": calls / updates if updates else None,
                "calls_per_second": calls / duration if duration else None,
                "mean_input_delta_s": values["sum_delta_s"] / calls if calls else None,
                "active_calls": active_calls,
                "active_fraction": values["active_fraction"],
                "increment_matches": matches,
                "resets": resets,
                "active_accounted_fraction": (matches + resets) / active_calls if active_calls else None,
                "sum_elapsed_increment_s": values["sum_elapsed_increment"],
                "moving_objects": values["moving_objects"],
            })

    condition_fields = [
        "target_fps", "session", "duration_s", "update_count", "update_rate_hz",
        "update_target_fraction", "delta_median_s", "delta_expected_s",
        "delta_median_error_s", "present_count", "present_rate_hz",
        "present_target_fraction", "present_interval_median_s", "sync_zero_fraction",
        "manual_qpc_wait_calls", "selector_calls", "selector_modes",
        "selector_exact_fraction", "f268_exact_fraction", "triple_latency_median_us",
        "present_call_median_us",
    ]
    consumer_fields = [
        "target_fps", "session", "site", "objects", "calls", "calls_per_update",
        "calls_per_second", "mean_input_delta_s", "active_calls", "active_fraction",
        "increment_matches", "resets", "active_accounted_fraction",
        "sum_elapsed_increment_s", "moving_objects",
    ]
    write_csv(output / "condition-summary.csv", conditions, condition_fields)
    write_csv(output / "consumer-site-summary.csv", consumers, consumer_fields)
    (output / "comparison.json").write_text(
        json.dumps({"schema": "I-0005-pass-b-comparison-v1",
                    "conditions": conditions, "consumers": consumers}, indent=2) + "\n",
        encoding="utf-8",
    )
    print(json.dumps({"conditions": len(conditions), "consumer_rows": len(consumers)}, indent=2))


if __name__ == "__main__":
    main()
