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

struct TitleMenuView {
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

    bool showBack = false;  // B: Voltar hint
    const char* footer = nullptr; // small text at the bottom of the panel (version)

    // Where the game draws its logo, in screen pixels (the subtitle goes under it).
    ImVec2 logoCenter = ImVec2(0, 0);
    float logoWidth = 0.0f;
    float logoBottom = 0.0f;

    float alpha = 1.0f;   // opening / closing fade
    float slide = 0.0f;   // 0 = in place, 1 = fully slid out to the right
};

// Mouse or touch, in the same coordinates as the drawing.
struct PointerState {
    bool valid = false;
    ImVec2 pos = ImVec2(0, 0);
    bool down = false;    // held
    bool clicked = false; // went down this frame
    bool moved = false;   // moved since the last frame (mouse hover)
};

struct TitleMenuInput {
    int hovered = -1;   // pointer over an item (mouse)
    int activated = -1; // item tapped or clicked
};

class TitleMenuRenderer {
  public:
    // Draws the menu over `origin`..`origin + size` and returns what the pointer did.
    TitleMenuInput Draw(ImDrawList* dl, ImVec2 origin, ImVec2 size, const TitleMenuView& view, float dt, float time,
                        const PointerState& pointer);
    void Reset();

  private:
    float mHighlight[16] = {};
    ImVec2 mNaviPos = ImVec2(-1, -1);
    float mTrailTimer = 0.0f;
    Particles mParticles;
    int mPressedItem = -1;
};

} // namespace DefinitiveUI
