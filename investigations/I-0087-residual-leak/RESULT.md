# I-0087 result: the residual leak

Complete (2026-10-22). The residual slopes of I-0085 came from the Load Game menu's portraits. Each opening made offscreen
views whose render targets stayed in the texture ingress after their view was destroyed, each with a committed target and a
CPU descriptor. They are now retired with their view. A zone loaded eight times keeps its descriptors and video memory flat.

## Observations

1. **The counter.** `cpu_descriptors` counts the live leases of `AllocateCpuDescriptor` (`renderer/backend.cpp`). Those are
   held by the texture ingress's records (`NativeTextureRecord::view`), so a rising count means records that stay alive. The
   records come from engine textures, FaceGen faces, menu offscreen targets, the scene copy and the decal sources.
2. **The cause, read in the code** (step review agreed).
   - `NativeOffscreenRendererRetire` erased the destroyed view's proxies from `native_offscreen_proxies`, but it never retired
     their ingress records.
   - The UI's first import of a proxy had admitted a committed RGBA8 target with a descriptor (`AdmitOffscreenTarget`).
   - Such a record left only when a new view landed on the same proxy address (`16_offscreen_and_movies.inc`, the scene
     graph's `textures.Retire(proxy)`).
   - The tour opens Load Game at every visit.
3. **Fix.**
   - The renderer's retirement collects its holder's proxies and retires their records after the offscreen lock is released.
   - A UI image that imported one keeps its own reference until it is destroyed. The engine's destructor frees the proxy, so
     no later import finds it.
   - A `native_model_registry` census was added: texture records, render targets and their MB.
4. **Measured.** Beta m31 was loaded 8 times from the title in one process (`i87-beta-1`, build-1, measure mode).
   - At the title after visits 2 to 8: CPU descriptors 1,663 every time, local memory 1,619 MB every time, texture records
     863, render targets 10.
   - On I-0085's build, the gate's same leg (`i85-final-z1`) gave descriptors 1,057, 1,221, 1,224 and local memory 1,557,
     1,621, 1,623.
   - The private memory varied by ±40 MB around 7.79 GB after the second visit, with no fault.

## Conclusion

H1 and H2 hold. The descriptors and the local video memory no longer grow per visit. The private memory's remaining variation
is noise around a level, after the first visits' warm-up. The nightly endurance (D-0204) checks the five-zone loop.

## Uncertainty

- Measured on one zone's loop. The five-zone loop's own slope (+2 descriptors and +0.6 MB a visit in `i85-long-fix`) is
  expected flat, but that is not yet measured.
- A composed FaceGen face is not counted as a render target in the census.

## Reviews

- Step review (Opus 5.5): sound. One Medium was applied: the census comment now says composed faces are not flagged. The other
  Medium (the video memory explained only in part) was settled by the run: both slopes are flat.
- Final review (Opus 5.5): no High or Medium. Low items: the records were finished. A re-import after the view's destruction would show black, but the portraits stay shown at every Load Game opening (`i87-beta-1` shots, visit 6). The census reads a byte count admitted outside the lock (at worst 0 MB for a target in flight), and its MB is w x h x 4, not the allocation.
- Gate: `i87-final` PASS (2026-10-22 05:16). The mean of the two measure runs: frames over 34 ms 13.5, over 25 ms 96, held 47.5, game steps over 34 ms 1.5. The pictures were 0.55 and 0.63 from the reference. The memory leg gave 0 MB a visit.

## Artifacts

- `evidence/I-0066/artifacts/i87-beta-1`.
- Build `evidence/I-0087/artifacts/build-1`.
