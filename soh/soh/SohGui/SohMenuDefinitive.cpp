// OOT D.E. Definitive Expansion: "Definitive" menu of the port menu. Each page is drawn by the expansion module
// (soh/Enhancements/DefinitiveExpansion), so the systems stay apart from the Ship of Harkinian menus.
#include "SohMenu.h"
#include "UIWidgets.hpp"
#include "soh/Enhancements/DefinitiveExpansion/DefinitiveExpansion.h"

namespace SohGui {

using namespace UIWidgets;

void SohMenu::AddMenuDefinitive() {
    AddMenuEntry("Definitive", CVAR_SETTING("Menu.DefinitiveSidebarSection"));

    WidgetPath path = { "Definitive", "Saves", SECTION_COLUMN_1 };
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Gerenciador de saves", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveSaveManager", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawSaveManager();
    });

    path.sidebarName = "Touch & Motion";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Touch & Motion", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveTouchMotion", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawTouchAndMotion();
    });

    path.sidebarName = "HUD";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "HUD Editor", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveHud", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawHudEditor();
    });

    path.sidebarName = "Conquistas";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Conquistas", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveAchievements", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawAchievements();
    });

    path.sidebarName = "Boss Rush";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Boss Rush: recordes e ranking", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveBossRush", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawBossRush();
    });

    path.sidebarName = "Master Mode";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Master Mode", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveMasterMode", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawMasterMode();
    });

    path.sidebarName = "Recursos";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Configurações Definitive", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveFeatures", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawFeatures();
    });
}

} // namespace SohGui
