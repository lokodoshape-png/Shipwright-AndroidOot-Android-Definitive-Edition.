// OOT D.E.: QuestMenu interface. The layout of the "Diário de Quests": a carved stone frame with the title plaque and
// the L/R tabs, the six categories on the left, the lines of the current list in the middle (with their state:
// locked, available, in progress or done), and a parchment card on the right with the selected line: description,
// state, progress, objectives and the place on the pause world map. Every part that can be touched or clicked is
// recorded as a touch area for questmenu_state.c. See questmenu.h.
#include "questmenu.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
#include "textures/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "textures/icon_item_field_static/icon_item_field_static.h"
#include "soh_assets.h"
#include "soh/OTRGlobals.h"
#include <stdio.h>
#include <math.h>

#define QM_SIDE_X0 80
#define QM_SIDE_X1 213
#define QM_SIDE_Y0 131
#define QM_SIDE_STEP 49
#define QM_SIDE_H 42

#define QM_LIST_X0 224
#define QM_LIST_X1 496
#define QM_LIST_Y0 131
#define QM_LIST_Y1 425

#define QM_CARD_X0 507
#define QM_CARD_Y0 131
#define QM_CARD_X1 779
#define QM_CARD_Y1 432

#define QM_MAP_X 520
#define QM_MAP_Y 368
#define QM_MAP_TEX_W 82
#define QM_MAP_TEX_H 17
#define QM_MAP_SCALE 3

// One line of the list on screen: a section title or a quest.
typedef struct {
    s16 row; // -1 for a section title
    const char* title;
    u8 style;
    s16 got; // done lines of the section
    s16 total;
} QuestMenuLine;

static QuestMenuLine sLines[QM_MAX_ROWS * 2];

static s32 QuestMenu_BuildLines(s32 cursor, s32* cursorLine) {
    s32 n = 0;
    s32 header = -1;
    s32 i;
    const char* section = NULL;

    *cursorLine = 0;
    for (i = 0; i < gQuestMenu.rowCount; i++) {
        const QuestMenuRow* row = &gQuestMenu.rows[i];

        if ((row->section != NULL) && (row->section != section)) {
            section = row->section;
            header = n;
            sLines[n].row = -1;
            sLines[n].title = section;
            sLines[n].style = row->sectionStyle;
            sLines[n].got = sLines[n].total = 0;
            n++;
        }
        if (header >= 0) {
            sLines[header].total++;
            sLines[header].got += row->done ? 1 : 0;
        }
        if (i == cursor) {
            *cursorLine = n;
        }
        sLines[n].row = i;
        sLines[n].title = NULL;
        sLines[n].style = 0;
        n++;
    }
    return n;
}

static s32 QuestMenu_LineH(s32 line) {
    return (sLines[line].row < 0) ? QM_HEADER_H : QM_ROW_H;
}

static s32 QuestMenu_Height(s32 from, s32 to) {
    s32 h = 0;

    for (; from <= to; from++) {
        h += QuestMenu_LineH(from);
    }
    return h;
}

static void QuestMenu_DrawSidebar(QuestMenuDraw* d, f32 pulse) {
    f32 pts[8][2];
    s32 i;

    for (i = 0; i < QM_CAT_COUNT; i++) {
        bool cur = i == gQuestMenu.category;
        f32 y0 = QM_SIDE_Y0 + i * QM_SIDE_STEP;
        s32 n = QM_Chamfer(pts, QM_SIDE_X0, y0, QM_SIDE_X1, y0 + QM_SIDE_H, 5);

        QM_Stone(d, pts, n, cur ? 86 : 64, cur ? 84 : 72, cur ? 82 : 88, 2.5f);
        if (cur) {
            QM_Glow(d, pts, n, pulse);
        }
        if (gQuestMenuCategoryIcons[i] == NULL) {
            QM_GoldTriforce(d, 89, y0 + 9, 25, cur);
        } else {
            // Silver when not selected, like the buttons of the reference.
            QM_IconTinted(d, gQuestMenuCategoryIcons[i], false, 87, (s16)y0 + 7, 28, cur, cur ? 255 : 235);
        }
        {
            // Smaller letters for long names ("Personagens", "Secundárias").
            s16 size = 16;

            while ((size > 11) && (QM_StrWidth(gQuestMenuCategoryNames[i], size) > QM_SIDE_X1 - 6 - 122)) {
                size--;
            }
            QM_Print(d, 122, y0 + 13 + (16 - size) / 2, gQuestMenuCategoryNames[i], size, cur ? 255 : 226,
                     cur ? 226 : 228, cur ? 130 : 232, true, QM_SIDE_X1 - 3);
        }
        QuestMenu_AddHit(QM_SIDE_X0, y0, QM_SIDE_X1, y0 + QM_SIDE_H + 4, QM_HIT_CATEGORY, i);
    }
}

// The box at the left of a line: a check when done, an hourglass when started, a padlock when locked.
static void QuestMenu_DrawStateBox(QuestMenuDraw* d, const QuestMenuRow* row, f32 y) {
    QM_Rect(d, 238, (s16)y + 6, 12, 12, 190, 192, 200, 255);
    QM_Rect(d, 240, (s16)y + 8, 8, 8, 16, 18, 24, 255);
    switch (row->state) {
        case QM_STATE_DONE:
            QM_Tex(d, gQMCheckTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 236, (s16)y - 1, 20, 20, true, 255);
            break;
        case QM_STATE_IN_PROGRESS:
            QM_Tex(d, gQMProgressTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 237, (s16)y + 5, 14, 14, true, 255);
            break;
        case QM_STATE_LOCKED:
            QM_Tex(d, gQMLockTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 237, (s16)y + 5, 14, 14, true, 220);
            break;
    }
}

static void QuestMenu_DrawList(QuestMenuDraw* d, s32 cursor, f32 pulse) {
    f32 pts[8][2];
    s32 n;
    s32 lineCount;
    s32 cursorLine;
    s32* top = &gQuestMenu.top[gQuestMenu.category];
    s32 avail = QM_LIST_Y1 - QM_LIST_Y0 - 8;
    s32 maxTop;
    s32 line;
    f32 y;

    n = QM_Chamfer(pts, QM_LIST_X0, QM_LIST_Y0, QM_LIST_X1, QM_LIST_Y1, 6);
    QM_Recess(d, pts, n, 40, 44, 56);
    // The whole list, for dragging; the lines are recorded on top of it.
    QuestMenu_AddHit(QM_LIST_X0, QM_LIST_Y0, QM_LIST_X1, QM_LIST_Y1, QM_HIT_LIST, 0);

    lineCount = QuestMenu_BuildLines(cursor, &cursorLine);
    if (lineCount == 0) {
        return;
    }

    // The first line from which the end of the list fits, so there is no empty space at the end.
    maxTop = lineCount - 1;
    while ((maxTop > 0) && (QuestMenu_Height(maxTop - 1, lineCount - 1) <= avail)) {
        maxTop--;
    }
    if (!gQuestMenu.freeScroll) {
        // Scroll so that the cursor (and the title of its section, when it is the first line) is visible.
        *top = CLAMP(*top, 0, lineCount - 1);
        if (cursorLine < *top) {
            *top = cursorLine;
        }
        if ((*top == cursorLine) && (cursorLine > 0) && (sLines[cursorLine - 1].row < 0)) {
            *top = cursorLine - 1;
        }
        while ((*top < cursorLine) && (QuestMenu_Height(*top, cursorLine) > avail)) {
            (*top)++;
        }
    }
    *top = CLAMP(*top, 0, maxTop);

    y = QM_LIST_Y0 + 4;
    for (line = *top; line < lineCount; line++) {
        const QuestMenuLine* l = &sLines[line];
        s32 h = QuestMenu_LineH(line);

        if (y + h > QM_LIST_Y1 - 3) {
            break;
        }
        if (l->row < 0) {
            static const u8 bandTop[4] = { 58, 66, 82, 255 };
            static const u8 bandBottom[4] = { 40, 46, 58, 255 };
            char count[16];
            f32 countX;

            snprintf(count, sizeof(count), "%d/%d", l->got, l->total);
            countX = QM_LIST_X1 - 10 - QM_StrWidth(count, 13);
            QM_Gradient(d, QM_LIST_X0 + 4, y + 1, QM_LIST_X1 - QM_LIST_X0 - 8, h - 3, bandTop, bandBottom, true);
            QM_Line(d, QM_LIST_X0 + 4, y + h - 1.5f, QM_LIST_X1 - 4, y + h - 1.5f, 1.0f, 96, 106, 128, 255);
            if (l->style == 0) {
                QM_WingedTriforce(d, 243, y + 6, 36);
                QM_Print(d, 264, y + 5, l->title, 17, 250, 214, 110, true, countX - 4);
            } else {
                QM_Circle(d, 243, y + 13, 10.5f, 10, 20, 60, 255);
                QM_Circle(d, 243, y + 13, 9.0f, 60, 110, 230, 255);
                QM_Circle(d, 243, y + 11, 6.0f, 110, 160, 255, 255);
                QM_PrintCentered(d, 243, y + 5, "!", 16, 255, 255, 255, true);
                QM_Print(d, 264, y + 5, l->title, 17, 150, 200, 255, true, countX - 4);
            }
            QM_Print(d, countX, y + 7, count, 13, 200, 206, 220, true, QM_LIST_X1 - 6);
        } else {
            const QuestMenuRow* row = &gQuestMenu.rows[l->row];
            bool locked = row->state == QM_STATE_LOCKED;
            f32 rightX = QM_LIST_X1 - 10;
            u8 r = 240;
            u8 g = 234;
            u8 b = 216;

            if (row->done) {
                r = 172, g = 170, b = 162;
            } else if (locked) {
                r = 128, g = 134, b = 148;
            }
            if (l->row == cursor) {
                s32 cn = QM_Chamfer(pts, QM_LIST_X0 + 6, y, QM_LIST_X1 - 6, y + h - 1, 4);

                QM_Fan(d, pts, cn, false, 120, 90, 20, (u8)(90 + 30 * pulse));
                QM_Glow(d, pts, cn, pulse);
            } else {
                QM_Line(d, QM_LIST_X0 + 12, y + h - 0.5f, QM_LIST_X1 - 12, y + h - 0.5f, 1.0f, 62, 68, 84, 255);
            }

            QuestMenu_DrawStateBox(d, row, y);
            QM_Icon(d, row->icon, row->iconIs24, 258, (s16)y + 3, 18, !locked);

            if (row->hasCount) {
                char text[16];

                snprintf(text, sizeof(text), "%d/%d", row->got, row->total);
                rightX -= QM_StrWidth(text, 15);
                QM_Print(d, rightX, y + 5, text, 15, 255, row->done ? 214 : 240, row->done ? 90 : 230, true,
                         QM_LIST_X1 - 6);
                rightX -= 6;
            }
            {
                // Smaller letters for long names.
                s16 size = 15;

                while ((size > 12) && (QM_StrWidth(row->name, size) > rightX - 282)) {
                    size--;
                }
                QM_Print(d, 282, y + 5 + (15 - size) / 2, row->name, size, r, g, b, true, rightX);
            }
            QuestMenu_AddHit(QM_LIST_X0 + 4, y, QM_LIST_X1 - 4, y + h, QM_HIT_ROW, l->row);
        }
        y += h;
    }

    // Scroll arrows, like the ones of the item lists of the game; they can be touched too.
    if (*top > 0) {
        QM_Tex(d, gArrowUpTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, QM_LIST_X1 - 18, QM_LIST_Y0 - 6, 12, 12, true,
               (u8)(150 + 100 * pulse));
        QuestMenu_AddHit(QM_LIST_X1 - 34, QM_LIST_Y0 - 14, QM_LIST_X1 + 6, QM_LIST_Y0 + 10, QM_HIT_SCROLL_UP, 0);
    }
    if (line < lineCount) {
        QM_Tex(d, gArrowDownTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, QM_LIST_X1 - 18, QM_LIST_Y1 - 6, 12, 12, true,
               (u8)(150 + 100 * pulse));
        QuestMenu_AddHit(QM_LIST_X1 - 34, QM_LIST_Y1 - 12, QM_LIST_X1 + 6, QM_LIST_Y1 + 12, QM_HIT_SCROLL_DOWN, 0);
    }
}

// The state of the selected line, as a small colored plate right-aligned at `rightX`.
static void QuestMenu_DrawStateBadge(QuestMenuDraw* d, f32 rightX, f32 y, u8 state) {
    static const u8 colors[4][3] = {
        { 96, 96, 104 },  // locked
        { 40, 76, 150 },  // available
        { 170, 110, 20 }, // in progress
        { 40, 120, 50 },  // done
    };
    const char* name = QuestMenu_StateName(state);
    f32 w = QM_StrWidth(name, 12) + 12;
    f32 pts[8][2];
    s32 n = QM_Chamfer(pts, rightX - w, y, rightX, y + 17, 4);

    QM_Fan(d, pts, n, false, colors[state][0], colors[state][1], colors[state][2], 235);
    QM_Outline(d, pts, n, 1.0f, 60, 40, 20, 200);
    QM_Print(d, rightX - w + 6, y + 3, name, 12, 255, 250, 236, false, rightX - 2);
}

static void QuestMenu_DrawCard(QuestMenuDraw* d, PlayState* play, s32 cursor, f32 pulse) {
    static const u8 burn[4] = { 112, 70, 30, 150 };
    static const u8 clear[4] = { 112, 70, 30, 0 };
    static const u8 barTop[4] = { 255, 226, 120, 255 };
    static const u8 barBottom[4] = { 190, 120, 20, 255 };
    const u8 ink[3] = { 58, 38, 20 };
    const u8 blue[3] = { 30, 40, 110 };
    const QuestMenuRow* row = (gQuestMenu.rowCount > 0) ? &gQuestMenu.rows[cursor] : NULL;
    f32 pts[4][2] = {
        { QM_CARD_X0, QM_CARD_Y0 }, { QM_CARD_X1, QM_CARD_Y0 }, { QM_CARD_X1, QM_CARD_Y1 }, { QM_CARD_X0, QM_CARD_Y1 }
    };
    QuestMenuGoal goals[QM_MAX_GOALS];
    s32 goalCount;
    s32 goalsDone = 0;
    s32 next = -1;
    s32 i;
    f32 y;
    s16 titleSize;
    char text[96];

    // Shadow, parchment, burnt edges and a thin brown rim.
    QM_Rect(d, QM_CARD_X0 + 4, QM_CARD_Y0 + 5, QM_CARD_X1 - QM_CARD_X0, QM_CARD_Y1 - QM_CARD_Y0, 0, 0, 0, 120);
    QM_Fan(d, pts, 4, false, 222, 200, 158, 255);
    QM_LoadTile(d, gQMParchmentTex);
    QM_Fan(d, pts, 4, true, 255, 246, 226, 255);
    QM_Gradient(d, QM_CARD_X0, QM_CARD_Y0, QM_CARD_X1 - QM_CARD_X0, 14, burn, clear, true);
    QM_Gradient(d, QM_CARD_X0, QM_CARD_Y1 - 14, QM_CARD_X1 - QM_CARD_X0, 14, clear, burn, true);
    QM_Gradient(d, QM_CARD_X0, QM_CARD_Y0, 14, QM_CARD_Y1 - QM_CARD_Y0, burn, clear, false);
    QM_Gradient(d, QM_CARD_X1 - 14, QM_CARD_Y0, 14, QM_CARD_Y1 - QM_CARD_Y0, clear, burn, false);
    QM_Outline(d, pts, 4, 1.5f, 110, 74, 40, 255);
    // Faded Triforce in the corner, like a watermark.
    QM_TriforceColor(d, QM_CARD_X1 - 58, QM_CARD_Y0 + 10, 44, 120, 88, 46, 60);

    if (row == NULL) {
        return;
    }
    goalCount = QuestMenu_GetGoals(play, row, goals, QM_MAX_GOALS);
    for (i = 0; i < goalCount; i++) {
        if (goals[i].done) {
            goalsDone++;
        } else if (next < 0) {
            next = i;
        }
    }

    // Title and description.
    // The title stops before the Triforce of the corner, with smaller letters when it is long.
    titleSize = 20;
    while ((titleSize > 13) && (QM_StrWidth(row->name, titleSize) > QM_CARD_X1 - 64 - (QM_CARD_X0 + 14))) {
        titleSize--;
    }
    QM_Print(d, QM_CARD_X0 + 14, QM_CARD_Y0 + 12 + (20 - titleSize) / 2, row->name, titleSize, 52, 30, 14, false,
             QM_CARD_X1 - 62);
    QM_Line(d, QM_CARD_X0 + 14, QM_CARD_Y0 + 36, QM_CARD_X1 - 60, QM_CARD_Y0 + 36, 1.2f, 150, 110, 70, 255);
    QM_DrawWrapped(d, QM_CARD_X0 + 14, QM_CARD_Y0 + 40, QM_CARD_X1 - QM_CARD_X0 - 28, row->detail, 13, 15, 6, ink[0],
                   ink[1], ink[2], 255);
    y = QM_CARD_Y0 + 132;
    QM_Line(d, QM_CARD_X0 + 8, y, QM_CARD_X1 - 8, y, 1.2f, 150, 112, 70, 200);
    QuestMenu_DrawStateBadge(d, QM_CARD_X1 - 14, y + 4, row->state);

    // Progress: the six medallions on the main quest; on the other lists a bar with the line's own progress (its
    // count, or its objectives), and the whole list below it.
    if (gQuestMenu.category == QM_CAT_MAIN) {
        static const u8 medallions[6] = { QUEST_MEDALLION_LIGHT, QUEST_MEDALLION_FOREST, QUEST_MEDALLION_FIRE,
                                          QUEST_MEDALLION_WATER, QUEST_MEDALLION_SHADOW, QUEST_MEDALLION_SPIRIT };
        static const char* medallionTexs[6] = { gQuestIconMedallionLightTex,  gQuestIconMedallionForestTex,
                                                gQuestIconMedallionFireTex,   gQuestIconMedallionWaterTex,
                                                gQuestIconMedallionShadowTex, gQuestIconMedallionSpiritTex };
        s32 owned = 0;

        for (i = 0; i < 6; i++) {
            owned += CHECK_QUEST_ITEM(medallions[i]) ? 1 : 0;
        }
        snprintf(text, sizeof(text), "Medalhões:  %d / 6", owned);
        QM_Print(d, QM_CARD_X0 + 16, y + 5, text, 15, blue[0], blue[1], blue[2], false, QM_CARD_X1 - 110);
        for (i = 0; i < 6; i++) {
            bool has = CHECK_QUEST_ITEM(medallions[i]) != 0;
            s16 mx = QM_CARD_X0 + 16 + i * 38;

            QM_Circle(d, mx + 14, y + 36, 15, 60, 40, 20, 90);
            QM_Icon(d, medallionTexs[i], true, mx, (s16)y + 22, 28, has);
            if (has) {
                QM_Tex(d, gQMCheckTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, mx + 16, (s16)y + 36, 16, 16, true, 255);
            }
        }
    } else {
        s16 got = row->hasCount ? row->got : goalsDone;
        s16 total = row->hasCount ? row->total : goalCount;
        f32 frac = (total > 0) ? CLAMP((f32)got / total, 0.0f, 1.0f) : 0.0f;
        f32 barW = QM_CARD_X1 - QM_CARD_X0 - 32;

        snprintf(text, sizeof(text), "Progresso:  %d / %d", got, total);
        QM_Print(d, QM_CARD_X0 + 16, y + 5, text, 15, blue[0], blue[1], blue[2], false, QM_CARD_X1 - 110);
        QM_Rect(d, QM_CARD_X0 + 15, (s16)y + 25, (s16)barW + 2, 14, 70, 44, 20, 220);
        if (frac > 0.0f) {
            QM_Gradient(d, QM_CARD_X0 + 16, y + 26, barW * frac, 12, barTop, barBottom, true);
        }
        snprintf(text, sizeof(text), "Nesta lista: %d / %d concluídas", gQuestMenu.got, gQuestMenu.total);
        QM_Print(d, QM_CARD_X0 + 16, y + 42, text, 13, ink[0], ink[1], ink[2], false, QM_CARD_X1 - 10);
    }

    // Next objective, then the objectives as icons, checked when done.
    y = QM_CARD_Y0 + 186;
    {
        f32 dm[4][2] = { { QM_CARD_X0 + 20, y + 2 },
                         { QM_CARD_X0 + 26, y + 8 },
                         { QM_CARD_X0 + 20, y + 14 },
                         { QM_CARD_X0 + 14, y + 8 } };
        QM_Fan(d, dm, 4, false, blue[0], blue[1], blue[2], 255);
    }
    if (row->state == QM_STATE_LOCKED) {
        snprintf(text, sizeof(text), "Próximo: conclua a etapa anterior.");
    } else if (QuestMenu_IsManualRow(row) && QuestMenu_CanMarkGanon()) {
        // The save cannot tell: the player marks it. The whole line can be touched.
        snprintf(text, sizeof(text), "%s", row->done ? QM_GLYPH_A " Desmarcar (marcada por você)"
                                                     : QM_GLYPH_A " Marcar como concluída");
        QuestMenu_AddHit(QM_CARD_X0 + 8, y - 6, QM_CARD_X1 - 8, y + 18, QM_HIT_MANUAL_DONE, 0);
    } else if (row->done) {
        bool last = (gQuestMenu.category == QM_CAT_MAIN) && (cursor == gQuestMenu.rowCount - 1);

        snprintf(text, sizeof(text), "%s", last ? "Hyrule está em paz. Parabéns!" : "Concluída!");
    } else if (row->hasCount) {
        snprintf(text, sizeof(text), "Próximo: faltam %d.", row->total - row->got);
    } else if (next >= 0) {
        snprintf(text, sizeof(text), "Próximo: %s", goals[next].label);
    } else {
        snprintf(text, sizeof(text), "Próximo: %s", row->name);
    }
    QM_Print(d, QM_CARD_X0 + 32, y, text, 14, blue[0], blue[1], blue[2], false, QM_CARD_X1 - 10);
    {
        s16 x = QM_CARD_X0 + 32;

        for (i = 0; (i < goalCount) && (x + 22 < QM_CARD_X1 - 8); i++) {
            QM_Circle(d, x + 11, y + 29, 12, 60, 40, 20, (i == next) ? (u8)(60 + 60 * pulse) : 50);
            QM_Icon(d, goals[i].icon, goals[i].iconIs24, x + 1, (s16)y + 19, 20, goals[i].done);
            if (goals[i].done) {
                QM_Tex(d, gQMCheckTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, x + 12, (s16)y + 29, 13, 13, true, 255);
            }
            x += 27;
        }
    }

    // The place on the pause world map, with Link's head blinking on it (or the Spiritual Stone / medallion of a
    // dungeon).
    {
        s16 boxW = QM_MAP_TEX_W * QM_MAP_SCALE;
        s16 boxH = QM_MAP_TEX_H * QM_MAP_SCALE;

        QM_Rect(d, QM_MAP_X - 2, QM_MAP_Y - 2, boxW + 4, boxH + 4, 60, 40, 20, 255);
        if (row->point >= 0) {
            s16 px = gQuestMenuPointPos[row->point][0];
            s16 py = gQuestMenuPointPos[row->point][1];
            s16 u0 = CLAMP(px - QM_MAP_TEX_W / 2, 0, 216 - QM_MAP_TEX_W);
            s16 v0 = CLAMP(py - QM_MAP_TEX_H / 2, 0, 128 - QM_MAP_TEX_H);
            f32 mx = QM_MAP_X + (px - u0) * QM_MAP_SCALE;
            f32 my = QM_MAP_Y + (py - v0) * QM_MAP_SCALE;
            static const u8 shadeTop[4] = { 0, 0, 0, 0 };
            static const u8 shadeBottom[4] = { 0, 0, 0, 170 };

            QM_MapCrop(d, QM_MAP_X, QM_MAP_Y, QM_MAP_TEX_W, QM_MAP_TEX_H, u0, v0, QM_MAP_SCALE);
            QM_Circle(d, mx, my, 15, 255, 230, 120, (u8)(40 + 70 * pulse));
            QM_Circle(d, mx, my, 9, 255, 240, 160, (u8)(50 + 80 * pulse));
            if (QuestMenu_IsDungeonIcon(row->icon)) {
                QM_Icon(d, row->icon, true, (s16)mx - 11, (s16)my - 11, 22, true);
            } else {
                QM_Tex(d, gDungeonMapLinkHeadTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, (s16)mx - 10, (s16)my - 10, 20,
                       20, true, (u8)(120 + 135 * pulse));
            }
            QM_Gradient(d, QM_MAP_X, QM_MAP_Y + boxH - 22, boxW, 22, shadeTop, shadeBottom, true);
            QM_PrintCentered(d, QM_MAP_X + boxW / 2.0f, QM_MAP_Y + boxH - 18, gQuestMenuPointNames[row->point], 14, 255,
                             246, 214, true);
        } else {
            // No place: the icon of the line on a dark plate.
            QM_Rect(d, QM_MAP_X, QM_MAP_Y, boxW, boxH, 44, 34, 24, 255);
            QM_Circle(d, QM_MAP_X + boxW / 2.0f, QM_MAP_Y + boxH / 2.0f, 24, 255, 210, 90, (u8)(30 + 40 * pulse));
            QM_Icon(d, row->icon, row->iconIs24, QM_MAP_X + boxW / 2 - 18, QM_MAP_Y + boxH / 2 - 18, 36, true);
        }
    }

    // Number of the quest, small, under the map.
    snprintf(text, sizeof(text), "#%d", row->id);
    QM_Print(d, QM_CARD_X1 - 14 - QM_StrWidth(text, 10), QM_CARD_Y1 - 11, text, 10, 120, 92, 60, false,
             QM_CARD_X1 - 4);
}

void QuestMenu_DrawJournal(PlayState* play, f32 ease) {
    PauseContext* pauseCtx = &play->pauseCtx;
    QuestMenuDraw draw;
    QuestMenuDraw* d = &draw;
    s32 cursor = gQuestMenu.cursor[gQuestMenu.category];
    f32 pulse = (sinf(gQuestMenu.frames * 0.16f) + 1.0f) * 0.5f;
    f32 left = OTRGetRectDimensionFromLeftEdge(0.0f);
    f32 right = OTRGetRectDimensionFromRightEdge((f32)SCREEN_WIDTH);
    // Fit the 427 x 240 canvas on the screen, and grow it a little while it opens.
    f32 k = MIN(1.0f, (right - left) / (QM_W / 2.0f)) * 0.5f;
    f32 sk = k * (0.92f + 0.08f * ease);

    gQuestMenu.scale = sk;
    gQuestMenu.hitCount = 0;
    QM_Begin(play, d, sk, (u8)(pauseCtx->alpha * ease));

    // Darken everything behind the journal, the sides of wide screens too.
    {
        f32 x0 = QM_W / 2.0f + (left - 4.0f - SCREEN_WIDTH / 2.0f) / sk;
        f32 x1 = QM_W / 2.0f + (right + 4.0f - SCREEN_WIDTH / 2.0f) / sk;
        f32 y0 = QM_H / 2.0f - (SCREEN_HEIGHT / 2.0f + 4.0f) / sk;
        f32 y1 = QM_H / 2.0f + (SCREEN_HEIGHT / 2.0f + 4.0f) / sk;
        f32 scrim[4][2] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };

        QM_Fan(d, scrim, 4, false, 4, 6, 12, 170);
    }

    // Stone frame.
    {
        f32 pts[8][2];
        s32 n = QM_Chamfer(pts, 59, 80, 795, 443, 21);

        QM_Rect(d, 64, 86, 736, 363, 0, 0, 0, 120);
        QM_Stone(d, pts, n, 74, 84, 104, 4.0f);
        n = QM_Chamfer(pts, 69, 90, 785, 433, 15);
        QM_Recess(d, pts, n, 30, 34, 44);
    }

    // L and R tabs at the sides of the title plaque.
    {
        static const f32 tabX[2] = { 251, 603 };
        s32 i;

        for (i = 0; i < 2; i++) {
            f32 cx = tabX[i];
            f32 tab[4][2] = { { cx - 27, 96 }, { cx - 17, 74 }, { cx + 17, 74 }, { cx + 27, 96 } };

            QM_Stone(d, tab, 4, 80, 90, 110, 2.5f);
            QM_PrintCentered(d, cx, 77, (i == 0) ? "L" : "R", 18, 200, 208, 224, true);
            QuestMenu_AddHit(cx - 34, 66, cx + 34, 104, (i == 0) ? QM_HIT_PREV_CATEGORY : QM_HIT_NEXT_CATEGORY, 0);
        }
    }

    // Title plaque with the winged Triforce.
    {
        f32 plaque[6][2] = { { 285, 88 }, { 308, 58 }, { 546, 58 }, { 569, 88 }, { 546, 118 }, { 308, 118 } };

        QM_Stone(d, plaque, 6, 88, 98, 120, 3.0f);
        QM_Outline(d, plaque, 6, 1.0f, 150, 160, 180, 120);
        QM_WingedTriforce(d, QM_W / 2.0f, 40, 64);
        QM_PrintCentered(d, QM_W / 2.0f, 76, "Diário de Quests", 24, 236, 224, 192, true);
    }

    // Controls, in the corners above the content. "Voltar" can be touched.
    {
        f32 backW = QM_StrWidth(QM_GLYPH_B " Voltar", 14);

        QM_Print(d, 82, 99, QM_GLYPH_L "/" QM_GLYPH_R " Categoria", 14, 210, 214, 224, true, 290);
        QM_Print(d, 772 - backW, 99, QM_GLYPH_B " Voltar", 14, 210, 214, 224, true, 790);
        QuestMenu_AddHit(772 - backW - 12, 90, 790, 124, QM_HIT_CLOSE, 0);
    }

    QuestMenu_DrawSidebar(d, pulse);
    QuestMenu_DrawList(d, cursor, pulse);
    QuestMenu_DrawCard(d, play, cursor, pulse);

    // Plaque at the bottom with a hint about the list.
    {
        f32 plaque[6][2] = { { 232, 442 }, { 250, 427 }, { 604, 427 }, { 622, 442 }, { 604, 458 }, { 250, 458 } };

        QM_Stone(d, plaque, 6, 70, 80, 98, 2.5f);
        QM_PrintCentered(d, QM_W / 2.0f, 435, gQuestMenuCategoryHints[gQuestMenu.category], 15, 226, 224, 214, true);
    }

    QM_End(play, d);
}
