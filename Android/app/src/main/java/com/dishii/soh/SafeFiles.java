package com.dishii.soh;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * OOT D.E.: file checks and safe replacement used by the setup, the ROM import and the mod install.
 *
 * Plain Java with no Android classes, so it can be tested on a computer (see scripts/tests/). Nothing here ever writes
 * over a good file before its replacement is complete and checked.
 */
final class SafeFiles {
    private SafeFiles() {
    }

    /**
     * Basic check of an .otr archive (MPQ): it exists, is not empty, starts with the MPQ signature and is at least as
     * long as the archive size written in its header. expectedLength >= 0 also checks the exact length of a copy.
     * Returns null when the file looks complete, or what is wrong with it.
     */
    static String checkOtrFile(File file, long expectedLength) {
        if (!file.isFile()) {
            return "arquivo ausente";
        }
        long length = file.length();
        if (length < 32) {
            return "arquivo vazio ou incompleto";
        }
        if (expectedLength >= 0 && length != expectedLength) {
            return "tamanho " + length + " em vez de " + expectedLength;
        }
        byte[] header = new byte[32];
        try (InputStream in = new FileInputStream(file)) {
            int read = 0;
            while (read < header.length) {
                int count = in.read(header, read, header.length - read);
                if (count < 0) break;
                read += count;
            }
            if (read < header.length) {
                return "cabeçalho incompleto";
            }
        } catch (IOException e) {
            return "não foi possível ler (" + e.getMessage() + ")";
        }
        // "MPQ\x1A" archive header, or "MPQ\x1B" user data header in front of it.
        if (header[0] != 'M' || header[1] != 'P' || header[2] != 'Q' || (header[3] != 0x1A && header[3] != 0x1B)) {
            return "não é um arquivo OTR (MPQ)";
        }
        if (header[3] == 0x1A) {
            long archiveSize = (header[8] & 0xFFL) | ((header[9] & 0xFFL) << 8) | ((header[10] & 0xFFL) << 16)
                    | ((header[11] & 0xFFL) << 24);
            if (archiveSize > length) {
                return "arquivo cortado (" + length + " de " + archiveSize + " bytes)";
            }
        }
        return null;
    }

    // Writes everything buffered for `out` to the storage before the file is renamed into place.
    static void syncToDisk(FileOutputStream out) {
        try {
            out.getFD().sync();
        } catch (IOException e) {
            System.err.println("OOTDE_FILES: sync failed: " + e);
        }
    }

    /**
     * Puts `source`, a complete temporary file, in the place of `target`. The previous target is renamed to
     * target.bak first and put back if the new file cannot be moved in, so there is always one complete copy.
     */
    static void replaceFileSafely(File source, File target) throws IOException {
        File backup = new File(target.getPath() + ".bak");
        if (backup.exists() && !backup.delete()) {
            throw new IOException("Não foi possível apagar " + backup.getName());
        }
        boolean hadTarget = target.exists();
        if (hadTarget && !target.renameTo(backup)) {
            throw new IOException("Não foi possível substituir " + target.getName());
        }
        if (!source.renameTo(target)) {
            if (hadTarget && !backup.renameTo(target)) {
                System.err.println("OOTDE_FILES: could not restore " + target + " from " + backup);
            }
            throw new IOException("Não foi possível gravar " + target.getName());
        }
        if (hadTarget && !backup.delete()) {
            System.err.println("OOTDE_FILES: could not delete " + backup);
        }
    }

    // --- ROM ----------------------------------------------------------------------------------------------------

    static final long MB = 1024L * 1024L;
    // Version CRC (header offset 0x10) of the PAL GameCube Master Quest debug ROM; Extract.cpp fixes byte 0x3E of
    // that ROM before its checksum is computed.
    private static final int OOT_PAL_GC_MQ_DBG = 0x917D18F6;

    // CRC32C of every ROM the extractor accepts, in big-endian (.z64) byte order. Copied from goodCrcs in
    // soh/soh/Extractor/Extract.cpp, the list the extractor itself checks; keep both lists the same.
    static final int[] GOOD_ROM_CRCS = {
            0xFA8C0555, // MQ DBG 64MB (Original overdump)
            0x8652AC4C, // MQ DBG 64MB
            0x5B8A1EB7, // MQ DBG 64MB (Empty overdump)
            0x1F731FFE, // MQ DBG 54MB
            0x044B3982, // NMQ DBG 54MB
            0xEB15D7B9, // NMQ DBG 64MB
            0xDA8E61BF, // GC PAL
            0x7A2FAE68, // GC MQ PAL
            0xFD9913B1, // N64 PAL 1.0
            0xE033FBBA, // N64 PAL 1.1
            0x460C938C, // N64 NTSC US 1.0
            0xD0C76FA9, // N64 NTSC JP 1.0
            0x3496EE47, // N64 NTSC US 1.1
            0xA25D1262, // N64 NTSC JP 1.1
            0x15736A58, // N64 NTSC US 1.2
            0x83B8967D, // N64 NTSC JP 1.2
            0xD61453DE, // GC NTSC US
            0x4129C825, // GC MQ NTSC US
            0x11A4BE61, // GC NTSC JP
            0x2BC6C6FD, // GC NTSC JP Collector's Edition
            0x02CD974C, // GC MQ NTSC JP
    };

    /**
     * Checks a ROM file before it replaces OOT.z64, with the same rules as the extractor (Extract.cpp ValidateRom):
     * not a zip/rar/7z archive, a size of 32, 54 or 64 MB, and a CRC32C of the whole ROM (after converting .v64/.n64
     * to big-endian order) that is in GOOD_ROM_CRCS. expectedLength >= 0 also checks the number of bytes copied.
     * Returns null when the ROM is valid, or the reason it is not.
     */
    static String checkRomFile(File file, long expectedLength) throws IOException {
        long length = file.length();
        if (expectedLength >= 0 && length != expectedLength) {
            return "a cópia ficou incompleta (" + length + " de " + expectedLength + " bytes)";
        }
        byte[] head = new byte[8];
        try (InputStream in = new FileInputStream(file)) {
            readFully(in, head, 0, head.length);
        } catch (java.io.EOFException e) {
            return "o arquivo está vazio ou incompleto";
        }
        if ((head[0] == 'P' && head[1] == 'K' && head[2] == 3 && head[3] == 4)
                || (head[0] == 'R' && head[1] == 'a' && head[2] == 'r' && head[3] == '!')
                || (head[0] == '7' && head[1] == 'z' && (head[2] & 0xFF) == 0xBC && (head[3] & 0xFF) == 0xAF)) {
            return "o arquivo está compactado (zip, rar ou 7z). Extraia a ROM antes de importar";
        }
        if (length != 32 * MB && length != 54 * MB && length != 64 * MB) {
            return "tamanho de " + (length / MB) + " MB (" + length + " bytes); a ROM deve ter 32, 54 ou 64 MB";
        }
        int order = head[0] & 0xFF;
        if (order != 0x80 && order != 0x37 && order != 0x40) {
            return "não parece uma ROM de Nintendo 64 (cabeçalho desconhecido)";
        }
        int crc = romCrc32c(file, order);
        for (int good : GOOD_ROM_CRCS) {
            if (crc == good) {
                return null;
            }
        }
        return String.format("a ROM não está na lista de versões compatíveis (CRC32C %08X). Veja as versões aceitas em "
                + "https://ship.equipment/", crc);
    }

    // CRC32C of a ROM in big-endian order. order is its first byte: 0x80 .z64, 0x37 .v64 (16-bit swapped),
    // 0x40 .n64 (32-bit little-endian), as libultraship's BitConverter::RomToBigEndian converts them.
    static int romCrc32c(File file, int order) throws IOException {
        byte[] buffer = new byte[1024 * 1024];
        int crc = 0xFFFFFFFF;
        boolean first = true;
        try (InputStream in = new FileInputStream(file)) {
            while (true) {
                int count = readUpTo(in, buffer);
                if (count <= 0) {
                    break;
                }
                if (order == 0x37) {
                    for (int i = 0; i + 1 < count; i += 2) {
                        byte t = buffer[i];
                        buffer[i] = buffer[i + 1];
                        buffer[i + 1] = t;
                    }
                } else if (order == 0x40) {
                    for (int i = 0; i + 3 < count; i += 4) {
                        byte t0 = buffer[i];
                        byte t1 = buffer[i + 1];
                        buffer[i] = buffer[i + 3];
                        buffer[i + 1] = buffer[i + 2];
                        buffer[i + 2] = t1;
                        buffer[i + 3] = t0;
                    }
                }
                if (first && count > 0x40) {
                    int version = ((buffer[0x10] & 0xFF) << 24) | ((buffer[0x11] & 0xFF) << 16)
                            | ((buffer[0x12] & 0xFF) << 8) | (buffer[0x13] & 0xFF);
                    if (version == OOT_PAL_GC_MQ_DBG) {
                        buffer[0x3E] = 'P';
                    }
                }
                first = false;
                crc = crc32cUpdate(crc, buffer, 0, count);
            }
        }
        return ~crc;
    }

    private static final int[] CRC32C_TABLE = new int[256];
    static {
        for (int i = 0; i < 256; i++) {
            int c = i;
            for (int k = 0; k < 8; k++) {
                c = ((c & 1) != 0) ? ((c >>> 1) ^ 0x82F63B78) : (c >>> 1);
            }
            CRC32C_TABLE[i] = c;
        }
    }

    // CRC32C (Castagnoli), the checksum of soh/soh/Extractor/FastCrc32C.c. Start with 0xFFFFFFFF and invert the end.
    static int crc32cUpdate(int crc, byte[] data, int offset, int length) {
        for (int i = offset; i < offset + length; i++) {
            crc = CRC32C_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >>> 8);
        }
        return crc;
    }

    // Fills the buffer unless the file ends first; returns the bytes read (a multiple of 4 except at the end).
    private static int readUpTo(InputStream in, byte[] buffer) throws IOException {
        int total = 0;
        while (total < buffer.length) {
            int count = in.read(buffer, total, buffer.length - total);
            if (count < 0) {
                break;
            }
            total += count;
        }
        return total;
    }

    private static void readFully(InputStream in, byte[] buffer, int offset, int length) throws IOException {
        while (length > 0) {
            int count = in.read(buffer, offset, length);
            if (count < 0) {
                throw new java.io.EOFException();
            }
            offset += count;
            length -= count;
        }
    }

    // --- Mods -----------------------------------------------------------------------------------------------------

    /**
     * Checks a copied mod before it is installed: an .otr must be a complete MPQ archive (checkOtrFile); an .o2r or
     * .zip must be a zip whose central directory can be read and that holds at least one file. Returns null when it
     * is valid, or the reason it is not. The supported formats are the ones the game already loads.
     */
    static String checkModFile(File file, String name) {
        String lower = name.toLowerCase();
        if (lower.endsWith(".otr")) {
            String problem = checkOtrFile(file, -1);
            return (problem == null) ? null : "não é um mod .otr válido (" + problem + ")";
        }
        if (lower.endsWith(".o2r") || lower.endsWith(".zip")) {
            if (!file.isFile() || file.length() < 22) {
                return "arquivo vazio ou incompleto";
            }
            try (java.util.zip.ZipFile zip = new java.util.zip.ZipFile(file)) {
                if (zip.size() == 0) {
                    return "o arquivo zip está vazio";
                }
                java.util.Enumeration<? extends java.util.zip.ZipEntry> entries = zip.entries();
                while (entries.hasMoreElements()) {
                    entries.nextElement();
                }
            } catch (IOException | RuntimeException e) {
                return "arquivo zip/o2r corrompido ou incompleto (" + e.getMessage() + ")";
            }
            return null;
        }
        return "extensão não suportada (use .otr, .o2r ou .zip)";
    }

    // True when both files have the same bytes.
    static boolean sameContent(File a, File b) throws IOException {
        if (a.length() != b.length()) {
            return false;
        }
        byte[] ba = new byte[256 * 1024];
        byte[] bb = new byte[256 * 1024];
        try (InputStream ia = new FileInputStream(a); InputStream ib = new FileInputStream(b)) {
            while (true) {
                int na = readUpTo(ia, ba);
                int nb = readUpTo(ib, bb);
                if (na != nb) {
                    return false;
                }
                if (na <= 0) {
                    return true;
                }
                for (int i = 0; i < na; i++) {
                    if (ba[i] != bb[i]) {
                        return false;
                    }
                }
            }
        }
    }

    // "name (2).otr", "name (3).otr"... the first one free in the folder, switched on or off.
    static String uniqueModName(File folder, String name, String disabledSuffix) {
        int dot = name.lastIndexOf('.');
        String base = (dot > 0) ? name.substring(0, dot) : name;
        String extension = (dot > 0) ? name.substring(dot) : "";
        for (int i = 2; ; i++) {
            String candidate = base + " (" + i + ")" + extension;
            if (!new File(folder, candidate).exists() && !new File(folder, candidate + disabledSuffix).exists()) {
                return candidate;
            }
        }
    }

    static void deleteQuietly(File file) {
        if (file.exists() && !file.delete()) {
            System.err.println("OOTDE_FILES: could not delete " + file);
        }
    }
}
