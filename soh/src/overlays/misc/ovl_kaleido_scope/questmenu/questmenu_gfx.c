// OOT D.E.: QuestMenu rendering. Shapes, textures and text of the journal, drawn with the game's own display lists
// (Fast3D on PC and Android), in "local units" with y growing downwards. See questmenu.h.
#include "questmenu.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
#include "textures/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "textures/message_static/message_static.h"
#include "textures/icon_item_field_static/icon_item_field_static.h"
#include "soh_assets.h"
#include "soh/ShipUtils.h"
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------------------------------------------------
// Text

// Accents drawn over the letter before them. The game font has À Â Ç É Ê Ô à á â ç é ê ô; the other Portuguese
// letters are a font letter with an accent drawn as shapes, so they look the same with or without the PT-BR mods
// (whose redrawn letters are smaller and leave a gap after them).
#define QM_ACUTE 0x10
#define QM_TILDE 0x11
#define QM_DIAERESIS 0x12
#define QM_DOTLESS_I 0x13 // an "i" whose dot is covered by the accent: drawn as "i" with the accent a bit higher

// Writes one or two font codes. A letter that needs two codes (letter + accent) is written only when both fit, so the
// output never goes past `max` and never ends with a letter whose accent was cut.
#define QM_EMIT1(c0)        \
    if (n + 1 > max) {      \
        goto full;          \
    }                       \
    out[n++] = (u8)(c0);    \
    continue
#define QM_EMIT2(c0, c1)    \
    if (n + 2 > max) {      \
        goto full;          \
    }                       \
    out[n++] = (u8)(c0);    \
    out[n++] = (u8)(c1);    \
    continue

// Converts UTF-8 to the codes of the game font. Writes at most `max` codes and returns how many were written; the
// result is a counted buffer (the drawing functions take its length). When there is room left, a 0 is written after
// the last code as well, without being counted. Unknown or broken UTF-8 sequences become '?'.
s32 QM_Encode(const char* text, u8* out, s32 max) {
    const u8* s = (const u8*)text;
    s32 n = 0;

    if ((out == NULL) || (max <= 0)) {
        return 0;
    }
    if (s == NULL) {
        out[0] = '\0';
        return 0;
    }

    while (*s != '\0') {
        u32 cp = *s++;

        if ((cp >= 0xC2) && (cp < 0xE0) && ((*s & 0xC0) == 0x80)) {
            cp = ((cp & 0x1F) << 6) | (*s++ & 0x3F);
        } else if (cp >= 0x80) {
            // Longer sequences are not used by these texts; skip their continuation bytes (never past the end).
            while ((*s != '\0') && ((*s & 0xC0) == 0x80)) {
                s++;
            }
            cp = '?';
        }

        switch (cp) {
            case 0x01: QM_EMIT1(0xA5); // C-Up
            case 0x02: QM_EMIT1(0xA6); // C-Down
            case 0x03: QM_EMIT1(0xA7); // C-Left
            case 0x04: QM_EMIT1(0xA8); // C-Right
            case 0x05: QM_EMIT1(0xA0); // B
            case 0x06: QM_EMIT1(0xA2); // L
            case 0x07: QM_EMIT1(0xA3); // R
            case 0xC0: QM_EMIT1(0x80); // À
            case 0xC1: QM_EMIT2('A', QM_ACUTE); // Á
            case 0xC2: QM_EMIT1(0x82); // Â
            case 0xC3: QM_EMIT2('A', QM_TILDE); // Ã
            case 0xC7: QM_EMIT1(0x84); // Ç
            case 0xC9: QM_EMIT1(0x86); // É
            case 0xCA: QM_EMIT1(0x87); // Ê
            case 0xCD: QM_EMIT2('I', QM_ACUTE); // Í
            case 0xD4: QM_EMIT1(0x8A); // Ô
            case 0xD5: QM_EMIT2('O', QM_TILDE); // Õ
            case 0xD3: QM_EMIT2('O', QM_ACUTE); // Ó
            case 0xDC: QM_EMIT2('U', QM_DIAERESIS); // Ü
            case 0xDA: QM_EMIT2('U', QM_ACUTE); // Ú
            case 0xE0: QM_EMIT1(0x90); // à
            case 0xE1: QM_EMIT1(0x91); // á
            case 0xE2: QM_EMIT1(0x92); // â
            case 0xE3: QM_EMIT2('a', QM_TILDE); // ã
            case 0xE7: QM_EMIT1(0x94); // ç
            case 0xE9: QM_EMIT1(0x96); // é
            case 0xEA: QM_EMIT1(0x97); // ê
            case 0xED: QM_EMIT2(QM_DOTLESS_I, QM_ACUTE); // í
            case 0xF4: QM_EMIT1(0x9A); // ô
            case 0xF5: QM_EMIT2('o', QM_TILDE); // õ
            case 0xF3: QM_EMIT2('o', QM_ACUTE); // ó
            case 0xFC: QM_EMIT2('u', QM_DIAERESIS); // ü
            case 0xFA: QM_EMIT2('u', QM_ACUTE); // ú
        }
        QM_EMIT1(((cp >= ' ') && (cp < 0x7F)) ? (u8)cp : '?');
    }
full:
    if (n < max) {
        out[n] = '\0';
    }
    return n;
}

#undef QM_EMIT1
#undef QM_EMIT2

// ---------------------------------------------------------------------------------------------------------------------
// Drawing
//
// Everything is drawn in "local units", with y growing downwards: half a pixel of the page for the hint on the Quest
// Status page, and half a pixel of a 427 x 240 screen for the journal.

#define QM_GFX_MAX 32000
#define QM_VTX_MAX 16000

static Gfx sQuestMenuGfx[QM_GFX_MAX];
static Vtx sQuestMenuVtx[QM_VTX_MAX];
static Gfx sQuestMenuHintGfx[400];
static Vtx sQuestMenuHintVtx[200];
static Vtx sQuestMenuTitleVtx[4];

bool QM_HasRoom(QuestMenuDraw* d) {
    return (d->gfx + 24 < d->gfxEnd) && (d->vtx + 4 <= d->vtxEnd);
}

void QM_Quad(QuestMenuDraw* d, s16 x, s16 y, s16 w, s16 h, s16 texW, s16 texH) {
    Vtx* v = d->vtx;
    s32 i;

    for (i = 0; i < 4; i++) {
        v[i].v.ob[0] = (i & 1) ? (x + w) : x;
        v[i].v.ob[1] = (i & 2) ? (y + h) : y;
        v[i].v.ob[2] = 0;
        v[i].v.flag = 0;
        v[i].v.tc[0] = (i & 1) ? (texW << 5) : 0;
        v[i].v.tc[1] = (i & 2) ? (texH << 5) : 0;
        v[i].v.cn[0] = v[i].v.cn[1] = v[i].v.cn[2] = 255;
        v[i].v.cn[3] = 255;
    }
    gSPVertex(d->gfx++, v, 4, 0);
    gSP1Quadrangle(d->gfx++, 0, 2, 3, 1, 0);
    d->vtx += 4;
}

void QM_SetColor(QuestMenuDraw* d, u8 r, u8 g, u8 b, u8 a) {
    gDPSetPrimColor(d->gfx++, 0, 0, r, g, b, (a * d->alpha) / 255);
}

void QM_Combine(QuestMenuDraw* d, u8 combine) {
    if (d->combine != combine) {
        gDPPipeSync(d->gfx++);
        if (combine == 1) {
            gDPSetCombineMode(d->gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        } else if (combine == 3) {
            gDPSetCombineMode(d->gfx++, G_CC_SHADE, G_CC_SHADE);
        } else {
            gDPSetCombineMode(d->gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        }
        d->combine = combine;
    }
}

void QM_Rect(QuestMenuDraw* d, s16 x, s16 y, s16 w, s16 h, u8 r, u8 g, u8 b, u8 a) {
    if (!QM_HasRoom(d)) {
        return;
    }
    QM_Combine(d, 2);
    QM_SetColor(d, r, g, b, a);
    QM_Quad(d, x, y, w, h, 0, 0);
}

// Draws a texture on a quad. Gray (and dimmer) when not "lit".
void QM_Tex(QuestMenuDraw* d, const char* tex, u8 fmt, u8 siz, s16 texW, s16 texH, s16 x, s16 y, s16 w,
                         s16 h, bool lit, u8 alpha) {
    if ((tex == NULL) || !QM_HasRoom(d)) {
        return;
    }
    QM_Combine(d, 1);
    if (!lit) {
        gDPSetGrayscaleColor(d->gfx++, 109, 109, 109, 255);
        gSPGrayscale(d->gfx++, true);
        QM_SetColor(d, 255, 255, 255, alpha * 150 / 255);
    } else {
        QM_SetColor(d, 255, 255, 255, alpha);
    }
    if (siz == G_IM_SIZ_32b) {
        gDPLoadTextureBlock(d->gfx++, tex, fmt, G_IM_SIZ_32b, texW, texH, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(d->gfx++, tex, fmt, G_IM_SIZ_16b, texW, texH, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    }
    QM_Quad(d, x, y, w, h, texW, texH);
    if (!lit) {
        gSPGrayscale(d->gfx++, false);
    }
}

// Item icons are 32x32 RGBA32, quest icons 24x24; the Sun and Moon of the time display are 16x16.
void QM_Icon(QuestMenuDraw* d, const char* tex, bool is24, s16 x, s16 y, s16 size, bool lit) {
    s16 texSize = is24 ? 24 : 32;

    if ((tex == gSunIconTex) || (tex == gMoonIconTex)) {
        texSize = 16;
    }
    QM_Tex(d, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, texSize, texSize, x, y, size, size, lit, 255);
}

// The letters of the game font are drawn a little closer than in the text boxes, like the reference.
#define QM_TRACKING 0.9f

f32 QM_CharAdvance(u8 c, s16 size) {
    if (c == QM_DOTLESS_I) {
        c = 'i';
    }
    return Ship_GetCharFontWidth(c) * size * QM_TRACKING / 16.0f;
}

// An accent over the letter that ends at `x`, `adv` wide.
static void QM_DrawAccent(QuestMenuDraw* d, u8 accent, u8 letter, f32 x, f32 adv, s16 y, s16 size, u8 r, u8 g, u8 b,
                          u8 a) {
    bool upper = (letter >= 'A') && (letter <= 'Z');
    f32 cx = x - adv * 0.5f - size * 0.03f;
    // Top of the letter in the 16 x 16 glyph: capitals from row 2, small letters from row 6 (row 3 for the "i").
    f32 top = y + size * (upper ? 0.13f : ((letter == QM_DOTLESS_I) ? 0.20f : 0.36f));
    f32 u = size / 16.0f;
    f32 w = MAX(1.2f, 1.6f * u);
    u8 alpha = a; // QM_Line applies the fade of the journal

    if (!QM_HasRoom(d)) {
        return;
    }
    switch (accent) {
        case QM_ACUTE:
            QM_Line(d, cx - 1.0f * u, top - 1.0f * u, cx + 1.6f * u, top - 3.6f * u, w, r, g, b, alpha);
            break;
        case QM_TILDE:
            QM_Line(d, cx - 2.8f * u, top - 1.2f * u, cx - 1.2f * u, top - 2.8f * u, w, r, g, b, alpha);
            QM_Line(d, cx - 1.2f * u, top - 2.8f * u, cx + 1.2f * u, top - 1.4f * u, w, r, g, b, alpha);
            QM_Line(d, cx + 1.2f * u, top - 1.4f * u, cx + 2.8f * u, top - 3.0f * u, w, r, g, b, alpha);
            break;
        case QM_DIAERESIS:
            QM_Line(d, cx - 2.2f * u, top - 2.4f * u, cx - 2.2f * u, top - 0.8f * u, w * 1.2f, r, g, b, alpha);
            QM_Line(d, cx + 2.2f * u, top - 2.4f * u, cx + 2.2f * u, top - 0.8f * u, w * 1.2f, r, g, b, alpha);
            break;
    }
}

f32 QM_TextWidth(const u8* text, s32 len, s16 size) {
    f32 w = 0.0f;
    s32 i;

    for (i = 0; i < len; i++) {
        w += QM_CharAdvance(text[i], size);
    }
    return w;
}

// Draws encoded text with glyphs of `size` units; returns the x after the text. Button glyphs keep their colors.
f32 QM_DrawCodes(QuestMenuDraw* d, f32 x, s16 y, const u8* text, s32 len, s16 size, u8 r, u8 g, u8 b,
                              u8 a, f32 maxX) {
    s32 i;
    s32 colorSet = -1;

    for (i = 0; i < len; i++) {
        u8 c = text[i];
        f32 adv = QM_CharAdvance(c, size);

        if ((c >= QM_ACUTE) && (c <= QM_DIAERESIS)) {
            if (i > 0) {
                QM_DrawAccent(d, c, text[i - 1], x, QM_CharAdvance(text[i - 1], size), y, size, r, g, b, a);
                colorSet = -1;
            }
            continue;
        }
        // Button glyphs keep their colors, except in shadows.
        s32 color = ((r | g | b) == 0) ? 0 : (((c >= 0xA5) && (c <= 0xA8)) ? 1 : ((c == 0xA0) ? 2 : 0));

        if (x + adv > maxX + 0.5f) {
            break;
        }
        if ((c != ' ') && QM_HasRoom(d)) {
            QM_Combine(d, 1);
            if (color != colorSet) {
                if (color == 1) {
                    QM_SetColor(d, 255, 255, 50, a);
                } else if (color == 2) {
                    QM_SetColor(d, 100, 255, 120, a);
                } else {
                    QM_SetColor(d, r, g, b, a);
                }
                colorSet = color;
            }
            gDPLoadTextureBlock_4b(d->gfx++, Ship_GetCharFontTexture((c == QM_DOTLESS_I) ? 'i' : c), G_IM_FMT_I, 16, 16, 0,
                                   G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                                   G_TX_NOLOD, G_TX_NOLOD);
            QM_Quad(d, (s16)(x + 0.5f), y, size, size, 16, 16);
        }
        x += adv;
    }
    return x;
}

f32 QM_DrawText(QuestMenuDraw* d, f32 x, s16 y, const char* text, s16 size, u8 r, u8 g, u8 b, u8 a,
                             f32 maxX) {
    u8 codes[128];
    s32 len = QM_Encode(text, codes, ARRAY_COUNT(codes));

    return QM_DrawCodes(d, x, y, codes, len, size, r, g, b, a, maxX);
}

// Draws text wrapped at word boundaries inside [x, x + width], at most maxLines lines.
void QM_DrawWrapped(QuestMenuDraw* d, s16 x, s16 y, s16 width, const char* text, s16 size, s16 lineH,
                                 s32 maxLines, u8 r, u8 g, u8 b, u8 a) {
    u8 codes[320];
    s32 len = QM_Encode(text, codes, ARRAY_COUNT(codes));
    s32 start = 0;
    s32 line;

    for (line = 0; (line < maxLines) && (start < len); line++) {
        f32 w = 0.0f;
        s32 end = start;
        s32 lastSpace = -1;

        while (end < len) {
            f32 adv = QM_CharAdvance(codes[end], size);

            if ((w + adv > width) && (end > start)) {
                break;
            }
            if (codes[end] == ' ') {
                lastSpace = end;
            }
            w += adv;
            end++;
        }
        if ((end < len) && (lastSpace > start)) {
            end = lastSpace;
        }
        QM_DrawCodes(d, x, y, &codes[start], end - start, size, r, g, b, a, x + width);
        start = end;
        while ((start < len) && (codes[start] == ' ')) {
            start++;
        }
        y += lineH;
    }
}

// Page background: the save page, with its title tile replaced by a plain band taken from the tile next to it, so the
// quest log can write its own title.
Gfx* KaleidoScope_DrawQuestLogPageSections(Gfx* gfx, Vtx* pageVtx, void** pageTexs) {
    s32 i;

    for (i = 0; i < 15; i++) {
        Vtx* v = &pageVtx[i * 4];
        void* tex = pageTexs[i];

        if (i == 5) {
            s32 j;

            // Top center tile: stretch a narrow strip of the inner edge of the top left tile, which has the same top
            // border and shading but no text.
            for (j = 0; j < 4; j++) {
                sQuestMenuTitleVtx[j] = pageVtx[i * 4 + j];
                sQuestMenuTitleVtx[j].v.tc[0] = (pageVtx[i * 4 + j].v.tc[0] == 0) ? (72 << 5) : (76 << 5);
            }
            v = sQuestMenuTitleVtx;
            tex = pageTexs[0];
        }
        gDPPipeSync(gfx++);
        gSPVertex(gfx++, v, 4, 0);
        gDPLoadTextureBlock(gfx++, tex, G_IM_FMT_IA, G_IM_SIZ_8b, 80, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSP1Quadrangle(gfx++, 0, 2, 3, 1, 0);
    }
    return gfx;
}


// ---------------------------------------------------------------------------------------------------------------------
// Shapes of the journal: polygons (stone, plaques), lines, gradients and circles, all in local units.

void QM_SetVtx(Vtx* v, f32 x, f32 y, u8 r, u8 g, u8 b, u8 a) {
    v->v.ob[0] = (s16)(x + ((x < 0.0f) ? -0.5f : 0.5f));
    v->v.ob[1] = (s16)(y + ((y < 0.0f) ? -0.5f : 0.5f));
    v->v.ob[2] = 0;
    v->v.flag = 0;
    // Tiled textures: one texel every two units, in s10.5.
    v->v.tc[0] = (s16)(x * 16.0f);
    v->v.tc[1] = (s16)(y * 16.0f);
    v->v.cn[0] = r;
    v->v.cn[1] = g;
    v->v.cn[2] = b;
    v->v.cn[3] = a;
}

// Loads a 32x32 RGBA32 texture that repeats over the shapes drawn after it.
void QM_LoadTile(QuestMenuDraw* d, const char* tex) {
    if (!QM_HasRoom(d)) {
        return;
    }
    gDPLoadTextureBlock(d->gfx++, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
}

// A convex polygon, as a fan around its center. Textured with the last tile loaded when `textured`.
void QM_Fan(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, bool textured, u8 r, u8 g, u8 b, u8 a) {
    Vtx* v = d->vtx;
    f32 cx = 0.0f;
    f32 cy = 0.0f;
    s32 i;

    if ((n < 3) || (n > 24) || (d->vtx + n + 1 > d->vtxEnd) || (d->gfx + n + 16 >= d->gfxEnd)) {
        return;
    }
    QM_Combine(d, textured ? 1 : 2);
    QM_SetColor(d, r, g, b, a);
    for (i = 0; i < n; i++) {
        cx += pts[i][0];
        cy += pts[i][1];
    }
    QM_SetVtx(&v[0], cx / n, cy / n, 255, 255, 255, 255);
    for (i = 0; i < n; i++) {
        QM_SetVtx(&v[i + 1], pts[i][0], pts[i][1], 255, 255, 255, 255);
    }
    gSPVertex(d->gfx++, v, n + 1, 0);
    for (i = 0; i < n; i++) {
        gSP1Triangle(d->gfx++, 0, i + 1, ((i + 1) % n) + 1, 0);
    }
    d->vtx += n + 1;
}

void QM_Line(QuestMenuDraw* d, f32 x0, f32 y0, f32 x1, f32 y1, f32 width, u8 r, u8 g, u8 b, u8 a) {
    f32 dx = x1 - x0;
    f32 dy = y1 - y0;
    f32 len = sqrtf(dx * dx + dy * dy);
    f32 nx;
    f32 ny;

    if (len < 0.01f) {
        return;
    }
    nx = -dy / len * width * 0.5f;
    ny = dx / len * width * 0.5f;
    {
        f32 pts[4][2] = { { x0 + nx, y0 + ny }, { x1 + nx, y1 + ny }, { x1 - nx, y1 - ny }, { x0 - nx, y0 - ny } };
        QM_Fan(d, pts, 4, false, r, g, b, a);
    }
}

void QM_Outline(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, f32 width, u8 r, u8 g, u8 b, u8 a) {
    s32 i;

    for (i = 0; i < n; i++) {
        QM_Line(d, pts[i][0], pts[i][1], pts[(i + 1) % n][0], pts[(i + 1) % n][1], width, r, g, b, a);
    }
}

// Rectangle with cut corners, clockwise on screen.
s32 QM_Chamfer(f32 (*pts)[2], f32 x0, f32 y0, f32 x1, f32 y1, f32 c) {
    f32 p[8][2] = { { x0 + c, y0 }, { x1 - c, y0 }, { x1, y0 + c }, { x1, y1 - c },
                    { x1 - c, y1 }, { x0 + c, y1 }, { x0, y1 - c }, { x0, y0 + c } };

    memcpy(pts, p, sizeof(p));
    return 8;
}

// Vertical (top to bottom) or horizontal (left to right) gradient between two colors.
void QM_Gradient(QuestMenuDraw* d, f32 x, f32 y, f32 w, f32 h, const u8* c0, const u8* c1, bool vertical) {
    Vtx* v = d->vtx;
    s32 i;

    if (!QM_HasRoom(d)) {
        return;
    }
    QM_Combine(d, 3);
    for (i = 0; i < 4; i++) {
        bool second = vertical ? ((i & 2) != 0) : ((i & 1) != 0);
        const u8* c = second ? c1 : c0;

        QM_SetVtx(&v[i], (i & 1) ? (x + w) : x, (i & 2) ? (y + h) : y, c[0], c[1], c[2], (c[3] * d->alpha) / 255);
    }
    gSPVertex(d->gfx++, v, 4, 0);
    gSP1Quadrangle(d->gfx++, 0, 2, 3, 1, 0);
    d->vtx += 4;
}

void QM_Circle(QuestMenuDraw* d, f32 cx, f32 cy, f32 radius, u8 r, u8 g, u8 b, u8 a) {
    f32 pts[16][2];
    s32 i;

    for (i = 0; i < 16; i++) {
        pts[i][0] = cx + cosf(i * (2.0f * (f32)M_PI / 16)) * radius;
        pts[i][1] = cy + sinf(i * (2.0f * (f32)M_PI / 16)) * radius;
    }
    QM_Fan(d, pts, 16, false, r, g, b, a);
}

// Carved stone: the stone texture tinted, a light edge on the sides facing the light (top, left), a dark edge on the
// others, and a dark outline.
void QM_Stone(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, u8 r, u8 g, u8 b, f32 bevel) {
    s32 i;

    // An opaque base first, so nothing behind shows through the texture.
    QM_Fan(d, pts, n, false, r * 6 / 10, g * 6 / 10, b * 6 / 10, 255);
    QM_LoadTile(d, gQMStoneTex);
    QM_Fan(d, pts, n, true, r, g, b, 255);
    for (i = 0; i < n; i++) {
        const f32* p = pts[i];
        const f32* q = pts[(i + 1) % n];
        f32 dx = q[0] - p[0];
        f32 dy = q[1] - p[1];
        f32 len = sqrtf(dx * dx + dy * dy);
        f32 nx;
        f32 ny;
        f32 o;

        if (len < 0.01f) {
            continue;
        }
        // Outward normal of a clockwise polygon on screen.
        nx = dy / len;
        ny = -dx / len;
        o = bevel * 0.5f;
        if (nx + ny < -0.1f) {
            QM_Line(d, p[0] - nx * o, p[1] - ny * o, q[0] - nx * o, q[1] - ny * o, bevel, 200, 210, 230, 80);
        } else {
            QM_Line(d, p[0] - nx * o, p[1] - ny * o, q[0] - nx * o, q[1] - ny * o, bevel, 0, 0, 0, 120);
        }
    }
    QM_Outline(d, pts, n, 1.5f, 10, 12, 18, 255);
}

// A recessed area: darker stone, shadow on the top and left edges, a thin light rim.
void QM_Recess(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, u8 r, u8 g, u8 b) {
    s32 i;

    // An opaque base first, so nothing behind shows through the texture.
    QM_Fan(d, pts, n, false, r * 6 / 10, g * 6 / 10, b * 6 / 10, 255);
    QM_LoadTile(d, gQMStoneTex);
    QM_Fan(d, pts, n, true, r, g, b, 255);
    for (i = 0; i < n; i++) {
        const f32* p = pts[i];
        const f32* q = pts[(i + 1) % n];
        f32 dx = q[0] - p[0];
        f32 dy = q[1] - p[1];
        f32 len = sqrtf(dx * dx + dy * dy);
        f32 nx;
        f32 ny;

        if (len < 0.01f) {
            continue;
        }
        nx = dy / len;
        ny = -dx / len;
        if (nx + ny < -0.1f) {
            QM_Line(d, p[0] - nx * 2.5f, p[1] - ny * 2.5f, q[0] - nx * 2.5f, q[1] - ny * 2.5f, 5.0f, 0, 0, 0, 110);
        }
    }
    QM_Outline(d, pts, n, 1.5f, 84, 94, 112, 255);
}

// Golden glow around a shape, pulsing with `pulse` (0..1).
void QM_Glow(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, f32 pulse) {
    QM_Outline(d, pts, n, 10.0f, 255, 190, 50, (u8)(25 + 20 * pulse));
    QM_Outline(d, pts, n, 6.0f, 255, 200, 60, (u8)(55 + 40 * pulse));
    QM_Outline(d, pts, n, 2.5f, 255, 228, 130, 255);
}

// The Triforce, `size` wide with its top at (x, y), in a color.
void QM_TriforceColor(QuestMenuDraw* d, f32 x, f32 y, f32 size, u8 r, u8 g, u8 b, u8 a) {
    f32 half = size / 2;
    f32 h = size * 0.87f;
    f32 hh = h / 2;
    f32 t0[3][2] = { { x + half, y }, { x + half * 1.5f, y + hh }, { x + half * 0.5f, y + hh } };
    f32 t1[3][2] = { { x + half * 0.5f, y + hh }, { x + half, y + h }, { x, y + h } };
    f32 t2[3][2] = { { x + half * 1.5f, y + hh }, { x + size, y + h }, { x + half, y + h } };

    QM_Fan(d, t0, 3, false, r, g, b, a);
    QM_Fan(d, t1, 3, false, r, g, b, a);
    QM_Fan(d, t2, 3, false, r, g, b, a);
}

// Golden Triforce with a dark rim.
void QM_GoldTriforce(QuestMenuDraw* d, f32 x, f32 y, f32 size, bool lit) {
    QM_TriforceColor(d, x - 2.0f, y - 2.0f, size + 4.0f, 40, 24, 4, 255);
    if (lit) {
        QM_TriforceColor(d, x, y, size, 250, 200, 60, 255);
    } else {
        QM_TriforceColor(d, x, y, size, 196, 200, 210, 255);
    }
}

// The winged Triforce of the title, `size` wide (the Triforce itself is a third of it).
void QM_WingedTriforce(QuestMenuDraw* d, f32 cx, f32 y, f32 size) {
    f32 tri = size * 0.36f;
    f32 wing = size * 0.42f;

    QM_Tex(d, gQMWingLTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, (s16)(cx - tri * 0.4f - wing), (s16)(y - wing * 0.3f),
                 (s16)wing, (s16)wing, true, 255);
    QM_Tex(d, gQMWingRTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, (s16)(cx + tri * 0.4f), (s16)(y - wing * 0.3f),
                 (s16)wing, (s16)wing, true, 255);
    QM_GoldTriforce(d, cx - tri / 2, y, tri, true);
}

// Text helpers in local units.
f32 QM_StrWidth(const char* text, s16 size) {
    u8 codes[160];
    s32 len = QM_Encode(text, codes, ARRAY_COUNT(codes));

    return QM_TextWidth(codes, len, size);
}

f32 QM_Print(QuestMenuDraw* d, f32 x, f32 y, const char* text, s16 size, u8 r, u8 g, u8 b, bool shadow,
                          f32 maxX) {
    if (shadow) {
        QM_DrawText(d, x + 1.5f, (s16)(y + 1.5f), text, size, 0, 0, 0, 200, maxX + 1.5f);
    }
    return QM_DrawText(d, x, (s16)y, text, size, r, g, b, 255, maxX);
}

void QM_PrintCentered(QuestMenuDraw* d, f32 cx, f32 y, const char* text, s16 size, u8 r, u8 g, u8 b,
                                   bool shadow) {
    f32 w = QM_StrWidth(text, size);

    QM_Print(d, cx - w / 2, y, text, size, r, g, b, shadow, cx + w / 2 + 2);
}

// The part of the pause world map around a point, `scale` units per texel.
void QM_MapCrop(QuestMenuDraw* d, s16 x, s16 y, s16 texW, s16 texH, s16 u0, s16 v0, s16 scale) {
    if (!QM_HasRoom(d)) {
        return;
    }
    QM_Combine(d, 1);
    QM_SetColor(d, 255, 255, 255, 255);
    gDPLoadTLUT_pal256(d->gfx++, gWorldMapImageTLUT);
    gDPSetTextureLUT(d->gfx++, G_TT_RGBA16);
    gDPLoadMultiTile(d->gfx++, gWorldMapImageTex, 0, G_TX_RENDERTILE, G_IM_FMT_CI, G_IM_SIZ_8b, 216, 128, u0, v0,
                     u0 + texW - 1, v0 + texH - 1, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                     G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetTileSize(d->gfx++, G_TX_RENDERTILE, 0, 0, (texW - 1) << G_TEXTURE_IMAGE_FRAC,
                   (texH - 1) << G_TEXTURE_IMAGE_FRAC);
    QM_Quad(d, x, y, texW * scale, texH * scale, texW, texH);
    gDPPipeSync(d->gfx++);
    gDPSetTextureLUT(d->gfx++, G_TT_NONE);
}


void QM_Begin(PlayState* play, QuestMenuDraw* d, f32 k, u8 alpha) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Mtx* proj;

    d->gfx = sQuestMenuGfx;
    d->gfxEnd = &sQuestMenuGfx[QM_GFX_MAX];
    d->vtx = sQuestMenuVtx;
    d->vtxEnd = &sQuestMenuVtx[QM_VTX_MAX];
    d->alpha = alpha;
    d->combine = 0;

    OPEN_DISPS(gfxCtx);
    // Screen space: a 320 x 240 orthographic projection (the sides of wide screens are outside 0..320), and the
    // canvas centered on the screen.
    proj = Graph_Alloc(gfxCtx, sizeof(Mtx));
    guOrtho(proj, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -10.0f, 10.0f, 1.0f);
    gSPMatrix(POLY_OPA_DISP++, proj, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    Matrix_Push();
    Matrix_Translate(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.0f, MTXMODE_NEW);
    Matrix_Scale(k, k, 1.0f, MTXMODE_APPLY);
    Matrix_Translate(-QM_W / 2.0f, -QM_H / 2.0f, 0.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);

    gDPPipeSync(d->gfx++);
    gDPSetCycleType(d->gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(d->gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(d->gfx++, G_TF_BILERP);
    gDPSetTexturePersp(d->gfx++, G_TP_PERSP);
    gDPSetTextureLUT(d->gfx++, G_TT_NONE);
    gSPClearGeometryMode(d->gfx++, G_CULL_BOTH | G_FOG | G_LIGHTING | G_ZBUFFER | G_TEXTURE_GEN);
    gSPSetGeometryMode(d->gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(d->gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
}

void QM_End(PlayState* play, QuestMenuDraw* d) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    gDPPipeSync(d->gfx++);
    gSPEndDisplayList(d->gfx++);

    OPEN_DISPS(gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sQuestMenuGfx);
    CLOSE_DISPS(gfxCtx);

    // Back to the pause view and its render state.
    KaleidoScope_SetView(&play->pauseCtx, 0.0f, 0.0f, 64.0f);
    Gfx_SetupDL_42Opa(gfxCtx);
}

// Small "C-Up Missões" hint at the bottom right of the normal Quest Status page, drawn with the page matrix.
void KaleidoScope_DrawQuestLogHint(PlayState* play, GraphicsContext* gfxCtx) {
    PauseContext* pauseCtx = &play->pauseCtx;
    QuestMenuDraw draw;
    QuestMenuDraw* d = &draw;
    u8 codes[32];
    s32 len;
    f32 w;

    d->gfx = sQuestMenuHintGfx;
    d->gfxEnd = &sQuestMenuHintGfx[ARRAY_COUNT(sQuestMenuHintGfx)];
    d->vtx = sQuestMenuHintVtx;
    d->vtxEnd = &sQuestMenuHintVtx[ARRAY_COUNT(sQuestMenuHintVtx)];
    d->alpha = pauseCtx->alpha;
    d->combine = 0;

    OPEN_DISPS(gfxCtx);
    // The page matrix is loaded; move the origin to the top left of the page content, in half pixels, y down.
    Matrix_Push();
    Matrix_Translate(-108.0f, 58.0f + pauseCtx->offsetY, 0.0f, MTXMODE_APPLY);
    Matrix_Scale(0.5f, -0.5f, 1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);

    gDPPipeSync(d->gfx++);
    gDPSetTextureFilter(d->gfx++, G_TF_BILERP);
    len = QM_Encode(QM_GLYPH_CUP "Missões", codes, ARRAY_COUNT(codes));
    w = QM_TextWidth(codes, len, 16);
    // Page y -71 to -79 is free below the Spiritual Stones and the song staff.
    QM_DrawCodes(d, 432 + 6 - w, 258, codes, len, 16, 220, 220, 220, 230, 432 + 12);
    gDPPipeSync(d->gfx++);
    gDPSetCombineMode(d->gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetPrimColor(d->gfx++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gSPEndDisplayList(d->gfx++);

    OPEN_DISPS(gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sQuestMenuHintGfx);
    // Back to the page matrix for whatever the page draws next.
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISPS(gfxCtx);
}


// An icon drawn in one tint of gray (the unselected buttons of the sidebar), or in color when `lit`.
void QM_IconTinted(QuestMenuDraw* d, const char* tex, bool is24, s16 x, s16 y, s16 size, bool lit, u8 alpha) {
    s16 texSize = is24 ? 24 : 32;

    if ((tex == NULL) || !QM_HasRoom(d)) {
        return;
    }
    QM_Combine(d, 1);
    if (!lit) {
        gDPSetGrayscaleColor(d->gfx++, 205, 210, 222, 255);
        gSPGrayscale(d->gfx++, true);
    }
    QM_SetColor(d, 255, 255, 255, alpha);
    gDPLoadTextureBlock(d->gfx++, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, texSize, texSize, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    QM_Quad(d, x, y, size, size, texSize, texSize);
    if (!lit) {
        gSPGrayscale(d->gfx++, false);
    }
}
