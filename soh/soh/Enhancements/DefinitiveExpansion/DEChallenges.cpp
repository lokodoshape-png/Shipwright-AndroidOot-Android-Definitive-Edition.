// OOT D.E. Definitive Expansion, phase 6: Boss Rush records and Master Mode.
//
// Boss Rush: the mode itself is the Ship of Harkinian Boss Rush (file select > Boss Rush), untouched. It already has
// its own state: it runs on a save that is never written to disk, so the campaign is safe. This file only watches a
// run and, when the game marks it as finished, stores the result (time, deaths, health left, score) in
// ootde/records.json. The time is real time from a monotonic clock, counted while the game's own Boss Rush timer is
// running (so the waiting room between bosses does not count), and does not depend on the frame rate.
//
// Master Mode: a set of difficulty presets made only of modifiers the game already has (Enhancements > Difficulty).
// Nothing new is changed in the enemies; each preset says exactly which modifiers it turns on.
#include "DefinitiveExpansion.h"
#include "DERecords.h"
#include "DEStorage.h"

#include <imgui.h>
#include <libultraship/libultraship.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/boss-rush/BossRushTypes.h"
#include "soh/Enhancements/mods.h"
#include "soh/Notification/Notification.h"
#include "soh/SohGui/UIWidgets.hpp"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace DefinitiveExpansion {

namespace {

const ImVec4 kGold(1.0f, 0.82f, 0.30f, 1.0f);
const ImVec4 kDim(0.62f, 0.64f, 0.70f, 1.0f);
const ImVec4 kOk(0.45f, 0.85f, 0.45f, 1.0f);

// ---------------------------------------------------------------------------------------------------------------
// Boss Rush records
// ---------------------------------------------------------------------------------------------------------------

std::unique_ptr<DE::RecordBook> sBook;
bool sBookClean = true;

DE::RecordBook& Book() {
    if (!sBook) {
        sBook = std::make_unique<DE::RecordBook>();
        sBookClean = sBook->Load(DataDir() / "records.json");
        if (!sBookClean) {
            SPDLOG_WARN("OOT D.E.: ootde/records.json não pôde ser usado; o arquivo foi guardado à parte");
        }
    }
    return *sBook;
}

struct BossSet {
    const char* id;
    const char* label;
};
const BossSet kBossSets[] = {
    { "all", "Todos os chefes" },
    { "child", "Chefes da infância" },
    { "adult", "Chefes da fase adulta" },
    { "ganon", "Ganondorf e Ganon" },
};

struct Difficulty {
    const char* id;
    const char* label;
    const char* rule;
};
const Difficulty kDifficulties[] = {
    { "normal", "Normal", "Chefes normais, com cura entre as lutas." },
    { "dificil", "Difícil", "Hyper Bosses ligados: os chefes agem duas vezes mais rápido." },
    { "sobrevivencia", "Sobrevivência", "Cura: nunca. A vida não volta entre as lutas." },
    { "dificil-sobrevivencia", "Difícil + Sobrevivência", "Hyper Bosses ligados e sem nenhuma cura." },
};

int BossSetIndex() {
    const u8 choice = gSaveContext.ship.quest.data.bossRush.options[BR_OPTIONS_BOSSES];
    switch (choice) {
        case BR_CHOICE_BOSSES_CHILD:
            return 1;
        case BR_CHOICE_BOSSES_ADULT:
            return 2;
        case BR_CHOICE_BOSSES_GANONDORF_GANON:
            return 3;
        default:
            return 0;
    }
}

bool RunIsHyper() {
    return gSaveContext.ship.quest.data.bossRush.options[BR_OPTIONS_HYPERBOSSES] == BR_CHOICE_HYPERBOSSES_YES;
}

bool RunIsNoHeal() {
    return gSaveContext.ship.quest.data.bossRush.options[BR_OPTIONS_HEAL] == BR_CHOICE_HEAL_NEVER;
}

int DifficultyIndex() {
    return (RunIsHyper() ? 1 : 0) + (RunIsNoHeal() ? 2 : 0);
}

std::string BoardId(int bossSet, int difficulty) {
    return std::string("bossrush/") + kBossSets[bossSet].id + "/" + kDifficulties[difficulty].id;
}

std::string RunDetails() {
    const u8* options = gSaveContext.ship.quest.data.bossRush.options;
    static const int kHearts[] = { 10, 15, 20, 3, 5, 7 };
    std::string text = std::to_string(kHearts[std::min<int>(options[BR_OPTIONS_HEARTS], 5)]) + " corações";
    text += options[BR_OPTIONS_AMMO] == BR_CHOICE_AMMO_LIMITED
                ? ", munição limitada"
                : (options[BR_OPTIONS_AMMO] == BR_CHOICE_AMMO_FULL ? ", munição cheia" : ", munição máxima");
    text += options[BR_OPTIONS_HEAL] == BR_CHOICE_HEAL_NEVER
                ? ", sem cura"
                : (options[BR_OPTIONS_HEAL] == BR_CHOICE_HEAL_EVERYBOSS ? ", cura a cada chefe"
                                                                        : ", cura antes de Ganondorf");
    if (options[BR_OPTIONS_MAGIC] == BR_CHOICE_MAGIC_DOUBLE) {
        text += ", magia dupla";
    }
    if (options[BR_OPTIONS_BGS] == BR_CHOICE_BGS_YES) {
        text += ", Biggoron's Sword";
    }
    if (options[BR_OPTIONS_BOTTLE] != BR_CHOICE_BOTTLE_NO) {
        text += ", garrafa";
    }
    if (options[BR_OPTIONS_LONGSHOT] == BR_CHOICE_LONGSHOT_YES) {
        text += ", Longshot";
    }
    if (options[BR_OPTIONS_HOVERBOOTS] == BR_CHOICE_HOVERBOOTS_YES) {
        text += ", Hover Boots";
    }
    if (options[BR_OPTIONS_BUNNYHOOD] == BR_CHOICE_BUNNYHOOD_YES) {
        text += ", Bunny Hood";
    }
    return text;
}

// State of the run being played.
using Clock = std::chrono::steady_clock;
Clock::time_point sLastFrame;
bool sHaveLastFrame = false;
int64_t sRunMicros = 0;
bool sRunRecorded = false;
bool sInRun = false;
DE::RunRecord sLastResult;
int sLastResultPlace = 0;

bool InBossRushPlay() {
    return gPlayState != nullptr && gSaveContext.gameMode == GAMEMODE_NORMAL && IS_BOSS_RUSH;
}

void FinishRun() {
    DE::RunRecord run;
    const int bossSet = BossSetIndex();
    const int difficulty = DifficultyIndex();
    run.board = BoardId(bossSet, difficulty);
    run.timeMs = sRunMicros / 1000;
    run.deaths = gSaveContext.deaths;
    run.healthLeft = std::max<int>(0, gSaveContext.health);
    run.healthMax = gSaveContext.healthCapacity;
    run.score = DE::ComputeRunScore(run.timeMs, run.deaths, run.healthLeft, run.healthMax,
                                    100 + (RunIsHyper() ? 50 : 0) + (RunIsNoHeal() ? 25 : 0));
    run.date = DE::NowUnixSeconds();
    run.details = RunDetails();

    sLastResultPlace = Book().Add(run);
    sLastResult = run;
    std::string error;
    if (!Book().Save(&error)) {
        SPDLOG_ERROR("OOT D.E.: não foi possível gravar ootde/records.json: {}", error);
    }
    Notification::Emit({
        .prefix = sLastResultPlace == 1 ? "Novo recorde:" : "Boss Rush:",
        .prefixColor = kGold,
        .message = DE::FormatRunTime(run.timeMs),
        .messageColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        .suffix = std::to_string(run.score) + " pontos",
        .remainingTime = 10.0f,
    });

    ReportAchievement("challenge.bossrush");
    if (bossSet == 0) {
        ReportAchievement("challenge.bossrush_all");
        if (RunIsNoHeal()) {
            ReportAchievement("challenge.bossrush_noheal");
        }
        if (run.deaths == 0) {
            ReportAchievement("challenge.bossrush_flawless");
        }
    }
    if (RunIsHyper()) {
        ReportAchievement("challenge.bossrush_hyper");
    }
}

void OnFrame() {
    const Clock::time_point now = Clock::now();
    int64_t delta = 0;
    if (sHaveLastFrame) {
        delta = std::chrono::duration_cast<std::chrono::microseconds>(now - sLastFrame).count();
        // The app was in the background (or the device slept): that time is not part of the run.
        delta = std::clamp<int64_t>(delta, 0, 500000);
    }
    sLastFrame = now;
    sHaveLastFrame = true;

    if (!InBossRushPlay()) {
        sInRun = false;
        return;
    }
    if (!sInRun) {
        sInRun = true;
        sRunMicros = 0;
        sRunRecorded = false;
    }
    if (gSaveContext.ship.stats.gameComplete) {
        if (!sRunRecorded) {
            sRunRecorded = true;
            FinishRun();
        }
        return;
    }
    // Same rule as the game's own timer: it stands still in the room between bosses.
    if (!gSaveContext.ship.quest.data.bossRush.isPaused) {
        sRunMicros += delta;
    }
}

void RegisterHooks() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(OnFrame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) {
        // A new game was loaded (a new Boss Rush run included): whatever was being timed is over.
        sInRun = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(
        [](int16_t sceneNum) { MarkPlaceVisited(sceneNum); });
}

RegisterShipInitFunc sInit(RegisterHooks);

void DrawRunRow(int place, const DE::RunRecord& run, bool showBoard) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    if (place > 0) {
        ImGui::TextColored(place == 1 ? kGold : kDim, "%d", place);
    } else {
        ImGui::TextUnformatted(DE::TimestampForDisplay(run.date).c_str());
    }
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(DE::FormatRunTime(run.timeMs).c_str());
    ImGui::TableNextColumn();
    ImGui::Text("%d", run.deaths);
    ImGui::TableNextColumn();
    ImGui::Text("%.2f / %d", static_cast<float>(run.healthLeft) / 16.0f, run.healthMax / 16);
    ImGui::TableNextColumn();
    ImGui::Text("%d", run.score);
    ImGui::TableNextColumn();
    if (showBoard) {
        std::string board = run.board;
        for (int b = 0; b < 4; b++) {
            for (int d = 0; d < 4; d++) {
                if (run.board == BoardId(b, d)) {
                    board = std::string(kBossSets[b].label) + " - " + kDifficulties[d].label;
                }
            }
        }
        ImGui::TextWrapped("%s", board.c_str());
    } else {
        ImGui::TextWrapped("%s (%s)", run.details.c_str(), DE::TimestampForDisplay(run.date).c_str());
    }
}

bool BeginRunTable(const char* id, const char* first, const char* last) {
    if (!ImGui::BeginTable(id, 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit)) {
        return false;
    }
    ImGui::TableSetupColumn(first);
    ImGui::TableSetupColumn("Tempo");
    ImGui::TableSetupColumn("Mortes");
    ImGui::TableSetupColumn("Vida final");
    ImGui::TableSetupColumn("Pontos");
    ImGui::TableSetupColumn(last, ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Master Mode
// ---------------------------------------------------------------------------------------------------------------

struct Modifier {
    const char* cvar;
    const char* backupKey; // in ootde/config.json
    const char* label;
    bool isMultiplier;     // value n means damage x 2^n
};
const Modifier kModifiers[] = {
    { CVAR_ENHANCEMENT("DamageMult"), "master.backup.damage", "Dano recebido", true },
    { CVAR_ENHANCEMENT("FallDamageMult"), "master.backup.fall", "Dano de queda", true },
    { CVAR_ENHANCEMENT("VoidDamageMult"), "master.backup.void", "Dano ao cair em abismos", true },
    { CVAR_ENHANCEMENT("NoHeartDrops"), "master.backup.noHearts", "Sem corações soltos por inimigos e objetos", false },
    { CVAR_ENHANCEMENT("NoRandomDrops"), "master.backup.noDrops", "Sem itens aleatórios (rúpias, munição, magia)",
      false },
    { CVAR_ENHANCEMENT("HyperEnemies"), "master.backup.hyperEnemies", "Inimigos duas vezes mais rápidos", false },
    { CVAR_ENHANCEMENT("HyperBosses"), "master.backup.hyperBosses", "Chefes duas vezes mais rápidos", false },
};
constexpr int kModifierCount = ARRAY_COUNT(kModifiers);

struct Preset {
    const char* label;
    const char* description;
    int values[kModifierCount];
};
const Preset kPresets[] = {
    { "Clássico", "O jogo original: nenhum modificador.", { 0, 0, 0, 0, 0, 0, 0 } },
    { "Herói", "Dano em dobro e nenhum coração solto pelo caminho.", { 1, 0, 0, 1, 0, 0, 0 } },
    { "Mestre", "Dano 4x, quedas mais perigosas, sem corações e inimigos acelerados.", { 2, 1, 1, 1, 0, 1, 0 } },
    { "Lendário", "Dano 8x, sem corações nem itens aleatórios, inimigos e chefes acelerados.",
      { 3, 2, 2, 1, 1, 1, 1 } },
};

int CurrentPreset() {
    for (int p = 0; p < static_cast<int>(ARRAY_COUNT(kPresets)); p++) {
        bool same = true;
        for (int m = 0; m < kModifierCount; m++) {
            if (CVarGetInteger(kModifiers[m].cvar, 0) != kPresets[p].values[m]) {
                same = false;
            }
        }
        if (same) {
            return p;
        }
    }
    return -1;
}

void ApplyValues(const int* values) {
    for (int m = 0; m < kModifierCount; m++) {
        CVarSetInteger(kModifiers[m].cvar, values[m]);
        ShipInit::Init(kModifiers[m].cvar);
    }
    UpdateHyperEnemiesState();
    UpdateHyperBossesState();
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

void ApplyPreset(int preset) {
    // Coming from the player's own mix of modifiers: keep it, so it can be brought back.
    if (CurrentPreset() < 0) {
        for (const Modifier& modifier : kModifiers) {
            Config().Set(modifier.backupKey, static_cast<int>(CVarGetInteger(modifier.cvar, 0)));
        }
        Config().Set("master.backup.valid", true);
        SaveConfig();
    }
    ApplyValues(kPresets[preset].values);
}

std::string ModifierText(const Modifier& modifier, int value) {
    if (modifier.isMultiplier) {
        return std::string(modifier.label) + ": " + std::to_string(1 << std::clamp(value, 0, 8)) + "x";
    }
    return std::string(modifier.label) + ": " + (value ? "sim" : "não");
}

} // namespace

void RegisterChallengeSettings(DE::Config& config) {
    for (const Modifier& modifier : kModifiers) {
        config.RegisterSetting(modifier.backupKey, 0, "Valor que o jogador usava antes de aplicar um preset do Master "
                                                      "Mode.");
    }
    config.RegisterSetting("master.backup.valid", false, "Há uma combinação própria guardada para restaurar.");
}

void DrawBossRush() {
    DE::RecordBook& book = Book();
    ImGui::TextWrapped("%s", "O Boss Rush é o modo do Ship of Harkinian: na seleção de arquivos, escolha um slot vazio "
                             "e troque o tipo de aventura para Boss Rush. Ele roda num estado próprio que nunca é "
                             "gravado, então a campanha não é tocada. Aqui ficam os resultados das suas corridas.");
    if (!sBookClean) {
        ImGui::TextWrapped("%s", "O arquivo de recordes anterior não pôde ser lido e foi guardado à parte em "
                                 "OOT-DE/ootde.");
    }
    if (InBossRushPlay() && sInRun && !sRunRecorded) {
        ImGui::TextColored(kGold, "Corrida em andamento: %s (%s - %s)", DE::FormatRunTime(sRunMicros / 1000).c_str(),
                           kBossSets[BossSetIndex()].label, kDifficulties[DifficultyIndex()].label);
    }
    if (sLastResultPlace > 0) {
        ImGui::TextColored(kOk, "Última corrida: %s, %d pontos, %dº lugar no ranking dela",
                           DE::FormatRunTime(sLastResult.timeMs).c_str(), sLastResult.score, sLastResultPlace);
    }
    ImGui::Spacing();

    if (ImGui::BeginTabBar("DEBossRushTabs")) {
        if (ImGui::BeginTabItem("Ranking")) {
            static int sBossSet = 0;
            static int sDifficulty = 0;
            ImGui::TextUnformatted("Chefes:");
            for (int b = 0; b < 4; b++) {
                ImGui::SameLine();
                if (ImGui::RadioButton(kBossSets[b].label, sBossSet == b)) {
                    sBossSet = b;
                }
            }
            ImGui::TextUnformatted("Dificuldade:");
            for (int d = 0; d < 4; d++) {
                ImGui::SameLine();
                if (ImGui::RadioButton(kDifficulties[d].label, sDifficulty == d)) {
                    sDifficulty = d;
                }
            }
            ImGui::TextColored(kDim, "%s", kDifficulties[sDifficulty].rule);
            const std::vector<DE::RunRecord> ranking = book.Ranking(BoardId(sBossSet, sDifficulty));
            if (ranking.empty()) {
                ImGui::TextWrapped("%s", "Nenhuma corrida terminada nesta categoria ainda.");
            } else if (BeginRunTable("DEBossRushRanking", "Lugar", "Opções")) {
                int place = 1;
                for (const DE::RunRecord& run : ranking) {
                    DrawRunRow(place++, run, false);
                    if (place > 20) {
                        break;
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Histórico")) {
            const std::vector<DE::RunRecord> history = book.History();
            if (history.empty()) {
                ImGui::TextWrapped("%s", "Nenhuma corrida terminada ainda.");
            } else if (BeginRunTable("DEBossRushHistory", "Data", "Categoria")) {
                for (const DE::RunRecord& run : history) {
                    DrawRunRow(0, run, true);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Regras")) {
            ImGui::TextWrapped("%s", "Tempo: tempo real, contado enquanto o cronômetro do Boss Rush está andando (a "
                                     "sala entre os chefes não conta). Não depende da taxa de quadros.");
            ImGui::TextWrapped("%s", "Ranking: menor tempo primeiro; em empate, menos mortes. Cada combinação de "
                                     "chefes e dificuldade tem o seu ranking.");
            ImGui::TextWrapped("%s", "Pontos: 10000, menos 5 por segundo, menos 500 por morte, mais até 2000 pela vida "
                                     "que sobrou. Difícil vale 50% a mais e Sobrevivência 25% a mais.");
            ImGui::Spacing();
            for (const Difficulty& difficulty : kDifficulties) {
                ImGui::TextColored(kGold, "%s", difficulty.label);
                ImGui::SameLine();
                ImGui::TextWrapped("%s", difficulty.rule);
            }
            ImGui::TextColored(kDim, "%s", "Desafio de tempo (com limite durante a luta) ainda não existe: o jogo não "
                                           "tem essa regra, então o modo não é oferecido.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DrawMasterMode() {
    ImGui::TextWrapped("%s", "O Master Mode junta modificadores de dificuldade que o jogo já tem (Aprimoramentos > "
                             "Dificuldade). Vale para qualquer save enquanto estiver ligado e pode ser desligado a "
                             "qualquer momento: nada é gravado no save.");
    const int current = CurrentPreset();
    ImGui::TextColored(kGold, "Agora: %s", current >= 0 ? kPresets[current].label : "Personalizado");
    ImGui::Spacing();

    for (int p = 0; p < static_cast<int>(ARRAY_COUNT(kPresets)); p++) {
        ImGui::PushID(p);
        if (UIWidgets::Button(kPresets[p].label, UIWidgets::ButtonOptions()
                                                     .Size(ImVec2(180.0f, 0.0f))
                                                     .Color(current == p ? UIWidgets::Colors::Yellow
                                                                         : UIWidgets::Colors::Gray))) {
            ApplyPreset(p);
        }
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextWrapped("%s", kPresets[p].description);
        std::string list;
        for (int m = 0; m < kModifierCount; m++) {
            if (kPresets[p].values[m] != 0) {
                list += (list.empty() ? "" : "; ") + ModifierText(kModifiers[m], kPresets[p].values[m]);
            }
        }
        if (!list.empty()) {
            ImGui::TextColored(kDim, "%s", list.c_str());
        }
        ImGui::EndGroup();
        ImGui::Spacing();
        ImGui::PopID();
    }

    if (Config().GetBool("master.backup.valid")) {
        if (UIWidgets::Button("Restaurar a minha combinação anterior",
                              UIWidgets::ButtonOptions().Size(ImVec2(360.0f, 0.0f)))) {
            int values[kModifierCount];
            for (int m = 0; m < kModifierCount; m++) {
                values[m] = Config().GetInt(kModifiers[m].backupKey);
            }
            ApplyValues(values);
            Config().Set("master.backup.valid", false);
            SaveConfig();
        }
    }

    ImGui::SeparatorText("Modificadores ligados agora");
    for (const Modifier& modifier : kModifiers) {
        const int value = CVarGetInteger(modifier.cvar, 0);
        ImGui::TextColored(value ? kGold : kDim, "%s", ModifierText(modifier, value).c_str());
    }
    ImGui::Spacing();
    ImGui::TextWrapped("%s", "Master Quest (masmorras refeitas) é escolhido ao criar um arquivo, na seleção de "
                             "arquivos, e tem progressão própria no save dele.");
    ImGui::TextWrapped("%s", "Desafios com resultado registrado: veja Definitive > Boss Rush (corridas, recordes) e "
                             "Definitive > Conquistas > Desafios (sem morrer, três corações, Master Quest).");
}

} // namespace DefinitiveExpansion
