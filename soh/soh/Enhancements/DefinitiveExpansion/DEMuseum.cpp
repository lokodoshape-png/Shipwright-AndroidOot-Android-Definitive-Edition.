// OOT D.E. Definitive Expansion, phase 3: Museum and Link Customization.
//
// Museum: a catalogue of the game's items, grouped by category. Each entry shows the game's own icon (the textures the
// port already registers for its menus, RegisterImGuiItemIcons), a description and whether the save being played has
// it. Entries stay locked until the save really has the item. A 3D view of the models is not done yet: the port has no
// way to draw a game model inside a menu window, so the icon is shown and labelled as such.
//
// Link Customization: tunic, hair, boots, shield and sword colours. They are the Ship of Harkinian Cosmetics Editor
// options, changed through that editor's own functions, so nothing else of the game (hitboxes, damage, physics) is
// touched and "restore" brings back the original colours. Changes are shown first and only applied on "Aplicar".
#include "DefinitiveExpansion.h"

#include <imgui.h>
#include <libultraship/libultraship.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "soh/OTRGlobals.h"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/Enhancements/cosmetics/CosmeticsEditor.h"

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
// Museum
// ---------------------------------------------------------------------------------------------------------------

enum class Category { Weapons, Shields, Equipment, Masks, Instruments, Special };

const char* CategoryLabel(Category category) {
    switch (category) {
        case Category::Weapons:
            return "Armas";
        case Category::Shields:
            return "Escudos";
        case Category::Equipment:
            return "Equipamentos";
        case Category::Masks:
            return "Máscaras";
        case Category::Instruments:
            return "Instrumentos";
        case Category::Special:
            return "Itens especiais";
    }
    return "";
}

bool Has(int item) {
    return INV_CONTENT(item) == item;
}

bool Owns(int type, int value) {
    return CHECK_OWNED_EQUIP(type, value) != 0;
}

bool MasksSold() {
    return Flags_GetEventChkInf(EVENTCHKINF_PAID_BACK_KEATON_MASK_FEE) &&
           Flags_GetEventChkInf(EVENTCHKINF_PAID_BACK_SKULL_MASK_FEE) &&
           Flags_GetEventChkInf(EVENTCHKINF_PAID_BACK_SPOOKY_MASK_FEE) &&
           Flags_GetEventChkInf(EVENTCHKINF_PAID_BACK_BUNNY_HOOD_FEE);
}

bool MaskSeen(int item, int paidFlag) {
    return INV_CONTENT(ITEM_MASK_KEATON) == item || (paidFlag >= 0 && Flags_GetEventChkInf(paidFlag)) ||
           (paidFlag < 0 && MasksSold());
}

int BottleCount() {
    int bottles = 0;
    for (int i = 0; i < 4; i++) {
        bottles += gSaveContext.inventory.items[SLOT_BOTTLE_1 + i] != ITEM_NONE ? 1 : 0;
    }
    return bottles;
}

struct Exhibit {
    Category category;
    int icon; // item id: the icon registered by the port
    const char* name;
    const char* description;
    bool (*owned)();
    const char* unlockRule; // shown while locked
};

const Exhibit kExhibits[] = {
    // Armas
    { Category::Weapons, ITEM_SWORD_KOKIRI, "Kokiri Sword", "A espada guardada pelos Kokiri. Curta e leve.",
      []() { return Owns(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_KOKIRI); }, "Encontrada na Kokiri Forest." },
    { Category::Weapons, ITEM_SWORD_MASTER, "Master Sword", "A lâmina do mal banido, guardada no Templo do Tempo.",
      []() { return Owns(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER); }, "Repousa no Templo do Tempo." },
    { Category::Weapons, ITEM_SWORD_BGS, "Biggoron's Sword", "Forjada por Biggoron. Usada com as duas mãos.",
      []() { return Owns(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON) && gSaveContext.bgsFlag != 0; },
      "Resultado de uma longa troca de itens." },
    { Category::Weapons, ITEM_STICK, "Deku Stick", "Um galho que serve de arma e de tocha.",
      []() { return Has(ITEM_STICK); }, "Deixado por Deku Babas." },
    { Category::Weapons, ITEM_NUT, "Deku Nut", "Atordoa os inimigos com um clarão.", []() { return Has(ITEM_NUT); },
      "Deixado por plantas e arbustos." },
    { Category::Weapons, ITEM_SLINGSHOT, "Fairy Slingshot", "Atira Sementes Deku.",
      []() { return Has(ITEM_SLINGSHOT); }, "Escondido dentro da Grande Árvore Deku." },
    { Category::Weapons, ITEM_BOOMERANG, "Boomerang", "Vai e volta, atordoando o que encontra.",
      []() { return Has(ITEM_BOOMERANG); }, "Escondido no ventre de Jabu-Jabu." },
    { Category::Weapons, ITEM_BOMB, "Bombas", "Explodem paredes rachadas e inimigos.", []() { return Has(ITEM_BOMB); },
      "Os Gorons conhecem a sua origem." },
    { Category::Weapons, ITEM_BOMBCHU, "Bombchu", "Bomba que corre pelo chão e pelas paredes.",
      []() { return Has(ITEM_BOMBCHU); }, "Vendida ou ganha em jogos." },
    { Category::Weapons, ITEM_BOW, "Fairy Bow", "Arco do Templo da Floresta.", []() { return Has(ITEM_BOW); },
      "Guardado no Templo da Floresta." },
    { Category::Weapons, ITEM_ARROW_FIRE, "Fire Arrows", "Flechas com o poder do fogo.",
      []() { return Has(ITEM_ARROW_FIRE); }, "Ligadas ao sol nascente do Lake Hylia." },
    { Category::Weapons, ITEM_ARROW_ICE, "Ice Arrows", "Flechas que congelam.", []() { return Has(ITEM_ARROW_ICE); },
      "Prêmio de um treino difícil." },
    { Category::Weapons, ITEM_ARROW_LIGHT, "Light Arrows", "A flecha que fere o mal.",
      []() { return Has(ITEM_ARROW_LIGHT); }, "Entregue no fim da jornada dos sábios." },
    { Category::Weapons, ITEM_HOOKSHOT, "Hookshot", "Corrente com gancho que puxa Link até os alvos.",
      []() { return Has(ITEM_HOOKSHOT) || Has(ITEM_LONGSHOT); }, "Levado por alguém que já se foi." },
    { Category::Weapons, ITEM_LONGSHOT, "Longshot", "A versão com o dobro do alcance.",
      []() { return Has(ITEM_LONGSHOT); }, "Guardado no Templo da Água." },
    { Category::Weapons, ITEM_HAMMER, "Megaton Hammer", "Martelo pesado que quebra rochas e pilares.",
      []() { return Has(ITEM_HAMMER); }, "Guardado no Templo do Fogo." },

    // Escudos
    { Category::Shields, ITEM_SHIELD_DEKU, "Deku Shield", "Escudo de madeira. Queima no fogo.",
      []() { return Owns(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_DEKU); }, "Vendido na Kokiri Forest." },
    { Category::Shields, ITEM_SHIELD_HYLIAN, "Hylian Shield", "Escudo de metal dos soldados de Hyrule.",
      []() { return Owns(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_HYLIAN); }, "Vendido no Mercado." },
    { Category::Shields, ITEM_SHIELD_MIRROR, "Mirror Shield", "Reflete a luz e certos feitiços.",
      []() { return Owns(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_MIRROR); }, "Guardado no Templo do Espírito." },

    // Equipamentos
    { Category::Equipment, ITEM_TUNIC_KOKIRI, "Kokiri Tunic", "A roupa de sempre.",
      []() { return Owns(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_KOKIRI); }, "" },
    { Category::Equipment, ITEM_TUNIC_GORON, "Goron Tunic", "Resiste ao calor intenso.",
      []() { return Owns(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_GORON); }, "Feita pelos Gorons." },
    { Category::Equipment, ITEM_TUNIC_ZORA, "Zora Tunic", "Permite respirar debaixo d'água.",
      []() { return Owns(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_ZORA); }, "Feita pelos Zoras." },
    { Category::Equipment, ITEM_BOOTS_KOKIRI, "Kokiri Boots", "As botas de sempre.",
      []() { return Owns(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_KOKIRI); }, "" },
    { Category::Equipment, ITEM_BOOTS_IRON, "Iron Boots", "Pesadas: afundam na água e resistem ao vento.",
      []() { return Owns(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON); }, "Escondidas numa caverna gelada." },
    { Category::Equipment, ITEM_BOOTS_HOVER, "Hover Boots", "Flutuam por um instante sobre o vazio.",
      []() { return Owns(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_HOVER); }, "Escondidas num lugar sombrio." },
    { Category::Equipment, ITEM_BRACELET, "Goron's Bracelet", "Permite erguer Bomb Flowers.",
      []() { return CUR_UPG_VALUE(UPG_STRENGTH) >= 1; }, "Presente de um amigo Goron." },
    { Category::Equipment, ITEM_GAUNTLETS_SILVER, "Silver Gauntlets", "Erguem blocos pesados.",
      []() { return CUR_UPG_VALUE(UPG_STRENGTH) >= 2; }, "Guardadas no Templo do Espírito." },
    { Category::Equipment, ITEM_GAUNTLETS_GOLD, "Golden Gauntlets", "Erguem os pilares mais pesados.",
      []() { return CUR_UPG_VALUE(UPG_STRENGTH) >= 3; }, "Guardadas no Castelo de Ganon." },
    { Category::Equipment, ITEM_SCALE_SILVER, "Silver Scale", "Mergulhos mais fundos.",
      []() { return CUR_UPG_VALUE(UPG_SCALE) >= 1; }, "Prêmio de um mergulho." },
    { Category::Equipment, ITEM_SCALE_GOLDEN, "Golden Scale", "Os mergulhos mais fundos.",
      []() { return CUR_UPG_VALUE(UPG_SCALE) >= 2; }, "Prêmio de uma pescaria." },

    // Máscaras
    { Category::Masks, ITEM_MASK_KEATON, "Keaton Mask", "A máscara da raposa amarela.",
      []() { return MaskSeen(ITEM_MASK_KEATON, EVENTCHKINF_PAID_BACK_KEATON_MASK_FEE); },
      "Emprestada pela Happy Mask Shop." },
    { Category::Masks, ITEM_MASK_SKULL, "Skull Mask", "Uma máscara de caveira com chifres.",
      []() { return MaskSeen(ITEM_MASK_SKULL, EVENTCHKINF_PAID_BACK_SKULL_MASK_FEE); },
      "Emprestada pela Happy Mask Shop." },
    { Category::Masks, ITEM_MASK_SPOOKY, "Spooky Mask", "Uma máscara assustadora.",
      []() { return MaskSeen(ITEM_MASK_SPOOKY, EVENTCHKINF_PAID_BACK_SPOOKY_MASK_FEE); },
      "Emprestada pela Happy Mask Shop." },
    { Category::Masks, ITEM_MASK_BUNNY, "Bunny Hood", "Orelhas de coelho. Dizem que dão pressa.",
      []() { return MaskSeen(ITEM_MASK_BUNNY, EVENTCHKINF_PAID_BACK_BUNNY_HOOD_FEE); },
      "Emprestada pela Happy Mask Shop." },
    { Category::Masks, ITEM_MASK_GORON, "Goron Mask", "Rosto de um Goron.", []() { return MaskSeen(ITEM_MASK_GORON, -1); },
      "Disponível depois de vender as quatro primeiras máscaras." },
    { Category::Masks, ITEM_MASK_ZORA, "Zora Mask", "Rosto de um Zora.", []() { return MaskSeen(ITEM_MASK_ZORA, -1); },
      "Disponível depois de vender as quatro primeiras máscaras." },
    { Category::Masks, ITEM_MASK_GERUDO, "Gerudo Mask", "Rosto de uma Gerudo.",
      []() { return MaskSeen(ITEM_MASK_GERUDO, -1); }, "Disponível depois de vender as quatro primeiras máscaras." },
    { Category::Masks, ITEM_MASK_TRUTH, "Mask of Truth", "Mostra o que está escondido e ouve as Gossip Stones.",
      []() { return MaskSeen(ITEM_MASK_TRUTH, -1); }, "Disponível depois de vender as quatro primeiras máscaras." },

    // Instrumentos
    { Category::Instruments, ITEM_OCARINA_FAIRY, "Fairy Ocarina", "Presente de Saria antes da partida.",
      []() { return Has(ITEM_OCARINA_FAIRY) || Has(ITEM_OCARINA_TIME); }, "Uma amiga entrega na ponte." },
    { Category::Instruments, ITEM_OCARINA_TIME, "Ocarina of Time", "O tesouro da família real.",
      []() { return Has(ITEM_OCARINA_TIME); }, "Guardada pela princesa." },

    // Itens especiais
    { Category::Special, ITEM_LENS, "Lens of Truth", "Revela o que é invisível.", []() { return Has(ITEM_LENS); },
      "Escondida no fundo do poço." },
    { Category::Special, ITEM_DINS_FIRE, "Din's Fire", "Uma explosão de fogo ao redor de Link.",
      []() { return Has(ITEM_DINS_FIRE); }, "Presente de uma Grande Fada." },
    { Category::Special, ITEM_FARORES_WIND, "Farore's Wind", "Cria um ponto de retorno dentro das masmorras.",
      []() { return Has(ITEM_FARORES_WIND); }, "Presente de uma Grande Fada." },
    { Category::Special, ITEM_NAYRUS_LOVE, "Nayru's Love", "Uma barreira que bloqueia o dano.",
      []() { return Has(ITEM_NAYRUS_LOVE); }, "Presente de uma Grande Fada." },
    { Category::Special, ITEM_BEAN, "Magic Bean", "Plantado no lugar certo, vira uma plataforma.",
      []() { return Has(ITEM_BEAN); }, "Vendido perto do Lake Hylia." },
    { Category::Special, ITEM_BOTTLE, "Garrafa", "Guarda fadas, poções, insetos e mais.",
      []() { return BottleCount() > 0; }, "Há quatro em Hyrule." },
    { Category::Special, ITEM_LETTER_ZELDA, "Zelda's Letter", "Uma carta com a assinatura da princesa.",
      []() { return Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_ZELDAS_LETTER) != 0; }, "Entregue no castelo." },
};

int sSelected = -1;
float sZoom = 2.0f;

void IconOrPlaceholder(int item, bool owned, float size) {
    auto mapping = itemMapping.find(item);
    ImTextureID texture = nullptr;
    if (mapping != itemMapping.end()) {
        texture = Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(
            owned ? mapping->second.name : mapping->second.nameFaded);
    }
    if (texture != nullptr) {
        ImGui::Image(texture, ImVec2(size, size));
    } else {
        // No icon in the loaded game files: say so instead of showing something else.
        ImGui::Button("sem ícone", ImVec2(size, size));
    }
}

void DrawExhibitDetail(const Exhibit& exhibit, bool owned) {
    ImGui::BeginGroup();
    IconOrPlaceholder(exhibit.icon, owned, 32.0f * sZoom);
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextColored(kGold, "%s", owned ? exhibit.name : "???");
    ImGui::TextColored(kDim, "%s", CategoryLabel(exhibit.category));
    if (owned) {
        ImGui::TextWrapped("%s", exhibit.description);
        ImGui::TextColored(kOk, "%s", "Desbloqueado neste save");
    } else {
        ImGui::TextWrapped("%s", exhibit.unlockRule[0] ? exhibit.unlockRule : "Ainda não encontrado.");
        ImGui::TextColored(kDim, "%s", "Bloqueado: o save ainda não tem este item");
    }
    ImGui::TextColored(kDim, "%s", "Mostrando o ícone do jogo. Modelo 3D com rotação: ainda não disponível.");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderFloat("Zoom", &sZoom, 1.0f, 4.0f, "%.1fx");
    ImGui::EndGroup();
}

// ---------------------------------------------------------------------------------------------------------------
// Link Customization
// ---------------------------------------------------------------------------------------------------------------

struct Part {
    const char* id; // Cosmetics Editor option id
    const char* label;
};
const Part kParts[] = {
    { "Link.KokiriTunic", "Túnica Kokiri (verde)" },
    { "Link.GoronTunic", "Túnica Goron (vermelha)" },
    { "Link.ZoraTunic", "Túnica Zora (azul)" },
    { "Link.Hair", "Cabelo" },
    { "Link.Boots", "Botas" },
    { "MirrorShield.Body", "Mirror Shield" },
    { "Swords.KokiriBlade", "Lâmina da Kokiri Sword" },
    { "Swords.MasterBlade", "Lâmina da Master Sword" },
    { "Swords.BiggoronBlade", "Lâmina da Biggoron's Sword" },
    { "Trails.MasterSword", "Rastro da Master Sword" },
};
constexpr int kPartCount = ARRAY_COUNT(kParts);

struct Look {
    const char* name;
    const char* achievement; // unlocked by this achievement; null = always
    const char* unlockText;
    Color_RGBA8 colors[kPartCount];
};
#define C(r, g, b) Color_RGBA8{ r, g, b, 255 }
const Look kLooks[] = {
    { "Clássico", nullptr, "",
      { C(30, 105, 27), C(100, 20, 0), C(0, 60, 100), C(255, 173, 27), C(93, 44, 18), C(215, 0, 0), C(255, 255, 255),
        C(255, 255, 255), C(255, 255, 255), C(255, 255, 255) } },
    { "Sombra", "combat.dark_link", "Derrote o Dark Link",
      { C(40, 40, 48), C(60, 10, 20), C(20, 20, 60), C(200, 200, 210), C(25, 25, 30), C(60, 0, 80), C(180, 180, 200),
        C(150, 120, 220), C(180, 180, 200), C(120, 60, 200) } },
    { "Sábio", "story.medallions", "Reúna os seis medalhões",
      { C(240, 240, 235), C(200, 60, 40), C(60, 120, 220), C(255, 220, 120), C(200, 170, 120), C(230, 200, 60),
        C(255, 255, 255), C(200, 230, 255), C(255, 255, 255), C(180, 220, 255) } },
    { "Herói Dourado", "story.hero", "Derrote Ganon",
      { C(200, 160, 40), C(190, 110, 20), C(40, 110, 170), C(255, 200, 60), C(150, 100, 30), C(230, 180, 40),
        C(255, 230, 150), C(255, 215, 90), C(255, 230, 150), C(255, 200, 60) } },
};
#undef C

Color_RGBA8 sDraft[kPartCount];
bool sDraftLoaded = false;
bool sDraftChanged = false;

void LoadDraftFromGame() {
    for (int i = 0; i < kPartCount; i++) {
        if (!CosmeticsEditor_GetColor(kParts[i].id, &sDraft[i])) {
            sDraft[i] = Color_RGBA8{ 255, 255, 255, 255 };
        }
    }
    sDraftLoaded = true;
    sDraftChanged = false;
}

void ApplyDraft() {
    for (int i = 0; i < kPartCount; i++) {
        CosmeticsEditor_SetColor(kParts[i].id, sDraft[i]);
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ApplyOrResetCustomGfxPatches();
    sDraftChanged = false;
}

void ResetToOriginal() {
    for (const Part& part : kParts) {
        CosmeticsEditor_ResetColor(part.id);
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ApplyOrResetCustomGfxPatches();
    LoadDraftFromGame();
}

std::string EncodeDraft() {
    std::string text;
    char buffer[8];
    for (int i = 0; i < kPartCount; i++) {
        std::snprintf(buffer, sizeof(buffer), "%02X%02X%02X", sDraft[i].r, sDraft[i].g, sDraft[i].b);
        text += (i ? "," : "") + std::string(buffer);
    }
    return text;
}

bool DecodeDraft(const std::string& text) {
    Color_RGBA8 colors[kPartCount];
    size_t pos = 0;
    for (int i = 0; i < kPartCount; i++) {
        if (pos + 6 > text.size()) {
            return false;
        }
        unsigned int value = 0;
        if (std::sscanf(text.substr(pos, 6).c_str(), "%06X", &value) != 1) {
            return false;
        }
        colors[i] = Color_RGBA8{ static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 8),
                                 static_cast<uint8_t>(value), 255 };
        pos += 7;
    }
    std::copy(colors, colors + kPartCount, sDraft);
    return true;
}

const char* kSlotKeys[3] = { "link.look.1", "link.look.2", "link.look.3" };

ImVec4 ToVec(const Color_RGBA8& color) {
    return ImVec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, 1.0f);
}

void DrawPreview() {
    // A flat figure built from the chosen colours: hair, tunic (the one Link is wearing), boots, shield and sword.
    const int tunic = std::clamp<int>(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC) - 1, 0, 2);
    const ImVec2 o = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float s = 1.6f;
    auto col = [](const Color_RGBA8& c) { return IM_COL32(c.r, c.g, c.b, 255); };
    draw->AddRectFilled(o, ImVec2(o.x + 150 * s, o.y + 120 * s), IM_COL32(28, 34, 46, 255), 6.0f);
    draw->AddRect(o, ImVec2(o.x + 150 * s, o.y + 120 * s), IM_COL32(200, 165, 70, 255), 6.0f, 0, 2.0f);
    const ImVec2 c(o.x + 60 * s, o.y + 14 * s);
    draw->AddTriangleFilled(ImVec2(c.x - 14 * s, c.y + 14 * s), ImVec2(c.x + 14 * s, c.y + 14 * s),
                            ImVec2(c.x + 26 * s, c.y - 4 * s), col(sDraft[tunic])); // cap
    draw->AddCircleFilled(ImVec2(c.x, c.y + 22 * s), 10 * s, IM_COL32(240, 200, 160, 255)); // face
    draw->AddRectFilled(ImVec2(c.x - 12 * s, c.y + 12 * s), ImVec2(c.x + 12 * s, c.y + 18 * s), col(sDraft[3]));
    draw->AddRectFilled(ImVec2(c.x - 14 * s, c.y + 32 * s), ImVec2(c.x + 14 * s, c.y + 70 * s), col(sDraft[tunic]),
                        4.0f); // tunic
    draw->AddRectFilled(ImVec2(c.x - 12 * s, c.y + 70 * s), ImVec2(c.x - 3 * s, c.y + 96 * s), col(sDraft[4]));
    draw->AddRectFilled(ImVec2(c.x + 3 * s, c.y + 70 * s), ImVec2(c.x + 12 * s, c.y + 96 * s), col(sDraft[4]));
    draw->AddRectFilled(ImVec2(c.x - 34 * s, c.y + 36 * s), ImVec2(c.x - 16 * s, c.y + 66 * s), col(sDraft[5]),
                        6.0f); // shield
    draw->AddLine(ImVec2(c.x + 18 * s, c.y + 60 * s), ImVec2(c.x + 54 * s, c.y + 4 * s), col(sDraft[7]), 4.0f * s);
    draw->AddLine(ImVec2(c.x + 22 * s, c.y + 64 * s), ImVec2(c.x + 58 * s, c.y + 8 * s), col(sDraft[9]), 1.5f * s);
    ImGui::Dummy(ImVec2(150 * s, 120 * s));
    ImGui::TextColored(kDim, "%s", "Prévia em 2D. Para ver no modelo 3D, aplique e abra o menu de pausa > Equipamento.");
}

} // namespace

void RegisterLinkSettings(DE::Config& config) {
    for (const char* key : kSlotKeys) {
        config.RegisterSetting(key, std::string(), "Visual do Link guardado (cores em hexadecimal).");
    }
}

void DrawMuseum() {
    if (gPlayState == nullptr || gSaveContext.fileNum < 0 || gSaveContext.fileNum >= 3 || IS_BOSS_RUSH) {
        ImGui::TextWrapped("%s", "Carregue um save dos slots 1 a 3: o museu mostra o que esse save já encontrou. Itens "
                                 "ainda não encontrados ficam bloqueados.");
        return;
    }
    int owned = 0;
    for (const Exhibit& exhibit : kExhibits) {
        owned += exhibit.owned() ? 1 : 0;
    }
    ImGui::TextColored(kGold, "Acervo: %d de %d peças", owned, static_cast<int>(ARRAY_COUNT(kExhibits)));

    if (sSelected >= 0) {
        DrawExhibitDetail(kExhibits[sSelected], kExhibits[sSelected].owned());
        ImGui::Separator();
    }

    if (ImGui::BeginTabBar("DEMuseumTabs")) {
        static const Category kCategories[] = { Category::Weapons, Category::Shields, Category::Equipment,
                                                Category::Masks,   Category::Instruments, Category::Special };
        for (Category category : kCategories) {
            if (!ImGui::BeginTabItem(CategoryLabel(category))) {
                continue;
            }
            const float cell = 64.0f;
            const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / (cell + 12.0f)));
            int column = 0;
            for (int i = 0; i < static_cast<int>(ARRAY_COUNT(kExhibits)); i++) {
                const Exhibit& exhibit = kExhibits[i];
                if (exhibit.category != category) {
                    continue;
                }
                if (column > 0) {
                    ImGui::SameLine();
                }
                ImGui::PushID(i);
                ImGui::BeginGroup();
                const bool has = exhibit.owned();
                const ImVec2 start = ImGui::GetCursorScreenPos();
                if (ImGui::InvisibleButton("exhibit", ImVec2(cell, cell))) {
                    sSelected = i;
                }
                const bool focused = ImGui::IsItemHovered() || ImGui::IsItemFocused() || sSelected == i;
                ImGui::GetWindowDrawList()->AddRect(start, ImVec2(start.x + cell, start.y + cell),
                                                    focused ? IM_COL32(255, 215, 60, 255) : IM_COL32(90, 90, 100, 255),
                                                    6.0f, 0, focused ? 2.5f : 1.0f);
                ImGui::SetCursorScreenPos(ImVec2(start.x + 8.0f, start.y + 8.0f));
                IconOrPlaceholder(exhibit.icon, has, cell - 16.0f);
                ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + cell + 2.0f));
                ImGui::Dummy(ImVec2(cell, 0.0f));
                ImGui::EndGroup();
                ImGui::PopID();
                column = (column + 1) % columns;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Personagens, chefes e inimigos")) {
            ImGui::TextWrapped("%s", "Quem você já encontrou, enfrentou e derrotou está em Definitive > Hyrule Explorer "
                                     "> Álbum, com os dados reais do save. Os modelos 3D deles ainda não aparecem aqui.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DrawLinkCustomization() {
    if (!sDraftLoaded) {
        LoadDraftFromGame();
    }
    ImGui::TextWrapped("%s", "Só a aparência muda: as cores são as do Editor de personalização do Ship of Harkinian. "
                             "Atributos, dano, física e animações não são tocados.");

    ImGui::BeginGroup();
    DrawPreview();
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    for (int i = 0; i < kPartCount; i++) {
        ImGui::PushID(i);
        float rgb[3] = { sDraft[i].r / 255.0f, sDraft[i].g / 255.0f, sDraft[i].b / 255.0f };
        if (ImGui::ColorEdit3(kParts[i].label, rgb, ImGuiColorEditFlags_NoInputs)) {
            sDraft[i] = Color_RGBA8{ static_cast<uint8_t>(rgb[0] * 255.0f), static_cast<uint8_t>(rgb[1] * 255.0f),
                                     static_cast<uint8_t>(rgb[2] * 255.0f), 255 };
            sDraftChanged = true;
        }
        ImGui::PopID();
    }
    ImGui::EndGroup();

    ImGui::SeparatorText("Visuais prontos");
    for (int l = 0; l < static_cast<int>(ARRAY_COUNT(kLooks)); l++) {
        const Look& look = kLooks[l];
        const bool unlocked = look.achievement == nullptr || AchievementUnlocked(look.achievement);
        ImGui::PushID(l);
        if (l > 0) {
            ImGui::SameLine();
        }
        ImGui::BeginDisabled(!unlocked);
        if (UIWidgets::Button(unlocked ? look.name : "Bloqueado", UIWidgets::ButtonOptions().Size(ImVec2(150.0f, 0.0f)))) {
            std::copy(look.colors, look.colors + kPartCount, sDraft);
            sDraftChanged = true;
        }
        ImGui::EndDisabled();
        if (!unlocked && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Conquista: %s", look.unlockText);
        }
        ImGui::PopID();
    }

    ImGui::Spacing();
    if (sDraftChanged) {
        ImGui::TextColored(kGold, "%s", "Prévia ainda não aplicada ao jogo.");
    }
    if (UIWidgets::Button("Aplicar", UIWidgets::ButtonOptions().Size(ImVec2(150.0f, 0.0f)))) {
        ApplyDraft();
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Descartar prévia", UIWidgets::ButtonOptions().Size(ImVec2(180.0f, 0.0f)))) {
        LoadDraftFromGame();
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Restaurar originais", UIWidgets::ButtonOptions().Size(ImVec2(200.0f, 0.0f)))) {
        ResetToOriginal();
    }

    ImGui::SeparatorText("Visuais guardados");
    for (int slot = 0; slot < 3; slot++) {
        ImGui::PushID(100 + slot);
        const std::string saved = Config().GetString(kSlotKeys[slot]);
        ImGui::Text("Visual %d: %s", slot + 1, saved.empty() ? "vazio" : "guardado");
        ImGui::SameLine();
        if (UIWidgets::Button("Guardar a prévia", UIWidgets::ButtonOptions().Size(ImVec2(170.0f, 0.0f)))) {
            Config().Set(kSlotKeys[slot], EncodeDraft());
            SaveConfig();
        }
        if (!saved.empty()) {
            ImGui::SameLine();
            if (UIWidgets::Button("Carregar na prévia", UIWidgets::ButtonOptions().Size(ImVec2(180.0f, 0.0f)))) {
                sDraftChanged = DecodeDraft(saved) || sDraftChanged;
            }
        }
        ImGui::PopID();
    }
}

} // namespace DefinitiveExpansion
