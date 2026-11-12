// OOT D.E.: main menu of the Definitive Edition interface (screen 1), over the title screen. See DefinitiveUI.h.
//
// Split in three parts:
//   - the model (this file, game thread): items, navigation with the controller, what each item does;
//   - the view (DeTitleMenuView.cpp): drawing and touch areas, fed by the model every frame;
//   - the resources (fonts and textures of soh.otr), loaded here when the menu needs them and released after.
// The menu never changes the save or the game state by itself: it hands each choice to the game's own code (file
// select, pause menu, port menu).
#include "soh/frame_interpolation.h"
#include "DefinitiveUI.h"
#include "DeTheme.h"
#include "DeTitleMenuText.h"
#include "DeTitleMenuView.h"
#include "DeMusic.h"

#include <libultraship/libultraship.h>
#include <Window.h>
#include <spdlog/spdlog.h>
#include <Fast3D/gfx_pc.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "soh/OTRGlobals.h"
#include "soh/SaveManager.h"
#include "soh/ResourceManagerHelpers.h"
#include <resource/ResourceType.h>
#include "window/gui/resource/Font.h"
#include "soh/cvar_prefixes.h"
#include "soh/SohGui/ModsMenu.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

// SaveManager.h only declares these for C files.
extern "C" SaveFileMetaInfo* Save_GetSaveMetaInfo(int fileNum);
extern "C" void Save_CopyFile(int from, int to);
extern "C" void Save_DeleteFile(int fileNum);

#ifndef OOTDE_VERSION
#define OOTDE_VERSION "dev"
#endif

#define CVAR_TITLE_MENU CVAR_SETTING("DefinitiveUI.TitleMenu")
#define CVAR_LAST_FILE CVAR_SETTING("DefinitiveUI.LastFile")

namespace DefinitiveUI {

// ---------------------------------------------------------------------------------------------------------------------
// Resources

static const char* const kTexturePaths[TEX_COUNT] = {
    "ootde/ui/textures/gDEStone",       "ootde/ui/effects/gDEGlow",          "ootde/ui/effects/gDESpark",
    "ootde/ui/decor/gDENaviWing",       "ootde/ui/backgrounds/gDETitleBg",   "ootde/ui/decor/gDELogo",
    "ootde/ui/buttons/gDEButtonNormal", "ootde/ui/buttons/gDEButtonSelected", "ootde/ui/buttons/gDEButtonPressed",
    "ootde/ui/frames/gDEFrame",
};
static const char* const kTextureNames[TEX_COUNT] = {
    "OOTDE_Stone",        "OOTDE_Glow",         "OOTDE_Spark",         "OOTDE_NaviWing", "OOTDE_TitleBg",
    "OOTDE_Logo",         "OOTDE_ButtonNormal", "OOTDE_ButtonSelected", "OOTDE_ButtonPressed", "OOTDE_Frame",
};
static bool sTexturesLoaded = false;
static bool sTextureAvailable[TEX_COUNT] = {};

static ImTextureID LookupTexture(Texture tex) {
    if (!sTexturesLoaded || !sTextureAvailable[tex]) {
        return (ImTextureID)0;
    }
    return Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(kTextureNames[tex]);
}

static void LoadTextures() {
    if (sTexturesLoaded) {
        return;
    }
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    auto resources = Ship::Context::GetInstance()->GetResourceManager();
    for (int i = 0; i < TEX_COUNT; i++) {
        // A soh.otr without the texture (an older one) must not crash: the shapes fall back to plain geometry.
        auto res = std::static_pointer_cast<Fast::Texture>(resources->LoadResource(kTexturePaths[i], true));
        sTextureAvailable[i] = res != nullptr;
        if (res != nullptr) {
            gui->LoadGuiTexture(kTextureNames[i], *res, ImVec4(1, 1, 1, 1));
        } else {
            SPDLOG_WARN("OOT D.E.: texture {} missing from soh.otr", kTexturePaths[i]);
        }
    }
    SetTextureLookup(LookupTexture);
    sTexturesLoaded = true;
}

static void UnloadTextures() {
    if (!sTexturesLoaded) {
        return;
    }
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    for (int i = 0; i < TEX_COUNT; i++) {
        if (sTextureAvailable[i]) {
            gui->UnloadTexture(kTextureNames[i]);
        }
        sTextureAvailable[i] = false;
    }
    sTexturesLoaded = false;
}

static std::shared_ptr<Ship::Font> sFontData[3];

static ImFont* AddFont(int slot, const char* path, float size) {
    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(RESOURCE_TYPE_FONT);
    initData->ResourceVersion = 0;
    initData->Path = path;
    sFontData[slot] = std::static_pointer_cast<Ship::Font>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, false, initData));
    if (sFontData[slot] == nullptr) {
        SPDLOG_WARN("OOT D.E.: font {} missing from soh.otr", path);
        return nullptr;
    }
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false; // the resource keeps the data
    cfg.OversampleH = 2;
    cfg.OversampleV = 1;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(sFontData[slot]->Data, (int)sFontData[slot]->DataSize, size,
                                                      &cfg);
}

void LoadFonts() {
    Fonts& f = GetFonts();
    // One size each; the menu scales them to the screen. Latin-1 covers every Portuguese letter.
    f.display = AddFont(0, "fonts/Cinzel-Bold.ttf", 48.0f);
    f.displayRegular = AddFont(1, "fonts/Cinzel-Regular.ttf", 28.0f);
    f.body = AddFont(2, "fonts/EBGaramond-Medium.ttf", 36.0f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Model

// In the order the player goes deeper (a screen with a higher number slides in from the right).
enum Screen {
    SCREEN_MAIN,
    SCREEN_FEATURES,      // Definitive Features, first page
    SCREEN_FEATURES_MORE, // Definitive Features, second page
    SCREEN_EXTRAS,
    SCREEN_FILES,  // the three files
    SCREEN_FILE,   // one file: play, copy, erase
    SCREEN_ERASE,  // erase it?
    SCREEN_COPY,   // copy it where?
    SCREEN_QUEST,  // new game: which adventure
    SCREEN_NAME,   // new game: the hero's name
    SCREEN_COUNT,
};

enum Action {
    ACT_NONE,
    ACT_CONTINUE,
    ACT_NEW_GAME,
    ACT_FILES,
    ACT_JOURNAL,
    ACT_MODS,
    ACT_SETTINGS,
    ACT_EXTRAS,
    ACT_VERSION,
    ACT_BACK,
    ACT_OPEN_FILE, // arg: file
    ACT_PLAY,
    ACT_COPY,
    ACT_ERASE,
    ACT_ERASE_YES,
    ACT_COPY_TO, // arg: file
    ACT_QUEST,   // arg: quest
    ACT_CLASSIC, // the game's own file select screen
    ACT_FEATURES,
    ACT_FEATURES_MORE,
    ACT_FEATURE, // arg: TitleFeatureItem
};

struct Item {
    char label[64];
    char detail[320];
    bool enabled;
    Action action;
    int arg;
};

static constexpr int kMaxItems = 8;
static constexpr int kNameMax = 8;

static bool sActive = false;   // the title screen is there and the menu is on it
static bool sClosing = false;  // leaving towards file select
static bool sDetached = false; // the title screen is restarting its opening: the menu stays as it is meanwhile
static float sDetachedTime = 0.0f;
static float sAlpha = 0.0f; // drawn opacity
static Screen sScreen = SCREEN_MAIN;
static int sCursor[SCREEN_COUNT] = {};
static int sPressed = -1;
static int sPressTimer = 0;
static bool sBgmMuted = false; // the game's title music is muted while the menu music plays
static int sOpenSfxTimer = -1; // frames until the sound of the panel sliding in (opening animation)
static int sPendingTouch = -1; // item tapped in the view, handled at the next update
static int sPendingKey = -1;   // key tapped in the view
static int sInputCooldown = 0; // frames to ignore the controller (after the port menu closes)
static int sHoldDirX = 0, sHoldDirY = 0;
static int sHoldFrames = 0;
static bool sPortMenuWasOpen = false;
#ifdef __ANDROID__
static bool sTouchMode = true;
#else
static bool sTouchMode = false;
#endif

// The file the Arquivos screens are about, and the new game being prepared.
static int sFile = 0;
static Screen sNewFrom = SCREEN_MAIN; // where Novo Jogo was chosen (to go back there)
static int sNewQuest = QUEST_NORMAL;
static char sName[kNameMax + 1] = "";
static bool sNameLower = false;
static KeyCell sKeys[KEY_COUNT];
static char sKeyLabels[KEY_CHARS][2];
static int sKeyCursor = 0;

// What file select must do for the menu, underneath the cover.
static int sFileRequest = -1; // file to play
static bool sAutoConfirm = false;
static int sNewRequest = -1; // file to create (with sName and sNewQuest)
static bool sCover = false;  // the "loading" screen is up and file select is working for the menu
static float sCoverTime = 0.0f;
static float sCoverAlpha = 0.0f;
static const char* sCoverText = Strings::kLoading;
static int sJournalWait = 0; // frames before opening the journal; 0 = no request
static int sJournalTimeout = 0;

// The image shown before the menu when the game opens.
static float sSplashTime = -1.0f; // seconds since it appeared; negative: not started
static float sSplashOut = -1.0f;  // when it started to fade out; negative: still holding
static bool sSplashDone = false;
static bool sSplashSkip = false;

static Item sItems[kMaxItems];
static TitleMenuItem sViewItems[kMaxItems];
static int sItemCount = 0;
static char sTitle[64];
static char sFooter[96];
static int sModsEnabled = 0;
static int sModsTotal = 0;

static void PlaySfx(u16 id) {
    Audio_PlaySoundGeneral(id, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultReverb);
}

static bool PortMenuOpen() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    return gui->GetMenuOrMenubarVisible();
}

static bool SplashOn() {
    return sSplashTime >= 0.0f && !sSplashDone;
}

static bool FileValid(int file) {
    return file >= 0 && file < 3 && Save_GetSaveMetaInfo(file)->valid;
}

// The same check the game's file select makes before opening a file.
static bool FileCompatible(int file) {
    const SaveFileMetaInfo* meta = Save_GetSaveMetaInfo(file);
    if (meta->requiresMasterQuest && !ResourceMgr_GameHasMasterQuest()) {
        return false;
    }
    if (meta->requiresOriginal && !ResourceMgr_GameHasOriginal()) {
        return false;
    }
    return true;
}

// The last file played, or the first one that can be played. -1 when there is none.
static int ContinueFile() {
    const int last = CVarGetInteger(CVAR_LAST_FILE, -1);
    if (FileValid(last) && FileCompatible(last)) {
        return last;
    }
    for (int i = 0; i < 3; i++) {
        if (FileValid(i) && FileCompatible(i)) {
            return i;
        }
    }
    return -1;
}

static int FirstFreeFile() {
    for (int i = 0; i < 3; i++) {
        if (!FileValid(i)) {
            return i;
        }
    }
    return -1;
}

// File names are stored in the game's own letter codes, which depend on the version of the game.
static void DecodeName(const SaveFileMetaInfo* meta, char* out, size_t size) {
    size_t n = 0;
    for (int i = 0; i < 8 && n + 1 < size; i++) {
        const u8 c = meta->playerName[i];
        char ch = '\0';
        if (c <= 0x09) {
            ch = '0' + c;
        } else if (meta->filenameLanguage == NAME_LANGUAGE_PAL) {
            if (c <= 0x23) {
                ch = 'A' + (c - 0x0A);
            } else if (c <= 0x3D) {
                ch = 'a' + (c - 0x24);
            } else if (c == 0x3E) {
                ch = ' ';
            } else if (c == 0x3F) {
                ch = '-';
            } else if (c == 0x40) {
                ch = '.';
            }
        } else {
            if (c >= 0xAB && c <= 0xC4) {
                ch = 'A' + (c - 0xAB);
            } else if (c >= 0xC5 && c <= 0xDE) {
                ch = 'a' + (c - 0xC5);
            } else if (c == 0xDF) {
                ch = ' ';
            }
        }
        if (ch != '\0') {
            out[n++] = ch;
        }
    }
    while (n > 0 && out[n - 1] == ' ') {
        n--;
    }
    out[n] = '\0';
    if (n == 0) {
        snprintf(out, size, "Link");
    }
}

// The opposite, for a new file (letters, digits and spaces only), in the codes the game's name entry would use.
static void EncodeName(const char* text, uint8_t* out) {
    const bool pal = ResourceMgr_GetGameRegion(0) == GAME_REGION_PAL && gSaveContext.language != LANGUAGE_JPN;
    const size_t len = strlen(text);
    for (size_t i = 0; i < 8; i++) {
        const char c = i < len ? text[i] : ' ';
        if (c >= '0' && c <= '9') {
            out[i] = (uint8_t)(c - '0');
        } else if (c >= 'A' && c <= 'Z') {
            out[i] = (uint8_t)((pal ? 0x0A : 0xAB) + (c - 'A'));
        } else if (c >= 'a' && c <= 'z') {
            out[i] = (uint8_t)((pal ? 0x24 : 0xC5) + (c - 'a'));
        } else {
            out[i] = pal ? 0x3E : 0xDF;
        }
    }
}

static const char* QuestName(const SaveFileMetaInfo* meta) {
    if (meta->randoSave) {
        return Strings::kQuestRando;
    }
    return meta->requiresMasterQuest ? Strings::kQuestMaster : Strings::kQuestNormal;
}

static void CountMods() {
    sModsEnabled = sModsTotal = 0;
    for (const auto& mod : ModsMenu::List()) {
        sModsTotal++;
        sModsEnabled += mod.enabled ? 1 : 0;
    }
}

static Item& AddItem(const char* label, const char* detail, Action action, int arg = 0, bool enabled = true) {
    Item& item = sItems[sItemCount < kMaxItems ? sItemCount++ : kMaxItems - 1];
    snprintf(item.label, sizeof(item.label), "%s", label);
    snprintf(item.detail, sizeof(item.detail), "%s", detail != nullptr ? detail : "");
    item.enabled = enabled;
    item.action = action;
    item.arg = arg;
    return item;
}

// The list of the current screen, rebuilt every frame from the real state of the saves.
static void BuildItems() {
    char name[16];
    char text[320];
    sItemCount = 0;
    sTitle[0] = '\0';
    switch (sScreen) {
        case SCREEN_MAIN: {
            const int cont = ContinueFile();
            const int free = FirstFreeFile();
            AddItem(Strings::kMainLabels[MAIN_CONTINUE], cont >= 0 ? Strings::kContinue : Strings::kContinueNone,
                    ACT_CONTINUE, 0, cont >= 0);
            if (free >= 0) {
                snprintf(text, sizeof(text), Strings::kNewGame, free + 1);
                AddItem(Strings::kMainLabels[MAIN_NEW_GAME], text, ACT_NEW_GAME);
            } else {
                AddItem(Strings::kMainLabels[MAIN_NEW_GAME], Strings::kNewGameFull, ACT_NEW_GAME, 0, false);
            }
            AddItem(Strings::kMainLabels[MAIN_FILES], Strings::kFiles, ACT_FILES);
            AddItem(Strings::kMainLabels[MAIN_JOURNAL], cont >= 0 ? Strings::kJournal : Strings::kJournalNone,
                    ACT_JOURNAL, 0, cont >= 0);
            AddItem(Strings::kMainLabels[MAIN_FEATURES], Strings::kFeatures, ACT_FEATURES);
            AddItem(Strings::kMainLabels[MAIN_MODS], Strings::kMods, ACT_MODS);
            AddItem(Strings::kMainLabels[MAIN_SETTINGS], Strings::kSettings, ACT_SETTINGS);
            AddItem(Strings::kMainLabels[MAIN_EXTRAS], Strings::kExtras, ACT_EXTRAS);
            break;
        }
        case SCREEN_FEATURES:
            snprintf(sTitle, sizeof(sTitle), "%s", Strings::kFeaturesTitle);
            for (int i = 0; i < Strings::kFeaturesFirstPage; i++) {
                AddItem(Strings::kFeatureLabels[i], Strings::kFeatureDetails[i], ACT_FEATURE, i);
            }
            AddItem(Strings::kFeaturesMore, Strings::kFeaturesMoreDetail, ACT_FEATURES_MORE);
            AddItem(Strings::kBack, Strings::kExtrasBack, ACT_BACK);
            break;
        case SCREEN_FEATURES_MORE:
            snprintf(sTitle, sizeof(sTitle), "%s", Strings::kFeaturesMoreTitle);
            for (int i = Strings::kFeaturesFirstPage; i < FEATURE_COUNT; i++) {
                AddItem(Strings::kFeatureLabels[i], Strings::kFeatureDetails[i], ACT_FEATURE, i);
            }
            AddItem(Strings::kBack, Strings::kBackDetail, ACT_BACK);
            break;
        case SCREEN_EXTRAS:
            snprintf(sTitle, sizeof(sTitle), "%s", Strings::kExtrasTitle);
            snprintf(text, sizeof(text), Strings::kExtrasMods, sModsEnabled, sModsTotal);
            AddItem(Strings::kExtrasLabels[EXTRAS_MODS], text, ACT_MODS);
            snprintf(text, sizeof(text), Strings::kExtrasVersion, OOTDE_VERSION, (char*)gBuildVersion);
            AddItem(Strings::kExtrasLabels[EXTRAS_VERSION], text, ACT_VERSION);
            // The credits screen comes in a later step: shown as coming, not as working.
            AddItem(Strings::kExtrasLabels[EXTRAS_CREDITS], Strings::kExtrasCredits, ACT_NONE, 0, false);
            AddItem(Strings::kExtrasLabels[EXTRAS_BACK], Strings::kExtrasBack, ACT_BACK);
            break;
        case SCREEN_FILES:
            snprintf(sTitle, sizeof(sTitle), "%s", Strings::kFilesTitle);
            for (int i = 0; i < 3; i++) {
                char label[64];
                if (FileValid(i)) {
                    const SaveFileMetaInfo* meta = Save_GetSaveMetaInfo(i);
                    DecodeName(meta, name, sizeof(name));
                    snprintf(label, sizeof(label), Strings::kFileLabel, i + 1, name);
                    if (FileCompatible(i)) {
                        snprintf(text, sizeof(text), Strings::kFileInfo, QuestName(meta), (int)meta->deaths);
                    } else {
                        snprintf(text, sizeof(text), Strings::kFileIncompatible, QuestName(meta));
                    }
                    AddItem(label, text, ACT_OPEN_FILE, i);
                } else {
                    snprintf(label, sizeof(label), Strings::kFileEmptyLabel, i + 1);
                    AddItem(label, Strings::kFileEmpty, ACT_OPEN_FILE, i);
                }
            }
            AddItem(Strings::kBack, Strings::kExtrasBack, ACT_BACK);
            break;
        case SCREEN_FILE: {
            const SaveFileMetaInfo* meta = Save_GetSaveMetaInfo(sFile);
            DecodeName(meta, name, sizeof(name));
            snprintf(sTitle, sizeof(sTitle), Strings::kFileLabel, sFile + 1, name);
            if (FileCompatible(sFile)) {
                AddItem(Strings::kPlay, Strings::kPlayDetail, ACT_PLAY);
            } else {
                snprintf(text, sizeof(text), Strings::kFileIncompatible, QuestName(meta));
                AddItem(Strings::kPlay, text, ACT_PLAY, 0, false);
            }
            const bool room = FirstFreeFile() >= 0;
            AddItem(Strings::kCopy, room ? Strings::kCopyDetail : Strings::kCopyNoRoom, ACT_COPY, 0, room);
            AddItem(Strings::kErase, Strings::kEraseDetail, ACT_ERASE);
            AddItem(Strings::kBack, Strings::kBackDetail, ACT_BACK);
            break;
        }
        case SCREEN_ERASE: {
            DecodeName(Save_GetSaveMetaInfo(sFile), name, sizeof(name));
            snprintf(sTitle, sizeof(sTitle), Strings::kEraseTitle, sFile + 1);
            AddItem(Strings::kEraseNo, Strings::kEraseNoDetail, ACT_BACK);
            snprintf(text, sizeof(text), Strings::kEraseYesDetail, name);
            AddItem(Strings::kEraseYes, text, ACT_ERASE_YES);
            break;
        }
        case SCREEN_COPY: {
            DecodeName(Save_GetSaveMetaInfo(sFile), name, sizeof(name));
            snprintf(sTitle, sizeof(sTitle), "%s", Strings::kCopyTitle);
            for (int i = 0; i < 3; i++) {
                if (!FileValid(i)) {
                    char label[64];
                    snprintf(label, sizeof(label), Strings::kFileEmptyLabel, i + 1);
                    snprintf(text, sizeof(text), Strings::kCopyTo, name, i + 1);
                    AddItem(label, text, ACT_COPY_TO, i);
                }
            }
            AddItem(Strings::kBack, Strings::kBackDetail, ACT_BACK);
            break;
        }
        case SCREEN_QUEST:
            snprintf(sTitle, sizeof(sTitle), Strings::kNewTitle, sFile + 1);
            // Only the adventures the player's own game data has.
            if (ResourceMgr_GameHasOriginal()) {
                AddItem(Strings::kQuestNormal, Strings::kQuestNormalDetail, ACT_QUEST, QUEST_NORMAL);
            }
            if (ResourceMgr_GameHasMasterQuest()) {
                AddItem(Strings::kQuestMaster, Strings::kQuestMasterDetail, ACT_QUEST, QUEST_MASTER);
            }
            AddItem(Strings::kClassic, Strings::kClassicDetail, ACT_CLASSIC);
            AddItem(Strings::kBack, Strings::kBackDetail, ACT_BACK);
            break;
        case SCREEN_NAME:
        default:
            snprintf(sTitle, sizeof(sTitle), "%s", Strings::kNameTitle);
            BuildNameKeys(sNameLower, sKeys, sKeyLabels);
            break;
    }
    for (int i = 0; i < sItemCount; i++) {
        sViewItems[i] = { sItems[i].label, sItems[i].detail, sItems[i].enabled };
    }
    if (sScreen != SCREEN_NAME && sCursor[sScreen] >= sItemCount) {
        sCursor[sScreen] = sItemCount > 0 ? sItemCount - 1 : 0;
    }
}

static void GoTo(Screen screen, int cursor = 0) {
    sScreen = screen;
    sCursor[screen] = cursor;
    sPressed = -1;
    sPendingTouch = -1;
    sPendingKey = -1;
}

static void Back() {
    switch (sScreen) {
        case SCREEN_FEATURES:
            GoTo(SCREEN_MAIN, MAIN_FEATURES);
            break;
        case SCREEN_FEATURES_MORE:
            GoTo(SCREEN_FEATURES, Strings::kFeaturesFirstPage);
            break;
        case SCREEN_EXTRAS:
            GoTo(SCREEN_MAIN, MAIN_EXTRAS);
            break;
        case SCREEN_FILES:
            GoTo(SCREEN_MAIN, MAIN_FILES);
            break;
        case SCREEN_FILE:
            GoTo(SCREEN_FILES, sFile);
            break;
        case SCREEN_ERASE:
            GoTo(SCREEN_FILE, 2);
            break;
        case SCREEN_COPY:
            GoTo(SCREEN_FILE, 1);
            break;
        case SCREEN_QUEST:
            GoTo(sNewFrom, sNewFrom == SCREEN_FILES ? sFile : (int)MAIN_NEW_GAME);
            break;
        case SCREEN_NAME:
            GoTo(SCREEN_QUEST, 0);
            break;
        default:
            return;
    }
    PlaySfx(NA_SE_SY_FSEL_CLOSE);
}

static void OpenPortMenu(const char* sidebar) {
    CVarSetString(CVAR_SETTING("Menu.ActiveHeader"), "Settings");
    CVarSetString(CVAR_SETTING("Menu.SettingsSidebarSection"), sidebar);
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    if (gui->GetMenu() != nullptr) {
        gui->GetMenu()->Show();
        // Same as opening it with its key: let the controller move in it when that option is on.
        if (CVarGetInteger(CVAR_IMGUI_CONTROLLER_NAV, 0)) {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        }
    }
}

// Definitive Features: their pages live in the port menu, header "Definitive" (SohMenuDefinitive.cpp).
static void OpenDefinitivePage(const char* sidebar) {
    CVarSetString(CVAR_SETTING("Menu.ActiveHeader"), "Definitive");
    CVarSetString(CVAR_SETTING("Menu.DefinitiveSidebarSection"), sidebar);
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    if (gui->GetMenu() != nullptr) {
        gui->GetMenu()->Show();
        if (CVarGetInteger(CVAR_IMGUI_CONTROLLER_NAV, 0)) {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        }
    }
}

// The game's title music stays silent while the menu's own music plays or while file select works under the cover.
// It uses a volume scale that nothing else in the game uses.
static constexpr u8 kBgmVolScale = 3;
static void RestoreBgm() {
    Audio_SetVolScale(SEQ_PLAYER_BGM_MAIN, kBgmVolScale, 0x7F, 0);
    sBgmMuted = false;
}

static void StartCover(const char* text) {
    sCover = true;
    sCoverTime = 0.0f;
    sCoverText = text;
}

static void EndCover() {
    if (sCover) {
        sCover = false;
        RestoreBgm();
    }
    sFileRequest = -1;
    sNewRequest = -1;
    sAutoConfirm = false;
}

static void StartNewGame(Screen from, int file) {
    sNewFrom = from;
    sFile = file;
    snprintf(sName, sizeof(sName), "Link");
    sNameLower = true;
    sKeyCursor = KEY_DONE;
    GoTo(SCREEN_QUEST, 0);
}

// Returns true when the game must go to file select (which does the work, covered).
static bool Activate(int index) {
    if (index < 0 || index >= sItemCount) {
        return false;
    }
    const Item& item = sItems[index];
    if (!item.enabled) {
        PlaySfx(NA_SE_SY_FSEL_ERROR);
        return false;
    }
    sPressed = index;
    sPressTimer = 4;
    switch (item.action) {
        case ACT_CONTINUE:
        case ACT_JOURNAL:
        case ACT_PLAY: {
            const int file = item.action == ACT_PLAY ? sFile : ContinueFile();
            if (!FileValid(file)) {
                PlaySfx(NA_SE_SY_FSEL_ERROR);
                return false;
            }
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            sFileRequest = file;
            sAutoConfirm = true;
            sNewRequest = -1;
            if (item.action == ACT_JOURNAL) {
                sJournalWait = 30;
                sJournalTimeout = 20 * 60;
            } else {
                sJournalWait = 0;
            }
            StartCover(Strings::kLoading);
            return true;
        }
        case ACT_NEW_GAME: {
            const int file = FirstFreeFile();
            if (file < 0) {
                PlaySfx(NA_SE_SY_FSEL_ERROR);
                return false;
            }
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            StartNewGame(SCREEN_MAIN, file);
            return false;
        }
        case ACT_FILES:
            PlaySfx(NA_SE_SY_WIN_OPEN);
            GoTo(SCREEN_FILES, ImMax(0, ContinueFile()));
            return false;
        case ACT_MODS:
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            OpenPortMenu("Mods");
            return false;
        case ACT_SETTINGS:
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            OpenPortMenu("General");
            return false;
        case ACT_FEATURES:
            PlaySfx(NA_SE_SY_WIN_OPEN);
            GoTo(SCREEN_FEATURES, 0);
            return false;
        case ACT_FEATURES_MORE:
            PlaySfx(NA_SE_SY_WIN_OPEN);
            GoTo(SCREEN_FEATURES_MORE, 0);
            return false;
        case ACT_FEATURE:
            if (item.arg >= 0 && item.arg < FEATURE_COUNT) {
                PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
                OpenDefinitivePage(Strings::kFeatureSidebars[item.arg]);
            }
            return false;
        case ACT_EXTRAS:
            PlaySfx(NA_SE_SY_WIN_OPEN);
            CountMods();
            GoTo(SCREEN_EXTRAS, 0);
            return false;
        case ACT_VERSION:
            // Its text is the card itself.
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            return false;
        case ACT_BACK:
            Back();
            return false;
        case ACT_OPEN_FILE:
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            if (FileValid(item.arg)) {
                sFile = item.arg;
                GoTo(SCREEN_FILE, 0);
            } else {
                StartNewGame(SCREEN_FILES, item.arg);
            }
            return false;
        case ACT_COPY:
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            GoTo(SCREEN_COPY, 0);
            return false;
        case ACT_ERASE:
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            GoTo(SCREEN_ERASE, 0); // on "Não"
            return false;
        case ACT_ERASE_YES:
            if (FileValid(sFile)) {
                // The game's own erase (the same call its Erase screen makes).
                Save_DeleteFile(sFile);
                if (CVarGetInteger(CVAR_LAST_FILE, -1) == sFile) {
                    CVarClear(CVAR_LAST_FILE);
                    CVarSave();
                }
                PlaySfx(NA_SE_EV_DIAMOND_SWITCH);
            }
            GoTo(SCREEN_FILES, sFile);
            return false;
        case ACT_COPY_TO:
            if (FileValid(sFile) && !FileValid(item.arg)) {
                // The game's own copy (the same call its Copy screen makes).
                Save_CopyFile(sFile, item.arg);
                PlaySfx(NA_SE_EV_DIAMOND_SWITCH);
                GoTo(SCREEN_FILES, item.arg);
            } else {
                PlaySfx(NA_SE_SY_FSEL_ERROR);
                GoTo(SCREEN_FILES, sFile);
            }
            return false;
        case ACT_QUEST:
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            sNewQuest = item.arg;
            sKeyCursor = KEY_DONE;
            GoTo(SCREEN_NAME, 0);
            return false;
        case ACT_CLASSIC:
            // Nothing asked: the game's file select opens and shows itself.
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            EndCover();
            sJournalWait = 0;
            return true;
        default:
            return false;
    }
}

// Name entry. Returns true when the game must go to file select (to create the file).
static bool PressKey(int key) {
    const size_t len = strlen(sName);
    const char ch = NameKeyChar(sNameLower, key);
    if (ch != '\0') {
        if (len < kNameMax && !(ch == ' ' && len == 0)) {
            sName[len] = ch;
            sName[len + 1] = '\0';
            PlaySfx(NA_SE_SY_FSEL_DECIDE_S);
            // As on a phone keyboard: after the first capital letter come the small ones.
            if (len == 0 && !sNameLower) {
                sNameLower = true;
            }
        } else {
            PlaySfx(NA_SE_SY_FSEL_ERROR);
        }
        return false;
    }
    switch (key) {
        case KEY_CASE:
            sNameLower = !sNameLower;
            PlaySfx(NA_SE_SY_FSEL_CURSOR);
            return false;
        case KEY_ERASE:
            if (len > 0) {
                sName[len - 1] = '\0';
                PlaySfx(NA_SE_SY_FSEL_CLOSE);
                if (len == 1) {
                    sNameLower = false;
                }
            } else {
                PlaySfx(NA_SE_SY_FSEL_ERROR);
            }
            return false;
        case KEY_BACK:
            Back();
            return false;
        case KEY_DONE: {
            size_t end = len;
            while (end > 0 && sName[end - 1] == ' ') {
                end--;
            }
            if (end == 0 || FileValid(sFile)) {
                PlaySfx(NA_SE_SY_FSEL_ERROR);
                return false;
            }
            sName[end] = '\0';
            PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
            sNewRequest = sFile;
            sFileRequest = -1;
            sAutoConfirm = false;
            sJournalWait = 0;
            StartCover(Strings::kCreating);
            return true;
        }
        default:
            return false;
    }
}

static void MoveCursor(int step) {
    if (sItemCount <= 0) {
        return;
    }
    int& cursor = sCursor[sScreen];
    cursor = (cursor + step + sItemCount) % sItemCount;
    PlaySfx(NA_SE_SY_FSEL_CURSOR);
}

// Moves on the keyboard: left / right inside the row (going around), up / down to the key above or below.
static void MoveKey(int dx, int dy) {
    const KeyCell& cur = sKeys[sKeyCursor];
    int best = sKeyCursor;
    if (dx != 0) {
        int first = -1, last = -1;
        for (int i = 0; i < KEY_COUNT; i++) {
            if (sKeys[i].row == cur.row) {
                first = first < 0 ? i : first;
                last = i;
            }
        }
        best = sKeyCursor + dx;
        if (best > last) {
            best = first;
        } else if (best < first) {
            best = last;
        }
    } else if (dy != 0) {
        int rows = 0;
        for (int i = 0; i < KEY_COUNT; i++) {
            rows = ImMax(rows, sKeys[i].row + 1);
        }
        const int row = (cur.row + dy + rows) % rows;
        const float center = cur.col + cur.span * 0.5f;
        float bestDist = 1000.0f;
        for (int i = 0; i < KEY_COUNT; i++) {
            if (sKeys[i].row == row) {
                const float d = fabsf(sKeys[i].col + sKeys[i].span * 0.5f - center);
                if (d < bestDist) {
                    bestDist = d;
                    best = i;
                }
            }
        }
    }
    if (best != sKeyCursor) {
        sKeyCursor = best;
        PlaySfx(NA_SE_SY_FSEL_CURSOR);
    }
}

static void OnLoadGame(int32_t fileNum) {
    if (fileNum >= 0 && fileNum < 3) {
        CVarSetInteger(CVAR_LAST_FILE, fileNum);
        CVarSave();
    }
    // The file is open: the game shows itself (the cover fades away) with its music back.
    EndCover();
}

// ---------------------------------------------------------------------------------------------------------------------
// View: drawn on top of the game every frame while the title menu is up.

class TitleMenuWindow : public Ship::GuiWindow {
  public:
    TitleMenuWindow() : GuiWindow(CVAR_WINDOW("DefinitiveTitleMenu"), true, "Definitive Edition##TitleMenu") {
    }
    void InitElement() override {
    }
    void DrawElement() override {
    }
    void UpdateElement() override {
    }
    void Draw() override;

  private:
    TitleMenuRenderer mRenderer;
    bool mWasDown = false;
    int mDownFrames = 0;
    bool mDragged = false;
    ImVec2 mPressPos = ImVec2(-1, -1);
    ImVec2 mLastPos = ImVec2(-1, -1);
    float mTime = 0.0f;
};

void TitleMenuWindow::Draw() {
    const float dt = ImClamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
    const bool enabled = CVarGetInteger(CVAR_TITLE_MENU, 1) != 0;
    const bool portMenu = PortMenuOpen();

    // The title screen restarts its opening now and then, and its logo actor with it: the menu waits for it.
    if (sDetached) {
        sDetachedTime += dt;
        if (sDetachedTime > 3.0f) {
            sDetached = false;
            sActive = false;
        }
    }
    if (!enabled) {
        sActive = false;
        sDetached = false;
        sSplashDone = true;
    }

    // The image before the menu, once, when the game opens: it appears in 1.5 s, stays, and gives way to the menu
    // when the title screen is ready (or after a while, whatever happens).
    if (sSplashTime < 0.0f && !sSplashDone) {
        if (CVarGetInteger(CVAR_SETTING("DefinitiveUI.Splash"), 1) != 0) {
            sSplashTime = 0.0f;
        } else {
            sSplashDone = true;
        }
    }
    float splashAlpha = 0.0f;
    if (SplashOn()) {
        sSplashTime += dt;
        if (sSplashOut < 0.0f &&
            ((sSplashTime >= (sSplashSkip ? 0.6f : 4.0f) && sActive) || sSplashTime >= 14.0f)) {
            sSplashOut = sSplashTime;
        }
        splashAlpha = sSplashOut < 0.0f ? 1.0f : 1.0f - (sSplashTime - sSplashOut) / 1.0f;
        if (splashAlpha <= 0.0f) {
            sSplashDone = true;
            splashAlpha = 0.0f;
        }
    }
    const bool splashHolding = SplashOn() && sSplashOut < 0.0f;

    // The cover while file select works for the menu. It never stays for ever.
    if (sCover) {
        sCoverTime += dt;
        if (sCoverTime > 25.0f) {
            EndCover();
        }
    }
    sCoverAlpha += ((sCover ? 1.0f : 0.0f) - sCoverAlpha) * ImMin(1.0f, dt * (sCover ? 9.0f : 3.5f));
    if (!sCover && sCoverAlpha < 0.01f) {
        sCoverAlpha = 0.0f;
    }

    const float target = (sActive && !sClosing && !portMenu && !splashHolding) ? 1.0f : 0.0f;
    sAlpha += (target - sAlpha) * ImMin(1.0f, dt * (target > sAlpha ? 5.0f : 9.0f));
    if (sAlpha < 0.01f && target == 0.0f) {
        sAlpha = 0.0f;
        if (!sActive) {
            mRenderer.Reset();
        }
    }
    if (sAlpha <= 0.0f && sCoverAlpha <= 0.0f && splashAlpha <= 0.0f) {
        if (!sActive) {
            UnloadTextures();
            mTime = 0.0f;
        }
        return;
    }
    LoadTextures();
    mTime += dt;

    // The area of the window the game is drawn to.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 origin = viewport->Pos;
    ImVec2 size = viewport->Size;
    if (gfx_current_game_window_viewport.width > 0 && gfx_current_game_window_viewport.height > 0) {
        origin += ImVec2((float)gfx_current_game_window_viewport.x, (float)gfx_current_game_window_viewport.y);
        size = ImVec2((float)gfx_current_game_window_viewport.width, (float)gfx_current_game_window_viewport.height);
    }

    // Touch / mouse, as the Diário de Quests reads it (window coordinates). A touch counts when the finger is let go
    // without having been dragged, at the place it was last seen.
    PointerState pointer;
    {
        auto window = Ship::Context::GetInstance()->GetWindow();
        const Ship::Coords mouse = window->GetMousePos();
        const bool down = window->GetMouseState(Ship::LUS_MOUSE_BTN_LEFT);
        const ImVec2 pos = viewport->Pos + ImVec2((float)mouse.x, (float)mouse.y);
        if (down) {
            mDownFrames = mWasDown ? mDownFrames + 1 : 1;
            if (mDownFrames == 1) {
                mDragged = false;
            }
            if (mDownFrames <= 2) {
                // The first position of a touch may still be the one of the touch before it.
                mPressPos = pos;
            } else {
                const ImVec2 d = pos - mPressPos;
                const float limit = size.y * 0.06f;
                if (d.x * d.x + d.y * d.y > limit * limit) {
                    mDragged = true;
                }
            }
        }
        pointer.down = down;
        pointer.tap = !down && mWasDown && !mDragged;
        pointer.pos = pointer.tap ? mLastPos : pos;
        pointer.moved = !down && (mLastPos.x != pos.x || mLastPos.y != pos.y);
        mWasDown = down;
        mLastPos = pos;
    }
    if (pointer.tap) {
        sTouchMode = true;
        if (SplashOn()) {
            sSplashSkip = true;
        }
    }

    // Drawn in front of the game (and of everything else: the menu fades out whenever the port menu opens).
    ImDrawList* dl = ImGui::GetForegroundDrawList(viewport);
    if (sAlpha > 0.0f) {
        TitleMenuView view;
        BuildItems();
        view.screen = (int)sScreen;
        view.items = sViewItems;
        view.count = sItemCount;
        view.selected = sCursor[sScreen];
        view.pressed = sPressTimer > 0 ? sPressed : -1;
        view.title = sTitle[0] != '\0' ? sTitle : nullptr;
        view.showBack = sScreen != SCREEN_MAIN;
        view.touchMode = sTouchMode;
        if (sScreen == SCREEN_NAME) {
            view.keys = sKeys;
            view.keyCount = KEY_COUNT;
            view.keySelected = sKeyCursor;
            view.nameText = sName;
            view.nameMax = kNameMax;
            view.nameHint = Strings::kNameHint;
        }
        snprintf(sFooter, sizeof(sFooter), "OOT D.E. %s · Ship of Harkinian %s · © Nintendo", OOTDE_VERSION,
                 (char*)gBuildVersion);
        view.footer = sFooter;
        // The save of the option under the cursor: name and hearts.
        int shown = -1;
        if (sScreen == SCREEN_MAIN && sItemCount > 0 &&
            (sItems[view.selected].action == ACT_CONTINUE || sItems[view.selected].action == ACT_JOURNAL)) {
            shown = ContinueFile();
        } else if (sScreen == SCREEN_FILES && view.selected < 3) {
            shown = view.selected;
        } else if (sScreen == SCREEN_FILE || sScreen == SCREEN_ERASE || sScreen == SCREEN_COPY) {
            shown = sFile;
        }
        if (FileValid(shown)) {
            const SaveFileMetaInfo* meta = Save_GetSaveMetaInfo(shown);
            char name[16];
            DecodeName(meta, name, sizeof(name));
            view.showSave = true;
            snprintf(view.saveTitle, sizeof(view.saveTitle), Strings::kSaveTitle, shown + 1, name);
            view.heartsCapacity = meta->healthCapacity / 16;
            view.health = meta->health;
        }
        view.alpha = sAlpha;
        view.slide = (1.0f - sAlpha) * (1.0f - sAlpha);

        pointer.valid = sActive && !sClosing && !sDetached && !portMenu && !SplashOn() && !sCover;
        const TitleMenuInput input = mRenderer.Draw(dl, origin, size, view, dt, mTime, pointer);
        if (input.activated >= 0) {
            sPendingTouch = input.activated;
        } else if (input.hovered >= 0 && input.hovered != sCursor[sScreen]) {
            sCursor[sScreen] = input.hovered;
            PlaySfx(NA_SE_SY_FSEL_CURSOR);
        }
        if (input.keyActivated >= 0) {
            sPendingKey = input.keyActivated;
            sKeyCursor = input.keyActivated;
        } else if (input.keyHovered >= 0 && input.keyHovered != sKeyCursor) {
            sKeyCursor = input.keyHovered;
        }
    }
    if (sCoverAlpha > 0.0f) {
        mRenderer.DrawLoading(dl, origin, size, mTime, sCoverAlpha, sCover ? sCoverText : nullptr);
    }
    if (splashAlpha > 0.0f) {
        mRenderer.DrawSplash(dl, origin, size, dt, sSplashTime, splashAlpha);
    }
}

static std::shared_ptr<TitleMenuWindow> sWindow;

void RegisterWindow() {
    sWindow = std::make_shared<TitleMenuWindow>();
    Ship::Context::GetInstance()->GetWindow()->GetGui()->AddGuiWindow(sWindow);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(OnLoadGame);
}

void UnregisterWindow() {
    UnloadTextures();
    sWindow = nullptr;
}

} // namespace DefinitiveUI

// ---------------------------------------------------------------------------------------------------------------------
// C entry points

using namespace DefinitiveUI;

extern "C" bool DefinitiveUI_TitleMenuEnabled(void) {
    return CVarGetInteger(CVAR_TITLE_MENU, 1) != 0;
}

// The menu's own music (DeMusic) is optional: off, the game's title music plays as always.
static void UpdateMusic() {
    const bool wanted = !sClosing && CVarGetInteger(CVAR_SETTING("DefinitiveUI.MenuMusicVolume"), 0) > 0;
    if (wanted) {
        MusicLoad();
    }
    const bool playing = wanted && MusicAvailable();
    MusicSetPlaying(playing);
    if (playing != sBgmMuted && !sCover) {
        Audio_SetVolScale(SEQ_PLAYER_BGM_MAIN, kBgmVolScale, playing ? 0 : 0x7F, 20);
        sBgmMuted = playing;
    }
}

extern "C" bool DefinitiveUI_TitleMenuUpdate(PlayState* play) {
    if (!sActive) {
        sActive = true;
        sClosing = false;
        sOpenSfxTimer = 9; // the game runs at 20 frames per second: with the panel (0.45 s)
        // Start on Continuar when there is a file, otherwise on Novo Jogo.
        GoTo(SCREEN_MAIN, ContinueFile() >= 0 ? MAIN_CONTINUE : MAIN_NEW_GAME);
        sInputCooldown = 6;
        // Back on the title screen: forget anything asked that was not done.
        EndCover();
        sJournalWait = 0;
    }
    if (sDetached) {
        sDetached = false;
        sInputCooldown = 4;
    }
    UpdateMusic();
    if (sClosing) {
        return false;
    }
    Input* input = &play->state.input[0];
    // The image before the menu is still up: any button sends it away, and nothing reaches the menu yet.
    if (SplashOn()) {
        if (input->press.button != 0) {
            sSplashSkip = true;
        }
        sInputCooldown = 4;
        sPendingTouch = -1;
        sPendingKey = -1;
        return false;
    }
    if (sOpenSfxTimer >= 0 && sOpenSfxTimer-- == 0) {
        PlaySfx(NA_SE_SY_WIN_OPEN);
    }
    if (sPressTimer > 0) {
        sPressTimer--;
    }

    // The port menu (Mods, Configurações) is on top: leave the controller to it, and ignore the button that closes it.
    if (PortMenuOpen()) {
        sPortMenuWasOpen = true;
        sPendingTouch = -1;
        sPendingKey = -1;
        return false;
    }
    if (sPortMenuWasOpen) {
        sPortMenuWasOpen = false;
        sInputCooldown = 10;
        CountMods();
    }
    BuildItems();

    // Touches.
    if (sScreen == SCREEN_NAME && sPendingKey >= 0) {
        const int key = sPendingKey;
        sPendingKey = -1;
        if (PressKey(key)) {
            sClosing = true;
            return true;
        }
        return false;
    }
    if (sScreen != SCREEN_NAME && sPendingTouch >= 0) {
        const int item = sPendingTouch;
        sPendingTouch = -1;
        sCursor[sScreen] = item;
        if (Activate(item)) {
            sClosing = true;
            return true;
        }
        return false;
    }
    sPendingTouch = -1;
    sPendingKey = -1;
    if (sInputCooldown > 0) {
        sInputCooldown--;
        return false;
    }

    // The controller: stick or D-pad, repeating while held.
    int dx = 0, dy = 0;
    if (input->cur.stick_y > 30 || CHECK_BTN_ALL(input->cur.button, BTN_DUP)) {
        dy = -1;
    } else if (input->cur.stick_y < -30 || CHECK_BTN_ALL(input->cur.button, BTN_DDOWN)) {
        dy = 1;
    } else if (input->cur.stick_x < -30 || CHECK_BTN_ALL(input->cur.button, BTN_DLEFT)) {
        dx = -1;
    } else if (input->cur.stick_x > 30 || CHECK_BTN_ALL(input->cur.button, BTN_DRIGHT)) {
        dx = 1;
    }
    if (dx != 0 || dy != 0) {
        bool move = false;
        if (dx != sHoldDirX || dy != sHoldDirY) {
            sHoldDirX = dx;
            sHoldDirY = dy;
            sHoldFrames = 0;
            move = true;
        } else if (++sHoldFrames >= 8 && (sHoldFrames - 8) % 3 == 0) {
            move = true;
        }
        if (move) {
            sTouchMode = false;
            if (sScreen == SCREEN_NAME) {
                MoveKey(dx, dy);
            } else if (dy != 0) {
                MoveCursor(dy);
            }
        }
    } else {
        sHoldDirX = sHoldDirY = 0;
        sHoldFrames = 0;
    }

    if (input->press.button != 0) {
        sTouchMode = false;
    }
    if (sScreen == SCREEN_NAME) {
        bool go = false;
        if (CHECK_BTN_ALL(input->press.button, BTN_START)) {
            sKeyCursor = KEY_DONE;
            go = PressKey(KEY_DONE);
        } else if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
            go = PressKey(sKeyCursor);
        } else if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
            // B erases a letter, and leaves when there is none.
            if (sName[0] != '\0') {
                PressKey(KEY_ERASE);
            } else {
                Back();
            }
        }
        if (go) {
            sClosing = true;
            return true;
        }
        return false;
    }
    if (CHECK_BTN_ANY(input->press.button, BTN_A | BTN_START)) {
        if (Activate(sCursor[sScreen])) {
            sClosing = true;
            return true;
        }
    } else if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        Back();
    }
    return false;
}

extern "C" void DefinitiveUI_TitleMenuClose(void) {
    MusicSetPlaying(false);
    if (sBgmMuted && !sCover) {
        // Back to normal for the music of the next screen.
        RestoreBgm();
    }
    if (sClosing) {
        // The menu sent the game to file select.
        sActive = false;
        sClosing = false;
        sDetached = false;
    } else if (sActive) {
        // The title screen is only restarting its opening: the menu stays (see TitleMenuWindow::Draw).
        sDetached = true;
        sDetachedTime = 0.0f;
    }
}

extern "C" bool DefinitiveUI_TitleMenuVisible(void) {
    return (sActive && !sClosing) || sCover || sCoverAlpha > 0.5f || SplashOn();
}

extern "C" bool DefinitiveUI_CoverActive(void) {
    return sCover;
}

extern "C" int32_t DefinitiveUI_TakeNewFileRequest(uint8_t* name, int32_t* quest) {
    const int file = sNewRequest;
    sNewRequest = -1;
    if (file < 0 || !sCover) {
        return -1;
    }
    EncodeName(sName, name);
    *quest = sNewQuest;
    return file;
}

extern "C" void DefinitiveUI_NewFileCreated(int32_t file) {
    if (sCover) {
        sFileRequest = file;
        sAutoConfirm = true;
        sCoverText = Strings::kLoading;
    }
}

extern "C" int32_t DefinitiveUI_TakeFileRequest(bool* newGame) {
    const int file = sCover ? sFileRequest : -1;
    *newGame = false;
    sFileRequest = -1;
    return file;
}

extern "C" void DefinitiveUI_CancelFileRequest(void) {
    EndCover();
    sJournalWait = 0;
}

extern "C" bool DefinitiveUI_TakeAutoConfirm(void) {
    const bool confirm = sAutoConfirm && sCover;
    sAutoConfirm = false;
    return confirm;
}

extern "C" bool DefinitiveUI_TakeJournalRequest(void) {
    if (sJournalWait <= 0) {
        return false;
    }
    if (gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return false;
    }
    if (--sJournalTimeout <= 0) {
        // Never open the pause by surprise much later.
        sJournalWait = 0;
        return false;
    }
    if (--sJournalWait > 0) {
        return false;
    }
    return true;
}
