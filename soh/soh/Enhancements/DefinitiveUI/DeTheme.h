#pragma once

// OOT D.E.: visual language of the Definitive Edition interface (palette, fonts, textures and the shapes every screen
// is built from: stone panels with aged gold frames, glowing buttons, the Triforce, Navi, hearts and particles).
//
// This file only depends on Dear ImGui, so the screens can be drawn and checked outside the game too
// (see scripts/ootde_ui_preview/). The game side gives it its fonts and textures (DeResources.cpp).

#include <imgui.h>

namespace DefinitiveUI {

// ---------------------------------------------------------------------------------------------------------------------
// Palette: Hylian blue, aged gold, black and graphite, old stone, luminous magic blue, Hyrule field green.

namespace Col {
constexpr ImU32 Rgba(int r, int g, int b, int a = 255) {
    return ((ImU32)a << 24) | ((ImU32)b << 16) | ((ImU32)g << 8) | (ImU32)r;
}
constexpr ImU32 Ink = Rgba(6, 8, 14);
constexpr ImU32 Graphite = Rgba(18, 21, 30);
constexpr ImU32 GraphiteLight = Rgba(32, 37, 50);
constexpr ImU32 Stone = Rgba(62, 68, 82);
constexpr ImU32 HylianDeep = Rgba(10, 22, 48);
constexpr ImU32 Hylian = Rgba(22, 56, 112);
constexpr ImU32 HylianLight = Rgba(46, 104, 184);
constexpr ImU32 Magic = Rgba(84, 182, 255);
constexpr ImU32 MagicLight = Rgba(170, 226, 255);
constexpr ImU32 GoldDark = Rgba(112, 80, 30);
constexpr ImU32 Gold = Rgba(196, 156, 72);
constexpr ImU32 GoldLight = Rgba(244, 214, 138);
constexpr ImU32 GoldPale = Rgba(255, 240, 200);
constexpr ImU32 Field = Rgba(78, 150, 62);
constexpr ImU32 Heart = Rgba(222, 38, 46);
constexpr ImU32 Parchment = Rgba(232, 220, 190);
constexpr ImU32 TextDim = Rgba(150, 156, 170);
} // namespace Col

// Same color with its alpha multiplied by `a` (0..1).
ImU32 Fade(ImU32 col, float a);
// Mix of two colors (t = 0 gives a, 1 gives b).
ImU32 Mix(ImU32 a, ImU32 b, float t);

// ---------------------------------------------------------------------------------------------------------------------
// Fonts and textures (set by the game, or by the preview tool).

struct Fonts {
    ImFont* display = nullptr; // Cinzel Bold: titles and buttons
    ImFont* displayRegular = nullptr; // Cinzel Regular: small captions
    ImFont* body = nullptr; // EB Garamond Medium: descriptions
};
Fonts& GetFonts();

enum Texture {
    TEX_STONE,
    TEX_GLOW,
    TEX_SPARK,
    TEX_NAVI_WING,
    TEX_COUNT,
};
// Returns 0 when the texture is not available; every shape then falls back to plain geometry.
using TextureLookup = ImTextureID (*)(Texture tex);
void SetTextureLookup(TextureLookup lookup);
ImTextureID GetTexture(Texture tex);

// ---------------------------------------------------------------------------------------------------------------------
// Text

// align: 0 = left, 0.5 = centered, 1 = right of pos.x. spacing: extra room between letters, in pixels.
ImVec2 TextSize(ImFont* font, float size, const char* text, float spacing = 0.0f);
void Text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text, float align = 0.0f,
          float spacing = 0.0f, float shadow = 1.0f);
// Gold letters with a dark outline and a vertical gradient (titles).
void GoldText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, const char* text, float align, float spacing,
              float alpha);
// Wraps `text` inside `width`; returns the height used.
float WrappedText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, float width, ImU32 col, const char* text,
                  float lineGap = 1.15f);

// ---------------------------------------------------------------------------------------------------------------------
// Shapes

// Convex polygon filled with a vertical gradient (keeps the alpha of `top` and `bottom`).
void GradientPoly(ImDrawList* dl, const ImVec2* pts, int n, ImU32 top, ImU32 bottom, float y0, float y1);
// Convex polygon filled with a tinted texture stretched over the box a..b.
void TexturedPoly(ImDrawList* dl, const ImVec2* pts, int n, ImTextureID tex, ImVec2 a, ImVec2 b, ImU32 tint);
// Rectangle with cut corners (8 points written to `out`).
int Chamfer(ImVec2* out, ImVec2 a, ImVec2 b, float c);
// Rectangle with pointed left and right ends (6 points written to `out`).
int Pointed(ImVec2* out, ImVec2 a, ImVec2 b, float tip);

void Glow(ImDrawList* dl, ImVec2 center, float radius, ImU32 col);
void Spark(ImDrawList* dl, ImVec2 center, float radius, ImU32 col, float angle = 0.0f);
void Diamond(ImDrawList* dl, ImVec2 center, float r, ImU32 fill, ImU32 edge);

// A stone panel with cut corners, framed in aged gold, with ornaments at the corners.
void Panel(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, float alpha);
// The plaque used for titles: dark blue, pointed ends, gold edge.
void Plaque(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, float alpha);

enum ButtonState { BUTTON_NORMAL, BUTTON_DISABLED };
// A menu button. `highlight` (0..1) is the selection animation: blue light and brighter gold. `press` (0..1) flashes
// it gold for a moment after it is chosen. `time` drives the light sweep.
void Button(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, const char* label, float highlight, float press,
            ButtonState state, float alpha, float time);

void Triforce(ImDrawList* dl, ImVec2 center, float size, float alpha, float glow);
// Triforce inside a medallion with a slowly turning ring of Hylian-like runes.
void TriforceCrest(ImDrawList* dl, ImVec2 center, float radius, float alpha, float time);
// One heart; fill is 0..1 (quarters of a heart show as a partial fill from the left).
void Heart(ImDrawList* dl, ImVec2 center, float size, float fill, float alpha);
// Round button badge with a letter (A, B, Start...).
void ButtonGlyph(ImDrawList* dl, ImVec2 center, float radius, const char* letter, ImU32 col, float alpha);
// Thin gold line with a diamond in the middle and fading ends.
void Divider(ImDrawList* dl, ImVec2 a, float width, float s, float alpha);

// The fairy: glowing orb with four flapping wings.
void Navi(ImDrawList* dl, ImVec2 center, float s, float time, float alpha);

// ---------------------------------------------------------------------------------------------------------------------
// Particles: a fixed pool (no allocation while running).

class Particles {
  public:
    // Golden dust drifting up inside `area`.
    void UpdateDust(float dt, ImVec2 a, ImVec2 b, float s);
    // A sparkle left behind by Navi.
    void EmitTrail(ImVec2 pos, float s);
    void Update(float dt);
    void Draw(ImDrawList* dl, float alpha) const;

  private:
    struct P {
        ImVec2 pos;
        ImVec2 vel;
        float life = 0.0f;
        float maxLife = 1.0f;
        float size = 1.0f;
        float phase = 0.0f;
        bool spark = false;
        ImU32 col = 0;
    };
    static constexpr int kMax = 96;
    P mPool[kMax];
    float mDustTimer = 0.0f;
    unsigned mSeed = 1234567u;
    float Rand();
    P* Free();
};

} // namespace DefinitiveUI
