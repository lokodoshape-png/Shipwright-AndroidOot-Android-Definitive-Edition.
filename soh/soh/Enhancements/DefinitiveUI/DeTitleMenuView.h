#pragma once

// OOT D.E.: drawing of the main menu (screen 1) of the Definitive Edition interface. It only draws what the model
// (DeTitleMenu.cpp) gives it and reports touches/clicks; it never changes the game by itself.

#include <imgui.h>
#include "DeTheme.h"

namespace DefinitiveUI {

struct TitleMenuItem {
    const char* label;
    const char* detail; // description shown in the card under the buttons
    bool enabled;
};

// One key of the name entry screen, on a grid 10 columns wide.
struct KeyCell {
    const char* label;
    int row;
    int col;
    int span; // columns taken
};

// The keyboard of the name entry: letters and digits first, then the keys below.
enum NameKey { KEY_CHARS = 36, KEY_SPACE = 36, KEY_CASE, KEY_ERASE, KEY_BACK, KEY_DONE, KEY_COUNT };
// Fills `out` (KEY_COUNT cells; the labels of the letters go to `labels`). `lower`: small letters.
void BuildNameKeys(bool lower, KeyCell* out, char labels[KEY_CHARS][2]);
// The character typed by key `index` (0 when it is not a letter, digit or space).
char NameKeyChar(bool lower, int index);

struct TitleMenuView {
    int screen = 0;              // which list is shown (main, extras...): a change slides the new buttons in
    const char* title = nullptr; // plaque above the buttons (null: none)
    const TitleMenuItem* items = nullptr;
    int count = 0;
    int selected = 0;
    int pressed = -1; // item flashing gold after being chosen

    // Save summary in the card (Continuar / Diário de Quests).
    bool showSave = false;
    char saveTitle[64] = {};
    int heartsCapacity = 0; // in hearts
    int health = 0;         // in 1/16 hearts (as the game keeps it)

    // Name entry (Novo Jogo): when `keys` is set, the keyboard is drawn in place of the buttons.
    const KeyCell* keys = nullptr;
    int keyCount = 0;
    int keySelected = 0;
    const char* nameText = nullptr; // what was typed so far
    int nameMax = 8;
    const char* nameHint = nullptr; // line under the name

    bool touchMode = false; // the last input was a touch: the hints talk about touching
    bool showBack = false;  // B: Voltar hint
    const char* footer = nullptr; // small text at the bottom of the panel (version)

    float alpha = 1.0f;   // opening / closing fade
    float slide = 0.0f;   // 0 = in place, 1 = fully slid out to the right
    // 0..1: an opaque dark screen under everything, whatever `alpha` is (the game's own title screen is never seen).
    float backdrop = 0.0f;
};

// Mouse or touch, in the same coordinates as the drawing.
struct PointerState {
    bool valid = false;
    ImVec2 pos = ImVec2(0, 0);
    bool down = false;  // held
    bool tap = false;   // released this frame without having been dragged: a touch or click on what is under it
    bool moved = false; // moved since the last frame (mouse hover)
};

struct TitleMenuInput {
    int hovered = -1;   // pointer over an item (mouse)
    int activated = -1; // item tapped or clicked
    int keyHovered = -1;
    int keyActivated = -1;
};

class TitleMenuRenderer {
  public:
    // Draws the menu over `origin`..`origin + size` and returns what the pointer did.
    TitleMenuInput Draw(ImDrawList* dl, ImVec2 origin, ImVec2 size, const TitleMenuView& view, float dt, float time,
                        const PointerState& pointer);
    void Reset();
    // The image shown before the menu, when the game opens: the Definitive Edition logo on black. `time` is the time
    // since it appeared, `alpha` its fade.
    void DrawSplash(ImDrawList* dl, ImVec2 origin, ImVec2 size, float dt, float time, float alpha);
    // Covers the screen while the game creates or loads a file for the menu.
    void DrawLoading(ImDrawList* dl, ImVec2 origin, ImVec2 size, float time, float alpha, const char* text);

  private:
    float mHighlight[16] = {};
    float mPop[16] = {};        // bounce of a button that was just chosen
    ImVec2 mNaviPos = ImVec2(-1, -1);
    float mTrailTimer = 0.0f;
    Particles mParticles;       // over the panel (dust, Navi's trail, bursts)
    Particles mSceneParticles;  // fireflies over the landscape
    Particles mSplashParticles; // dust around the logo of the opening image
    int mPressedItem = -1;
    float mKeyGlow[64] = {};
    int mKeyFlash = -1;
    float mKeyFlashTime = 0.0f;
    float mAge = 0.0f;          // time since the menu appeared (opening animation)
    int mScreen = -1;
    float mScreenAge = 10.0f;   // time since the list of buttons changed
    float mScreenDir = 1.0f;    // +1: new list comes from the right, -1: from the left
    int mLastSelected = -1;
    float mCardAge = 10.0f;     // time since the description changed
};

} // namespace DefinitiveUI
