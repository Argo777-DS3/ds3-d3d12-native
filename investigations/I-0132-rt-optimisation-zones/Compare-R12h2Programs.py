"""R12h step 2: the build-time programmes against the run-time ones, byte for byte.
Old: each C++ string at git `base` compiled by D3DCompile (a d3dcompiler_47.dll, ctypes) with the flags the module used.
New: the moved .hlsl compiled by fxc with the build's flags (/Fo, binary); with a build folder, its generated .dxbc.h too.
Usage: python -I Compare-R12h2Programs.py <worktree> <base commit, before the move: b4708decc> <fxc.exe> <d3dcompiler_47.dll> [<build folder>]
Prints one row a programme and the count; RESULT « R12h step 2 » has its runs."""
import ctypes, hashlib, os, subprocess, sys, tempfile
wt, base, fxc, dll = sys.argv[1:5]
built = sys.argv[5] if len(sys.argv) > 5 else None   # a build folder: its <file>_<entry>.dxbc.h compared as well
import re
def header_bytes(path):
    text = open(path, encoding='ascii', errors='replace').read()
    body = text[text.index('{', text.index('_dxbc[]')) + 1:text.rindex('}')]
    return bytes(int(v, 0) for v in re.findall(r'0x[0-9a-fA-F]+|\d+', body))
d3d = ctypes.WinDLL(dll)
def compile_old(src, entry, profile, flags):
    code = ctypes.c_void_p(); err = ctypes.c_void_p()
    f = d3d.D3DCompile
    f.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p,
                  ctypes.c_uint, ctypes.c_uint, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_void_p)]
    f.restype = ctypes.c_long
    hr = f(src, len(src), None, None, None, entry.encode(), profile.encode(), flags, 0, ctypes.byref(code), ctypes.byref(err))
    if hr < 0: raise RuntimeError(f'{entry}: {hr:#x}')
    vt = ctypes.cast(ctypes.cast(code, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
    ptr = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(vt[3])(code)
    size = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(vt[4])(code)
    data = ctypes.string_at(ptr, size)
    for blob in (code, err):  # IUnknown::Release
        if blob.value: ctypes.WINFUNCTYPE(ctypes.c_ulong, ctypes.c_void_p)(ctypes.cast(ctypes.cast(blob, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))[2])(blob)
    return data
def git_show(path):
    return subprocess.run(['git', '-C', wt, 'show', f'{base}:native/{path}'], capture_output=True, check=True).stdout.decode('utf-8')
def old_string(path, token, nth=0):
    t = git_show(path); i = -1
    for _ in range(nth + 1): i = t.index(token, i + 1)
    s = i + len(token); e = t.index(')"', s)
    return t[s:e].encode('utf-8')
STRICT, IEEE, O3 = 1 << 11, 1 << 13, 1 << 15
ROWS = [  # (C++ path at base, token, nth, hlsl, entries, profile(s), run-time flags, fxc flags)
    ('renderer/taa_shader.h', 'kTaaShader=R"(', 0, 'taa', [('Resolve', 'cs_5_0'), ('Sharpen', 'cs_5_0'), ('Mirror', 'cs_5_0')], STRICT | IEEE | O3, ['/Ges', '/Gis', '/O3']),
    ('renderer/auto_exposure_shader.h', 'kAutoExposureShader[]=R"(', 0, 'auto_exposure', [('Histogram', 'cs_5_0'), ('Adapt', 'cs_5_0')], STRICT, ['/Ges']),
    ('world/menu_tonemap.h', 'kOffscreenCompositionHlsl=R"(', 0, 'offscreen_composition', [('VS', 'vs_5_0'), ('PS', 'ps_5_0'), ('BackgroundPS', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass.cpp', 'constexpr const char* shader=R"(', 0, 'material_composition', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_frame.cpp', 'constexpr const char* shader=R"(', 0, 'effect_layer', [('VS', 'vs_5_0'), ('Downsample', 'ps_5_0'), ('Composite', 'ps_5_0'), ('Bilateral', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_velocity.cpp', 'velocity_resolve_shader=R"(', 0, 'velocity_resolve', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_velocity.cpp', 'velocity_half_shader=R"(', 0, 'velocity_half', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_velocity.cpp', 'velocity_occlusion_shader=R"(', 0, 'velocity_occlusion', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_velocity.cpp', 'depth_out_shader=R"(', 0, 'depth_out', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_velocity.cpp', 'cloth_velocity_vertex_shader=R"(', 0, 'cloth_velocity', [('VS', 'vs_5_0')], STRICT, ['/Ges']),
    ('renderer/scene_pass_views.cpp', 'kDrawScanShader=R"(', 0, 'effect_draw_scan', [('CS', 'cs_5_0')], STRICT, ['/Ges']),
    ('renderer/overlay_pass.cpp', 'kShader[]=R"(', 0, 'overlay', [('VS', 'vs_5_1'), ('PS', 'ps_5_1'), ('PSScreen', 'ps_5_1')], STRICT, ['/Ges']),
    ('frame/game_texture_ingress.cpp', 'constexpr const char* shader=R"(', 0, 'facegen_composition', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
    ('frame/game_texture_ingress.cpp', 'constexpr const char* shader=R"(', 1, 'facegen_mark', [('VS', 'vs_5_0'), ('PS', 'ps_5_0')], STRICT, ['/Ges']),
]
tmp = tempfile.mkdtemp(prefix='r12h2-eq-'); same = differ = 0
for path, token, nth, hlsl, entries, rflags, fflags in ROWS:
    src = old_string(path, token, nth)
    for entry, profile in entries:
        old = compile_old(src, entry, profile, rflags)
        out = os.path.join(tmp, f'{hlsl}_{entry}.cso')
        subprocess.run([fxc, '/nologo', '/T', profile, '/E', entry] + fflags + ['/Fo', out, os.path.join(wt, 'native/renderer/shaders', hlsl + '.hlsl')], check=True, capture_output=True)
        new = open(out, 'rb').read()
        ok = old == new
        if built:
            hb = header_bytes(os.path.join(built, f'{hlsl}_{entry.lower()}.dxbc.h')); ok = ok and hb == old
        same += ok; differ += not ok
        print(f'{"same" if ok else "DIFF"} {hlsl}:{entry} {profile} {len(old)} {len(new)} {hashlib.sha256(new).hexdigest()[:16]}')
print(f'{same} identical, {differ} different ({dll}{", and the build folder " + built if built else ""})')
