// OOT D.E.: QuestMenu, the "Diário de Quests" of the pause menu.
//
// The module is split in four parts:
//   questmenu_data.c   data: the quest tables, the checks against the save and the states of the quests;
//   questmenu_state.c  state: opening and closing, current list and line, controller, keyboard and touch input;
//   questmenu_gfx.c    rendering: shapes, textures and text drawn with the game's own display lists;
//   questmenu_ui.c     interface: the layout of the journal and what each part of it does when touched.
// The pause menu calls it through the KaleidoScope_* functions declared in z_kaleido_scope.h.
#ifndef QUESTMENU_H
#define QUESTMENU_H

#include "../z_kaleido_scope.h"

// ---------------------------------------------------------------------------------------------------------------------
// Data

// How a quest (or one of its objectives) is checked against the save. Nothing is ever marked done without one.
typedef enum {
    QM_CHK_NONE,          // not saved by the game: never shown as done
    QM_CHK_HEART_PIECE,   // index into the world map counter table (z_kaleido_map_collect.c)
    QM_CHK_COLLECT,       // collectible flag of a scene
    QM_CHK_ITEMGETINF,
    QM_CHK_INFTABLE,
    QM_CHK_EVENTCHKINF,
    QM_CHK_POE_POINTS,    // Poe Collector bottle
    QM_CHK_CHILD_TRADE,   // child trading slot holds this item or a later one
    QM_CHK_ADULT_TRADE,   // adult trading slot holds this item or a later one
    QM_CHK_BIGGORON,      // Biggoron's Sword
    QM_CHK_GS_TOKENS,     // at least this many Gold Skulltula tokens
    QM_CHK_QUEST_ITEM,    // quest status item (Spiritual Stones, medallions, songs, Gerudo card...)
    QM_CHK_ITEM,          // inventory slot of this item holds it
    QM_CHK_MASTER_SWORD,
    QM_CHK_MEDALLIONS,    // all six medallions
    QM_CHK_EQUIP,         // flag = (equipment type << 4) | equipment index
    QM_CHK_UPGRADE,       // flag = (upgrade type << 4) | minimum level
    QM_CHK_MAGIC,         // magic meter
    QM_CHK_AREA,          // flag = world map area visited (the clouds of the pause map)
    QM_CHK_VISITED,       // scene visited (rooms of its dungeon map)
    QM_CHK_BOSS_KEY,      // boss key of a dungeon (scene)
    QM_CHK_GANON,         // Ganon defeated: Ship of Harkinian's own record of it, or the player's mark (see below)
} QuestMenuCheckType;

#define QM_EQUIP(type, inv) (((type) << 4) | (inv))
#define QM_UPGRADE(upg, level) (((upg) << 4) | (level))

typedef enum {
    QM_AGE_ANY,
    QM_AGE_CHILD,
    QM_AGE_ADULT,
} QuestMenuAge;

// What a quest needs, shown as its objectives.
#define QM_NEED_BEAN (1 << 0)
#define QM_NEED_HOOKSHOT (1 << 1)
#define QM_NEED_LONGSHOT (1 << 2)
#define QM_NEED_BOMB (1 << 3)
#define QM_NEED_BOW (1 << 4)
#define QM_NEED_IRON_BOOTS (1 << 5)
#define QM_NEED_HOVER_BOOTS (1 << 6)
#define QM_NEED_SILVER_SCALE (1 << 7)
#define QM_NEED_GOLD_SCALE (1 << 8)
#define QM_NEED_LENS (1 << 9)
#define QM_NEED_OCARINA (1 << 10)
#define QM_NEED_BOOMERANG (1 << 11)
#define QM_NEED_ZORA_TUNIC (1 << 12)
#define QM_NEED_BLUE_FIRE (1 << 13)
#define QM_NEED_DEKU_STICK (1 << 14)
#define QM_NEED_NIGHT (1 << 15) // moon: only at night
#define QM_NEED_DAY (1 << 16)   // sun: only by day
#define QM_NEED_COUNT 17

// One objective of a quest.
typedef struct {
    const char* label; // UTF-8
    const char* icon;  // 32x32 RGBA32 item icon, or 24x24 when iconIs24
    u8 iconIs24;
    u8 check;
    s16 scene;
    u16 flag;
} QuestMenuObjective;

// One quest. To add a quest, add a line to one of the tables of questmenu_data.c (and to its parallel tables).
typedef struct {
    const char* name;   // UTF-8
    const char* detail; // UTF-8
    const char* icon;   // 32x32 RGBA32 item icon, or 24x24 when iconIs24
    u8 iconIs24;
    u8 age;
    u32 needs;
    u8 check;
    s16 scene;
    u16 flag;
    const QuestMenuObjective* objectives; // optional, ends with a NULL label
} QuestMenuEntry;

// World map points (the dots of the pause world map), used for the map of the journal card.
typedef enum {
    QM_PT_NONE = -1,
    QM_PT_WASTELAND,
    QM_PT_FORTRESS,
    QM_PT_VALLEY,
    QM_PT_LAKE,
    QM_PT_LON_LON,
    QM_PT_MARKET,
    QM_PT_FIELD,
    QM_PT_DEATH_MOUNTAIN,
    QM_PT_KAKARIKO,
    QM_PT_LOST_WOODS,
    QM_PT_KOKIRI,
    QM_PT_ZORA,
    QM_PT_COUNT,
} QuestMenuPoint;

// A titled group of lines, starting at line `start` of a table.
typedef struct {
    s16 start;
    u8 style; // 0: golden, with the winged Triforce; 1: blue, with the "!"
    const char* title;
} QuestMenuSection;

typedef enum {
    QM_CAT_MAIN,    // Missões principais
    QM_CAT_SIDE,    // Missões secundárias
    QM_CAT_PEOPLE,  // Personagens e NPCs
    QM_CAT_PLACES,  // Locais descobertos
    QM_CAT_ITEMS,   // Itens importantes
    QM_CAT_RECORDS, // Registros e documentos
    QM_CAT_COUNT,
} QuestMenuCategory;

typedef enum {
    QM_STATE_LOCKED,      // an earlier step of the same story is not done yet
    QM_STATE_AVAILABLE,
    QM_STATE_IN_PROGRESS, // part of it is done
    QM_STATE_DONE,
} QuestMenuState;

// One line of the current list, built every frame from the tables.
typedef struct {
    u16 id; // unique: (category + 1) * 1000 + line + 1
    const char* name;
    const char* detail;
    const char* icon;
    u8 iconIs24;
    u8 age;
    u32 needs;
    u8 done;
    u8 state;
    u8 hasCount;
    s16 got;
    s16 total;
    s8 point;            // world map point, or QM_PT_NONE
    u8 sectionStyle;
    const char* section; // title of the group the line belongs to
    const QuestMenuEntry* entry;
} QuestMenuRow;

// An objective of the selected line, checked.
typedef struct {
    const char* label;
    const char* icon;
    u8 iconIs24;
    u8 done;
} QuestMenuGoal;

#define QM_MAX_ROWS 128
#define QM_MAX_GOALS 8

extern const u8 gQuestMenuPointPos[QM_PT_COUNT][2];
extern const char* gQuestMenuPointNames[QM_PT_COUNT];
extern const char* gQuestMenuCategoryNames[QM_CAT_COUNT];
extern const char* gQuestMenuCategoryHints[QM_CAT_COUNT];
extern const char* gQuestMenuCategoryIcons[QM_CAT_COUNT];

s32 QuestMenu_BuildRows(PlayState* play, s32 category, QuestMenuRow* rows, s32 max);
s32 QuestMenu_GetGoals(PlayState* play, const QuestMenuRow* row, QuestMenuGoal* goals, s32 max);
const char* QuestMenu_StateName(u8 state);

// Beating Ganon ends the game with the credits, so the save normally never records it. The line can be marked done by
// the player (A, or touching the card). The mark is kept in the settings for this save file only, never in the save.
bool QuestMenu_IsManualRow(const QuestMenuRow* row);
bool QuestMenu_GanonDetected(void);
bool QuestMenu_GetGanonMark(void);
bool QuestMenu_CanMarkGanon(void);
void QuestMenu_SetGanonMark(bool done);
bool QuestMenu_IsDungeonIcon(const char* icon);

// ---------------------------------------------------------------------------------------------------------------------
// State

// What a part of the journal does when it is touched or clicked.
typedef enum {
    QM_HIT_CATEGORY, // value: category
    QM_HIT_ROW,      // value: line
    QM_HIT_PREV_CATEGORY,
    QM_HIT_NEXT_CATEGORY,
    QM_HIT_SCROLL_UP,
    QM_HIT_SCROLL_DOWN,
    QM_HIT_LIST,     // the list area, for dragging
    QM_HIT_CLOSE,
    QM_HIT_MANUAL_DONE, // mark / unmark a line the save does not record (Ganondorf)
} QuestMenuHitType;

typedef struct {
    f32 x0, y0, x1, y1; // local units
    u8 type;
    s16 value;
} QuestMenuHit;

#define QM_MAX_HITS 64

typedef struct {
    s32 category;
    s32 cursor[QM_CAT_COUNT];
    s32 top[QM_CAT_COUNT]; // first line of the list on screen
    bool freeScroll;       // the list was scrolled by touch or wheel: do not bring the cursor back into view
    s32 frames;
    s16 anim;              // 0 (closed) .. QM_OPEN_FRAMES (open)
    QuestMenuRow rows[QM_MAX_ROWS];
    s32 rowCount;
    s16 got;               // done lines of the current list
    s16 total;
    // Transform of the last frame, local units -> screen: screen = center + (local - canvas center) * scale.
    f32 scale;
    // Touch / mouse areas of the last frame.
    QuestMenuHit hits[QM_MAX_HITS];
    s32 hitCount;
} QuestMenuContext;

#define QM_OPEN_FRAMES 8

// Lines of the list, in local units.
#define QM_HEADER_H 26
#define QM_ROW_H 24

extern QuestMenuContext gQuestMenu;

// Opens/closes, reads the input and rebuilds the lines. Returns false when the journal is not shown.
bool QuestMenu_Update(PlayState* play, f32* openAmount);
void QuestMenu_AddHit(f32 x0, f32 y0, f32 x1, f32 y1, u8 type, s16 value);

// ---------------------------------------------------------------------------------------------------------------------
// Rendering (questmenu_gfx.c)

// The journal canvas: 854 x 480 local units (half a pixel of a 427 x 240 screen), centered and scaled on the screen.
#define QM_W 854
#define QM_H 480

// Button glyphs of the game font, written in the texts as control characters.
#define QM_GLYPH_CUP "\x01"
#define QM_GLYPH_CLEFT "\x03"
#define QM_GLYPH_CRIGHT "\x04"
#define QM_GLYPH_B "\x05"
#define QM_GLYPH_L "\x06"
#define QM_GLYPH_R "\x07"
#define QM_GLYPH_A "\x08"

typedef struct {
    Gfx* gfx;
    Gfx* gfxEnd;
    Vtx* vtx;
    Vtx* vtxEnd;
    u8 alpha;
    u8 combine; // 0 = none set, 1 = textured, 2 = solid, 3 = vertex colors
} QuestMenuDraw;

s32 QM_Encode(const char* text, u8* out, s32 max);
bool QM_HasRoom(QuestMenuDraw* d);
void QM_Quad(QuestMenuDraw* d, s16 x, s16 y, s16 w, s16 h, s16 texW, s16 texH);
void QM_SetColor(QuestMenuDraw* d, u8 r, u8 g, u8 b, u8 a);
void QM_Combine(QuestMenuDraw* d, u8 combine);
void QM_Rect(QuestMenuDraw* d, s16 x, s16 y, s16 w, s16 h, u8 r, u8 g, u8 b, u8 a);
void QM_Tex(QuestMenuDraw* d, const char* tex, u8 fmt, u8 siz, s16 texW, s16 texH, s16 x, s16 y, s16 w, s16 h, bool lit, u8 alpha);
void QM_Icon(QuestMenuDraw* d, const char* tex, bool is24, s16 x, s16 y, s16 size, bool lit);
f32 QM_CharAdvance(u8 c, s16 size);
f32 QM_TextWidth(const u8* text, s32 len, s16 size);
f32 QM_DrawCodes(QuestMenuDraw* d, f32 x, s16 y, const u8* text, s32 len, s16 size, u8 r, u8 g, u8 b, u8 a, f32 maxX);
f32 QM_DrawText(QuestMenuDraw* d, f32 x, s16 y, const char* text, s16 size, u8 r, u8 g, u8 b, u8 a, f32 maxX);
void QM_DrawWrapped(QuestMenuDraw* d, s16 x, s16 y, s16 width, const char* text, s16 size, s16 lineH, s32 maxLines, u8 r, u8 g, u8 b, u8 a);
void QM_SetVtx(Vtx* v, f32 x, f32 y, u8 r, u8 g, u8 b, u8 a);
void QM_LoadTile(QuestMenuDraw* d, const char* tex);
void QM_Line(QuestMenuDraw* d, f32 x0, f32 y0, f32 x1, f32 y1, f32 width, u8 r, u8 g, u8 b, u8 a);
void QM_Gradient(QuestMenuDraw* d, f32 x, f32 y, f32 w, f32 h, const u8* c0, const u8* c1, bool vertical);
void QM_Circle(QuestMenuDraw* d, f32 cx, f32 cy, f32 radius, u8 r, u8 g, u8 b, u8 a);
void QM_TriforceColor(QuestMenuDraw* d, f32 x, f32 y, f32 size, u8 r, u8 g, u8 b, u8 a);
void QM_GoldTriforce(QuestMenuDraw* d, f32 x, f32 y, f32 size, bool lit);
void QM_WingedTriforce(QuestMenuDraw* d, f32 cx, f32 y, f32 size);
f32 QM_StrWidth(const char* text, s16 size);
f32 QM_Print(QuestMenuDraw* d, f32 x, f32 y, const char* text, s16 size, u8 r, u8 g, u8 b, bool shadow, f32 maxX);
void QM_PrintCentered(QuestMenuDraw* d, f32 cx, f32 y, const char* text, s16 size, u8 r, u8 g, u8 b, bool shadow);
void QM_MapCrop(QuestMenuDraw* d, s16 x, s16 y, s16 texW, s16 texH, s16 u0, s16 v0, s16 scale);
void QM_Begin(PlayState* play, QuestMenuDraw* d, f32 k, u8 alpha);
void QM_End(PlayState* play, QuestMenuDraw* d);
void QM_IconTinted(QuestMenuDraw* d, const char* tex, bool is24, s16 x, s16 y, s16 size, bool lit, u8 alpha);
void QM_Fan(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, bool textured, u8 r, u8 g, u8 b, u8 a);
void QM_Outline(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, f32 width, u8 r, u8 g, u8 b, u8 a);
s32 QM_Chamfer(f32 (*pts)[2], f32 x0, f32 y0, f32 x1, f32 y1, f32 c);
void QM_Stone(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, u8 r, u8 g, u8 b, f32 bevel);
void QM_Recess(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, u8 r, u8 g, u8 b);
void QM_Glow(QuestMenuDraw* d, const f32 (*pts)[2], s32 n, f32 pulse);

// ---------------------------------------------------------------------------------------------------------------------
// Interface

void QuestMenu_DrawJournal(PlayState* play, f32 openAmount);

#endif
