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

    // ---- Layout
    const float margin = 64.0f * s;
    const float panelW = ImMin(600.0f * s, size.x * 0.45f);
    const float slide = view.slide * (panelW + margin * 2.0f);
    const float panelX1 = end.x - margin + slide;
    const float panelX0 = panelX1 - panelW;
    const float panelY0 = origin.y + 118.0f * s;
    const float panelY1 = end.y - 118.0f * s;
    const float crestR = 54.0f * s;
    const float midX = (panelX0 + panelX1) * 0.5f;

    // ---- Backdrop: darken the side of the screen that holds the menu, and the edges.
    {
        const ImU32 clear = Fade(Col::Ink, 0.0f);
        const ImU32 dark = Fade(Col::Ink, alpha * 0.62f);
        const float x0 = panelX0 - 360.0f * s;
        dl->AddRectFilledMultiColor(ImVec2(x0, origin.y), ImVec2(panelX0 - 40.0f * s, end.y), clear, dark, dark, clear);
        dl->AddRectFilled(ImVec2(panelX0 - 40.0f * s, origin.y), end, dark);
        dl->AddRectFilledMultiColor(origin, ImVec2(end.x, origin.y + 120.0f * s), Fade(Col::Ink, alpha * 0.45f),
                                    Fade(Col::Ink, alpha * 0.45f), clear, clear);
        dl->AddRectFilledMultiColor(ImVec2(origin.x, end.y - 150.0f * s), end, clear, clear,
                                    Fade(Col::Ink, alpha * 0.55f), Fade(Col::Ink, alpha * 0.55f));
    }

    // ---- Subtitle under the game's own logo.
    if (view.logoWidth > 0.0f) {
        const float sz = ImClamp(view.logoWidth * 0.085f, 18.0f * s, 52.0f * s);
        const ImVec2 p(view.logoCenter.x, view.logoBottom);
        Glow(dl, p + ImVec2(0, sz * 0.6f), sz * 5.0f, Fade(Col::Ink, alpha * 0.22f));
        GoldText(dl, fonts.display, sz, p, "DEFINITIVE EDITION", 0.5f, sz * 0.12f, alpha);
        Divider(dl, ImVec2(p.x - view.logoWidth * 0.42f, p.y + sz * 1.32f), view.logoWidth * 0.84f, s, alpha * 0.9f);
    }

    // ---- Panel and crest.
    Panel(dl, ImVec2(panelX0, panelY0), ImVec2(panelX1, panelY1), s, alpha);
    mParticles.UpdateDust(dt, ImVec2(panelX0, panelY0), ImVec2(panelX1, panelY1), s);
    mParticles.Update(dt);
    mParticles.Draw(dl, alpha);
    TriforceCrest(dl, ImVec2(midX, panelY0), crestR, alpha, time);

    float y = panelY0 + crestR + 22.0f * s;
    if (view.title != nullptr) {
        const float ph = 50.0f * s;
        Plaque(dl, ImVec2(panelX0 + 70.0f * s, y), ImVec2(panelX1 - 70.0f * s, y + ph), s, alpha);
        GoldText(dl, fonts.display, ph * 0.5f, ImVec2(midX, y + ph * 0.22f), view.title, 0.5f, ph * 0.04f, alpha);
        y += ph + 22.0f * s;
    }

    // ---- Buttons.
    const float bh = 64.0f * s;
    const float gap = 13.0f * s;
    const float bx0 = panelX0 + 54.0f * s;
    const float bx1 = panelX1 - 54.0f * s;
    ImVec2 selA(bx0, y), selB(bx1, y + bh);
    for (int i = 0; i < view.count && i < 16; i++) {
        const ImVec2 a(bx0, y);
        const ImVec2 b(bx1, y + bh);
        const float target = (i == view.selected) ? 1.0f : 0.0f;
        mHighlight[i] += (target - mHighlight[i]) * ImMin(1.0f, dt * 12.0f);
        const float press = (i == view.pressed) ? 0.55f + 0.45f * sinf(time * 18.0f) : 0.0f;
        Button(dl, a, b, s, view.items[i].label, mHighlight[i], press * 0.6f,
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
    const float cardX0 = panelX0 + 40.0f * s;
    const float cardX1 = panelX1 - 40.0f * s;
    float cy = y + 6.0f * s;
    Divider(dl, ImVec2(cardX0, cy), cardX1 - cardX0, s, alpha);
    cy += 18.0f * s;
    if (view.showSave && view.saveTitle[0] != '\0') {
        const float tsz = 30.0f * s;
        Text(dl, fonts.display, tsz, ImVec2(cardX0 + 8.0f * s, cy), Fade(Col::GoldLight, alpha), view.saveTitle);
        cy += tsz * 1.25f;
        const int hearts = ImClamp(view.heartsCapacity, 0, 20);
        const float hs = 24.0f * s;
        for (int i = 0; i < hearts; i++) {
            const int row = i / 10;
            const int col = i % 10;
            const float fill = ImClamp((view.health - i * 16) / 16.0f, 0.0f, 1.0f);
            Heart(dl, ImVec2(cardX0 + 20.0f * s + col * hs * 1.12f, cy + hs * 0.55f + row * hs * 1.05f), hs,
                  floorf(fill * 4.0f) / 4.0f, alpha);
        }
        cy += hs * (hearts > 10 ? 2.25f : 1.25f);
    }
    if (view.selected >= 0 && view.selected < view.count && view.items[view.selected].detail != nullptr) {
        WrappedText(dl, fonts.body, 31.0f * s, ImVec2(cardX0 + 8.0f * s, cy), cardX1 - cardX0 - 16.0f * s,
                    Fade(Col::Parchment, alpha * 0.95f), view.items[view.selected].detail, 1.08f);
    }

    // ---- Navi, beside the selected button.
    {
        const ImVec2 target(selA.x - 46.0f * s + sinf(time * 1.9f) * 7.0f * s,
                            (selA.y + selB.y) * 0.5f - 6.0f * s + cosf(time * 2.6f) * 9.0f * s);
        if (mNaviPos.x < 0.0f) {
            mNaviPos = ImVec2(end.x + 40.0f * s, origin.y + size.y * 0.3f);
        }
        const ImVec2 before = mNaviPos;
        mNaviPos += (target - mNaviPos) * ImMin(1.0f, dt * 7.0f);
        const ImVec2 d = mNaviPos - before;
        mTrailTimer -= dt;
        if (mTrailTimer <= 0.0f) {
            mTrailTimer = (d.x * d.x + d.y * d.y > 4.0f * s * s) ? 0.015f : 0.12f;
            mParticles.EmitTrail(mNaviPos, s);
        }
        Navi(dl, mNaviPos, s, time, alpha);
    }

    // ---- Hints under the panel.
    {
        const float gr = 17.0f * s;
        const float tsz = 27.0f * s;
        const float hy = panelY1 + 44.0f * s;
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
            Text(dl, fonts.displayRegular, 20.0f * s, ImVec2(panelX1 - 10.0f * s, end.y - 34.0f * s),
                 Fade(Col::TextDim, alpha * 0.8f), view.footer, 1.0f, 1.0f * s);
        }
    }

    return result;
}

} // namespace DefinitiveUI
