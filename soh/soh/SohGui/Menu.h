#ifndef MENU_H
#define MENU_H

#include <libultraship/libultraship.h>
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "MenuTypes.h"

namespace Ship {
uint32_t GetVectorIndexOf(std::vector<std::string>& vector, std::string value);
class Menu : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    Menu(const std::string& cVar, const std::string& name, uint8_t searchSidebarIndex_ = 0,
         UIWidgets::Colors menuThemeIndex_ = UIWidgets::Colors::LightBlue);

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
    void Draw() override;
    void InsertSidebarSearch();
    void RemoveSidebarSearch();
    void UpdateWindowBackendObjects();
    bool IsMenuPopped();
    UIWidgets::Colors GetMenuThemeColor();

    void MenuDrawItem(WidgetInfo& widget, uint32_t width, UIWidgets::Colors menuThemeIndex);
    void AddMenuEntry(std::string entryName, const char* entryCvar);
    std::unordered_map<uint32_t, disabledInfo>& GetDisabledMap();

    // OOT D.E.: the Definitive Edition main menu shows the pages of this menu inside its own screens.
    const std::vector<std::string>& GetHeaderOrder() const;
    // The sidebars of a header, in order (empty when the header does not exist).
    std::vector<std::string> GetSidebarOrder(const std::string& header) const;
    // Translated names, as this menu shows them.
    static std::string HeaderDisplayName(const std::string& header);
    static std::string SidebarDisplayName(const std::string& sidebar);
    // Draws the options of one sidebar in the current window (same widgets and rules as this menu), using up to
    // `width` pixels. Returns false when the sidebar does not exist.
    bool DrawSection(const std::string& header, const std::string& sidebar, float width);

  protected:
    ImVec2 mOriginalSize;
    std::string mName;
    uint32_t mWindowFlags;
    std::unordered_map<std::string, MainMenuEntry> menuEntries;
    std::vector<std::string> menuOrder;
    uint32_t DrawSearchResults(std::string& menuSearchText);
    ImGuiTextFilter menuSearch;
    uint8_t searchSidebarIndex;
    UIWidgets::Colors defaultThemeIndex;
    std::shared_ptr<std::vector<Ship::WindowBackend>> availableWindowBackends;
    std::unordered_map<Ship::WindowBackend, const char*> availableWindowBackendsMap;
    Ship::WindowBackend configWindowBackend;

    std::unordered_map<uint32_t, disabledInfo> disabledMap;
    std::vector<disabledInfo> disabledVector;
    const SidebarEntry searchSidebarEntry = {
        .columnCount = 1,
        .columnWidgets = { { { .name = "Sidebar Search",
                               .type = WIDGET_SEARCH,
                               .options = std::make_shared<UIWidgets::WidgetOptions>(UIWidgets::WidgetOptions{}.Tooltip(
                                   "Searches all menus for the given text, including tooltips.")) } } }
    };

  private:
    bool allowPopout = true; // PortNote: should be set to false on small screen ports
    bool popped;
    ImVec2 poppedSize;
    ImVec2 poppedPos;
    float windowHeight;
    float windowWidth;
    UIWidgets::Colors menuThemeIndex;
};
} // namespace Ship

#endif // MENU_H
