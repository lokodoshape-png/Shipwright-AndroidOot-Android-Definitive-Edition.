// OOT D.E.: Gold Skulltulas and Pieces of Heart per region on the pause screen world map.
//
// When the cursor is on a region of the world map, a small panel next to its dot shows how many of the region's
// Gold Skulltulas and Pieces of Heart have been collected, e.g. "1/4". The counts are read from the save flags every
// time the map is drawn, so they always match the game. A region with everything collected gets a golden dot, golden
// numbers and Link's map head blinking next to the panel.
//
// The panel is drawn with the map page's own matrix, so it turns with the page like the rest of the map.
// Only the overworld is counted: the dungeons (and the Ice Cavern Piece of Heart) are not regions of this map.
#include "z_kaleido_scope.h"

extern f32 sFontWidths[144];

#define CVAR_MAP_COLLECTIBLES CVAR_ENHANCEMENT("MapCollectibleCounts")

#define WORLD_MAP_POINT_COUNT 12

static const char sMapCollectSkullTex[] = "__OTR__textures/icon_item_24_static/gQuestIconGoldSkulltulaTex";
static const char sMapCollectHeartTex[] = "__OTR__textures/parameter_static/gHeartQuarterTex";
static const char sMapCollectLinkHeadTex[] = "__OTR__textures/icon_item_dungeon_static/gDungeonMapLinkHeadTex";
static const char sMapCollectDotTex[] = "__OTR__textures/icon_item_field_static/gWorldMapDotTex";
static const char* sMapCollectDigitTexs[] = {
    "__OTR__textures/nes_font_static/gMsgChar30Digit0Tex", "__OTR__textures/nes_font_static/gMsgChar31Digit1Tex",
    "__OTR__textures/nes_font_static/gMsgChar32Digit2Tex", "__OTR__textures/nes_font_static/gMsgChar33Digit3Tex",
    "__OTR__textures/nes_font_static/gMsgChar34Digit4Tex", "__OTR__textures/nes_font_static/gMsgChar35Digit5Tex",
    "__OTR__textures/nes_font_static/gMsgChar36Digit6Tex", "__OTR__textures/nes_font_static/gMsgChar37Digit7Tex",
    "__OTR__textures/nes_font_static/gMsgChar38Digit8Tex", "__OTR__textures/nes_font_static/gMsgChar39Digit9Tex",
};
static const char sMapCollectSlashTex[] = "__OTR__textures/nes_font_static/gMsgChar2FSolidusTex";

// World map points: 0 Haunted Wasteland/Desert Colossus, 1 Gerudo's Fortress, 2 Gerudo Valley, 3 Lake Hylia,
// 4 Lon Lon Ranch, 5 Market/Hyrule Castle, 6 Hyrule Field, 7 Death Mountain, 8 Kakariko Village/Graveyard,
// 9 Lost Woods/Sacred Forest Meadow, 10 Kokiri Forest, 11 Zora's River/Domain/Fountain.

typedef struct {
    u8 point;
    u8 gsIndex; // index into the gsFlags (one byte per area)
    u8 flag;
} MapCollectSkulltula;

// Generated from the Gold Skulltula locations of the randomizer location table (overworld only, 56 in total).
static const MapCollectSkulltula sMapCollectSkulltulas[] = {
    { 10, 0x0C, 0x01 }, // RC_KF_GS_BEAN_PATCH
    { 10, 0x0C, 0x02 }, // RC_KF_GS_KNOW_IT_ALL_HOUSE
    { 10, 0x0C, 0x04 }, // RC_KF_GS_HOUSE_OF_TWINS
    { 9, 0x0D, 0x01 }, // RC_LW_GS_BEAN_PATCH_NEAR_BRIDGE
    { 9, 0x0D, 0x02 }, // RC_LW_GS_BEAN_PATCH_NEAR_THEATER
    { 9, 0x0D, 0x04 }, // RC_LW_GS_ABOVE_THEATER
    { 9, 0x0D, 0x08 }, // RC_SFM_GS
    { 6, 0x0A, 0x01 }, // RC_HF_GS_COW_GROTTO
    { 6, 0x0A, 0x02 }, // RC_HF_GS_NEAR_KAK_GROTTO
    { 3, 0x12, 0x01 }, // RC_LH_GS_BEAN_PATCH
    { 3, 0x12, 0x02 }, // RC_LH_GS_SMALL_ISLAND
    { 3, 0x12, 0x04 }, // RC_LH_GS_LAB_WALL
    { 3, 0x12, 0x08 }, // RC_LH_GS_LAB_CRATE
    { 3, 0x12, 0x10 }, // RC_LH_GS_TREE
    { 2, 0x13, 0x01 }, // RC_GV_GS_BEAN_PATCH
    { 2, 0x13, 0x02 }, // RC_GV_GS_SMALL_BRIDGE
    { 2, 0x13, 0x04 }, // RC_GV_GS_PILLAR
    { 2, 0x13, 0x08 }, // RC_GV_GS_BEHIND_TENT
    { 1, 0x14, 0x01 }, // RC_GF_GS_ARCHERY_RANGE
    { 1, 0x14, 0x02 }, // RC_GF_GS_TOP_FLOOR
    { 0, 0x15, 0x02 }, // RC_WASTELAND_GS
    { 0, 0x15, 0x01 }, // RC_COLOSSUS_GS_BEAN_PATCH
    { 0, 0x15, 0x04 }, // RC_COLOSSUS_GS_HILL
    { 0, 0x15, 0x08 }, // RC_COLOSSUS_GS_TREE
    { 5, 0x0E, 0x01 }, // RC_OGC_GS
    { 5, 0x0E, 0x02 }, // RC_HC_GS_STORMS_GROTTO
    { 5, 0x0E, 0x04 }, // RC_HC_GS_TREE
    { 5, 0x0E, 0x08 }, // RC_MARKET_GS_GUARD_HOUSE
    { 8, 0x10, 0x08 }, // RC_KAK_GS_HOUSE_UNDER_CONSTRUCTION
    { 8, 0x10, 0x10 }, // RC_KAK_GS_SKULLTULA_HOUSE
    { 8, 0x10, 0x02 }, // RC_KAK_GS_GUARDS_HOUSE
    { 8, 0x10, 0x20 }, // RC_KAK_GS_TREE
    { 8, 0x10, 0x04 }, // RC_KAK_GS_WATCHTOWER
    { 8, 0x10, 0x40 }, // RC_KAK_GS_ABOVE_IMPAS_HOUSE
    { 8, 0x10, 0x80 }, // RC_GRAVEYARD_GS_WALL
    { 8, 0x10, 0x01 }, // RC_GRAVEYARD_GS_BEAN_PATCH
    { 7, 0x0F, 0x01 }, // RC_DMC_GS_BEAN_PATCH
    { 7, 0x0F, 0x80 }, // RC_DMC_GS_CRATE
    { 7, 0x0F, 0x02 }, // RC_DMT_GS_BEAN_PATCH
    { 7, 0x0F, 0x04 }, // RC_DMT_GS_NEAR_KAK
    { 7, 0x0F, 0x08 }, // RC_DMT_GS_ABOVE_DODONGOS_CAVERN
    { 7, 0x0F, 0x10 }, // RC_DMT_GS_FALLING_ROCKS_PATH
    { 7, 0x0F, 0x20 }, // RC_GC_GS_CENTER_PLATFORM
    { 7, 0x0F, 0x40 }, // RC_GC_GS_BOULDER_MAZE
    { 11, 0x11, 0x01 }, // RC_ZR_GS_LADDER
    { 11, 0x11, 0x02 }, // RC_ZR_GS_TREE
    { 11, 0x11, 0x08 }, // RC_ZR_GS_ABOVE_BRIDGE
    { 11, 0x11, 0x10 }, // RC_ZR_GS_NEAR_RAISED_GROTTOS
    { 11, 0x11, 0x40 }, // RC_ZD_GS_FROZEN_WATERFALL
    { 11, 0x11, 0x04 }, // RC_ZF_GS_ABOVE_THE_LOG
    { 11, 0x11, 0x20 }, // RC_ZF_GS_HIDDEN_CAVE
    { 11, 0x11, 0x80 }, // RC_ZF_GS_TREE
    { 4, 0x0B, 0x01 }, // RC_LLR_GS_BACK_WALL
    { 4, 0x0B, 0x02 }, // RC_LLR_GS_RAIN_SHED
    { 4, 0x0B, 0x04 }, // RC_LLR_GS_HOUSE_WINDOW
    { 4, 0x0B, 0x08 }, // RC_LLR_GS_TREE
};

typedef enum {
    MC_COLLECT,   // collectible flag of a scene (freestanding Piece of Heart)
    MC_CHEST,     // chest flag of a scene
    MC_ITEMGETINF,
    MC_INFTABLE,
    MC_EVENTCHKINF,
    MC_FISHING,   // child fishing prize
    MC_HF_SCRUB,  // Deku Scrub in the Hyrule Field grotto
} MapCollectCheckType;

typedef struct {
    u8 point;
    u8 type;
    s16 scene;
    u16 flag;
} MapCollectHeartPiece;

// The 35 overworld Pieces of Heart (the 36th is in the Ice Cavern).
static const MapCollectHeartPiece sMapCollectHeartPieces[] = {
    { 0, MC_COLLECT, SCENE_DESERT_COLOSSUS, 0x0D },
    { 1, MC_CHEST, SCENE_GERUDOS_FORTRESS, 0x00 },
    { 1, MC_INFTABLE, -1, INFTABLE_190 }, // Horseback archery, 1000 points
    { 2, MC_COLLECT, SCENE_GERUDO_VALLEY, 0x01 },
    { 2, MC_COLLECT, SCENE_GERUDO_VALLEY, 0x02 },
    { 3, MC_ITEMGETINF, -1, 0x10 }, // Lab dive
    { 3, MC_FISHING, -1, 0 },
    { 3, MC_COLLECT, SCENE_LAKE_HYLIA, 0x1E },
    { 4, MC_COLLECT, SCENE_LON_LON_BUILDINGS, 0x01 },
    { 5, MC_ITEMGETINF, -1, 0x12 }, // Bombchu Bowling second prize
    { 5, MC_INFTABLE, -1, INFTABLE_191 }, // Lost dog
    { 5, MC_ITEMGETINF, -1, ITEMGETINF_1B }, // Treasure chest game
    { 6, MC_COLLECT, SCENE_GROTTOS, 0x01 }, // Tektite grotto
    { 6, MC_HF_SCRUB, -1, 0 },
    { 7, MC_COLLECT, SCENE_DEATH_MOUNTAIN_TRAIL, 0x1E },
    { 7, MC_COLLECT, SCENE_GORON_CITY, 0x1F },
    { 7, MC_COLLECT, SCENE_DEATH_MOUNTAIN_CRATER, 0x02 },
    { 7, MC_COLLECT, SCENE_DEATH_MOUNTAIN_CRATER, 0x08 },
    { 8, MC_EVENTCHKINF, -1, 0xDE }, // 50 Gold Skulltulas reward
    { 8, MC_ITEMGETINF, -1, 0x15 }, // Man on the roof
    { 8, MC_COLLECT, SCENE_IMPAS_HOUSE, 0x01 },
    { 8, MC_COLLECT, SCENE_WINDMILL_AND_DAMPES_GRAVE, 0x01 },
    { 8, MC_CHEST, SCENE_REDEAD_GRAVE, 0x00 },
    { 8, MC_COLLECT, SCENE_GRAVEYARD, 0x04 },
    { 8, MC_COLLECT, SCENE_WINDMILL_AND_DAMPES_GRAVE, 0x07 }, // Dampe race
    { 8, MC_COLLECT, SCENE_GRAVEYARD, 0x19 },                 // Gravedigging tour
    { 9, MC_ITEMGETINF, -1, 0x16 },                           // Skull Kid
    { 9, MC_ITEMGETINF, -1, 0x17 },                           // Ocarina memory game
    { 11, MC_EVENTCHKINF, -1, EVENTCHKINF_SONGS_FOR_FROGS_STORMS },
    { 11, MC_EVENTCHKINF, -1, EVENTCHKINF_SONGS_FOR_FROGS_CHOIR },
    { 11, MC_COLLECT, SCENE_ZORAS_RIVER, 0x04 },
    { 11, MC_COLLECT, SCENE_ZORAS_RIVER, 0x0B },
    { 11, MC_CHEST, SCENE_ZORAS_DOMAIN, 0x00 },
    { 11, MC_COLLECT, SCENE_ZORAS_FOUNTAIN, 0x01 },
    { 11, MC_COLLECT, SCENE_ZORAS_FOUNTAIN, 0x14 },
};

static s32 sMapCollectFrames = 0;

static bool MapCollect_SceneFlag(PlayState* play, s16 scene, u16 flag, bool chest) {
    if (scene == play->sceneNum) {
        // The flags of the current scene live in the actor context until the scene is left.
        return chest ? Flags_GetTreasure(play, flag) : Flags_GetCollectible(play, flag);
    }
    return (chest ? gSaveContext.sceneFlags[scene].chest : gSaveContext.sceneFlags[scene].collect) & (1 << flag);
}

static bool MapCollect_HeartPieceTaken(PlayState* play, const MapCollectHeartPiece* hp) {
    switch (hp->type) {
        case MC_COLLECT:
            return MapCollect_SceneFlag(play, hp->scene, hp->flag, false);
        case MC_CHEST:
            return MapCollect_SceneFlag(play, hp->scene, hp->flag, true);
        case MC_ITEMGETINF:
            return GET_ITEMGETINF(hp->flag) != 0;
        case MC_INFTABLE:
            return GET_INFTABLE(hp->flag) != 0;
        case MC_EVENTCHKINF:
            return GET_EVENTCHKINF(hp->flag) != 0;
        case MC_FISHING:
            return (HIGH_SCORE(HS_FISHING) & HS_FISH_PRIZE_CHILD) || Flags_GetRandomizerInf(RAND_INF_CHILD_FISHING);
        case MC_HF_SCRUB:
            return GET_ITEMGETINF(ITEMGETINF_DEKU_SCRUB_HEART_PIECE) ||
                   Flags_GetRandomizerInf(RAND_INF_SCRUBS_PURCHASED_HF_DEKU_SCRUB_GROTTO);
    }
    return false;
}

typedef struct {
    s16 skullGot;
    s16 skullTotal;
    s16 heartGot;
    s16 heartTotal;
} MapCollectCounts;

static void MapCollect_Count(PlayState* play, s32 point, MapCollectCounts* counts) {
    s32 i;

    counts->skullGot = counts->skullTotal = counts->heartGot = counts->heartTotal = 0;
    for (i = 0; i < ARRAY_COUNT(sMapCollectSkulltulas); i++) {
        if (sMapCollectSkulltulas[i].point == point) {
            counts->skullTotal++;
            if (GET_GS_FLAGS(sMapCollectSkulltulas[i].gsIndex) & sMapCollectSkulltulas[i].flag) {
                counts->skullGot++;
            }
        }
    }
    for (i = 0; i < ARRAY_COUNT(sMapCollectHeartPieces); i++) {
        if (sMapCollectHeartPieces[i].point == point) {
            counts->heartTotal++;
            if (MapCollect_HeartPieceTaken(play, &sMapCollectHeartPieces[i])) {
                counts->heartGot++;
            }
        }
    }
}

static bool MapCollect_IsComplete(const MapCollectCounts* counts) {
    return (counts->skullGot == counts->skullTotal) && (counts->heartGot == counts->heartTotal) &&
           (counts->skullTotal + counts->heartTotal > 0);
}

// Glyph size of the digits; the font glyphs are 16x16 and are drawn at 10x10.
#define MC_GLYPH 10
#define MC_ICON 12
#define MC_ROW_H 13
#define MC_PAD 3

// Writes "got/total" into text (at least 8 bytes).
static void MapCollect_Format(char* text, s16 got, s16 total) {
    s32 n = 0;
    s16 values[2] = { got, total };
    s32 v;

    for (v = 0; v < 2; v++) {
        s16 value = CLAMP(values[v], 0, 99);
        if (value >= 10) {
            text[n++] = '0' + value / 10;
        }
        text[n++] = '0' + value % 10;
        if (v == 0) {
            text[n++] = '/';
        }
    }
    text[n] = '\0';
}

static s16 MapCollect_CharAdvance(s32 c) {
    return (s16)(sFontWidths[c - ' '] * MC_GLYPH / 16.0f + 0.5f);
}

static s16 MapCollect_TextWidth(s16 got, s16 total) {
    char text[8];
    s16 w = 0;
    s32 i;

    MapCollect_Format(text, got, total);
    for (i = 0; text[i] != '\0'; i++) {
        w += MapCollect_CharAdvance(text[i]);
    }
    return w;
}

// One textured quad. x/y is the top left corner on screen, in the page's units (y grows upwards). In the mirrored
// world the page matrix flips x, so the quad is placed at -x and its texture is flipped back to read normally.
static void MapCollect_Quad(Gfx** gfxp, Vtx* vtx, bool mirrored, s16 x, s16 y, s16 w, s16 h, s16 texW, s16 texH,
                            u8 alpha) {
    Gfx* gfx = *gfxp;
    s32 i;
    s16 left = mirrored ? -(x + w) : x;
    s16 right = left + w;

    for (i = 0; i < 4; i++) {
        vtx[i].v.ob[0] = (i & 1) ? right : left;
        vtx[i].v.ob[1] = (i & 2) ? (y - h) : y;
        vtx[i].v.ob[2] = 0;
        vtx[i].v.flag = 0;
        // The vertex on the screen's left gets s = 0, also when the page is mirrored.
        bool screenLeft = mirrored ? (i & 1) : !(i & 1);
        vtx[i].v.tc[0] = screenLeft ? 0 : (texW << 5);
        vtx[i].v.tc[1] = (i & 2) ? (texH << 5) : 0;
        vtx[i].v.cn[0] = vtx[i].v.cn[1] = vtx[i].v.cn[2] = 255;
        vtx[i].v.cn[3] = alpha;
    }

    gSPVertex(gfx++, vtx, 4, 0);
    gSP1Quadrangle(gfx++, 0, 2, 3, 1, 0);
    *gfxp = gfx;
}

static void MapCollect_DrawText(Gfx** gfxp, Vtx** vtxp, bool mirrored, s16 x, s16 y, s16 got, s16 total, u8 r, u8 g,
                                u8 b, u8 alpha) {
    char text[8];
    s32 pass;
    s32 i;

    MapCollect_Format(text, got, total);

    // A dark shadow first, one pixel down and to the right, then the text, like the game's own counters.
    for (pass = 0; pass < 2; pass++) {
        Gfx* gfx = *gfxp;
        s16 cx = x + (pass == 0 ? 1 : 0);
        s16 cy = y - (pass == 0 ? 1 : 0);

        gDPPipeSync(gfx++);
        if (pass == 0) {
            gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, alpha);
        } else {
            gDPSetPrimColor(gfx++, 0, 0, r, g, b, alpha);
        }
        *gfxp = gfx;

        for (i = 0; text[i] != '\0'; i++) {
            const char* tex = (text[i] == '/') ? sMapCollectSlashTex : sMapCollectDigitTexs[text[i] - '0'];

            gfx = *gfxp;
            gDPLoadTextureBlock_4b(gfx++, tex, G_IM_FMT_I, 16, 16, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                   G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            *gfxp = gfx;
            MapCollect_Quad(gfxp, *vtxp, mirrored, cx, cy, MC_GLYPH, MC_GLYPH, 16, 16, alpha);
            *vtxp += 4;
            cx += MapCollect_CharAdvance(text[i]);
        }
    }
}

// Draws the panel of one region next to its dot and returns the display list pointer after it.
static Gfx* MapCollect_DrawPanel(Gfx* gfx, Vtx* vtx, PauseContext* pauseCtx, s32 point, const MapCollectCounts* c,
                                 bool mirrored) {
    bool complete = MapCollect_IsComplete(c);
    s16 rows = (c->skullTotal > 0 ? 1 : 0) + (c->heartTotal > 0 ? 1 : 0);
    if (rows == 0) {
        return gfx;
    }

    s16 textW = 0;
    if (c->skullTotal > 0) {
        textW = MAX(textW, MapCollect_TextWidth(c->skullGot, c->skullTotal));
    }
    if (c->heartTotal > 0) {
        textW = MAX(textW, MapCollect_TextWidth(c->heartGot, c->heartTotal));
    }
    s16 panelW = MC_PAD + MC_ICON + 2 + textW + MC_PAD + (complete ? 14 : 0);
    s16 panelH = MC_PAD + rows * MC_ROW_H + MC_PAD - 1;

    // Next to the dot: on its right, or on its left when there is no room before the right edge of the map.
    Vtx* dot = &pauseCtx->mapPageVtx[124 + point * 4];
    s16 dotLeft = mirrored ? -(dot->v.ob[0] + 8) : dot->v.ob[0];
    s16 dotTop = dot->v.ob[1];
    s16 px = dotLeft + 8 + 4;
    if (px + panelW > 106) {
        px = dotLeft - 4 - panelW;
    }
    px = CLAMP(px, -106, 106 - panelW);
    s16 py = dotTop - 4 + panelH / 2;
    py = CLAMP(py, -68 + panelH + pauseCtx->offsetY, 56 + pauseCtx->offsetY);

    u8 alpha = pauseCtx->alpha;

    // Dark rounded-looking background: a bigger soft box and a smaller solid one.
    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, alpha * 150 / 255);
    MapCollect_Quad(&gfx, vtx, mirrored, px, py, panelW, panelH, 0, 0, alpha);
    vtx += 4;
    if (complete) {
        gDPPipeSync(gfx++);
        gDPSetPrimColor(gfx++, 0, 0, 255, 200, 50, alpha * 90 / 255);
        MapCollect_Quad(&gfx, vtx, mirrored, px + 1, py - 1, panelW - 2, panelH - 2, 0, 0, alpha);
        vtx += 4;
    }

    s16 rowY = py - MC_PAD;
    s16 iconX = px + MC_PAD;
    s16 textX = iconX + MC_ICON + 2;

    if (c->skullTotal > 0) {
        bool done = c->skullGot == c->skullTotal;

        gDPPipeSync(gfx++);
        gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, alpha);
        gDPLoadTextureBlock(gfx++, sMapCollectSkullTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 24, 24, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        MapCollect_Quad(&gfx, vtx, mirrored, iconX, rowY, MC_ICON, MC_ICON, 24, 24, alpha);
        vtx += 4;

        gDPPipeSync(gfx++);
        gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        MapCollect_DrawText(&gfx, &vtx, mirrored, textX, rowY - 1, c->skullGot, c->skullTotal, 255,
                            done ? 210 : 255, done ? 60 : 255, alpha);
        rowY -= MC_ROW_H;
    }

    if (c->heartTotal > 0) {
        bool done = c->heartGot == c->heartTotal;

        // Same colors as the hearts of the life meter.
        gDPPipeSync(gfx++);
        gDPSetCombineLERP(gfx++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                          ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetPrimColor(gfx++, 0, 0, 255, 70, 50, alpha);
        gDPSetEnvColor(gfx++, 50, 40, 60, 255);
        gDPLoadTextureBlock(gfx++, sMapCollectHeartTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 16, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        MapCollect_Quad(&gfx, vtx, mirrored, iconX, rowY, MC_ICON, MC_ICON, 16, 16, alpha);
        vtx += 4;

        gDPPipeSync(gfx++);
        gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        MapCollect_DrawText(&gfx, &vtx, mirrored, textX, rowY - 1, c->heartGot, c->heartTotal, 255,
                            done ? 210 : 255, done ? 60 : 255, alpha);
    }

    if (complete) {
        // Link's head from the dungeon map, blinking slowly at the right end of the panel.
        s32 phase = sMapCollectFrames % 40;
        s32 blink = (phase < 20) ? phase : (40 - phase);
        u8 headAlpha = (u8)(alpha * (110 + blink * 145 / 20) / 255);

        gDPPipeSync(gfx++);
        gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, headAlpha);
        gDPLoadTextureBlock(gfx++, sMapCollectLinkHeadTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        MapCollect_Quad(&gfx, vtx, mirrored, px + panelW - MC_PAD - 12, py - (panelH - 12) / 2, 12, 12, 16, 16,
                        headAlpha);
        vtx += 4;
    }

    gDPPipeSync(gfx++);
    return gfx;
}

// Called by KaleidoScope_DrawWorldMap after the map, its dots and the cursor, with the page matrix still loaded.
void KaleidoScope_DrawWorldMapCollectibles(PlayState* play, GraphicsContext* gfxCtx) {
    PauseContext* pauseCtx = &play->pauseCtx;
    bool mirrored = CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) != 0;
    MapCollectCounts counts[WORLD_MAP_POINT_COUNT];
    s32 point = pauseCtx->cursorPoint[PAUSE_WORLD_MAP];
    s32 i;

    if (!CVarGetInteger(CVAR_MAP_COLLECTIBLES, 1)) {
        return;
    }

    sMapCollectFrames++;

    for (i = 0; i < WORLD_MAP_POINT_COUNT; i++) {
        MapCollect_Count(play, i, &counts[i]);
    }

    // Up to: 2 background boxes, 2 icons, 2 rows x 2 passes x 5 glyphs and the Link head.
    Vtx* vtx = Graph_Alloc(gfxCtx, (2 + 2 + 20 + 1) * 4 * sizeof(Vtx));

    OPEN_DISPS(gfxCtx);

    // Golden dots for the regions with everything collected, drawn over the normal dots.
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 215, 60, pauseCtx->alpha);
    gDPSetEnvColor(POLY_OPA_DISP++, 140, 80, 0, 0);
    gDPLoadTextureBlock(POLY_OPA_DISP++, sMapCollectDotTex, G_IM_FMT_IA, G_IM_SIZ_8b, 8, 8, 0,
                        G_TX_WRAP | G_TX_NOMIRROR, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    for (i = 0; i < WORLD_MAP_POINT_COUNT; i++) {
        if ((pauseCtx->worldMapPoints[i] != 0) && MapCollect_IsComplete(&counts[i])) {
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->mapPageVtx[124 + i * 4], 4, 0);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
        }
    }

    // The panel only shows while the cursor is on a region of the map.
    if ((pauseCtx->cursorSpecialPos == 0) && (point >= 0) && (point < WORLD_MAP_POINT_COUNT) &&
        (pauseCtx->worldMapPoints[point] != 0)) {
        POLY_OPA_DISP = MapCollect_DrawPanel(POLY_OPA_DISP, vtx, pauseCtx, point, &counts[point], mirrored);
    }

    CLOSE_DISPS(gfxCtx);
}
