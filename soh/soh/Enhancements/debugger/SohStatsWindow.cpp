#include "SohStatsWindow.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/OTRGlobals.h"

void SohStatsWindow::DrawElement() {
    const float framerate = ImGui::GetIO().Framerate;
    const float deltatime = ImGui::GetIO().DeltaTime;
    ImGui::PushFont(OTRGlobals::Instance->fontMonoLarger);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

#if defined(_WIN32)
    ImGui::Text("%s", UIWidgets::Tr("Platform: Windows"));
#elif defined(__IOS__)
    ImGui::Text("%s", UIWidgets::Tr("Platform: iOS"));
#elif defined(__APPLE__)
    ImGui::Text("%s", UIWidgets::Tr("Platform: macOS"));
#elif defined(__linux__)
    ImGui::Text("%s", UIWidgets::Tr("Platform: Linux"));
#else
    ImGui::Text("%s", UIWidgets::Tr("Platform: Unknown"));
#endif
    ImGui::Text(UIWidgets::Tr("Status: %.3f ms/frame (%.1f FPS)"), deltatime * 1000.0f, framerate);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}
