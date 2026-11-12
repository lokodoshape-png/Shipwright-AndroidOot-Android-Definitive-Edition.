package com.dishii.soh;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;

/**
 * OOT D.E. Touch 2.0: applies the saved layout to the on-screen controls and lets the player edit it.
 *
 * The controls keep their original layout (touchcontrol_overlay.xml) and their original touch listeners, which feed
 * the same virtual controller as before. A layout only moves (translation), resizes (scale) and fades (alpha) the
 * views, so the default layout is "no change" and physical/Bluetooth controllers are not involved at all.
 * While editing, a layer on top receives every touch, so nothing reaches the game.
 */
final class TouchLayoutEditor {
    private final Activity activity;
    private final ViewGroup overlay;     // root of touchcontrol_overlay
    private final ViewGroup buttonGroup; // all controls; its alpha is the opacity
    private final Runnable onEditingChanged;
    private final Map<String, View[]> targets = new LinkedHashMap<>();
    private final TouchLayoutStore store = new TouchLayoutStore();
    private boolean loaded = false;

    // Editing state.
    private FrameLayout editLayer;
    private TextView statusText;
    private Button profileButton;
    private TouchLayoutStore.Profile[] savedProfiles; // what was on disk when editing started (for Cancel)
    private int savedActive;
    private String selected;
    private float lastX, lastY;

    TouchLayoutEditor(Activity activity, ViewGroup overlay, ViewGroup buttonGroup, Runnable onEditingChanged) {
        this.activity = activity;
        this.overlay = overlay;
        this.buttonGroup = buttonGroup;
        this.onEditingChanged = onEditingChanged;
        targets.put("stick", views(R.id.left_joystick));
        targets.put("cbuttons", views(R.id.imageView, R.id.buttonDpadUp, R.id.buttonDpadDown, R.id.buttonDpadLeft,
                R.id.buttonDpadRight));
        targets.put("a", views(R.id.buttonA));
        targets.put("b", views(R.id.buttonB));
        targets.put("x", views(R.id.buttonX));
        targets.put("y", views(R.id.buttonY));
        targets.put("l", views(R.id.buttonLB));
        targets.put("r", views(R.id.buttonRB));
        targets.put("z", views(R.id.buttonZ));
        targets.put("start", views(R.id.buttonStart));
        targets.put("back", views(R.id.buttonBack));
        // Re-apply whenever the screen is laid out again (rotation, split screen, system bars), because positions
        // are fractions of the screen size.
        overlay.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> apply());
    }

    private View[] views(int... ids) {
        View[] result = new View[ids.length];
        for (int i = 0; i < ids.length; i++) {
            result[i] = overlay.findViewById(ids[i]);
        }
        return result;
    }

    boolean isEditing() {
        return editLayer != null;
    }

    private static File layoutFile() {
        return new File(new File(MainActivity.getDataFolder(), "ootde"), "touch_layouts.properties");
    }

    private void ensureLoaded() {
        if (loaded) {
            return;
        }
        File folder = MainActivity.getDataFolder();
        if (!folder.isDirectory()) {
            return; // storage not ready yet (first run, permission pending): keep the original layout for now
        }
        String problem = store.load(layoutFile());
        loaded = true;
        if (problem != null) {
            Toast.makeText(activity, "Controles de toque no layout original: " + problem, Toast.LENGTH_LONG).show();
        }
    }

    // Centre of a group of views in the overlay's coordinates, from the original layout (ignores translation/scale).
    private static float[] layoutCenter(View[] group) {
        float left = Float.MAX_VALUE, top = Float.MAX_VALUE, right = -Float.MAX_VALUE, bottom = -Float.MAX_VALUE;
        for (View view : group) {
            if (view == null) {
                continue;
            }
            left = Math.min(left, view.getLeft());
            top = Math.min(top, view.getTop());
            right = Math.max(right, view.getRight());
            bottom = Math.max(bottom, view.getBottom());
        }
        return new float[] { (left + right) / 2f, (top + bottom) / 2f, right - left, bottom - top };
    }

    /** Puts every control where the active profile says. Safe to call at any time on the UI thread. */
    void apply() {
        ensureLoaded();
        int width = overlay.getWidth();
        int height = overlay.getHeight();
        if (width <= 0 || height <= 0) {
            return;
        }
        TouchLayoutStore.Profile profile = store.activeProfile();
        buttonGroup.setAlpha(profile.opacity);

        // Spacing spreads A/B/X/Y around the middle of the four.
        float[] cluster = layoutCenter(new View[] { first("a"), first("b"), first("x"), first("y") });

        for (Map.Entry<String, View[]> entry : targets.entrySet()) {
            String key = entry.getKey();
            View[] group = entry.getValue();
            TouchLayoutStore.Item item = profile.items.get(key);
            float dx = item == null ? 0f : item.dx * width;
            float dy = item == null ? 0f : item.dy * height;
            float scale = item == null ? 1f : item.scale;
            float[] center = layoutCenter(group);
            if (key.equals("a") || key.equals("b") || key.equals("x") || key.equals("y")) {
                dx += (center[0] - cluster[0]) * (profile.spacing - 1f);
                dy += (center[1] - cluster[1]) * (profile.spacing - 1f);
            }
            // Keep the centre of the control on the screen whatever the file says.
            dx = TouchLayoutStore.clamp(center[0] + dx, 0f, width) - center[0];
            dy = TouchLayoutStore.clamp(center[1] + dy, 0f, height) - center[1];
            for (View view : group) {
                if (view == null) {
                    continue;
                }
                // A group scales around its common centre, so its parts stay together.
                view.setPivotX(center[0] - view.getLeft());
                view.setPivotY(center[1] - view.getTop());
                view.setScaleX(scale);
                view.setScaleY(scale);
                view.setTranslationX(dx);
                view.setTranslationY(dy);
            }
        }
    }

    private View first(String key) {
        View[] group = targets.get(key);
        return group == null || group.length == 0 ? null : group[0];
    }

    // ------------------------------------------------------------------------------------------------------------
    // Editor
    // ------------------------------------------------------------------------------------------------------------

    void open() {
        if (isEditing()) {
            return;
        }
        ensureLoaded();
        if (!loaded) {
            Toast.makeText(activity, "A pasta do jogo ainda não está pronta.", Toast.LENGTH_LONG).show();
            return;
        }
        savedProfiles = new TouchLayoutStore.Profile[TouchLayoutStore.PROFILE_COUNT];
        for (int i = 0; i < savedProfiles.length; i++) {
            savedProfiles[i] = store.profiles[i].copy();
        }
        savedActive = store.active;
        selected = null;

        editLayer = new FrameLayout(activity);
        editLayer.setBackgroundColor(Color.argb(110, 0, 0, 0));
        editLayer.setClickable(true);
        editLayer.setOnTouchListener(this::onEditTouch);

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER_HORIZONTAL);
        GradientDrawable background = new GradientDrawable();
        background.setColor(Color.argb(230, 16, 28, 54));
        background.setStroke(dp(2), Color.rgb(212, 175, 55));
        background.setCornerRadius(dp(10));
        panel.setBackground(background);
        panel.setPadding(dp(10), dp(6), dp(10), dp(6));

        statusText = new TextView(activity);
        statusText.setTextColor(Color.rgb(240, 220, 150));
        statusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        statusText.setGravity(Gravity.CENTER);
        panel.addView(statusText);

        LinearLayout row1 = row();
        profileButton = panelButton(row1, "", v -> {
            store.active = (store.active + 1) % TouchLayoutStore.PROFILE_COUNT;
            refresh();
        });
        panelButton(row1, "Tamanho -", v -> changeScale(-0.1f));
        panelButton(row1, "Tamanho +", v -> changeScale(0.1f));
        panelButton(row1, "Opacidade -", v -> changeOpacity(-0.1f));
        panelButton(row1, "Opacidade +", v -> changeOpacity(0.1f));
        panel.addView(row1);

        LinearLayout row2 = row();
        panelButton(row2, "Espaço -", v -> changeSpacing(-0.1f));
        panelButton(row2, "Espaço +", v -> changeSpacing(0.1f));
        panelButton(row2, "Padrão", v -> {
            store.activeProfile().reset();
            selected = null;
            refresh();
        });
        panelButton(row2, "Cancelar", v -> close(false));
        panelButton(row2, "Salvar", v -> close(true));
        panel.addView(row2);

        FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER);
        editLayer.addView(panel, panelParams);

        // Same parent and size as the overlay, so both share one coordinate space.
        ((ViewGroup) overlay.getParent()).addView(editLayer, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        onEditingChanged.run();
        refresh();
    }

    private void close(boolean save) {
        if (!isEditing()) {
            return;
        }
        if (save) {
            try {
                store.save(layoutFile());
                Toast.makeText(activity, "Layout salvo no perfil " + (store.active + 1) + ".", Toast.LENGTH_SHORT)
                        .show();
            } catch (IOException e) {
                Toast.makeText(activity, "Layout não salvo: " + e.getMessage(), Toast.LENGTH_LONG).show();
                return; // stay in the editor so nothing is lost
            }
        } else {
            for (int i = 0; i < savedProfiles.length; i++) {
                store.profiles[i] = savedProfiles[i];
            }
            store.active = savedActive;
        }
        ((ViewGroup) editLayer.getParent()).removeView(editLayer);
        editLayer = null;
        statusText = null;
        profileButton = null;
        savedProfiles = null;
        selected = null;
        apply();
        onEditingChanged.run();
    }

    private LinearLayout row() {
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        return row;
    }

    private Button panelButton(LinearLayout row, String text, View.OnClickListener action) {
        Button button = new Button(activity);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        button.setTextColor(Color.WHITE);
        GradientDrawable background = new GradientDrawable();
        background.setColor(Color.rgb(38, 58, 100));
        background.setStroke(dp(1), Color.rgb(212, 175, 55));
        background.setCornerRadius(dp(6));
        button.setBackground(background);
        button.setPadding(dp(8), 0, dp(8), 0);
        button.setMinWidth(0);
        button.setMinimumWidth(0);
        button.setOnClickListener(action);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, dp(38));
        params.setMargins(dp(3), dp(3), dp(3), dp(3));
        row.addView(button, params);
        return button;
    }

    private int dp(int value) {
        return Math.round(value * activity.getResources().getDisplayMetrics().density);
    }

    private static String label(String key) {
        switch (key) {
            case "stick":
                return "Analógico";
            case "cbuttons":
                return "Botões C";
            case "start":
                return "Start";
            case "back":
                return "Menu";
            default:
                return key.toUpperCase(Locale.ROOT);
        }
    }

    private void refresh() {
        apply();
        TouchLayoutStore.Profile profile = store.activeProfile();
        if (profileButton != null) {
            profileButton.setText("Perfil " + (store.active + 1));
        }
        if (statusText != null) {
            String chosen = selected == null ? "Arraste um controle para mover. Toque nele para escolher."
                    : label(selected) + ": tamanho " + Math.round(profile.item(selected).scale * 100) + "%";
            statusText.setText(chosen + "   |   Opacidade " + Math.round(profile.opacity * 100) + "%   |   Espaço "
                    + Math.round(profile.spacing * 100) + "%");
        }
    }

    private void changeScale(float delta) {
        if (selected == null) {
            Toast.makeText(activity, "Toque em um controle primeiro.", Toast.LENGTH_SHORT).show();
            return;
        }
        TouchLayoutStore.Item item = store.activeProfile().item(selected);
        item.scale = TouchLayoutStore.clamp(Math.round((item.scale + delta) * 10f) / 10f, TouchLayoutStore.MIN_SCALE,
                TouchLayoutStore.MAX_SCALE);
        refresh();
    }

    private void changeOpacity(float delta) {
        TouchLayoutStore.Profile profile = store.activeProfile();
        profile.opacity = TouchLayoutStore.clamp(Math.round((profile.opacity + delta) * 10f) / 10f,
                TouchLayoutStore.MIN_OPACITY, TouchLayoutStore.MAX_OPACITY);
        refresh();
    }

    private void changeSpacing(float delta) {
        TouchLayoutStore.Profile profile = store.activeProfile();
        profile.spacing = TouchLayoutStore.clamp(Math.round((profile.spacing + delta) * 10f) / 10f,
                TouchLayoutStore.MIN_SPACING, TouchLayoutStore.MAX_SPACING);
        refresh();
    }

    // The control under a point of the overlay, taking its current position and size into account.
    private String controlAt(float x, float y) {
        String best = null;
        float bestDistance = Float.MAX_VALUE;
        float slop = dp(12);
        for (Map.Entry<String, View[]> entry : targets.entrySet()) {
            View[] group = entry.getValue();
            float[] center = layoutCenter(group);
            View any = group[0];
            if (any == null) {
                continue;
            }
            float cx = center[0] + any.getTranslationX();
            float cy = center[1] + any.getTranslationY();
            float halfW = center[2] * any.getScaleX() / 2f + slop;
            float halfH = center[3] * any.getScaleY() / 2f + slop;
            if (Math.abs(x - cx) <= halfW && Math.abs(y - cy) <= halfH) {
                float distance = (x - cx) * (x - cx) + (y - cy) * (y - cy);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    best = entry.getKey();
                }
            }
        }
        return best;
    }

    private boolean onEditTouch(View view, MotionEvent event) {
        int width = overlay.getWidth();
        int height = overlay.getHeight();
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                String hit = controlAt(event.getX(), event.getY());
                if (hit != null) {
                    selected = hit;
                    lastX = event.getX();
                    lastY = event.getY();
                    refresh();
                }
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                if (selected == null || width <= 0 || height <= 0) {
                    return true;
                }
                TouchLayoutStore.Item item = store.activeProfile().item(selected);
                item.dx = TouchLayoutStore.clamp(item.dx + (event.getX() - lastX) / width, -1f, 1f);
                item.dy = TouchLayoutStore.clamp(item.dy + (event.getY() - lastY) / height, -1f, 1f);
                lastX = event.getX();
                lastY = event.getY();
                apply();
                return true;
            }
            default:
                return true;
        }
    }
}
