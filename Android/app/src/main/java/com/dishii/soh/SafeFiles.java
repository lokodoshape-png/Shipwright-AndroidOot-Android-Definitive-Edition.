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
}
