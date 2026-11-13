// OOT D.E.: shapes of the Definitive Edition interface. See DeTheme.h.
#include "DeTheme.h"

#include <imgui_internal.h>
#include <cmath>
#include <cstring>

namespace DefinitiveUI {

static constexpr float kPi = 3.14159265f;

ImU32 Fade(ImU32 col, float a) {
    a = ImClamp(a, 0.0f, 1.0f);
    const ImU32 alpha = (ImU32)(((col >> IM_COL32_A_SHIFT) & 0xFF) * a + 0.5f);
    return (col & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

ImU32 Mix(ImU32 a, ImU32 b, float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    ImU32 out = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        const float ca = (float)((a >> shift) & 0xFF);
        const float cb = (float)((b >> shift) & 0xFF);
        out |= ((ImU32)(ca + (cb - ca) * t + 0.5f) & 0xFF) << shift;
    }
    return out;
}

Fonts& GetFonts() {
    static Fonts sFonts;
    return sFonts;
}

static TextureLookup sTextureLookup = nullptr;

void SetTextureLookup(TextureLookup lookup) {
    sTextureLookup = lookup;
}

ImTextureID GetTexture(Texture tex) {
    return sTextureLookup != nullptr ? sTextureLookup(tex) : (ImTextureID)0;
}

ImVec2 TextureSize(Texture tex) {
    switch (tex) {
        case TEX_STONE:
            return ImVec2(256, 256);
        case TEX_GLOW:
            return ImVec2(128, 128);
        case TEX_SPARK:
            return ImVec2(64, 64);
        case TEX_NAVI_WING:
            return ImVec2(96, 64);
        case TEX_TITLE_BG:
            return ImVec2(2880, 1250);
        case TEX_LOGO:
            return ImVec2(1944, 672);
        case TEX_BUTTON_SELECTED:
            return ImVec2(240, 136);
        case TEX_BUTTON_NORMAL:
        case TEX_BUTTON_PRESSED:
            return ImVec2(224, 136);
        case TEX_FRAME:
            return ImVec2(600, 232);
        default:
            return ImVec2(1, 1);
    }
}

static ImFont* OrDefault(ImFont* font) {
    return font != nullptr ? font : ImGui::GetFont();
}

// ---------------------------------------------------------------------------------------------------------------------
// Text

ImVec2 TextSize(ImFont* font, float size, const char* text, float spacing) {
    font = OrDefault(font);
    ImVec2 sz = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    if (spacing != 0.0f) {
        int count = 0;
        for (const char* p = text; *p != '\0';) {
            unsigned int c;
            p += ImTextCharFromUtf8(&c, p, nullptr);
            count++;
        }
        if (count > 1) {
            sz.x += spacing * (count - 1);
        }
    }
    return sz;
}

static void RawText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text, float spacing) {
    if (spacing == 0.0f) {
        dl->AddText(font, size, pos, col, text);
        return;
    }
    const float scale = size / font->FontSize;
    char buf[8];
    for (const char* p = text; *p != '\0';) {
        unsigned int c;
        const int len = ImTextCharFromUtf8(&c, p, nullptr);
        memcpy(buf, p, len);
        buf[len] = '\0';
        dl->AddText(font, size, pos, col, buf);
        pos.x += font->GetCharAdvance((ImWchar)c) * scale + spacing;
        p += len;
    }
}

void Text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text, float align,
          float spacing, float shadow) {
    font = OrDefault(font);
    if (align != 0.0f) {
        pos.x -= TextSize(font, size, text, spacing).x * align;
    }
    pos = ImFloor(pos);
    if (shadow > 0.0f) {
        const float o = ImMax(1.0f, size * 0.06f);
        RawText(dl, font, size, pos + ImVec2(o, o), Fade(Col::Ink, (float)((col >> 24) & 0xFF) / 255.0f * 0.8f * shadow),
                text, spacing);
    }
    RawText(dl, font, size, pos, col, text, spacing);
}

void GoldText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, const char* text, float align, float spacing,
              float alpha) {
    font = OrDefault(font);
    const ImVec2 sz = TextSize(font, size, text, spacing);
    pos.x -= sz.x * align;
    pos = ImFloor(pos);
    const float o = ImMax(1.0f, size * 0.045f);
    const ImU32 outline = Fade(Col::Rgba(30, 18, 4), alpha * 0.95f);
    for (int i = 0; i < 8; i++) {
        const float ang = i * kPi / 4.0f;
        RawText(dl, font, size, pos + ImVec2(cosf(ang) * o, sinf(ang) * o), outline, text, spacing);
    }
    RawText(dl, font, size, pos + ImVec2(0, o * 2.2f), Fade(Col::Ink, alpha * 0.6f), text, spacing);
    const int v0 = dl->VtxBuffer.Size;
    RawText(dl, font, size, pos, Fade(IM_COL32_WHITE, alpha), text, spacing);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, pos + ImVec2(0, size * 0.15f),
                                                  pos + ImVec2(0, size * 0.95f), Col::GoldPale, Col::Gold);
}

float WrappedText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, float width, ImU32 col, const char* text,
                  float lineGap) {
    font = OrDefault(font);
    const float scale = size / font->FontSize;
    const char* p = text;
    const char* end = text + strlen(text);
    float y = pos.y;
    while (p < end) {
        const char* lineEnd = font->CalcWordWrapPositionA(scale, p, end, width);
        if (lineEnd == p) {
            lineEnd++;
        }
        // Honor explicit line breaks.
        const char* nl = (const char*)memchr(p, '\n', lineEnd - p);
        if (nl != nullptr) {
            lineEnd = nl;
        }
        dl->AddText(font, size, ImFloor(ImVec2(pos.x + 1, y + 1)), Fade(Col::Ink, (float)((col >> 24) & 0xFF) / 400.0f),
                    p, lineEnd);
        dl->AddText(font, size, ImFloor(ImVec2(pos.x, y)), col, p, lineEnd);
        y += size * lineGap;
        p = lineEnd;
        while (p < end && (*p == ' ' || *p == '\n')) {
            p++;
        }
    }
    return y - pos.y;
}

// ---------------------------------------------------------------------------------------------------------------------
// Shapes

void GradientPoly(ImDrawList* dl, const ImVec2* pts, int n, ImU32 top, ImU32 bottom, float y0, float y1) {
    const int v0 = dl->VtxBuffer.Size;
    dl->AddConvexPolyFilled(pts, n, IM_COL32_WHITE);
    // Alpha comes from the vertices; give them the alpha of the gradient first.
    const int v1 = dl->VtxBuffer.Size;
    for (int i = v0; i < v1; i++) {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float t = ImClamp((v.pos.y - y0) / ImMax(1.0f, y1 - y0), 0.0f, 1.0f);
        const float a = ImLerp((float)((top >> 24) & 0xFF), (float)((bottom >> 24) & 0xFF), t) / 255.0f;
        v.col = Fade(v.col, a);
    }
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, v1, ImVec2(0, y0), ImVec2(0, y1), top, bottom);
}

void TexturedPoly(ImDrawList* dl, const ImVec2* pts, int n, ImTextureID tex, ImVec2 a, ImVec2 b, ImU32 tint) {
    if (tex == (ImTextureID)0) {
        dl->AddConvexPolyFilled(pts, n, tint);
        return;
    }
    dl->PushTextureID(tex);
    const int v0 = dl->VtxBuffer.Size;
    dl->AddConvexPolyFilled(pts, n, tint);
    // Keep the texture's own proportions: 1 texel per ~2 pixels of a 1080p screen, repeated by mirroring the UVs.
    ImGui::ShadeVertsLinearUV(dl, v0, dl->VtxBuffer.Size, a, b, ImVec2(0, 0), ImVec2(1, 1), true);
    dl->PopTextureID();
}

int Chamfer(ImVec2* out, ImVec2 a, ImVec2 b, float c) {
    out[0] = ImVec2(a.x + c, a.y);
    out[1] = ImVec2(b.x - c, a.y);
    out[2] = ImVec2(b.x, a.y + c);
    out[3] = ImVec2(b.x, b.y - c);
    out[4] = ImVec2(b.x - c, b.y);
    out[5] = ImVec2(a.x + c, b.y);
    out[6] = ImVec2(a.x, b.y - c);
    out[7] = ImVec2(a.x, a.y + c);
    return 8;
}

int Pointed(ImVec2* out, ImVec2 a, ImVec2 b, float tip) {
    const float my = (a.y + b.y) * 0.5f;
    out[0] = ImVec2(a.x + tip, a.y);
    out[1] = ImVec2(b.x - tip, a.y);
    out[2] = ImVec2(b.x, my);
    out[3] = ImVec2(b.x - tip, b.y);
    out[4] = ImVec2(a.x + tip, b.y);
    out[5] = ImVec2(a.x, my);
    return 6;
}

static ImVec2 Inset(const ImVec2* pts, int n, int i, float d) {
    // Moves point i towards the inside along the bisector of its two edges (convex, clockwise in screen space).
    const ImVec2 p = pts[i];
    const ImVec2 prev = pts[(i + n - 1) % n];
    const ImVec2 next = pts[(i + 1) % n];
    ImVec2 e0 = p - prev;
    ImVec2 e1 = next - p;
    const float l0 = ImMax(1e-4f, sqrtf(e0.x * e0.x + e0.y * e0.y));
    const float l1 = ImMax(1e-4f, sqrtf(e1.x * e1.x + e1.y * e1.y));
    e0 /= l0;
    e1 /= l1;
    // Inward normals (screen y points down, polygon is clockwise on screen).
    const ImVec2 n0(-e0.y, e0.x);
    const ImVec2 n1(-e1.y, e1.x);
    ImVec2 m = n0 + n1;
    const float lm = ImMax(1e-4f, sqrtf(m.x * m.x + m.y * m.y));
    m /= lm;
    const float cosHalf = ImMax(0.2f, m.x * n0.x + m.y * n0.y);
    return p + m * (d / cosHalf);
}

static void InsetPoly(ImVec2* out, const ImVec2* pts, int n, float d) {
    for (int i = 0; i < n; i++) {
        out[i] = Inset(pts, n, i, d);
    }
}

void Glow(ImDrawList* dl, ImVec2 center, float radius, ImU32 col) {
    ImTextureID tex = GetTexture(TEX_GLOW);
    if (tex != (ImTextureID)0) {
        dl->AddImage(tex, center - ImVec2(radius, radius), center + ImVec2(radius, radius), ImVec2(0, 0), ImVec2(1, 1),
                     col);
        return;
    }
    // Without the texture: a few transparent discs.
    for (int i = 4; i >= 1; i--) {
        dl->AddCircleFilled(center, radius * i / 4.0f, Fade(col, 0.18f), 24);
    }
}

void Spark(ImDrawList* dl, ImVec2 center, float radius, ImU32 col, float angle) {
    ImTextureID tex = GetTexture(TEX_SPARK);
    if (tex == (ImTextureID)0) {
        dl->AddCircleFilled(center, radius * 0.25f, col, 8);
        return;
    }
    const float c = cosf(angle) * radius;
    const float s = sinf(angle) * radius;
    dl->AddImageQuad(tex, center + ImVec2(-c + s, -s - c), center + ImVec2(c + s, s - c), center + ImVec2(c - s, s + c),
                     center + ImVec2(-c - s, -s + c), ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), col);
}

void Diamond(ImDrawList* dl, ImVec2 c, float r, ImU32 fill, ImU32 edge) {
    const ImVec2 pts[4] = { ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y) };
    dl->AddConvexPolyFilled(pts, 4, fill);
    if ((edge >> 24) != 0) {
        dl->AddPolyline(pts, 4, edge, ImDrawFlags_Closed, ImMax(1.0f, r * 0.18f));
    }
}

static void GoldOutline(ImDrawList* dl, const ImVec2* pts, int n, float s, float alpha, float bright) {
    // Bevelled metal: dark under-line, gold body, light top line.
    dl->AddPolyline(pts, n, Fade(Col::Ink, alpha * 0.8f), ImDrawFlags_Closed, 4.5f * s);
    dl->AddPolyline(pts, n, Fade(Mix(Col::GoldDark, Col::Gold, 0.4f + bright * 0.6f), alpha), ImDrawFlags_Closed,
                    3.0f * s);
    dl->AddPolyline(pts, n, Fade(Mix(Col::Gold, Col::GoldPale, bright), alpha * 0.9f), ImDrawFlags_Closed,
                    ImMax(1.0f, 1.1f * s));
}

static void CornerOrnament(ImDrawList* dl, ImVec2 corner, ImVec2 dir, float s, float alpha) {
    // dir: (+1/-1, +1/-1) pointing into the panel.
    const ImU32 gold = Fade(Col::Gold, alpha);
    const ImU32 light = Fade(Col::GoldLight, alpha);
    const float L = 46.0f * s;
    const ImVec2 o = corner + ImVec2(dir.x * 9.0f * s, dir.y * 9.0f * s);
    dl->AddLine(o, o + ImVec2(dir.x * L, 0), gold, 2.0f * s);
    dl->AddLine(o, o + ImVec2(0, dir.y * L), gold, 2.0f * s);
    // Little curl at both ends.
    dl->AddCircle(o + ImVec2(dir.x * (L + 4 * s), 0), 4.0f * s, gold, 10, 1.6f * s);
    dl->AddCircle(o + ImVec2(0, dir.y * (L + 4 * s)), 4.0f * s, gold, 10, 1.6f * s);
    Diamond(dl, o + ImVec2(dir.x * 10.0f * s, dir.y * 10.0f * s), 6.5f * s, light, Fade(Col::GoldDark, alpha));
    Diamond(dl, corner + ImVec2(dir.x * 2.5f * s, dir.y * 2.5f * s), 5.0f * s, Fade(Col::Gold, alpha), 0);
}

void Panel(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, float alpha) {
    if (alpha <= 0.0f) {
        return;
    }
    const float c = 22.0f * s;
    ImVec2 outer[8];
    Chamfer(outer, a, b, c);

    // Soft shadow and a faint blue aura around the panel.
    for (int i = 3; i >= 1; i--) {
        ImVec2 grow[8];
        InsetPoly(grow, outer, 8, -6.0f * s * i);
        dl->AddConvexPolyFilled(grow, 8, Fade(Col::Ink, alpha * 0.10f));
    }

    // Stone body, darkened towards the bottom.
    TexturedPoly(dl, outer, 8, GetTexture(TEX_STONE), a, a + ImVec2(512.0f * s, 512.0f * s),
                 Fade(Col::Rgba(58, 68, 92), alpha * 0.96f));
    GradientPoly(dl, outer, 8, Fade(Col::Rgba(14, 26, 54), alpha * 0.45f), Fade(Col::Ink, alpha * 0.82f), a.y, b.y);

    // Inner recess.
    ImVec2 inner[8];
    InsetPoly(inner, outer, 8, 11.0f * s);
    dl->AddPolyline(inner, 8, Fade(Col::Ink, alpha * 0.7f), ImDrawFlags_Closed, 3.0f * s);
    dl->AddPolyline(inner, 8, Fade(Col::GoldDark, alpha * 0.9f), ImDrawFlags_Closed, 1.2f * s);

    GoldOutline(dl, outer, 8, s, alpha, 0.35f);

    CornerOrnament(dl, inner[7] + ImVec2(0, -c * 0.7f), ImVec2(1, 1), s, alpha);
    CornerOrnament(dl, inner[2] + ImVec2(0, -c * 0.7f), ImVec2(-1, 1), s, alpha);
    CornerOrnament(dl, inner[6] + ImVec2(0, c * 0.7f), ImVec2(1, -1), s, alpha);
    CornerOrnament(dl, inner[3] + ImVec2(0, c * 0.7f), ImVec2(-1, -1), s, alpha);
}

void Plaque(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, float alpha) {
    ImVec2 pts[6];
    Pointed(pts, a, b, (b.y - a.y) * 0.5f);
    TexturedPoly(dl, pts, 6, GetTexture(TEX_STONE), a, a + ImVec2(400 * s, 400 * s), Fade(Col::Rgba(40, 52, 80), alpha));
    GradientPoly(dl, pts, 6, Fade(Col::Hylian, alpha * 0.55f), Fade(Col::HylianDeep, alpha * 0.85f), a.y, b.y);
    GoldOutline(dl, pts, 6, s, alpha, 0.6f);
    Diamond(dl, pts[5], 6.0f * s, Fade(Col::GoldLight, alpha), Fade(Col::GoldDark, alpha));
    Diamond(dl, pts[2], 6.0f * s, Fade(Col::GoldLight, alpha), Fade(Col::GoldDark, alpha));
}

void Button(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, const char* label, float highlight, float press,
            ButtonState state, float alpha, float time) {
    if (alpha <= 0.0f) {
        return;
    }
    const float h = b.y - a.y;
    const float tip = h * 0.42f;
    const bool disabled = state == BUTTON_DISABLED;
    const float hl = disabled ? highlight * 0.35f : highlight;

    // The buttons of the reference art: left end, stretched middle, right end. The texture is taller than the
    // button body (it has room for the glow), so it is drawn a bit larger than a..b.
    ImTextureID normalTex = GetTexture(TEX_BUTTON_NORMAL);
    ImTextureID selectedTex = GetTexture(TEX_BUTTON_SELECTED);
    ImTextureID pressedTex = GetTexture(TEX_BUTTON_PRESSED);
    if (normalTex != (ImTextureID)0 && selectedTex != (ImTextureID)0 && pressedTex != (ImTextureID)0) {
        auto draw = [&](ImTextureID tex, Texture id, float a01) {
            if (a01 <= 0.0f) {
                return;
            }
            const ImVec2 ts = TextureSize(id);
            const float cap = (id == TEX_BUTTON_SELECTED) ? 108.0f : 100.0f;
            const float k = (h * 1.34f) / ts.y;
            const float my = (a.y + b.y) * 0.5f;
            const float y0 = my - ts.y * k * 0.5f;
            const float y1 = my + ts.y * k * 0.5f;
            const float x0 = a.x - (id == TEX_BUTTON_SELECTED ? 14.0f : 8.0f) * k;
            const float x1 = b.x + (id == TEX_BUTTON_SELECTED ? 14.0f : 8.0f) * k;
            const ImU32 tint = Fade(disabled ? Col::Rgba(150, 150, 160) : IM_COL32_WHITE, alpha * a01);
            const float cw = cap * k;
            const float u0 = cap / ts.x;
            const float u1 = (ts.x - cap) / ts.x;
            dl->AddImage(tex, ImVec2(x0, y0), ImVec2(x0 + cw, y1), ImVec2(0, 0), ImVec2(u0, 1), tint);
            dl->AddImage(tex, ImVec2(x0 + cw, y0), ImVec2(x1 - cw, y1), ImVec2(u0, 0), ImVec2(u1, 1), tint);
            dl->AddImage(tex, ImVec2(x1 - cw, y0), ImVec2(x1, y1), ImVec2(u1, 0), ImVec2(1, 1), tint);
        };
        const float pulse = 0.85f + 0.15f * sinf(time * 3.2f);
        if (hl > 0.01f) {
            Glow(dl, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), (b.x - a.x) * 0.62f,
                 Fade(Col::Magic, alpha * hl * 0.22f * pulse));
        }
        draw(normalTex, TEX_BUTTON_NORMAL, 1.0f - ImMax(hl, press));
        draw(selectedTex, TEX_BUTTON_SELECTED, hl * (1.0f - press));
        draw(pressedTex, TEX_BUTTON_PRESSED, press);
        Fonts& f = GetFonts();
        const float size = h * 0.47f;
        const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f - size * 0.58f);
        const ImU32 col = disabled ? Fade(Col::TextDim, alpha * 0.6f)
                                   : Fade(Mix(Col::Parchment, IM_COL32_WHITE, hl), alpha);
        Text(dl, f.display, size, center, col, label, 0.5f, size * 0.03f, 1.0f);
        return;
    }

    ImVec2 pts[6];
    Pointed(pts, a, b, tip);

    // Blue aura of the selected button.
    if (hl > 0.01f) {
        const float pulse = 0.85f + 0.15f * sinf(time * 3.2f);
        Glow(dl, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), (b.x - a.x) * 0.62f,
             Fade(Col::Magic, alpha * hl * 0.30f * pulse));
        for (int i = 3; i >= 1; i--) {
            ImVec2 grow[6];
            InsetPoly(grow, pts, 6, -2.6f * s * i);
            dl->AddPolyline(grow, 6, Fade(Col::Magic, alpha * hl * pulse * (0.32f - i * 0.07f)), ImDrawFlags_Closed,
                            2.6f * s);
        }
    }

    // Body: graphite stone; deep Hylian blue when selected; gold flash when chosen.
    TexturedPoly(dl, pts, 6, GetTexture(TEX_STONE), a + ImVec2(time * 0.0f, 0), a + ImVec2(380 * s, 380 * s),
                 Fade(Col::Rgba(46, 52, 66), alpha));
    const ImU32 top = Mix(Mix(Col::Rgba(30, 35, 48, 200), Col::Rgba(40, 104, 196, 235), hl), Col::GoldLight, press);
    const ImU32 bottom = Mix(Mix(Col::Rgba(8, 10, 16, 235), Col::Rgba(10, 34, 82, 245), hl), Col::GoldDark, press);
    GradientPoly(dl, pts, 6, Fade(top, alpha), Fade(bottom, alpha), a.y, b.y);

    // Glossy top half.
    {
        ImVec2 gloss[6];
        InsetPoly(gloss, pts, 6, 3.0f * s);
        const float my = (a.y + b.y) * 0.5f;
        ImVec2 half[4] = { gloss[5], gloss[0], gloss[1], gloss[2] };
        half[0].y = my;
        half[3].y = my;
        GradientPoly(dl, half, 4, Fade(IM_COL32_WHITE, alpha * (0.06f + 0.10f * hl)), Fade(IM_COL32_WHITE, 0.0f), a.y,
                     my);
    }

    // Light sweep across the selected button.
    if (hl > 0.05f && !disabled) {
        const float period = 2.6f;
        const float u = fmodf(time, period) / period;
        const float x = ImLerp(a.x - h, b.x + h, u);
        ImVec2 band[4] = { ImVec2(x - h * 0.35f, a.y + 3 * s), ImVec2(x + h * 0.10f, a.y + 3 * s),
                           ImVec2(x - h * 0.05f, b.y - 3 * s), ImVec2(x - h * 0.50f, b.y - 3 * s) };
        // Keep the band inside the button.
        for (ImVec2& p : band) {
            p.x = ImClamp(p.x, a.x + tip, b.x - tip);
        }
        dl->AddConvexPolyFilled(band, 4, Fade(IM_COL32_WHITE, alpha * hl * 0.10f));
    }

    // Frame.
    dl->AddPolyline(pts, 6, Fade(Col::Ink, alpha * 0.9f), ImDrawFlags_Closed, 4.0f * s);
    dl->AddPolyline(pts, 6, Fade(Mix(Col::GoldDark, Col::GoldLight, hl * 0.85f + press), alpha * (disabled ? 0.6f : 1)),
                    ImDrawFlags_Closed, (1.6f + hl * 0.9f) * s);
    {
        ImVec2 in[6];
        InsetPoly(in, pts, 6, 4.0f * s);
        dl->AddPolyline(in, 6, Fade(Mix(Col::GoldDark, Col::Gold, hl), alpha * (0.35f + 0.4f * hl)), ImDrawFlags_Closed,
                        ImMax(1.0f, 0.9f * s));
    }

    // Gold arrow tips at both ends of the selected button.
    if (hl > 0.01f) {
        const float my = (a.y + b.y) * 0.5f;
        const float w = 11.0f * s * hl;
        const float bob = sinf(time * 5.0f) * 2.0f * s;
        const ImU32 g = Fade(Col::GoldLight, alpha * hl);
        const ImU32 gd = Fade(Col::GoldDark, alpha * hl);
        const ImVec2 l[3] = { ImVec2(a.x - 4 * s - w - bob, my - w), ImVec2(a.x - 4 * s - bob, my),
                              ImVec2(a.x - 4 * s - w - bob, my + w) };
        const ImVec2 r[3] = { ImVec2(b.x + 4 * s + w + bob, my - w), ImVec2(b.x + 4 * s + w + bob, my + w),
                              ImVec2(b.x + 4 * s + bob, my) };
        dl->AddTriangleFilled(l[0], l[1], l[2], g);
        dl->AddTriangle(l[0], l[1], l[2], gd, 1.0f * s);
        dl->AddTriangleFilled(r[0], r[1], r[2], g);
        dl->AddTriangle(r[0], r[1], r[2], gd, 1.0f * s);
    }

    // Label.
    Fonts& f = GetFonts();
    const float size = h * 0.44f;
    const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f - size * 0.56f);
    ImU32 col = disabled ? Fade(Col::TextDim, alpha * 0.55f) : Fade(Mix(Col::Parchment, IM_COL32_WHITE, hl), alpha);
    if (hl > 0.05f && !disabled) {
        Glow(dl, center + ImVec2(0, size * 0.55f), size * 2.0f, Fade(Col::MagicLight, alpha * hl * 0.18f));
    }
    Text(dl, f.display, size, center, col, label, 0.5f, size * 0.04f, disabled ? 0.4f : 1.0f);
}

void Triforce(ImDrawList* dl, ImVec2 c, float size, float alpha, float glow) {
    // size = height of the whole Triforce.
    const float h = size;
    const float w = h * 1.1547f; // equilateral
    const ImVec2 top(c.x, c.y - h * 0.5f);
    const ImVec2 bl(c.x - w * 0.5f, c.y + h * 0.5f);
    const ImVec2 br(c.x + w * 0.5f, c.y + h * 0.5f);
    const ImVec2 ml = (top + bl) * 0.5f;
    const ImVec2 mr = (top + br) * 0.5f;
    const ImVec2 mb = (bl + br) * 0.5f;
    if (glow > 0.0f) {
        Glow(dl, c, size * 1.25f, Fade(Col::GoldLight, alpha * glow * 0.45f));
    }
    const ImVec2 tris[3][3] = { { top, mr, ml }, { ml, mb, bl }, { mr, br, mb } };
    for (const auto& t : tris) {
        const int v0 = dl->VtxBuffer.Size;
        dl->AddTriangleFilled(t[0], t[1], t[2], Fade(IM_COL32_WHITE, alpha));
        ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, t[0], (t[1] + t[2]) * 0.5f,
                                                      Col::GoldPale, Col::Gold);
        dl->AddTriangle(t[0], t[1], t[2], Fade(Col::GoldDark, alpha), ImMax(1.0f, size * 0.025f));
    }
}

void TriforceCrest(ImDrawList* dl, ImVec2 c, float r, float alpha, float time) {
    if (alpha <= 0.0f) {
        return;
    }
    Glow(dl, c, r * 2.0f, Fade(Col::Magic, alpha * 0.28f));
    // Medallion.
    dl->AddCircleFilled(c, r, Fade(Col::Ink, alpha * 0.9f), 48);
    {
        const int v0 = dl->VtxBuffer.Size;
        dl->AddCircleFilled(c, r * 0.94f, Fade(IM_COL32_WHITE, alpha), 48);
        ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, c - ImVec2(0, r), c + ImVec2(0, r),
                                                      Col::Hylian, Col::HylianDeep);
    }
    dl->AddCircle(c, r, Fade(Col::Gold, alpha), 48, 3.0f * r / 60.0f);
    dl->AddCircle(c, r * 0.78f, Fade(Col::GoldDark, alpha), 48, 1.5f * r / 60.0f);
    // Rune ring, turning slowly: short glyph strokes between the two circles.
    const int count = 18;
    const float ringR = r * 0.88f;
    for (int i = 0; i < count; i++) {
        const float ang = time * 0.25f + i * 2.0f * kPi / count;
        const ImVec2 dir(cosf(ang), sinf(ang));
        const ImVec2 tan(-dir.y, dir.x);
        const ImVec2 p = c + dir * ringR;
        const float g = r * 0.045f;
        const ImU32 col = Fade(Col::MagicLight, alpha * (0.55f + 0.45f * sinf(time * 2.0f + i)));
        // Each rune is one of four small shapes.
        switch (i % 4) {
            case 0:
                dl->AddLine(p - tan * g * 1.2f, p + tan * g * 1.2f, col, 1.4f * r / 60.0f);
                dl->AddLine(p - dir * g, p + dir * g, col, 1.4f * r / 60.0f);
                break;
            case 1:
                dl->AddTriangle(p - dir * g, p + dir * g + tan * g, p + dir * g - tan * g, col, 1.2f * r / 60.0f);
                break;
            case 2:
                dl->AddCircle(p, g * 0.9f, col, 8, 1.2f * r / 60.0f);
                break;
            default:
                dl->AddLine(p - tan * g - dir * g, p + tan * g + dir * g, col, 1.4f * r / 60.0f);
                dl->AddLine(p + tan * g - dir * g, p - tan * g + dir * g, col, 1.4f * r / 60.0f);
                break;
        }
    }
    Triforce(dl, c + ImVec2(0, r * 0.04f), r * 0.98f, alpha, 0.6f + 0.25f * sinf(time * 2.2f));
}

void Heart(ImDrawList* dl, ImVec2 c, float size, float fill, float alpha) {
    // Two circles and a triangle, filled from the left for partial hearts.
    auto shape = [&](ImU32 col, float grow) {
        const float r = size * 0.27f + grow;
        const ImVec2 l(c.x - size * 0.22f, c.y - size * 0.12f);
        const ImVec2 rr(c.x + size * 0.22f, c.y - size * 0.12f);
        dl->AddCircleFilled(l, r, col, 16);
        dl->AddCircleFilled(rr, r, col, 16);
        dl->AddTriangleFilled(ImVec2(c.x - size * 0.48f - grow, c.y - size * 0.04f),
                              ImVec2(c.x + size * 0.48f + grow, c.y - size * 0.04f),
                              ImVec2(c.x, c.y + size * 0.46f + grow * 1.4f), col);
    };
    shape(Fade(Col::Ink, alpha * 0.85f), size * 0.07f);
    shape(Fade(Col::Rgba(70, 20, 24), alpha), 0.0f);
    if (fill > 0.0f) {
        const ImVec2 clipA(c.x - size, c.y - size);
        const ImVec2 clipB(c.x - size * 0.5f + size * fill, c.y + size);
        dl->PushClipRect(clipA, clipB, true);
        shape(Fade(Col::Heart, alpha), 0.0f);
        dl->AddCircleFilled(ImVec2(c.x - size * 0.24f, c.y - size * 0.2f), size * 0.08f, Fade(IM_COL32_WHITE, alpha * 0.7f),
                            8);
        dl->PopClipRect();
    }
}

void ButtonGlyph(ImDrawList* dl, ImVec2 c, float r, const char* letter, ImU32 col, float alpha) {
    dl->AddCircleFilled(c, r + 1.5f, Fade(Col::Ink, alpha), 24);
    const int v0 = dl->VtxBuffer.Size;
    dl->AddCircleFilled(c, r, Fade(IM_COL32_WHITE, alpha), 24);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, c - ImVec2(0, r), c + ImVec2(0, r),
                                                  Mix(col, IM_COL32_WHITE, 0.35f), Mix(col, Col::Ink, 0.35f));
    dl->AddCircle(c, r, Fade(Col::GoldLight, alpha * 0.8f), 24, ImMax(1.0f, r * 0.1f));
    Fonts& f = GetFonts();
    const float size = r * 1.3f;
    Text(dl, f.display, size, c - ImVec2(0, size * 0.55f), Fade(IM_COL32_WHITE, alpha), letter, 0.5f, 0.0f, 0.6f);
}

void NineSlice(ImDrawList* dl, ImTextureID tex, ImVec2 ts, ImVec2 a, ImVec2 b, ImVec2 corner, float scale,
               ImU32 tint) {
    // Texel edges of the three columns/rows: corner, a 2-texel strip next to it (stretched), opposite corner.
    const float tx[4] = { 0.0f, corner.x, corner.x + 2.0f, ts.x - corner.x };
    const float ty[4] = { 0.0f, corner.y, corner.y + 2.0f, ts.y - corner.y };
    const float cw = ImMin(corner.x * scale, (b.x - a.x) * 0.5f);
    const float ch = ImMin(corner.y * scale, (b.y - a.y) * 0.5f);
    const float sx[4] = { a.x, a.x + cw, b.x - cw, b.x };
    const float sy[4] = { a.y, a.y + ch, b.y - ch, b.y };
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            if (sx[i + 1] <= sx[i] || sy[j + 1] <= sy[j]) {
                continue;
            }
            const float u0 = tx[i] / ts.x;
            const float u1 = (i == 2 ? ts.x : tx[i + 1]) / ts.x;
            const float v0 = ty[j] / ts.y;
            const float v1 = (j == 2 ? ts.y : ty[j + 1]) / ts.y;
            // The middle column/row uses the strip; the last one starts at the opposite corner.
            const float uu0 = (i == 2) ? tx[3] / ts.x : u0;
            const float vv0 = (j == 2) ? ty[3] / ts.y : v0;
            dl->AddImage(tex, ImVec2(sx[i], sy[j]), ImVec2(sx[i + 1], sy[j + 1]), ImVec2(uu0, vv0), ImVec2(u1, v1),
                         tint);
        }
    }
}

void CoverImage(ImDrawList* dl, ImTextureID tex, ImVec2 ts, ImVec2 a, ImVec2 b, float zoom, ImVec2 focus, ImU32 tint) {
    const ImVec2 size = b - a;
    // Part of the texture (in UV) with the proportions of the screen, zoomed around `focus` (UV).
    const float screenAspect = size.x / ImMax(1.0f, size.y);
    const float texAspect = ts.x / ts.y;
    ImVec2 uvSize(1.0f, 1.0f);
    if (screenAspect > texAspect) {
        uvSize.y = texAspect / screenAspect;
    } else {
        uvSize.x = screenAspect / texAspect;
    }
    uvSize /= ImMax(1.0f, zoom);
    ImVec2 uv0 = focus - uvSize * 0.5f;
    uv0.x = ImClamp(uv0.x, 0.0f, 1.0f - uvSize.x);
    uv0.y = ImClamp(uv0.y, 0.0f, 1.0f - uvSize.y);
    dl->AddImage(tex, a, b, uv0, uv0 + uvSize, tint);
}

void Divider(ImDrawList* dl, ImVec2 a, float width, float s, float alpha) {
    const ImVec2 m(a.x + width * 0.5f, a.y);
    dl->AddRectFilledMultiColor(ImVec2(a.x, a.y - 0.8f * s), ImVec2(m.x, a.y + 0.8f * s), Fade(Col::Gold, 0),
                                Fade(Col::Gold, alpha), Fade(Col::Gold, alpha), Fade(Col::Gold, 0));
    dl->AddRectFilledMultiColor(ImVec2(m.x, a.y - 0.8f * s), ImVec2(a.x + width, a.y + 0.8f * s), Fade(Col::Gold, alpha),
                                Fade(Col::Gold, 0), Fade(Col::Gold, 0), Fade(Col::Gold, alpha));
    Diamond(dl, m, 5.0f * s, Fade(Col::GoldLight, alpha), Fade(Col::GoldDark, alpha));
    Diamond(dl, m - ImVec2(14 * s, 0), 2.6f * s, Fade(Col::Gold, alpha), 0);
    Diamond(dl, m + ImVec2(14 * s, 0), 2.6f * s, Fade(Col::Gold, alpha), 0);
}

void Navi(ImDrawList* dl, ImVec2 c, float s, float time, float alpha) {
    if (alpha <= 0.0f) {
        return;
    }
    const float pulse = 0.8f + 0.2f * sinf(time * 4.3f);
    Glow(dl, c, 70.0f * s * pulse, Fade(Col::Magic, alpha * 0.30f));
    Glow(dl, c, 34.0f * s, Fade(Col::MagicLight, alpha * 0.55f));

    // Wings: two pairs, flapping fast.
    ImTextureID wing = GetTexture(TEX_NAVI_WING);
    const float flap = 0.35f + 0.65f * fabsf(sinf(time * 22.0f));
    const struct {
        float ang;
        float len;
    } wings[4] = { { -0.55f, 34.0f }, { -2.59f, 34.0f }, { 0.45f, 24.0f }, { 2.69f, 24.0f } };
    for (const auto& wdef : wings) {
        const float ang = wdef.ang;
        const ImVec2 dir(cosf(ang), sinf(ang));
        const ImVec2 nrm(-dir.y, dir.x);
        const float len = wdef.len * s;
        const float wid = len * 0.34f * flap;
        const ImVec2 root = c + dir * 4.0f * s;
        const ImVec2 tipP = root + dir * len;
        if (wing != (ImTextureID)0) {
            dl->AddImageQuad(wing, root - nrm * wid, tipP - nrm * wid, tipP + nrm * wid, root + nrm * wid, ImVec2(0, 0),
                             ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), Fade(Col::Rgba(220, 245, 255), alpha * 0.9f));
        } else {
            dl->AddEllipseFilled(root + dir * len * 0.5f, ImVec2(len * 0.5f, wid * 0.6f),
                                 Fade(Col::Rgba(220, 245, 255), alpha * 0.4f), ang, 16);
        }
    }

    // Body.
    dl->AddCircleFilled(c, 8.5f * s, Fade(Col::MagicLight, alpha), 20);
    dl->AddCircleFilled(c, 5.0f * s, Fade(IM_COL32_WHITE, alpha), 16);
    Spark(dl, c, 22.0f * s * pulse, Fade(IM_COL32_WHITE, alpha * 0.7f), time * 0.6f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Particles

float Particles::Rand() {
    mSeed = mSeed * 1664525u + 1013904223u;
    return (float)((mSeed >> 8) & 0xFFFFFF) / (float)0x1000000;
}

Particles::P* Particles::Free() {
    for (P& p : mPool) {
        if (p.life <= 0.0f) {
            return &p;
        }
    }
    return nullptr;
}

void Particles::UpdateDust(float dt, ImVec2 a, ImVec2 b, float s) {
    mDustTimer -= dt;
    while (mDustTimer <= 0.0f) {
        mDustTimer += 0.11f;
        P* p = Free();
        if (p == nullptr) {
            break;
        }
        p->pos = ImVec2(ImLerp(a.x, b.x, Rand()), ImLerp(a.y, b.y, 0.35f + 0.65f * Rand()));
        p->vel = ImVec2((Rand() - 0.5f) * 10.0f * s, -(12.0f + Rand() * 22.0f) * s);
        p->maxLife = p->life = 3.0f + Rand() * 3.5f;
        p->size = (5.0f + Rand() * 9.0f) * s;
        p->phase = Rand() * 6.28f;
        p->spark = Rand() < 0.18f;
        p->col = Rand() < 0.7f ? Col::GoldLight : Col::MagicLight;
    }
}

void Particles::EmitTrail(ImVec2 pos, float s) {
    P* p = Free();
    if (p == nullptr) {
        return;
    }
    p->pos = pos + ImVec2((Rand() - 0.5f) * 10.0f * s, (Rand() - 0.5f) * 10.0f * s);
    p->vel = ImVec2((Rand() - 0.5f) * 16.0f * s, (8.0f + Rand() * 16.0f) * s);
    p->maxLife = p->life = 0.45f + Rand() * 0.5f;
    p->size = (6.0f + Rand() * 8.0f) * s;
    p->phase = Rand() * 6.28f;
    p->spark = true;
    p->col = Col::MagicLight;
}

void Particles::EmitBurst(ImVec2 a, ImVec2 b, float s, int count) {
    for (int i = 0; i < count; i++) {
        P* p = Free();
        if (p == nullptr) {
            return;
        }
        p->pos = ImVec2(ImLerp(a.x, b.x, Rand()), ImLerp(a.y, b.y, Rand()));
        const ImVec2 c = (a + b) * 0.5f;
        ImVec2 d = p->pos - c;
        const float len = ImMax(1.0f, sqrtf(d.x * d.x + d.y * d.y));
        const float speed = (60.0f + Rand() * 140.0f) * s;
        p->vel = ImVec2(d.x / len * speed, d.y / len * speed * 0.6f - 30.0f * s);
        p->maxLife = p->life = 0.5f + Rand() * 0.6f;
        p->size = (7.0f + Rand() * 10.0f) * s;
        p->phase = Rand() * 6.28f;
        p->spark = Rand() < 0.75f;
        p->col = Rand() < 0.6f ? Col::GoldPale : Col::MagicLight;
    }
}

void Particles::Update(float dt) {
    for (P& p : mPool) {
        if (p.life <= 0.0f) {
            continue;
        }
        p.life -= dt;
        p.pos += p.vel * dt;
        p.pos.x += sinf(p.phase + p.life * 1.7f) * 6.0f * dt;
    }
}

void Particles::Draw(ImDrawList* dl, float alpha) const {
    for (const P& p : mPool) {
        if (p.life <= 0.0f) {
            continue;
        }
        const float t = p.life / p.maxLife;           // 1 -> 0
        const float fade = ImMin(1.0f, (1.0f - t) * 4.0f) * ImMin(1.0f, t * 2.5f);
        const float tw = 0.65f + 0.35f * sinf(p.phase + (1.0f - t) * 14.0f);
        if (p.spark) {
            Spark(dl, p.pos, p.size, Fade(p.col, alpha * fade * tw), p.phase);
        } else {
            Glow(dl, p.pos, p.size, Fade(p.col, alpha * fade * tw * 0.8f));
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Animation

float EaseOut(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

float EaseOutBack(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    const float c1 = 1.70158f;
    const float u = t - 1.0f;
    return 1.0f + (c1 + 1.0f) * u * u * u + c1 * u * u;
}

float Appear(float time, float start, float duration) {
    return EaseOut((time - start) / duration);
}

void LightSweep(ImDrawList* dl, ImVec2 a, ImVec2 b, float t, float width, ImU32 col) {
    if (((col >> IM_COL32_A_SHIFT) & 0xFF) == 0 || t <= 0.0f || t >= 1.0f) {
        return;
    }
    const float skew = (b.y - a.y) * 0.6f;
    const float cx = ImLerp(a.x - width - skew, b.x + width, t);
    const ImU32 clear = col & ~IM_COL32_A_MASK;
    dl->PushClipRect(a, b, true);
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(12, 6);
    const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
    const float xs[3] = { cx - width, cx, cx + width };
    const ImU32 cols[3] = { clear, col, clear };
    for (int i = 0; i < 3; i++) {
        dl->PrimWriteVtx(ImVec2(xs[i] + skew, a.y), uv, cols[i]);
        dl->PrimWriteVtx(ImVec2(xs[i], b.y), uv, cols[i]);
    }
    for (int i = 0; i < 2; i++) {
        const ImDrawIdx v = (ImDrawIdx)(base + i * 2);
        dl->PrimWriteIdx(v);
        dl->PrimWriteIdx((ImDrawIdx)(v + 2));
        dl->PrimWriteIdx((ImDrawIdx)(v + 1));
        dl->PrimWriteIdx((ImDrawIdx)(v + 1));
        dl->PrimWriteIdx((ImDrawIdx)(v + 2));
        dl->PrimWriteIdx((ImDrawIdx)(v + 3));
    }
    dl->PopClipRect();
}

void SunRays(ImDrawList* dl, ImVec2 center, float length, float time, ImU32 col) {
    const int kRays = 11;
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    const ImU32 clear = col & ~IM_COL32_A_MASK;
    for (int i = 0; i < kRays; i++) {
        const float ang = i * (6.2831853f / kRays) + time * 0.025f + sinf(i * 3.7f) * 0.25f;
        const float spread = 0.05f + 0.035f * (0.5f + 0.5f * sinf(i * 1.9f + time * 0.4f));
        const float breathe = 0.45f + 0.55f * (0.5f + 0.5f * sinf(time * (0.35f + i * 0.04f) + i * 2.1f));
        const float len = length * (0.7f + 0.3f * (0.5f + 0.5f * sinf(i * 5.3f)));
        const ImU32 c = Fade(col, breathe);
        // Two rows (near the sun and at the far end), each left side / middle / right side: bright along the
        // middle near the sun, fading to nothing at both sides and at the far end.
        const ImVec2 dir(cosf(ang), sinf(ang));
        const ImVec2 nrm(-dir.y, dir.x);
        const float r0 = len * 0.04f;
        const float w0 = r0 * spread * 3.0f + 2.0f;
        const float w1 = len * spread;
        const ImVec2 n0 = center + dir * r0;
        const ImVec2 n1 = center + dir * len;
        dl->PrimReserve(12, 6);
        const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(n0 - nrm * w0, uv, clear);
        dl->PrimWriteVtx(n0, uv, c);
        dl->PrimWriteVtx(n0 + nrm * w0, uv, clear);
        dl->PrimWriteVtx(n1 - nrm * w1, uv, clear);
        dl->PrimWriteVtx(n1, uv, clear);
        dl->PrimWriteVtx(n1 + nrm * w1, uv, clear);
        for (int k = 0; k < 2; k++) {
            const ImDrawIdx v = (ImDrawIdx)(base + k);
            dl->PrimWriteIdx(v);
            dl->PrimWriteIdx((ImDrawIdx)(v + 1));
            dl->PrimWriteIdx((ImDrawIdx)(v + 4));
            dl->PrimWriteIdx(v);
            dl->PrimWriteIdx((ImDrawIdx)(v + 4));
            dl->PrimWriteIdx((ImDrawIdx)(v + 3));
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Hyrule at dusk, drawn with shapes (so it is sharp at any screen resolution).

static float Hash(float x) {
    const float v = sinf(x * 127.1f + 311.7f) * 43758.5453f;
    return v - floorf(v);
}

// Smooth noise along x (0..1).
static float Noise1(float x) {
    const float i = floorf(x);
    const float f = x - i;
    const float u = f * f * (3.0f - 2.0f * f);
    return Hash(i) * (1.0f - u) + Hash(i + 1.0f) * u;
}

// A band of ground from the ridge line height(x) down to `bottom`, with a vertical gradient (top -> low).
template <typename F>
static void Ridge(ImDrawList* dl, float x0, float x1, float bottom, F height, ImU32 top, ImU32 low, float step) {
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    const int n = ImMax(2, (int)((x1 - x0) / step) + 1);
    for (int i = 0; i < n; i++) {
        const float xa = x0 + (x1 - x0) * i / n;
        const float xb = x0 + (x1 - x0) * (i + 1) / n;
        const float ya = height(xa);
        const float yb = height(xb);
        dl->PrimReserve(6, 4);
        const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(ImVec2(xa, ya), uv, top);
        dl->PrimWriteVtx(ImVec2(xb, yb), uv, top);
        dl->PrimWriteVtx(ImVec2(xb, bottom), uv, low);
        dl->PrimWriteVtx(ImVec2(xa, bottom), uv, low);
        dl->PrimWriteIdx(base);
        dl->PrimWriteIdx((ImDrawIdx)(base + 1));
        dl->PrimWriteIdx((ImDrawIdx)(base + 2));
        dl->PrimWriteIdx(base);
        dl->PrimWriteIdx((ImDrawIdx)(base + 2));
        dl->PrimWriteIdx((ImDrawIdx)(base + 3));
    }
}

// A soft cloud: a wide ellipse of light (the glow texture) or, without it, a faint band.
static void Cloud(ImDrawList* dl, ImVec2 c, ImVec2 half, ImU32 col) {
    ImTextureID tex = GetTexture(TEX_GLOW);
    if (tex != (ImTextureID)0) {
        dl->AddImage(tex, c - half, c + half, ImVec2(0, 0), ImVec2(1, 1), col);
    } else {
        dl->AddEllipseFilled(c, half * 0.5f, Fade(col, 0.4f), 0.0f, 32);
    }
}

static void Tower(ImDrawList* dl, float cx, float base, float w, float h, float roof, ImU32 wall, ImU32 roofCol,
                  ImU32 light, float s) {
    dl->AddRectFilled(ImVec2(cx - w * 0.5f, base - h), ImVec2(cx + w * 0.5f, base), wall);
    // Crenels.
    const float m = w * 0.18f;
    dl->AddRectFilled(ImVec2(cx - w * 0.62f, base - h - m * 0.2f), ImVec2(cx + w * 0.62f, base - h + m), wall);
    if (roof > 0.0f) {
        dl->AddTriangleFilled(ImVec2(cx - w * 0.66f, base - h), ImVec2(cx + w * 0.66f, base - h),
                              ImVec2(cx, base - h - roof), roofCol);
        dl->AddLine(ImVec2(cx, base - h - roof), ImVec2(cx, base - h - roof - roof * 0.22f), roofCol,
                    ImMax(1.0f, 1.5f * s));
    }
    // A lit window.
    if ((light >> 24) != 0) {
        const float ww = ImMax(2.0f, w * 0.16f);
        dl->AddRectFilled(ImVec2(cx - ww * 0.5f, base - h * 0.62f), ImVec2(cx + ww * 0.5f, base - h * 0.62f + ww * 1.8f),
                          light, ww * 0.4f);
    }
}

void HyruleScene(ImDrawList* dl, ImVec2 a, ImVec2 b, float s, float time, float alpha) {
    if (alpha <= 0.0f) {
        return;
    }
    const float w = b.x - a.x;
    const float h = b.y - a.y;
    const float horizon = a.y + h * 0.60f;
    const auto A = [alpha](ImU32 c) { return Fade(c, alpha); };

    // Sky: night blue at the top, violet, then the warm sunset at the horizon.
    const ImU32 skyTop = A(Col::Rgba(10, 16, 40));
    const ImU32 skyMid = A(Col::Rgba(58, 44, 92));
    const ImU32 skyLow = A(Col::Rgba(214, 120, 84));
    const ImU32 skyHorizon = A(Col::Rgba(252, 196, 120));
    const float yMid = a.y + h * 0.30f;
    const float yLow = a.y + h * 0.50f;
    dl->AddRectFilledMultiColor(a, ImVec2(b.x, yMid), skyTop, skyTop, skyMid, skyMid);
    dl->AddRectFilledMultiColor(ImVec2(a.x, yMid), ImVec2(b.x, yLow), skyMid, skyMid, skyLow, skyLow);
    dl->AddRectFilledMultiColor(ImVec2(a.x, yLow), ImVec2(b.x, horizon + 2.0f), skyLow, skyLow, skyHorizon,
                                skyHorizon);

    // Stars in the upper sky, twinkling.
    for (int i = 0; i < 90; i++) {
        const float x = a.x + Hash(i * 1.37f) * w;
        const float y = a.y + Hash(i * 2.91f + 4.0f) * h * 0.38f;
        const float fade = 1.0f - (y - a.y) / (h * 0.38f);
        const float tw = 0.55f + 0.45f * sinf(time * (0.8f + Hash(i * 0.7f) * 1.6f) + i);
        const float r = (0.8f + Hash(i * 5.3f) * 1.4f) * s;
        dl->AddCircleFilled(ImVec2(x, y), r, A(Fade(Col::Rgba(255, 246, 228), fade * tw * 0.9f)), 6);
    }

    // The sun going down behind the castle hill.
    const ImVec2 sun(a.x + w * 0.82f, horizon - h * 0.10f);
    Glow(dl, sun, h * 0.75f, A(Fade(Col::Rgba(255, 170, 90), 0.55f)));
    Glow(dl, sun, h * 0.30f, A(Fade(Col::Rgba(255, 226, 160), 0.75f)));
    dl->AddCircleFilled(sun, h * 0.045f, A(Col::Rgba(255, 244, 214)), 48);
    SunRays(dl, sun, w * 0.75f, time, A(Fade(Col::Rgba(255, 226, 170), 0.14f)));

    // Long clouds lit from below, drifting slowly.
    for (int i = 0; i < 7; i++) {
        const float drift = fmodf(Hash(i * 3.1f) * w + time * (6.0f + i * 1.5f) * s, w * 1.4f) - w * 0.2f;
        const ImVec2 c(a.x + drift, a.y + h * (0.20f + 0.05f * i + 0.02f * Hash(i * 9.1f)));
        const ImVec2 half(w * (0.16f + 0.08f * Hash(i * 4.3f)), h * (0.030f + 0.012f * Hash(i * 6.7f)));
        const ImU32 col = Mix(Col::Rgba(150, 96, 150), Col::Rgba(255, 170, 120), (float)i / 6.0f);
        Cloud(dl, c, half, A(Fade(col, 0.85f)));
        Cloud(dl, c + ImVec2(-half.x * 0.35f, half.y * 0.2f), half * 0.7f, A(Fade(col, 0.6f)));
        Cloud(dl, c + ImVec2(half.x * 0.3f, half.y * 0.35f), half * 0.6f, A(Fade(Col::Rgba(255, 214, 170), 0.55f)));
    }

    // Far mountains; Death Mountain on the left with its ring of smoke.
    const float dmX = a.x + w * 0.40f;
    const float dmTop = horizon - h * 0.30f;
    Ridge(
        dl, a.x, b.x, b.y,
        [&](float x) {
            const float u = (x - a.x) / w;
            float y = horizon - h * (0.04f + 0.09f * Noise1(u * 7.0f) + 0.035f * Noise1(u * 19.0f + 7.0f) +
                                     0.012f * Noise1(u * 61.0f + 3.0f));
            // Death Mountain.
            const float d = fabsf(x - dmX) / (w * 0.17f);
            if (d < 1.0f) {
                y = ImMin(y, dmTop + (horizon - dmTop) * d * (0.85f + 0.15f * Noise1(u * 30.0f)));
            }
            return y;
        },
        A(Col::Rgba(96, 72, 118)), A(Col::Rgba(150, 96, 112)), 6.0f * s);
    {
        const ImVec2 ring(dmX, dmTop - h * 0.035f);
        Cloud(dl, ring, ImVec2(w * 0.07f, h * 0.035f), A(Fade(Col::Rgba(255, 190, 150), 0.45f)));
    }
    // Haze where the mountains meet the field.
    dl->AddRectFilledMultiColor(ImVec2(a.x, horizon - h * 0.06f), ImVec2(b.x, horizon + h * 0.04f),
                                A(Fade(Col::Rgba(255, 200, 150), 0.0f)), A(Fade(Col::Rgba(255, 200, 150), 0.0f)),
                                A(Fade(Col::Rgba(255, 190, 140), 0.45f)), A(Fade(Col::Rgba(255, 190, 140), 0.45f)));

    // The castle on its hill, dark against the sunset, with a few lit windows.
    const float hillX = a.x + w * 0.66f;
    const float hillTop = horizon - h * 0.02f;
    Ridge(
        dl, a.x, b.x, b.y,
        [&](float x) {
            const float u = (x - a.x) / w;
            const float d = (x - hillX) / (w * 0.11f);
            const float hill = hillTop + h * 0.07f * d * d;
            const float rolling = horizon + h * (0.03f + 0.04f * Noise1(u * 6.0f + 2.0f));
            return ImMin(hill, rolling);
        },
        A(Col::Rgba(40, 46, 58)), A(Col::Rgba(18, 26, 26)), 5.0f * s);
    {
        const ImU32 wall = A(Col::Rgba(46, 46, 62));
        const ImU32 roof = A(Col::Rgba(40, 42, 70));
        const ImU32 light = A(Col::Rgba(255, 210, 130));
        const float u = h / 1080.0f; // castle unit
        const float base = hillTop + 6.0f * u;
        dl->AddRectFilled(ImVec2(hillX - 120.0f * u, base - 70.0f * u), ImVec2(hillX + 120.0f * u, base), wall);
        Tower(dl, hillX - 120.0f * u, base, 34.0f * u, 120.0f * u, 46.0f * u, wall, roof, light, s);
        Tower(dl, hillX + 120.0f * u, base, 34.0f * u, 112.0f * u, 44.0f * u, wall, roof, 0, s);
        Tower(dl, hillX - 55.0f * u, base, 40.0f * u, 150.0f * u, 54.0f * u, wall, roof, light, s);
        Tower(dl, hillX + 50.0f * u, base, 44.0f * u, 170.0f * u, 60.0f * u, wall, roof, light, s);
        Tower(dl, hillX, base, 64.0f * u, 210.0f * u, 92.0f * u, wall, roof, light, s);
        // Rim light from the sun on the right edges.
        const ImU32 rim = A(Fade(Col::Rgba(255, 190, 120), 0.35f));
        dl->AddLine(ImVec2(hillX + 32.0f * u, base - 210.0f * u), ImVec2(hillX + 32.0f * u, base - 70.0f * u), rim,
                    ImMax(1.0f, 2.0f * u));
        dl->AddLine(ImVec2(hillX + 72.0f * u, base - 170.0f * u), ImVec2(hillX + 72.0f * u, base - 70.0f * u), rim,
                    ImMax(1.0f, 2.0f * u));
    }

    // Hyrule Field in front, rolling and darker, with a lone tree.
    Ridge(
        dl, a.x, b.x, b.y,
        [&](float x) {
            const float u = (x - a.x) / w;
            return a.y + h * (0.74f + 0.06f * Noise1(u * 3.0f + 11.0f) + 0.02f * Noise1(u * 9.0f));
        },
        A(Col::Rgba(30, 50, 34)), A(Col::Rgba(10, 16, 14)), 5.0f * s);
    {
        const float tx = a.x + w * 0.88f;
        const float ty = a.y + h * 0.79f;
        const ImU32 dark = A(Col::Rgba(12, 18, 16));
        dl->AddRectFilled(ImVec2(tx - 7.0f * s, ty - 90.0f * s), ImVec2(tx + 7.0f * s, ty + 4.0f * s), dark);
        dl->AddCircleFilled(ImVec2(tx, ty - 120.0f * s), 62.0f * s, dark, 40);
        dl->AddCircleFilled(ImVec2(tx - 52.0f * s, ty - 95.0f * s), 40.0f * s, dark, 32);
        dl->AddCircleFilled(ImVec2(tx + 50.0f * s, ty - 100.0f * s), 44.0f * s, dark, 32);
    }
    Ridge(
        dl, a.x, b.x, b.y,
        [&](float x) {
            const float u = (x - a.x) / w;
            return a.y + h * (0.88f + 0.05f * Noise1(u * 2.2f + 21.0f));
        },
        A(Col::Rgba(16, 24, 20)), A(Col::Rgba(4, 6, 8)), 6.0f * s);

    // Vignette.
    const ImU32 k0 = A(Fade(Col::Ink, 0.0f));
    const ImU32 k1 = A(Fade(Col::Ink, 0.55f));
    dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + h * 0.18f), k1, k1, k0, k0);
    dl->AddRectFilledMultiColor(ImVec2(b.x - w * 0.12f, a.y), b, k0, k1, k1, k0);
}

void TitleLogo(ImDrawList* dl, ImVec2 top, float width, float s, float alpha, float time) {
    Fonts& f = GetFonts();
    const float cx = top.x;
    Triforce(dl, ImVec2(cx, top.y + 34.0f * s), 56.0f * s, alpha, 0.35f + 0.15f * sinf(time * 1.4f));
    Text(dl, f.displayRegular, 22.0f * s, ImVec2(cx, top.y + 72.0f * s), Fade(Col::GoldLight, alpha * 0.9f),
         "THE LEGEND OF ZELDA", 0.5f, 6.0f * s);
    float big = 58.0f * s;
    const float spacing = big * 0.06f;
    const float tw = TextSize(f.display, big, "OCARINA OF TIME", spacing).x;
    if (tw > width) {
        big *= width / tw;
    }
    GoldText(dl, f.display, big, ImVec2(cx, top.y + 100.0f * s), "OCARINA OF TIME", 0.5f, big * 0.06f, alpha);
    const float lineY = top.y + 100.0f * s + big * 1.12f;
    Divider(dl, ImVec2(cx - width * 0.45f, lineY), width * 0.9f, s, alpha);
    Text(dl, f.display, 26.0f * s, ImVec2(cx, lineY + 10.0f * s), Fade(Col::MagicLight, alpha), "DEFINITIVE EDITION",
         0.5f, 8.0f * s);
}

} // namespace DefinitiveUI
