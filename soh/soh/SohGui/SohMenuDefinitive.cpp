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

    path.sidebarName = "Recursos";
    AddSidebarEntry("Definitive", path.sidebarName, 1);
    AddWidget(path, "Configurações Definitive", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "DefinitiveFeatures", WIDGET_CUSTOM).RaceDisable(false).CustomFunction([](WidgetInfo& info) {
        DefinitiveExpansion::DrawFeatures();
    });
}

} // namespace SohGui
