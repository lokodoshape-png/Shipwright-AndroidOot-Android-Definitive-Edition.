// OOT D.E.: drawing of the main menu of the Definitive Edition interface. See DeTitleMenuView.h.
#include "DeTitleMenuView.h"

#include <imgui_internal.h>
#include <cmath>

namespace DefinitiveUI {

void TitleMenuRenderer::Reset() {
    for (float& h : mHighlight) {
        h = 0.0f;
    }
    mNaviPos = ImVec2(-1, -1);
    mParticles = Particles();
}

static bool Inside(ImVec2 p, ImVec2 a, ImVec2 b) {
    return p.x >= a.x && p.x < b.x && p.y >= a.y && p.y < b.y;
}

TitleMenuInput TitleMenuRenderer::Draw(ImDrawList* dl, ImVec2 origin, ImVec2 size, const TitleMenuView& view, float dt,
                                       float time, const PointerState& pointer) {
    TitleMenuInput result;
    const float alpha = ImClamp(view.alpha, 0.0f, 1.0f);
    if (alpha <= 0.0f || size.x <= 0.0f || size.y <= 0.0f) {
        return result;
    }
    dt = ImClamp(dt, 0.0f, 0.1f);
    const float s = size.y / 1080.0f;
    Fonts& fonts = GetFonts();
    const ImVec2 end = origin + size;

    // ---- Layout: the menu column on the left (logo, panel, hints), the scene on the right.
    const float colW = ImMin(660.0f * s, size.x * 0.40f);
    const float slide = view.slide * (colW + 80.0f * s);
    const float colX0 = origin.x + 36.0f * s - slide;
    const float colX1 = colX0 + colW;
    const float midX = (colX0 + colX1) * 0.5f;

    // ---- Background: Hyrule at dusk with the castle, Link and Navi, slowly drifting closer.
    ImTextureID bgTex = GetTexture(TEX_TITLE_BG);
    if (bgTex != (ImTextureID)0) {
        const float zoom = 1.0f + 0.05f * (0.5f - 0.5f * cosf(time * 6.2831853f / 45.0f));
        CoverImage(dl, bgTex, TextureSize(TEX_TITLE_BG), origin, end, zoom, ImVec2(0.6f, 0.5f), Fade(IM_COL32_WHITE, alpha));
    }
    {
        const ImU32 clear = Fade(Col::Ink, 0.0f);
        const ImU32 dark = Fade(Col::Ink, alpha * 0.78f);
        dl->AddRectFilled(origin, ImVec2(colX1 + 20.0f * s, end.y), dark);
        dl->AddRectFilledMultiColor(ImVec2(colX1 + 20.0f * s, origin.y), ImVec2(colX1 + 360.0f * s, end.y), dark, clear,
                                    clear, dark);
        dl->AddRectFilledMultiColor(ImVec2(origin.x, end.y - 140.0f * s), end, clear, clear,
                                    Fade(Col::Ink, alpha * 0.5f), Fade(Col::Ink, alpha * 0.5f));
    }

    // ---- Logo.
    float y = origin.y + 16.0f * s;
    ImTextureID logoTex = GetTexture(TEX_LOGO);
    if (logoTex != (ImTextureID)0) {
        const ImVec2 ts = TextureSize(TEX_LOGO);
        const float lw = colW + 30.0f * s;
        const float lh = lw * ts.y / ts.x;
        dl->AddImage(logoTex, ImVec2(midX - lw * 0.5f, y), ImVec2(midX + lw * 0.5f, y + lh), ImVec2(0, 0), ImVec2(1, 1),
                     Fade(IM_COL32_WHITE, alpha));
        y += lh - 4.0f * s;
    } else {
        const float sz = 48.0f * s;
        GoldText(dl, fonts.display, sz, ImVec2(midX, y + 40.0f * s), "OCARINA OF TIME", 0.5f, sz * 0.08f, alpha);
        GoldText(dl, fonts.display, sz * 0.7f, ImVec2(midX, y + 100.0f * s), "DEFINITIVE EDITION", 0.5f, sz * 0.1f,
                 alpha);
        y += 170.0f * s;
    }

    // ---- Panel.
    const float panelY0 = y;
    const float panelY1 = end.y - 74.0f * s;
    ImTextureID frameTex = GetTexture(TEX_FRAME);
    if (frameTex != (ImTextureID)0) {
        NineSlice(dl, frameTex, TextureSize(TEX_FRAME), ImVec2(colX0, panelY0), ImVec2(colX1, panelY1), ImVec2(72, 72),
                  0.8f * s, Fade(IM_COL32_WHITE, alpha * 0.97f));
    } else {
        Panel(dl, ImVec2(colX0, panelY0), ImVec2(colX1, panelY1), s, alpha);
    }
    mParticles.UpdateDust(dt, ImVec2(colX0, panelY0), ImVec2(colX1, panelY1), s);
    mParticles.Update(dt);
    mParticles.Draw(dl, alpha);

    y = panelY0 + 34.0f * s;
    if (view.title != nullptr) {
        const float ph = 48.0f * s;
        Plaque(dl, ImVec2(colX0 + 90.0f * s, y), ImVec2(colX1 - 90.0f * s, y + ph), s, alpha);
        GoldText(dl, fonts.display, ph * 0.5f, ImVec2(midX, y + ph * 0.22f), view.title, 0.5f, ph * 0.04f, alpha);
        y += ph + 20.0f * s;
    }

    // ---- Buttons.
    const float bh = 60.0f * s;
    const float gap = 11.0f * s;
    const float bx0 = colX0 + 64.0f * s;
    const float bx1 = colX1 - 64.0f * s;
    ImVec2 selA(bx0, y), selB(bx1, y + bh);
    for (int i = 0; i < view.count && i < 16; i++) {
        const ImVec2 a(bx0, y);
        const ImVec2 b(bx1, y + bh);
        const float target = (i == view.selected) ? 1.0f : 0.0f;
        mHighlight[i] += (target - mHighlight[i]) * ImMin(1.0f, dt * 12.0f);
        const float press = (i == view.pressed) ? 1.0f : 0.0f;
        Button(dl, a, b, s, view.items[i].label, mHighlight[i], press,
               view.items[i].enabled ? BUTTON_NORMAL : BUTTON_DISABLED, alpha, time + i * 0.17f);
        if (i == view.selected) {
            selA = a;
            selB = b;
        }
        if (pointer.valid && alpha > 0.9f && Inside(pointer.pos, a - ImVec2(20 * s, 0), b + ImVec2(20 * s, 0))) {
            if (pointer.clicked) {
                result.activated = i;
            } else if (pointer.moved && !pointer.down) {
                result.hovered = i;
            }
        }
        y += bh + gap;
    }

    // ---- Card with the description of the selected option.
    const float cardX0 = colX0 + 52.0f * s;
    const float cardX1 = colX1 - 52.0f * s;
    float cy = y + 4.0f * s;
    Divider(dl, ImVec2(cardX0, cy), cardX1 - cardX0, s, alpha);
    cy += 16.0f * s;
    if (view.showSave && view.saveTitle[0] != '\0') {
        const float tsz = 28.0f * s;
        Text(dl, fonts.display, tsz, ImVec2(cardX0 + 8.0f * s, cy), Fade(Col::GoldLight, alpha), view.saveTitle);
        // Hearts on the same line, after the title.
        const int hearts = ImClamp(view.heartsCapacity, 0, 20);
        const float hs = 21.0f * s;
        const float hx = cardX0 + 16.0f * s + TextSize(fonts.display, tsz, view.saveTitle).x + hs;
        const int perRow = ImMax(1, (int)((cardX1 - hx) / (hs * 1.1f)));
        for (int i = 0; i < hearts; i++) {
            const float fill = ImClamp((view.health - i * 16) / 16.0f, 0.0f, 1.0f);
            Heart(dl, ImVec2(hx + (i % perRow) * hs * 1.1f, cy + tsz * 0.62f + (i / perRow) * hs * 1.05f), hs,
                  floorf(fill * 4.0f) / 4.0f, alpha);
        }
        cy += tsz * 1.3f + (hearts > perRow ? hs : 0.0f);
    }
    if (view.selected >= 0 && view.selected < view.count && view.items[view.selected].detail != nullptr) {
        WrappedText(dl, fonts.body, 29.0f * s, ImVec2(cardX0 + 8.0f * s, cy), cardX1 - cardX0 - 16.0f * s,
                    Fade(Col::Parchment, alpha * 0.95f), view.items[view.selected].detail, 1.05f);
    }

    // ---- Navi, beside the selected button.
    {
        const ImVec2 target(selB.x + 44.0f * s + sinf(time * 1.9f) * 7.0f * s,
                            (selA.y + selB.y) * 0.5f - 6.0f * s + cosf(time * 2.6f) * 9.0f * s);
        if (mNaviPos.x < 0.0f) {
            mNaviPos = ImVec2(origin.x + size.x * 0.75f, origin.y + size.y * 0.2f);
        }
        const ImVec2 before = mNaviPos;
        mNaviPos += (target - mNaviPos) * ImMin(1.0f, dt * 6.0f);
        const ImVec2 d = mNaviPos - before;
        mTrailTimer -= dt;
        if (mTrailTimer <= 0.0f) {
            mTrailTimer = (d.x * d.x + d.y * d.y > 4.0f * s * s) ? 0.015f : 0.12f;
            mParticles.EmitTrail(mNaviPos, s);
        }
        Navi(dl, mNaviPos, s * 0.85f, time, alpha);
    }

    // ---- Hints under the panel, version and copyright.
    {
        const float gr = 16.0f * s;
        const float tsz = 25.0f * s;
        const float hy = panelY1 + 36.0f * s;
        const char* confirm = "Confirmar";
        const char* back = "Voltar";
        float w = gr * 2 + 10 * s + TextSize(fonts.display, tsz, confirm).x;
        if (view.showBack) {
            w += 36 * s + gr * 2 + 10 * s + TextSize(fonts.display, tsz, back).x;
        }
        float x = midX - w * 0.5f;
        ButtonGlyph(dl, ImVec2(x + gr, hy), gr, "A", Col::Rgba(40, 90, 210), alpha);
        x += gr * 2 + 10 * s;
        Text(dl, fonts.display, tsz, ImVec2(x, hy - tsz * 0.6f), Fade(Col::Parchment, alpha), confirm);
        x += TextSize(fonts.display, tsz, confirm).x + 36 * s;
        if (view.showBack) {
            ButtonGlyph(dl, ImVec2(x + gr, hy), gr, "B", Col::Rgba(40, 150, 70), alpha);
            x += gr * 2 + 10 * s;
            Text(dl, fonts.display, tsz, ImVec2(x, hy - tsz * 0.6f), Fade(Col::Parchment, alpha), back);
        }
        if (view.footer != nullptr) {
            Text(dl, fonts.displayRegular, 19.0f * s, ImVec2(end.x - 24.0f * s, end.y - 34.0f * s),
                 Fade(Col::Parchment, alpha * 0.75f), view.footer, 1.0f, 1.0f * s);
        }
    }

    return result;
}

} // namespace DefinitiveUI
