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

#include <libultraship/libultraship.h>
#include <Window.h>
#include <spdlog/spdlog.h>
#include <Fast3D/gfx_pc.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>
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

// SaveManager.h only declares this one for C files.
extern "C" SaveFileMetaInfo* Save_GetSaveMetaInfo(int fileNum);

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

enum Screen { SCREEN_MAIN, SCREEN_EXTRAS };

static bool sActive = false;      // title screen with the logo shown
static bool sClosing = false;     // fading out towards file select
static float sAlpha = 0.0f;       // drawn opacity
static Screen sScreen = SCREEN_MAIN;
static int sCursor[2] = { 0, 0 };
static int sPressed = -1;
static int sPressTimer = 0;
static int sOpenSfxTimer = -1; // frames until the sound of the panel sliding in (opening animation)
static int sPendingTouch = -1;    // item tapped in the view, handled at the next update
static int sInputCooldown = 0;    // frames to ignore the controller (after the port menu closes)
static int sHoldDir = 0;
static int sHoldFrames = 0;
static bool sPortMenuWasOpen = false;

static int sFileRequest = -1;
static bool sFileRequestNew = false;
static bool sAutoConfirm = false;
static int sJournalWait = 0; // frames before opening the journal; 0 = no request
static int sJournalTimeout = 0;

static TitleMenuItem sItems[MAIN_COUNT > EXTRAS_COUNT ? MAIN_COUNT : EXTRAS_COUNT];
static char sNewGameText[160];
static char sModsText[200];
static char sVersionText[300];
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

// The last file played, or the first one that exists. -1 when there is none.
static int ContinueFile() {
    const int last = CVarGetInteger(CVAR_LAST_FILE, -1);
    if (last >= 0 && last < 3 && Save_GetSaveMetaInfo(last)->valid) {
        return last;
    }
    for (int i = 0; i < 3; i++) {
        if (Save_GetSaveMetaInfo(i)->valid) {
            return i;
        }
    }
    return -1;
}

static int FirstFreeFile() {
    for (int i = 0; i < 3; i++) {
        if (!Save_GetSaveMetaInfo(i)->valid) {
            return i;
        }
    }
    return -1;
}

// File names are stored in the game's own font codes.
static void DecodeName(const SaveFileMetaInfo* meta, char* out, size_t size) {
    size_t n = 0;
    for (int i = 0; i < 8 && n + 1 < size; i++) {
        const u8 c = meta->playerName[i];
        char ch = '\0';
        if (meta->filenameLanguage == NAME_LANGUAGE_PAL) {
            if (c <= 0x09) {
                ch = '0' + c;
            } else if (c <= 0x23) {
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

static void CountMods() {
    sModsEnabled = sModsTotal = 0;
    for (const auto& mod : ModsMenu::List()) {
        sModsTotal++;
        sModsEnabled += mod.enabled ? 1 : 0;
    }
}

static int ItemCount() {
    return sScreen == SCREEN_MAIN ? MAIN_COUNT : EXTRAS_COUNT;
}

static void BuildItems() {
    if (sScreen == SCREEN_MAIN) {
        const int cont = ContinueFile();
        const int free = FirstFreeFile();
        for (int i = 0; i < MAIN_COUNT; i++) {
            sItems[i] = { Strings::kMainLabels[i], nullptr, true };
        }
        sItems[MAIN_CONTINUE].detail = cont >= 0 ? Strings::kContinue : Strings::kContinueNone;
        sItems[MAIN_CONTINUE].enabled = cont >= 0;
        if (free >= 0) {
            snprintf(sNewGameText, sizeof(sNewGameText), Strings::kNewGame, free + 1);
            sItems[MAIN_NEW_GAME].detail = sNewGameText;
        } else {
            sItems[MAIN_NEW_GAME].detail = Strings::kNewGameFull;
            sItems[MAIN_NEW_GAME].enabled = false;
        }
        sItems[MAIN_FILE_SELECT].detail = Strings::kFileSelect;
        sItems[MAIN_JOURNAL].detail = cont >= 0 ? Strings::kJournal : Strings::kJournalNone;
        sItems[MAIN_JOURNAL].enabled = cont >= 0;
        sItems[MAIN_MODS].detail = Strings::kMods;
        sItems[MAIN_SETTINGS].detail = Strings::kSettings;
        sItems[MAIN_EXTRAS].detail = Strings::kExtras;
    } else {
        snprintf(sModsText, sizeof(sModsText), Strings::kExtrasMods, sModsEnabled, sModsTotal);
        snprintf(sVersionText, sizeof(sVersionText), Strings::kExtrasVersion, OOTDE_VERSION, (char*)gBuildVersion);
        sItems[EXTRAS_MODS] = { Strings::kExtrasLabels[EXTRAS_MODS], sModsText, true };
        sItems[EXTRAS_VERSION] = { Strings::kExtrasLabels[EXTRAS_VERSION], sVersionText, true };
        // Screen 11 comes in a later step: shown as coming, not as working.
        sItems[EXTRAS_CREDITS] = { Strings::kExtrasLabels[EXTRAS_CREDITS], Strings::kExtrasCredits, false };
        sItems[EXTRAS_BACK] = { Strings::kExtrasLabels[EXTRAS_BACK], Strings::kExtrasBack, true };
    }
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

// Returns true when the game must go to file select.
static bool Activate(int item) {
    if (item < 0 || item >= ItemCount()) {
        return false;
    }
    if (!sItems[item].enabled) {
        PlaySfx(NA_SE_SY_FSEL_ERROR);
        return false;
    }
    sPressed = item;
    sPressTimer = 10;
    if (sScreen == SCREEN_MAIN) {
        switch (item) {
            case MAIN_CONTINUE:
            case MAIN_JOURNAL:
                sFileRequest = ContinueFile();
                sFileRequestNew = false;
                sAutoConfirm = true;
                if (item == MAIN_JOURNAL) {
                    sJournalWait = 30;
                    sJournalTimeout = 20 * 60;
                }
                return true;
            case MAIN_NEW_GAME:
                sFileRequest = FirstFreeFile();
                sFileRequestNew = true;
                return true;
            case MAIN_FILE_SELECT:
                return true;
            case MAIN_MODS:
                PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
                OpenPortMenu("Mods");
                return false;
            case MAIN_SETTINGS:
                PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
                OpenPortMenu("General");
                return false;
            case MAIN_EXTRAS:
                PlaySfx(NA_SE_SY_WIN_OPEN);
                CountMods();
                sScreen = SCREEN_EXTRAS;
                sCursor[SCREEN_EXTRAS] = 0;
                sPressed = -1;
                return false;
        }
    } else {
        switch (item) {
            case EXTRAS_MODS:
                PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
                OpenPortMenu("Mods");
                return false;
            case EXTRAS_VERSION:
                // Its text is the card itself.
                PlaySfx(NA_SE_SY_FSEL_DECIDE_L);
                return false;
            case EXTRAS_BACK:
                PlaySfx(NA_SE_SY_FSEL_CLOSE);
                sScreen = SCREEN_MAIN;
                sPressed = -1;
                return false;
        }
    }
    return false;
}

static void MoveCursor(int step) {
    const int count = ItemCount();
    int& cursor = sCursor[sScreen];
    cursor = (cursor + step + count) % count;
    PlaySfx(NA_SE_SY_FSEL_CURSOR);
}

static void OnLoadGame(int32_t fileNum) {
    if (fileNum >= 0 && fileNum < 3) {
        CVarSetInteger(CVAR_LAST_FILE, fileNum);
        CVarSave();
    }
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
    ImVec2 mLastPos = ImVec2(-1, -1);
    float mTime = 0.0f;
};

void TitleMenuWindow::Draw() {
    const float dt = ImGui::GetIO().DeltaTime;
    const bool portMenu = PortMenuOpen();
    const float target = (sActive && !sClosing && !portMenu) ? 1.0f : 0.0f;
    sAlpha += (target - sAlpha) * ImMin(1.0f, dt * (target > sAlpha ? 5.0f : 9.0f));
    if (sAlpha < 0.01f && target == 0.0f) {
        sAlpha = 0.0f;
        if (!sActive) {
            UnloadTextures();
            mRenderer.Reset();
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

    // Drawn in front of the game (and of everything else: the menu fades out whenever the port menu opens).
    ImDrawList* dl = ImGui::GetForegroundDrawList(viewport);
    {
        TitleMenuView view;
        BuildItems();
        view.items = sItems;
        view.count = ItemCount();
        view.selected = sCursor[sScreen];
        view.pressed = sPressTimer > 0 ? sPressed : -1;
        view.title = sScreen == SCREEN_EXTRAS ? Strings::kExtrasTitle : nullptr;
        view.showBack = sScreen == SCREEN_EXTRAS;
        view.screen = (int)sScreen;
        snprintf(sFooter, sizeof(sFooter), "OOT D.E. %s · Ship of Harkinian %s · © Nintendo", OOTDE_VERSION, (char*)gBuildVersion);
        view.footer = sFooter;
        if (sScreen == SCREEN_MAIN && (view.selected == MAIN_CONTINUE || view.selected == MAIN_JOURNAL)) {
            const int file = ContinueFile();
            if (file >= 0) {
                const SaveFileMetaInfo* meta = Save_GetSaveMetaInfo(file);
                char name[16];
                DecodeName(meta, name, sizeof(name));
                view.showSave = true;
                snprintf(view.saveTitle, sizeof(view.saveTitle), Strings::kSaveTitle, file + 1, name);
                view.heartsCapacity = meta->healthCapacity / 16;
                view.health = meta->health;
            }
        }
        view.alpha = sAlpha;
        view.slide = (1.0f - sAlpha) * (1.0f - sAlpha);

        // Touch / mouse, as the Diário de Quests reads it (window coordinates).
        PointerState pointer;
        auto window = Ship::Context::GetInstance()->GetWindow();
        const Ship::Coords mouse = window->GetMousePos();
        const bool down = window->GetMouseState(Ship::LUS_MOUSE_BTN_LEFT);
        pointer.pos = viewport->Pos + ImVec2((float)mouse.x, (float)mouse.y);
        pointer.valid = sActive && !sClosing && !portMenu;
        pointer.down = down;
        pointer.clicked = down && !mWasDown;
        pointer.moved = mLastPos.x != pointer.pos.x || mLastPos.y != pointer.pos.y;
        mWasDown = down;
        mLastPos = pointer.pos;

        const TitleMenuInput input =
            mRenderer.Draw(dl, origin, size, view, dt, mTime, pointer);
        if (input.activated >= 0) {
            sPendingTouch = input.activated;
        } else if (input.hovered >= 0 && input.hovered != sCursor[sScreen]) {
            sCursor[sScreen] = input.hovered;
            PlaySfx(NA_SE_SY_FSEL_CURSOR);
        }
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

extern "C" bool DefinitiveUI_TitleMenuUpdate(PlayState* play) {
    if (!sActive) {
        sActive = true;
        sClosing = false;
        sOpenSfxTimer = 9; // the game runs at 20 frames per second: with the panel (0.45 s)
        sScreen = SCREEN_MAIN;
        // Start on Continuar when there is a file, otherwise on Novo Jogo.
        sCursor[SCREEN_MAIN] = ContinueFile() >= 0 ? MAIN_CONTINUE : MAIN_NEW_GAME;
        sPendingTouch = -1;
        sInputCooldown = 6;
        // Back on the title screen: forget any request that was not used.
        sFileRequest = -1;
        sAutoConfirm = false;
        sJournalWait = 0;
    }
    if (sClosing) {
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
        return false;
    }
    if (sPortMenuWasOpen) {
        sPortMenuWasOpen = false;
        sInputCooldown = 10;
        CountMods();
    }
    BuildItems();

    if (sPendingTouch >= 0) {
        const int item = sPendingTouch;
        sPendingTouch = -1;
        sCursor[sScreen] = item;
        if (Activate(item)) {
            sClosing = true;
            return true;
        }
        return false;
    }
    if (sInputCooldown > 0) {
        sInputCooldown--;
        return false;
    }

    Input* input = &play->state.input[0];
    // Up/down with the stick or the D-pad, repeating while held.
    int dir = 0;
    if (input->cur.stick_y > 30 || CHECK_BTN_ALL(input->cur.button, BTN_DUP)) {
        dir = -1;
    } else if (input->cur.stick_y < -30 || CHECK_BTN_ALL(input->cur.button, BTN_DDOWN)) {
        dir = 1;
    }
    if (dir != 0) {
        if (dir != sHoldDir) {
            sHoldDir = dir;
            sHoldFrames = 0;
            MoveCursor(dir);
        } else if (++sHoldFrames >= 12 && (sHoldFrames - 12) % 4 == 0) {
            MoveCursor(dir);
        }
    } else {
        sHoldDir = 0;
        sHoldFrames = 0;
    }

    if (CHECK_BTN_ANY(input->press.button, BTN_A | BTN_START)) {
        if (Activate(sCursor[sScreen])) {
            sClosing = true;
            return true;
        }
    } else if (CHECK_BTN_ALL(input->press.button, BTN_B) && sScreen == SCREEN_EXTRAS) {
        PlaySfx(NA_SE_SY_FSEL_CLOSE);
        sScreen = SCREEN_MAIN;
    }
    return false;
}

extern "C" void DefinitiveUI_TitleMenuClose(void) {
    sActive = false;
    sClosing = false;
}

extern "C" bool DefinitiveUI_TitleMenuVisible(void) {
    return sActive && !sClosing;
}

extern "C" int32_t DefinitiveUI_TakeFileRequest(bool* newGame) {
    const int file = sFileRequest;
    *newGame = sFileRequestNew;
    sFileRequest = -1;
    sFileRequestNew = false;
    if (file < 0) {
        sAutoConfirm = false;
        sJournalWait = 0;
    }
    return file;
}

extern "C" bool DefinitiveUI_TakeAutoConfirm(void) {
    const bool confirm = sAutoConfirm;
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
