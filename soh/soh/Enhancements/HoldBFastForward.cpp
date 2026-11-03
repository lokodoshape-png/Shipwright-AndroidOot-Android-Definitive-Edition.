// Hold B to fast-forward cutscenes (OOT D.E.).
//
// Holding B for a moment during a scripted cutscene runs extra game logic frames per displayed frame, so the cutscene
// plays through quickly with all of its script commands, actor cues and flags intact (nothing is skipped).
// A hint in the bottom right corner tells the player about it. It only works in a game started from file select:
// the title screen intro (gameMode GAMEMODE_TITLE_SCREEN) is left alone.
#include <libultraship/bridge.h>
#include <cstring>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/cvar_prefixes.h"
#include "soh/ShipInit.hpp"
#include "soh/OTRGlobals.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
int16_t OTRGetRectDimensionFromRightEdge(float v);
}

#define CVAR_NAME CVAR_ENHANCEMENT("HoldBToFastForward")
#define CVAR_SPEED CVAR_ENHANCEMENT("HoldBToFastForwardSpeed")
#define CVAR_HINT CVAR_ENHANCEMENT("HoldBToFastForwardHint")
#define CVAR_DIALOGS CVAR_ENHANCEMENT("HoldBToFastForwardDialogs")

// B has to be held this many frames before fast-forwarding starts, so a tap on B (to advance text) does nothing.
#define HOLD_FRAMES 10
// The hint appears once a cutscene has run this long, so it stays out of very short camera cuts.
#define HINT_DELAY_FRAMES 20

static s32 sHeldFrames = 0;
static s32 sCutsceneFrames = 0;
static bool sFastForwarding = false;

static bool InFastForwardableCutscene(PlayState* play) {
    if (gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2) {
        return false;
    }
    if (play->csCtx.state == CS_STATE_IDLE) {
        return false;
    }
    // Never rush through a question: the extra frames would repeat the held B on the choice.
    return Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE;
}

extern "C" s32 HoldBFastForward_GetExtraFrames(PlayState* play) {
    sFastForwarding = false;
    if (!CVarGetInteger(CVAR_NAME, 1) || play == NULL || !InFastForwardableCutscene(play)) {
        sHeldFrames = 0;
        sCutsceneFrames = 0;
        return 0;
    }
    sCutsceneFrames++;

    if (!CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_B)) {
        sHeldFrames = 0;
        return 0;
    }
    if (sHeldFrames < HOLD_FRAMES) {
        sHeldFrames++;
        return 0;
    }
    sFastForwarding = true;
    return CVarGetInteger(CVAR_SPEED, 5) - 1;
}

// Optional: while B is held in a cutscene, text boxes pass by themselves, the same way "Skip Text" does
// (z_message_PAL.c, SKIP_TEXT_ACTIVE). Off by default.
extern "C" bool HoldBFastForward_SkipTextActive(PlayState* play) {
    if (play == NULL || !CVarGetInteger(CVAR_NAME, 1) || !CVarGetInteger(CVAR_DIALOGS, 0)) {
        return false;
    }
    if (gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2) {
        return false;
    }
    return play->csCtx.state != CS_STATE_IDLE;
}

// Extra logic frames stop as soon as the cutscene ends or a scene change begins.
extern "C" bool HoldBFastForward_ShouldContinue(PlayState* play) {
    return play->state.running && play->transitionTrigger == TRANS_TRIGGER_OFF && InFastForwardableCutscene(play);
}

static void DrawHint() {
    if (gPlayState == NULL || !CVarGetInteger(CVAR_HINT, 1) || sCutsceneFrames < HINT_DELAY_FRAMES ||
        !InFastForwardableCutscene(gPlayState)) {
        return;
    }
    // The debug font has no accents, so the text is written without them.
    const char* text = sFastForwarding ? "Acelerando >>" : "Segure B para acelerar";
    const s32 length = (s32)strlen(text);

    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx* polyOpa = POLY_OPA_DISP;
    Gfx* gfx = Graph_GfxPlusOne(polyOpa);
    gSPDisplayList(OVERLAY_DISP++, gfx);

    GfxPrint printer;
    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);
    // Bottom right corner of the screen (8x8 pixel characters), over the letterbox.
    s32 x = OTRGetRectDimensionFromRightEdge(SCREEN_WIDTH - 8 - length * 8);
    GfxPrint_SetPosPx(&printer, x, SCREEN_HEIGHT - 20);
    if (sFastForwarding) {
        GfxPrint_SetColor(&printer, 255, 220, 80, 255);
    } else {
        GfxPrint_SetColor(&printer, 255, 255, 255, 220);
    }
    GfxPrint_Printf(&printer, "%s", text);
    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    Graph_BranchDlist(polyOpa, gfx);
    POLY_OPA_DISP = gfx;
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

static void RegisterHoldBFastForward() {
    COND_HOOK(OnPlayDrawEnd, CVarGetInteger(CVAR_NAME, 1), DrawHint);
}

static RegisterShipInitFunc sInit(RegisterHoldBFastForward, { CVAR_NAME });
