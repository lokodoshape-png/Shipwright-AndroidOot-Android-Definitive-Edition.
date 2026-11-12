// OOT D.E. Definitive Expansion, phase 7: Randomizer page and history.
//
// The randomizer is the Ship of Harkinian one, untouched: its generator, logic, settings, spoiler log and the seed
// signature shown on file select. A seed with the same settings always gives the same game. This page only:
// - shows the seed that is ready (code and signature) and lets the player copy it to share;
// - generates a seed (random or typed) by calling the same function as the Randomizer Settings window;
// - keeps a history of the randomizer saves played (ootde/randomizer_history.json): seed, signature, slot, dates,
//   and whether the game was finished, read from the save when it is loaded and when the game is completed.
#include "DefinitiveExpansion.h"
#include "DEStorage.h"

#include <imgui.h>
#include <libultraship/libultraship.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/gameplaystats.h"
#include "soh/Enhancements/randomizer/context.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/SohGui/UIWidgets.hpp"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

using nlohmann::json;
namespace fs = std::filesystem;

namespace DefinitiveExpansion {

namespace {

const ImVec4 kGold(1.0f, 0.82f, 0.30f, 1.0f);
const ImVec4 kDim(0.62f, 0.64f, 0.70f, 1.0f);
const ImVec4 kOk(0.45f, 0.85f, 0.45f, 1.0f);
constexpr int kHistorySchema = 1;
constexpr size_t kMaxHistory = 100;

struct Played {
    std::string seed;
    std::string signature; // the 5 seed icons, as numbers
    int slot = 0;          // 1-3
    uint64_t createdAt = 0; // save creation time (ms), identifies the run together with the slot
    int64_t firstSeen = 0;
    int64_t lastPlayed = 0;
    bool finished = false;
    uint32_t timeTenths = 0; // game time when finished, tenths of a second
};

std::vector<Played> sHistory; // newest first
bool sHistoryLoaded = false;

fs::path HistoryPath() {
    return DataDir() / "randomizer_history.json";
}

void LoadHistory() {
    sHistoryLoaded = true;
    sHistory.clear();
    std::string text;
    if (!DE::ReadWholeFile(HistoryPath(), text)) {
        return;
    }
    json doc = json::parse(text, nullptr, false);
    if (doc.is_discarded() || !doc.is_object() || !doc.contains("runs") || !doc["runs"].is_array()) {
        std::error_code ec;
        fs::rename(HistoryPath(), HistoryPath().string() + ".corrompido", ec);
        return;
    }
    if (doc.value("schemaVersion", 0) > kHistorySchema) {
        return; // a newer app wrote it: leave it alone
    }
    for (const json& entry : doc["runs"]) {
        if (!entry.is_object()) {
            continue;
        }
        Played run;
        run.seed = entry.value("seed", std::string());
        run.signature = entry.value("signature", std::string());
        run.slot = entry.value("slot", 0);
        run.createdAt = entry.value("createdAt", static_cast<uint64_t>(0));
        run.firstSeen = entry.value("firstSeen", static_cast<int64_t>(0));
        run.lastPlayed = entry.value("lastPlayed", static_cast<int64_t>(0));
        run.finished = entry.value("finished", false);
        run.timeTenths = entry.value("timeTenths", static_cast<uint32_t>(0));
        sHistory.push_back(run);
    }
}

void SaveHistory() {
    json runs = json::array();
    for (const Played& run : sHistory) {
        runs.push_back({ { "seed", run.seed },
                         { "signature", run.signature },
                         { "slot", run.slot },
                         { "createdAt", run.createdAt },
                         { "firstSeen", run.firstSeen },
                         { "lastPlayed", run.lastPlayed },
                         { "finished", run.finished },
                         { "timeTenths", run.timeTenths } });
    }
    json doc = { { "schemaVersion", kHistorySchema }, { "runs", runs } };
    std::string error;
    if (!DE::AtomicWriteFile(HistoryPath(), doc.dump(2) + "\n", &error)) {
        SPDLOG_ERROR("OOT D.E.: não foi possível gravar ootde/randomizer_history.json: {}", error);
    }
}

std::string Signature() {
    std::string text;
    for (uint8_t index : Rando::Context::GetInstance()->hashIconIndexes) {
        text += (text.empty() ? "" : "-") + std::to_string(index);
    }
    return text;
}

bool PlayingRandoSlot() {
    return gPlayState != nullptr && IS_RANDO && gSaveContext.fileNum >= 0 && gSaveContext.fileNum < 3;
}

void Record(bool finishing) {
    if (!PlayingRandoSlot()) {
        return;
    }
    if (!sHistoryLoaded) {
        LoadHistory();
    }
    const int slot = gSaveContext.fileNum + 1;
    const uint64_t created = gSaveContext.ship.stats.fileCreatedAt;
    auto it = std::find_if(sHistory.begin(), sHistory.end(),
                           [&](const Played& run) { return run.slot == slot && run.createdAt == created; });
    Played run;
    if (it != sHistory.end()) {
        run = *it;
        sHistory.erase(it);
    } else {
        run.slot = slot;
        run.createdAt = created;
        run.firstSeen = DE::NowUnixSeconds();
    }
    run.seed = Rando::Context::GetInstance()->GetSeedString();
    run.signature = Signature();
    run.lastPlayed = DE::NowUnixSeconds();
    if (finishing || gSaveContext.ship.stats.gameComplete) {
        run.finished = true;
        run.timeTenths = GAMEPLAYSTAT_TOTAL_TIME;
    }
    sHistory.insert(sHistory.begin(), run);
    if (sHistory.size() > kMaxHistory) {
        sHistory.resize(kMaxHistory);
    }
    SaveHistory();
}

bool sWasComplete = false;

void RegisterHooks() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) {
        sWasComplete = gSaveContext.ship.stats.gameComplete;
        Record(false);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        if (!PlayingRandoSlot()) {
            return;
        }
        const bool complete = gSaveContext.ship.stats.gameComplete;
        if (complete && !sWasComplete) {
            Record(true);
        }
        sWasComplete = complete;
    });
}

RegisterShipInitFunc sInit(RegisterHooks);

char sSeedInput[128] = "";

std::string FormatTenths(uint32_t tenths) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%u:%02u:%02u", tenths / 36000, (tenths / 600) % 60, (tenths / 10) % 60);
    return buffer;
}

} // namespace

void DrawRandomizer() {
    auto ctx = Rando::Context::GetInstance();
    ImGui::TextWrapped("%s", "O Randomizer é o do Ship of Harkinian, completo: dificuldade, itens, progressão, "
                             "truques e lógica de acesso ficam nas configurações dele. A mesma semente com as mesmas "
                             "configurações gera sempre o mesmo jogo, e a assinatura (os 5 ícones na seleção de "
                             "arquivos) confirma isso. Saves do Randomizer são separados da campanha.");
    if (UIWidgets::Button("Abrir as configurações do Randomizer", UIWidgets::ButtonOptions().Size(ImVec2(360.0f, 0.0f)))) {
        auto window = Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Randomizer Settings");
        if (window != nullptr) {
            window->Show();
        }
    }

    ImGui::SeparatorText("Semente pronta para o próximo arquivo");
    const bool generating = CVarGetInteger(CVAR_GENERAL("RandoGenerating"), 0) != 0;
    if (generating) {
        ImGui::TextColored(kGold, "%s", "Gerando a semente...");
    } else if (ctx->IsSeedGenerated() || ctx->IsSpoilerLoaded()) {
        ImGui::Text("Semente: %s", ctx->GetSeedString().c_str());
        ImGui::Text("Assinatura: %s", Signature().c_str());
        if (UIWidgets::Button("Copiar para compartilhar", UIWidgets::ButtonOptions().Size(ImVec2(260.0f, 0.0f)))) {
            const std::string share = "OOT D.E. Randomizer - semente " + ctx->GetSeedString() + " - assinatura " +
                                      Signature() + " (use as mesmas configurações)";
            ImGui::SetClipboardText(share.c_str());
        }
        ImGui::TextColored(kDim, "%s", "Crie um arquivo novo na seleção de arquivos para jogar esta semente.");
    } else {
        ImGui::TextColored(kDim, "%s", "Nenhuma semente gerada ainda.");
    }

    ImGui::SeparatorText("Gerar");
    const bool canGenerate = gSaveContext.gameMode == GAMEMODE_FILE_SELECT && !GameInteractor::IsSaveLoaded() &&
                             !generating;
    ImGui::SetNextItemWidth(300.0f);
    ImGui::InputTextWithHint("##DESeed", "Semente (vazio = aleatória)", sSeedInput, sizeof(sSeedInput),
                             ImGuiInputTextFlags_CallbackCharFilter, UIWidgets::TextFilters::FilterAlphaNum);
    ImGui::BeginDisabled(!canGenerate);
    if (UIWidgets::Button("Gerar semente", UIWidgets::ButtonOptions().Size(ImVec2(220.0f, 0.0f)))) {
        ctx->SetSpoilerLoaded(false);
        GenerateRandomizer(std::string(sSeedInput));
    }
    ImGui::EndDisabled();
    if (!canGenerate && !generating) {
        ImGui::TextColored(kDim, "%s", "Disponível só na seleção de arquivos, sem um save carregado.");
    }

    ImGui::SeparatorText("Histórico de partidas");
    if (!sHistoryLoaded) {
        LoadHistory();
    }
    if (sHistory.empty()) {
        ImGui::TextColored(kDim, "%s", "Nenhuma partida do Randomizer registrada ainda. Elas aparecem aqui ao carregar "
                                       "um save do Randomizer.");
        return;
    }
    if (ImGui::BeginTable("DERandoHistory", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("Semente", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Assinatura");
        ImGui::TableSetupColumn("Última vez");
        ImGui::TableSetupColumn("Resultado");
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < sHistory.size(); i++) {
            const Played& run = sHistory[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", run.slot);
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(run.seed.empty() ? "(sem semente)" : run.seed.c_str())) {
                std::strncpy(sSeedInput, run.seed.c_str(), sizeof(sSeedInput) - 1);
                ImGui::SetClipboardText(run.seed.c_str());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", "Toque para copiar e usar esta semente de novo.");
            }
            ImGui::PopID();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(run.signature.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(DE::TimestampForDisplay(run.lastPlayed).c_str());
            ImGui::TableNextColumn();
            if (run.finished) {
                ImGui::TextColored(kOk, "Concluída em %s", FormatTenths(run.timeTenths).c_str());
            } else {
                ImGui::TextColored(kDim, "%s", "Em andamento");
            }
        }
        ImGui::EndTable();
    }
}

} // namespace DefinitiveExpansion
