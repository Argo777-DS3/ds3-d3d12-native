# I-0005 runtime Pass A — stock-60 observation result

## Decision

**Pass A succeeded as an observation checkpoint.** The capture is complete, internally consistent, and sufficient to establish live cadence, scalar propagation, central-state relationships, presentation mode, selector activation, and the first dynamically reached callback targets.

I-0005 remains active. No timing value was changed, no game file was modified, and no 90/120 FPS run was performed. Any Pass A2 or Pass B intervention requires a separate authorization.

## Exact run

- Executable: `DarkSoulsIII.exe` 1.15.2.0, Steam BuildID `10167187`, SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Session: `20260919T083240Z-pid12832`.
- Mode: offline, stock 60 FPS behavior, observation-only external instrumentation.
- Captured broad-update interval: 239.092487 seconds.
- Clean probe stop: `COMPLETE` marker present; buffered CSV streams were flushed successfully.
- Game-directory impact: none.

## Findings

### CONFIRMED — the broad scalar is live and forwarded unchanged

`DS3-1152-F-00D57900`, `DS3-1152-F-00023410`, and `DS3-1152-F-000499C0` formed 14,222 ordered triples. All 14,222 triples carried the exact same float. Median end-to-end hook-observation latency was 5.5 microseconds.

The scalar distribution over the full capture was:

| Metric | Seconds |
|---|---:|
| minimum | 0.0166666005 |
| median | 0.0166666005 |
| p99 | 0.016669192048 |
| maximum | 0.0490661301 |
| mean | 0.0167164761043 |

This establishes a live, broadly distributed update delta on the observed stock run. It does not establish that every gameplay system consumes it or that it is the only engine timestep.

### SUPPORTED — central field `+0x268` is the strongest observed source relation

The immediately preceding central-state `after` snapshot field `+0x268` matched the next broad delta within `1e-7` for 14,067 of 14,221 aligned pairs (98.92%). Its median ratio to the broad delta was 1.0 and its Pearson correlation was 0.7969, stronger than the other sampled central fields.

The measured order was central update completion, then the following broad-update chain. This is strong provenance evidence, but it does not prove whether `+0x268` is the original arithmetic producer, a copied value, or one member of a related timing history.

### CONFIRMED — stock presentation used VSync forwarding, not the manual wait branch

Within the broad-update window:

- 14,221 calls reached `DS3-1152-F-0192CF00`;
- every `Present` used `SyncInterval=1` and returned success;
- every `DS3-1152-F-000D5E30` observation had `requested=1`, `active=0`, `mode=60`, and returned `1`;
- the median `Present` interval was 0.0166659 seconds.

Therefore I-0004's optional QPC deadline wait was inactive in this stock session. Presentation pacing and the broad engine-update delta remained distinct mechanisms even though their observed cadences were nearly one-to-one.

### CONFIRMED — the specialized selector used its dynamic entry

`DS3-1152-F-00DD4B60` was observed 8,644 times on one object. Every call selected index `0`, and output equaled the dynamic input within `1e-7` on all 8,644 calls. Over its own active span it ran at 58.89 Hz.

This confirms activation of the selector's dynamic branch during the captured sequence. It does not promote that selector to the global gameplay timestep.

### CONFIRMED — two virtual callback targets were dynamically reached

The first callback vector reached from `000499C0` contained only two target addresses during this run:

| Stable ID | Occurrences | Update rows | Objects | Bounded interpretation |
|---|---:|---:|---:|---|
| `DS3-1152-F-000EFDA0` | 871,628 | 9,242 | 344 | live transform/matrix update; later calling-convention reinspection confirms it forwards incoming `XMM1` to `002124A0` |
| `DS3-1152-F-000F0D40` | 53,932 | 8,896 | 14 | live callback that retains the delta and passes it to `DS3-1152-F-002124A0` |

`002124A0` accumulates the supplied value, selects a bounded interval/range, and computes a normalized/interpolated scalar. This proves live time-driven transform/interpolation consumers without establishing a high-level animation, physics, camera, or gameplay class name.

**Post-Pass-B correction:** the original Pass A reading said `000EFDA0` ignored the forwarded delta. That was incorrect. At `0x1400EFDB6` the function changes only `RCX`; incoming `XMM1` remains intact for the immediate call at `0x1400EFDC2`. Pass B direct instrumentation independently confirmed delta-sized elapsed increments in the helper when active.

The callback vector was empty for 4,979 broad-update rows and nonempty for 9,242. The interval from the first through last nonempty row is labeled only as a **gameplay-candidate window** for analysis; the label is methodological and not semantic proof of gameplay state.

## Additional bounded consumer

Static follow-up confirms an unconditional direct call from `000499C0` to `DS3-1152-F-0017BE20`, which forwards the delta to `DS3-1152-F-0021CC20`. The latter can accumulate elapsed time at `+0x4F0`, divide it by a duration at `+0x4F4`, clamp the result, and write progress at `+0x260` when its activation flag is set.

The call path is on the live distributor, but this capture did not instrument the internal activation flag. A live transition in this particular consumer is therefore not claimed.

## What Pass A establishes

- The `context+0x08` scalar is alive in normal play and is distributed at approximately one call per presented stock frame.
- Its value is overwhelmingly near `1/60` and is transmitted unchanged across the mapped chain.
- Central `+0x268` is the strongest measured preceding-field relation.
- The presentation path used `SyncInterval=1`; I-0004's manual QPC wait branch was inactive.
- The separate selector used its dynamic input, not its fixed `1/30` or `1/60` entries.
- Both live virtual callback targets forward the delta to a local interpolation helper; their bounded owners and activation patterns differ.

## What Pass A does not establish

- Causality between Present cadence and the engine delta.
- A universal gameplay timestep or complete coverage of animation, movement, camera, physics, or collision.
- Correctness or failure above 60 FPS.
- The causal reach of the historical `+0x354/+0x358` override.
- That changing one field is a safe framerate-independence patch.

## Stop boundary and next justified experiment

Pass A is complete. The next discriminating experiment is a separately authorized, reversible in-memory cadence comparison that preserves all logs and adds matched behavioral invariants at 60/90/120 FPS. A narrower Pass A2 would only be justified if the project first requires direct activation proof for `0021CC20` or broader callback coverage.

No such experiment has been started.

## Evidence map

- Raw session: `sessions/20260919T083240Z-pid12832/`
- Deterministic analysis: `analyze_pass_a.py`
- Derived summary: `sessions/20260919T083240Z-pid12832/derived/summary.json`
- Cadence: `sessions/20260919T083240Z-pid12832/derived/cadence-summary.csv`
- Central relationships: `sessions/20260919T083240Z-pid12832/derived/central-delta-relations.csv`
- Callback targets: `sessions/20260919T083240Z-pid12832/derived/callback-targets.csv`
- Selector: `sessions/20260919T083240Z-pid12832/derived/selector-summary.csv`
- Static callback exports: `static-callback-targets/`
- Instrumentation source and binaries: `instrumentation/`
