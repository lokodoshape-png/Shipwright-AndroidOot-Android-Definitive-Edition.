package com.dishii.soh;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Random;

// OOT D.E.: tests of SafeFiles (OTR, ROM and mod checks, safe replacement) that run on a computer.
// Run with scripts/tests/run_java_tests.sh.
public class SafeFilesTest {
    private static int failures = 0;

    private static void check(boolean ok, String what) {
        System.out.println((ok ? "PASS " : "FAIL ") + what);
        if (!ok) failures++;
    }

    private static File write(File dir, String name, byte[] data) throws IOException {
        File f = new File(dir, name);
        try (FileOutputStream out = new FileOutputStream(f)) {
            out.write(data);
        }
        return f;
    }

    // A fake ROM in .z64 order: random bytes with the z64 first byte.
    private static byte[] fakeRom(long size, long seed) {
        byte[] rom = new byte[(int) size];
        new Random(seed).nextBytes(rom);
        rom[0] = (byte) 0x80; rom[1] = 0x37; rom[2] = 0x12; rom[3] = 0x40;
        return rom;
    }

    private static byte[] toV64(byte[] z64) {
        byte[] r = z64.clone();
        for (int i = 0; i + 1 < r.length; i += 2) { byte t = r[i]; r[i] = r[i + 1]; r[i + 1] = t; }
        return r;
    }

    private static byte[] toN64(byte[] z64) {
        byte[] r = new byte[z64.length];
        for (int i = 0; i + 3 < r.length; i += 4) {
            r[i] = z64[i + 3]; r[i + 1] = z64[i + 2]; r[i + 2] = z64[i + 1]; r[i + 3] = z64[i];
        }
        return r;
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 2 && args[0].equals("--make-rom")) {
            // Writes the fake ROM of the test, for the Python reference checksum.
            write(new File(args[1]).getParentFile(), new File(args[1]).getName(), fakeRom(32 * SafeFiles.MB, 1));
            return;
        }
        File dir = Files.createTempDirectory("safefiles").toFile();

        // CRC32C test vector (RFC 3720): "123456789" -> 0xE3069283.
        byte[] v = "123456789".getBytes("US-ASCII");
        check(~SafeFiles.crc32cUpdate(0xFFFFFFFF, v, 0, v.length) == 0xE3069283, "CRC32C test vector");

        // ROM: the same ROM in the three byte orders gives the same checksum (as RomToBigEndian + CRC32C).
        byte[] z64 = fakeRom(32 * SafeFiles.MB, 1);
        File fz = write(dir, "a.z64", z64);
        File fv = write(dir, "a.v64", toV64(z64));
        File fn = write(dir, "a.n64", toN64(z64));
        int cz = SafeFiles.romCrc32c(fz, 0x80);
        check(cz == ~SafeFiles.crc32cUpdate(0xFFFFFFFF, z64, 0, z64.length), "ROM CRC32C equals one-shot CRC32C");
        check(SafeFiles.romCrc32c(fv, 0x37) == cz, "v64 byte order converted like the extractor");
        check(SafeFiles.romCrc32c(fn, 0x40) == cz, "n64 byte order converted like the extractor");
        if (args.length > 0) {
            // Cross-check with an independent implementation (scripts/tests/run_java_tests.sh passes Python's result).
            check(cz == (int) Long.parseLong(args[0], 16), "ROM CRC32C equals the Python reference");
        }

        String p = SafeFiles.checkRomFile(fz, z64.length);
        check(p != null && p.contains("compatíveis"), "unknown ROM rejected by checksum: " + p);
        p = SafeFiles.checkRomFile(fz, z64.length + 10);
        check(p != null && p.contains("incompleta"), "short copy rejected: " + p);
        File small = write(dir, "small.z64", fakeRom(SafeFiles.MB, 2));
        p = SafeFiles.checkRomFile(small, -1);
        check(p != null && p.contains("32, 54 ou 64"), "wrong size rejected: " + p);
        byte[] zip = fakeRom(32 * SafeFiles.MB, 3);
        zip[0] = 'P'; zip[1] = 'K'; zip[2] = 3; zip[3] = 4;
        p = SafeFiles.checkRomFile(write(dir, "rom.zip", zip), -1);
        check(p != null && p.contains("compactado"), "zip rejected: " + p);
        byte[] junk = fakeRom(32 * SafeFiles.MB, 4);
        junk[0] = 0x12;
        p = SafeFiles.checkRomFile(write(dir, "junk.z64", junk), -1);
        check(p != null && p.contains("cabeçalho"), "unknown header rejected: " + p);
        check(SafeFiles.checkRomFile(write(dir, "empty.z64", new byte[0]), -1) != null, "empty file rejected");

        // replaceFileSafely: the target gets the new content, no .bak left behind.
        File target = write(dir, "OOT.z64", "old".getBytes());
        File tmp = write(dir, "OOT.z64.tmp", "new".getBytes());
        SafeFiles.replaceFileSafely(tmp, target);
        check(new String(Files.readAllBytes(target.toPath())).equals("new") && !tmp.exists()
                && !new File(dir, "OOT.z64.bak").exists(), "replaceFileSafely replaces and cleans up");
        // A missing source leaves the old target in place.
        try {
            SafeFiles.replaceFileSafely(new File(dir, "missing.tmp"), target);
            check(false, "replaceFileSafely fails when the source is missing");
        } catch (IOException e) {
            check(new String(Files.readAllBytes(target.toPath())).equals("new"), "old file kept when the move fails");
        }

        // OTR (MPQ) check.
        if (args.length > 1) {
            File otr = new File(args[1]);
            check(SafeFiles.checkOtrFile(otr, otr.length()) == null, "committed soh.otr passes");
            byte[] d = Files.readAllBytes(otr.toPath());
            File half = new File(dir, "half.otr");
            try (FileOutputStream out = new FileOutputStream(half)) { out.write(d, 0, d.length / 2); }
            check(SafeFiles.checkOtrFile(half, -1) != null, "truncated soh.otr rejected");
        }
        check(SafeFiles.checkOtrFile(write(dir, "z.otr", new byte[64]), -1) != null, "non-MPQ file rejected");
        check(SafeFiles.checkOtrFile(new File(dir, "none.otr"), -1) != null, "missing file rejected");

        // Mods.
        File zipFile = new File(dir, "mod.o2r");
        try (java.util.zip.ZipOutputStream z = new java.util.zip.ZipOutputStream(new FileOutputStream(zipFile))) {
            z.putNextEntry(new java.util.zip.ZipEntry("textures/a"));
            z.write(new byte[5000]);
            z.closeEntry();
        }
        check(SafeFiles.checkModFile(zipFile, "mod.o2r") == null, "valid .o2r accepted");
        byte[] zd = Files.readAllBytes(zipFile.toPath());
        File cut = new File(dir, "cut.o2r");
        try (FileOutputStream out = new FileOutputStream(cut)) { out.write(zd, 0, zd.length - 30); }
        check(SafeFiles.checkModFile(cut, "cut.o2r") != null, "truncated .o2r rejected");
        check(SafeFiles.checkModFile(write(dir, "bad.otr", new byte[100]), "bad.otr") != null, "invalid .otr rejected");
        check(SafeFiles.checkModFile(zipFile, "mod.rar") != null, "unsupported extension rejected");
        if (args.length > 1) {
            check(SafeFiles.checkModFile(new File(args[1]), "soh.otr") == null, "valid .otr accepted");
        }
        File copy = write(dir, "copy.o2r", zd);
        check(SafeFiles.sameContent(zipFile, copy), "same content detected");
        check(!SafeFiles.sameContent(zipFile, cut), "different content detected");
        File mods = new File(dir, "mods");
        mods.mkdirs();
        write(mods, "Letras.otr", new byte[1]);
        write(mods, "Letras (2).otr.desativado", new byte[1]);
        check(SafeFiles.uniqueModName(mods, "Letras.otr", ".desativado").equals("Letras (3).otr"),
                "unique name skips names in use (also switched off)");

        System.out.println(failures == 0 ? "ALL TESTS PASSED" : failures + " TEST(S) FAILED");
        System.exit(failures == 0 ? 0 : 1);
    }
}
