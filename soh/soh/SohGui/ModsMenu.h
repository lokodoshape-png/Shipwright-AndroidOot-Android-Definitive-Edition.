#pragma once

#include <string>
#include <vector>

// "Mods" page of the port menu: lists the archives in <app folder>/mods, turns them on and off, and installs new
// ones from a file the player picks. Mods are loaded when the game starts, so changes apply on the next launch.
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

} // namespace ModsMenu
