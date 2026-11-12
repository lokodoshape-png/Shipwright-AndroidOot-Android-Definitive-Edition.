// OOT D.E. Definitive Expansion, phase 5: achievements.
//
// Every achievement is checked against what the game itself records: the save in memory (inventory, quest items,
// event flags), the Ship of Harkinian statistics of the save (bosses defeated, counters) or a game event (enemy
// defeated, Boss Rush finished). Nothing is unlocked by opening a screen. Progress is global to the player and kept
// in ootde/achievements.json (DEAchievementsCore), never inside a save.
//
// Where they count:
// - "Campaign" achievements: only while playing a normal or Master Quest save of slots 1-3. Randomizer saves are
//   left out because their starting inventory is a setting, and Boss Rush has its own achievements.
// - The title screen demo and the debug save use file number 0xFF, so they never count.
#include "DefinitiveExpansion.h"
#include "DEAchievementsCore.h"
#include "DEStorage.h"

#include <imgui.h>
#include <libultraship/libultraship.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/boss-rush/BossRushTypes.h"
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

enum class Scope {
    Campaign, // normal / Master Quest save in slots 1-3
    AnySave,  // any save of slots 1-3 (randomizer included)
    Event,    // reported by a game event, never polled
};

using Evaluator = int (*)();

struct Tracked {
    DE::AchievementDef def;
    Scope scope;
    Evaluator value; // current progress read from the game; null for Scope::Event
};

int CountBits(u32 value) {
    int count = 0;
    for (; value != 0; value &= value - 1) {
        count++;
    }
    return count;
}

int QuestItems(int first, int count) {
    int owned = 0;
    for (int i = 0; i < count; i++) {
        if (CHECK_QUEST_ITEM(first + i)) {
            owned++;
        }
    }
    return owned;
}

int BossesDefeated() {
    static const int kBosses[] = { TIMESTAMP_DEFEAT_GOHMA,   TIMESTAMP_DEFEAT_KING_DODONGO,  TIMESTAMP_DEFEAT_BARINADE,
                                   TIMESTAMP_DEFEAT_PHANTOM_GANON, TIMESTAMP_DEFEAT_VOLVAGIA, TIMESTAMP_DEFEAT_MORPHA,
                                   TIMESTAMP_DEFEAT_BONGO_BONGO,   TIMESTAMP_DEFEAT_TWINROVA };
    int defeated = 0;
    for (int boss : kBosses) {
        if (gSaveContext.ship.stats.itemTimestamp[boss] != 0) {
            defeated++;
        }
    }
    return defeated;
}

bool GanonDefeated() {
    return gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_GANON] != 0;
}

int Stat(int counter) {
    return static_cast<int>(std::min<u32>(gSaveContext.ship.stats.count[counter], 1000000));
}

int EquipmentOwned() {
    int owned = 0;
    for (int type = EQUIP_TYPE_SWORD; type < EQUIP_TYPE_MAX; type++) {
        for (int value = 0; value < 3; value++) {
            if (!CHECK_OWNED_EQUIP(type, value)) {
                continue;
            }
            // The Biggoron slot also holds the Giant's Knife: only the real sword counts.
            if (type == EQUIP_TYPE_SWORD && value == EQUIP_INV_SWORD_BIGGORON && !gSaveContext.bgsFlag) {
                continue;
            }
            owned++;
        }
    }
    return owned;
}

int Bottles() {
    int bottles = 0;
    for (int i = 0; i < 4; i++) {
        if (gSaveContext.inventory.items[SLOT_BOTTLE_1 + i] != ITEM_NONE) {
            bottles++;
        }
    }
    return bottles;
}

int PlacesVisited() {
    int visited = 0;
    for (u32 word : gSaveContext.ship.stats.scenesDiscovered) {
        visited += CountBits(word);
    }
    return visited;
}

using Cat = DE::AchievementCategory;

// The ids are the keys in achievements.json: never rename them. New achievements can be added at any time.
const std::vector<Tracked>& Table() {
    static const std::vector<Tracked> table = {
        // História
        { { "story.first_step", Cat::Story, "Primeiro passo", "Comece uma aventura.", 1 },
          Scope::Campaign,
          []() { return 1; } },
        { { "story.stones", Cat::Story, "Pedras Espirituais", "Reúna as três Pedras Espirituais.", 3 },
          Scope::Campaign,
          []() { return QuestItems(QUEST_KOKIRI_EMERALD, 3); } },
        { { "story.master_sword", Cat::Story, "A lâmina do mal banido", "Retire a Master Sword do pedestal.", 1 },
          Scope::Campaign,
          []() { return Flags_GetEventChkInf(EVENTCHKINF_PULLED_MASTER_SWORD_FROM_PEDESTAL) ? 1 : 0; } },
        { { "story.medallions", Cat::Story, "Os Seis Sábios", "Reúna os seis medalhões.", 6 },
          Scope::Campaign,
          []() { return QuestItems(QUEST_MEDALLION_FOREST, 6); } },
        { { "story.songs", Cat::Story, "Músico de Hyrule", "Aprenda as doze canções da ocarina.", 12 },
          Scope::Campaign,
          []() { return QuestItems(QUEST_SONG_MINUET, 12); } },
        { { "story.hero", Cat::Story, "Herói de Hyrule", "Derrote Ganon e complete a história.", 1 },
          Scope::Campaign,
          []() { return GanonDefeated() ? 1 : 0; } },

        // Exploração
        { { "explore.places_20", Cat::Exploration, "Viajante", "Visite 20 lugares diferentes de Hyrule.", 20 },
          Scope::Campaign, PlacesVisited },
        { { "explore.places_60", Cat::Exploration, "Cartógrafo de Hyrule", "Visite 60 lugares diferentes de Hyrule.",
            60 },
          Scope::Campaign, PlacesVisited },
        { { "explore.epona", Cat::Exploration, "Companheira de estrada", "Conquiste a Epona.", 1 },
          Scope::Campaign,
          []() { return Flags_GetEventChkInf(EVENTCHKINF_EPONA_OBTAINED) ? 1 : 0; } },
        { { "explore.gerudo_card", Cat::Exploration, "Membro honorário", "Receba o Cartão Gerudo.", 1 },
          Scope::Campaign,
          []() { return CHECK_QUEST_ITEM(QUEST_GERUDO_CARD) ? 1 : 0; } },
        { { "explore.chests_50", Cat::Exploration, "Caçador de tesouros", "Abra 50 baús numa mesma aventura.", 50 },
          Scope::Campaign,
          []() { return Stat(COUNT_CHESTS_OPENED); } },

        // Combate
        { { "combat.enemies_100", Cat::Combat, "Espadachim", "Derrote 100 inimigos.", 100 }, Scope::Event, nullptr },
        { { "combat.enemies_1000", Cat::Combat, "Mestre da Espada", "Derrote 1000 inimigos.", 1000 },
          Scope::Event,
          nullptr },
        { { "combat.dark_link", Cat::Combat, "Vencer a si mesmo", "Derrote o Dark Link.", 1 },
          Scope::Campaign,
          []() { return Stat(COUNT_ENEMIES_DEFEATED_DARK_LINK) > 0 ? 1 : 0; } },
        { { "combat.iron_knuckle", Cat::Combat, "Quebra-armaduras", "Derrote 5 Iron Knuckles numa mesma aventura.", 5 },
          Scope::Campaign,
          []() { return Stat(COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE) + Stat(COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE_NABOORU); } },
        { { "combat.biggoron", Cat::Combat, "Lâmina do gigante", "Obtenha a Biggoron's Sword.", 1 },
          Scope::Campaign,
          []() { return gSaveContext.bgsFlag ? 1 : 0; } },

        // Colecionáveis
        { { "collect.skulltulas_10", Cat::Collectibles, "Primeiras fichas", "Encontre 10 Gold Skulltulas.", 10 },
          Scope::Campaign,
          []() { return static_cast<int>(gSaveContext.inventory.gsTokens); } },
        { { "collect.skulltulas_50", Cat::Collectibles, "Quebrando a maldição", "Encontre 50 Gold Skulltulas.", 50 },
          Scope::Campaign,
          []() { return static_cast<int>(gSaveContext.inventory.gsTokens); } },
        { { "collect.skulltulas_100", Cat::Collectibles, "Caçador de Skulltulas", "Encontre as 100 Gold Skulltulas.",
            100 },
          Scope::Campaign,
          []() { return static_cast<int>(gSaveContext.inventory.gsTokens); } },
        { { "collect.hearts_20", Cat::Collectibles, "Coração cheio", "Chegue a 20 corações.", 20 },
          Scope::Campaign,
          []() { return gSaveContext.healthCapacity / 16; } },
        { { "collect.bottles_4", Cat::Collectibles, "Engarrafador", "Tenha as quatro garrafas.", 4 },
          Scope::Campaign, Bottles },
        { { "collect.equipment", Cat::Collectibles, "Colecionador",
            "Tenha todas as espadas, escudos, túnicas e botas.", 12 },
          Scope::Campaign, EquipmentOwned },
        { { "collect.great_fairies", Cat::Collectibles, "Bênção das Grandes Fadas",
            "Receba a magia dupla e a defesa dupla.", 2 },
          Scope::Campaign,
          []() { return (gSaveContext.isDoubleMagicAcquired ? 1 : 0) + (gSaveContext.isDoubleDefenseAcquired ? 1 : 0); } },

        // Masmorras
        { { "dungeon.first", Cat::Dungeons, "Primeira masmorra", "Derrote o chefe de uma masmorra.", 1 },
          Scope::Campaign, BossesDefeated },
        { { "dungeon.child", Cat::Dungeons, "Infância de herói", "Derrote os chefes das três primeiras masmorras.", 3 },
          Scope::Campaign,
          []() {
              return (gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_GOHMA] ? 1 : 0) +
                     (gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_KING_DODONGO] ? 1 : 0) +
                     (gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_BARINADE] ? 1 : 0);
          } },
        { { "dungeon.all", Cat::Dungeons, "Mestre das Masmorras", "Derrote os chefes das oito masmorras.", 8 },
          Scope::Campaign, BossesDefeated },
        { { "dungeon.trials", Cat::Dungeons, "Barreira dissipada", "Dissipe a barreira da Torre de Ganon.", 1 },
          Scope::Campaign,
          []() { return Flags_GetEventChkInf(EVENTCHKINF_DISPELLED_GANONS_TOWER_BARRIER) ? 1 : 0; } },

        // Desafios
        { { "challenge.no_death", Cat::Challenges, "Sem cair uma vez", "Complete a história sem morrer nenhuma vez.",
            1 },
          Scope::Campaign,
          []() { return GanonDefeated() && gSaveContext.deaths == 0 ? 1 : 0; } },
        { { "challenge.three_hearts", Cat::Challenges, "Três corações",
            "Complete a história com apenas três corações.", 1 },
          Scope::Campaign,
          []() { return GanonDefeated() && gSaveContext.healthCapacity <= 0x30 ? 1 : 0; } },
        { { "challenge.master_quest", Cat::Challenges, "Master Quest", "Complete a história no Master Quest.", 1 },
          Scope::Campaign,
          []() { return GanonDefeated() && IS_MASTER_QUEST ? 1 : 0; } },
        { { "challenge.randomizer", Cat::Challenges, "Destino embaralhado", "Complete uma partida do Randomizer.", 1 },
          Scope::AnySave,
          []() { return IS_RANDO && gSaveContext.ship.stats.gameComplete ? 1 : 0; } },
        { { "challenge.bossrush", Cat::Challenges, "Maratona de chefes", "Complete o Boss Rush.", 1 },
          Scope::Event,
          nullptr },
        { { "challenge.bossrush_all", Cat::Challenges, "Todos os chefes", "Complete o Boss Rush com todos os chefes.",
            1 },
          Scope::Event,
          nullptr },
        { { "challenge.bossrush_hyper", Cat::Challenges, "Chefes acelerados",
            "Complete o Boss Rush com os Hyper Bosses ligados.", 1 },
          Scope::Event,
          nullptr },
        { { "challenge.bossrush_noheal", Cat::Challenges, "Sobrevivente",
            "Complete o Boss Rush com todos os chefes sem nenhuma cura entre as lutas.", 1 },
          Scope::Event,
          nullptr },
        { { "challenge.bossrush_flawless", Cat::Challenges, "Impecável",
            "Complete o Boss Rush com todos os chefes sem morrer.", 1 },
          Scope::Event,
          nullptr },

        // Segredos
        { { "secret.ice_arrows", Cat::Secrets, "Frio opcional", "Obtenha as Flechas de Gelo.", 1, true },
          Scope::Campaign,
          []() { return gSaveContext.inventory.items[SLOT_ARROW_ICE] == ITEM_ARROW_ICE ? 1 : 0; } },
        { { "secret.agony", Cat::Secrets, "Pressentimento", "Obtenha a Pedra da Agonia.", 1, true },
          Scope::Campaign,
          []() { return CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY) ? 1 : 0; } },
        { { "secret.cucco", Cat::Secrets, "Madrugador", "Faça um ovo chocar.", 1, true }, Scope::Event, nullptr },
        { { "secret.bonks", Cat::Secrets, "Cabeça dura", "Bata de cara em 25 paredes numa mesma aventura.", 25, true },
          Scope::Campaign,
          []() { return Stat(COUNT_BONKS); } },
        { { "secret.pots", Cat::Secrets, "Inimigo da cerâmica", "Quebre 200 potes numa mesma aventura.", 200, true },
          Scope::Campaign,
          []() { return Stat(COUNT_POTS_BROKEN); } },
    };
    return table;
}

std::unique_ptr<DE::AchievementStore> sStore;
bool sLoadedClean = true;
int sFrames = 0;
int64_t sLastSave = 0;

DE::AchievementStore& Store() {
    if (!sStore) {
        std::vector<DE::AchievementDef> defs;
        for (const Tracked& tracked : Table()) {
            defs.push_back(tracked.def);
        }
        sStore = std::make_unique<DE::AchievementStore>(std::move(defs));
        sLoadedClean = sStore->Load(DataDir() / "achievements.json");
        if (!sLoadedClean) {
            SPDLOG_WARN("OOT D.E.: ootde/achievements.json não pôde ser usado; o arquivo foi guardado à parte");
        }
    }
    return *sStore;
}

bool PlayingSlot() {
    return gPlayState != nullptr && gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum >= 0 &&
           gSaveContext.fileNum < DE::SaveVault::kSlots;
}

bool InScope(Scope scope) {
    if (!PlayingSlot()) {
        return false;
    }
    switch (scope) {
        case Scope::Campaign:
            return IS_VANILLA || IS_MASTER_QUEST;
        case Scope::AnySave:
            return !IS_BOSS_RUSH;
        case Scope::Event:
            return false;
    }
    return false;
}

void Announce(const std::string& id) {
    const DE::AchievementDef* def = Store().Find(id);
    if (def == nullptr) {
        return;
    }
    if (Config().GetBool("achievements.notify")) {
        Notification::Emit({
            .prefix = "Conquista:",
            .prefixColor = ImVec4(1.0f, 0.82f, 0.30f, 1.0f),
            .message = def->title,
            .messageColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
            .remainingTime = 8.0f,
        });
    }
    SPDLOG_INFO("OOT D.E.: conquista desbloqueada: {}", id);
}

void SaveStore(bool force) {
    DE::AchievementStore& store = Store();
    const int64_t now = DE::NowUnixSeconds();
    if (!store.Dirty() || (!force && now - sLastSave < 30)) {
        return;
    }
    std::string error;
    if (!store.Save(&error)) {
        SPDLOG_ERROR("OOT D.E.: não foi possível gravar ootde/achievements.json: {}", error);
    }
    sLastSave = now;
}

void Poll() {
    DE::AchievementStore& store = Store();
    const int64_t now = DE::NowUnixSeconds();
    bool unlocked = false;
    for (const Tracked& tracked : Table()) {
        if (tracked.value == nullptr || !InScope(tracked.scope)) {
            continue;
        }
        if (store.Report(tracked.def.id, tracked.value(), now)) {
            Announce(tracked.def.id);
            unlocked = true;
        }
    }
    SaveStore(unlocked);
}

void RegisterHooks() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
        // Once a second is plenty: every condition is a state of the save, not a moment.
        if (++sFrames >= 20) {
            sFrames = 0;
            Poll();
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnEnemyDefeat>([](void* actor) {
        if (!PlayingSlot() || IS_BOSS_RUSH) {
            return;
        }
        const int64_t now = DE::NowUnixSeconds();
        if (Store().Add("combat.enemies_100", 1, now)) {
            Announce("combat.enemies_100");
        }
        if (Store().Add("combat.enemies_1000", 1, now)) {
            Announce("combat.enemies_1000");
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnCuccoOrChickenHatch>([]() {
        if (InScope(Scope::Campaign) && Store().Report("secret.cucco", 1, DE::NowUnixSeconds())) {
            Announce("secret.cucco");
            SaveStore(true);
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveFile>([](int32_t fileNum) { SaveStore(true); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t fileNum) { SaveStore(true); });
}

RegisterShipInitFunc sInit(RegisterHooks);

const ImVec4 kGold(1.0f, 0.82f, 0.30f, 1.0f);
const ImVec4 kDim(0.62f, 0.64f, 0.70f, 1.0f);
const ImVec4 kOk(0.45f, 0.85f, 0.45f, 1.0f);

void DrawOne(const DE::AchievementDef& def, const DE::AchievementState& state) {
    const bool hidden = def.secret && !state.Unlocked();
    ImGui::PushID(def.id.c_str());
    ImGui::TextColored(state.Unlocked() ? kGold : kDim, "%s  %s", state.Unlocked() ? "[X]" : "[ ]",
                       hidden ? "Conquista secreta" : def.title.c_str());
    ImGui::Indent();
    ImGui::TextWrapped("%s", hidden ? "Continue jogando para descobrir." : def.description.c_str());
    if (state.Unlocked()) {
        ImGui::TextColored(kOk, "Desbloqueada em %s", DE::TimestampForDisplay(state.unlockedAt).c_str());
    } else if (!hidden && def.target > 1) {
        const std::string label = std::to_string(std::min(state.progress, def.target)) + " / " +
                                  std::to_string(def.target);
        ImGui::ProgressBar(static_cast<float>(state.progress) / static_cast<float>(def.target),
                           ImVec2(std::min(360.0f, ImGui::GetContentRegionAvail().x), 0.0f), label.c_str());
    }
    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::PopID();
}

} // namespace

void RegisterAchievementSettings(DE::Config& config) {
    config.RegisterSetting("achievements.notify", true, "Mostra um aviso na tela quando uma conquista é desbloqueada.");
}

void ReportAchievement(const char* id) {
    if (Store().Report(id, 1, DE::NowUnixSeconds())) {
        Announce(id);
    }
    SaveStore(true);
}

bool AchievementUnlocked(const char* id) {
    return Store().State(id).Unlocked();
}

void MarkPlaceVisited(int sceneNum) {
    // The save already has a "places discovered" bit set (written with the Ship of Harkinian statistics); the game
    // only fills it for randomizer entrances. Setting the bit here does not write the save: it goes to disk the next
    // time the player saves, like the rest of the statistics.
    if (!PlayingSlot() || IS_BOSS_RUSH || sceneNum < 0 || sceneNum >= SCENE_ID_MAX) {
        return;
    }
    const u32 index = static_cast<u32>(sceneNum) / 32;
    if (index < ARRAY_COUNT(gSaveContext.ship.stats.scenesDiscovered)) {
        gSaveContext.ship.stats.scenesDiscovered[index] |= 1u << (static_cast<u32>(sceneNum) % 32);
    }
}

void DrawAchievements() {
    DE::AchievementStore& store = Store();
    const int total = static_cast<int>(store.Defs().size());
    ImGui::TextColored(kGold, "%d de %d conquistas (%d%%)", store.UnlockedCount(), total, store.Percent());
    ImGui::ProgressBar(static_cast<float>(store.Percent()) / 100.0f,
                       ImVec2(std::min(480.0f, ImGui::GetContentRegionAvail().x), 0.0f), "");
    if (!sLoadedClean) {
        ImGui::TextWrapped("%s", "O arquivo de conquistas anterior não pôde ser lido e foi guardado à parte em "
                                 "OOT-DE/ootde. O progresso recomeçou.");
    }
    ImGui::TextWrapped("%s", "As conquistas valem para o jogador, não para um save: ficam em ootde/achievements.json. "
                             "As de campanha contam em saves normais e Master Quest dos slots 1 a 3.");

    bool notify = Config().GetBool("achievements.notify");
    if (UIWidgets::Checkbox("Avisar na tela ao desbloquear", &notify)) {
        Config().Set("achievements.notify", notify);
        SaveConfig();
    }
    ImGui::Spacing();

    if (ImGui::BeginTabBar("DEAchievementTabs")) {
        if (ImGui::BeginTabItem("Histórico")) {
            const std::vector<const DE::AchievementDef*> history = store.History();
            if (history.empty()) {
                ImGui::TextWrapped("%s", "Nenhuma conquista desbloqueada ainda.");
            }
            for (const DE::AchievementDef* def : history) {
                DrawOne(*def, store.State(def->id));
            }
            ImGui::EndTabItem();
        }
        static const Cat kCategories[] = { Cat::Story,    Cat::Exploration, Cat::Combat, Cat::Collectibles,
                                           Cat::Dungeons, Cat::Challenges,  Cat::Secrets };
        for (Cat category : kCategories) {
            if (!ImGui::BeginTabItem(DE::AchievementCategoryLabel(category))) {
                continue;
            }
            for (const DE::AchievementDef& def : store.Defs()) {
                if (def.category == category) {
                    DrawOne(def, store.State(def.id));
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

} // namespace DefinitiveExpansion
