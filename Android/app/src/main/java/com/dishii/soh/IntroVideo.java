package com.dishii.soh;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.io.File;

/**
 * OOT D.E.: the opening video, played over the game before the Definitive Edition menu.
 *
 * It plays OOT-DE/intro.mp4 when the player put one there, otherwise assets/intro.mp4 of the APK. The native menu
 * (DeTitleMenu.cpp) starts it, asks every frame whether it is over, and then shows its golden transition while this
 * view fades away. A touch on "PULAR INTRO" (or any controller button, through the native side) ends it.
 *
 * A TextureView is used (not a VideoView) so the video is an ordinary view above SDL's SurfaceView, with the skip
 * button above it.
 */
final class IntroVideo implements TextureView.SurfaceTextureListener {
    private static final String TAG = "OOTDE-Intro";
    static final int STATE_IDLE = 0;
    static final int STATE_PLAYING = 1;
    static final int STATE_DONE = 2;
    // How long the video takes to fade out once over (the native light starts underneath at the same time).
    private static final long FADE_MS = 900;

    private final Activity activity;
    private volatile int state = STATE_IDLE;
    private FrameLayout overlay;
    private TextureView texture;
    private MediaPlayer player;
    private Surface surface;
    private boolean pausedByLifecycle = false;

    IntroVideo(Activity activity) {
        this.activity = activity;
    }

    int getState() {
        return state;
    }

    /** True when a video exists and starts playing (on the UI thread, soon after). */
    boolean start() {
        if (state != STATE_IDLE) {
            return state == STATE_PLAYING;
        }
        final File custom = new File(MainActivity.getDataFolder(), "intro.mp4");
        final boolean useCustom = custom.isFile() && custom.length() > 0;
        if (!useCustom && !assetExists()) {
            return false;
        }
        state = STATE_PLAYING;
        activity.runOnUiThread(() -> show(useCustom ? custom : null));
        return true;
    }

    /** Ends the video now (skip): it fades out like when it ends by itself. */
    void skip() {
        if (state == STATE_PLAYING) {
            activity.runOnUiThread(this::finish);
        }
    }

    void onPause() {
        activity.runOnUiThread(() -> {
            if (player != null && state == STATE_PLAYING) {
                try {
                    if (player.isPlaying()) {
                        player.pause();
                        pausedByLifecycle = true;
                    }
                } catch (IllegalStateException ignored) {
                }
            }
        });
    }

    void onResume() {
        activity.runOnUiThread(() -> {
            if (player != null && state == STATE_PLAYING && pausedByLifecycle) {
                pausedByLifecycle = false;
                try {
                    player.start();
                } catch (IllegalStateException e) {
                    finish();
                }
            }
        });
    }

    void release() {
        state = state == STATE_PLAYING ? STATE_DONE : state;
        activity.runOnUiThread(this::releaseNow);
    }

    private boolean assetExists() {
        try (AssetFileDescriptor fd = activity.getAssets().openFd("intro.mp4")) {
            return fd.getLength() > 0;
        } catch (Exception e) {
            return false;
        }
    }

    private void show(File custom) {
        if (activity.isFinishing()) {
            state = STATE_DONE;
            return;
        }
        overlay = new FrameLayout(activity);
        overlay.setBackgroundColor(Color.BLACK);
        // Touches on the video never reach the game.
        overlay.setClickable(true);
        overlay.setFocusable(true);

        texture = new TextureView(activity);
        texture.setSurfaceTextureListener(this);
        overlay.addView(texture, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        TextView skip = new TextView(activity);
        skip.setText("PULAR INTRO  ▸");
        skip.setTextColor(Color.rgb(244, 214, 138));
        skip.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        skip.setLetterSpacing(0.12f);
        int padH = dp(18), padV = dp(10);
        skip.setPadding(padH, padV, padH, padV);
        android.graphics.drawable.GradientDrawable bg = new android.graphics.drawable.GradientDrawable();
        bg.setColor(Color.argb(150, 6, 8, 14));
        bg.setStroke(dp(1), Color.rgb(196, 156, 72));
        bg.setCornerRadius(dp(6));
        skip.setBackground(bg);
        skip.setOnClickListener(v -> finish());
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM | Gravity.END);
        lp.setMargins(0, 0, dp(28), dp(24));
        overlay.addView(skip, lp);
        // Tapping anywhere else on the video skips too.
        overlay.setOnClickListener(v -> finish());

        activity.addContentView(overlay, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        overlay.setTag(custom);
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture st, int width, int height) {
        if (state != STATE_PLAYING) {
            return;
        }
        File custom = overlay != null ? (File) overlay.getTag() : null;
        try {
            surface = new Surface(st);
            player = new MediaPlayer();
            if (custom != null) {
                player.setDataSource(custom.getAbsolutePath());
            } else {
                try (AssetFileDescriptor fd = activity.getAssets().openFd("intro.mp4")) {
                    player.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
                }
            }
            player.setSurface(surface);
            player.setOnVideoSizeChangedListener((mp, w, h) -> fit(w, h));
            player.setOnPreparedListener(mp -> {
                if (state == STATE_PLAYING) {
                    fit(mp.getVideoWidth(), mp.getVideoHeight());
                    mp.start();
                }
            });
            player.setOnCompletionListener(mp -> finish());
            player.setOnErrorListener((mp, what, extra) -> {
                Log.w(TAG, "video error " + what + "/" + extra);
                finish();
                return true;
            });
            player.prepareAsync();
        } catch (Exception e) {
            Log.w(TAG, "cannot play the opening video", e);
            finish();
        }
    }

    // Keeps the proportions of the video (black bars where the screen is wider or taller).
    private void fit(int videoW, int videoH) {
        if (texture == null || videoW <= 0 || videoH <= 0) {
            return;
        }
        float viewW = texture.getWidth(), viewH = texture.getHeight();
        if (viewW <= 0 || viewH <= 0) {
            return;
        }
        float scale = Math.min(viewW / videoW, viewH / videoH);
        Matrix m = new Matrix();
        m.setScale(videoW * scale / viewW, videoH * scale / viewH, viewW / 2f, viewH / 2f);
        texture.setTransform(m);
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture st, int width, int height) {
        if (player != null) {
            try {
                fit(player.getVideoWidth(), player.getVideoHeight());
            } catch (IllegalStateException ignored) {
            }
        }
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture st) {
        releasePlayer();
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture st) {
    }

    // Over: the native side sees STATE_DONE at once and starts the light while the video fades away.
    private void finish() {
        if (state != STATE_PLAYING) {
            return;
        }
        state = STATE_DONE;
        if (player != null) {
            try {
                // The sound fades with the picture.
                final MediaPlayer mp = player;
                final long start = System.currentTimeMillis();
                final View host = overlay;
                if (host != null) {
                    host.post(new Runnable() {
                        @Override
                        public void run() {
                            float k = 1f - Math.min(1f, (System.currentTimeMillis() - start) / (float) FADE_MS);
                            try {
                                mp.setVolume(k, k);
                            } catch (IllegalStateException ignored) {
                                return;
                            }
                            if (k > 0f && player == mp) {
                                host.postDelayed(this, 30);
                            }
                        }
                    });
                }
            } catch (IllegalStateException ignored) {
            }
        }
        if (overlay != null) {
            overlay.setClickable(false);
            overlay.setOnClickListener(null);
            overlay.animate().alpha(0f).setDuration(FADE_MS).withEndAction(this::releaseNow).start();
        } else {
            releaseNow();
        }
    }

    private void releasePlayer() {
        if (player != null) {
            try {
                player.release();
            } catch (Exception ignored) {
            }
            player = null;
        }
        if (surface != null) {
            surface.release();
            surface = null;
        }
    }

    private void releaseNow() {
        releasePlayer();
        if (overlay != null) {
            ViewGroup parent = (ViewGroup) overlay.getParent();
            if (parent != null) {
                parent.removeView(overlay);
            }
            overlay = null;
            texture = null;
        }
    }

    private int dp(int v) {
        return Math.round(v * activity.getResources().getDisplayMetrics().density);
    }
}
