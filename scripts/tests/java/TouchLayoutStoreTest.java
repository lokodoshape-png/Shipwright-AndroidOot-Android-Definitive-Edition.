package com.dishii.soh;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

// OOT D.E.: tests of TouchLayoutStore (saved touch control layouts) that run on a computer.
// Run with scripts/tests/run_java_tests.sh.
public class TouchLayoutStoreTest {
    private static int failures = 0;

    private static void check(boolean ok, String what) {
        System.out.println((ok ? "PASS " : "FAIL ") + what);
        if (!ok) failures++;
    }

    private static void write(File f, String text) throws Exception {
        try (FileOutputStream out = new FileOutputStream(f)) {
            out.write(text.getBytes(StandardCharsets.UTF_8));
        }
    }

    public static void main(String[] args) throws Exception {
        File dir = Files.createTempDirectory("touchlayout").toFile();
        File file = new File(new File(dir, "ootde"), "touch_layouts.properties");

        TouchLayoutStore store = new TouchLayoutStore();
        check(store.load(file) == null, "missing file loads as the original layout");
        check(store.activeProfile().items.isEmpty() && store.activeProfile().opacity == 1.0f, "defaults");

        store.active = 2;
        store.activeProfile().opacity = 0.5f;
        store.activeProfile().spacing = 1.4f;
        store.activeProfile().item("a").dx = -0.25f;
        store.activeProfile().item("a").scale = 1.5f;
        store.activeProfile().item("stick"); // untouched item: not written
        store.profiles[0].item("z").dy = 0.1f;
        store.save(file);
        check(file.isFile() && !new File(file.getPath() + ".tmp").exists(), "saved without leftovers");

        TouchLayoutStore again = new TouchLayoutStore();
        check(again.load(file) == null, "saved file loads");
        check(again.active == 2, "active profile kept");
        check(Math.abs(again.activeProfile().opacity - 0.5f) < 1e-4 && Math.abs(again.activeProfile().spacing - 1.4f) < 1e-4,
                "opacity and spacing kept");
        check(Math.abs(again.activeProfile().item("a").dx + 0.25f) < 1e-4
                && Math.abs(again.activeProfile().item("a").scale - 1.5f) < 1e-4, "item kept");
        check(!again.activeProfile().items.containsKey("stick"), "default items are not stored");
        check(Math.abs(again.profiles[0].item("z").dy - 0.1f) < 1e-4, "other profile kept");
        check(again.profiles[1].items.isEmpty(), "unused profile is the original layout");

        again.save(file); // overwrite an existing file
        check(new TouchLayoutStore().load(file) == null && !new File(file.getPath() + ".bak").exists(),
                "overwrite leaves one file");

        write(file, "schemaVersion=1\nactive=9\np0.opacity=7\np0.spacing=abc\np0.a.scale=99\np0.a.dx=NaN\np0.hack.dx=1\n");
        TouchLayoutStore clamped = new TouchLayoutStore();
        check(clamped.load(file) == null, "out of range values load");
        check(clamped.active == TouchLayoutStore.PROFILE_COUNT - 1, "active clamped");
        check(clamped.profiles[0].opacity == TouchLayoutStore.MAX_OPACITY && clamped.profiles[0].spacing == 1.0f,
                "opacity clamped, bad spacing is the default");
        check(clamped.profiles[0].item("a").scale == TouchLayoutStore.MAX_SCALE, "scale clamped");
        check(clamped.profiles[0].item("a").dx >= -1.0f && clamped.profiles[0].item("a").dx <= 1.0f, "NaN handled");
        check(!clamped.profiles[0].items.containsKey("hack"), "unknown control ignored");

        write(file, "isto nao tem versao\n");
        TouchLayoutStore broken = new TouchLayoutStore();
        check(broken.load(file) != null, "file without version is reported");
        check(!file.exists() && new File(file.getPath() + ".corrompido").isFile(), "broken file kept aside");
        check(broken.activeProfile().items.isEmpty(), "broken file means the original layout");

        write(file, "schemaVersion=99\np0.a.dx=0.5\n");
        TouchLayoutStore newer = new TouchLayoutStore();
        check(newer.load(file) != null && file.isFile(), "newer file is reported and left alone");
        check(newer.activeProfile().items.isEmpty(), "newer file means the original layout");

        TouchLayoutStore.Profile copy = again.profiles[2].copy();
        copy.item("a").dx = 0.9f;
        check(Math.abs(again.profiles[2].item("a").dx + 0.25f) < 1e-4, "copy is independent");

        System.out.println(failures == 0 ? "ALL PASS" : failures + " FAILED");
        System.exit(failures == 0 ? 0 : 1);
    }
}
