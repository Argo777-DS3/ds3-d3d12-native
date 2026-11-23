// I-0135 (Thomas, 2026-11-12 05:40): a reader of Thomas's own Elden Ring archives, read-only, for local development scripts
// on his PC only (Read-EldenRingArchive.ps1, Extract-EldenRingLogo.ps1); never shipped, nothing it reads goes into Git or the cloud.
// The BHD5 index RSA-decrypted with the public keys the executable carries in clear; a file's BDT range read exactly and its
// keyed ranges AES-decrypted; a DCX (Kraken) decompressed through the game's own oo2core_6_win64.dll; a TPF's textures listed;
// BC1 decoded; the logo composed for DS3's page (ComposeLogo) and written as an uncompressed DDS.
using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
public static class ErArchive {
    public static List<Tuple<BigInteger, BigInteger>> Keys(string exe) {
        var text = Encoding.ASCII.GetString(File.ReadAllBytes(exe));
        var keys = new List<Tuple<BigInteger, BigInteger>>();
        const string head = "-----BEGIN RSA PUBLIC KEY-----", tail = "-----END RSA PUBLIC KEY-----";
        for (int at = text.IndexOf(head); at >= 0; at = text.IndexOf(head, at + 1)) {
            int end = text.IndexOf(tail, at); if (end < 0) break;
            var der = Convert.FromBase64String(text.Substring(at + head.Length, end - at - head.Length).Replace("\n", "").Replace("\r", "").Replace("\0", ""));
            int p = 0; Tag(der, ref p, 0x30); var n = Int(der, ref p); var e = Int(der, ref p);
            keys.Add(Tuple.Create(n, e));
        }
        return keys;
    }
    static int Len(byte[] d, ref int p) { int l = d[p++]; if (l < 0x80) return l; int k = l & 0x7F; l = 0; while (k-- > 0) l = (l << 8) | d[p++]; return l; }
    static void Tag(byte[] d, ref int p, byte t) { if (d[p++] != t) throw new InvalidDataException("DER"); Len(d, ref p); }
    static BigInteger Int(byte[] d, ref int p) { if (d[p++] != 2) throw new InvalidDataException("DER int"); int l = Len(d, ref p);
        var be = new byte[l + 1]; for (int i = 0; i < l; i++) be[l - 1 - i] = d[p + i]; p += l; return new BigInteger(be); }
    // Raw RSA with the public key, block by block (in: the modulus' size; out: one byte less), as the game reads its index.
    public static byte[] DecryptIndex(string bhd, Tuple<BigInteger, BigInteger> key) {
        var data = File.ReadAllBytes(bhd); var n = key.Item1; var e = key.Item2;
        int size = n.ToByteArray().Length; if (n.ToByteArray()[size - 1] == 0) size--;
        var outp = new MemoryStream();
        for (int at = 0; at + size <= data.Length; at += size) {
            var le = new byte[size + 1]; for (int i = 0; i < size; i++) le[i] = data[at + size - 1 - i];
            var m = BigInteger.ModPow(new BigInteger(le), e, n).ToByteArray();
            var block = new byte[size - 1]; for (int i = 0; i < size - 1 && i < m.Length; i++) block[size - 2 - i] = m[i];
            outp.Write(block, 0, block.Length);
        }
        return outp.ToArray();
    }
    public static ulong Hash(string path) { ulong h = 0; foreach (char c in path.ToLowerInvariant().Replace('\\', '/')) h = h * 0x85 + c; return h; }
    public class Entry { public ulong Hash; public int Padded, Unpadded; public long Offset, ShaOffset, AesOffset; public byte[] AesKey; public List<long[]> Ranges = new List<long[]>(); }
    // Elden Ring's BHD5: "BHD5", endianness, ..., bucket count at 0x10, bucket offset at 0x14; a bucket: count, offset; an entry:
    // u64 hash, i32 padded size, i32 unpadded size, i64 offset, i64 SHA block offset, i64 AES block offset (40 bytes).
    public static List<Entry> Entries(byte[] bhd) {
        if (Encoding.ASCII.GetString(bhd, 0, 4) != "BHD5") throw new InvalidDataException("not a BHD5");
        int buckets = BitConverter.ToInt32(bhd, 0x10), table = BitConverter.ToInt32(bhd, 0x14); var all = new List<Entry>();
        for (int b = 0; b < buckets; b++) {
            int count = BitConverter.ToInt32(bhd, table + b * 8), at = BitConverter.ToInt32(bhd, table + b * 8 + 4);
            for (int i = 0; i < count; i++, at += 40) {
                var en = new Entry { Hash = BitConverter.ToUInt64(bhd, at), Padded = BitConverter.ToInt32(bhd, at + 8), Unpadded = BitConverter.ToInt32(bhd, at + 12),
                    Offset = BitConverter.ToInt64(bhd, at + 16), ShaOffset = BitConverter.ToInt64(bhd, at + 24), AesOffset = BitConverter.ToInt64(bhd, at + 32) };
                if (en.AesOffset > 0) { en.AesKey = new byte[16]; Array.Copy(bhd, en.AesOffset, en.AesKey, 0, 16);
                    int ranges = BitConverter.ToInt32(bhd, (int)en.AesOffset + 16);
                    for (int r = 0; r < ranges; r++) en.Ranges.Add(new[] { BitConverter.ToInt64(bhd, (int)en.AesOffset + 20 + r * 16), BitConverter.ToInt64(bhd, (int)en.AesOffset + 28 + r * 16) }); }
                all.Add(en);
            }
        }
        return all;
    }
    public static byte[] Read(string bdt, Entry en) {
        var bytes = new byte[en.Padded];
        using (var f = new FileStream(bdt, FileMode.Open, FileAccess.Read, FileShare.Read)) { f.Seek(en.Offset, SeekOrigin.Begin); int got = 0; while (got < bytes.Length) { int k = f.Read(bytes, got, bytes.Length - got); if (k <= 0) throw new EndOfStreamException(); got += k; } }
        if (en.AesKey != null) using (var aes = Aes.Create()) { aes.Mode = CipherMode.ECB; aes.Padding = PaddingMode.None; aes.Key = en.AesKey;
            using (var dec = aes.CreateDecryptor()) foreach (var r in en.Ranges) { if (r[0] < 0 || r[1] <= r[0]) continue; int len = (int)(r[1] - r[0]); dec.TransformBlock(bytes, (int)r[0], len, bytes, (int)r[0]); } }
        if (en.Unpadded > 0 && en.Unpadded < bytes.Length) Array.Resize(ref bytes, en.Unpadded);
        return bytes;
    }
    [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)] static extern IntPtr LoadLibraryW(string path);
    [DllImport("kernel32")] static extern IntPtr GetProcAddress(IntPtr module, string name);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate long Decompress(byte[] comp, long compLen, byte[] raw, long rawLen, int fuzz, int crc, int verbosity, IntPtr a, long b, IntPtr c, IntPtr d, IntPtr e, long f, int phase);
    static Decompress oodle;
    public static byte[] Undcx(byte[] dcx, string oodlePath) {
        if (Encoding.ASCII.GetString(dcx, 0, 4) != "DCX\0") return null;
        int dcs = Find(dcx, "DCS\0"), dcp = Find(dcx, "DCP\0"), dca = Find(dcx, "DCA\0");
        int raw = BE(dcx, dcs + 4), comp = BE(dcx, dcs + 8), start = dca + BE(dcx, dca + 4);
        string kind = Encoding.ASCII.GetString(dcx, dcp + 4, 4);
        var outp = new byte[raw];
        if (kind == "KRAK") {
            if (oodle == null) { var m = LoadLibraryW(oodlePath); if (m == IntPtr.Zero) throw new IOException("oo2core not loaded");
                oodle = (Decompress)Marshal.GetDelegateForFunctionPointer(GetProcAddress(m, "OodleLZ_Decompress"), typeof(Decompress)); }
            var src = new byte[comp]; Array.Copy(dcx, start, src, 0, comp);
            long n = oodle(src, comp, outp, raw, 1, 0, 0, IntPtr.Zero, 0, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, 0, 3);
            if (n != raw) throw new InvalidDataException("Kraken: " + n + " of " + raw);
            return outp;
        }
        if (kind == "DFLT") { using (var z = new System.IO.Compression.DeflateStream(new MemoryStream(dcx, start + 2, comp - 2), System.IO.Compression.CompressionMode.Decompress)) { int got = 0, k; while ((k = z.Read(outp, got, raw - got)) > 0) got += k; } return outp; }
        throw new InvalidDataException("DCX " + kind);
    }
    static int Find(byte[] d, string s) { var p = Encoding.ASCII.GetBytes(s); for (int i = 0; i + p.Length <= d.Length && i < 0x100; i++) { bool ok = true; for (int j = 0; j < p.Length; j++) if (d[i + j] != p[j]) { ok = false; break; } if (ok) return i; } throw new InvalidDataException(s); }
    static int BE(byte[] d, int at) { return d[at] << 24 | d[at + 1] << 16 | d[at + 2] << 8 | d[at + 3]; }
    // A TPF's texture named `name` (UTF-16 or Shift-JIS names, PC entries of 20 bytes): its DDS bytes, or null.
    public static byte[] TpfTexture(byte[] tpf, string name) {
        if (Encoding.ASCII.GetString(tpf, 0, 4) != "TPF\0") throw new InvalidDataException("not a TPF");
        int count = BitConverter.ToInt32(tpf, 8), encoding = tpf[14], pos = 16;
        for (int i = 0; i < count; i++) {
            int off = BitConverter.ToInt32(tpf, pos), size = BitConverter.ToInt32(tpf, pos + 4), nameAt = BitConverter.ToInt32(tpf, pos + 12), floats = BitConverter.ToInt32(tpf, pos + 16);
            pos += 20; if (floats == 1) pos += 8 + BitConverter.ToInt32(tpf, pos + 4);
            string n;
            if (encoding == 1) { int end = nameAt; while (tpf[end] != 0 || tpf[end + 1] != 0) end += 2; n = Encoding.Unicode.GetString(tpf, nameAt, end - nameAt); }
            else { int end = nameAt; while (tpf[end] != 0) end++; n = Encoding.ASCII.GetString(tpf, nameAt, end - nameAt); }
            if (string.Equals(n, name, StringComparison.OrdinalIgnoreCase)) { var dds = new byte[size]; Array.Copy(tpf, off, dds, 0, size); return dds; }
        }
        return null;
    }
    // A DXT1 DDS to RGBA8 rows (the same decoding as native/frame/dark_logo.h's DecodeBc1).
    public static byte[] DecodeBc1Dds(byte[] dds, out int width, out int height) {
        if (Encoding.ASCII.GetString(dds, 0, 4) != "DDS " || Encoding.ASCII.GetString(dds, 84, 4) != "DXT1") throw new InvalidDataException("a DXT1 DDS is expected");
        height = BitConverter.ToInt32(dds, 12); width = BitConverter.ToInt32(dds, 16);
        if (width % 4 != 0 || height % 4 != 0) throw new InvalidDataException("size");
        var rgba = new byte[width * height * 4]; int pos = 128;
        for (int by = 0; by < height / 4; by++) for (int bx = 0; bx < width / 4; bx++, pos += 8) {
            int c0 = dds[pos] | dds[pos + 1] << 8, c1 = dds[pos + 2] | dds[pos + 3] << 8; uint bits = BitConverter.ToUInt32(dds, pos + 4);
            var pal = new int[4, 4];
            Expand(c0, pal, 0); Expand(c1, pal, 1);
            for (int k = 0; k < 3; k++) {
                if (c0 > c1) { pal[2, k] = (2 * pal[0, k] + pal[1, k]) / 3; pal[3, k] = (pal[0, k] + 2 * pal[1, k]) / 3; }
                else pal[2, k] = (pal[0, k] + pal[1, k]) / 2; }
            pal[2, 3] = 255; pal[3, 3] = c0 > c1 ? 255 : 0;
            for (int i = 0; i < 16; i++) { int idx = (int)((bits >> (2 * i)) & 3), at = ((by * 4 + i / 4) * width + bx * 4 + i % 4) * 4;
                for (int k = 0; k < 4; k++) rgba[at + k] = (byte)pal[idx, k]; }
        }
        return rgba;
    }
    static void Expand(int c, int[,] pal, int row) { pal[row, 0] = ((c >> 11) & 31) * 255 / 31; pal[row, 1] = ((c >> 5) & 63) * 255 / 63; pal[row, 2] = (c & 31) * 255 / 31; pal[row, 3] = 255; }
    // Elden Ring's page (05_901_logo_bne.gfx, read statically): the logo, 1920x440, shown at half size (960x220) at (480, 446) on
    // black. DS3's page draws its image as a 768x768 square centred on the stage (576..1344, 156..924), so the logo is fitted to
    // that square's width at its own aspect (768x176, area-averaged) and centred where Elden Ring centres it (y 556, the square's
    // row 400), on opaque black: a straight-alpha RGBA8 square for DarkLogoAdapt.
    public static byte[] ComposeLogo(byte[] src, int sw, int sh, int canvas, int centreRow) {
        int dw = canvas, dh = (int)Math.Round((double)sh * canvas / sw), top = centreRow - dh / 2;
        if (top < 0 || top + dh > canvas) throw new InvalidDataException("the logo does not fit the square");
        var outp = new byte[canvas * canvas * 4]; for (int i = 3; i < outp.Length; i += 4) outp[i] = 255;
        double fx = (double)sw / dw, fy = (double)sh / dh;
        for (int y = 0; y < dh; y++) {
            double y0 = y * fy, y1 = y0 + fy;
            for (int x = 0; x < dw; x++) {
                double x0 = x * fx, x1 = x0 + fx; double r = 0, g = 0, b = 0, area = 0;
                for (int sy = (int)y0; sy < Math.Min(sh, (int)Math.Ceiling(y1)); sy++) {
                    double wy = Math.Min(y1, sy + 1) - Math.Max(y0, sy); if (wy <= 0) continue;
                    for (int sx = (int)x0; sx < Math.Min(sw, (int)Math.Ceiling(x1)); sx++) {
                        double wx = Math.Min(x1, sx + 1) - Math.Max(x0, sx); if (wx <= 0) continue;
                        double w = wx * wy; int at = (sy * sw + sx) * 4; r += src[at] * w; g += src[at + 1] * w; b += src[at + 2] * w; area += w; } }
                int o = ((top + y) * canvas + x) * 4;
                outp[o] = (byte)Math.Round(r / area); outp[o + 1] = (byte)Math.Round(g / area); outp[o + 2] = (byte)Math.Round(b / area);
            }
        }
        return outp;
    }
    // An uncompressed DDS (RGBA8 in byte order R, G, B, A: DXGI R8G8B8A8_UNORM), one mip.
    public static byte[] Dds(byte[] rgba, int width, int height) {
        var d = new byte[128 + rgba.Length]; Encoding.ASCII.GetBytes("DDS ").CopyTo(d, 0);
        Action<int, uint> put = (at, v) => BitConverter.GetBytes(v).CopyTo(d, at);
        put(4, 124); put(8, 0x100F); put(12, (uint)height); put(16, (uint)width); put(20, (uint)width * 4); put(28, 1);
        put(76, 32); put(80, 0x41); put(88, 32); put(92, 0xFF); put(96, 0xFF00); put(100, 0xFF0000); put(104, 0xFF000000u); put(108, 0x1000);
        rgba.CopyTo(d, 128); return d;
    }
}
