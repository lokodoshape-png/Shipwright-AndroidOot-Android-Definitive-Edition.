// OOT D.E. Definitive Expansion: game glue and menu pages of the expansion core (settings + save manager).
#include "DefinitiveExpansion.h"
#include "DEStorage.h"

#include <imgui.h>
#include <libultraship/libultraship.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <mutex>

#include "soh/OTRGlobals.h"
#include "soh/SaveManager.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/UIWidgets.hpp"

extern "C" {
#include "z64.h"
extern PlayState* gPlayState;
}

namespace fs = std::filesystem;

namespace DefinitiveExpansion {

// Setting keys. Each phase adds its own group ("touch.", "hud.", ...).
static const char* kAutoBackup = "saves.autoBackup";
static const char* kAutoBackupMinutes = "saves.autoBackupIntervalMinutes";
static const char* kAutoBackupKeep = "saves.autoBackupKeep";
static const char* kBackupBeforeDelete = "saves.backupBeforeDeleteOrCopy";

static std::once_flag sInitOnce;
static std::unique_ptr<DE::Config> sConfig;
static std::unique_ptr<DE::SaveVault> sVault;
static DE::Config::LoadResult sLoadResult = DE::Config::LoadResult::Loaded;

fs::path DataDir() {
    return fs::path(Ship::Context::GetPathRelativeToAppDirectory("ootde"));
}

static void InitOnce() {
    std::call_once(sInitOnce, []() {
        sConfig = std::make_unique<DE::Config>();
        sConfig->RegisterSetting(kAutoBackup, true,
                                 "Guarda uma cópia do save antes de o jogo gravar por cima dele.");
        sConfig->RegisterSetting(kAutoBackupMinutes, 10, "Intervalo mínimo entre backups automáticos (minutos).");
        sConfig->RegisterSetting(kAutoBackupKeep, 10, "Quantos backups automáticos manter por slot.");
        sConfig->RegisterSetting(kBackupBeforeDelete, true,
                                 "Guarda uma cópia antes de apagar um save ou copiar outro por cima.");
        RegisterControlSettings(*sConfig);
        RegisterAchievementSettings(*sConfig);
        RegisterChallengeSettings(*sConfig);
        RegisterLinkSettings(*sConfig);
        sLoadResult = sConfig->Load(DataDir() / "config.json");
        if (sLoadResult == DE::Config::LoadResult::RecoveredBroken) {
            SPDLOG_WARN("OOT D.E.: ootde/config.json estava corrompido; foi guardado à parte e os padrões foram usados");
        }
        sVault = std::make_unique<DE::SaveVault>(fs::path(Ship::Context::GetPathRelativeToAppDirectory("Save")),
                                                 DataDir());
    });
}

DE::Config& Config() {
    InitOnce();
    return *sConfig;
}

DE::SaveVault& Vault() {
    InitOnce();
    return *sVault;
}

void SaveConfig() {
    std::string error;
    if (!Config().Save(&error)) {
        SPDLOG_ERROR("OOT D.E.: não foi possível gravar ootde/config.json: {}", error);
    }
}

void BeforeSaveOverwrite(int fileNum) {
    if (fileNum < 0 || fileNum >= DE::SaveVault::kSlots || !Config().GetBool(kAutoBackup)) {
        return;
    }
    const int minutes = std::max(0, Config().GetInt(kAutoBackupMinutes));
    if (Vault().AutoBackupBeforeOverwrite(fileNum, static_cast<int64_t>(minutes) * 60,
                                          std::max(1, Config().GetInt(kAutoBackupKeep)))) {
        SPDLOG_INFO("OOT D.E.: backup automático do slot {}", fileNum + 1);
    }
}

static void SafetyBackup(int fileNum, DE::BackupKind kind) {
    if (fileNum < 0 || fileNum >= DE::SaveVault::kSlots || !Config().GetBool(kBackupBeforeDelete)) {
        return;
    }
    std::string error;
    if (fs::exists(Vault().SlotPath(fileNum)) && !Vault().CreateBackup(fileNum, kind, &error)) {
        SPDLOG_ERROR("OOT D.E.: backup de segurança do slot {} falhou: {}", fileNum + 1, error);
    }
}

void BeforeSlotDeleted(int fileNum) {
    SafetyBackup(fileNum, DE::BackupKind::BeforeDelete);
}

void BeforeSlotCopiedOver(int fileNum) {
    SafetyBackup(fileNum, DE::BackupKind::BeforeCopy);
}

// ---------------------------------------------------------------------------------------------------------------
// Menu: Save manager
// ---------------------------------------------------------------------------------------------------------------

static const ImVec4 kGold(1.0f, 0.82f, 0.30f, 1.0f);
static const ImVec4 kOk(0.45f, 0.85f, 0.45f, 1.0f);
static const ImVec4 kBad(1.0f, 0.45f, 0.40f, 1.0f);

struct ViewCache {
    double nextRefresh = 0.0;
    DE::SaveInfo slots[DE::SaveVault::kSlots];
    std::vector<DE::BackupEntry> backups[DE::SaveVault::kSlots];
    std::vector<std::pair<fs::path, DE::SaveInfo>> importable;
};
static ViewCache sView;
static std::string sMessage;
static bool sMessageIsError = false;

static void SetMessage(const std::string& text, bool isError) {
    sMessage = text;
    sMessageIsError = isError;
    sView.nextRefresh = 0.0;
}

static void RefreshView() {
    if (ImGui::GetTime() < sView.nextRefresh) {
        return;
    }
    // Files on disk can change behind the menu (autosave, a file copied in by the player), so look again now and then.
    sView.nextRefresh = ImGui::GetTime() + 3.0;
    for (int slot = 0; slot < DE::SaveVault::kSlots; slot++) {
        sView.slots[slot] = DE::InspectSave(Vault().SlotPath(slot));
        sView.backups[slot] = Vault().ListBackups(slot);
    }
    sView.importable.clear();
    for (const fs::path& file : Vault().ListImportable()) {
        sView.importable.emplace_back(file, DE::InspectSave(file));
    }
}

static std::string Describe(const DE::SaveInfo& info) {
    if (!info.exists) {
        return "Vazio";
    }
    if (!info.valid) {
        return "Inválido: " + info.problem;
    }
    std::string text = (info.playerName.empty() ? std::string("(sem nome)") : info.playerName) + "  |  " +
                       std::to_string(info.hearts) + " corações  |  " + std::to_string(info.deaths) + " mortes  |  " +
                       std::to_string(info.skulltulas) + " Skulltulas";
    if (info.randomizer) {
        text += "  |  Randomizer";
    }
    if (info.masterQuest) {
        text += "  |  Master Quest";
    }
    return text;
}

// Replacing a save while a game is running would let the running game save over it again, and the file select
// screen keeps the slot summaries it read at boot. So restore/import are offered only outside gameplay, and the game
// goes back to the start afterwards, which reloads every slot (z_title.c -> Sram_InitSram -> Save_Init).
static bool CanReplaceSaves() {
    return gPlayState == nullptr;
}

static void RestartToTitle() {
    std::reinterpret_pointer_cast<Ship::ConsoleWindow>(
        Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"))
        ->Dispatch("reset");
}

static void ConfirmReplace(int slot, const fs::path& source, DE::BackupKind safetyKind, const std::string& what) {
    const DE::SaveInfo& current = sView.slots[slot];
    std::string message = what + "\n\nSlot " + std::to_string(slot + 1) + " agora: " + Describe(current) +
                          "\nNovo conteúdo: " + Describe(DE::InspectSave(source)) + "\n\n";
    if (current.exists) {
        message += "O save atual será guardado nos backups antes, então dá para desfazer.\n";
    }
    message += "Depois o jogo volta para a tela inicial para recarregar os saves.";
    SohGui::RegisterPopup("Substituir save do slot " + std::to_string(slot + 1) + "?", message, "Substituir",
                          "Cancelar", [slot, source, safetyKind]() {
                              if (!CanReplaceSaves()) {
                                  SetMessage("Volte para a tela de título antes de substituir um save.", true);
                                  return;
                              }
                              SaveManager::Instance->ThreadPoolWait(); // no save may still be on its way to disk
                              std::string error;
                              if (Vault().RestoreInto(slot, source, safetyKind, &error)) {
                                  SetMessage("Slot " + std::to_string(slot + 1) + " substituído. Recarregando...",
                                             false);
                                  RestartToTitle();
                              } else {
                                  SetMessage("Nada foi alterado: " + error, true);
                              }
                          });
}

static void DrawSlot(int slot) {
    const DE::SaveInfo& info = sView.slots[slot];
    ImGui::PushID(slot);
    ImGui::SeparatorText(("Slot " + std::to_string(slot + 1)).c_str());
    ImGui::TextColored(!info.exists ? ImVec4(0.7f, 0.7f, 0.7f, 1.0f) : (info.valid ? kOk : kBad), "%s",
                       Describe(info).c_str());

    ImGui::BeginDisabled(!info.exists);
    if (UIWidgets::Button("Fazer backup agora", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
        SaveManager::Instance->ThreadPoolWait();
        std::string error;
        fs::path created;
        if (Vault().CreateBackup(slot, DE::BackupKind::Manual, &error, &created)) {
            SetMessage("Backup criado: " + created.filename().string(), false);
        } else {
            SetMessage("Backup não criado: " + error, true);
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!info.valid);
    if (UIWidgets::Button("Exportar", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
        SaveManager::Instance->ThreadPoolWait();
        std::string error;
        fs::path created;
        if (Vault().Export(slot, &error, &created)) {
            SetMessage("Exportado para " + created.string(), false);
        } else {
            SetMessage("Não exportado: " + error, true);
        }
    }
    ImGui::EndDisabled();

    const auto& backups = sView.backups[slot];
    const std::string header = "Backups (" + std::to_string(backups.size()) + ")";
    if (ImGui::TreeNode(header.c_str())) {
        if (backups.empty()) {
            ImGui::TextWrapped("%s", "Nenhum backup deste slot ainda.");
        }
        for (const DE::BackupEntry& entry : backups) {
            ImGui::PushID(entry.path.filename().string().c_str());
            ImGui::Text("%s  -  %s", DE::TimestampForDisplay(entry.time).c_str(), DE::BackupKindLabel(entry.kind));
            ImGui::SameLine();
            ImGui::BeginDisabled(!CanReplaceSaves());
            if (UIWidgets::Button("Restaurar", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
                ConfirmReplace(slot, entry.path, DE::BackupKind::BeforeRestore,
                               "Restaurar o backup de " + DE::TimestampForDisplay(entry.time) + "?");
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (UIWidgets::Button("Apagar", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
                const DE::BackupEntry copy = entry;
                SohGui::RegisterPopup("Apagar backup?",
                                      "O backup de " + DE::TimestampForDisplay(entry.time) + " do slot " +
                                          std::to_string(slot + 1) + " será apagado para sempre. O save do slot "
                                                                     "não muda.",
                                      "Apagar", "Cancelar", [copy]() {
                                          std::string error;
                                          if (Vault().DeleteBackup(copy, &error)) {
                                              SetMessage("Backup apagado.", false);
                                          } else {
                                              SetMessage("Backup não apagado: " + error, true);
                                          }
                                      });
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawSaveManager() {
    RefreshView();
    ImGui::TextWrapped("%s", "Os saves continuam no formato original do jogo. Backups e exportações ficam em:");
    ImGui::TextColored(kGold, "%s", DataDir().string().c_str());
    if (!CanReplaceSaves()) {
        ImGui::TextColored(kGold, "%s",
                           "Restaurar e importar só funcionam fora da partida (na tela de título ou na seleção de "
                           "arquivos), para o jogo em andamento não gravar por cima.");
    }
    if (!sMessage.empty()) {
        ImGui::TextColored(sMessageIsError ? kBad : kOk, "%s", sMessage.c_str());
    }

    for (int slot = 0; slot < DE::SaveVault::kSlots; slot++) {
        DrawSlot(slot);
    }

    ImGui::SeparatorText("Importar");
    ImGui::TextWrapped("Coloque arquivos .sav (exportados por este jogo ou pelo Ship of Harkinian) nesta pasta e "
                       "escolha o slot de destino:");
    ImGui::TextColored(kGold, "%s", Vault().ExportDir().string().c_str());
    if (sView.importable.empty()) {
        ImGui::TextWrapped("%s", "Nenhum arquivo para importar.");
    }
    for (const auto& [file, info] : sView.importable) {
        ImGui::PushID(file.filename().string().c_str());
        ImGui::TextColored(info.valid ? kOk : kBad, "%s", file.filename().string().c_str());
        ImGui::TextWrapped("   %s", Describe(info).c_str());
        ImGui::BeginDisabled(!info.valid || !CanReplaceSaves());
        for (int slot = 0; slot < DE::SaveVault::kSlots; slot++) {
            if (slot > 0) {
                ImGui::SameLine();
            }
            const std::string label = "Para o slot " + std::to_string(slot + 1);
            if (UIWidgets::Button(label.c_str(), UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
                ConfirmReplace(slot, file, DE::BackupKind::BeforeImport,
                               "Importar " + file.filename().string() + "?");
            }
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Menu: Definitive settings and status of the expansion systems
// ---------------------------------------------------------------------------------------------------------------

struct SystemStatus {
    const char* name;
    const char* status;
};

// What each system of the expansion is today. Only what really works is listed as available.
static const SystemStatus kSystems[] = {
    { "Gerenciador de saves", "Disponível (Definitive > Saves)" },
    { "Configurações Definitive", "Disponível (esta página)" },
    { "Touch & Motion", "Disponível (Definitive > Touch & Motion)" },
    { "HUD Editor", "Disponível (Definitive > HUD): perfis, posição e visibilidade. Tamanho e transparência por "
                    "elemento ainda não" },
    { "3D Museum", "Parcial (Definitive > 3D Museum): catálogo com ícones do jogo, descrição, zoom e desbloqueio real. "
                   "Modelos 3D com rotação ainda não" },
    { "Link Customization", "Disponível (Definitive > Link Customization): cores com prévia, aplicar, restaurar, 3 "
                            "visuais guardados e visuais desbloqueados por conquistas" },
    { "Hyrule Explorer", "Disponível (Definitive > Hyrule Explorer): mapa das áreas visitadas, progresso por região e "
                         "álbum (personagens, chefes, inimigos, itens, eventos, lugares)" },
    { "Achievements", "Disponível (Definitive > Conquistas)" },
    { "Boss Rush", "Disponível (Definitive > Boss Rush): recordes, ranking e histórico das corridas do Boss Rush "
                   "do Ship of Harkinian. Desafio de tempo ainda não" },
    { "Master Mode", "Disponível (Definitive > Master Mode): presets feitos dos modificadores que o jogo já tem" },
    { "Randomizer", "Disponível (Definitive > Randomizer): o Randomizer do Ship of Harkinian com geração por semente, "
                    "código para compartilhar e histórico de partidas" },
};

static char sProfileName[48] = "";

void DrawFeatures() {
    DE::Config& config = Config();
    ImGui::TextWrapped("%s", "Configurações da expansão Definitive. Ficam num arquivo próprio, separado dos saves e "
                             "das configurações do Ship of Harkinian:");
    ImGui::TextColored(kGold, "%s", config.Path().string().c_str());
    ImGui::Text("Formato da configuração: v%d", DE::Config::kSchemaVersion);
    if (sLoadResult == DE::Config::LoadResult::RecoveredBroken) {
        ImGui::TextColored(kBad, "%s", "O arquivo anterior estava corrompido: foi guardado à parte e os padrões foram "
                                       "usados.");
    } else if (sLoadResult == DE::Config::LoadResult::NewerVersion) {
        ImGui::TextColored(kGold, "%s", "O arquivo foi criado por uma versão mais nova; uma cópia foi guardada (.bak).");
    }

    bool changed = false;
    ImGui::SeparatorText("Backups de save");
    bool autoBackup = config.GetBool(kAutoBackup);
    if (UIWidgets::Checkbox("Backup automático antes de gravar", &autoBackup,
                            UIWidgets::CheckboxOptions().Tooltip(
                                "Antes de o jogo gravar por cima de um save, guarda uma cópia do save anterior."))) {
        changed |= config.Set(kAutoBackup, autoBackup);
    }
    int minutes = config.GetInt(kAutoBackupMinutes);
    if (UIWidgets::SliderInt("Intervalo mínimo entre backups: %d min", &minutes,
                             UIWidgets::IntSliderOptions().Min(0).Max(120).DefaultValue(10))) {
        changed |= config.Set(kAutoBackupMinutes, minutes);
    }
    int keep = config.GetInt(kAutoBackupKeep);
    if (UIWidgets::SliderInt("Backups automáticos por slot: %d", &keep,
                             UIWidgets::IntSliderOptions().Min(1).Max(50).DefaultValue(10))) {
        changed |= config.Set(kAutoBackupKeep, keep);
    }
    bool beforeDelete = config.GetBool(kBackupBeforeDelete);
    if (UIWidgets::Checkbox("Backup antes de apagar ou copiar por cima", &beforeDelete,
                            UIWidgets::CheckboxOptions().Tooltip(
                                "Na seleção de arquivos, guarda o save antes de ele ser apagado ou substituído por "
                                "uma cópia."))) {
        changed |= config.Set(kBackupBeforeDelete, beforeDelete);
    }

    ImGui::SeparatorText("Perfis");
    ImGui::TextWrapped("%s", "Um perfil guarda todas as configurações Definitive para aplicar depois.");
    ImGui::InputText("##deprofile", sProfileName, sizeof(sProfileName));
    ImGui::SameLine();
    if (UIWidgets::Button("Salvar perfil", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline)) &&
        sProfileName[0] != '\0') {
        changed |= config.SaveProfile(sProfileName);
        sProfileName[0] = '\0';
    }
    for (const std::string& name : config.ListProfiles()) {
        ImGui::PushID(name.c_str());
        ImGui::Text("%s", name.c_str());
        ImGui::SameLine();
        if (UIWidgets::Button("Aplicar", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
            changed |= config.ApplyProfile(name);
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Apagar", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
            changed |= config.DeleteProfile(name);
        }
        ImGui::PopID();
    }

    ImGui::SeparatorText("Padrões");
    if (UIWidgets::Button("Restaurar configurações padrão",
                          UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
        SohGui::RegisterPopup("Restaurar padrões?",
                              "Todas as configurações Definitive voltam ao padrão. Perfis, saves e backups não "
                              "mudam.",
                              "Restaurar", "Cancelar", []() {
                                  Config().ResetToDefaults();
                                  SaveConfig();
                              });
    }

    if (changed) {
        SaveConfig();
    }

    ImGui::SeparatorText("Sistemas da expansão");
    if (ImGui::BeginTable("##desystems", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
        for (const SystemStatus& system : kSystems) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(kGold, "%s", system.name);
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", system.status);
        }
        ImGui::EndTable();
    }
}

} // namespace DefinitiveExpansion
