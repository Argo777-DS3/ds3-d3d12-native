# Native Bink movie player (menus workstream, 2026-10-11, branch `claude/bink-native` from main 1a463ec9)

Specification: I-0068 part J [BINK-NATIVE-READY](../I-0068-static-yebis-ffx-lights-spec/BINK-NATIVE-READY.md) (checks K1-K8),
general note I-0069 [BINK-MOVIES-NATIVE](../I-0069-static-menu-audio-map/BINK-MOVIES-NATIVE.md) (witnesses BW1-BW4, rows BL1-BL5).

## What is in the build

- **Removed:** the explicit omission `NativeMoviePlayerFactory` (1171). The original factory `141ADE670` runs.
- **CPU as the original:** Bink's decode, its four decode threads, its pacing (`BinkWait`, `BinkShouldSkip`) and its own
  audio (XAudio2, or DirectSound). Bink is called through the game's own import cells; each cell is checked once to
  resolve into `binkw64.dll`.
- **Transcribed without the old device `[144961AB0]`** (Native1171's fault at `EAF8F6`):
  - the steps `EAF7E0` (the player init receives a native device token that nothing dereferences) and `EAFAC0` (the
    render receives no GX context);
  - the player's open `1ADF230`: every Bink call and path helper as the original. The GX output texture, view and
    target become one native output token in `+0x30/+0x38/+0x40`.
  - the player's close `1ADE850`: retires the native output and the proxy's target;
  - the player's render `1ADEAF0`: without the GX state save and restore;
  - the publish `EADE50`: the proxy's GX view slot stays empty (no GX reader of the proxy is reached natively).
- **Native graphics object:**
  - init `1AE0830`: checks the game's programmes (DXBC cells `1446CAD40`, `1446CB4C8`) and the unit quad;
  - planes `1AE0BA0`: Bink's frame buffers as the original, with no GX plane textures;
  - draw `1ADFF10`: copies the ready set's Y/cR/cB rows on the game thread with the 9 x float4 block of the original.
    The latest frame is queued.
- **Presentation (`RenderNativeMovie`):**
  - R8 planes and the game's VS and 3-plane PS;
  - root CBV b0, SRV table t0-t3 (null t3), and a static clamped `MIN_MAG_LINEAR_MIP_POINT` sampler;
  - a triangle strip with no blend and no depth;
  - the frame is drawn into the native R8G8B8A8 target, which `EAF7E0` admits for the movie proxy (`CSMovieGxTexture`)
    at open, black until the first frame. This happens before the menu pass samples the target.
- **Witnesses:**
  - both modes: `movie_witness` rows for open (header, colour block, plane buffers), first frame, every 240th frame and
    close;
  - native: `native_movie_open`, `_planes`, `_target`, `_published`, `_frame`, `_close`, and the frame pixels at Bink
    frames 240 and 2400 (`movie-<n>.rgba`);
  - pilot: `Run-PilotPair.ps1 -Pilot CharMake` runs New Game with `Pilot-CharMakeWitness.ps1 -MovieShots`
    (screenshots 10, 60 and 150 s after the movie screen opens).

## UI imports bound as is (found by Native1250)

- The movie screen imports its image again on every update (`E4E790` looks the name up in `E48980`'s cache, misses,
  and imports through `E49320`). Each native import or retirement copies the whole UI texture set
  (`PrimitiveTextureStore` Define and Commit), with the CPU bytes of every image in it.
- Native1250 played the movie at 10 frames per second, with 8 MB per copy for the movie target alone. Native1251
  reached 14 frames per second after the render targets stopped carrying CPU bytes.
- Since 1252, every imported asset whose resource matches its content's description (Texture2D, size, mips, format) is
  bound as is. These are resident native resources, in the PIXEL | NON_PIXEL shader-resource state since their upload
  or their target admission.
- Such an image keeps one empty mip entry per level: `ValidPrimitiveTexture` accepts an empty entry only for an
  external texture.
- Result: the UI texture set no longer copies image bytes, and imported assets are no longer uploaded again by the
  primitive pass.

## Live (2026-10-11, New Game on Thomas's save, `Run-PilotPair.ps1 -Pilot CharMake`)

| Check | Reference1250 | Native1250 | Native1252 (after the import fix) |
|---|---|---|---|
| K1 open: header, buffers | 1920x1080, 4750 frames, 24/1, 2 sets, luma 1920x1080, chroma 960x544, no alpha | same, 16 colour floats identical | same |
| K2 first frame | first render after open, Bink frame 2 | same | same |
| K4 duration (open to close) | 197.875 s | 198.218 s (2035 drawn, 2714 skipped by `BinkShouldSkip`) | **197.875 s, as the original; 4749 drawn, 0 skipped**, 60 frames per second |
| K5 natural end | close at frame count 4750, `ChrMake_Top` 16 ms later | same order | same |
| K3 image | screenshot at 60 s | same picture and subtitle as the original | pixels at Bink frames 240 and 2400 (`movie-240.png`) |
| K7 audio | `flags` 0x4400 (sound track set, 1 track: mode 1 stereo) | same | same |
| K8 old device | — | no fault at `EAF8F6`, no refusal | same; 0 D3D12 debug errors on 124 checked frames |

- Both modes end on the known close fault at `D12759` (window close during Create Character, also in 1173), unrelated
  to the movie.
- The Create Character screen after the movie matches the original (same icons and texts).

## Open

- K6 (witnessed, no longer open), skip by key: **Esc skips the movie in both modes** (Reference1256 / Native1256, pilot `-Pilot CharMake -ConfirmKey 27`,
  Esc held 200 ms ten seconds into the movie). The close comes 11.8 s (original) and 11.9 s (native) after open, and
  `ChrMake_Top` opens 32 ms (original) and 31 ms (native) later. Natively 286 frames were drawn, and the output and target
  were retired. Clicks, Enter and E do not skip it (1173 pilot). Esc as the cause is inferred from timing (no trace row records
  the press; the close follows the hold within 2 s in both modes). Static: `FUN_140B60850` runs the five checks first when its
  flag argument (`+0x9D9` per BINK-NATIVE-READY) is set, then command 0x54.

- The credits movie (BW2) needs a save at the end of the game.
- The proxy's GX view slot stays empty natively. No CPU reader of it was reached in these runs.

## Pre-merge review (fresh Opus 5.5 agent, high effort, D-0163)

- **Blocking:** nothing. Every transcription matches its decompiled original: control flow, layouts, AL returns,
  Bink cells and the constant block. Nothing dereferences the old device or the native tokens, and every called helper
  is pinned.
- **Locking:** the game thread never takes the movie lock while it admits or retires the target, so there is no
  reverse nesting.
- **UI import change:** sound. The resident states match, and the byte readers skip external textures.
- **Fixed:**
  - one cached RTV heap instead of one per movie frame;
  - the movie pixel witnesses only for the process's first movie;
  - a comment on the mirrored multisample flag.
- **Recorded, not changed:**
  - the plane vectors and programme copies of each frame (about 3 MB on the game thread, measured fine);
  - the proxy's GX fields left empty (see Open);
  - no unit case for `ValidPrimitiveTexture` with a real external resource (it needs a device; covered live by
    Native1252).

K6 pre-merge review (fresh Opus 5.5 agent, D-0163): nothing blocking; wording fixes applied (check order of FUN_140B60850, Esc inferred from timing).
