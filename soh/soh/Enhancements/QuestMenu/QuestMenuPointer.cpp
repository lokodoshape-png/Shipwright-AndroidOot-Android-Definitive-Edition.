// OOT D.E.: the mouse / touch pointer for the QuestMenu. See QuestMenuPointer.h.
#include "QuestMenuPointer.h"
#include "soh/OTRGlobals.h"
#include <Window.h>
#include <Fast3D/gfx_pc.h>
#include <imgui.h>

extern "C" void QuestMenuPointer_Get(QuestMenuPointer* out) {
    auto window = OTRGlobals::Instance->context->GetWindow();
    Ship::Coords pos = window->GetMousePos();
    Ship::CoordsF wheel = window->GetMouseWheel();
    float vx = (float)(pos.x - gfx_current_game_window_viewport.x);
    float vy = (float)(pos.y - gfx_current_game_window_viewport.y);
    float vw = (float)gfx_current_game_window_viewport.width;
    float vh = (float)gfx_current_game_window_viewport.height;
    float aspect;
    float ndcX;

    out->valid = false;
    out->down = false;
    out->x = out->y = out->wheel = 0.0f;
    if ((vw <= 0.0f) || (vh <= 0.0f) || (gfx_current_dimensions.height == 0)) {
        return;
    }
    if ((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    if ((vx < 0.0f) || (vy < 0.0f) || (vx >= vw) || (vy >= vh)) {
        return;
    }
    // Inverse of the screen mapping of Fast3D: x is scaled around the center by 4:3 / aspect ratio
    // (gfx_adjust_x_for_aspect_ratio), so the sides of wide screens are outside 0..320.
    aspect = (float)gfx_current_dimensions.width / (float)gfx_current_dimensions.height;
    ndcX = vx / vw * 2.0f - 1.0f;
    out->x = (ndcX * aspect / (4.0f / 3.0f) + 1.0f) * 160.0f;
    out->y = vy / vh * 240.0f;
    out->down = window->GetMouseState(Ship::LUS_MOUSE_BTN_LEFT);
    out->wheel = wheel.y;
    out->valid = true;
}
