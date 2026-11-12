// OOT D.E. Definitive Expansion, phase 2: Touch & Motion and HUD profiles.
//
// Nothing here is a second input system:
// - Touch: the on-screen controls are the Android overlay that already feeds the virtual controller. This file only
//   asks the app to open its layout editor (TouchLayoutEditor.java).
// - Motion: the gyro values come from libultraship's gyro mapping (the phone sensor on Android) and are consumed in
//   z_player.c exactly where they always were. DefinitiveExpansion_FilterGyro sits in between and applies the
//   player's choices (which items use it, axes, inversion, smoothing).
// - HUD: positions and visibility are the Ship of Harkinian HUD CVars (Cosmetics Editor). The profiles write those
//   same CVars, so the game's HUD code is untouched.
#include "DefinitiveExpansion.h"

#include <imgui.h>
#include <libultraship/libultraship.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "controller/controldevice/controller/mapping/sdl/SDLGyroMapping.h"
#include "soh/OTRGlobals.h"
#include "soh/cvar_prefixes.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"

#ifdef __ANDROID__
#include <SDL2/SDL.h>
#include <jni.h>
#endif

extern "C" {
#include "z64.h"
#include "macros.h"
extern PlayState* gPlayState;
}

using nlohmann::json;

namespace DefinitiveExpansion {

// ---------------------------------------------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------------------------------------------

static const char* kMotionSmoothing = "motion.smoothing";       // 0..90 (%)
static const char* kMotionHorizontal = "motion.horizontal";     // use the left/right axis
static const char* kMotionVertical = "motion.vertical";         // use the up/down axis
static const char* kMotionInvertH = "motion.invertHorizontal";
static const char* kMotionInvertV = "motion.invertVertical";
static const char* kMotionSlingshot = "motion.items.slingshot";
static const char* kMotionBow = "motion.items.bow";
static const char* kMotionHookshot = "motion.items.hookshot";
static const char* kMotionBoomerang = "motion.items.boomerang";
static const char* kMotionLook = "motion.items.firstPersonView";
static const char* kHudCustom = "hud.custom"; // the player's own HUD layout (name -> value of each HUD CVar)

void RegisterControlSettings(DE::Config& config) {
    config.RegisterSetting(kMotionSmoothing, 30, "Suavização da mira por movimento (0 a 90%).");
    config.RegisterSetting(kMotionHorizontal, true, "Usa o eixo horizontal do giroscópio.");
    config.RegisterSetting(kMotionVertical, true, "Usa o eixo vertical do giroscópio.");
    config.RegisterSetting(kMotionInvertH, false, "Inverte o eixo horizontal do giroscópio.");
    config.RegisterSetting(kMotionInvertV, false, "Inverte o eixo vertical do giroscópio.");
    config.RegisterSetting(kMotionSlingshot, true, "Mira por movimento com o estilingue.");
    config.RegisterSetting(kMotionBow, true, "Mira por movimento com o arco.");
    config.RegisterSetting(kMotionHookshot, true, "Mira por movimento com o gancho.");
    config.RegisterSetting(kMotionBoomerang, true, "Mira por movimento com o bumerangue.");
    config.RegisterSetting(kMotionLook, true, "Mira por movimento ao olhar em primeira pessoa.");
    config.RegisterSetting(kHudCustom, json::object(), "Layout de HUD personalizado do jogador.");
}

// ---------------------------------------------------------------------------------------------------------------
// Motion (gyro) filter
// ---------------------------------------------------------------------------------------------------------------

// Read once per frame at most: the config is behind a mutex and this runs every frame of first person aiming.
struct MotionSettings {
    bool horizontal, vertical, invertH, invertV;
    bool slingshot, bow, hookshot, boomerang, look;
    float smoothing;
};
static MotionSettings sMotion;
static bool sMotionLoaded = false;
static float sSmoothX = 0.0f, sSmoothY = 0.0f;

static void LoadMotionSettings() {
    DE::Config& config = Config();
    sMotion.horizontal = config.GetBool(kMotionHorizontal);
    sMotion.vertical = config.GetBool(kMotionVertical);
    sMotion.invertH = config.GetBool(kMotionInvertH);
    sMotion.invertV = config.GetBool(kMotionInvertV);
    sMotion.slingshot = config.GetBool(kMotionSlingshot);
    sMotion.bow = config.GetBool(kMotionBow);
    sMotion.hookshot = config.GetBool(kMotionHookshot);
    sMotion.boomerang = config.GetBool(kMotionBoomerang);
    sMotion.look = config.GetBool(kMotionLook);
    sMotion.smoothing = std::clamp(config.GetInt(kMotionSmoothing), 0, 90) / 100.0f;
    sMotionLoaded = true;
}

static bool MotionAllowedFor(Player* player, bool withoutWeapon) {
    if (withoutWeapon) {
        return sMotion.look;
    }
    switch (player->heldItemAction) {
        case PLAYER_IA_SLINGSHOT:
            return sMotion.slingshot;
        case PLAYER_IA_HOOKSHOT:
        case PLAYER_IA_LONGSHOT:
            return sMotion.hookshot;
        case PLAYER_IA_BOOMERANG:
            return sMotion.boomerang;
        default:
            // Every bow action (plain, fire, ice, light and the unused ones) sits between these two.
            if (player->heldItemAction >= PLAYER_IA_BOW && player->heldItemAction < PLAYER_IA_SLINGSHOT) {
                return sMotion.bow;
            }
            return true; // anything else that aims in first person keeps the original behaviour
    }
}

} // namespace DefinitiveExpansion

// Called by z_player.c (first person aiming) with the gyro values of this frame; changes them in place.
// `withoutWeapon` is the game's own distinction between looking around (C-Up) and aiming an item.
extern "C" void DefinitiveExpansion_FilterGyro(Player* player, s32 withoutWeapon, f32* gyroX, f32* gyroY) {
    using namespace DefinitiveExpansion;
    if (!sMotionLoaded) {
        LoadMotionSettings();
    }
    if (player == nullptr || !MotionAllowedFor(player, withoutWeapon != 0)) {
        *gyroX = 0.0f;
        *gyroY = 0.0f;
        sSmoothX = sSmoothY = 0.0f;
        return;
    }
    // gyroX moves the aim up/down, gyroY left/right (see func_8084ABD8).
    float x = sMotion.vertical ? *gyroX : 0.0f;
    float y = sMotion.horizontal ? *gyroY : 0.0f;
    if (sMotion.invertV) {
        x = -x;
    }
    if (sMotion.invertH) {
        y = -y;
    }
    // Exponential smoothing: each frame keeps `smoothing` of the previous value. 0 = raw sensor.
    sSmoothX = sSmoothX * sMotion.smoothing + x * (1.0f - sMotion.smoothing);
    sSmoothY = sSmoothY * sMotion.smoothing + y * (1.0f - sMotion.smoothing);
    *gyroX = sSmoothX;
    *gyroY = sSmoothY;
}

namespace DefinitiveExpansion {

// ---------------------------------------------------------------------------------------------------------------
// Menu: Touch & Motion
// ---------------------------------------------------------------------------------------------------------------

static const ImVec4 kGold(1.0f, 0.82f, 0.30f, 1.0f);

static std::shared_ptr<Ship::ControllerGyro> Gyro() {
    return Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(0)->GetGyro();
}

static bool ConfigCheckbox(const char* label, const char* key, const char* tooltip = "") {
    bool value = Config().GetBool(key);
    if (UIWidgets::Checkbox(label, &value, UIWidgets::CheckboxOptions().Tooltip(tooltip))) {
        Config().Set(key, value);
        return true;
    }
    return false;
}

#ifdef __ANDROID__
static void OpenTouchEditor() {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (env == nullptr || activity == nullptr) {
        return;
    }
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(activityClass, "openTouchLayoutEditor", "()V");
    if (method != nullptr) {
        env->CallVoidMethod(activity, method);
    } else {
        env->ExceptionClear();
    }
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(activity);
}
#endif

void DrawTouchAndMotion() {
    ImGui::SeparatorText("Controles de toque");
#ifdef __ANDROID__
    ImGui::TextWrapped("%s", "Mova cada controle arrastando, mude o tamanho de cada um, a distância entre os botões "
                             "e a transparência. São 3 perfis de layout; \"Padrão\" volta ao layout original.");
    if (UIWidgets::Button("Editar controles de toque", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))) {
        OpenTouchEditor();
    }
    ImGui::TextWrapped("%s", "Controles Bluetooth e USB continuam funcionando como antes, junto com os de toque.");
#else
    ImGui::TextWrapped("%s", "Os controles de toque só existem na versão Android.");
#endif

    ImGui::SeparatorText("Mira por movimento (giroscópio)");
    ImGui::TextWrapped("%s", "O giroscópio soma à mira do analógico, sem substituir. Vale para a mira em primeira "
                             "pessoa.");
    auto gyro = Gyro();
    auto mapping = gyro->GetGyroMapping();
    bool enabled = mapping != nullptr;
    if (UIWidgets::Checkbox("Ativar mira por movimento", &enabled)) {
        if (enabled) {
            auto created = std::make_shared<Ship::SDLGyroMapping>(0, 1.0f, 0.0f, 0.0f, 0.0f);
            created->Recalibrate();
            gyro->SetGyroMapping(created);
            created->SaveToConfig();
            gyro->SaveGyroMappingIdToConfig();
        } else {
            gyro->ClearGyroMapping();
        }
        mapping = gyro->GetGyroMapping();
    }

    ImGui::BeginDisabled(mapping == nullptr);
    int32_t sensitivity = mapping != nullptr ? mapping->GetSensitivityPercent() : 100;
    if (UIWidgets::SliderInt("Sensibilidade: %d%%", &sensitivity,
                             UIWidgets::IntSliderOptions().Min(1).Max(100).DefaultValue(100)) &&
        mapping != nullptr) {
        mapping->SetSensitivity(static_cast<uint8_t>(std::clamp(sensitivity, 1, 100)));
        mapping->SaveToConfig();
    }
    if (UIWidgets::Button("Calibrar agora", UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline).Tooltip(
                                                "Deixe o aparelho parado na posição em que você joga e toque aqui.")) &&
        mapping != nullptr) {
        mapping->Recalibrate();
        mapping->SaveToConfig();
    }

    bool changed = false;
    int smoothing = Config().GetInt(kMotionSmoothing);
    if (UIWidgets::SliderInt("Suavização: %d%%", &smoothing,
                             UIWidgets::IntSliderOptions().Min(0).Max(90).DefaultValue(30))) {
        changed |= Config().Set(kMotionSmoothing, smoothing);
    }
    changed |= ConfigCheckbox("Eixo horizontal", kMotionHorizontal);
    changed |= ConfigCheckbox("Inverter horizontal", kMotionInvertH);
    changed |= ConfigCheckbox("Eixo vertical", kMotionVertical);
    changed |= ConfigCheckbox("Inverter vertical", kMotionInvertV);

    ImGui::SeparatorText("Onde usar a mira por movimento");
    changed |= ConfigCheckbox("Estilingue", kMotionSlingshot);
    changed |= ConfigCheckbox("Arco", kMotionBow);
    changed |= ConfigCheckbox("Gancho e Gancho Longo", kMotionHookshot);
    changed |= ConfigCheckbox("Bumerangue", kMotionBoomerang);
    changed |= ConfigCheckbox("Olhar em primeira pessoa", kMotionLook);
    ImGui::EndDisabled();

    if (changed) {
        SaveConfig();
        LoadMotionSettings();
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Menu: HUD
// ---------------------------------------------------------------------------------------------------------------

struct HudElement {
    const char* cvar;  // CVar prefix used by the game's HUD code (Cosmetics Editor)
    const char* label; // pt-BR
    bool anchoredByDefault;
};

// Same elements as the Cosmetics Editor's HUD list (MarginCvarList / MarginCvarNonAnchor).
static const HudElement kHudElements[] = {
    { CVAR_COSMETIC("HUD.Hearts"), "Corações (margens)", true },
    { CVAR_COSMETIC("HUD.HeartsCount"), "Corações", true },
    { CVAR_COSMETIC("HUD.MagicBar"), "Barra de magia", true },
    { CVAR_COSMETIC("HUD.VisualSoA"), "Pedra da Agonia visual", true },
    { CVAR_COSMETIC("HUD.BButton"), "Botão B", true },
    { CVAR_COSMETIC("HUD.AButton"), "Botão A", true },
    { CVAR_COSMETIC("HUD.StartButton"), "Botão Start", true },
    { CVAR_COSMETIC("HUD.CUpButton"), "C para cima", true },
    { CVAR_COSMETIC("HUD.CDownButton"), "C para baixo", true },
    { CVAR_COSMETIC("HUD.CLeftButton"), "C esquerda", true },
    { CVAR_COSMETIC("HUD.CRightButton"), "C direita", true },
    { CVAR_COSMETIC("HUD.Dpad"), "Itens do direcional", true },
    { CVAR_COSMETIC("HUD.Minimap"), "Minimapa", true },
    { CVAR_COSMETIC("HUD.SmallKey"), "Chaves", true },
    { CVAR_COSMETIC("HUD.Rupees"), "Rupees", true },
    { CVAR_COSMETIC("HUD.Carrots"), "Cenouras da Epona", false },
    { CVAR_COSMETIC("HUD.Timers"), "Cronômetros", false },
    { CVAR_COSMETIC("HUD.ArcheryScore"), "Pontos do tiro ao alvo", false },
    { CVAR_COSMETIC("HUD.TitleCard.Map"), "Nome do local", false },
    { CVAR_COSMETIC("HUD.TitleCard.Boss"), "Nome do chefe", false },
    { CVAR_COSMETIC("HUD.IGT"), "Tempo de jogo", false },
};

static const char* kHudSuffixes[] = { ".PosType", ".PosX", ".PosY", ".UseMargins" };

static void SaveCVars() {
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

static void HudClassic() {
    for (const HudElement& element : kHudElements) {
        for (const char* suffix : kHudSuffixes) {
            CVarClear((std::string(element.cvar) + suffix).c_str());
        }
    }
}

static void HudMinimal() {
    HudClassic();
    for (const char* hidden : { CVAR_COSMETIC("HUD.Minimap"), CVAR_COSMETIC("HUD.StartButton"),
                                CVAR_COSMETIC("HUD.Dpad"), CVAR_COSMETIC("HUD.IGT") }) {
        CVarSetInteger((std::string(hidden) + ".PosType").c_str(), HIDDEN);
    }
}

// "Definitive": original positions, with every edge-anchored element following the screen margins, which is what
// keeps the HUD at the screen edges on wide phone screens (same rule as the Cosmetics Editor's "all margins on").
static void HudDefinitive() {
    HudClassic();
    for (const HudElement& element : kHudElements) {
        if (element.anchoredByDefault) {
            CVarSetInteger((std::string(element.cvar) + ".UseMargins").c_str(), 1);
        }
    }
}

static json HudSnapshot() {
    json snapshot = json::object();
    for (const HudElement& element : kHudElements) {
        for (const char* suffix : kHudSuffixes) {
            const std::string name = std::string(element.cvar) + suffix;
            const int value = CVarGetInteger(name.c_str(), 0);
            if (value != 0) {
                snapshot[name] = value;
            }
        }
    }
    return snapshot;
}

static void HudApplySnapshot(const json& snapshot) {
    HudClassic();
    if (!snapshot.is_object()) {
        return;
    }
    for (const HudElement& element : kHudElements) {
        for (const char* suffix : kHudSuffixes) {
            const std::string name = std::string(element.cvar) + suffix;
            // Only the HUD CVars listed above are ever written, whatever the settings file contains.
            if (snapshot.contains(name) && snapshot[name].is_number_integer()) {
                CVarSetInteger(name.c_str(), snapshot[name].get<int>());
            }
        }
    }
}

void DrawHudEditor() {
    ImGui::TextWrapped("%s", "Posição e visibilidade de cada elemento do HUD. Usa as mesmas opções do Editor de "
                             "personalização; a lógica dos indicadores do jogo não muda.");

    ImGui::SeparatorText("Perfis");
    const UIWidgets::ButtonOptions inlineButton = UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline);
    if (UIWidgets::Button("Clássico", inlineButton)) {
        HudClassic();
        SaveCVars();
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Minimalista", inlineButton)) {
        HudMinimal();
        SaveCVars();
    }
    ImGui::SameLine();
    if (UIWidgets::Button("Definitive", inlineButton)) {
        HudDefinitive();
        SaveCVars();
    }
    const json custom = Config().Get(kHudCustom);
    ImGui::SameLine();
    ImGui::BeginDisabled(!custom.is_object() || custom.empty());
    if (UIWidgets::Button("Personalizado", inlineButton)) {
        HudApplySnapshot(custom);
        SaveCVars();
    }
    ImGui::EndDisabled();
    if (UIWidgets::Button("Salvar o HUD atual como Personalizado", inlineButton)) {
        Config().Set(kHudCustom, HudSnapshot());
        SaveConfig();
    }
    ImGui::TextWrapped("%s", "Clássico: posições originais. Minimalista: sem minimapa, botão Start, itens do "
                             "direcional e tempo de jogo. Definitive: elementos acompanham as margens da tela.");

    ImGui::SeparatorText("Elementos");
    static const char* kPositions[] = { "Posição original", "Preso à esquerda", "Preso à direita", "Livre",
                                        "Escondido" };
    for (const HudElement& element : kHudElements) {
        ImGui::PushID(element.cvar);
        const std::string posTypeName = std::string(element.cvar) + ".PosType";
        int posType = std::clamp(CVarGetInteger(posTypeName.c_str(), 0), 0, 4);
        if (ImGui::TreeNode(element.label)) {
            bool changed = false;
            bool visible = posType != HIDDEN;
            if (UIWidgets::Checkbox("Visível", &visible)) {
                posType = visible ? ORIGINAL_LOCATION : HIDDEN;
                CVarSetInteger(posTypeName.c_str(), posType);
                changed = true;
            }
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
            if (ImGui::Combo("Posição", &posType, kPositions, 5)) {
                CVarSetInteger(posTypeName.c_str(), posType);
                changed = true;
            }
            if (posType != ORIGINAL_LOCATION && posType != HIDDEN) {
                int x = CVarGetInteger((std::string(element.cvar) + ".PosX").c_str(), 0);
                int y = CVarGetInteger((std::string(element.cvar) + ".PosY").c_str(), 0);
                if (UIWidgets::SliderInt("Esquerda <-> Direita: %d", &x,
                                         UIWidgets::IntSliderOptions().Min(-320).Max(320).DefaultValue(0))) {
                    CVarSetInteger((std::string(element.cvar) + ".PosX").c_str(), x);
                    changed = true;
                }
                if (UIWidgets::SliderInt("Cima <-> Baixo: %d", &y,
                                         UIWidgets::IntSliderOptions().Min(-240).Max(240).DefaultValue(0))) {
                    CVarSetInteger((std::string(element.cvar) + ".PosY").c_str(), y);
                    changed = true;
                }
            }
            bool margins = CVarGetInteger((std::string(element.cvar) + ".UseMargins").c_str(), 0) != 0;
            if (UIWidgets::Checkbox("Acompanhar as margens da tela", &margins)) {
                CVarSetInteger((std::string(element.cvar) + ".UseMargins").c_str(), margins);
                changed = true;
            }
            if (changed) {
                SaveCVars();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::TextColored(kGold, "%s", "Tamanho e transparência por elemento ainda não existem: o jogo não tem essas "
                                    "opções no HUD original.");
}

} // namespace DefinitiveExpansion
