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
        copyAssetsToExternal(context, assetsFolderPath, externalFolderPath, null);
    }

    public static void copyAssetsToExternal(Context context, String assetsFolderPath, String externalFolderPath,
                                            ProgressListener listener) throws IOException {
        AssetManager manager = context.getAssets();
        long totalBytes = calculateAssetBytes(manager, assetsFolderPath);
        long[] copiedBytes = new long[]{0};
        copyAssetTree(manager, assetsFolderPath, new File(externalFolderPath), totalBytes, copiedBytes, listener);
    }

    private static long calculateAssetBytes(AssetManager manager, String path) throws IOException {
        long total = 0;
        String[] children = manager.list(path);
        if (children == null) return 0;
        for (String child : children) {
            String childPath = path + "/" + child;
            String[] nested = manager.list(childPath);
            if (nested != null && nested.length > 0) {
                total += calculateAssetBytes(manager, childPath);
            } else {
                try (InputStream in = manager.open(childPath)) {
                    total += in.available();
                }
            }
        }
        return total;
    }

    private static void copyAssetTree(AssetManager manager, String assetPath, File targetDir,
                                      long totalBytes, long[] copiedBytes, ProgressListener listener) throws IOException {
        if (!targetDir.exists() && !targetDir.mkdirs()) {
            throw new IOException("Não foi possível criar " + targetDir.getAbsolutePath());
        }
        String[] children = manager.list(assetPath);
        if (children == null) return;
        for (String child : children) {
            String childPath = assetPath + "/" + child;
            String[] nested = manager.list(childPath);
            File target = new File(targetDir, child);
            if (nested != null && nested.length > 0) {
                copyAssetTree(manager, childPath, target, totalBytes, copiedBytes, listener);
            } else if (!target.exists()) {
                try (InputStream in = manager.open(childPath);
                     OutputStream out = new FileOutputStream(target)) {
                    byte[] buffer = new byte[256 * 1024];
                    int read;
                    while ((read = in.read(buffer)) != -1) {
                        out.write(buffer, 0, read);
                        copiedBytes[0] += read;
                        if (listener != null) listener.onProgress(copiedBytes[0], totalBytes, child);
                    }
                }
            } else {
                // Existing user files are intentionally preserved and count as completed.
                if (listener != null) listener.onProgress(copiedBytes[0], totalBytes, child);
            }
        }
    }

    public static void copyDirectory(File sourceDir, File targetDir) throws IOException {
        if (!targetDir.exists()) {
            targetDir.mkdirs();
        }
        for (File file : sourceDir.listFiles()) {
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