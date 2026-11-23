[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Dir,          # folder of recovered menu .gfx files (uncompressed GFX, Scaleform)
    [Parameter(Mandatory = $true)][string]$Out           # output .tsv (one row per file)
)
# I-0067: Scaleform feature inventory of the menu movies, by page. Read-only on the files; no game process.
# The movies are plain "GFX" files (the Flash tag stream). The reader walks every tag, recurses into sprites, parses
# PlaceObject2/3 (matrix, colour transform, clip depth, filter list, blend mode, cache-as-bitmap) and the style arrays and
# edge records of DefineShape1..4, and records what the native Scaleform HAL (MENUS-MAP section 36) has to draw:
# fill kinds, blend modes, filters, masks, 9-slice grids, external images, glyph textures, video, morphs.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

public class GfxFeatures {
    public Dictionary<string, int> C = new Dictionary<string, int>();
    public List<string> External = new List<string>();
    public List<string> FontTex = new List<string>();
    public List<string> Imports = new List<string>();
    public List<string> FilterSigs = new List<string>();
    byte[] d; int bitPos; // bit cursor within d (absolute bit index) for bit reads

    void Inc(string k, int n = 1) { int v; C.TryGetValue(k, out v); C[k] = v + n; }

    // --- bit helpers (absolute byte position p kept by caller)
    int p;
    uint Bits(int n) { uint v = 0; for (int i = 0; i < n; i++) { int byt = bitPos >> 3; int sh = 7 - (bitPos & 7); v = (v << 1) | (uint)((d[byt] >> sh) & 1); bitPos++; } return v; }
    int SBits(int n) { if (n == 0) return 0; uint v = Bits(n); if ((v & (1u << (n - 1))) != 0) return (int)(v | (uint.MaxValue << n)); return (int)v; }
    void Align() { bitPos = (bitPos + 7) & ~7; }
    void BeginBits() { bitPos = p * 8; }
    void EndBits() { Align(); p = bitPos >> 3; }

    void Rect() { BeginBits(); int n = (int)Bits(5); Bits(n); Bits(n); Bits(n); Bits(n); EndBits(); }
    void Matrix() {
        BeginBits();
        if (Bits(1) == 1) { int n = (int)Bits(5); Bits(n); Bits(n); }
        if (Bits(1) == 1) { int n = (int)Bits(5); Bits(n); Bits(n); }
        int t = (int)Bits(5); Bits(t); Bits(t); EndBits();
    }
    bool CxformAlpha() {
        BeginBits(); uint hasAdd = Bits(1), hasMul = Bits(1); int n = (int)Bits(4);
        if (hasMul == 1) { Bits(n); Bits(n); Bits(n); Bits(n); }
        if (hasAdd == 1) { Bits(n); Bits(n); Bits(n); Bits(n); }
        EndBits(); return hasAdd == 1;
    }
    int U8() { return d[p++]; }
    int U16() { int v = d[p] | (d[p + 1] << 8); p += 2; return v; }
    uint U32() { uint v = BitConverter.ToUInt32(d, p); p += 4; return v; }
    string Str() { int s = p; while (d[p] != 0) p++; string r = Encoding.UTF8.GetString(d, s, p - s); p++; return r; }

    // --- shapes
    void FillStyles(int shapeKind) {
        int n = U8(); if (n == 0xFF && shapeKind >= 2) n = U16();
        for (int i = 0; i < n; i++) FillStyle(shapeKind);
    }
    void Color(int shapeKind) { p += shapeKind >= 3 ? 4 : 3; }
    void Gradient(int shapeKind, bool focal) {
        int b = U8(); int cnt = b & 0x0F; int spread = (b >> 6) & 3; int interp = (b >> 4) & 3;
        if (spread != 0) Inc("gradient.spread." + spread); if (interp != 0) Inc("gradient.interp.linearRGB");
        for (int i = 0; i < cnt; i++) { p++; Color(shapeKind); }
        if (focal) p += 2;
        Inc("gradient.stops", cnt);
    }
    void FillStyle(int shapeKind) {
        int t = U8();
        if (t == 0x00) { Color(shapeKind); Inc("fill.solid"); }
        else if (t == 0x10) { Matrix(); Gradient(shapeKind, false); Inc("fill.linear"); }
        else if (t == 0x12) { Matrix(); Gradient(shapeKind, false); Inc("fill.radial"); }
        else if (t == 0x13) { Matrix(); Gradient(shapeKind, true); Inc("fill.focal"); }
        else if (t >= 0x40 && t <= 0x43) { U16(); Matrix(); Inc("fill.bitmap." + t.ToString("x2")); }
        else { Inc("fill.unknown." + t.ToString("x2")); throw new InvalidDataException("fill " + t); }
    }
    void LineStyles(int shapeKind) {
        int n = U8(); if (n == 0xFF) n = U16();
        for (int i = 0; i < n; i++) {
            if (shapeKind == 4) {
                U16(); int f1 = U8(); int f2 = U8(); // width, flags
                bool join = ((f1 >> 4) & 3) == 2; if (join) p += 2; // miter limit
                if (((f2 >> 0) & 1) == 1 || false) { }
                bool hasFill = (f1 & 0x08) != 0; // HasFillFlag is bit 3 of the first flag byte (bits: StartCap2 JoinStyle2 HasFill NoHScale NoVScale Pixel)
                if (hasFill) FillStyle(shapeKind); else Color(shapeKind);
                Inc("line.style4"); if (hasFill) Inc("line.filled");
            } else { U16(); Color(shapeKind); Inc("line"); }
        }
    }
    void ShapeRecords(int shapeKind) {
        int fillBits, lineBits;
        BeginBits(); // we are byte aligned here: read NumFillBits/NumLineBits
        fillBits = (int)Bits(4); lineBits = (int)Bits(4);
        long edges = 0;
        while (true) {
            if (Bits(1) == 0) {
                uint newStyles = Bits(1), lineSt = Bits(1), fill1 = Bits(1), fill0 = Bits(1), moveTo = Bits(1);
                if (newStyles == 0 && lineSt == 0 && fill1 == 0 && fill0 == 0 && moveTo == 0) break;
                if (moveTo == 1) { int n = (int)Bits(5); Bits(n); Bits(n); }
                if (fill0 == 1) Bits(fillBits);
                if (fill1 == 1) Bits(fillBits);
                if (lineSt == 1) Bits(lineBits);
                if (newStyles == 1 && shapeKind >= 2) {
                    EndBits(); FillStyles(shapeKind); LineStyles(shapeKind); BeginBits(); fillBits = (int)Bits(4); lineBits = (int)Bits(4); Inc("shape.newstyles");
                }
                Inc("shape.paths");
            } else {
                edges++;
                if (Bits(1) == 1) { int n = (int)Bits(4) + 2; if (Bits(1) == 1) { Bits(n); Bits(n); } else { Bits(1); Bits(n); } }
                else { int n = (int)Bits(4) + 2; Bits(n); Bits(n); Bits(n); Bits(n); }
            }
        }
        EndBits();
        Inc("shape.edges", (int)edges);
    }
    void DefineShape(int shapeKind, int end) {
        U16(); Rect();
        if (shapeKind == 4) { Rect(); p++; }
        FillStyles(shapeKind); LineStyles(shapeKind); ShapeRecords(shapeKind);
        Inc("shape");
    }

    // --- place object
    static readonly string[] FilterNames = { "dropshadow", "blur", "glow", "bevel", "gradientglow", "convolution", "colormatrix", "gradientbevel" };
    static readonly string[] BlendNames = { "normal0", "normal1", "layer", "multiply", "screen", "lighten", "darken", "difference", "add", "subtract", "invert", "alpha", "erase", "overlay", "hardlight" };
    void Filters() {
        int n = U8();
        for (int i = 0; i < n; i++) {
            int id = U8(); if (id < FilterNames.Length) Inc("filter." + FilterNames[id]); else Inc("filter.unknown." + id);
            switch (id) {
                case 0: { int col = (int)BitConverter.ToUInt32(d, p); double bx = BitConverter.ToInt32(d, p + 4) / 65536.0, by = BitConverter.ToInt32(d, p + 8) / 65536.0, ang = BitConverter.ToInt32(d, p + 12) / 65536.0, dist = BitConverter.ToInt32(d, p + 16) / 65536.0, str = BitConverter.ToInt16(d, p + 20) / 256.0; int fl = d[p + 22];
                    FilterSigs.Add(string.Format("dropshadow rgba={0:x8} blur={1:0.##}x{2:0.##} angle={3:0.#} dist={4:0.##} strength={5:0.##} inner={6} knockout={7} composite={8} passes={9}", col, bx, by, ang, dist, str, (fl >> 7) & 1, (fl >> 6) & 1, (fl >> 5) & 1, fl & 31)); p += 23; break; }
                case 1: { double bx = BitConverter.ToInt32(d, p) / 65536.0, by = BitConverter.ToInt32(d, p + 4) / 65536.0; int fl = d[p + 8];
                    FilterSigs.Add(string.Format("blur blur={0:0.##}x{1:0.##} passes={2}", bx, by, fl >> 3)); p += 9; break; }
                case 2: { int col = (int)BitConverter.ToUInt32(d, p); double bx = BitConverter.ToInt32(d, p + 4) / 65536.0, by = BitConverter.ToInt32(d, p + 8) / 65536.0, str = BitConverter.ToInt16(d, p + 12) / 256.0; int fl = d[p + 14];
                    FilterSigs.Add(string.Format("glow rgba={0:x8} blur={1:0.##}x{2:0.##} strength={3:0.##} inner={4} knockout={5} composite={6} passes={7}", col, bx, by, str, (fl >> 7) & 1, (fl >> 6) & 1, (fl >> 5) & 1, fl & 31)); p += 15; break; }
                case 3: p += 27; break;
                case 4: case 7: { int nc = U8(); p += 5 * nc + 16 + 2 + 1; break; }
                case 5: { int mx = U8(); int my = U8(); p += 8 + 4 * mx * my + 4 + 1; break; }
                case 6: p += 80; break;
                default: throw new InvalidDataException("filter " + id);
            }
        }
    }
    void PlaceObject(int tag, int end) {
        int f1 = U8(); int f2 = 0;
        if (tag == 70) f2 = U8();
        int depth = U16();
        if (tag == 70 && ((f2 & 0x08) != 0 || ((f2 & 0x10) != 0 && (f1 & 0x02) != 0))) Str(); // class name
        if ((f1 & 0x02) != 0) { U16(); Inc("place.character"); }
        if ((f1 & 0x04) != 0) Matrix();
        if ((f1 & 0x08) != 0) { if (CxformAlpha()) Inc("place.cxform.add"); else Inc("place.cxform.mul"); }
        if ((f1 & 0x10) != 0) { U16(); Inc("place.ratio"); }
        if ((f1 & 0x20) != 0) Str();
        if ((f1 & 0x40) != 0) { U16(); Inc("place.clipdepth"); }
        if (tag == 70) {
            if ((f2 & 0x01) != 0) Filters();
            if ((f2 & 0x02) != 0) { int b = U8(); if (b < BlendNames.Length) Inc("blend." + BlendNames[b]); else Inc("blend.unknown." + b); }
            if ((f2 & 0x04) != 0) { U8(); Inc("place.cacheasbitmap"); }
        }
        Inc("place");
    }

    void Tags(int start, int end, bool inSprite) {
        p = start;
        while (p < end) {
            int hdr = U16(); int tag = hdr >> 6; int len = hdr & 0x3F; if (len == 0x3F) len = (int)U32();
            int body = p; int tend = body + len; if (tend > end) throw new InvalidDataException("tag overrun " + tag);
            Inc("tag." + tag);
            try {
                switch (tag) {
                    case 0: p = tend; return;
                    case 2: DefineShape(1, tend); break;
                    case 22: DefineShape(2, tend); break;
                    case 32: DefineShape(3, tend); break;
                    case 83: DefineShape(4, tend); break;
                    case 46: case 84: Inc("morph"); break;
                    case 6: case 21: case 35: case 90: Inc("image.jpeg"); break;
                    case 20: case 36: Inc("image.lossless"); break;
                    case 10: case 48: case 75: case 91: Inc("font.swf"); break;
                    case 11: case 33: Inc("text.static"); break;
                    case 37: Inc("text.edit"); break;
                    case 7: case 34: Inc("button"); break;
                    case 60: case 61: Inc("video"); break;
                    case 14: case 15: case 17: case 18: case 19: case 45: Inc("sound"); break;
                    case 12: Inc("as2.action"); break;
                    case 59: Inc("as2.initaction"); break;
                    case 82: Inc("as3.abc"); break;
                    case 78: Inc("scalinggrid"); break;
                    case 26: case 70: PlaceObject(tag, tend); break;
                    case 39: { U16(); U16(); Inc("sprite"); Tags(p, tend, true); break; }
                    case 1001: case 1009: { External.Add(Printable(body, tend) + "|hdr" + BitConverter.ToString(d, body, Math.Min(8, tend - body))); Inc("external.image"); break; }
                    case 1002: { FontTex.Add(Printable(body, tend) + "|hdr" + BitConverter.ToString(d, body, Math.Min(8, tend - body))); Inc("font.texture"); break; }
                    case 71: case 57: { Imports.Add(Printable(body, tend)); Inc("import.assets"); break; }
                    case 1005: Inc("font.compacted"); break;
                    case 1008: Inc("subimage"); break;
                    case 1000: Inc("gfx.exporterinfo"); break;
                    default: break;
                }
            } catch (Exception ex) { Inc("parse.error." + tag + "." + ex.GetType().Name); }
            p = tend;
        }
    }

    string Printable(int s, int e) {
        var sb = new StringBuilder(); var cur = new StringBuilder();
        for (int i = s; i < e; i++) { char c = (char)d[i]; if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '/' || c == '-') cur.Append(c); else { if (cur.Length >= 4) { if (sb.Length > 0) sb.Append(','); sb.Append(cur); } cur.Clear(); } }
        if (cur.Length >= 4) { if (sb.Length > 0) sb.Append(','); sb.Append(cur); }
        return sb.ToString();
    }
    public string Run(string path) {
        d = File.ReadAllBytes(path);
        string sig = Encoding.ASCII.GetString(d, 0, 3);
        if (sig != "GFX") throw new InvalidDataException("signature " + sig);
        int ver = d[3]; C["version"] = ver; C["bytes"] = d.Length;
        p = 8; Rect(); U16(); int frames = U16(); C["frames"] = frames; C["framerate"] = 0;
        Tags(p, d.Length, false);
        return "ok";
    }
}
'@
$all = [System.Collections.Generic.SortedSet[string]]::new()
$rows = @()
foreach ($f in Get-ChildItem -LiteralPath $Dir -Filter *.gfx | Sort-Object Name) {
    $g = [GfxFeatures]::new()
    try { [void]$g.Run($f.FullName) } catch { $g.C['fatal.error'] = 1 }
    foreach ($k in $g.C.Keys) { [void]$all.Add($k) }
    $rows += [pscustomobject]@{ name = $f.Name; g = $g }
}
$keep = $all | Where-Object { $_ -notlike 'tag.*' -or $_ -in @('tag.82', 'tag.12') }
$lines = @("name`t" + (($keep) -join "`t") + "`texternal_images`tfont_textures`timports`tfilters")
foreach ($r in $rows) {
    $vals = foreach ($k in $keep) { $v = 0; [void]$r.g.C.TryGetValue($k, [ref]$v); $v }
    $lines += $r.name + "`t" + ($vals -join "`t") + "`t" + ($r.g.External -join ';') + "`t" + ($r.g.FontTex -join ';') + "`t" + ($r.g.Imports -join ';') + "`t" + ($r.g.FilterSigs -join ';')
}
[IO.File]::WriteAllLines($Out, $lines)
"{0} files, {1} columns -> {2}" -f $rows.Count, @($keep).Count, $Out



