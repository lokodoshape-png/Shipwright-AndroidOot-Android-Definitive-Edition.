// OOT D.E. Definitive Expansion, phase 4: Hyrule Explorer and Album.
//
// Everything shown here is read from the save being played; nothing is written to it.
// - Map: the 22 areas of the pause screen world map, at the positions and sizes the game itself uses for the
//   "current position" box (areaBoxPosX/Y and sizes in z_kaleido_map_PAL.c). An area counts as visited when the game
//   has set its bit in worldMapAreaData (the same bit that removes the cloud over it on the pause map).
// - Regions: the 12 points of the pause world map, with the Gold Skulltulas and Pieces of Heart already counted by
//   the map collectibles panel (z_kaleido_map_collect.c).
// - Album: characters met, bosses faced and defeated, enemies defeated, items and events, all from the save's own
//   flags, inventory and Ship of Harkinian statistics.
#include "DefinitiveExpansion.h"

#include <imgui.h>
#include <libultraship/libultraship.h>

#include <algorithm>
#include <string>
#include <vector>

#include "soh/OTRGlobals.h"
#include "soh/util.h"
#include "soh/Enhancements/gameplaystats.h"
#include "soh/SohGui/UIWidgets.hpp"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
void KaleidoScope_MapCollectRegionCounts(PlayState* play, s32 point, s16* skullGot, s16* skullTotal, s16* heartGot,
                                         s16* heartTotal);
}

namespace DefinitiveExpansion {

namespace {

const ImVec4 kGold(1.0f, 0.82f, 0.30f, 1.0f);
const ImVec4 kDim(0.62f, 0.64f, 0.70f, 1.0f);
const ImVec4 kOk(0.45f, 0.85f, 0.45f, 1.0f);

struct Area {
    const char* name;
    s16 x, y, w, h; // world map coordinates of the game's area box (y grows upwards)
};
// Order = gSaveContext.worldMapArea (set by each scene's header). 20 is the "?" area and is not shown.
const Area kAreas[22] = {
    { "Hyrule Field", -41, 30, 96, 59 },         { "Kakariko Village", 19, 36, 32, 19 },
    { "Cemitério", 44, 35, 32, 13 },             { "Zora's River", 40, 26, 48, 19 },
    { "Kokiri Forest", 49, 7, 48, 38 },          { "Sacred Forest Meadow", 51, 11, 32, 17 },
    { "Lake Hylia", -49, -31, 48, 38 },          { "Zora's Domain", 83, 30, 32, 17 },
    { "Zora's Fountain", 80, 38, 32, 13 },       { "Gerudo Valley", -67, 23, 32, 26 },
    { "Lost Woods", 50, 2, 16, 16 },             { "Desert Colossus", -109, 42, 32, 26 },
    { "Gerudo's Fortress", -76, 40, 32, 26 },    { "Haunted Wasteland", -86, 32, 16, 16 },
    { "Mercado", -10, 38, 32, 19 },              { "Castelo de Hyrule", -6, 50, 32, 17 },
    { "Death Mountain Trail", 19, 57, 32, 26 },  { "Death Mountain Crater", 24, 58, 32, 13 },
    { "Goron City", 11, 56, 32, 17 },            { "Lon Lon Ranch", -17, 12, 32, 17 },
    { "?", 37, 36, 16, 16 },                     { "Castelo de Ganon", -6, 50, 32, 17 },
};
constexpr int kUnknownArea = 20;

struct Region {
    const char* name;
    std::vector<int> areas;
};
// The 12 points of the pause world map (z_kaleido_map_collect.c lists the same order).
const Region kRegions[12] = {
    { "Haunted Wasteland e Desert Colossus", { 13, 11 } },
    { "Gerudo's Fortress", { 12 } },
    { "Gerudo Valley", { 9 } },
    { "Lake Hylia", { 6 } },
    { "Lon Lon Ranch", { 19 } },
    { "Mercado e Castelo de Hyrule", { 14, 15, 21 } },
    { "Hyrule Field", { 0 } },
    { "Death Mountain", { 16, 17, 18 } },
    { "Kakariko Village e Cemitério", { 1, 2 } },
    { "Lost Woods e Sacred Forest Meadow", { 10, 5 } },
    { "Kokiri Forest", { 4 } },
    { "Zora's River, Domain e Fountain", { 3, 7, 8 } },
};

bool AreaVisited(int area) {
    return (gSaveContext.worldMapAreaData & gBitFlags[area]) != 0;
}

struct Enemy {
    int counter;
    const char* name;
};
const Enemy kEnemies[] = {
    { COUNT_ENEMIES_DEFEATED_ANUBIS, "Anubis" },
    { COUNT_ENEMIES_DEFEATED_ARMOS, "Armos" },
    { COUNT_ENEMIES_DEFEATED_ARWING, "Arwing" },
    { COUNT_ENEMIES_DEFEATED_BARI, "Bari" },
    { COUNT_ENEMIES_DEFEATED_BEAMOS, "Beamos" },
    { COUNT_ENEMIES_DEFEATED_BIG_OCTO, "Big Octo" },
    { COUNT_ENEMIES_DEFEATED_BIRI, "Biri" },
    { COUNT_ENEMIES_DEFEATED_BUBBLE_GREEN, "Bubble verde" },
    { COUNT_ENEMIES_DEFEATED_BUBBLE_BLUE, "Bubble azul" },
    { COUNT_ENEMIES_DEFEATED_BUBBLE_WHITE, "Bubble branco" },
    { COUNT_ENEMIES_DEFEATED_BUBBLE_RED, "Bubble vermelho" },
    { COUNT_ENEMIES_DEFEATED_BUSINESS_SCRUB, "Deku comerciante" },
    { COUNT_ENEMIES_DEFEATED_DARK_LINK, "Dark Link" },
    { COUNT_ENEMIES_DEFEATED_DEAD_HAND, "Dead Hand" },
    { COUNT_ENEMIES_DEFEATED_DEKU_BABA, "Deku Baba" },
    { COUNT_ENEMIES_DEFEATED_DEKU_BABA_BIG, "Deku Baba (grande)" },
    { COUNT_ENEMIES_DEFEATED_DEKU_SCRUB, "Deku Scrub" },
    { COUNT_ENEMIES_DEFEATED_DINOLFOS, "Dinolfos" },
    { COUNT_ENEMIES_DEFEATED_DODONGO, "Dodongo" },
    { COUNT_ENEMIES_DEFEATED_DODONGO_BABY, "Dodongo bebê" },
    { COUNT_ENEMIES_DEFEATED_DOOR_TRAP, "Porta armadilha" },
    { COUNT_ENEMIES_DEFEATED_FLARE_DANCER, "Flare Dancer" },
    { COUNT_ENEMIES_DEFEATED_FLOORMASTER, "Floormaster" },
    { COUNT_ENEMIES_DEFEATED_FLYING_POT, "Pote voador" },
    { COUNT_ENEMIES_DEFEATED_FLOOR_TILE, "Ladrilho voador" },
    { COUNT_ENEMIES_DEFEATED_FREEZARD, "Freezard" },
    { COUNT_ENEMIES_DEFEATED_GERUDO_THIEF, "Ladra Gerudo" },
    { COUNT_ENEMIES_DEFEATED_GIBDO, "Gibdo" },
    { COUNT_ENEMIES_DEFEATED_GOHMA_LARVA, "Gohma (larva)" },
    { COUNT_ENEMIES_DEFEATED_GUAY, "Guay" },
    { COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE, "Iron Knuckle" },
    { COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE_NABOORU, "Iron Knuckle (Nabooru)" },
    { COUNT_ENEMIES_DEFEATED_KEESE, "Keese" },
    { COUNT_ENEMIES_DEFEATED_KEESE_FIRE, "Keese de fogo" },
    { COUNT_ENEMIES_DEFEATED_KEESE_ICE, "Keese de gelo" },
    { COUNT_ENEMIES_DEFEATED_LEEVER, "Leever" },
    { COUNT_ENEMIES_DEFEATED_LEEVER_BIG, "Leever (grande)" },
    { COUNT_ENEMIES_DEFEATED_LIKE_LIKE, "Like Like" },
    { COUNT_ENEMIES_DEFEATED_LIZALFOS, "Lizalfos" },
    { COUNT_ENEMIES_DEFEATED_MAD_SCRUB, "Mad Scrub" },
    { COUNT_ENEMIES_DEFEATED_MOBLIN, "Moblin" },
    { COUNT_ENEMIES_DEFEATED_MOBLIN_CLUB, "Moblin (clava)" },
    { COUNT_ENEMIES_DEFEATED_OCTOROK, "Octorok" },
    { COUNT_ENEMIES_DEFEATED_PARASITIC_TENTACLE, "Tentáculo parasita" },
    { COUNT_ENEMIES_DEFEATED_PEAHAT, "Peahat" },
    { COUNT_ENEMIES_DEFEATED_PEAHAT_LARVA, "Peahat (larva)" },
    { COUNT_ENEMIES_DEFEATED_POE, "Poe" },
    { COUNT_ENEMIES_DEFEATED_POE_BIG, "Big Poe" },
    { COUNT_ENEMIES_DEFEATED_POE_COMPOSER, "Poe compositor" },
    { COUNT_ENEMIES_DEFEATED_POE_SISTERS, "Irmãs Poe" },
    { COUNT_ENEMIES_DEFEATED_REDEAD, "Redead" },
    { COUNT_ENEMIES_DEFEATED_SHABOM, "Shabom" },
    { COUNT_ENEMIES_DEFEATED_SHELLBLADE, "Shell Blade" },
    { COUNT_ENEMIES_DEFEATED_SKULLTULA, "Skulltula" },
    { COUNT_ENEMIES_DEFEATED_SKULLTULA_BIG, "Skulltula (grande)" },
    { COUNT_ENEMIES_DEFEATED_SKULLTULA_GOLD, "Gold Skulltula" },
    { COUNT_ENEMIES_DEFEATED_SKULLWALLTULA, "Skullwalltula" },
    { COUNT_ENEMIES_DEFEATED_SKULL_KID, "Skull Kid" },
    { COUNT_ENEMIES_DEFEATED_SPIKE, "Spike" },
    { COUNT_ENEMIES_DEFEATED_STALCHILD, "Stalchild" },
    { COUNT_ENEMIES_DEFEATED_STALFOS, "Stalfos" },
    { COUNT_ENEMIES_DEFEATED_STINGER, "Stinger" },
    { COUNT_ENEMIES_DEFEATED_TAILPASARAN, "Tailpasaran" },
    { COUNT_ENEMIES_DEFEATED_TEKTITE_BLUE, "Tektite azul" },
    { COUNT_ENEMIES_DEFEATED_TEKTITE_RED, "Tektite vermelho" },
    { COUNT_ENEMIES_DEFEATED_TORCH_SLUG, "Torch Slug" },
    { COUNT_ENEMIES_DEFEATED_WALLMASTER, "Wallmaster" },
    { COUNT_ENEMIES_DEFEATED_WITHERED_DEKU_BABA, "Deku Baba seca" },
    { COUNT_ENEMIES_DEFEATED_WOLFOS, "Wolfos" },
    { COUNT_ENEMIES_DEFEATED_WOLFOS_WHITE, "White Wolfos" },
};

struct Flagged {
    const char* name;
    int (*check)();
};

#define EVENT(flag) []() -> int { return Flags_GetEventChkInf(flag) ? 1 : 0; }
#define INF(flag) []() -> int { return Flags_GetInfTable(flag) ? 1 : 0; }

const Flagged kCharacters[] = {
    { "Saria", INF(INFTABLE_GREETED_BY_SARIA) },
    { "Mido", EVENT(EVENTCHKINF_FIRST_SPOKE_TO_MIDO) },
    { "Grande Árvore Deku", EVENT(EVENTCHKINF_MET_DEKU_TREE) },
    { "Kaepora Gaebora", EVENT(EVENTCHKINF_SPOKE_TO_KAEPORA_BY_LOST_WOODS) },
    { "Malon", EVENT(EVENTCHKINF_SPOKE_TO_CHILD_MALON_AT_CASTLE_OR_MARKET) },
    { "Talon", INF(INFTABLE_TALKED_TO_TALON_IN_RANCH_HOUSE) },
    { "Ingo", EVENT(EVENTCHKINF_SPOKE_TO_INGO_AT_RANCH_BEFORE_TALON_RETURNS) },
    { "Princesa Zelda", EVENT(EVENTCHKINF_OBTAINED_ZELDAS_LETTER) },
    { "Impa", EVENT(EVENTCHKINF_LEARNED_ZELDAS_LULLABY) },
    { "Princesa Ruto", INF(INFTABLE_RUTO_IN_JJ_MEET_RUTO) },
    { "Rei Zora", EVENT(EVENTCHKINF_KING_ZORA_MOVED) },
    { "Sheik", EVENT(EVENTCHKINF_LEARNED_MINUET_OF_FOREST) },
    { "Link Goron", INF(INFTABLE_SPOKE_TO_GORON_LINK) },
    { "Darunia (Templo do Fogo)", INF(INFTABLE_SPOKE_TO_DARUNIA_IN_FIRE_TEMPLE) },
    { "Nabooru", EVENT(EVENTCHKINF_SPOKE_TO_NABOORU_IN_SPIRIT_TEMPLE) },
    { "Homem amaldiçoado da Casa das Skulltulas", EVENT(EVENTCHKINF_SPOKE_TO_CURSED_MAN_IN_SKULL_HOUSE) },
    { "Colecionador de Poes", INF(INFTABLE_SPOKE_TO_POE_COLLECTOR_IN_RUINED_MARKET) },
    { "Broto da Árvore Deku", EVENT(EVENTCHKINF_SPOKE_TO_DEKU_TREE_SPROUT) },
    { "Ganondorf (em batalha)", EVENT(EVENTCHKINF_BEGAN_GANONDORF_BATTLE) },
};

const Flagged kEvents[] = {
    { "A Grande Árvore Deku abriu a boca", EVENT(EVENTCHKINF_DEKU_TREE_OPENED_MOUTH) },
    { "Entrada da Dodongo's Cavern aberta", EVENT(EVENTCHKINF_BOMBED_DODONGOS_CAVERN_ENTRANCE) },
    { "Peixe oferecido a Lord Jabu-Jabu", EVENT(EVENTCHKINF_OFFERED_FISH_TO_JABU_JABU) },
    { "Ocarina of Time recebida", EVENT(EVENTCHKINF_OBTAINED_OCARINA_OF_TIME) },
    { "Porta do Tempo aberta", EVENT(EVENTCHKINF_OPENED_THE_DOOR_OF_TIME) },
    { "Master Sword retirada do pedestal", EVENT(EVENTCHKINF_PULLED_MASTER_SWORD_FROM_PEDESTAL) },
    { "Viagem no tempo para a fase adulta", EVENT(EVENTCHKINF_TIME_TRAVELED_TO_ADULT) },
    { "Epona conquistada", EVENT(EVENTCHKINF_EPONA_OBTAINED) },
    { "Poço de Kakariko drenado", EVENT(EVENTCHKINF_DRAINED_WELL_IN_KAKARIKO) },
    { "Água do Lake Hylia restaurada", EVENT(EVENTCHKINF_RAISED_LAKE_HYLIA_WATER) },
    { "Nabooru capturada por Twinrova", EVENT(EVENTCHKINF_NABOORU_CAPTURED_BY_TWINROVA) },
    { "Ponte arco-íris construída", EVENT(EVENTCHKINF_RAINBOW_BRIDGE_BUILT) },
    { "Provas do Castelo de Ganon: Floresta", EVENT(EVENTCHKINF_COMPLETED_FOREST_TRIAL) },
    { "Provas do Castelo de Ganon: Fogo", EVENT(EVENTCHKINF_COMPLETED_FIRE_TRIAL) },
    { "Provas do Castelo de Ganon: Água", EVENT(EVENTCHKINF_COMPLETED_WATER_TRIAL) },
    { "Provas do Castelo de Ganon: Espírito", EVENT(EVENTCHKINF_COMPLETED_SPIRIT_TRIAL) },
    { "Provas do Castelo de Ganon: Sombra", EVENT(EVENTCHKINF_COMPLETED_SHADOW_TRIAL) },
    { "Provas do Castelo de Ganon: Luz", EVENT(EVENTCHKINF_COMPLETED_LIGHT_TRIAL) },
    { "Barreira da Torre de Ganon dissipada", EVENT(EVENTCHKINF_DISPELLED_GANONS_TOWER_BARRIER) },
};

#undef EVENT
#undef INF

struct Boss {
    const char* name;
    int beganFlag; // -1: no "battle began" flag
    int timestamp;
};
const Boss kBosses[] = {
    { "Queen Gohma", EVENTCHKINF_BEGAN_GOHMA_BATTLE, TIMESTAMP_DEFEAT_GOHMA },
    { "King Dodongo", EVENTCHKINF_BEGAN_KING_DODONGO_BATTLE, TIMESTAMP_DEFEAT_KING_DODONGO },
    { "Barinade", EVENTCHKINF_BEGAN_BARINA_BATTLE, TIMESTAMP_DEFEAT_BARINADE },
    { "Phantom Ganon", EVENTCHKINF_BEGAN_PHANTOM_GANON_BATTLE, TIMESTAMP_DEFEAT_PHANTOM_GANON },
    { "Volvagia", EVENTCHKINF_BEGAN_VOLVAGIA_BATTLE, TIMESTAMP_DEFEAT_VOLVAGIA },
    { "Morpha", EVENTCHKINF_BEGAN_MORPHA_BATTLE, TIMESTAMP_DEFEAT_MORPHA },
    { "Bongo Bongo", EVENTCHKINF_BEGAN_BONGO_BONGO_BATTLE, TIMESTAMP_DEFEAT_BONGO_BONGO },
    { "Twinrova", EVENTCHKINF_BEGAN_TWINROVA_BATTLE, TIMESTAMP_DEFEAT_TWINROVA },
    { "Ganondorf", EVENTCHKINF_BEGAN_GANONDORF_BATTLE, TIMESTAMP_DEFEAT_GANONDORF },
    { "Ganon", -1, TIMESTAMP_DEFEAT_GANON },
};

const char* const kQuestNames[] = {
    "Medalhão da Floresta", "Medalhão do Fogo", "Medalhão da Água", "Medalhão do Espírito", "Medalhão da Sombra",
    "Medalhão da Luz", "Minueto da Floresta", "Bolero do Fogo", "Serenata da Água", "Réquiem do Espírito",
    "Noturno da Sombra", "Prelúdio da Luz", "Canção de Ninar da Zelda", "Canção da Epona", "Canção da Saria",
    "Canção do Sol", "Canção do Tempo", "Canção da Tempestade", "Esmeralda Kokiri", "Rubi Goron", "Safira Zora",
    "Pedra da Agonia", "Cartão Gerudo",
};

int sSelectedArea = -1;

bool HaveSave() {
    return gPlayState != nullptr && gSaveContext.fileNum >= 0 && gSaveContext.fileNum < 3;
}

void Status(bool done, const char* name) {
    ImGui::TextColored(done ? kGold : kDim, "%s %s", done ? "[X]" : "[ ]", name);
}

void DrawMap() {
    // Map space: x -110..116, y -70..60 (the world map page, y up). Fit it in the available width.
    const float mapW = 226.0f, mapH = 130.0f, minX = -110.0f, maxY = 60.0f;
    const float width = std::min(ImGui::GetContentRegionAvail().x, 720.0f);
    const float scale = width / mapW;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton("DEHyruleMap", ImVec2(width, mapH * scale));
    const bool clicked = ImGui::IsItemClicked();
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + mapH * scale), IM_COL32(28, 34, 46, 255), 6.0f);
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + mapH * scale), IM_COL32(200, 165, 70, 255), 6.0f, 0,
                  2.0f);

    // Big areas first so the small ones inside them stay visible and clickable.
    std::vector<int> order;
    for (int i = 0; i < 22; i++) {
        if (i != kUnknownArea && !(i == 21 && LINK_IS_CHILD) && !(i == 15 && LINK_IS_ADULT)) {
            order.push_back(i);
        }
    }
    std::sort(order.begin(), order.end(),
              [](int a, int b) { return kAreas[a].w * kAreas[a].h > kAreas[b].w * kAreas[b].h; });

    int hovered = -1;
    for (int i : order) {
        const Area& area = kAreas[i];
        const ImVec2 a(origin.x + (area.x - minX) * scale, origin.y + (maxY - area.y) * scale);
        const ImVec2 b(a.x + area.w * scale, a.y + area.h * scale);
        const bool visited = AreaVisited(i);
        const bool here = gSaveContext.worldMapArea == i;
        draw->AddRectFilled(a, b, visited ? IM_COL32(40, 90, 160, 170) : IM_COL32(70, 70, 76, 150), 3.0f);
        draw->AddRect(a, b,
                      here ? IM_COL32(255, 215, 60, 255)
                           : (sSelectedArea == i ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 160, 200)),
                      3.0f, 0, here || sSelectedArea == i ? 2.5f : 1.0f);
        if (mouse.x >= a.x && mouse.x < b.x && mouse.y >= a.y && mouse.y < b.y) {
            hovered = i; // the last one drawn (the smallest) wins
        }
    }
    for (int i : order) {
        if (!AreaVisited(i) && sSelectedArea != i) {
            continue;
        }
        const Area& area = kAreas[i];
        const ImVec2 a(origin.x + (area.x - minX) * scale + 3.0f, origin.y + (maxY - area.y) * scale + 2.0f);
        if (area.w * scale > 60.0f) {
            draw->AddText(a, IM_COL32(235, 235, 235, 255), area.name);
        }
    }
    if (clicked && hovered >= 0) {
        sSelectedArea = hovered;
    }
    if (hovered >= 0) {
        ImGui::SetTooltip("%s: %s", kAreas[hovered].name, AreaVisited(hovered) ? "visitado" : "ainda não visitado");
    }
}

void DrawRegions() {
    int visitedAreas = 0, totalAreas = 0;
    for (int i = 0; i < 22; i++) {
        if (i != kUnknownArea && i != 21) {
            totalAreas++;
            visitedAreas += AreaVisited(i) ? 1 : 0;
        }
    }
    ImGui::TextColored(kGold, "Áreas visitadas: %d de %d", visitedAreas, totalAreas);

    if (!ImGui::BeginTable("DERegions", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
        return;
    }
    ImGui::TableSetupColumn("Região", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Áreas");
    ImGui::TableSetupColumn("Skulltulas");
    ImGui::TableSetupColumn("Pedaços de coração");
    ImGui::TableHeadersRow();
    for (int r = 0; r < 12; r++) {
        const Region& region = kRegions[r];
        int seen = 0;
        bool selected = false;
        for (int area : region.areas) {
            seen += AreaVisited(area) ? 1 : 0;
            selected = selected || area == sSelectedArea;
        }
        s16 skullGot, skullTotal, heartGot, heartTotal;
        KaleidoScope_MapCollectRegionCounts(gPlayState, r, &skullGot, &skullTotal, &heartGot, &heartTotal);
        const bool complete = skullGot == skullTotal && heartGot == heartTotal;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::PushID(r);
        if (ImGui::Selectable(region.name, selected, ImGuiSelectableFlags_SpanAllColumns)) {
            sSelectedArea = region.areas[0];
        }
        ImGui::PopID();
        ImGui::TableNextColumn();
        ImGui::TextColored(seen == static_cast<int>(region.areas.size()) ? kOk : kDim, "%d/%d", seen,
                           static_cast<int>(region.areas.size()));
        ImGui::TableNextColumn();
        ImGui::TextColored(complete ? kGold : ImVec4(1, 1, 1, 1), "%d/%d", skullGot, skullTotal);
        ImGui::TableNextColumn();
        ImGui::TextColored(complete ? kGold : ImVec4(1, 1, 1, 1), "%d/%d", heartGot, heartTotal);
    }
    ImGui::EndTable();
    ImGui::TextColored(kDim, "%s", "Skulltulas e pedaços de coração das masmorras não entram nas regiões (são contados "
                                   "como no mapa do menu de pausa).");
}

void DrawSelectedArea() {
    if (sSelectedArea < 0) {
        ImGui::TextColored(kDim, "%s", "Toque numa área do mapa ou numa região da lista para ver os detalhes.");
        return;
    }
    const Area& area = kAreas[sSelectedArea];
    ImGui::TextColored(kGold, "%s", area.name);
    ImGui::SameLine();
    ImGui::TextUnformatted(AreaVisited(sSelectedArea) ? "- visitada" : "- ainda não visitada");
    if (gSaveContext.worldMapArea == sSelectedArea) {
        ImGui::SameLine();
        ImGui::TextColored(kGold, "%s", "(você está aqui)");
    }
}

void DrawPlaces() {
    std::vector<int> visited;
    for (int scene = 0; scene < SCENE_ID_MAX; scene++) {
        const u32 index = static_cast<u32>(scene) / 32;
        if (index < ARRAY_COUNT(gSaveContext.ship.stats.scenesDiscovered) &&
            (gSaveContext.ship.stats.scenesDiscovered[index] & (1u << (scene % 32)))) {
            visited.push_back(scene);
        }
    }
    ImGui::TextColored(kGold, "Lugares visitados: %d", static_cast<int>(visited.size()));
    ImGui::TextColored(kDim, "%s", "Registrados a partir desta versão (o jogo original não guardava essa lista). As "
                                   "áreas do mapa acima valem desde o começo do save.");
    for (int scene : visited) {
        ImGui::BulletText("%s", scene < 110 ? SohUtils::GetSceneName(scene).c_str() : "?");
    }
}

void DrawAlbum() {
    if (ImGui::BeginTabBar("DEAlbumTabs")) {
        if (ImGui::BeginTabItem("Personagens")) {
            int met = 0;
            for (const Flagged& c : kCharacters) {
                met += c.check();
            }
            ImGui::TextColored(kGold, "%d de %d encontrados", met, static_cast<int>(ARRAY_COUNT(kCharacters)));
            for (const Flagged& c : kCharacters) {
                Status(c.check() != 0, c.check() ? c.name : "???");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chefes")) {
            for (const Boss& boss : kBosses) {
                const bool defeated = gSaveContext.ship.stats.itemTimestamp[boss.timestamp] != 0;
                const bool faced = defeated || (boss.beganFlag >= 0 && Flags_GetEventChkInf(boss.beganFlag));
                Status(defeated, faced ? boss.name : "???");
                if (faced) {
                    ImGui::SameLine();
                    ImGui::TextColored(defeated ? kOk : kDim, "%s", defeated ? "derrotado" : "enfrentado");
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Inimigos")) {
            int kinds = 0;
            for (const Enemy& enemy : kEnemies) {
                kinds += gSaveContext.ship.stats.count[enemy.counter] > 0 ? 1 : 0;
            }
            ImGui::TextColored(kGold, "%d de %d tipos derrotados", kinds, static_cast<int>(ARRAY_COUNT(kEnemies)));
            if (ImGui::BeginTable("DEEnemies", 2, ImGuiTableFlags_RowBg)) {
                for (const Enemy& enemy : kEnemies) {
                    const u32 count = gSaveContext.ship.stats.count[enemy.counter];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextColored(count ? ImVec4(1, 1, 1, 1) : kDim, "%s", count ? enemy.name : "???");
                    ImGui::TableNextColumn();
                    if (count) {
                        ImGui::Text("%u", count);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Itens")) {
            for (int slot = 0; slot < 24; slot++) {
                const u8 item = gSaveContext.inventory.items[slot];
                if (item <= ITEM_CLAIM_CHECK) {
                    ImGui::BulletText("%s", SohUtils::GetItemName(item).c_str());
                }
            }
            for (int quest = QUEST_MEDALLION_FOREST; quest <= QUEST_GERUDO_CARD; quest++) {
                if (CHECK_QUEST_ITEM(quest)) {
                    ImGui::BulletText("%s", kQuestNames[quest]);
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Eventos")) {
            for (const Flagged& e : kEvents) {
                Status(e.check() != 0, e.check() ? e.name : "???");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lugares")) {
            DrawPlaces();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

} // namespace

void DrawExplorer() {
    if (!HaveSave()) {
        ImGui::TextWrapped("%s", "Carregue um save dos slots 1 a 3 para ver o mapa e o álbum dele. Nada aqui é gravado "
                                 "no save: tudo é lido do progresso da partida.");
        return;
    }
    if (IS_BOSS_RUSH) {
        ImGui::TextWrapped("%s", "O Boss Rush não tem exploração: volte a um save da campanha.");
        return;
    }
    if (ImGui::BeginTabBar("DEExplorerTabs")) {
        if (ImGui::BeginTabItem("Mapa de Hyrule")) {
            DrawMap();
            DrawSelectedArea();
            ImGui::Spacing();
            DrawRegions();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Álbum")) {
            DrawAlbum();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

} // namespace DefinitiveExpansion
