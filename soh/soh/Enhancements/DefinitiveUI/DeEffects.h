#pragma once

// OOT D.E.: cinematic effects of the Definitive Edition interface, made to be reused by every screen.
//
// Everything is drawn with Dear ImGui primitives and the two soft textures of DeTheme (glow and spark): no shader, no
// render target, no allocation while running (fixed pools), so it costs a few hundred vertices a frame on a phone.
// Like DeTheme, this file only depends on Dear ImGui, so the preview tool (scripts/ootde_ui_preview/) draws it too.

#include <imgui.h>

namespace DefinitiveUI {
namespace Fx {

// ---------------------------------------------------------------------------------------------------------------------
// Curves (t = 0..1)

float EaseInOut(float t);
float EaseIn(float t);
// 1 between `start` and `start + duration` ramping in and out over `edge` seconds; 0 outside.
float Window(float time, float start, float duration, float edge);

// ---------------------------------------------------------------------------------------------------------------------
// Fades and masks

// Plain fade: `col` over a..b with its alpha multiplied by `alpha`.
void ScreenFade(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float alpha);
// Darkens the edges of a..b (a cinema vignette). strength 0..1.
void Vignette(ImDrawList* dl, ImVec2 a, ImVec2 b, float strength);
// Covers a..b with `col`, except for a round hole of `radius` around `center` whose edge fades over `soft` pixels.
// radius <= 0 covers everything. The classic iris of a film transition.
void IrisMask(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec2 center, float radius, float soft, ImU32 col);
// Covers a..b with `col` from the left edge up to t (0..1) of its width, with a soft edge of `soft` pixels: a wipe.
void WipeMask(ImDrawList* dl, ImVec2 a, ImVec2 b, float t, float soft, ImU32 col);

// ---------------------------------------------------------------------------------------------------------------------
// Light

// Golden light gathered at `center`: a few soft layers and a white core. intensity 0..1 (more than 1 overexposes).
void GoldenGlow(ImDrawList* dl, ImVec2 center, float radius, float intensity, float time);
// A small lens flare: horizontal streak, star and a faint halo. Good on a sun, a gem, the Triforce.
void LensFlare(ImDrawList* dl, ImVec2 center, float size, float alpha, float time);
// "Time distortion": rippling rings spreading out of `center` (t = 0..1 over their life), gold and Hylian blue, with
// a slight wobble like air bending around magic.
void TimeRipple(ImDrawList* dl, ImVec2 center, float maxRadius, float t, float alpha, float s);

// ---------------------------------------------------------------------------------------------------------------------
// Magic particles: gold and blue motes that can be pulled toward a point (gathering magic) or blown out of it.

class MagicParticles {
  public:
    // `count` motes born along the edges of a..b, drifting toward `target`.
    void EmitFromEdges(ImVec2 a, ImVec2 b, ImVec2 target, float s, int count);
    // `count` motes thrown out of `center` in every direction (an explosion of light).
    void EmitRing(ImVec2 center, float radius, float s, int count);
    // pull: how strongly the motes go to the target (0: free).
    void Update(float dt, float pull);
    void Draw(ImDrawList* dl, float alpha) const;
    // Frees every mote (after a transition).
    void Clear();
    bool Active() const;

  private:
    struct P {
        ImVec2 pos;
        ImVec2 vel;
        ImVec2 target;
        float life = 0.0f;
        float maxLife = 1.0f;
        float size = 1.0f;
        float phase = 0.0f;
        bool spark = false;
        bool gather = false;
        ImU32 col = 0;
    };
    static constexpr int kMax = 160;
    P mPool[kMax];
    unsigned mSeed = 7654321u;
    float Rand();
    P* Free();
};

// ---------------------------------------------------------------------------------------------------------------------
// Ambient mist: a few large soft clouds drifting slowly over the lower part of a..b.

class Mist {
  public:
    void Update(float dt, ImVec2 a, ImVec2 b, float s);
    void Draw(ImDrawList* dl, float alpha, ImU32 col) const;
    void Clear();

  private:
    static constexpr int kClouds = 9;
    ImVec2 mPos[kClouds];
    float mSpeed[kClouds] = {};
    float mRadius[kClouds] = {};
    float mPhase[kClouds] = {};
    bool mReady = false;
    ImVec2 mA, mB;
};

// ---------------------------------------------------------------------------------------------------------------------
// The opening of the menu: the Definitive Edition image fades, golden motes come in from the edges, the light gathers
// in the middle, the Triforce appears in it, and the light opens like an iris to show the menu.
//
// Drawn in three layers by the caller: DrawCover (under the opening image: black with the opening iris), the opening
// image itself with SplashAlpha(), then DrawEffects on top. The music is never touched.

class CinematicReveal {
  public:
    // Seconds since Start() at which the menu starts to show (the iris opens) and at which it is all over.
    static constexpr float kIrisStart = 1.75f;
    static constexpr float kIrisLength = 0.8f;
    static constexpr float kEnd = 2.8f;

    void Start();
    // The player skipped: jump straight to the opening of the iris (nothing is lost, it just goes faster).
    void Skip();
    void Update(float dt, ImVec2 origin, ImVec2 size);
    void DrawCover(ImDrawList* dl, ImVec2 origin, ImVec2 size) const;
    void DrawEffects(ImDrawList* dl, ImVec2 origin, ImVec2 size);
    // Frees the particles; Running() becomes false.
    void Release();

    bool Running() const { return mRunning; }
    bool Done() const { return mRunning && mTime >= kEnd; }
    float Time() const { return mTime; }
    // The menu can be seen (and touched) from here on.
    bool MenuShown() const { return mRunning && mTime >= kIrisStart; }
    // Alpha of the opening image while it fades.
    float SplashAlpha() const;

  private:
    bool mRunning = false;
    float mTime = 0.0f;
    float mEmitTimer = 0.0f;
    bool mBurst = false;
    MagicParticles mMotes;
};

} // namespace Fx
} // namespace DefinitiveUI
