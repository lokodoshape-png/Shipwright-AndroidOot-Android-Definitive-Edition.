package com.dishii.soh;

import android.content.Context;
import android.content.res.AssetManager;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class AssetCopyUtil {

    public interface ProgressListener {
        void onProgress(long copiedBytes, long totalBytes, String currentFile);
    }

    public static void copyAssetsToExternal(Context context, String assetsFolderPath, String externalFolderPath) throws IOException {
        copyAssetsToExternal(context, assetsFolderPath, externalFolderPath, false, null);
    }

    /**
     * Copies the APK's assets folder to external storage.
     *
     * The folder holds thousands of small XML files (the extractor's files for every ROM version), so the copy lists
     * the tree once, then copies the files on a few threads at the same time; on a phone's shared storage most of the
     * time goes into opening and closing each file, not into the bytes. Progress is reported by file count.
     *
     * @param overwrite true after an app update: files that exist are replaced by the new version's copy.
     *                  false: existing files are kept and only missing ones are copied.
     */
    public static void copyAssetsToExternal(Context context, String assetsFolderPath, String externalFolderPath,
                                            boolean overwrite, ProgressListener listener) throws IOException {
        AssetManager manager = context.getAssets();
        File root = new File(externalFolderPath);
        java.util.List<String> files = new java.util.ArrayList<>();
        listAssetFiles(manager, assetsFolderPath, "", files);

        final int total = files.size();
        final java.util.concurrent.atomic.AtomicInteger done = new java.util.concurrent.atomic.AtomicInteger();
        final java.util.concurrent.atomic.AtomicReference<IOException> failure = new java.util.concurrent.atomic.AtomicReference<>();
        int threads = Math.max(2, Math.min(6, Runtime.getRuntime().availableProcessors()));
        java.util.concurrent.ExecutorService pool = java.util.concurrent.Executors.newFixedThreadPool(threads);
        final ThreadLocal<byte[]> buffers = ThreadLocal.withInitial(() -> new byte[64 * 1024]);

        for (String relative : files) {
            pool.execute(() -> {
                if (failure.get() != null) return;
                File target = new File(root, relative);
                try {
                    if (overwrite || !target.exists()) {
                        File parent = target.getParentFile();
                        if (parent != null && !parent.isDirectory() && !parent.mkdirs() && !parent.isDirectory()) {
                            throw new IOException("Não foi possível criar " + parent.getAbsolutePath());
                        }
                        byte[] buffer = buffers.get();
                        try (InputStream in = manager.open(assetsFolderPath + "/" + relative);
                             OutputStream out = new FileOutputStream(target)) {
                            int read;
                            while ((read = in.read(buffer)) != -1) {
                                out.write(buffer, 0, read);
                            }
                        }
                    }
                } catch (IOException e) {
                    failure.compareAndSet(null, e);
                }
                int count = done.incrementAndGet();
                // Report every 25 files, so the dialog is not flooded with updates.
                if (listener != null && (count % 25 == 0 || count == total)) {
                    listener.onProgress(count, total, relative);
                }
            });
        }
        pool.shutdown();
        try {
            pool.awaitTermination(1, java.util.concurrent.TimeUnit.HOURS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new IOException("Cópia interrompida", e);
        }
        if (failure.get() != null) throw failure.get();
    }

    // Collects the paths of all files under an assets folder, relative to it.
    private static void listAssetFiles(AssetManager manager, String base, String relative, java.util.List<String> out)
            throws IOException {
        String path = relative.isEmpty() ? base : base + "/" + relative;
        String[] children = manager.list(path);
        if (children == null) return;
        for (String child : children) {
            String childRelative = relative.isEmpty() ? child : relative + "/" + child;
            // Asset files all have an extension; asking list() about every one of the ~7700 files is slow.
            if (child.contains(".")) {
                out.add(childRelative);
                continue;
            }
            String[] nested = manager.list(base + "/" + childRelative);
            if (nested != null && nested.length > 0) {
                listAssetFiles(manager, base, childRelative, out);
            } else {
                out.add(childRelative);
            }
        }
    }

    public static void copyDirectory(File sourceDir, File targetDir) throws IOException {
        if (!targetDir.exists()) {
            targetDir.mkdirs();
        }
        File[] files = sourceDir.listFiles();
        if (files == null) return;
        for (File file : files) {
            File dest = new File(targetDir, file.getName());
            if (file.isDirectory()) {
                copyDirectory(file, dest);
            } else {
                copyFile(file, dest);
            }
        }
    }

    public static void copyFile(File source, File dest) throws IOException {
        try (InputStream in = new FileInputStream(source);
             OutputStream out = new FileOutputStream(dest)) {
            byte[] buf = new byte[64*1024];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
        }
    }
}