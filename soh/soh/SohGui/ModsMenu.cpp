#include "ModsMenu.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <set>
#include <imgui.h>
#include <libultraship/libultraship.h>
#include <utils/StringHelper.h>

#include "UIWidgets.hpp"
#include "soh/OTRGlobals.h"
#include <Fast3D/gfx_pc.h>
#include <resource/ResourceType.h>

extern "C" void OTRMessage_ReloadTables();

#ifdef __ANDROID__
#include <SDL2/SDL.h>
#include <jni.h>
#else
#include "soh/Extractor/portable-file-dialogs.h"
#endif

namespace ModsMenu {

static const char* sDisabledSuffix = ".desativado";

// A mod switched in the menu, applied to the loaded archives at the end of the frame.
struct PendingChange {
    std::string oldPath; // path of the file before the rename
    std::string newPath; // path after the rename
    bool enable;
};
static std::vector<PendingChange> sPendingChanges;
// Shown under the list: what was applied at once and what needs a restart.
static std::string sApplyMessage;

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
    if (ec) {
        return false;
    }
    sPendingChanges.push_back({ path.string(), target.string(), enabled });
    return true;
}

static std::string NormalPath(const std::string& path) {
    std::error_code ec;
    std::filesystem::path absolute = std::filesystem::absolute(path, ec);
    return (ec ? std::filesystem::path(path) : absolute).lexically_normal().generic_string();
}

// Marks the cached resources of a mod's files as dirty, so the next use loads them from the archives as they are
// now. Only textures and text are refreshed at once: the game draws textures by name every frame and the text tables
// are rebuilt below, while models, animations and sounds are held by pointer and could be in use right now.
static void DirtyModFiles(const std::shared_ptr<std::unordered_map<uint64_t, std::string>>& files, bool& textChanged,
                          bool& othersChanged) {
    if (files == nullptr) {
        return;
    }
    auto resourceManager = Ship::Context::GetInstance()->GetResourceManager();
    for (const auto& [hash, name] : *files) {
        const std::string base = StringHelper::StartsWith(name, "alt/") ? name.substr(4) : name;
        const bool isText = StringHelper::StartsWith(base, "text/") || StringHelper::StartsWith(base, "override/text/");
        if (base.find("staff_message_data_static") != std::string::npos) {
            // The credits table is only read at startup.
            othersChanged = true;
            continue;
        }
        if (isText) {
            textChanged = true;
        }
        // With alternate assets on, a cached "alt/x" wins over "x", and a cached "x" hides a new "alt/x": both go.
        for (const std::string& key : { name, base }) {
            auto resource = resourceManager->GetCachedResource(key, true);
            if (resource == nullptr) {
                continue;
            }
            auto initData = resource->GetInitData();
            if (isText || (initData != nullptr && initData->Type == static_cast<uint32_t>(Fast::ResourceType::Texture))) {
                resource->Dirty();
            } else {
                othersChanged = true;
            }
        }
    }
}

void ApplyPendingChanges() {
    if (sPendingChanges.empty()) {
        return;
    }
    auto archiveManager = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager();
    bool textChanged = false;
    bool othersChanged = false;
    bool failed = false;

    for (const PendingChange& change : sPendingChanges) {
        if (change.enable) {
            auto archive = archiveManager->AddArchive(change.newPath);
            if (archive == nullptr) {
                failed = true;
                continue;
            }
            DirtyModFiles(archive->ListFiles(), textChanged, othersChanged);
        } else {
            const std::string oldPath = NormalPath(change.oldPath);
            std::shared_ptr<Ship::Archive> found = nullptr;
            auto archives = archiveManager->GetArchives();
            if (archives != nullptr) {
                for (const auto& archive : *archives) {
                    if (NormalPath(archive->GetPath()) == oldPath) {
                        found = archive;
                        break;
                    }
                }
            }
            if (found == nullptr) {
                // It was not loaded (for example turned on and off again before a restart).
                continue;
            }
            auto files = found->ListFiles();
            archiveManager->RemoveArchive(found->GetPath());
            DirtyModFiles(files, textChanged, othersChanged);
        }
    }
    sPendingChanges.clear();

    gfx_texture_cache_clear();
    if (textChanged) {
        OTRMessage_ReloadTables();
    }

    if (failed) {
        sApplyMessage = "Um dos mods não pôde ser carregado agora. Feche e abra o jogo para tentar de novo.";
    } else if (othersChanged) {
        sApplyMessage = "Texturas e textos já foram trocados. Modelos, sons e o resto do mod valem quando você fechar "
                        "e abrir o jogo.";
    } else {
        sApplyMessage = "Aplicado.";
    }
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
        message += "Mod instalado:" + installed;
    }
    if (!failed.empty()) {
        message += (message.empty() ? "" : "\n\n") + std::string("Não foi possível instalar:") + failed;
    }
    return message;
}
#endif

static bool IsLoaded(const std::string& path) {
    auto archives = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetArchives();
    if (archives == nullptr) {
        return false;
    }
    const std::string wanted = NormalPath(path);
    for (const auto& archive : *archives) {
        if (NormalPath(archive->GetPath()) == wanted) {
            return true;
        }
    }
    return false;
}

// A mod that just arrived in the folder (installed by the button) is loaded right away, once.
static void QueueNewMods(const std::vector<ModFile>& mods) {
    static std::set<std::string> sTried;
    for (const ModFile& mod : mods) {
        if (!mod.enabled || sTried.count(mod.path) != 0) {
            continue;
        }
        sTried.insert(mod.path);
        bool pending = false;
        for (const PendingChange& change : sPendingChanges) {
            pending = pending || change.newPath == mod.path;
        }
        if (!pending && !IsLoaded(mod.path)) {
            sPendingChanges.push_back({ mod.path, mod.path, true });
            sApplyMessage = "Aplicando...";
        }
    }
}

void Draw() {
    static std::vector<ModFile> sMods;
    static std::string sMessage;
    static double sNextScan = 0.0;
    // Rescan now and then, so a file the app copied in (Android picker) shows up without reopening the menu.
    if (ImGui::GetTime() >= sNextScan) {
        sMods = List();
        sNextScan = ImGui::GetTime() + 1.0;
        QueueNewMods(sMods);
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
            sApplyMessage = "Aplicando...";
            sNextScan = 0.0;
        }
    }
    if (!sApplyMessage.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", sApplyMessage.c_str());
    }
}

} // namespace ModsMenu
