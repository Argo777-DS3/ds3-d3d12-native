# I-0068 part AM, point 4: programme fingerprints for matching draws in a capture

Request (coordinator, 2026-10-11): programme hashes among the static reductions, so a live capture (original or native) is matched to a programme name without reflection. NPC-FAMILIES-STATIC §9 already gives 17 programmes; this part gives the effect, glow, velocity, shadow, light, decal and post families, and the tooling for the rest.

Labels: **STATIC** (the loose DXBC programmes of the I-0063 cache `C:/Users/User/.codex/I0063-cache/programs`, extracted from the game archives, 11,641 blobs). Nothing launched.

## 1. Keys and how to use them

Two keys per blob, both computed from the file bytes:

- **SHA-256** (16 first hex digits here, full value in the TSV): the hash of the whole DXBC container;
- **DXBC checksum** (8 first hex digits here): the 16 bytes at offset 4 of the DXBC header, i.e. the "shader hash" that capture tools (RenderDoc, GPA) show. Use it first when the capture tool shows a hash; use the SHA-256 when the capture exports bytes.

If a capture identifies programmes by another hash, match by byte size and by the resource list (`fam-reflect.tsv`, `ffx-program-layouts.tsv`).

**Files** (`evidence/I-0068/queries/fingerprints/`, external store; regenerate with `Get-ProgramFingerprints.ps1`):

| File | Content |
|---|---|
| `programs.tsv` | all 11,641 blobs: bundle, program, ext, stage, bytes, sha256, dxbc |
| `ffx-index-hashes.tsv` | the 158 FFX shader-set indices: VS name and hashes, PS name and file (the GlowD twin for indices 75..127) and hashes, hull/domain hashes of the three tessellation patterns where files exist |

```
pwsh -NoProfile -File investigations/I-0068-static-yebis-ffx-lights-spec/Get-ProgramFingerprints.ps1 -ShaderSet evidence/I-0068/queries/ffx/ffx-shader-set.tsv -Out evidence/I-0068/queries/fingerprints
```

**Facts the table shows that the notes did not state:**

- several shader-set indices share one blob: the glow twin of a VS is the same blob as its base (only the pixel programme changes to `…GlowD`), and the `Multi*` indices 41-56 and 116-131 share one VS blob (`BEE2B1C115A98570`, the same for `GXFfxtessellateMultiTexture` and `…SoftMultiTexture`) with eight distinct pixel programmes (`MultiTexture`, `SoftMultiTexture`, their `2Sampler` and `Blend…` variants): a draw is identified by its **pixel** programme, not by the VS, for E01 multi-texture;
- `GXFfxtessellateLineColorOnly` and `GXFfxtessellateQuadLineColorOnly` share one pixel programme (`B0731EF604107CDD`);
- the point-sprite pixel programme is shared between the non-tessellated and tessellated variants (`3B008E249D7DF53F`);
- hull/domain: the billboard families (`Texture`, `Bump`, `Soft`, `SoftBump`) have the three patterns `_quad`, `_octed`, `_quincunx`; `MultiTexture`/`SoftMultiTexture` only `_quad` and `_octed`; the tracer, line and point-sprite programmes have **one** hull and domain under their own name (`ffx-index-hashes.tsv` column `ths_ds_variants`: `single`). Indices with no hull/domain file (non-tessellated, distortion, radial blur, light shaft) leave the column empty.

## 2. Fingerprints by area

Columns: programme file, stage, bytes, SHA-256 (16), DXBC checksum (8).

### E01 tessellated billboards and their hull/domain (quad, octed, quincunx) (44 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFfxtessellateBlendBumpTexture.ppo` | ps | 10372 | `15DD39CB712A29FC` | `27F1E68E` |
| `GXFfxtessellateBlendBumpTextureGlowD.ppo` | ps | 10472 | `D3EE84662DC144ED` | `1E26908E` |
| `GXFfxtessellateBlendSoftBumpTexture.ppo` | ps | 10792 | `3A95283F76699B17` | `3FA48A7F` |
| `GXFfxtessellateBlendSoftBumpTextureGlowD.ppo` | ps | 10892 | `AF3384EAC1AAB99D` | `76039B19` |
| `GXFfxtessellateBlendSoftTexture.ppo` | ps | 8080 | `AAF453F931FA3937` | `743428EB` |
| `GXFfxtessellateBlendSoftTextureGlowD.ppo` | ps | 8180 | `67838B5E0CF40CAA` | `85344E9F` |
| `GXFfxtessellateBlendTexture.ppo` | ps | 7612 | `092BDC1342995766` | `26D5BD1A` |
| `GXFfxtessellateBlendTextureGlowD.ppo` | ps | 7712 | `015F7CB031D26F81` | `B4299A06` |
| `GXFfxtessellateBumpTexture_octed.dpo` | ds | 5588 | `0A7C8C8ACD2D4C6B` | `1AB3878E` |
| `GXFfxtessellateBumpTexture_octed.hpo` | hs | 1844 | `BC8192B460F62F95` | `DB7318E9` |
| `GXFfxtessellateBumpTexture_quad.dpo` | ds | 5324 | `FA3344A243FBEA60` | `57E1ECB4` |
| `GXFfxtessellateBumpTexture_quad.hpo` | hs | 1844 | `9280790A369059AD` | `A828123E` |
| `GXFfxtessellateBumpTexture_quincunx.dpo` | ds | 5796 | `49E365C6883F1A67` | `1E9453BB` |
| `GXFfxtessellateBumpTexture_quincunx.hpo` | hs | 1844 | `6FFE3D2D4EC20D68` | `1B79FDB1` |
| `GXFfxtessellateBumpTexture.ppo` | ps | 10248 | `5333E9C75AF7C07D` | `50020301` |
| `GXFfxtessellateBumpTexture.vpo` | vs | 1280 | `22274B13AC066EE5` | `BBCE109F` |
| `GXFfxtessellateBumpTextureGlowD.ppo` | ps | 10348 | `407133153AE1C7DA` | `798F6466` |
| `GXFfxtessellateSoftBumpTexture_octed.dpo` | ds | 6288 | `28DA73F06CAFC203` | `1CD094AB` |
| `GXFfxtessellateSoftBumpTexture_octed.hpo` | hs | 1844 | `BC8192B460F62F95` | `DB7318E9` |
| `GXFfxtessellateSoftBumpTexture_quad.dpo` | ds | 6024 | `CE09F1BEB8E9AAAA` | `38014003` |
| `GXFfxtessellateSoftBumpTexture_quad.hpo` | hs | 1844 | `9280790A369059AD` | `A828123E` |
| `GXFfxtessellateSoftBumpTexture_quincunx.dpo` | ds | 6492 | `15E5C2B874174B73` | `B6CD4046` |
| `GXFfxtessellateSoftBumpTexture_quincunx.hpo` | hs | 1844 | `6FFE3D2D4EC20D68` | `1B79FDB1` |
| `GXFfxtessellateSoftBumpTexture.ppo` | ps | 10668 | `327246EAD0A02C18` | `FD6ECEBC` |
| `GXFfxtessellateSoftBumpTexture.vpo` | vs | 1280 | `22274B13AC066EE5` | `BBCE109F` |
| `GXFfxtessellateSoftBumpTextureGlowD.ppo` | ps | 10768 | `711C0816E35305ED` | `30FD183D` |
| `GXFfxtessellateSoftTexture_octed.dpo` | ds | 5508 | `FEB101666C439FF3` | `C7216934` |
| `GXFfxtessellateSoftTexture_octed.hpo` | hs | 1844 | `BC8192B460F62F95` | `DB7318E9` |
| `GXFfxtessellateSoftTexture_quad.dpo` | ds | 5356 | `1CBB24C155D00044` | `B677E25E` |
| `GXFfxtessellateSoftTexture_quad.hpo` | hs | 1844 | `9280790A369059AD` | `A828123E` |
| `GXFfxtessellateSoftTexture_quincunx.dpo` | ds | 5356 | `1CBB24C155D00044` | `B677E25E` |
| `GXFfxtessellateSoftTexture_quincunx.hpo` | hs | 1844 | `6FFE3D2D4EC20D68` | `1B79FDB1` |
| `GXFfxtessellateSoftTexture.ppo` | ps | 7936 | `543F69E386272F1D` | `84154FD3` |
| `GXFfxtessellateSoftTexture.vpo` | vs | 1280 | `22274B13AC066EE5` | `BBCE109F` |
| `GXFfxtessellateSoftTextureGlowD.ppo` | ps | 8036 | `0325850D8051FC45` | `8541A337` |
| `GXFfxtessellateTexture_octed.dpo` | ds | 4936 | `979E63267D01BF4F` | `39F69F82` |
| `GXFfxtessellateTexture_octed.hpo` | hs | 1844 | `BC8192B460F62F95` | `DB7318E9` |
| `GXFfxtessellateTexture_quad.dpo` | ds | 4656 | `65DF62117134592F` | `471563CD` |
| `GXFfxtessellateTexture_quad.hpo` | hs | 1844 | `9280790A369059AD` | `A828123E` |
| `GXFfxtessellateTexture_quincunx.dpo` | ds | 4656 | `65DF62117134592F` | `471563CD` |
| `GXFfxtessellateTexture_quincunx.hpo` | hs | 1844 | `6FFE3D2D4EC20D68` | `1B79FDB1` |
| `GXFfxtessellateTexture.ppo` | ps | 7488 | `870E731AC3A394A6` | `06333893` |
| `GXFfxtessellateTexture.vpo` | vs | 1280 | `22274B13AC066EE5` | `BBCE109F` |
| `GXFfxtessellateTextureGlowD.ppo` | ps | 7588 | `664F55183EAE2C3B` | `3F20A6AF` |

### E01 multi-texture, point sprite, line programmes (19 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFfxtessellateLineColorOnly.ppo` | ps | 6188 | `B0731EF604107CDD` | `34F15B35` |
| `GXFfxtessellateLineColorOnly.vpo` | vs | 892 | `E476540D550E1326` | `65E5E47E` |
| `GXFfxtessellateLineColorOnlyGlowD.ppo` | ps | 6308 | `A842C19D1935BABD` | `65C889E0` |
| `GXFfxtessellateMultiTexture.ppo` | ps | 11288 | `989ED25E7CB8F23E` | `735BE5B4` |
| `GXFfxtessellateMultiTexture.vpo` | vs | 1720 | `BEE2B1C115A98570` | `31839E5B` |
| `GXFfxtessellateMultiTexture2Sampler.ppo` | ps | 9424 | `5862C6EF9282482C` | `353A54B4` |
| `GXFfxtessellateMultiTexture2SamplerGlowD.ppo` | ps | 9512 | `AA540CE26FAAA534` | `C6F201A6` |
| `GXFfxtessellateMultiTextureGlowD.ppo` | ps | 11388 | `87FCD1D52D455F61` | `C0A9032D` |
| `GXFfxtessellatePointSprite.ppo` | ps | 6752 | `3B008E249D7DF53F` | `1BB60058` |
| `GXFfxtessellatePointSprite.vpo` | vs | 3184 | `C7CF4501B9998B73` | `84C1C07C` |
| `GXFfxtessellatePointSpriteGlowD.ppo` | ps | 6852 | `21568ADEC8D57B58` | `F9FC1D3D` |
| `GXFfxtessellateQuadLineColorOnly.ppo` | ps | 6188 | `B0731EF604107CDD` | `34F15B35` |
| `GXFfxtessellateQuadLineColorOnly.vpo` | vs | 984 | `E6DF06570C096497` | `0FC6FD39` |
| `GXFfxtessellateQuadLineColorOnlyGlowD.ppo` | ps | 6308 | `A842C19D1935BABD` | `65C889E0` |
| `GXFfxtessellateSoftMultiTexture.ppo` | ps | 11476 | `358FE17172DF89A9` | `930721C9` |
| `GXFfxtessellateSoftMultiTexture.vpo` | vs | 1720 | `BEE2B1C115A98570` | `31839E5B` |
| `GXFfxtessellateSoftMultiTexture2Sampler.ppo` | ps | 9872 | `9C580CB4CDFF71B1` | `992658AB` |
| `GXFfxtessellateSoftMultiTexture2SamplerGlowD.ppo` | ps | 9972 | `363C58B05215ED7F` | `054A3486` |
| `GXFfxtessellateSoftMultiTextureGlowD.ppo` | ps | 11576 | `4FAD39E4962DF72A` | `95218CF7` |

### E01 non-tessellated (texture, bump, soft, multi, color only, point sprite) (24 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFfxbumpTexture.ppo` | ps | 10220 | `E8510D726316CE79` | `1BA223F9` |
| `GXFfxbumpTexture.vpo` | vs | 4988 | `6AF3236C0AF4B0F6` | `2ED38CF5` |
| `GXFfxbumpTextureGlowD.ppo` | ps | 10320 | `3F19A834248DB524` | `A34AD4B1` |
| `GXFfxcolorOnly.ppo` | ps | 6480 | `39BF5748E5C74BE5` | `18588A51` |
| `GXFfxcolorOnly.vpo` | vs | 2852 | `5E6CAF49B22BD1CB` | `40EF263C` |
| `GXFfxcolorOnlyGlowD.ppo` | ps | 6600 | `B76C5FFE7234AB3E` | `A3303468` |
| `GXFfxmultiTexture.ppo` | ps | 11428 | `7ECD8A6A80454CE9` | `0B64AB5A` |
| `GXFfxmultiTexture.vpo` | vs | 4220 | `2054CA63378A8584` | `AE06D2CA` |
| `GXFfxmultiTextureGlowD.ppo` | ps | 11528 | `CD1848A84DEEF767` | `DD00AEB4` |
| `GXFfxpointSprite.ppo` | ps | 6752 | `3B008E249D7DF53F` | `1BB60058` |
| `GXFfxpointSprite.vpo` | vs | 3144 | `D9DE871296E783EE` | `CB2160FC` |
| `GXFfxpointSpriteGlowD.ppo` | ps | 6852 | `21568ADEC8D57B58` | `F9FC1D3D` |
| `GXFfxsoftBumpTexture.ppo` | ps | 10708 | `86CE83DDF2CABF25` | `6A6CA8B8` |
| `GXFfxsoftBumpTexture.vpo` | vs | 5584 | `F7A195A34C60A392` | `B5617020` |
| `GXFfxsoftBumpTextureGlowD.ppo` | ps | 10808 | `04B850A978D0AD49` | `44513AE0` |
| `GXFfxsoftMultiTexture.ppo` | ps | 12028 | `CAB0BCFE04C64570` | `46B00A3C` |
| `GXFfxsoftMultiTexture.vpo` | vs | 4916 | `5F976B44E2D98C37` | `D456E4B2` |
| `GXFfxsoftMultiTextureGlowD.ppo` | ps | 12128 | `95DAD5943A9D723E` | `8ED9CFF4` |
| `GXFfxsoftTexture.ppo` | ps | 8096 | `623F314A24E76518` | `FA9574D3` |
| `GXFfxsoftTexture.vpo` | vs | 4676 | `DAA37749DB70E0A1` | `2F949E2E` |
| `GXFfxsoftTextureGlowD.ppo` | ps | 8196 | `78EAF9F47BFB0CD7` | `E7911ED5` |
| `GXFfxtexture.ppo` | ps | 8548 | `C5040ABBC39A6CCD` | `B0060F98` |
| `GXFfxtexture.vpo` | vs | 3924 | `12632A0CA770E96E` | `76743DD1` |
| `GXFfxtextureGlowD.ppo` | ps | 8648 | `71BA88E2B2149778` | `D5E188F8` |

### E02 tracers (15 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFfxtessellateDistortionTracerBumpTexture.ppo` | ps | 10056 | `343BF05020259B5A` | `2CB2FEF3` |
| `GXFfxtessellateDistortionTracerBumpTexture.vpo` | vs | 1188 | `9C9A162E4AE24AF2` | `282491E0` |
| `GXFfxtessellateDistortionTracerBumpTextureGlowD.ppo` | ps | 10156 | `BBB69FC77544AA0F` | `6B675F97` |
| `GXFfxtessellateSoftTracerBumpTexture.ppo` | ps | 10204 | `403A3F9ED77D577A` | `7AF9DC5B` |
| `GXFfxtessellateSoftTracerBumpTexture.vpo` | vs | 1188 | `9C9A162E4AE24AF2` | `282491E0` |
| `GXFfxtessellateSoftTracerBumpTextureGlowD.ppo` | ps | 10304 | `A16C3AFF6002610E` | `E3EBDB96` |
| `GXFfxtessellateSoftTracerTexture.ppo` | ps | 7836 | `44BC4B49CC40C2BA` | `FC8A497E` |
| `GXFfxtessellateSoftTracerTexture.vpo` | vs | 1052 | `162B8D99FF810C2F` | `34E61551` |
| `GXFfxtessellateSoftTracerTextureGlowD.ppo` | ps | 7936 | `2F4E7618F5C6A9F0` | `98A3F080` |
| `GXFfxtessellateTracerBumpTexture.ppo` | ps | 9800 | `7C222DE31FADA91B` | `8A56058D` |
| `GXFfxtessellateTracerBumpTexture.vpo` | vs | 1188 | `9C9A162E4AE24AF2` | `282491E0` |
| `GXFfxtessellateTracerBumpTextureGlowD.ppo` | ps | 9900 | `52E64C847812818A` | `617FD7C0` |
| `GXFfxtessellateTracerTexture.ppo` | ps | 7468 | `03CAC7E009755574` | `527643B6` |
| `GXFfxtessellateTracerTexture.vpo` | vs | 1052 | `162B8D99FF810C2F` | `34E61551` |
| `GXFfxtessellateTracerTextureGlowD.ppo` | ps | 7568 | `A9C1CF7130D2D754` | `D3AEF493` |

### E04 distortion and E05 radial blur (19 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFfxbumpDistorsionSoftTexture.ppo` | ps | 9044 | `E754C78BDE69A10E` | `2AC33A67` |
| `GXFfxbumpDistorsionSoftTexture.vpo` | vs | 5592 | `244BD5B83180FCB5` | `6884CFA5` |
| `GXFfxbumpDistorsionSoftTextureGlowD.ppo` | ps | 9144 | `4B793688A69B7545` | `2CFAFBCA` |
| `GXFfxdistortion.ppo` | ps | 5996 | `35F58BBCC652AF34` | `B9B450CD` |
| `GXFfxdistortion.vpo` | vs | 3164 | `6BA3A88E8105E54B` | `A3FE4748` |
| `GXFfxdistortionBump.ppo` | ps | 6200 | `23E7B7A47B67FAC7` | `06A94A0C` |
| `GXFfxdistortionBumpTexture.ppo` | ps | 6432 | `B062F2AED642D0FD` | `0B0C8F40` |
| `GXFfxdistortionRotation.ppo` | ps | 6268 | `0D03461B48D946D0` | `3E9540C7` |
| `GXFfxdistortionSoft.ppo` | ps | 6404 | `EDF98B8A80D32751` | `CF5301EA` |
| `GXFfxdistortionSoft.vpo` | vs | 3740 | `B42CCC90F92B909D` | `D1B29EB7` |
| `GXFfxdistortionSoftBump.ppo` | ps | 6608 | `BE6F01DC1B53DFD3` | `45F8E2A7` |
| `GXFfxdistortionSoftBumpTexture.ppo` | ps | 6840 | `B43B93F0364F4E3D` | `F6E90F49` |
| `GXFfxdistortionSoftRotation.ppo` | ps | 6676 | `52B2F972F0D0F3B4` | `8250CE8F` |
| `GXFfxdistortionSoftTexture.ppo` | ps | 6636 | `48475AA8DC088740` | `19545C17` |
| `GXFfxdistortionTexture.ppo` | ps | 6228 | `7141D3D8C070A7E1` | `0FE42573` |
| `GXFfxradialBlur.ppo` | ps | 5812 | `E7094993FEC9B52F` | `BD41B630` |
| `GXFfxradialBlur.vpo` | vs | 3016 | `C8BADD634B7E7BD4` | `CD10BAE9` |
| `GXFfxSoftRadialBlur.ppo` | ps | 6284 | `54A235AA1B5BAEF5` | `120C339A` |
| `GXFfxSoftRadialBlur.vpo` | vs | 3860 | `B16BC86D8DFE990A` | `6C2C7D62` |

### E08 light shaft (5 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFfxLightShaft.vpo` | vs | 912 | `2720BCDAD01E7934` | `8BA446FE` |
| `GXFfxLightShaftDust.ppo` | ps | 6128 | `ADA0A85734A19FA4` | `8EF5DFC3` |
| `GXFfxLightShaftProcedual.ppo` | ps | 4904 | `941BD755D8FCD503` | `FBA87DB2` |
| `GXFfxLightShaftProcedualMask.ppo` | ps | 5084 | `EF2C9586D22F9139` | `DC470723` |
| `GXFfxLightShaftTexture.ppo` | ps | 5696 | `67E7066E349FAA85` | `8B61643F` |

### E07 GPU particles (compute, vertex, geometry, pixel) (58 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `CS_VFX_BitonicSort.cpo` | cs | 2288 | `7DFF932DF60E0793` | `9053B160` |
| `CS_VFX_BitonicSort1024.cpo` | cs | 2288 | `959C3A3222C2EFC1` | `FD864737` |
| `CS_VFX_BitonicSort128.cpo` | cs | 2288 | `FB16C35815B8EC91` | `874EE941` |
| `CS_VFX_BitonicSort256.cpo` | cs | 2288 | `6B4323D5833E990B` | `E061CCDE` |
| `CS_VFX_BitonicSort512.cpo` | cs | 2288 | `5859009ED8EC480C` | `3F5AB900` |
| `CS_VFX_InitSort.cpo` | cs | 6140 | `194876B31F17B5EF` | `8C8AEDEC` |
| `CS_VFX_InitSort1024.cpo` | cs | 6140 | `302F28131780EEB2` | `F4AA8CB1` |
| `CS_VFX_InitSort128.cpo` | cs | 6140 | `731107D0356E20C6` | `8A2D8F90` |
| `CS_VFX_InitSort256.cpo` | cs | 6140 | `F4B2755EA79411D2` | `E20D084B` |
| `CS_VFX_InitSort512.cpo` | cs | 6140 | `E88365884975F727` | `4635CE08` |
| `CS_VFX_StandardParticle.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleCounterDownSizeHigher.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleCounterDownSizeHighest.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleCounterDownSizeLower.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleCounterDownSizeLowest.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleCounterDownSizeMiddle.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleCounterDownSizeMiddleHigher.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleCounterDownSizeMiddleLower.cpo` | cs | 936 | `DCDBD092151D6730` | `3A2D9F15` |
| `CS_VFX_StandardParticleEmit.cpo` | cs | 36540 | `2DAA74CA444547FC` | `12BDFC4C` |
| `CS_VFX_StandardParticleGenerateSdw.cpo` | cs | 1808 | `275338C9369DEEBF` | `A3EA67C0` |
| `CS_VFX_StandardParticleHigher.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleHighest.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleLower.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleLowest.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleMiddle.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleMiddleHigher.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `CS_VFX_StandardParticleMiddleLower.cpo` | cs | 77572 | `B1F7202AFA8D25F1` | `381785E7` |
| `GS_VFX_StandardParticle.gpo` | gs | 18920 | `CA77CC925A851C37` | `8F6E450D` |
| `GS_VFX_StandardParticleGenerateSdw.gpo` | gs | 18920 | `CA77CC925A851C37` | `8F6E450D` |
| `PS_VFX_SingleTexture.ppo` | ps | 9052 | `875E8F612999F89D` | `0220C89F` |
| `PS_VFX_SingleTextureBump.ppo` | ps | 12436 | `4BD2C038ECF8D164` | `1191783B` |
| `PS_VFX_SingleTextureBumpGlowD.ppo` | ps | 12556 | `615AB8BA1501A2F9` | `75D83E9D` |
| `PS_VFX_SingleTextureBumpGlowLP.ppo` | ps | 12556 | `418718D5247BE7DE` | `C06C6974` |
| `PS_VFX_SingleTextureCheap.ppo` | ps | 6348 | `E16B4A4B273628F2` | `716E1CE6` |
| `PS_VFX_SingleTextureCheapBump.ppo` | ps | 9880 | `ADF5B96A0F91E661` | `29E10336` |
| `PS_VFX_SingleTextureCheapBumpGlowD.ppo` | ps | 10000 | `DF71F967CCF9B279` | `4E555E40` |
| `PS_VFX_SingleTextureCheapBumpGlowLP.ppo` | ps | 10000 | `68DFF8B39D85C15B` | `E879D400` |
| `PS_VFX_SingleTextureCheapGlowD.ppo` | ps | 6468 | `1BF4233692E2AF30` | `620A862C` |
| `PS_VFX_SingleTextureCheapGlowLP.ppo` | ps | 6468 | `53EB4DAABE2930CA` | `244BBD1F` |
| `PS_VFX_SingleTextureGenerateSdw.ppo` | ps | 7548 | `F699DE453A562D2F` | `07D4B355` |
| `PS_VFX_SingleTextureGlowD.ppo` | ps | 9172 | `99906DAA0379865B` | `32CD1DA8` |
| `PS_VFX_SingleTextureGlowLP.ppo` | ps | 9172 | `923371B2CFD62ED0` | `809CE101` |
| `PS_VFX_SingleTextureSdw.ppo` | ps | 9052 | `875E8F612999F89D` | `0220C89F` |
| `PS_VFX_SoftTexture.ppo` | ps | 9472 | `3A525489BDB0DE65` | `AB2C499A` |
| `PS_VFX_SoftTextureBump.ppo` | ps | 12892 | `7DF6146000F782CD` | `81FBA735` |
| `PS_VFX_SoftTextureBumpGlowD.ppo` | ps | 13012 | `49A97C91E7ADDDF3` | `37103AFB` |
| `PS_VFX_SoftTextureBumpGlowLP.ppo` | ps | 13012 | `499C0E5D5FAD7698` | `91BFA244` |
| `PS_VFX_SoftTextureCheap.ppo` | ps | 6784 | `459B49C292A2D9B3` | `D060B29D` |
| `PS_VFX_SoftTextureCheapBump.ppo` | ps | 10336 | `F994DE06C5853A17` | `B91678BC` |
| `PS_VFX_SoftTextureCheapBumpGlowD.ppo` | ps | 10456 | `6B6E57AAF0C4A125` | `96EF22A9` |
| `PS_VFX_SoftTextureCheapBumpGlowLP.ppo` | ps | 10456 | `DDB4AC7EE4262092` | `9836B5F9` |
| `PS_VFX_SoftTextureCheapGlowD.ppo` | ps | 6904 | `FD2AB02BFDB7E624` | `D68F7099` |
| `PS_VFX_SoftTextureCheapGlowLP.ppo` | ps | 6904 | `6151E1753BC9BCFD` | `B3997EA1` |
| `PS_VFX_SoftTextureGenerateSdw.ppo` | ps | 9532 | `9CEAB7513F1D5501` | `8243C10E` |
| `PS_VFX_SoftTextureGlowD.ppo` | ps | 9592 | `E162419030819331` | `CC5490D9` |
| `PS_VFX_SoftTextureGlowLP.ppo` | ps | 9592 | `63FBD44521AD49DA` | `CDC9F450` |
| `PS_VFX_SoftTextureSdw.ppo` | ps | 9472 | `3A525489BDB0DE65` | `AB2C499A` |
| `VS_VFX_StandardParticle.vpo` | vs | 2644 | `F3A7E8F574911CD0` | `2B128936` |

### Glow chain (extraction, blur taps, composite) (5 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `EmissiveToGlowPS.ppo` | ps | 744 | `BC02ECF1710A0752` | `C7E4B6D0` |
| `GlowCompositeSinglePS.ppo` | ps | 936 | `F85C2B7C55774BE4` | `692AF0ED` |
| `Sample13Weight.ppo` | ps | 1384 | `CD4CE825F5B81AE6` | `8A661248` |
| `Sample13WeightforGlow.ppo` | ps | 1540 | `7C1D499D3EA5118F` | `BD115216` |
| `Sample13WeightforGlowKernel.ppo` | ps | 1560 | `401450D16F2BF397` | `F58EB6D8` |

### Velocity and motion blur (GX side) (4 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `MotionBlur.ppo` | ps | 2028 | `CEF5310828AAE4FB` | `2065A1DA` |
| `MotionBlurDilateVelocity.ppo` | ps | 4796 | `DC16977EED7AF7DD` | `3E020013` |
| `MotionBlurWriteCMBVelocity.ppo` | ps | 1716 | `36DEFF9FE10002AE` | `871C693D` |
| `MotionBlurWriteCMBVelocityNoMove.ppo` | ps | 1440 | `AA40BCBA67D9A671` | `671184F2` |

### Screen-space AO and shadow blur/composite (8 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `ScreenSpaceAO.ppo` | ps | 3568 | `1559CCF2497D2153` | `EF751C82` |
| `ScreenSpaceAO2_BilateralUpScale.ppo` | ps | 3764 | `78710C47DEB74DD6` | `BA1F6CCF` |
| `ScreenSpaceAO2_ReProj.ppo` | ps | 5304 | `E06454AE90ECEA1E` | `509443E3` |
| `ScreenSpaceAO2_SeparatedReProj.ppo` | ps | 3876 | `64D47110B81ED4C6` | `FF2C481E` |
| `ScreenSpaceAO2_WithNormal.ppo` | ps | 4932 | `38299D8728C5FD99` | `82EAC6F6` |
| `ScreenSpaceAO2_WithNormalReProj.ppo` | ps | 5792 | `0156472914F73400` | `6397A524` |
| `ScreenSpaceAO2.ppo` | ps | 4384 | `5B304D81A9559B79` | `6CC3AA9B` |
| `ScreenSpaceAOAndCombine.ppo` | ps | 3920 | `AB579AEA7B1760D8` | `815A7BA5` |

### Shadow maps and composite (11 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXLightAcc_DDirLight1.ppo` | ps | 8764 | `AC0119416379BC88` | `4409EA4F` |
| `GXLightAcc_DDirLightShadowed1_Cloud.ppo` | ps | 11628 | `9329A42198673CBF` | `27A9530A` |
| `GXLightAcc_DDirLightShadowed1.ppo` | ps | 11092 | `30A79AC6DC7F5430` | `456BD332` |
| `GXRend_CompositeShadowMask.ppo` | ps | 1200 | `63622B8A3B308FB9` | `FB96FE7B` |
| `GXRend_CopyShadowMapDepth.ppo` | ps | 1940 | `1F9CA6226C667E92` | `CCBB0E32` |
| `GXScreenSpaceCascadedShadow_PCF4.ppo` | ps | 4432 | `030846666693C1A4` | `56FC2F5D` |
| `GXScreenSpaceCascadedShadow_PCF9.ppo` | ps | 4912 | `4B9185D42BD54D56` | `59754F8C` |
| `GXScreenSpaceCascadedShadow_Soft.ppo` | ps | 9496 | `9602852AFBC7E77F` | `19CD87E0` |
| `GXScreenSpaceShadow_Blur.ppo` | ps | 6720 | `217725D742DCC336` | `471FDAED` |
| `GXScreenSpaceShadow_PCF4.ppo` | ps | 4244 | `85991AEE024540D9` | `D839343F` |
| `GXScreenSpaceShadow_PCF9.ppo` | ps | 4724 | `F5432EF0DEE04505` | `151130F5` |

### Spot, line and point lights (deferred) (20 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXLightAcc_DLineLight1.ppo` | ps | 5492 | `DFDF3F1C168C9CAF` | `2E25E2E3` |
| `GXLightAcc_DLineLight2.ppo` | ps | 7120 | `5694346B460CF1ED` | `75E00D5D` |
| `GXLightAcc_DLineLight3.ppo` | ps | 8720 | `A228BE20929DE3CE` | `2F066286` |
| `GXLightAcc_DLineLight4.ppo` | ps | 10292 | `52D91D1B5C69D444` | `370ADB07` |
| `GXLightAcc_DPointLight1.ppo` | ps | 5324 | `50202AA9AF69AF4E` | `CECC4E7A` |
| `GXLightAcc_DPointLight12.ppo` | ps | 21244 | `C80DE46B5F07F2EE` | `4CC27FBF` |
| `GXLightAcc_DPointLight4.ppo` | ps | 9688 | `5CBD917ECA55A15A` | `83787EA1` |
| `GXLightAcc_DPointLight8.ppo` | ps | 15476 | `AE5570D738D9866D` | `1EF4755E` |
| `GXLightAcc_DPointLightShadowed1.ppo` | ps | 7520 | `11113E3F4C8D9273` | `76ED9E9D` |
| `GXLightAcc_DPointLightShadowed2.ppo` | ps | 10796 | `0568CBB6841641EA` | `8923B6D0` |
| `GXLightAcc_DPointLightShadowed3.ppo` | ps | 14080 | `1DE957207529B119` | `60DBB5C6` |
| `GXLightAcc_DPointLightShadowed4.ppo` | ps | 17364 | `6234912B9C4D65A3` | `236EC6B3` |
| `GXLightAcc_DSpotLight1.ppo` | ps | 6732 | `3891656C5971FD3A` | `C7F3354E` |
| `GXLightAcc_DSpotLight2.ppo` | ps | 9316 | `56DD3210F6CAA866` | `8096A8F3` |
| `GXLightAcc_DSpotLight3.ppo` | ps | 11872 | `1C0B22E8F040ABA0` | `EFCF629C` |
| `GXLightAcc_DSpotLight4.ppo` | ps | 14428 | `C2BA4A7DBA71EAFB` | `B49AE932` |
| `GXLightAcc_DSpotLightShadowed1.ppo` | ps | 8048 | `9C8EDA715073F59A` | `1484E317` |
| `GXLightAcc_DSpotLightShadowed2.ppo` | ps | 11668 | `AEA235FE0E8E93DE` | `C7B7F031` |
| `GXLightAcc_DSpotLightShadowed3.ppo` | ps | 15220 | `AEC37BC32AB48A71` | `E89EF484` |
| `GXLightAcc_DSpotLightShadowed4.ppo` | ps | 18792 | `62E9C9927C9DD4D2` | `3EE30432` |

### Light accumulation vertex programmes, composite, AO, env map, SSS (21 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXEnvDecal_Deferred.ppo` | ps | 6500 | `93910DF87651FA2E` | `F32603BA` |
| `GXEnvDecal.ppo` | ps | 6500 | `705F2BE31792A12F` | `ACD735C1` |
| `GXEnvMapRegion_Dbg.ppo` | ps | 4540 | `B37483869E4CA625` | `2C6B994D` |
| `GXEnvMapRegion_IemDiff_Pcc.ppo` | ps | 7684 | `9BAAF0B23AE584EB` | `DC3FB03A` |
| `GXEnvMapRegion_IemDiff.ppo` | ps | 6428 | `3EF1D20465259E53` | `15CAF955` |
| `GXEnvMapRegion_IvDiff_Pcc.ppo` | ps | 8888 | `1C974829F6C3A39C` | `0212073B` |
| `GXEnvMapRegion_IvDiff.ppo` | ps | 7768 | `01C2F53E4E6B185B` | `E13BB888` |
| `GXEnvMapRegion_Pcc.ppo` | ps | 7400 | `7D72145D1994D452` | `A78C1E80` |
| `GXEnvMapRegion_RemDiff_Pcc.ppo` | ps | 7620 | `5471E363123F3394` | `4BDBB194` |
| `GXEnvMapRegion_RemDiff.ppo` | ps | 6364 | `4A082C54F4A400C3` | `690C540D` |
| `GXEnvMapRegion.ppo` | ps | 6144 | `A1BF7D07CCB8427F` | `A99A3991` |
| `GXLightAcc.vpo` | vs | 1016 | `865D4E3C093CB1C1` | `B465C1F3` |
| `GXLightAccTransform.vpo` | vs | 1108 | `7B9B78B1262580A9` | `7E0BC94D` |
| `GXLightCmbAlbedo.ppo` | ps | 876 | `510B43E8B40F6B6E` | `82ACCC37` |
| `GXLightComposite.ppo` | ps | 4528 | `390497DC0063668D` | `2728A315` |
| `GXLightDeferredAO.ppo` | ps | 1156 | `DD70D013869331C1` | `5AB63F61` |
| `GXLightDeferredAOWithSsr.ppo` | ps | 1576 | `5566CC0A14456524` | `7A0A12F2` |
| `GXSsr.vpo` | vs | 672 | `8DF272721562B791` | `078C7C29` |
| `GXSss.vpo` | vs | 672 | `8DF272721562B791` | `078C7C29` |
| `GXSssBlurX.ppo` | ps | 10252 | `50B0C959B5535850` | `8A0386DA` |
| `GXSssBlurY.ppo` | ps | 10312 | `7C0B2025E5ACB35D` | `A5025D97` |

### Deferred decals (the commonest) (12 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `Decal_BlendWeight.ppo` | ps | 3084 | `588D7CF253EB975C` | `D95CB8B4` |
| `Decal_BlendWeight.vpo` | vs | 1216 | `104A66F4B06F8000` | `0C6CEC51` |
| `Decal_BlendWeightAm.ppo` | ps | 3084 | `588D7CF253EB975C` | `D95CB8B4` |
| `Decal_Blood_DifSpcBump.ppo` | ps | 9184 | `F1BEBA70A3C68693` | `62372391` |
| `Decal_Blood_DifSpcBump.vpo` | vs | 1216 | `104A66F4B06F8000` | `0C6CEC51` |
| `Decal_Blood_DifSpcBumpAm.ppo` | ps | 9320 | `F091B52E21F911D6` | `10BAA905` |
| `Decal_DifSpc.ppo` | ps | 7512 | `8EB16E68D252C88A` | `10AA2E3D` |
| `Decal_DifSpc.vpo` | vs | 1216 | `104A66F4B06F8000` | `0C6CEC51` |
| `Decal_DifSpcAm.ppo` | ps | 7648 | `2C612771A14EDBC8` | `A0A47F64` |
| `Decal_DifSpcBump.ppo` | ps | 7976 | `AB018E2B6276BD32` | `10E51DB6` |
| `Decal_DifSpcBump.vpo` | vs | 1216 | `104A66F4B06F8000` | `0C6CEC51` |
| `Decal_DifSpcBumpAm.ppo` | ps | 8112 | `DE26926FE98E3AC2` | `413064D8` |

### Default character family (GXFlver_ColDifSpcBumpIbl_Bloody): G-buffer, depth, velocity, forward (17 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `GXFlver_ColDifSpcBumpIbl_Bloody_Dpt_Skin.vpo` | vs | 5704 | `856094185F838864` | `91EE3702` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Dpt.ppo` | ps | 460 | `F532ED2F370BF7CC` | `CD700FF3` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Dpt.vpo` | vs | 5008 | `D83BF08BC7BC3CC6` | `99F5992C` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_DptA_Skin.vpo` | vs | 6440 | `2D9BE9DBC4166798` | `0429B025` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_DptA.ppo` | ps | 8844 | `57C75634360A84AA` | `7ACCF598` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_DptA.vpo` | vs | 5744 | `13A691A4429ADD2B` | `AFF5B756` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd_[A].ppo` | ps | 32824 | `22BBD8A344E676C9` | `7495F0C9` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd_Skin.vpo` | vs | 6628 | `A31CF5E6168E4EEA` | `09A9F34E` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd.ppo` | ps | 32208 | `9B3273B4E09222FD` | `28E0A8F0` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd.vpo` | vs | 5984 | `2462BF2D6AC812BD` | `EA56BE45` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Gbuf_[A].ppo` | ps | 12272 | `A6A110C821BE85E1` | `D2E1D593` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_GBuf_Skin.vpo` | vs | 6572 | `8C9B851160967E7E` | `06C56EB4` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Gbuf.ppo` | ps | 9508 | `08AF5F726CF17196` | `073C3444` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_GBuf.vpo` | vs | 5928 | `AD17D888B877B0DF` | `8F68D535` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Velo_[A].ppo` | ps | 5596 | `3CBE2C4405DDC767` | `4B07493B` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Velo_Skin.vpo` | vs | 5056 | `E69E65F5E246F01B` | `2F4C5BD7` |
| `GXFlver_ColDifSpcBumpIbl_Bloody_Velo.vpo` | vs | 3864 | `77FA437BC9FA056A` | `5A50BE63` |

### Tone map, gamma and colour grading (7 programmes)

| Programme | Stage | Bytes | SHA-256 (16) | DXBC (8) |
|---|---|---|---|---|
| `ColorGrading.ppo` | ps | 960 | `F0544E293BEF625C` | `7E97D75E` |
| `OutputColor.ppo` | ps | 704 | `67C313EF7BD9F599` | `BB9B1887` |
| `OutputOne.ppo` | ps | 536 | `7AEDCEC4D5A7C137` | `03F46A9D` |
| `ToneMap_GammaPS.ppo` | ps | 1628 | `754068A0A5FC7E95` | `146A5602` |
| `ToneMap_NonePS.ppo` | ps | 692 | `43B82A960C3F3C07` | `668F792B` |
| `ToneMapAdaptVS.vpo` | vs | 760 | `26CFA496BDA7D82C` | `384DB4C7` |
| `ToneMapVS.vpo` | vs | 636 | `D6B9D2C1BED19368` | `FAD9EBDA` |

