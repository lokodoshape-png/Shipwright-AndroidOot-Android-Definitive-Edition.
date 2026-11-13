// OOT D.E.: drawing of the main menu of the Definitive Edition interface. See DeTitleMenuView.h.
//
// Everything moves: the scene fades in with a slow zoom, sun rays and fireflies; the logo grows in with a flash and
// a light passing over it now and then; the panel and its buttons slide in one after the other (and again when the
// list changes, from the side the player is going to); the chosen button grows, shines and bursts into sparks; the
// description slides in; two glints run around the frame; Navi follows the cursor.
#include "DeTitleMenuView.h"
#include "DeTitleMenuText.h"

#include <imgui_internal.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace DefinitiveUI {

char NameKeyChar(bool lower, int index) {
    if (index >= 0 && index < 26) {
        return (char)((lower ? 'a' : 'A') + index);
    }
    if (index >= 26 && index < KEY_CHARS) {
        return (char)('0' + (index - 26));
    }
    return index == KEY_SPACE ? ' ' : '\0';
}

void BuildNameKeys(bool lower, KeyCell* out, char labels[KEY_CHARS][2]) {
    for (int i = 0; i < KEY_CHARS; i++) {
        labels[i][0] = NameKeyChar(lower, i);
        labels[i][1] = '\0';
        out[i] = { labels[i], i / 10, i % 10, 1 };
    }
    out[KEY_SPACE] = { Strings::kKeySpace, 3, 6, 4 };
    out[KEY_CASE] = { Strings::kKeyCase, 4, 0, 2 };
    out[KEY_ERASE] = { Strings::kKeyErase, 4, 2, 3 };
    out[KEY_BACK] = { Strings::kKeyBack, 4, 5, 2 };
    out[KEY_DONE] = { Strings::kKeyDone, 4, 7, 3 };
}

void TitleMenuRenderer::Reset() {
    for (int i = 0; i < 16; i++) {
        mHighlight[i] = 0.0f;
        mPop[i] = 0.0f;
    }
    mNaviPos = ImVec2(-1, -1);
    mParticles = Particles();
    mSceneParticles = Particles();
    mPressedItem = -1;
    mAge = 0.0f;
    mScreen = -1;
    mScreenAge = 10.0f;
    mLastSelected = -1;
    mCardAge = 10.0f;
    for (float& g : mKeyGlow) {
        g = 0.0f;
    }
    mKeyFlash = -1;
    mKeyFlashTime = 0.0f;
}

static bool Inside(ImVec2 p, ImVec2 a, ImVec2 b) {
    return p.x >= a.x && p.x < b.x && p.y >= a.y && p.y < b.y;
}

// Point at `t` (0..1) along the edge of the rectangle a..b, clockwise from the top left corner.
static ImVec2 AlongRect(ImVec2 a, ImVec2 b, float t) {
    const float w = b.x - a.x;
    const float h = b.y - a.y;
    float d = (t - floorf(t)) * 2.0f * (w + h);
    if (d < w) {
        return ImVec2(a.x + d, a.y);
    }
    d -= w;
    if (d < h) {
        return ImVec2(b.x, a.y + d);
    }
    d -= h;
    if (d < w) {
        return ImVec2(b.x - d, b.y);
    }
    d -= w;
    return ImVec2(a.x, b.y - d);
}

TitleMenuInput TitleMenuRenderer::Draw(ImDrawList* dl, ImVec2 origin, ImVec2 size, const TitleMenuView& view, float dt,
                                       float time, const PointerState& pointer) {
    TitleMenuInput result;
    const float alpha = ImClamp(view.alpha, 0.0f, 1.0f);
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return result;
    }
    if (view.backdrop > 0.0f) {
        dl->AddRectFilled(origin, origin + size, Fade(Col::Ink, ImClamp(view.backdrop, 0.0f, 1.0f)));
    }
    if (alpha <= 0.0f) {
        return result;
    }
    dt = ImClamp(dt, 0.0f, 0.1f);
    const float s = size.y / 1080.0f;
    Fonts& fonts = GetFonts();
    const ImVec2 end = origin + size;

    // ---- Animation clocks.
    mAge += dt;
    if (view.screen != mScreen) {
        if (mScreen >= 0) {
            mScreenAge = 0.0f;
            mScreenDir = view.screen > mScreen ? 1.0f : -1.0f;
        }
        mScreen = view.screen;
        mLastSelected = -1;
    }
    mScreenAge += dt;
    if (view.selected != mLastSelected) {
        if (mLastSelected >= 0) {
            mCardAge = 0.0f;
        }
        mLastSelected = view.selected;
    }
    mCardAge += dt;

    // Opening: scene, then logo, then panel, then each button, then the description and hints.
    const float sceneIn = Appear(mAge, 0.0f, 1.4f);
    const float logoIn = Appear(mAge, 0.25f, 0.8f);
    const float panelIn = Appear(mAge, 0.45f, 0.6f);
    const float cardIn = Appear(mAge, 1.0f, 0.5f) * Appear(mCardAge, 0.0f, 0.35f);
    const float hintsIn = Appear(mAge, 1.2f, 0.5f);

    // ---- Layout: the menu column on the left (logo, panel, hints), the scene on the right.
    const float colW = ImMin(660.0f * s, size.x * 0.40f);
    const float slide = view.slide * (colW + 80.0f * s);
    const float colX0 = origin.x + 36.0f * s - slide;
    const float colX1 = colX0 + colW;
    const float midX = (colX0 + colX1) * 0.5f;

    // ---- Background: Hyrule at dusk with the castle, Link and Navi. It comes out of the dark while drawing back,
    // then keeps breathing slowly closer and drifting sideways like a camera.
    HyruleScene(dl, origin, end, s, time, alpha * sceneIn);
    // Fireflies over the landscape.
    mSceneParticles.UpdateDust(dt, ImVec2(colX1 + 120.0f * s, origin.y + size.y * 0.35f), end, s * 0.8f);
    mSceneParticles.Update(dt);
    mSceneParticles.Draw(dl, alpha * sceneIn * 0.8f);
    {
        const ImU32 clear = Fade(Col::Ink, 0.0f);
        const float darkA = alpha * (0.78f * panelIn + 0.25f * (1.0f - panelIn));
        const ImU32 dark = Fade(Col::Ink, darkA);
        dl->AddRectFilled(origin, ImVec2(colX1 + 20.0f * s, end.y), dark);
        dl->AddRectFilledMultiColor(ImVec2(colX1 + 20.0f * s, origin.y), ImVec2(colX1 + 360.0f * s, end.y), dark, clear,
                                    clear, dark);
        dl->AddRectFilledMultiColor(ImVec2(origin.x, end.y - 140.0f * s), end, clear, clear,
                                    Fade(Col::Ink, alpha * 0.5f), Fade(Col::Ink, alpha * 0.5f));
    }

    // ---- Logo: grows in with a flash of light, then a light passes over it every few seconds and stars twinkle on
    // the shield and the letters.
    float y = origin.y + 16.0f * s;
    ImTextureID logoTex = GetTexture(TEX_LOGO);
    if (logoTex != (ImTextureID)0) {
        const ImVec2 ts = TextureSize(TEX_LOGO);
        const float lw = colW + 30.0f * s;
        const float lh = lw * ts.y / ts.x;
        const ImVec2 lc(midX, y + lh * 0.5f + 3.0f * s * sinf(time * 1.3f));
        const float grow = 0.82f + 0.18f * EaseOutBack(ImClamp((mAge - 0.25f) / 0.8f, 0.0f, 1.0f));
        const ImVec2 half(lw * 0.5f * grow, lh * 0.5f * grow);
        const float flash = logoIn * (1.0f - Appear(mAge, 0.6f, 1.0f));
        if (flash > 0.0f) {
            Glow(dl, lc, lw * 0.55f, Fade(Col::GoldPale, alpha * flash * 0.55f));
        }
        dl->AddImage(logoTex, lc - half, lc + half, ImVec2(0, 0), ImVec2(1, 1), Fade(IM_COL32_WHITE, alpha * logoIn));
        // Light passing over "DEFINITIVE EDITION" and the title every 7 seconds.
        const float sweep = fmodf(time + 5.0f, 7.0f) / 1.3f;
        LightSweep(dl, lc - half + ImVec2(half.x * 0.6f, half.y * 0.2f), lc + half - ImVec2(0.0f, half.y * 0.08f),
                   sweep, 40.0f * s, Fade(Col::GoldPale, alpha * logoIn * 0.32f));
        static const ImVec2 kStars[] = { { 0.20f, 0.43f }, { 0.43f, 0.18f }, { 0.86f, 0.26f },
                                         { 0.62f, 0.86f }, { 0.09f, 0.22f } };
        for (int i = 0; i < (int)(sizeof(kStars) / sizeof(kStars[0])); i++) {
            const float ph = fmodf(time * 0.55f + i * 0.37f, 1.0f);
            const float tw = ph < 0.22f ? sinf(ph / 0.22f * 3.1415926f) : 0.0f;
            if (tw > 0.0f) {
                const ImVec2 p = lc - half + ImVec2(kStars[i].x * half.x * 2.0f, kStars[i].y * half.y * 2.0f);
                Spark(dl, p, (10.0f + 8.0f * tw) * s, Fade(Col::GoldPale, alpha * logoIn * tw), time * 0.8f);
            }
        }
        y += lh - 4.0f * s;
    } else {
        const float grow = 0.9f + 0.1f * EaseOutBack(ImClamp((mAge - 0.25f) / 0.8f, 0.0f, 1.0f));
        TitleLogo(dl, ImVec2(midX, y), colW * grow, s * grow, alpha * logoIn, time);
        y += 236.0f * s;
    }

    // ---- Panel: slides in from the left; two glints run around its frame.
    const float panelShift = (1.0f - panelIn) * -90.0f * s;
    const float pX0 = colX0 + panelShift;
    const float pX1 = colX1 + panelShift;
    const float panelY0 = y;
    const float panelY1 = end.y - 74.0f * s;
    const float pa = alpha * panelIn;
    ImTextureID frameTex = GetTexture(TEX_FRAME);
    if (frameTex != (ImTextureID)0) {
        NineSlice(dl, frameTex, TextureSize(TEX_FRAME), ImVec2(pX0, panelY0), ImVec2(pX1, panelY1), ImVec2(72, 72),
                  0.8f * s, Fade(IM_COL32_WHITE, pa * 0.97f));
    } else {
        Panel(dl, ImVec2(pX0, panelY0), ImVec2(pX1, panelY1), s, pa);
    }
    for (int i = 0; i < 2; i++) {
        const ImVec2 inset(9.0f * s, 9.0f * s);
        const ImVec2 p = AlongRect(ImVec2(pX0, panelY0) + inset, ImVec2(pX1, panelY1) - inset, time / 12.0f + i * 0.5f);
        Glow(dl, p, 26.0f * s, Fade(Col::GoldLight, pa * 0.35f));
        Spark(dl, p, 13.0f * s, Fade(Col::GoldPale, pa * 0.9f), time * 1.5f);
    }
    mParticles.UpdateDust(dt, ImVec2(pX0, panelY0), ImVec2(pX1, panelY1), s);
    mParticles.Update(dt);
    mParticles.Draw(dl, pa);

    // The list of buttons (and its title) comes in from the side the player is going to.
    auto itemIn = [&](int i) {
        const float open = Appear(mAge, 0.6f + i * 0.07f, 0.45f);
        const float change = Appear(mScreenAge, i * 0.05f, 0.38f);
        return ImMin(open, change);
    };
    const float listFrom = mScreenAge < 1.0f ? mScreenDir : -1.0f;

    y = panelY0 + 34.0f * s;
    if (view.title != nullptr) {
        const float ph = 48.0f * s;
        const float tIn = Appear(mScreenAge, 0.0f, 0.4f) * panelIn;
        const float dx = panelShift + (1.0f - tIn) * listFrom * 60.0f * s;
        Plaque(dl, ImVec2(colX0 + 90.0f * s + dx, y), ImVec2(colX1 - 90.0f * s + dx, y + ph), s, alpha * tIn);
        GoldText(dl, fonts.display, ph * 0.5f, ImVec2(midX + dx, y + ph * 0.22f), view.title, 0.5f, ph * 0.04f,
                 alpha * tIn);
        y += ph + 20.0f * s;
    }

    // ---- Buttons, as tall as the panel allows (easier to touch), or the keyboard of the name entry.
    const float cardH = 120.0f * s;
    const float gap = 11.0f * s;
    const int rows = ImMax(1, ImMin(view.count, 16));
    const float bh = ImClamp((panelY1 - 30.0f * s - cardH - y) / rows - gap, 50.0f * s, 82.0f * s);
    const float bx0 = colX0 + 64.0f * s;
    const float bx1 = colX1 - 64.0f * s;
    ImVec2 selA(bx0, y), selB(bx1, y + bh);
    const bool pointerOn = pointer.valid && alpha > 0.9f;
    if (view.keys != nullptr) {
        // The name, one box per letter, with a blinking mark where the next one goes.
        const float in = Appear(mScreenAge, 0.0f, 0.4f) * panelIn;
        const float dx = panelShift + (1.0f - in) * listFrom * 60.0f * s;
        const float ka = alpha * in;
        const int typed = view.nameText != nullptr ? (int)strlen(view.nameText) : 0;
        const float slotW = 46.0f * s;
        const float slotGap = 8.0f * s;
        const float nameW = view.nameMax * slotW + (view.nameMax - 1) * slotGap;
        float nx = midX - nameW * 0.5f + dx;
        for (int i = 0; i < view.nameMax; i++) {
            const ImVec2 a(nx, y);
            const ImVec2 b(nx + slotW, y + 62.0f * s);
            const bool next = i == ImMin(typed, view.nameMax - 1) && typed < view.nameMax;
            dl->AddRectFilled(a, b, Fade(Col::HylianDeep, ka * 0.85f), 6.0f * s);
            dl->AddRect(a, b, Fade(next ? Col::Magic : Col::Gold, ka * (next ? 0.6f + 0.4f * sinf(time * 6.0f) : 0.7f)),
                        6.0f * s, 0, (next ? 2.5f : 1.5f) * s);
            if (i < typed) {
                const char letter[2] = { view.nameText[i], '\0' };
                Text(dl, fonts.display, 38.0f * s, ImVec2((a.x + b.x) * 0.5f, a.y + 9.0f * s), Fade(Col::GoldPale, ka),
                     letter, 0.5f);
            }
            nx += slotW + slotGap;
        }
        y += 62.0f * s + 10.0f * s;
        if (view.nameHint != nullptr) {
            Text(dl, fonts.body, 25.0f * s, ImVec2(midX + dx, y), Fade(Col::Parchment, ka * 0.9f), view.nameHint, 0.5f);
        }
        y += 40.0f * s;

        // The keys.
        int keyRows = 1;
        for (int i = 0; i < view.keyCount; i++) {
            keyRows = ImMax(keyRows, view.keys[i].row + 1);
        }
        const float kx0 = colX0 + 44.0f * s;
        const float kx1 = colX1 - 44.0f * s;
        const float kGap = 6.0f * s;
        const float kw = (kx1 - kx0 - kGap * 9.0f) / 10.0f;
        const float kh = ImClamp((panelY1 - 34.0f * s - y) / keyRows - kGap, 40.0f * s, 74.0f * s);
        for (int i = 0; i < view.keyCount && i < 64; i++) {
            const KeyCell& key = view.keys[i];
            const float kIn = Appear(mScreenAge, 0.05f + key.row * 0.04f, 0.3f) * panelIn;
            const ImVec2 a(kx0 + key.col * (kw + kGap) + dx, y + key.row * (kh + kGap));
            const ImVec2 b(a.x + key.span * kw + (key.span - 1) * kGap, a.y + kh);
            const bool sel = i == view.keySelected;
            mKeyGlow[i] += ((sel ? 1.0f : 0.0f) - mKeyGlow[i]) * ImMin(1.0f, dt * 14.0f);
            const float g = mKeyGlow[i];
            const float flash = i == mKeyFlash ? mKeyFlashTime : 0.0f;
            if (g > 0.02f) {
                Glow(dl, (a + b) * 0.5f, (b.x - a.x) * 0.8f, Fade(Col::Magic, ka * kIn * g * 0.35f));
            }
            ImVec2 pts[8];
            Chamfer(pts, a, b, 7.0f * s);
            GradientPoly(dl, pts, 8, Fade(Mix(Mix(Col::GraphiteLight, Col::Hylian, g), Col::Gold, flash), ka * kIn * 0.96f),
                         Fade(Mix(Mix(Col::Ink, Col::HylianDeep, g), Col::GoldDark, flash), ka * kIn * 0.96f), a.y, b.y);
            dl->AddPolyline(pts, 8, Fade(Mix(Col::Gold, Col::MagicLight, g), ka * kIn * (0.6f + 0.4f * g)),
                            ImDrawFlags_Closed, (1.4f + g) * s);
            const float fsz = (key.span > 1 ? 0.40f : 0.56f) * kh;
            Text(dl, fonts.display, fsz, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f - fsz * 0.58f),
                 Fade(Mix(Col::Parchment, IM_COL32_WHITE, g), ka * kIn), key.label, 0.5f);
            if (pointerOn && Inside(pointer.pos, a - ImVec2(kGap, kGap) * 0.5f, b + ImVec2(kGap, kGap) * 0.5f)) {
                if (pointer.tap) {
                    result.keyActivated = i;
                    mKeyFlash = i;
                    mKeyFlashTime = 1.0f;
                } else if (pointer.down || pointer.moved) {
                    result.keyHovered = i;
                }
            }
            if (sel) {
                selA = a;
                selB = b;
            }
        }
        mKeyFlashTime = ImMax(0.0f, mKeyFlashTime - dt * 5.0f);
    } else {
        if (view.pressed >= 0 && view.pressed != mPressedItem && view.pressed < 16) {
            mPop[view.pressed] = 1.0f;
        }
        mPressedItem = view.pressed;
        for (int i = 0; i < view.count && i < 16; i++) {
            const float in = itemIn(i);
            const ImVec2 a(bx0, y);
            const ImVec2 b(bx1, y + bh);
            const float target = (i == view.selected) ? 1.0f : 0.0f;
            mHighlight[i] += (target - mHighlight[i]) * ImMin(1.0f, dt * 12.0f);
            const float hl = view.items[i].enabled ? mHighlight[i] : mHighlight[i] * 0.3f;
            const float press = (i == view.pressed) ? 1.0f : 0.0f;

            // Chosen: a bounce and a burst of sparks.
            if (mPop[i] >= 1.0f) {
                mParticles.EmitBurst(a + ImVec2(20.0f * s, 6.0f * s), b - ImVec2(20.0f * s, 6.0f * s), s, 26);
            }
            const float pop = mPop[i] > 0.0f ? sinf((1.0f - mPop[i]) * 3.1415926f) * mPop[i] : 0.0f;
            mPop[i] = ImMax(0.0f, mPop[i] - dt * 3.0f);

            // The selected button is a bit larger and floats; buttons coming in slide and fade.
            const ImVec2 c = (a + b) * 0.5f + ImVec2(panelShift + (1.0f - in) * listFrom * 90.0f * s,
                                                     hl * 1.5f * s * sinf(time * 2.4f));
            const float grow = 1.0f + 0.035f * hl + 0.07f * pop;
            const ImVec2 half((b.x - a.x) * 0.5f * grow, (b.y - a.y) * 0.5f * (1.0f + 0.06f * hl + 0.1f * pop));
            const ImVec2 da = c - half;
            const ImVec2 db = c + half;
            Button(dl, da, db, s, view.items[i].label, mHighlight[i], press,
                   view.items[i].enabled ? BUTTON_NORMAL : BUTTON_DISABLED, alpha * in, time + i * 0.17f);
            if (hl > 0.5f) {
                // A light runs over the selected button every 2.6 seconds.
                const float sweep = fmodf(time, 2.6f) / 0.9f;
                LightSweep(dl, da + ImVec2(half.y * 0.6f, half.y * 0.32f), db - ImVec2(half.y * 0.6f, half.y * 0.32f),
                           sweep, 34.0f * s, Fade(IM_COL32_WHITE, alpha * in * (hl - 0.5f) * 0.5f));
            }
            if (i == view.selected) {
                selA = da;
                selB = db;
            }
            // The whole row counts for a finger, up to the middle of the gaps around it. The button is chosen when
            // the finger (or the click) is let go on it.
            if (pointerOn && in > 0.9f &&
                Inside(pointer.pos, a - ImVec2(30 * s, gap * 0.5f), b + ImVec2(30 * s, gap * 0.5f))) {
                if (pointer.tap) {
                    result.activated = i;
                } else if (pointer.down || pointer.moved) {
                    result.hovered = i;
                }
            }
            y += bh + gap;
        }

        // ---- Card with the description of the selected option: slides up and fades in when the option changes.
        const float cardX0 = colX0 + 52.0f * s + panelShift;
        const float cardX1 = colX1 - 52.0f * s + panelShift;
        float cy = y + 4.0f * s;
        Divider(dl, ImVec2(cardX0, cy), cardX1 - cardX0, s, alpha * panelIn);
        cy += 16.0f * s + (1.0f - cardIn) * 14.0f * s;
        const float ca = alpha * cardIn;
        if (view.showSave && view.saveTitle[0] != '\0') {
            const float tsz = 28.0f * s;
            Text(dl, fonts.display, tsz, ImVec2(cardX0 + 8.0f * s, cy), Fade(Col::GoldLight, ca), view.saveTitle);
            // Hearts on the same line, after the title; they fill one after the other and the last one beats.
            const int hearts = ImClamp(view.heartsCapacity, 0, 20);
            const float hs = 21.0f * s;
            const float hx = cardX0 + 16.0f * s + TextSize(fonts.display, tsz, view.saveTitle).x + hs;
            const int perRow = ImMax(1, (int)((cardX1 - hx) / (hs * 1.1f)));
            const int lastFilled = view.health > 0 ? ImMin(hearts - 1, (view.health - 1) / 16) : -1;
            for (int i = 0; i < hearts; i++) {
                const float hIn = Appear(mCardAge < mAge ? mCardAge : mAge - 1.0f, i * 0.035f, 0.25f);
                const float fill = ImClamp((view.health - i * 16) / 16.0f, 0.0f, 1.0f);
                float beat = 1.0f;
                if (i == lastFilled) {
                    const float ph = fmodf(time, 1.1f);
                    beat = 1.0f + 0.16f * (ph < 0.12f ? sinf(ph / 0.12f * 3.1415926f) : 0.0f) +
                           0.1f * (ph > 0.18f && ph < 0.3f ? sinf((ph - 0.18f) / 0.12f * 3.1415926f) : 0.0f);
                }
                Heart(dl, ImVec2(hx + (i % perRow) * hs * 1.1f, cy + tsz * 0.62f + (i / perRow) * hs * 1.05f),
                      hs * beat * (0.6f + 0.4f * hIn), floorf(fill * 4.0f) / 4.0f, ca * hIn);
            }
            cy += tsz * 1.3f + (hearts > perRow ? hs : 0.0f);
        }
        if (view.selected >= 0 && view.selected < view.count && view.items[view.selected].detail != nullptr) {
            WrappedText(dl, fonts.body, 27.0f * s, ImVec2(cardX0 + 8.0f * s, cy), cardX1 - cardX0 - 16.0f * s,
                        Fade(Col::Parchment, ca * 0.95f), view.items[view.selected].detail, 1.05f);
        }
    }

    // ---- Navi: flies in from the scene and follows the selected button.
    {
        const ImVec2 target(selB.x + 44.0f * s + sinf(time * 1.9f) * 7.0f * s,
                            (selA.y + selB.y) * 0.5f - 6.0f * s + cosf(time * 2.6f) * 9.0f * s);
        if (mNaviPos.x < 0.0f) {
            mNaviPos = ImVec2(origin.x + size.x * 0.95f, origin.y + size.y * 0.28f);
        }
        const ImVec2 before = mNaviPos;
        const float follow = mAge < 0.8f ? 0.0f : 6.0f;
        mNaviPos += (target - mNaviPos) * ImMin(1.0f, dt * follow);
        const ImVec2 d = mNaviPos - before;
        mTrailTimer -= dt;
        if (mTrailTimer <= 0.0f) {
            mTrailTimer = (d.x * d.x + d.y * d.y > 4.0f * s * s) ? 0.015f : 0.12f;
            mParticles.EmitTrail(mNaviPos, s);
        }
        Navi(dl, mNaviPos, s * 0.85f, time, alpha * Appear(mAge, 0.5f, 0.4f));
    }

    // ---- Hints under the panel, version and copyright.
    {
        const float ha = alpha * hintsIn;
        const float gr = 16.0f * s;
        const float pulse = 1.0f + 0.07f * sinf(time * 4.0f);
        const float tsz = 25.0f * s;
        const float hy = panelY1 + 36.0f * s + (1.0f - hintsIn) * 16.0f * s;
        if (view.touchMode) {
            // After a touch: no controller buttons to show.
            Text(dl, fonts.display, tsz, ImVec2(midX, hy - tsz * 0.6f), Fade(Col::Parchment, ha * 0.9f),
                 view.keys != nullptr ? "Toque nas letras" : "Toque para escolher", 0.5f);
        } else {
            const char* confirm = view.keys != nullptr ? "Escolher" : "Confirmar";
            const char* back = view.keys != nullptr ? "Apagar" : "Voltar";
            float w = gr * 2 + 10 * s + TextSize(fonts.display, tsz, confirm).x;
            if (view.showBack) {
                w += 36 * s + gr * 2 + 10 * s + TextSize(fonts.display, tsz, back).x;
            }
            float x = midX - w * 0.5f;
            ButtonGlyph(dl, ImVec2(x + gr, hy), gr * pulse, "A", Col::Rgba(40, 90, 210), ha);
            x += gr * 2 + 10 * s;
            Text(dl, fonts.display, tsz, ImVec2(x, hy - tsz * 0.6f), Fade(Col::Parchment, ha), confirm);
            x += TextSize(fonts.display, tsz, confirm).x + 36 * s;
            if (view.showBack) {
                ButtonGlyph(dl, ImVec2(x + gr, hy), gr, "B", Col::Rgba(40, 150, 70), ha);
                x += gr * 2 + 10 * s;
                Text(dl, fonts.display, tsz, ImVec2(x, hy - tsz * 0.6f), Fade(Col::Parchment, ha), back);
            }
        }
        if (view.footer != nullptr) {
            Text(dl, fonts.displayRegular, 19.0f * s, ImVec2(end.x - 24.0f * s, end.y - 34.0f * s),
                 Fade(Col::Parchment, ha * 0.75f), view.footer, 1.0f, 1.0f * s);
        }
    }

    return result;
}

void TitleMenuRenderer::DrawSplash(ImDrawList* dl, ImVec2 origin, ImVec2 size, float dt, float time, float alpha) {
    alpha = ImClamp(alpha, 0.0f, 1.0f);
    if (alpha <= 0.0f || size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }
    const float s = size.y / 1080.0f;
    const ImVec2 end = origin + size;
    Fonts& fonts = GetFonts();
    // Black, with a faint blue light in the middle.
    dl->AddRectFilled(origin, end, Fade(IM_COL32_BLACK, alpha));
    const ImVec2 c(origin.x + size.x * 0.5f, origin.y + size.y * 0.46f);
    const float in = EaseOut(time / 1.5f);
    const float la = alpha * in;
    Glow(dl, c, size.y * 0.75f, Fade(Col::Hylian, la * 0.45f));
    Glow(dl, c, size.y * 0.42f, Fade(Col::Gold, la * (0.16f + 0.05f * sinf(time * 1.7f))));

    mSplashParticles.UpdateDust(dt, ImVec2(c.x - size.y * 0.7f, c.y - size.y * 0.2f),
                                ImVec2(c.x + size.y * 0.7f, c.y + size.y * 0.4f), s);
    mSplashParticles.Update(dt);
    mSplashParticles.Draw(dl, la * 0.9f);

    // The logo, slowly coming closer.
    ImTextureID logo = GetTexture(TEX_LOGO);
    const float zoom = 0.94f + 0.06f * in + 0.012f * time;
    if (logo != (ImTextureID)0) {
        const ImVec2 ts = TextureSize(TEX_LOGO);
        const float lw = ImMin(size.x * 0.66f, size.y * 0.62f * ts.x / ts.y) * zoom;
        const ImVec2 half(lw * 0.5f, lw * ts.y / ts.x * 0.5f);
        const ImVec2 a = c - half, b = c + half;
        dl->AddImage(logo, a, b, ImVec2(0, 0), ImVec2(1, 1), Fade(IM_COL32_WHITE, la));
        // The picture has its own dark background: its sides melt into the black around it (two passes, for a
        // soft curve) and a golden line closes it above and below, as a banner.
        const ImU32 k1 = Fade(IM_COL32_BLACK, alpha);
        const ImU32 k0 = Fade(IM_COL32_BLACK, 0.0f);
        const float o = 2.0f * s;
        for (int pass = 0; pass < 2; pass++) {
            const float fx = half.x * (pass == 0 ? 0.24f : 0.12f);
            dl->AddRectFilledMultiColor(ImVec2(a.x - o, a.y - o), ImVec2(a.x + fx, b.y + o), k1, k0, k0, k1);
            dl->AddRectFilledMultiColor(ImVec2(b.x - fx, a.y - o), ImVec2(b.x + o, b.y + o), k0, k1, k1, k0);
        }
        const float line = Appear(time, 0.6f, 0.9f);
        Divider(dl, ImVec2(c.x - half.x * 0.92f * line, a.y), half.x * 1.84f * line, s, la * line);
        Divider(dl, ImVec2(c.x - half.x * 0.92f * line, b.y), half.x * 1.84f * line, s, la * line);
        // One light passing over it when it has appeared.
        LightSweep(dl, c - half + ImVec2(half.x * 0.5f, half.y * 0.15f), c + half - ImVec2(0.0f, half.y * 0.06f),
                   (time - 1.4f) / 1.5f, 70.0f * s, Fade(Col::GoldPale, la * 0.4f));
    } else {
        const float k = 1.45f * zoom;
        TitleLogo(dl, ImVec2(c.x, c.y - 170.0f * s * k), size.y * 0.62f * k, s * k, la, time);
    }
}

void TitleMenuRenderer::DrawLoading(ImDrawList* dl, ImVec2 origin, ImVec2 size, float time, float alpha,
                                    const char* text) {
    alpha = ImClamp(alpha, 0.0f, 1.0f);
    if (alpha <= 0.0f || size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }
    const float s = size.y / 1080.0f;
    const ImVec2 end = origin + size;
    Fonts& fonts = GetFonts();
    // Opaque: the game's own file select works underneath and is never seen.
    dl->AddRectFilled(origin, end, Fade(IM_COL32_BLACK, alpha));
    HyruleScene(dl, origin, end, s, time, alpha);
    dl->AddRectFilled(origin, end, Fade(Col::Ink, alpha * 0.55f));
    const ImVec2 c(origin.x + size.x * 0.5f, origin.y + size.y * 0.46f);
    TriforceCrest(dl, c, 120.0f * s, alpha, time);
    if (text != nullptr) {
        // Three dots that come one after the other.
        char line[96];
        const int dots = 1 + ((int)(time * 2.5f) % 3);
        snprintf(line, sizeof(line), "%s%.*s", text, dots, "...");
        GoldText(dl, fonts.display, 40.0f * s, ImVec2(c.x, c.y + 170.0f * s), line, 0.5f, 3.0f * s, alpha);
    }
}

} // namespace DefinitiveUI
