# I-0005 Pass B reversible instrumentation

This probe extends the validated Pass A observation boundaries with one documented intervention: the central timing object's `+0x354` float and `+0x358` enable byte. It records the original values, applies the requested pair in memory, monitors drift, and restores the exact original values before disabling hooks.

No file in the Dark Souls III installation is written or replaced.

`pass_b_config.txt` controls one run:

- `target_fps`: central target written at `+0x354`;
- `enable_timing_override`: whether the pair is applied;
- `force_present_zero`: optional second-stage Present intervention; keep `0` until central-only runs prove it necessary;
- `callback_detail`: whether the full callback vector is serialized each update;
- `auto_stop_seconds`: bounded capture duration;
- `condition`: provenance label copied into `session-info.txt`.

Every transition is written to `override-transitions.csv`. A successful session must contain both `apply` and `restore` rows with `success=1`.

The final comparison build also aggregates three bounded consumers while the override is active:

- `000EFDA0 → 002124A0` local elapsed/output state;
- `000F0D40 → 002124A0` local elapsed/output state;
- `0021CC20` activation, elapsed `+0x4F0`, and progress `+0x260`.

`consumer-summary.csv` stores per-object call counts, delta totals, elapsed increments, resets, and local coordinate endpoints. These offsets are evidence-bounded fields, not high-level class names. Full callback-vector serialization is independently controlled by `callback_detail` because it is substantially heavier.
