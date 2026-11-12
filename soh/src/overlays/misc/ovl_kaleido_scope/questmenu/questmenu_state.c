// OOT D.E.: QuestMenu state. Opening and closing the journal (the page flip of the Quest Status page), the current
// list and line, and the input: controller and keyboard (through the controller mapping of the game), touch screen
// and mouse (through the pointer of QuestMenuPointer.cpp and the touch areas recorded by the interface). See
// questmenu.h.
#include "questmenu.h"
#include "soh/Enhancements/QuestMenu/QuestMenuPointer.h"
#include <math.h>

QuestMenuContext gQuestMenu;

static s16 sSavedCursorItem = PAUSE_ITEM_NONE;

// Controller.
static s8 sHeldX = 0;
static s8 sHeldY = 0;
static s16 sRepeat = 0;

// Page flip between the Quest Status page and the journal.
#define QM_FLIP_FRAMES 12
static s16 sFlipTimer = 0;
static u8 sFlipTarget = PAUSE_QUEST_MODE_STATUS;

// Pointer (touch / mouse), in local units.
#define QM_DRAG_START 10.0f
static bool sPointerDown = false;
static bool sDragging = false;
static f32 sPressX;
static f32 sPressY;
static s32 sPressHit = -1;
static s32 sDragStartTop = 0;

static void QuestMenu_PlayCursorSound(void) {
    Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

void QuestMenu_AddHit(f32 x0, f32 y0, f32 x1, f32 y1, u8 type, s16 value) {
    QuestMenuHit* hit;

    if (gQuestMenu.hitCount >= QM_MAX_HITS) {
        return;
    }
    hit = &gQuestMenu.hits[gQuestMenu.hitCount++];
    hit->x0 = x0;
    hit->y0 = y0;
    hit->x1 = x1;
    hit->y1 = y1;
    hit->type = type;
    hit->value = value;
}

// The area under a point: the last one recorded, which is the one drawn on top.
static s32 QuestMenu_FindHit(f32 x, f32 y) {
    s32 i;

    for (i = gQuestMenu.hitCount - 1; i >= 0; i--) {
        const QuestMenuHit* hit = &gQuestMenu.hits[i];

        if ((x >= hit->x0) && (x < hit->x1) && (y >= hit->y0) && (y < hit->y1)) {
            return i;
        }
    }
    return -1;
}

static void QuestMenu_SetCategory(s32 category) {
    category = (category + QM_CAT_COUNT) % QM_CAT_COUNT;
    if (category != gQuestMenu.category) {
        gQuestMenu.category = category;
        gQuestMenu.freeScroll = false;
        QuestMenu_PlayCursorSound();
    }
}

static void QuestMenu_MoveCursor(s32 step) {
    s32* cursor = &gQuestMenu.cursor[gQuestMenu.category];
    s32 next = CLAMP(*cursor + step, 0, MAX(gQuestMenu.rowCount - 1, 0));

    gQuestMenu.freeScroll = false;
    if (next != *cursor) {
        *cursor = next;
        QuestMenu_PlayCursorSound();
    }
}

// Whether the journal may read the input now: the pause menu is on its main state and no page is flipping.
static bool QuestMenu_CanUseInput(PauseContext* pauseCtx) {
    return (sFlipTimer == 0) && (pauseCtx->state == 6) &&
           ((pauseCtx->unk_1E4 == 0) || (pauseCtx->unk_1E4 == 5) || (pauseCtx->unk_1E4 == 8));
}

// A (or touching the card) on the Ganondorf line, once it is unlocked: mark it done, or take the mark back.
static void QuestMenu_ToggleManualDone(void) {
    const QuestMenuRow* row;

    if (gQuestMenu.rowCount <= 0) {
        return;
    }
    row = &gQuestMenu.rows[CLAMP(gQuestMenu.cursor[gQuestMenu.category], 0, gQuestMenu.rowCount - 1)];
    if (!QuestMenu_IsManualRow(row) || (row->state == QM_STATE_LOCKED) || !QuestMenu_CanMarkGanon()) {
        return;
    }
    QuestMenu_SetGanonMark(!QuestMenu_GetGanonMark());
    Audio_PlaySoundGeneral(NA_SE_SY_DECIDE, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

// Up/down (stick or D-pad, held to repeat) choose the line. L/R, Z, left/right and C-Left/C-Right choose the list.
// B closes the journal (see the pause menu update). The keyboard reaches these through the controller mapping.
static void QuestMenu_HandleController(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = &play->state.input[0];
    s8 dirX;
    s8 dirY;

    if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
        QuestMenu_ToggleManualDone();
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_R) || CHECK_BTN_ALL(input->press.button, BTN_CRIGHT)) {
        QuestMenu_SetCategory(gQuestMenu.category + 1);
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_Z) || CHECK_BTN_ALL(input->press.button, BTN_L) ||
        CHECK_BTN_ALL(input->press.button, BTN_CLEFT)) {
        QuestMenu_SetCategory(gQuestMenu.category - 1);
        return;
    }

    dirX = 0;
    if ((pauseCtx->stickRelX < -30) || CHECK_BTN_ALL(input->cur.button, BTN_DLEFT)) {
        dirX = -1;
    } else if ((pauseCtx->stickRelX > 30) || CHECK_BTN_ALL(input->cur.button, BTN_DRIGHT)) {
        dirX = 1;
    }
    if ((dirX != 0) && (dirX != sHeldX)) {
        sHeldX = dirX;
        QuestMenu_SetCategory(gQuestMenu.category + dirX);
        return;
    }
    sHeldX = dirX;

    dirY = 0;
    if ((pauseCtx->stickRelY > 30) || CHECK_BTN_ALL(input->cur.button, BTN_DUP)) {
        dirY = -1;
    } else if ((pauseCtx->stickRelY < -30) || CHECK_BTN_ALL(input->cur.button, BTN_DDOWN)) {
        dirY = 1;
    }
    if (dirY == 0) {
        sHeldY = 0;
    } else if (dirY != sHeldY) {
        sHeldY = dirY;
        sRepeat = 10;
        QuestMenu_MoveCursor(dirY);
    } else if (--sRepeat <= 0) {
        sRepeat = 3;
        QuestMenu_MoveCursor(dirY);
    }
}

static void QuestMenu_Scroll(s32 lines) {
    gQuestMenu.top[gQuestMenu.category] = MAX(gQuestMenu.top[gQuestMenu.category] + lines, 0);
    gQuestMenu.freeScroll = true;
}

static void QuestMenu_DoHit(PlayState* play, const QuestMenuHit* hit) {
    switch (hit->type) {
        case QM_HIT_CATEGORY:
            QuestMenu_SetCategory(hit->value);
            break;
        case QM_HIT_ROW:
            if ((hit->value < gQuestMenu.rowCount) && (gQuestMenu.cursor[gQuestMenu.category] != hit->value)) {
                gQuestMenu.cursor[gQuestMenu.category] = hit->value;
                QuestMenu_PlayCursorSound();
            }
            break;
        case QM_HIT_PREV_CATEGORY:
            QuestMenu_SetCategory(gQuestMenu.category - 1);
            break;
        case QM_HIT_NEXT_CATEGORY:
            QuestMenu_SetCategory(gQuestMenu.category + 1);
            break;
        case QM_HIT_SCROLL_UP:
            QuestMenu_Scroll(-3);
            break;
        case QM_HIT_SCROLL_DOWN:
            QuestMenu_Scroll(3);
            break;
        case QM_HIT_CLOSE:
            KaleidoScope_CloseQuestLog(play);
            break;
        case QM_HIT_MANUAL_DONE:
            QuestMenu_ToggleManualDone();
            break;
    }
}

// Tap: select what is under the finger when it is lifted on the same area it went down on. Drag on the list: scroll
// it. Mouse wheel: scroll the list.
static void QuestMenu_HandlePointer(PlayState* play) {
    QuestMenuPointer p;
    f32 x;
    f32 y;

    QuestMenuPointer_Get(&p);
    if (!p.valid || (gQuestMenu.scale <= 0.0f)) {
        sPointerDown = false;
        sDragging = false;
        sPressHit = -1;
        return;
    }
    // Game screen -> local units of the journal (the inverse of the matrix of QM_Begin).
    x = QM_W / 2.0f + (p.x - SCREEN_WIDTH / 2.0f) / gQuestMenu.scale;
    y = QM_H / 2.0f + (p.y - SCREEN_HEIGHT / 2.0f) / gQuestMenu.scale;

    if (p.wheel != 0.0f) {
        QuestMenu_Scroll((p.wheel > 0.0f) ? -2 : 2);
    }

    if (p.down && !sPointerDown) {
        sPressX = x;
        sPressY = y;
        sPressHit = QuestMenu_FindHit(x, y);
        sDragging = false;
        sDragStartTop = gQuestMenu.top[gQuestMenu.category];
    } else if (p.down && sPointerDown) {
        bool onList = (sPressHit >= 0) && ((gQuestMenu.hits[sPressHit].type == QM_HIT_ROW) ||
                                           (gQuestMenu.hits[sPressHit].type == QM_HIT_LIST));

        if (onList && !sDragging && (fabsf(y - sPressY) > QM_DRAG_START)) {
            sDragging = true;
        }
        if (sDragging) {
            // Dragging up shows the lines below.
            gQuestMenu.top[gQuestMenu.category] = MAX(sDragStartTop - (s32)((y - sPressY) / QM_ROW_H), 0);
            gQuestMenu.freeScroll = true;
        }
    } else if (!p.down && sPointerDown) {
        if (!sDragging && (sPressHit >= 0)) {
            s32 hit = QuestMenu_FindHit(x, y);

            if ((hit >= 0) && (gQuestMenu.hits[hit].type == gQuestMenu.hits[sPressHit].type) &&
                (gQuestMenu.hits[hit].value == gQuestMenu.hits[sPressHit].value)) {
                QuestMenu_DoHit(play, &gQuestMenu.hits[hit]);
            }
        }
        sDragging = false;
        sPressHit = -1;
    }
    sPointerDown = p.down;
}

static void QuestMenu_Rebuild(PlayState* play) {
    s32 i;

    gQuestMenu.rowCount = QuestMenu_BuildRows(play, gQuestMenu.category, gQuestMenu.rows, QM_MAX_ROWS);
    gQuestMenu.got = 0;
    gQuestMenu.total = gQuestMenu.rowCount;
    for (i = 0; i < gQuestMenu.rowCount; i++) {
        gQuestMenu.got += gQuestMenu.rows[i].done ? 1 : 0;
    }
}

bool QuestMenu_Update(PlayState* play, f32* openAmount) {
    PauseContext* pauseCtx = &play->pauseCtx;
    bool closing = (sFlipTimer > 0) && (sFlipTarget != PAUSE_QUEST_MODE_LOG);
    bool open = (pauseCtx->randoQuestMode == PAUSE_QUEST_MODE_LOG) && (pauseCtx->pageIndex == PAUSE_QUEST) && !closing;
    s32* cursor;
    f32 t;

    if (open) {
        if (gQuestMenu.anim == 0) {
            // Opening: show the next step of the story first.
            s32 i;

            gQuestMenu.rowCount = QuestMenu_BuildRows(play, QM_CAT_MAIN, gQuestMenu.rows, QM_MAX_ROWS);
            for (i = 0; i < gQuestMenu.rowCount; i++) {
                if (!gQuestMenu.rows[i].done) {
                    gQuestMenu.cursor[QM_CAT_MAIN] = i;
                    break;
                }
            }
            gQuestMenu.freeScroll = false;
            gQuestMenu.hitCount = 0;
            sPointerDown = false;
            sPressHit = -1;
        }
        gQuestMenu.anim = MIN(gQuestMenu.anim + 1, QM_OPEN_FRAMES);
    } else {
        gQuestMenu.anim = MAX(gQuestMenu.anim - 2, 0);
    }
    if (gQuestMenu.anim == 0) {
        gQuestMenu.hitCount = 0;
        return false;
    }
    gQuestMenu.frames++;

    QuestMenu_Rebuild(play);
    if (open) {
        if (QuestMenu_CanUseInput(pauseCtx)) {
            // The page arrows are not used here.
            pauseCtx->cursorSpecialPos = 0;
            QuestMenu_HandleController(play);
            QuestMenu_HandlePointer(play);
            QuestMenu_Rebuild(play);
        } else {
            sHeldX = sHeldY = 0;
        }
        // No item name in the panel below the page while the journal is shown.
        pauseCtx->cursorItem[PAUSE_QUEST] = PAUSE_ITEM_NONE;
    }
    cursor = &gQuestMenu.cursor[gQuestMenu.category];
    *cursor = CLAMP(*cursor, 0, MAX(gQuestMenu.rowCount - 1, 0));

    t = (f32)gQuestMenu.anim / QM_OPEN_FRAMES;
    *openAmount = t * t * (3.0f - 2.0f * t);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// The pause menu side: the journal is the PAUSE_QUEST_MODE_LOG mode of the Quest Status page.

void KaleidoScope_DrawQuestJournal(PlayState* play) {
    f32 openAmount;

    if (QuestMenu_Update(play, &openAmount)) {
        QuestMenu_DrawJournal(play, openAmount);
    }
}

static void QuestMenu_SetMode(PauseContext* pauseCtx, u8 mode) {
    if (mode == pauseCtx->randoQuestMode) {
        return;
    }
    if (mode == PAUSE_QUEST_MODE_LOG) {
        sSavedCursorItem = pauseCtx->cursorItem[PAUSE_QUEST];
    } else if (pauseCtx->randoQuestMode == PAUSE_QUEST_MODE_LOG) {
        pauseCtx->cursorItem[PAUSE_QUEST] = sSavedCursorItem;
    }
    pauseCtx->randoQuestMode = mode;
}

// C-Up on the Quest Status page: Quest Status -> journal -> (randomizer collectibles) -> Quest Status.
// The change happens halfway through the page flip.
void KaleidoScope_CycleQuestMode(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;

    if (sFlipTimer > 0) {
        return;
    }
    switch (pauseCtx->randoQuestMode) {
        case PAUSE_QUEST_MODE_STATUS:
            sFlipTarget = PAUSE_QUEST_MODE_LOG;
            break;
        case PAUSE_QUEST_MODE_LOG:
            sFlipTarget = IS_RANDO ? PAUSE_QUEST_MODE_RANDO : PAUSE_QUEST_MODE_STATUS;
            break;
        default:
            sFlipTarget = PAUSE_QUEST_MODE_STATUS;
            break;
    }
    sFlipTimer = QM_FLIP_FRAMES;
    Audio_PlaySoundGeneral(NA_SE_SY_WIN_SCROLL_RIGHT, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

// B (or the "Voltar" button) on the journal: flip back to the Quest Status page.
void KaleidoScope_CloseQuestLog(PlayState* play) {
    if ((sFlipTimer > 0) || (play->pauseCtx.randoQuestMode != PAUSE_QUEST_MODE_LOG)) {
        return;
    }
    sFlipTarget = PAUSE_QUEST_MODE_STATUS;
    sFlipTimer = QM_FLIP_FRAMES;
    Audio_PlaySoundGeneral(NA_SE_SY_WIN_SCROLL_LEFT, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

// Called with the Quest Status page matrix, before it is loaded: narrows the page while it flips.
void KaleidoScope_ApplyQuestPageFlip(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    f32 t;
    f32 scale;

    if (sFlipTimer <= 0) {
        return;
    }
    if ((pauseCtx->state != 6) || (pauseCtx->pageIndex != PAUSE_QUEST)) {
        // The menu closed or moved on: finish at once.
        QuestMenu_SetMode(pauseCtx, sFlipTarget);
        sFlipTimer = 0;
        return;
    }
    sFlipTimer--;
    if (sFlipTimer == QM_FLIP_FRAMES / 2) {
        QuestMenu_SetMode(pauseCtx, sFlipTarget);
    }
    t = (f32)(QM_FLIP_FRAMES - sFlipTimer) / QM_FLIP_FRAMES;
    scale = fabsf(cosf(t * (f32)M_PI));
    Matrix_Scale(MAX(scale, 0.02f), 1.0f, 1.0f, MTXMODE_APPLY);
}
