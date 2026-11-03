#include "ModsMenu.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <libultraship/libultraship.h>
#include <utils/StringHelper.h>

#include "UIWidgets.hpp"
#include "soh/OTRGlobals.h"

#ifdef __ANDROID__
#include <SDL2/SDL.h>
#include <jni.h>
#else
#include "soh/Extractor/portable-file-dialogs.h"
#endif

namespace ModsMenu {

static const char* sDisabledSuffix = ".desativado";

static bool IsArchiveName(const std::string& name) {
    const std::string extension = std::filesystem::path(name).extension().string();
    return StringHelper::IEquals(extension, ".otr") || StringHelper::IEquals(extension, ".o2r") ||
           StringHelper::IEquals(extension, ".zip") || StringHelper::IEquals(extension, ".mpq");
}

std::string GetModsFolder() {
    // Same lookup the loader uses (OTRGlobals.cpp), so the menu shows the folder the game really reads.
    std::string folder = Ship::Context::LocateFileAcrossAppDirs("mods", appShortName);
    if (folder.empty() || !std::filesystem::exists(folder)) {
        folder = Ship::Context::GetPathRelativeToAppDirectory("mods", appShortName);
    }
    return folder;
}

std::vector<ModFile> List() {
    std::vector<ModFile> mods;
    std::error_code ec;
    const std::string folder = GetModsFolder();
    if (!std::filesystem::is_directory(folder, ec)) {
        return mods;
    }
    for (auto it = std::filesystem::recursive_directory_iterator(folder, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) {
            continue;
        }
        std::string name = it->path().filename().string();
        bool enabled = true;
        if (StringHelper::EndsWith(name, sDisabledSuffix)) {
            name = name.substr(0, name.size() - strlen(sDisabledSuffix));
            enabled = false;
        }
        if (IsArchiveName(name)) {
            mods.push_back({ it->path().string(), name, enabled });
        }
    }
    std::sort(mods.begin(), mods.end(), [](const ModFile& a, const ModFile& b) { return a.name < b.name; });
    return mods;
}

bool SetEnabled(const ModFile& mod, bool enabled) {
    if (mod.enabled == enabled) {
        return true;
    }
    const std::filesystem::path path(mod.path);
    const std::filesystem::path target = path.parent_path() / (enabled ? mod.name : mod.name + sDisabledSuffix);
    std::error_code ec;
    std::filesystem::rename(path, target, ec);
    return !ec;
}

#ifdef __ANDROID__
std::string OpenPicker() {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (env == nullptr || activity == nullptr) {
        return "";
    }
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(activityClass, "openModPicker", "()V");
    if (method != nullptr) {
        env->CallVoidMethod(activity, method);
    } else {
        env->ExceptionClear();
    }
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(activity);
    return "";
}
#else
std::string OpenPicker() {
    std::vector<std::string> selection =
        pfd::open_file("Escolha um mod", ".", { "Mods (.otr .o2r .zip)", "*.otr *.o2r *.zip" }, pfd::opt::multiselect)
            .result();
    if (selection.empty()) {
        return "";
    }
    const std::filesystem::path folder = GetModsFolder();
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);

    std::string installed, failed;
    for (const std::string& file : selection) {
        const std::filesystem::path source(file);
        const std::string name = source.filename().string();
        if (!IsArchiveName(name)) {
            failed += "\n" + name + " (formato não suportado)";
            continue;
        }
        std::filesystem::copy_file(source, folder / name, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            failed += "\n" + name + " (" + ec.message() + ")";
        } else {
            installed += "\n" + name;
        }
    }
    std::string message;
    if (!installed.empty()) {
        message += "Mod instalado. Feche e abra o jogo para usar:" + installed;
    }
    if (!failed.empty()) {
        message += (message.empty() ? "" : "\n\n") + std::string("Não foi possível instalar:") + failed;
    }
    return message;
}
#endif

void Draw() {
    static std::vector<ModFile> sMods;
    static std::string sMessage;
    static bool sChanged = false;
    static double sNextScan = 0.0;
    // Rescan now and then, so a file the app copied in (Android picker) shows up without reopening the menu.
    if (ImGui::GetTime() >= sNextScan) {
        sMods = List();
        sNextScan = ImGui::GetTime() + 1.0;
    }

    if (UIWidgets::Button("Adicionar mod...", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
        sMessage = OpenPicker();
        sNextScan = 0.0;
    }
    ImGui::TextWrapped("%s", "Aceita mods do Ship of Harkinian em .otr, .o2r ou .zip. No Android, os arquivos .otr e "
                             ".o2r de dentro de um .zip são extraídos. Os mods ficam nesta pasta:");
    ImGui::TextWrapped("%s", GetModsFolder().c_str());
    if (!sMessage.empty()) {
        ImGui::TextWrapped("%s", sMessage.c_str());
    }

    ImGui::SeparatorText("Mods instalados");
    if (sMods.empty()) {
        ImGui::TextWrapped("%s", "Nenhum mod instalado.");
    }
    for (ModFile& mod : sMods) {
        bool enabled = mod.enabled;
        // The file name is not menu text, so it goes straight to ImGui instead of through the translated widgets.
        if (ImGui::Checkbox(mod.name.c_str(), &enabled) && SetEnabled(mod, enabled)) {
            sChanged = true;
            sNextScan = 0.0;
        }
    }
    if (sChanged) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", "Feche e abra o jogo para aplicar.");
    }
}

} // namespace ModsMenu
