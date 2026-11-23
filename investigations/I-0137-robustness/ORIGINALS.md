# I-0137: the original's functions read for tranche 2

Read on 2026-11-12 through GhidraMCP (Thomas's Ghidra on `ghidra-mcp-ds3`, DarkSoulsIII.exe 1.15.2, SHA-256 ef5e07c5…498c), the
decompiler's C trimmed to the lines the corrections rest on (security cookie, reference counting and stack variables left out).
READ statically; nothing here was observed in a run. The integrity review asked for this record (its L6: the reads could not be
checked from the repository).

## 14002B260: a 2D texture wrapper made from an encoded source (hooked by `AdmitEncodedTexture`)

```c
void FUN_14002b260(longlong *param_1 /* wrapper */, longlong *param_2 /* source */) {
  (**(code **)(*param_1 + 0x88))();                 // the wrapper retired first (its own slot +0x88)
  puStack_88 = 0; lStack_80 = 0;
  iVar4 = (**(code **)(*param_2 + 0x50))(param_2,&puStack_88,&lStack_80,0);   // the source makes the GX texture
  if (iVar4 == 0) {
    GXTexture_GetDescAndSize_141924730(puStack_88 + 3,auStack_68);
    if ((iStack_58 - 0x46U < 3) || (iStack_58 - 0x4cU < 3)) {                  // formats 70..72, 76..78
      iVar4 = (**(code **)(*param_2 + 0x38))(param_2);
      if (iVar4 != 0) FUN_140147090(param_1 + 2,param_2,iVar4);                // the authored constant
    } else {
      uVar5 = (**(code **)(*param_2 + 0x10))(param_2);
      uVar6 = (**(code **)(*param_2 + 0x68))(param_2);
      FUN_140147110(param_1 + 2,uVar6,uVar5);
    }
    ...                                                                        // +0x58 the size, +0x40 the texture, +0x48, +0x60
  }
  return;                                                                      // iVar4 != 0: nothing more, the wrapper stays empty
}
```

## 1400AC110: the cube wrapper's (hooked by `AdmitEncodedCube`)

```c
void FUN_1400ac110(longlong param_1 /* wrapper */, longlong *param_2 /* source */) {
  FUN_1400ac020();                                   // the wrapper retired first
  puStack_88 = 0; uStack_80 = 0;
  iVar4 = (**(code **)(*param_2 + 0x50))(param_2,&puStack_88,&uStack_80,0);
  if (iVar4 == 0) { ... *(undefined4 *)(param_1 + 0x58) = uVar5; }             // the constant, the size
  ...                                                // +0x40 takes puStack_88 (null when the source failed), +0x48 uStack_80
  return;
}
```

Both: when the source's slot +0x50 fails, the wrapper is left retired and empty and the function returns. Natively
(`AdmitEncodedTextureFamily`'s `skip`): the wrapper retired, no record.

## 140E49320: an image source made from a GX texture (hooked by `NativeUiTextureImport`)

```c
undefined8 CreateImageSourceFromGxTexture_140e49320(undefined8 param_1, undefined8 param_2 /* name */,
                                                    longlong *param_3 /* manager */, longlong *param_4 /* holder */) {
  if ((param_3 != 0) && (param_4 != 0)) {
    ...                                                                        // under the lock 144793D28
    lVar2 = (**(code **)(*param_4 + 0x20))(param_4);
    plVar3 = (longlong *)(**(code **)(*param_3 + 0xd8))(param_3,*(undefined8 *)(lVar2 + 0x18),auStackX_18,0);
    if (plVar3 != 0) {
      lVar2 = (**(code **)(*_DAT_144936cc0 + 0x50))(_DAT_144936cc0,0x70,0);    // the 0x70-byte allocation
      if (lVar2 == 0) return 0;
      ...
      return ImageSource_Build_140e478f0(lVar2,param_2,uVar1,&uStack_38,0x100,plVar3,0);
    }
  }
  return 0;                                                                    // no manager, no holder, or the manager made no image
}
```

## 140E48980: its one caller, the image repository's lookup by name

`get_function_xrefs` of 140E49320: one call, from 140E48ABE inside 140E48980 (and one data reference, 144A488EC).

```c
void TextureImageRepo_LookupOrImport_140e48980(longlong param_1 /* repo */, short *param_2 /* name */, undefined8 param_3) {
  lVar2 = TextureImageRepo_FindByName_140e48280();
  if (lVar2 == 0) {                                                            // a miss: import by name
    ...
    lVar4 = TextureRegistry_FindByName_140d4ed20(_DAT_144792f58,param_2);
    lVar5 = 0; if (lVar4 != 0) lVar5 = *(longlong *)(lVar4 + 0x60);            // the holder, or none
    lVar5 = CreateImageSourceFromGxTexture_140e49320(param_1,&uStack_2a8,param_3,lVar5);
    ...
    if (lVar5 != 0) {                                                          // stored only when an image was made
      ...
      plVar6 = (longlong *)FUN_140e47eb0(param_1 + 0x60,auStack_278);
      *plVar6 = lVar5;
    }
  }
  return;
}
```

So a 0 from 140E49320 stores nothing under the name: the next lookup of that name misses again and imports again.
