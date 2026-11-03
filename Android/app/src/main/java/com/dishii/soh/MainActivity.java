
package com.dishii.soh;
import org.libsdl.app.SDLActivity;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.util.concurrent.CountDownLatch;

import android.Manifest;
import android.content.pm.PackageManager;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.os.Build;
import android.view.WindowManager;
import android.widget.Toast;

import android.util.Log;
import android.database.Cursor;
import android.provider.OpenableColumns;

import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;

import java.util.concurrent.Executors;
import android.app.AlertDialog;
import android.app.ProgressDialog;

//This class is the main SDLActivity and just sets up a bunch of default files
public class MainActivity extends SDLActivity{

    SharedPreferences preferences;
    // The app's own folder on the phone. It must match libultraship's Context.cpp (patches/libultraship-ootde.patch).
    static final String DATA_FOLDER = "OOT-DE";
    // Folder used before 0.5, shared with the original Ship of Harkinian app. Its files are copied, never moved.
    static final String OLD_DATA_FOLDER = "SOH";

    static File getDataFolder() {
        return new File(Environment.getExternalStorageDirectory(), DATA_FOLDER);
    }
    private static final CountDownLatch setupLatch = new CountDownLatch(1);
    private ProgressDialog setupProgressDialog;
    // True when this install is newer than the one that created the SOH folder: the files the app ships
    // (soh.otr and the extractor's assets/) are then replaced, while saves, mods and oot.otr are kept.
    private boolean refreshPortFiles = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        preferences = getSharedPreferences("com.dishii.soh.prefs",Context.MODE_PRIVATE);

        // Check if storage permissions are granted
        if (hasStoragePermission()) {
            checkAndSetupFiles();
        } else {
            requestStoragePermission();
        }

        super.onCreate(savedInstanceState);

        setupControllerOverlay();
        attachController();
    }

    public static void waitForSetupFromNative() {
        try {
            setupLatch.await();  // Block until setup is complete
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    /**
     * Detects an app update without deleting user data.
     *
     * The SOH folder holds saves, mods, custom assets and the oot.otr made from the user's ROM, which are
     * never deleted. Only the files that come inside the APK (soh.otr and assets/, used by the ROM
     * extractor) are replaced on update, because files left by an older version can break the new one.
     * The new version is recorded only after those files were copied successfully.
     */
    private void doVersionCheck(){
        int currentVersion = BuildConfig.VERSION_CODE;
        int storedVersion = preferences.getInt("appVersion", 1);

        refreshPortFiles = currentVersion > storedVersion;
        if (refreshPortFiles) {
            Log.i("OOTDE_UPDATE", "Updating app version " + storedVersion + " -> " + currentVersion
                    + "; replacing soh.otr and assets/, keeping saves, mods and oot.otr.");
        }
    }



    // Check if storage permission is granted
    private boolean hasStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11 and above
            return Environment.isExternalStorageManager();
        } else {
            // Android 10 and below
            return ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED &&
                    ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                            == PackageManager.PERMISSION_GRANTED;
        }
    }

    private static final int STORAGE_PERMISSION_REQUEST_CODE = 2296;
    private static final int FILE_PICKER_REQUEST_CODE = 0;
    private static final int MOD_PICKER_REQUEST_CODE = 4;

    private void requestStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ → MANAGE_EXTERNAL_STORAGE
            if (!Environment.isExternalStorageManager()) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivityForResult(intent, STORAGE_PERMISSION_REQUEST_CODE);
            } else {
                // Already granted
                checkAndSetupFiles();
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Android 6–10 → request READ/WRITE at runtime
            ActivityCompat.requestPermissions(this,
                    new String[]{
                            Manifest.permission.READ_EXTERNAL_STORAGE,
                            Manifest.permission.WRITE_EXTERNAL_STORAGE
                    },
                    STORAGE_PERMISSION_REQUEST_CODE);
        } else {
            // Below Android 6 → permissions granted at install time
            checkAndSetupFiles();
        }
    }

    public void checkAndSetupFiles() {
        doVersionCheck();
        File targetRootFolder = getDataFolder();
        File assetsFolder = new File(targetRootFolder, "assets");
        File sohOtrFile = new File(targetRootFolder, "soh.otr");

        boolean isMissingAssets = !assetsFolder.exists() || assetsFolder.listFiles() == null || assetsFolder.listFiles().length == 0;
        boolean isMissingSohOtr = !sohOtrFile.exists();

        if (!targetRootFolder.exists() || isMissingAssets || isMissingSohOtr || refreshPortFiles) {
            String message = refreshPortFiles && targetRootFolder.exists()
                    ? "O aplicativo foi atualizado. Os arquivos do jogo serão atualizados (cerca de 1 minuto). Seus saves, mods e o oot.otr serão mantidos. Toque em OK para começar."
                    : "Alguns arquivos necessários estão faltando. O aplicativo vai criá-los (cerca de 1 minuto). Toque em OK para começar.";
            new AlertDialog.Builder(this)
                    .setTitle("Preparação necessária")
                    .setMessage(message)
                    .setCancelable(false)
                    .setPositiveButton("OK", (dialog, which) -> {
                        Executors.newSingleThreadExecutor().execute(() -> {
                            runOnUiThread(() -> { setupProgressDialog = new ProgressDialog(this); setupProgressDialog.setTitle("Preparando Ocarina of Time"); setupProgressDialog.setMessage("Criando arquivos necessários..."); setupProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL); setupProgressDialog.setMax(100); setupProgressDialog.setProgress(0); setupProgressDialog.setCancelable(false); setupProgressDialog.show(); });
                            setupFilesInBackground(targetRootFolder);
                        });
                    })
                    .show();
        } else {
            // No setup needed, still need to count down
            setupLatch.countDown();
        }
    }


    private void setupFilesInBackground(File targetRootFolder) {
        boolean setupOk = true;

        File sourceOldRoot = getExternalFilesDir(null);
        File sourceSavesDir = new File(sourceOldRoot, "Save"); // how to tell if there's anything to migrate

        // === Migration from old Android/data/.../files/ directory ===
        // Skipped once Save exists in the data folder, so an update never puts old saves over newer ones.
        boolean hasCurrentSaves = new File(targetRootFolder, "Save").isDirectory();
        if (sourceOldRoot != null && sourceSavesDir.isDirectory() && !hasCurrentSaves) {
            Log.i("setupFiles", "Migrating old data from: " + sourceOldRoot.getAbsolutePath());

            File[] sourceFiles = sourceOldRoot.listFiles();
            if (sourceFiles != null) {
                for (File file : sourceFiles) {
                    String name = file.getName();
                    if (name.equals("assets") || name.equals("soh.otr") || name.equals("oot-mq.otr") || name.equals("oot.otr")) {
                        continue; // Skip these
                    }

                    File dest = new File(targetRootFolder, name);
                    try {
                        if (file.isDirectory()) {
                            AssetCopyUtil.copyDirectory(file, dest);
                        } else {
                            AssetCopyUtil.copyFile(file, dest);
                        }
                        Log.i("setupFiles", "Migrated: " + name);
                    } catch (IOException e) {
                        Log.e("setupFiles", "Failed to migrate: " + name, e);
                    }
                }
            }

            runOnUiThread(() -> Toast.makeText(this, "Saves antigos migrados", Toast.LENGTH_SHORT).show());
        }

        updateSetupProgress(5, "Criando pasta principal...");
        // Ensure root folder exists
        if (!targetRootFolder.exists()) {
            if (!targetRootFolder.mkdirs()) {
                Log.e("setupFiles", "Failed to create root folder");
                runOnUiThread(() -> {
                    Toast.makeText(this, "Não foi possível criar a pasta " + DATA_FOLDER, Toast.LENGTH_LONG).show();
                    if (setupProgressDialog != null && setupProgressDialog.isShowing()) setupProgressDialog.dismiss();
                });
                setupLatch.countDown();
                return;
            }
        }

        // From 0.5 on the app has its own folder. The first time, saves, mods, oot.otr and settings are copied from
        // the old SOH folder, which stays as it is (the original Ship of Harkinian app may still use it).
        File oldRootFolder = new File(Environment.getExternalStorageDirectory(), OLD_DATA_FOLDER);
        if (oldRootFolder.isDirectory() && !preferences.getBoolean("copiedFromSohFolder", false)) {
            updateSetupProgress(7, "Copiando saves e mods da pasta SOH...");
            boolean copyOk = true;
            File[] oldFiles = oldRootFolder.listFiles();
            if (oldFiles != null) {
                for (File file : oldFiles) {
                    String name = file.getName();
                    // The app's own files come from the APK; the ROM copy is not needed once oot.otr exists.
                    if (name.equals("assets") || name.equals("soh.otr") || name.endsWith(".tmp") || name.equals("OOT.z64")) {
                        continue;
                    }
                    File dest = new File(targetRootFolder, name);
                    if (dest.exists()) {
                        continue; // never overwrite anything already in OOT-DE
                    }
                    try {
                        if (file.isDirectory()) {
                            AssetCopyUtil.copyDirectory(file, dest);
                        } else {
                            AssetCopyUtil.copyFile(file, dest);
                        }
                        Log.i("setupFiles", "Copied from SOH: " + name);
                    } catch (IOException e) {
                        copyOk = false;
                        Log.e("setupFiles", "Failed to copy from SOH: " + name, e);
                    }
                }
            }
            if (copyOk) {
                preferences.edit().putBoolean("copiedFromSohFolder", true).apply();
                runOnUiThread(() -> Toast.makeText(this, "Saves, mods e oot.otr copiados para a pasta " + DATA_FOLDER,
                        Toast.LENGTH_LONG).show());
            }
        }

        updateSetupProgress(15, "Preparando pastas do jogo...");
        // Always ensure mods folder exists
        File targetModsDir = new File(targetRootFolder, "mods");
        if (!targetModsDir.exists()) {
            targetModsDir.mkdirs();
        }

        updateSetupProgress(25, "Copiando recursos do jogo...");
        // Copy assets/ from internal
        File targetAssetsDir = new File(targetRootFolder, "assets");
        try {
            if (!targetAssetsDir.exists()) {
                targetAssetsDir.mkdirs();
            }
            AssetCopyUtil.copyAssetsToExternal(this, "assets", targetAssetsDir.getAbsolutePath(), refreshPortFiles,
                    (copied, total, currentFile) -> {
                        if (total > 0) {
                            int percent = 25 + (int) Math.min(55, (copied * 55L) / total);
                            updateSetupProgress(percent, "Copiando recursos: " + percent + "%");
                        }
                    });
            updateSetupProgress(80, "Recursos copiados. Preparando dados principais...");
        } catch (IOException e) {
            setupOk = false;
            Log.e("setupFiles", "Failed to copy assets", e);
            runOnUiThread(() -> Toast.makeText(this, "Erro ao copiar os recursos do jogo", Toast.LENGTH_LONG).show());
        }

        // Copy soh.otr from internal assets. It is written to a temporary file first, so an interrupted
        // copy never leaves a truncated soh.otr behind.
        File targetOtrFile = new File(targetRootFolder, "soh.otr");
        File tempOtrFile = new File(targetRootFolder, "soh.otr.tmp");
        try {
            try (InputStream in = getAssets().open("soh.otr");
                 OutputStream out = new FileOutputStream(tempOtrFile)) {

                byte[] buffer = new byte[256 * 1024];
                int read;
                long totalOtrBytes = in.available();
                long copiedOtrBytes = 0;
                while ((read = in.read(buffer)) != -1) {
                    out.write(buffer, 0, read);
                    copiedOtrBytes += read;
                    if (totalOtrBytes > 0) {
                        int percent = 80 + (int) Math.min(15, (copiedOtrBytes * 15L) / totalOtrBytes);
                        updateSetupProgress(percent, "Preparando dados principais: " + percent + "%");
                    }
                }
            }
            if (targetOtrFile.exists() && !targetOtrFile.delete()) {
                throw new IOException("Não foi possível substituir " + targetOtrFile.getAbsolutePath());
            }
            if (!tempOtrFile.renameTo(targetOtrFile)) {
                throw new IOException("Não foi possível gravar " + targetOtrFile.getAbsolutePath());
            }
            updateSetupProgress(95, "Finalizando preparação...");
        } catch (IOException e) {
            setupOk = false;
            tempOtrFile.delete();
            Log.e("setupFiles", "Failed to copy soh.otr", e);
            runOnUiThread(() -> Toast.makeText(this, "Erro ao copiar o soh.otr", Toast.LENGTH_LONG).show());
        }

        if (setupOk) {
            // Only now is this version's set of files complete; a failed copy is retried on the next start.
            preferences.edit().putInt("appVersion", BuildConfig.VERSION_CODE).apply();
            refreshPortFiles = false;
        }
        updateSetupProgress(100, "Configuração concluída.");
        runOnUiThread(() -> {
            if (setupProgressDialog != null && setupProgressDialog.isShowing()) {
                setupProgressDialog.dismiss();
            }
        });
        setupLatch.countDown();
    }




    private void updateSetupProgress(int progress, String message) { runOnUiThread(() -> { if (setupProgressDialog != null && setupProgressDialog.isShowing()) { setupProgressDialog.setMessage(message); setupProgressDialog.setProgress(progress); } }); }

    private native void nativeHandleSelectedFile(String filePath);

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == FILE_PICKER_REQUEST_CODE) {
            // Importa a ROM em segundo plano e exibe progresso real em bytes.
            Uri selectedFileUri = (resultCode == RESULT_OK && data != null) ? data.getData() : null;
            if (selectedFileUri != null) {
                importRomWithProgress(selectedFileUri);
            } else {
                // The native extractor waits for an answer; an empty path tells it nothing was chosen.
                nativeHandleSelectedFile("");
            }

        } else if (requestCode == MOD_PICKER_REQUEST_CODE) {
            if (resultCode == RESULT_OK && data != null) {
                java.util.List<Uri> uris = new java.util.ArrayList<>();
                if (data.getClipData() != null) {
                    for (int i = 0; i < data.getClipData().getItemCount(); i++) {
                        uris.add(data.getClipData().getItemAt(i).getUri());
                    }
                } else if (data.getData() != null) {
                    uris.add(data.getData());
                }
                Executors.newSingleThreadExecutor().execute(() -> {
                    for (Uri uri : uris) {
                        installMod(uri);
                    }
                });
            }

        } else if (requestCode == STORAGE_PERMISSION_REQUEST_CODE) {
            // Handle MANAGE_EXTERNAL_STORAGE result
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                if (Environment.isExternalStorageManager()) {
                    checkAndSetupFiles();
                } else {
                    Toast.makeText(this, "A permissão de acesso aos arquivos é necessária para jogar.", Toast.LENGTH_LONG).show();
                }
            }
        }
    }

    // Called by the native extractor (Extract.cpp) while it turns the ROM into oot.otr.
    private ProgressDialog extractionDialog;

    public void showExtractionProgress(final int percent, final String message) {
        runOnUiThread(() -> {
            if (isFinishing()) return;
            if (extractionDialog == null) {
                extractionDialog = new ProgressDialog(this);
                extractionDialog.setTitle("Criando os arquivos do jogo");
                extractionDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                extractionDialog.setIndeterminate(false);
                extractionDialog.setMax(100);
                extractionDialog.setCancelable(false);
            }
            extractionDialog.setProgress(Math.max(0, Math.min(100, percent)));
            extractionDialog.setMessage(message + "\n\nNão feche o app.");
            if (!extractionDialog.isShowing()) extractionDialog.show();
        });
    }

    public void hideExtractionProgress() {
        runOnUiThread(() -> {
            if (extractionDialog != null) {
                if (extractionDialog.isShowing()) extractionDialog.dismiss();
                extractionDialog = null;
            }
        });
    }

    private void importRomWithProgress(Uri sourceUri) {
        ProgressDialog dialog = new ProgressDialog(this);
        dialog.setTitle("Importando ROM");
        dialog.setMessage("Preparando importação...");
        dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        dialog.setIndeterminate(false);
        dialog.setMax(100);
        dialog.setProgress(0);
        dialog.setCancelable(false);
        dialog.show();

        Executors.newSingleThreadExecutor().execute(() -> {
            File destinationDirectory = getDataFolder();
            File destinationFile = new File(destinationDirectory, "OOT.z64");
            try {
                if (!destinationDirectory.exists() && !destinationDirectory.mkdirs()) {
                    throw new IOException("Não foi possível criar a pasta " + DATA_FOLDER + ".");
                }

                long totalBytes = -1;
                try (android.content.res.AssetFileDescriptor afd =
                             getContentResolver().openAssetFileDescriptor(sourceUri, "r")) {
                    if (afd != null) totalBytes = afd.getLength();
                } catch (Exception ignored) { }

                long copied = 0;
                try (InputStream in = getContentResolver().openInputStream(sourceUri);
                     OutputStream out = new FileOutputStream(destinationFile)) {
                    if (in == null) throw new IOException("Não foi possível abrir a ROM selecionada.");
                    byte[] buffer = new byte[256 * 1024];
                    int count;
                    while ((count = in.read(buffer)) != -1) {
                        out.write(buffer, 0, count);
                        copied += count;
                        if (totalBytes > 0) {
                            final int percent = (int) Math.min(100, (copied * 100L) / totalBytes);
                            runOnUiThread(() -> {
                                dialog.setProgress(percent);
                                dialog.setMessage("Importando ROM... " + percent + "%");
                            });
                        } else {
                            final long current = copied / (1024 * 1024);
                            runOnUiThread(() -> dialog.setMessage("Importando ROM... " + current + " MB"));
                        }
                    }
                    out.flush();
                }

                runOnUiThread(() -> {
                    dialog.setProgress(100);
                    dialog.dismiss();
                    Toast.makeText(this, "ROM importada com sucesso.", Toast.LENGTH_LONG).show();
                    nativeHandleSelectedFile(destinationFile.getAbsolutePath());
                });
            } catch (IOException e) {
                Log.e("OOTDE_IMPORT", "Falha ao importar ROM", e);
                destinationFile.delete();
                runOnUiThread(() -> {
                    dialog.dismiss();
                    Toast.makeText(this, "Falha ao importar ROM: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    nativeHandleSelectedFile("");
                });
            }
        });
    }

    // Called from the port menu (ModsMenu.cpp) to add mods.
    public void openModPicker() {
        runOnUiThread(() -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            startActivityForResult(intent, MOD_PICKER_REQUEST_CODE);
        });
    }

    private static boolean isModArchiveName(String name) {
        String lower = name.toLowerCase();
        return lower.endsWith(".otr") || lower.endsWith(".o2r") || lower.endsWith(".zip");
    }

    private static void copyStream(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[256 * 1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }

    private void showToast(String text) {
        runOnUiThread(() -> Toast.makeText(this, text, Toast.LENGTH_LONG).show());
    }

    // Copies a mod into the mods folder. A .zip that only wraps .otr/.o2r files (how mods are often downloaded)
    // has those files taken out; any other .zip is a mod archive itself and is copied as it is. Runs off the UI thread.
    private void installMod(Uri uri) {
        File folder = new File(getDataFolder(), "mods");

        String name = null;
        try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) {
                    name = cursor.getString(column);
                }
            }
        } catch (Exception e) {
            Log.w("mods", "Could not read the file name", e);
        }
        if (name == null || name.isEmpty()) {
            name = "mod.otr";
        }
        name = name.replaceAll("[/\\\\]", "_");
        if (!isModArchiveName(name)) {
            showToast("Escolha um mod .otr, .o2r ou .zip (" + name + ")");
            return;
        }
        if (!folder.isDirectory() && !folder.mkdirs()) {
            showToast("Não foi possível criar a pasta " + folder.getPath());
            return;
        }

        java.util.List<String> installed = new java.util.ArrayList<>();
        try {
            if (name.toLowerCase().endsWith(".zip")) {
                // First pass: does the zip only wrap mod archives (plus readme files)?
                boolean hasArchives = false;
                boolean hasOther = false;
                try (java.util.zip.ZipInputStream zip =
                         new java.util.zip.ZipInputStream(getContentResolver().openInputStream(uri))) {
                    java.util.zip.ZipEntry entry;
                    while ((entry = zip.getNextEntry()) != null) {
                        if (entry.isDirectory()) {
                            continue;
                        }
                        String lower = new File(entry.getName()).getName().toLowerCase();
                        if (lower.endsWith(".otr") || lower.endsWith(".o2r")) {
                            hasArchives = true;
                        } else if (!lower.endsWith(".txt") && !lower.endsWith(".md") && !lower.endsWith(".pdf")
                                && !lower.endsWith(".png") && !lower.endsWith(".jpg")) {
                            hasOther = true;
                        }
                    }
                }
                if (hasArchives && !hasOther) {
                    try (java.util.zip.ZipInputStream zip =
                             new java.util.zip.ZipInputStream(getContentResolver().openInputStream(uri))) {
                        java.util.zip.ZipEntry entry;
                        while ((entry = zip.getNextEntry()) != null) {
                            String entryName = new File(entry.getName()).getName();
                            String lower = entryName.toLowerCase();
                            if (entry.isDirectory() || !(lower.endsWith(".otr") || lower.endsWith(".o2r"))) {
                                continue;
                            }
                            try (OutputStream out = new FileOutputStream(new File(folder, entryName))) {
                                copyStream(zip, out);
                            }
                            installed.add(entryName);
                        }
                    }
                }
            }
            if (installed.isEmpty()) {
                try (InputStream in = getContentResolver().openInputStream(uri);
                     OutputStream out = new FileOutputStream(new File(folder, name))) {
                    if (in == null) throw new IOException("Não foi possível abrir o arquivo.");
                    copyStream(in, out);
                }
                installed.add(name);
            }
        } catch (Exception e) {
            Log.e("mods", "Failed to install the mod", e);
            showToast("Erro ao instalar o mod: " + e.getMessage());
            return;
        }
        showToast("Mod instalado: " + android.text.TextUtils.join(", ", installed) + ". Feche e abra o jogo para usar.");
    }

    public void openFilePicker() {
        // Create an Intent to open the file picker dialog
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.setType("*/*");

        // Start the file picker dialog
        startActivityForResult(intent, FILE_PICKER_REQUEST_CODE);
    }

    // Check if external storage is available and writable
    private boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        return Environment.MEDIA_MOUNTED.equals(state);
    }

    public native void attachController();
    public native void detachController();
    // Native method for setting button state
    public native void setButton(int button, boolean value);
    public native void setCameraState(int axis, float value);

    // Native method for setting joystick axis value
    public native void setAxis(int axis, short value);

    private Button button1, button2, button3, button4;
    private Button buttonA, buttonB, buttonX, buttonY;
    private Button buttonDpadUp, buttonDpadDown, buttonDpadLeft, buttonDpadRight;
    private Button buttonLB, buttonRB, buttonZ, buttonStart, buttonBack;
    private Button buttonToggle;
    private FrameLayout leftJoystick;
    private ImageView leftJoystickKnob;
    private View overlayView;

    // Function to set up the controller overlay (inflate layout and initialize buttons)
    private void setupControllerOverlay() {
        // Inflate the touchcontrol_overlay layout
        LayoutInflater inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
        overlayView = inflater.inflate(R.layout.touchcontrol_overlay, null);

        // Set layout params for overlayView to control positioning and sizing
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        );
        overlayView.setLayoutParams(layoutParams);
        // Add overlay view to the main layout (you may need to add it to a container like FrameLayout)
        ViewGroup view = (ViewGroup) getContentView();
        view.addView(overlayView);
        view.setKeepScreenOn(true);

        final ViewGroup buttonGroup = overlayView.findViewById(R.id.button_group);
        touchButtonGroup = buttonGroup;

        buttonA = overlayView.findViewById(R.id.buttonA);
        buttonB = overlayView.findViewById(R.id.buttonB);
        buttonX = overlayView.findViewById(R.id.buttonX);
        buttonY = overlayView.findViewById(R.id.buttonY);

        buttonDpadUp = overlayView.findViewById(R.id.buttonDpadUp);
        buttonDpadDown = overlayView.findViewById(R.id.buttonDpadDown);
        buttonDpadLeft = overlayView.findViewById(R.id.buttonDpadLeft);
        buttonDpadRight = overlayView.findViewById(R.id.buttonDpadRight);

        buttonLB = overlayView.findViewById(R.id.buttonLB);
        buttonRB = overlayView.findViewById(R.id.buttonRB);
        buttonZ = overlayView.findViewById(R.id.buttonZ);

        buttonStart = overlayView.findViewById(R.id.buttonStart);
        buttonBack = overlayView.findViewById(R.id.buttonBack);

        buttonToggle = overlayView.findViewById(R.id.buttonToggle);

        // Initialize joysticks and joystick knobs from the inflated layout
        leftJoystick = overlayView.findViewById(R.id.left_joystick);
        leftJoystickKnob = overlayView.findViewById(R.id.left_joystick_knob);

        FrameLayout rightScreenArea = overlayView.findViewById(R.id.right_screen_area);

        // Set OnTouchListeners for the Xbox controller buttons
        addTouchListener(buttonA, ControllerButtons.BUTTON_A); // SDL Button 0 (A)
        addTouchListener(buttonB, ControllerButtons.BUTTON_B); // SDL Button 1 (B)
        addTouchListener(buttonX, ControllerButtons.BUTTON_X); // SDL Button 2 (X)
        addTouchListener(buttonY, ControllerButtons.BUTTON_Y); // SDL Button 3 (Y)

        setupCButtons(buttonDpadUp, ControllerButtons.AXIS_RY, 1); // SDL Button 10 (D-Pad Up)
        setupCButtons(buttonDpadDown, ControllerButtons.AXIS_RY , -1); // SDL Button 11 (D-Pad Down)
        setupCButtons(buttonDpadLeft, ControllerButtons.AXIS_RX, 1); // SDL Button 12 (D-Pad Left)
        setupCButtons(buttonDpadRight, ControllerButtons.AXIS_RX, -1); // SDL Button 13 (D-Pad Right)

        addTouchListener(buttonLB, ControllerButtons.BUTTON_LB); // SDL Button 4 (LB)
        addTouchListener(buttonRB, ControllerButtons.BUTTON_RB); // SDL Button 5 (RB)
        addTouchListener(buttonZ, ControllerButtons.AXIS_RT); // SDL Button 5 (Z)

        addTouchListener(buttonStart, ControllerButtons.BUTTON_START); // SDL Button 7 (Start)
        addTouchListener(buttonBack, ControllerButtons.BUTTON_BACK); // SDL Button 6 (Back)


        // Setup joystick movement
        setupJoystick(leftJoystick, leftJoystickKnob, true); // Left joystick

        setupLookAround(rightScreenArea);

        setupToggleButton(buttonToggle,buttonGroup);

    }

    private void setupToggleButton(Button button, ViewGroup uiGroup){
        // The preference is named "controlsVisible" but holds whether the user hid the controls (kept for old installs).
        controlsHiddenByUser = preferences.getBoolean("controlsVisible", false);
        updateTouchControlsVisibility();
        button.setOnClickListener(v -> {
            controlsHiddenByUser = !controlsHiddenByUser;
            preferences.edit().putBoolean("controlsVisible", controlsHiddenByUser).apply();
            updateTouchControlsVisibility();
        });
    }

    // Shows the touch controls only when the user has not hidden them and the port menu is closed, so every touch
    // goes to the menu while it is open. Must run on the UI thread.
    private void updateTouchControlsVisibility() {
        if (touchButtonGroup == null) {
            return;
        }
        boolean menuOpen = !TouchAreaEnabled;
        boolean hidden = controlsHiddenByUser || menuOpen;
        touchButtonGroup.setVisibility(hidden ? View.INVISIBLE : View.VISIBLE);
        if (buttonToggle != null) {
            buttonToggle.setVisibility(menuOpen ? View.INVISIBLE : View.VISIBLE);
        }
        if (hidden) {
            // Let go of anything the overlay was holding, so the game does not see a stuck button or stick.
            for (int button : new int[] { ControllerButtons.BUTTON_A, ControllerButtons.BUTTON_B,
                    ControllerButtons.BUTTON_X, ControllerButtons.BUTTON_Y, ControllerButtons.BUTTON_LB,
                    ControllerButtons.BUTTON_RB, ControllerButtons.BUTTON_START, ControllerButtons.BUTTON_BACK }) {
                setButton(button, false);
            }
            setButton(ControllerButtons.AXIS_RT, false);
            setAxis(ControllerButtons.AXIS_LX, (short) 0);
            setAxis(ControllerButtons.AXIS_LY, (short) 0);
            setAxis(ControllerButtons.AXIS_RX, (short) 0);
            setAxis(ControllerButtons.AXIS_RY, (short) 0);
            setCameraState(0, 0.0f);
            setCameraState(1, 0.0f);
        }
    }

    // Function to set a touch listener for each button
    private void addTouchListener(Button button, int buttonNum) {
        button.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        setButton(buttonNum, true);
                        button.setPressed(true);
                        return true;
                    case MotionEvent.ACTION_UP:
                        setButton(buttonNum, false);
                        button.setPressed(false);
                        return true;
                    case MotionEvent.ACTION_CANCEL:
                        setButton(buttonNum, false);
                        return true;
                }
                return false;
            }
        });
    }

    private void setupCButtons(Button button, int buttonNum, int direction) {
        button.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        setAxis(buttonNum, direction<0 ? Short.MAX_VALUE : Short.MIN_VALUE);
                        button.setPressed(true);
                        return true;
                    case MotionEvent.ACTION_UP:
                        setAxis(buttonNum, (short) 0);
                        button.setPressed(false);
                        return true;
                    case MotionEvent.ACTION_CANCEL:
                        setAxis(buttonNum, (short) 0);
                        return true;
                }
                return false;
            }
        });
    }

    // False while the port menu is open. The native side (libultraship) calls DisableTouchArea/EnableTouchArea
    // every frame from the game thread, so only a change is passed on to the UI thread.
    volatile boolean TouchAreaEnabled = true;
    private boolean controlsHiddenByUser = false;
    private ViewGroup touchButtonGroup;

    void DisableTouchArea(){
        setMenuOpen(true);
    }
    void EnableTouchArea(){
        setMenuOpen(false);
    }
    private void setMenuOpen(boolean open) {
        if (TouchAreaEnabled != open) {
            return;
        }
        TouchAreaEnabled = !open;
        runOnUiThread(this::updateTouchControlsVisibility);
    }

    private void setupLookAround(FrameLayout rightScreenArea) {
        rightScreenArea.setOnTouchListener(new View.OnTouchListener() {
            private float lastX = 0;
            private float lastY = 0;
            private boolean isTouching = false;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        // Start tracking the finger's position
                        lastX = event.getX();
                        lastY = event.getY();
                        isTouching = true;
                        break;

                    case MotionEvent.ACTION_MOVE:
                        if (isTouching) {
                            // Calculate the change in position (delta)
                            float deltaX = event.getX() - lastX;
                            float deltaY = event.getY() - lastY;

                            // Update the last position
                            lastX = event.getX();
                            lastY = event.getY();

                            // Increase sensitivity by using a larger multiplier
                            // Adjust these multipliers to suit your needs
                            float sensitivityMultiplier = 15; // Higher value for more sensitivity
                            float rx = (deltaX * sensitivityMultiplier);
                            float ry = (deltaY * sensitivityMultiplier);

                            // Send the mapped values to the joystick axes
                            setCameraState(0, rx); // Right stick X axis
                            setCameraState(1, ry); // Right stick Y axis
                        }
                        break;

                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        // Stop tracking the finger's position and reset joystick input
                        isTouching = false;
                        setCameraState(0, 0.0f); // Reset right stick X axis
                        setCameraState(1, 0.0f); // Reset right stick Y axis
                        break;
                }
                return TouchAreaEnabled; // Event full handled
            }
        });
    }





    // Function to set joystick movement with reset to center when not touched
    private void setupJoystick(FrameLayout joystickLayout, ImageView joystickKnob, boolean isLeft) {
        joystickLayout.post(() -> {
            // Calculate the joystick center once, before any events
            final float joystickCenterX = joystickLayout.getWidth() / 2f;
            final float joystickCenterY = joystickLayout.getHeight() / 2f;

            joystickLayout.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    switch (event.getAction()) {
                        case MotionEvent.ACTION_DOWN:
                        case MotionEvent.ACTION_MOVE:
                            // Calculate the joystick movement and move the knob
                            float deltaX = event.getX() - joystickCenterX;
                            float deltaY = event.getY() - joystickCenterY;

                            // Clamp the joystick movement to prevent it from going outside the area
                            float maxRadius = joystickLayout.getWidth() / 2f - joystickKnob.getWidth() / 2f;
                            float distance = (float) Math.sqrt(deltaX * deltaX + deltaY * deltaY);
                            if (distance > maxRadius) {
                                float scale = maxRadius / distance;
                                deltaX *= scale;
                                deltaY *= scale;
                            }

                            joystickKnob.setX(joystickCenterX + deltaX - joystickKnob.getWidth() / 2f);
                            joystickKnob.setY(joystickCenterY + deltaY - joystickKnob.getHeight() / 2f);

                            // Send joystick values to native C code
                            short x = (short) (deltaX / maxRadius * Short.MAX_VALUE);
                            short y = (short) (deltaY / maxRadius * Short.MAX_VALUE);

                            // Send X-axis and Y-axis values
                            setAxis(isLeft ? ControllerButtons.AXIS_LX : ControllerButtons.AXIS_RX, x); // X-axis
                            setAxis(isLeft ? ControllerButtons.AXIS_LY : ControllerButtons.AXIS_RY, y); // Y-axis
                            break;

                        case MotionEvent.ACTION_UP:
                        case MotionEvent.ACTION_CANCEL:
                            // Reset joystick knob to the center position (ensure it's placed correctly)
                            joystickKnob.setX(joystickCenterX - joystickKnob.getWidth() / 2f);
                            joystickKnob.setY(joystickCenterY - joystickKnob.getHeight() / 2f);

                            // Reset joystick values to 0 when released or canceled
                            setAxis(isLeft ? ControllerButtons.AXIS_LX : ControllerButtons.AXIS_RX, (short) 0); // X-axis
                            setAxis(isLeft ? ControllerButtons.AXIS_LY : ControllerButtons.AXIS_RY, (short) 0); // Y-axis
                            break;
                    }
                    return true;
                }
            });
        });


    }


}
