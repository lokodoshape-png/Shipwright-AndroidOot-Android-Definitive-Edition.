#pragma once

#include <string>
#include <vector>

// "Mods" page of the port menu: lists the archives in <app folder>/mods, turns them on and off, and installs new
// ones from a file the player picks. Turning a mod on or off applies at once to textures and text; models, sounds
// and other data of the mod take effect on the next launch.
namespace ModsMenu {

struct ModFile {
    std::string path;
    std::string name; // file name without the ".desativado" suffix
    bool enabled;
};

std::string GetModsFolder();

// Archives in the mods folder, sorted by name. Enabled ones end in .otr, .o2r or .zip; disabled ones also end in
// ".desativado", which the loader (OTRGlobals.cpp) skips because it only loads those extensions.
std::vector<ModFile> List();

// Adds or removes the ".desativado" suffix. Returns false if the rename failed.
bool SetEnabled(const ModFile& mod, bool enabled);

// Opens the system file picker. On Android the app copies the chosen files into the mods folder itself; on PC they
// are copied here. Returns what happened (empty when the copy happens later, on Android).
std::string OpenPicker();

void Draw();

// Adds or removes the archives of the mods switched in the menu, between two frames (called from OTRGlobals.cpp, at
// the end of the frame, where the alternate assets switch is applied too).
void ApplyPendingChanges();

} // namespace ModsMenu
