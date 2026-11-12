package com.dishii.soh;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.Properties;
import java.util.TreeMap;

/**
 * OOT D.E. Touch 2.0: the saved layouts of the on-screen controls (position, size, spacing and opacity), kept in
 * OOT-DE/ootde/touch_layouts.properties, apart from the saves and from the game settings.
 *
 * Positions are stored as a fraction of the screen (dx of the width, dy of the height) relative to where the
 * original layout puts each control, so a layout fits any resolution and an empty profile is the original layout.
 * The file has a schema version; a missing, broken or newer file just means the original layout (a broken file is
 * kept aside). Plain Java, no Android classes: tested on the computer by scripts/tests/run_java_tests.sh.
 */
final class TouchLayoutStore {
    static final int SCHEMA_VERSION = 1;
    static final int PROFILE_COUNT = 3;

    static final float MIN_SCALE = 0.5f, MAX_SCALE = 2.5f;
    static final float MIN_OPACITY = 0.15f, MAX_OPACITY = 1.0f;
    static final float MIN_SPACING = 0.6f, MAX_SPACING = 2.2f;

    /** The controls that can be edited. The names are the keys in the file: never rename them. */
    static final String[] CONTROLS = { "stick", "cbuttons", "a", "b", "x", "y", "l", "r", "z", "start", "back" };

    static final class Item {
        float dx, dy;
        float scale = 1.0f;

        boolean isDefault() {
            return dx == 0.0f && dy == 0.0f && scale == 1.0f;
        }
    }

    static final class Profile {
        float opacity = 1.0f;
        float spacing = 1.0f; // distance between the A/B/X/Y buttons, 1 = original
        final TreeMap<String, Item> items = new TreeMap<>();

        Item item(String control) {
            Item item = items.get(control);
            if (item == null) {
                item = new Item();
                items.put(control, item);
            }
            return item;
        }

        void reset() {
            opacity = 1.0f;
            spacing = 1.0f;
            items.clear();
        }

        Profile copy() {
            Profile p = new Profile();
            p.opacity = opacity;
            p.spacing = spacing;
            for (String key : items.keySet()) {
                Item from = items.get(key);
                Item to = p.item(key);
                to.dx = from.dx;
                to.dy = from.dy;
                to.scale = from.scale;
            }
            return p;
        }
    }

    final Profile[] profiles = new Profile[PROFILE_COUNT];
    int active = 0;

    TouchLayoutStore() {
        for (int i = 0; i < PROFILE_COUNT; i++) {
            profiles[i] = new Profile();
        }
    }

    Profile activeProfile() {
        return profiles[active];
    }

    static float clamp(float value, float min, float max) {
        if (Float.isNaN(value)) {
            return min;
        }
        return Math.max(min, Math.min(max, value));
    }

    private static boolean isControl(String name) {
        for (String control : CONTROLS) {
            if (control.equals(name)) {
                return true;
            }
        }
        return false;
    }

    private static float number(Properties p, String key, float fallback) {
        String text = p.getProperty(key);
        if (text == null) {
            return fallback;
        }
        try {
            return Float.parseFloat(text.trim());
        } catch (NumberFormatException e) {
            return fallback;
        }
    }

    /**
     * Reads the file. Returns null when it was read (or does not exist yet), otherwise why the original layout is
     * being used. A broken file is renamed to *.corrompido so the next save does not destroy what was there.
     */
    String load(File file) {
        for (Profile profile : profiles) {
            profile.reset();
        }
        active = 0;
        if (!file.isFile()) {
            return null;
        }
        Properties p = new Properties();
        try (InputStreamReader in = new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8)) {
            p.load(in);
        } catch (IOException | IllegalArgumentException e) {
            keepAside(file);
            return "o arquivo de layouts não pôde ser lido";
        }
        int version;
        try {
            version = Integer.parseInt(p.getProperty("schemaVersion", "").trim());
        } catch (NumberFormatException e) {
            keepAside(file);
            return "o arquivo de layouts está corrompido";
        }
        if (version > SCHEMA_VERSION) {
            // Written by a newer app: leave it alone, the newer app can still use it.
            return "o arquivo de layouts é de uma versão mais nova do jogo";
        }
        // version 1 is the only layout so far; older versions would be converted here.
        active = (int) clamp(number(p, "active", 0), 0, PROFILE_COUNT - 1);
        for (int i = 0; i < PROFILE_COUNT; i++) {
            Profile profile = profiles[i];
            String prefix = "p" + i + ".";
            profile.opacity = clamp(number(p, prefix + "opacity", 1.0f), MIN_OPACITY, MAX_OPACITY);
            profile.spacing = clamp(number(p, prefix + "spacing", 1.0f), MIN_SPACING, MAX_SPACING);
            for (String control : CONTROLS) {
                String key = prefix + control + ".";
                if (p.getProperty(key + "dx") == null && p.getProperty(key + "dy") == null
                        && p.getProperty(key + "scale") == null) {
                    continue;
                }
                Item item = profile.item(control);
                item.dx = clamp(number(p, key + "dx", 0.0f), -1.0f, 1.0f);
                item.dy = clamp(number(p, key + "dy", 0.0f), -1.0f, 1.0f);
                item.scale = clamp(number(p, key + "scale", 1.0f), MIN_SCALE, MAX_SCALE);
            }
        }
        return null;
    }

    private static void keepAside(File file) {
        File aside = new File(file.getPath() + ".corrompido");
        if (aside.exists() && !aside.delete()) {
            return;
        }
        if (!file.renameTo(aside)) {
            System.err.println("OOTDE_TOUCH: could not keep the broken layout file aside");
        }
    }

    /** Writes the file through a complete temporary copy, so an interruption leaves the previous file. */
    void save(File file) throws IOException {
        File folder = file.getParentFile();
        if (folder != null && !folder.isDirectory() && !folder.mkdirs() && !folder.isDirectory()) {
            throw new IOException("Não foi possível criar a pasta " + folder.getName());
        }
        StringBuilder text = new StringBuilder();
        text.append("# OOT D.E. - layouts dos controles de toque\n");
        text.append("schemaVersion=").append(SCHEMA_VERSION).append('\n');
        text.append("active=").append(active).append('\n');
        for (int i = 0; i < PROFILE_COUNT; i++) {
            Profile profile = profiles[i];
            String prefix = "p" + i + ".";
            text.append(prefix).append("opacity=").append(format(profile.opacity)).append('\n');
            text.append(prefix).append("spacing=").append(format(profile.spacing)).append('\n');
            for (String control : profile.items.keySet()) {
                Item item = profile.items.get(control);
                if (!isControl(control) || item.isDefault()) {
                    continue;
                }
                String key = prefix + control + ".";
                text.append(key).append("dx=").append(format(item.dx)).append('\n');
                text.append(key).append("dy=").append(format(item.dy)).append('\n');
                text.append(key).append("scale=").append(format(item.scale)).append('\n');
            }
        }
        File temp = new File(file.getPath() + ".tmp");
        try (FileOutputStream out = new FileOutputStream(temp)) {
            OutputStreamWriter writer = new OutputStreamWriter(out, StandardCharsets.UTF_8);
            writer.write(text.toString());
            writer.flush();
            SafeFiles.syncToDisk(out);
        } catch (IOException e) {
            SafeFiles.deleteQuietly(temp);
            throw e;
        }
        try {
            SafeFiles.replaceFileSafely(temp, file);
        } catch (IOException e) {
            SafeFiles.deleteQuietly(temp);
            throw e;
        }
    }

    private static String format(float value) {
        return String.format(Locale.US, "%.4f", value);
    }
}
