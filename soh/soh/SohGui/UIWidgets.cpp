#include "UIWidgets.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <sstream>
#include <libultraship/libultraship.h>
#include <string>
#include <random>
#include <math.h>
#include <unordered_map>
#include <libultraship/libultra/types.h>
#include <spdlog/fmt/fmt.h>

#include "MenuTranslations.h"

namespace UIWidgets {

std::string TranslateText(const std::string& source) {
    static const std::unordered_map<std::string, std::string> translations = {
        {"Small", "Pequeno"}, {"Normal", "Normal"}, {"Large", "Grande"}, {"X-Large", "Extra grande"},
        {"Vanilla", "Original"}, {"Faster Run", "Corrida mais rápida"}, {"Faster + Longer Jump", "Corrida mais rápida + salto mais longo"},
        {"Presets", "Predefinições"}, {"Saving", "Salvamento"}, {"Autosave", "Salvamento automático"},
        {"Remember Save Location", "Lembrar local do salvamento"}, {"Containers Match Contents", "Baús correspondem ao conteúdo"},
        {"Chest Size & Texture Matches Contents", "Tamanho e textura dos baús correspondem ao conteúdo"},
        {"Chests of Agony", "Baús da Agonia"}, {"Time of Day", "Período do dia"}, {"Nighttime GS Always Spawn", "Skulltulas douradas noturnas sempre aparecem"},
        {"Pull Grave During the Day", "Abrir túmulos durante o dia"}, {"Dampe Appears All Night", "Dampé aparece a noite toda"},
        {"Exit Market at Night", "Sair do Mercado à noite"}, {"Shops and Games Always Open", "Lojas e minijogos sempre abertos"},
        {"Pause Menu", "Menu de pausa"}, {"Allow the Cursor to be on Any Slot", "Permitir cursor em qualquer espaço"},
        {"Pause Warp", "Teletransporte pelo menu de pausa"}, {"Answer Navi Prompt with L Button", "Responder à Navi com o botão L"},
        {"Don't Require Input for Credits Sequence", "Não exigir comandos durante os créditos"},
        {"Include Held Inputs at the Start of Pause Buffer Input Window", "Incluir comandos pressionados no início da janela de buffer"},
        {"Item Count Messages", "Mensagens de quantidade de itens"}, {"Gold Skulltula Tokens", "Símbolos de Skulltula dourada"},
        {"Pieces of Heart", "Pedaços de coração"}, {"Heart Containers", "Recipientes de coração"},
        {"Disable Crit Wiggle", "Desativar tremor de câmera com pouca vida"}, {"Better Owl", "Coruja aprimorada"},
        {"Convenience", "Conveniência"}, {"Quit Fishing at Door", "Sair da pescaria na porta"},
        {"Instant Putaway", "Guardar item instantaneamente"}, {"Navi Timer Resets on Scene Change", "Reiniciar temporizador da Navi ao mudar de cenário"},
        {"Link's Cow in Both Time Periods", "Vaca de Link nos dois períodos"}, {"Cutscenes", "Cenas"},
        {"All##Skips", "Todos##Skips"}, {"None##Skips", "Nenhum##Skips"}, {"Skip Intro", "Pular introdução"},
        {"Skip Entrance Cutscenes", "Pular cenas de entrada"}, {"Skip Story Cutscenes", "Pular cenas da história"},
        {"Skip Song Cutscenes", "Pular cenas de aprendizado de músicas"}, {"Skip Boss Introductions", "Pular apresentações dos chefes"},
        {"Quick Boss Deaths", "Morte rápida dos chefes"}, {"Skip Owl Interactions", "Pular interações com a coruja"},
        {"Skip Misc Interactions", "Pular interações diversas"}, {"Disable Title Card", "Desativar cartão de título"},
        {"Exclude Glitch-Aiding Cutscenes", "Manter cenas úteis para glitches"}, {"Text", "Texto"},
        {"Skip Pickup Messages", "Pular mensagens de coleta"}, {"Skip Forced Dialog", "Pular diálogos obrigatórios"},
        {"Skip Text", "Pular texto"}, {"Animations", "Animações"}, {"Faster Heavy Block Lift", "Levantar blocos pesados mais rápido"},
        {"Fast Chests", "Abrir baús rapidamente"}, {"Skip Water Take Breath Animation", "Pular animação de respirar ao sair da água"},
        {"Misc", "Diversos"}, {"Skip Child Stealth", "Pular furtividade como criança"}, {"Skip Tower Escape", "Pular fuga da torre"},
        {"Skip Scarecrow's Song", "Pular música do Espantalho"}, {"Faster Rupee Accumulator", "Acumular rupias mais rápido"},
        {"No Skulltula Freeze", "Não congelar ao coletar Skulltulas"}, {"Skip Save Confirmation", "Pular confirmação de salvamento"},
        {"Link as Default File Name", "Usar Link como nome padrão do arquivo"}, {"Mods", "Modificações"},
        {"Use Alternate Assets", "Usar recursos alternativos"}, {"Disable Bomb Billboarding", "Desativar rotação dos explosivos para a câmera"},
        {"Disable Grotto Fixed Rotation", "Desativar rotação fixa das grutas"}, {"Models & Textures", "Modelos e texturas"},
        {"Disable LOD", "Desativar nível de detalhe (LOD)"}, {"Enemy Health Bars", "Barras de vida dos inimigos"},
        {"Enable 3D Dropped Items/Projectiles", "Ativar itens e projéteis 3D"}, {"Animated Link in Pause Menu", "Link animado no menu de pausa"},
        {"Show Age-Dependent Equipment", "Mostrar equipamentos independentemente da idade"},
        {"Scale Adult Equipment as Child", "Ajustar equipamentos de adulto para Link criança"},
        {"Show Gauntlets in First Person", "Mostrar manoplas em primeira pessoa"},
        {"Show Chains on Both Sides of Locked Doors", "Mostrar correntes nos dois lados das portas trancadas"},
        {"Color Temple of Time's Medallions", "Colorir medalhões no Templo do Tempo"}, {"UI", "Interface"},
        {"Minimal UI", "Interface minimalista"}, {"Disable Hot/Underwater Warning Text", "Desativar avisos de calor e submersão"},
        {"Remember Minimap State Between Areas", "Lembrar estado do minimapa entre áreas"},
        {"Visual Stone of Agony", "Pedra da Agonia visual"}, {"Disable HUD Heart Animations", "Desativar animações dos corações na interface"},
        {"Glitch Line-up Tick", "Marcador de alinhamento para glitches"}, {"Disable Black Bar Letterboxes", "Desativar barras pretas das cenas"},
        {"Dynamic Wallet Icon", "Ícone dinâmico da carteira"}, {"Always Show Dungeon Entrances", "Sempre mostrar entradas de masmorras"},
        {"More Info in File Select", "Mais informações na seleção de arquivo"},
        {"Better Ammo Rendering in Pause Menu", "Exibição aprimorada de munição no menu de pausa"},
        {"Enable Passage of Time on File Select", "Ativar passagem do tempo na seleção de arquivo"},
        {"N64 Mode", "Modo N64"}, {"Remove Spin Attack Darkness", "Remover escurecimento do ataque giratório"},
        {"Draw Distance", "Distância de renderização"}, {"Kokiri Draw Distance", "Distância de renderização dos Kokiri"},
        {"Widescreen Actor Culling", "Recorte de atores em tela widescreen"}, {"Cull Glitch Useful Actors", "Recortar atores úteis para glitches"},
        {"Equipment", "Equipamentos"}, {"Equip Items on Dpad", "Equipar itens pelo direcional"},
        {"Assignable Tunics and Boots", "Atribuir túnicas e botas"}, {"Equipment Toggle", "Alternar equipamentos"},
        {"Allow Strength Equipment to be Toggled", "Permitir alternar aprimoramentos de força"},
        {"Sword Toggle Options", "Opções para guardar espadas"}, {"Ask to Equip New Items", "Perguntar ao equipar novos itens"},
        {"Ocarina", "Ocarina"}, {"Prevent Dropped Ocarina Inputs", "Evitar perda de comandos na Ocarina"},
        {"Fast Ocarina Playback", "Reprodução rápida da Ocarina"}, {"Time Travel with Song of Time", "Viajar no tempo com a Canção do Tempo"},
        {"Masks", "Máscaras"}, {"Bunny Hood Effect", "Efeito do Capuz de Coelho"},
        {"Masks Equippable as Adult", "Permitir máscaras como adulto"}, {"Persistent Masks", "Máscaras persistentes"},
        {"Invisible Bunny Hood", "Capuz de Coelho invisível"}, {"Mask Select in Inventory", "Selecionar máscara no inventário"},
        {"Explosives", "Explosivos"}, {"Deku Nuts Explode Bombs", "Nozes Deku detonam bombas"},
        {"Remove Explosive Limit", "Remover limite de explosivos"}, {"Static Explosion Radius", "Raio de explosão fixo"},
        {"Prevent Bombchus Forcing First-Person", "Impedir Bombchus de forçar primeira pessoa"},
        {"Better Bombchu Shopping", "Compra aprimorada de Bombchus"}, {"Bow / Slingshot", "Arco e estilingue"},
        {"Equip Multiple Arrows at Once", "Equipar vários tipos de flecha simultaneamente"},
        {"Skip Magic Arrow Equip Animation", "Pular animação ao equipar flechas mágicas"},
        {"Blue Fire Arrows", "Flechas de fogo azul"}, {"Sunlight Arrows", "Flechas solares"},
        {"Bow as Child/Slingshot as Adult", "Arco como criança e estilingue como adulto"},
        {"Aiming Reticle for the Bow/Slingshot", "Mira para arco e estilingue"}, {"Hookshot", "Gancho"},
        {"Targetable Hookshot Reticle", "Mira indicativa de alvos do gancho"}, {"Boomerang", "Bumerangue"},
        {"Instant Boomerang Recall", "Retorno instantâneo do bumerangue"},
        {"Aim Boomerang in First-Person Mode", "Mirar bumerangue em primeira pessoa"},
        {"Aiming Reticle for Boomerang", "Mira para o bumerangue"}, {"Magic Spells", "Magias"},
        {"Better Farore's Wind", "Vento de Farore aprimorado"}, {"Faster Farore's Wind", "Vento de Farore mais rápido"},
        {"Gameplay Fixes", "Correções de jogabilidade"}, {"Item-related Fixes", "Correções relacionadas a itens"},
        {"Camera Fixes", "Correções de câmera"}, {"Graphical Fixes", "Correções gráficas"},
        {"Audio Fixes", "Correções de áudio"}, {"Desync Fixes", "Correções de dessincronização"},
        {"Graphical Restorations", "Restaurações gráficas"}, {"Glitch Restorations", "Restaurações de glitches"},
        {"Misc Restorations", "Restaurações diversas"}, {"Health", "Vida"},
        {"Permanent Heart Loss", "Perda permanente de corações"}, {"Damage Multiplier", "Multiplicador de dano"},
        {"Fall Damage Multiplier", "Multiplicador de dano de queda"}, {"Void Damage Multiplier", "Multiplicador de dano no vazio"},
        {"Bonk Damage Multiplier", "Multiplicador de dano por impacto"}, {"Spawn with Full Health", "Renascer com vida completa"},
        {"No Heart Drops", "Não deixar corações caírem"}, {"Drops", "Itens derrubados"},
        {"No Random Drops", "Desativar itens aleatórios"}, {"Enable Bombchu Drops", "Ativar queda de Bombchus"},
        {"Trees Drop Sticks", "Árvores derrubam gravetos"}, {"Miscellaneous", "Diversos"},
        {"Delete File on Death", "Excluir arquivo ao morrer"}, {"Always Win Goron Pot", "Sempre vencer o vaso Goron"},
        {"Always Win Dampe Digging Game", "Sempre vencer o jogo de escavação de Dampé"},
        {"All Dogs are Richard", "Todos os cães são Richard"}, {"Enemies", "Inimigos"},
        {"Hyper Bosses", "Chefes acelerados"}, {"Hyper Enemies", "Inimigos acelerados"},
        {"Enable Visual Guard Vision", "Exibir campo de visão dos guardas"}, {"Minigames", "Minijogos"},
        {"Shooting Gallery", "Galeria de tiro"}, {"Customize Behavior##Shooting", "Personalizar comportamento##Shooting"},
        {"Instant Win", "Vitória instantânea"}, {"No Rupee Randomization", "Não aleatorizar rupias"},
        {"Bombchu Bowling", "Boliche de Bombchu"}, {"Remove Small Cucco", "Remover Cucco pequeno"},
        {"Remove Big Cucco", "Remover Cucco grande"}, {"Frogs' Ocarina Game", "Jogo da Ocarina dos sapos"},
        {"Lost Woods Ocarina Game", "Jogo da Ocarina nos Bosques Perdidos"}, {"Fishing", "Pescaria"},
        {"Instant Fishing", "Pesca instantânea"}, {"Guarantee Bite", "Garantir mordida"},
        {"Fish never Escape", "Peixes nunca escapam"}, {"Loaches always Appear", "Sempre fazer loaches aparecerem"},
        {"Skip Keep Confirmation", "Pular confirmação para ficar com o peixe"},
        {"All Fish are Hyrule Loaches", "Todos os peixes são loaches de Hyrule"},
        {"Extra Modes", "Modos extras"}, {"Mirrored World", "Mundo espelhado"},
        {"Ivan the Fairy (Coop Mode)", "Ivan, a fada (modo cooperativo)"},
        {"Dogs Follow You Everywhere", "Cães seguem você por toda parte"}, {"Rupee Dash Mode", "Modo corrida das rupias"},
        {"Shadow Tag Mode", "Modo perseguição das sombras"}, {"Hurt Container Mode", "Modo recipientes de dano"},
        {"Additional Traps", "Armadilhas adicionais"}, {"Trap Options", "Opções de armadilhas"},
        {"Freeze Traps", "Armadilhas de congelamento"}, {"Burn Traps", "Armadilhas de queimadura"},
        {"Shock Traps", "Armadilhas elétricas"}, {"Knockback Traps", "Armadilhas de repulsão"},
        {"Speed Traps", "Armadilhas de lentidão"}, {"Bomb Traps", "Armadilhas explosivas"},
        {"Void Traps", "Armadilhas do vazio"}, {"Ammo Traps", "Armadilhas de munição"},
        {"Death Traps", "Armadilhas mortais"}, {"Teleport Traps", "Armadilhas de teletransporte"},
        {"Enemy Randomizer", "Aleatorizador de inimigos"}, {"Randomized Enemy Sizes", "Tamanhos aleatórios de inimigos"},
        {"Scale Health with Size", "Ajustar vida conforme o tamanho"}, {"Select all Enemies", "Selecionar todos os inimigos"},
        {"Infinite...", "Infinito..."}, {"Money", "Dinheiro"}, {"Ammo", "Munição"}, {"Magic", "Magia"},
        {"Nayru's Love", "Amor de Nayru"}, {"Epona Boost", "Impulso de Epona"}, {"Timeless Equipment", "Equipamentos sem restrição de idade"},
        {"Unrestricted Items", "Itens sem restrições"}, {"Super Tunic", "Super túnica"},
        {"Fireproof Deku Shield", "Escudo Deku à prova de fogo"}, {"Shield with Two-Handed Weapons", "Escudo com armas de duas mãos"},
        {"Deku Sticks:", "Gravetos Deku:"}, {"Hookshot Everything", "Gancho em qualquer superfície"},
        {"No Clip", "Atravessar paredes"}, {"Climb Everything", "Escalar qualquer superfície"},
        {"Moon Jump on L", "Salto lunar com L"}, {"No ReDead/Gibdo Freeze", "Não congelar com gritos de ReDead/Gibdo"},
        {"Keese/Guay don't Target You", "Keese e Guay não miram em você"}, {"Glitch Aids", "Auxílios para glitches"},
        {"Easy Frame Advancing with Pause", "Avanço de quadros fácil com pausa"}, {"Easy ISG", "ISG fácil"},
        {"Easy QPA", "QPA fácil"}, {"Clear Cutscene Pointer", "Limpar ponteiro de cena"},
        {"Despawn Timers", "Temporizadores de desaparecimento"}, {"Drops Don't Despawn", "Itens derrubados não desaparecem"},
        {"Fish don't Despawn", "Peixes não desaparecem"}, {"Bugs don't Despawn", "Insetos não desaparecem"},
        {"Freeze Time", "Congelar o tempo"}, {"Time Sync", "Sincronizar horário"},
        {"Instant Age Change", "Mudança instantânea de idade"}, {"Change Age", "Mudar idade"},
        {"Save States", "Estados de salvamento"}, {"I promise I have read the warning", "Prometo que li o aviso"},
        {"I understand, enable save states", "Entendi, ativar estados de salvamento"},
        {"Beta Quest", "Missão Beta"}, {"Enable Beta Quest", "Ativar Missão Beta"},
        {"Cosmetics Editor", "Editor de aparência"}, {"Audio Editor", "Editor de áudio"},
        {"Gameplay Stats", "Estatísticas de jogo"}, {"Time Splits", "Tempos parciais"},
        {"Timers", "Cronômetros"}, {"Hide Background", "Ocultar plano de fundo"},
        {"Stats", "Estatísticas"}, {"Console", "Console"}, {"Save Editor", "Editor de salvamento"},
        {"Hook Debugger", "Depurador de ganchos"}, {"Collision Viewer", "Visualizador de colisões"},
        {"Actor Viewer", "Visualizador de atores"}, {"Display List Viewer", "Visualizador de listas de desenho"},
        {"Value Viewer", "Visualizador de valores"}, {"Message Viewer", "Visualizador de mensagens"},
        {"Gfx Debugger", "Depurador gráfico"}, {"Popout Menu", "Abrir menu em janela separada"},
        {"Debug Mode", "Modo de depuração"}, {"OoT Registry Editor", "Editor de registro do OoT"},
        {"Debug Save File Mode", "Modo de salvamento de depuração"}, {"OoT Skulltula Debug", "Depuração de Skulltulas do OoT"},
        {"Better Debug Warp Screen", "Tela de teletransporte de depuração aprimorada"},
        {"Debug Warp Screen Translation", "Tradução da tela de teletransporte de depuração"},
        {"Resource logging", "Registro de recursos"}, {"Frame Advance", "Avanço de quadros"},
        {"Advance 1", "Avançar 1 quadro"}, {"Advance (Hold)", "Avançar (manter pressionado)"},
        {"Popout Stats Window", "Abrir janela de estatísticas"}, {"Popout Console", "Abrir console"},
        {"Popout Save Editor", "Abrir editor de salvamento"}, {"Popout Hook Debugger", "Abrir depurador de ganchos"},
        {"Popout Collision Viewer", "Abrir visualizador de colisões"}, {"Popout Actor Viewer", "Abrir visualizador de atores"},
        {"Popout Display List Viewer", "Abrir visualizador de listas de desenho"},
        {"Popout Value Viewer", "Abrir visualizador de valores"}, {"Popout Message Viewer", "Abrir visualizador de mensagens"},
        {"Popout Gfx Debugger", "Abrir depurador gráfico"}, {"Host & Port", "Servidor e porta"},
        {"Enable##Sail", "Ativar##Sail"}, {"Disable##Sail", "Desativar##Sail"},
        {"Connecting...##Sail", "Conectando...##Sail"}, {"Connected##Sail", "Conectado##Sail"},
        {"Enable##CrowdControl", "Ativar##CrowdControl"}, {"Disable##CrowdControl", "Desativar##CrowdControl"},
        {"Connecting...##CrowdControl", "Conectando...##CrowdControl"}, {"Connected##CrowdControl", "Conectado##CrowdControl"},
        {"Copied to clipboard", "Copiado para a área de transferência"},
        {"Clear", "Limpar"}, {"Search...", "Pesquisar..."}, {"No results found", "Nenhum resultado encontrado"},
        {"Start typing to see results.", "Comece a digitar para ver os resultados."}
    };
    const auto idPosition = source.find("##");
    const std::string visibleText = source.substr(0, idPosition);
    const std::string suffix = idPosition == std::string::npos ? "" : source.substr(idPosition);
    const auto it = translations.find(visibleText);
    if (it != translations.end()) return it->second + suffix;
    if (const std::string* translated = MenuTranslations_Find(visibleText)) return *translated + suffix;
    return source;
}

const char* Tr(const char* text) {
    static std::unordered_map<const char*, std::string> cache;
    auto it = cache.find(text);
    if (it == cache.end()) {
        it = cache.emplace(text, TranslateText(text)).first;
    }
    return it->second.c_str();
}

// Automatically adds newlines to break up text longer than a specified number of characters
// Manually included newlines will still be respected and reset the line length
// If line is midword when it hits the limit, text should break at the last encountered space
std::string WrappedText(const char* text, unsigned int charactersPerLine) {
    std::string newText(text);
    const size_t tipLength = newText.length();
    int lastSpace = -1;
    int currentLineLength = 0;
    for (unsigned int currentCharacter = 0; currentCharacter < tipLength; currentCharacter++) {
        if (newText[currentCharacter] == '\n') {
            currentLineLength = 0;
            lastSpace = -1;
            continue;
        } else if (newText[currentCharacter] == ' ') {
            lastSpace = currentCharacter;
        }

        if ((currentLineLength >= charactersPerLine) && (lastSpace >= 0)) {
            newText[lastSpace] = '\n';
            currentLineLength = currentCharacter - lastSpace - 1;
            lastSpace = -1;
        }
        currentLineLength++;
    }

    return newText;
}

std::string WrappedText(const std::string& text, unsigned int charactersPerLine) {
    return WrappedText(text.c_str(), charactersPerLine);
}

// OOT D.E.: on a phone a label next to a slider leaves no room for the slider.
// When the label takes more than half of the space, move it above the control.
LabelPositions FitLabelPosition(LabelPositions position, const char* label) {
    if (position != LabelPositions::Near && position != LabelPositions::Far) {
        return position;
    }
    const float avail = ImGui::GetContentRegionAvail().x;
    if (ImGui::CalcTextSize(label, NULL, true).x > avail * 0.5f) {
        return LabelPositions::Above;
    }
    return position;
}

void PaddedSeparator(bool padTop, bool padBottom, float extraVerticalTopPadding, float extraVerticalBottomPadding) {
    if (padTop) {
        Spacer(extraVerticalTopPadding);
    }
    ImGui::Separator();
    if (padBottom) {
        Spacer(extraVerticalBottomPadding);
    }
}

void Tooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(text)).c_str());
    }
}

void PushStyleMenu(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.x, color.y, color.z, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ColorValues.at(Colors::DarkGray));
    ImGui::PushStyleColor(ImGuiCol_Border, ColorValues.at(Colors::DarkGray));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 15.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 3.0f);
}

void PushStyleMenu(Colors color) {
    PushStyleMenu(ColorValues.at(color));
}

void PopStyleMenu() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

bool BeginMenu(const char* label, Colors color) {
    bool dirty = false;
    PushStyleMenu(color);
    ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginMenu(label)) {
        dirty = true;
    }
    PopStyleMenu();
    return dirty;
}

void PushStyleMenuItem(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0f, 15.0f));
}

void PushStyleMenuItem(Colors color) {
    PushStyleMenuItem(ColorValues.at(color));
}

void PopStyleMenuItem() {
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(1);
}

bool MenuItem(const char* label, const char* shortcut, Colors color) {
    bool dirty = false;
    PushStyleMenuItem(color);
    if (ImGui::MenuItem(label, shortcut)) {
        dirty = true;
    }
    PopStyleMenuItem();
    return dirty;
}

void PushStyleButton(const ImVec4& color, const ImVec2 padding) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.0f);
}

void PushStyleButton(Colors color, ImVec2 padding) {
    PushStyleButton(ColorValues.at(color), padding);
}

void PopStyleButton() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);
}

void PushStyleInput(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.0f);
}

void PushStyleInput(Colors color) {
    PushStyleInput(ColorValues.at(color));
}

void PopStyleInput() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(7);
}

void PushStyleHeader(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(color.x, color.y, color.z, 0.6f));
}

void PushStyleHeader(Colors color) {
    PushStyleHeader(ColorValues.at(color));
}

void PopStyleHeader() {
    ImGui::PopStyleColor(3);
}

bool Button(const char* label, const ButtonOptions& options) {
    ImGui::BeginDisabled(options.disabled);
    PushStyleButton(options.color, options.padding);
    const std::string translatedLabel = TranslateText(label);
    bool dirty = ImGui::Button(translatedLabel.c_str(), options.size);
    PopStyleButton();
    ImGui::EndDisabled();
    if (options.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !Ship_IsCStringEmpty(options.disabledTooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.disabledTooltip)).c_str());
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }
    return dirty;
}

bool WindowButton(const char* label, const char* cvarName, std::shared_ptr<Ship::GuiWindow> windowPtr,
                  const WindowButtonOptions& options) {
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
    std::string buttonText = label;
    bool dirty = false;
    if (CVarGetInteger(cvarName, 0)) {
        buttonText = ICON_FA_WINDOW_CLOSE " " + buttonText;
    } else {
        buttonText = ICON_FA_EXTERNAL_LINK_SQUARE " " + buttonText;
    }
    if (Button(buttonText.c_str(), { { options.tooltip, options.disabled, options.disabledTooltip },
                                     options.size,
                                     options.padding,
                                     options.color })) {
        windowPtr->ToggleVisibility();
        dirty = true;
    }
    ImGui::PopStyleVar();
    return dirty;
}

void PushStyleCheckbox(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 0.7f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.0f);
}

void PushStyleCheckbox(Colors color) {
    PushStyleCheckbox(ColorValues.at(color));
}

void PopStyleCheckbox() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

void Spacer(float height) {
    ImGui::Dummy(ImVec2(0.0f, height));
}

void Separator(bool padTop, bool padBottom, float extraVerticalTopPadding, float extraVerticalBottomPadding) {
    if (padTop) {
        Spacer(extraVerticalTopPadding);
    }
    ImGui::Separator();
    if (padBottom) {
        Spacer(extraVerticalBottomPadding);
    }
}

// Adds a "?" next to the previous ImGui item with a custom tooltip
void InsertHelpHoverText(const std::string& text) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "?");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", WrappedText(text, 60).c_str());
        ImGui::EndTooltip();
    }
}

void InsertHelpHoverText(const char* text) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "?");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", WrappedText(text, 60).c_str());
        ImGui::EndTooltip();
    }
}

void RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Hide anything after a '##' string
    const char* text_display_end;
    if (hide_text_after_hash) {
        text_display_end = ImGui::FindRenderedTextEnd(text, text_end);
    } else {
        if (!text_end)
            text_end = text + strlen(text); // FIXME-OPT
        text_display_end = text_end;
    }

    if (text != text_display_end) {
        window->DrawList->AddText(g.Font, g.FontSize, pos, ImGui::GetColorU32(ImGuiCol_Text), text, text_display_end);
        if (g.LogEnabled)
            ImGui::LogRenderedText(&pos, text, text_display_end);
    }
}

bool Checkbox(const char* _labelId, bool* value, const CheckboxOptions& options) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGui::BeginDisabled(options.disabled);

    bool above = options.labelPosition == LabelPositions::Above;
    bool lpFar = options.labelPosition == LabelPositions::Far;
    bool right = options.alignment == ComponentAlignments::Right;
    bool none = options.labelPosition == LabelPositions::None;

    // OOT D.E.: the ID keeps the original text, the drawn label is translated.
    std::string idStr = (none ? "##" : "");
    idStr.append(_labelId);
    std::string labelStr = (none ? "##" : "");
    labelStr.append(TranslateText(_labelId));

    const char* label = labelStr.c_str();

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(idStr.c_str());
    const float square_sz = ImGui::GetFrameHeight();
    // OOT D.E.: long labels wrap instead of running off a phone screen.
    const float wrapWidth =
        ImMax(ImGui::GetContentRegionAvail().x - (above ? 0.0f : (style.ItemInnerSpacing.x * 2.0f) + square_sz),
              square_sz * 2.0f);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true, wrapWidth);
    ImVec2 pos = window->DC.CursorPos;

    if (right) {
        float labelOffsetX = (above ? 0 : (style.ItemInnerSpacing.x * 2.0f) + square_sz);
        if (!lpFar) {
            pos.x += ImGui::GetContentRegionAvail().x - (label_size.x + labelOffsetX);
        }
    }
    float bbAboveX = lpFar ? ImGui::GetContentRegionAvail().x
                           : (label_size.x + (above ? 0 : (style.ItemInnerSpacing.x * 2.0f) + square_sz));
    float bbAboveY = label_size.y + (above ? square_sz : 0) + (style.FramePadding.y * 2.0f);
    const ImRect total_bb(pos, pos + ImVec2(bbAboveX, bbAboveY));

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id)) {
        ImGui::EndDisabled();
        return false;
    }
    bool hovered, held, pressed;
    pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        *value = !(*value);
        ImGui::MarkItemEdited(id);
    }
    PushStyleCheckbox(options.color);
    ImVec2 checkPos = pos;
    ImVec2 labelPos = pos;
    if (options.labelPosition == LabelPositions::Above) {
        checkPos.y += label_size.y + (style.ItemInnerSpacing.y * 2.0f);
    } else {
        // Center with checkbox automatically
        labelPos.y += ImGui::CalcTextSize("g").y / 8;
    }
    if (options.alignment == ComponentAlignments::Right) {
        checkPos.x = total_bb.Max.x - square_sz;
    } else {
        float labelFarOffset = ImGui::GetContentRegionAvail().x - label_size.x;
        float labelOffsetX = above ? 0 : (lpFar ? labelFarOffset : (style.ItemInnerSpacing.x * 2.0f) + square_sz);
        labelPos.x += labelOffsetX;
    }
    const ImRect check_bb(checkPos, checkPos + ImVec2(square_sz, square_sz));
    ImGui::RenderNavHighlight(total_bb, id);
    ImGui::RenderFrame(check_bb.Min, check_bb.Max,
                       ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive
                                          : hovered         ? ImGuiCol_FrameBgHovered
                                                            : ImGuiCol_FrameBg),
                       true, style.FrameRounding);
    ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);
    bool mixed_value = (g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0;
    if (mixed_value) {
        // Undocumented tristate/mixed/indeterminate checkbox (#2644)
        // This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all
        // widgets (not just checkbox)
        ImVec2 pad(ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)), ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)));
        window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
    } else if (*value) {
        const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 6.0f));
        ImGui::RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
    }
    {
        const char* labelEnd = ImGui::FindRenderedTextEnd(label);
        if (label != labelEnd) {
            window->DrawList->AddText(g.Font, g.FontSize, labelPos, ImGui::GetColorU32(ImGuiCol_Text), label,
                                      labelEnd, wrapWidth);
        }
    }
    PopStyleCheckbox();
    ImGui::EndDisabled();
    if (options.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !Ship_IsCStringEmpty(options.disabledTooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.disabledTooltip)).c_str());
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }
    return pressed;
}

bool CVarCheckbox(const char* label, const char* cvarName, const CheckboxOptions& options) {
    bool dirty = false;
    bool value = (bool)CVarGetInteger(cvarName, options.defaultValue);
    if (Checkbox(label, &value, options)) {
        CVarSetInteger(cvarName, value);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ShipInit::Init(cvarName);
        dirty = true;
    }
    return dirty;
}

bool StateButton(const char* str_id, const char* label, ImVec2 size, ButtonOptions options, ImGuiButtonFlags flags) {

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const float default_size = ImGui::GetFrameHeight();
    ImGui::ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    if (g.LastItemData.ItemFlags & ImGuiItemFlags_ButtonRepeat) {
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    }

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    if (g.LastItemData.ItemFlags & ImGuiItemFlags_ButtonRepeat) {
        ImGui::PopItemFlag(); // ImGuiItemFlags_ButtonRepeat;
    }
    PushStyleButton(options.color);
    // Render
    const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive
                                            : hovered         ? ImGuiCol_ButtonHovered
                                                              : ImGuiCol_Button);
    // const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
    ImGui::RenderTextClipped(bb.Min + (style.FramePadding * 0.35f), bb.Max - (style.FramePadding / 4), label, NULL,
                             &label_size, style.ButtonTextAlign, &bb);
    PopStyleButton();
    /*ImGui::RenderArrow(window->DrawList,
    bb.Min +
    ImVec2(ImMax(0.0f, (size.x - g.FontSize) * 0.5f), ImMax(0.0f, (size.y - g.FontSize) * 0.5f)),
    text_col, dir);*/

    IMGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
    return pressed;
}

float CalcComboWidth(const char* preview_value, ImGuiComboFlags flags) {
    ImGuiContext& g = *GImGui;

    const ImGuiStyle& style = g.Style;
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) !=
              (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together
    if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | (ImGuiComboFlags)ImGuiComboFlags_CustomPreview)) == 0);

    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : ImGui::GetFrameHeight();
    const float preview_width = ImGui::CalcTextSize(preview_value, NULL, true).x;
    float w = arrow_size + preview_width + (style.FramePadding.x * 2.0f);
    // OOT D.E.: never wider than the space left, so it fits on a phone screen.
    return ImMin(w, ImMax(ImGui::GetContentRegionAvail().x, arrow_size * 3.0f));
}

void PushStyleCombobox(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.x, color.y, color.z, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
}

void PushStyleCombobox(Colors color) {
    PushStyleCombobox(ColorValues.at(color));
}

void PopStyleCombobox() {
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(9);
}

void PushStyleTabs(const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(color.x, color.y, color.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.x, color.y, color.z, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(color.x, color.y, color.z, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
}

void PushStyleTabs(Colors color) {
    PushStyleTabs(ColorValues.at(color));
}

void PopStyleTabs() {
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(4);
}

void PushStyleSlider(Colors color_) {
    const ImVec4& color = ColorValues.at(color_);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(color.x, color.y, color.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0, 1.0, 1.0, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0, 1.0, 1.0, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
}

void PopStyleSlider() {
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(6);
}

bool SliderInt(const char* _labelId, int32_t* value, const IntSliderOptions& _options) {
    // OOT D.E.: translated label, unchanged ID, label above when the screen is narrow.
    const std::string translatedLabelStr = TranslateText(_labelId);
    const char* label = translatedLabelStr.c_str();
    IntSliderOptions options = _options;
    options.labelPosition = FitLabelPosition(options.labelPosition, label);
    bool dirty = false;
    std::string invisibleLabelStr = "##" + std::string(_labelId);
    const char* invisibleLabel = invisibleLabelStr.c_str();
    ImGui::PushID(_labelId);
    ImGui::BeginGroup();
    ImGui::BeginDisabled(options.disabled);
    PushStyleSlider(options.color);
    float width = (options.size == ImVec2(0, 0)) ? ImGui::GetContentRegionAvail().x : options.size.x;
    if (options.labelPosition == LabelPositions::Near || options.labelPosition == LabelPositions::Far) {
        width = width - (ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x);
    }
    ImGui::AlignTextToFramePadding();
    if (options.alignment == ComponentAlignments::Right) {
        ImGui::Text(label, *value);
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::NewLine();
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
        } else if (options.labelPosition == LabelPositions::Near) {
            ImGui::SameLine();
        } else if (options.labelPosition == LabelPositions::Far || options.labelPosition == LabelPositions::None) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
        }
    } else if (options.alignment == ComponentAlignments::Left) {
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::TextWrapped(label, *value);
        }
    }
    if (options.showButtons) {
        if (Button("-", ButtonOptions{ .color = options.color }.Size(Sizes::Inline)) && *value > options.min) {
            *value -= options.step;
            if (options.clamp) {
                if (*value < options.min) {
                    *value = options.min;
                }
            }
            dirty = true;
        }
        ImGui::SameLine(0, 3.0f);
        ImGui::SetNextItemWidth(width - (ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2 + 3) * 2);
    } else {
        ImGui::SetNextItemWidth(width);
    }
    if (ImGui::SliderScalar(invisibleLabel, ImGuiDataType_S32, value, &options.min, &options.max, options.format,
                            options.flags)) {
        if (options.clamp) {
            if (*value < options.min) {
                *value = options.min;
            }
            if (*value > options.max)
                *value = options.max;
        }
        dirty = true;
    }
    if (options.showButtons) {
        ImGui::SameLine(0, 3.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (Button("+", ButtonOptions{ .color = options.color }.Size(Sizes::Inline)) && *value < options.max) {
            *value += options.step;
            if (options.clamp) {
                if (*value > options.max)
                    *value = options.max;
            }
            dirty = true;
        }
    }

    if (options.alignment == ComponentAlignments::Left) {
        if (options.labelPosition == LabelPositions::Near) {
            ImGui::SameLine();
            ImGui::Text(label, *value);
        } else if (options.labelPosition == LabelPositions::Far || options.labelPosition == LabelPositions::None) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(label).x +
                            ImGui::GetStyle().ItemSpacing.x);
            ImGui::Text(label, *value);
        }
    }
    PopStyleSlider();
    ImGui::EndDisabled();
    ImGui::EndGroup();
    if (options.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !Ship_IsCStringEmpty(options.disabledTooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.disabledTooltip)).c_str());
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }
    ImGui::PopID();
    return dirty;
}

bool CVarSliderInt(const char* label, const char* cvarName, const IntSliderOptions& options) {
    bool dirty = false;
    int32_t value = CVarGetInteger(cvarName, options.defaultValue);
    if (SliderInt(label, &value, options)) {
        CVarSetInteger(cvarName, value);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ShipInit::Init(cvarName);
        dirty = true;
    }
    return dirty;
}

void ClampFloat(float* value, float min, float max, float step) {
    int factor = 1;
    if (step < 1.0f) {
        factor *= 10;
    }
    if (step < 0.1f) {
        factor *= 10;
    }
    if (step < 0.01f) {
        factor *= 10;
    }
    if (step < 0.001f) {
        factor *= 10;
    }
    if (step < 0.0001f) {
        factor *= 10;
    }
    if (step < 0.00001f) {
        factor *= 10;
    }
    if (*value < min) {
        *value = min;
    } else if (*value > max) {
        *value = max;
    } else {
        int trunc = (int)std::round(*value * factor);
        *value = (float)trunc / factor;
    }
}

bool SliderFloat(const char* _labelId, float* value, const FloatSliderOptions& _options) {
    // OOT D.E.: translated label, unchanged ID, label above when the screen is narrow.
    const std::string translatedLabelStr = TranslateText(_labelId);
    const char* label = translatedLabelStr.c_str();
    FloatSliderOptions options = _options;
    options.labelPosition = FitLabelPosition(options.labelPosition, label);
    bool dirty = false;
    std::string invisibleLabelStr = "##" + std::string(_labelId);
    const char* invisibleLabel = invisibleLabelStr.c_str();
    float valueToDisplay = options.isPercentage ? *value * 100.0f : *value;
    float maxToDisplay = options.isPercentage ? options.max * 100.0f : options.max;
    float minToDisplay = options.isPercentage ? options.min * 100.0f : options.min;
    ImGui::PushID(_labelId);
    ImGui::BeginGroup();
    ImGui::BeginDisabled(options.disabled);
    PushStyleSlider(options.color);
    float labelSpacing = ImGui::CalcTextSize(label).x + ImGui::GetStyle().ItemSpacing.x;
    float width = (options.size == ImVec2(0, 0)) ? ImGui::GetContentRegionAvail().x : options.size.x;
    if (options.labelPosition == LabelPositions::Near || options.labelPosition == LabelPositions::Far) {
        width = width - (ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x);
    }
    ImGui::AlignTextToFramePadding();
    if (options.alignment == ComponentAlignments::Right) {
        ImGui::Text(label, *value);
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::NewLine();
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
        } else if (options.labelPosition == LabelPositions::Near) {
            width -= labelSpacing;
            ImGui::SameLine();
        } else if (options.labelPosition == LabelPositions::Far || options.labelPosition == LabelPositions::None) {
            width -= labelSpacing;
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
        }
    } else if (options.alignment == ComponentAlignments::Left) {
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::TextWrapped(label, *value);
        }
    }
    if (options.showButtons) {
        if (Button("-", ButtonOptions{ .color = options.color }.Size(Sizes::Inline)) && *value > options.min) {
            *value -= options.step;
            if (options.clamp) {
                ClampFloat(value, options.min, options.max, options.step);
            }
            dirty = true;
        }
        ImGui::SameLine(0, 3.0f);
        ImGui::SetNextItemWidth(width - (ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2 + 3) * 2);
    } else {
        ImGui::SetNextItemWidth(width);
    }
    if (ImGui::SliderScalar(invisibleLabel, ImGuiDataType_Float, &valueToDisplay, &minToDisplay, &maxToDisplay,
                            options.format, options.flags)) {
        *value = options.isPercentage ? valueToDisplay / 100.0f : valueToDisplay;
        if (options.clamp) {
            ClampFloat(value, options.min, options.max, options.step);
        }
        dirty = true;
    }
    if (options.showButtons) {
        ImGui::SameLine(0, 3.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (Button("+", ButtonOptions{ .color = options.color }.Size(Sizes::Inline)) && *value < options.max) {
            *value += options.step;
            if (options.clamp) {
                ClampFloat(value, options.min, options.max, options.step);
            }
            dirty = true;
        }
    }

    if (options.alignment == ComponentAlignments::Left) {
        if (options.labelPosition == LabelPositions::Near) {
            ImGui::SameLine();
            ImGui::Text(label, *value);
        } else if (options.labelPosition == LabelPositions::Far || options.labelPosition == LabelPositions::None) {
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - labelSpacing);
            ImGui::Text(label, *value);
        }
    }
    PopStyleSlider();
    ImGui::EndDisabled();
    ImGui::EndGroup();
    if (options.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !Ship_IsCStringEmpty(options.disabledTooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.disabledTooltip)).c_str());
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }
    ImGui::PopID();
    return dirty;
}

bool CVarSliderFloat(const char* label, const char* cvarName, const FloatSliderOptions& options) {
    bool dirty = false;
    float value = CVarGetFloat(cvarName, options.defaultValue);
    if (SliderFloat(label, &value, options)) {
        CVarSetFloat(cvarName, value);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ShipInit::Init(cvarName);
        dirty = true;
    }
    return dirty;
}

int InputTextResizeCallback(ImGuiInputTextCallbackData* data) {
    std::string* value = (std::string*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        value->resize(data->BufTextLen);
        data->Buf = (char*)value->c_str();
    }
    return 0;
}

bool InputString(const char* _labelId, std::string* value, const InputOptions& options) {
    const std::string translatedLabelStr = TranslateText(_labelId);
    const char* label = translatedLabelStr.c_str();
    const std::string hiddenIdStr = "##" + std::string(_labelId);
    bool dirty = false;
    ImGui::PushID(_labelId);
    ImGui::BeginGroup();
    ImGui::BeginDisabled(options.disabled);
    PushStyleInput(options.color);
    float width = (options.size == ImVec2(0, 0)) ? ImGui::GetContentRegionAvail().x : options.size.x;
    if (options.alignment == ComponentAlignments::Left) {
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::TextWrapped("%s", label);
        }
    } else if (options.alignment == ComponentAlignments::Right) {
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::NewLine();
            ImGui::SameLine(width - ImGui::CalcTextSize(label).x);
            ImGui::TextWrapped("%s", label);
        }
    }
    ImGui::SetNextItemWidth(width);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;
    if (options.secret) {
        flags |= ImGuiInputTextFlags_Password;
    }
    flags |= options.addedFlags;
    if (ImGui::InputText(options.labelPosition == LabelPositions::Above ? hiddenIdStr.c_str() : label, (char*)value->c_str(), value->capacity() + 1, flags, InputTextResizeCallback, value)) {
        dirty = true;
    }
    if (value->empty() && !options.placeholder.empty()) {
        ImGui::SameLine(17.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "%s", options.placeholder.c_str());
    }
    PopStyleInput();
    ImGui::EndDisabled();
    ImGui::EndGroup();
    if (options.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !Ship_IsCStringEmpty(options.disabledTooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.disabledTooltip)).c_str());
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }
    ImGui::PopID();
    return dirty;
}

bool CVarInputString(const char* label, const char* cvarName, const InputOptions& options) {
    bool dirty = false;
    std::string value = CVarGetString(cvarName, options.defaultValue.c_str());
    if (InputString(label, &value, options)) {
        CVarSetString(cvarName, value.c_str());
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ShipInit::Init(cvarName);
        dirty = true;
    }
    return dirty;
}

bool InputInt(const char* _labelId, int32_t* value, const InputOptions& options) {
    const std::string translatedLabelStr = TranslateText(_labelId);
    const char* label = translatedLabelStr.c_str();
    const std::string hiddenIdStr = "##" + std::string(_labelId);
    bool dirty = false;
    ImGui::PushID(_labelId);
    ImGui::BeginGroup();
    ImGui::BeginDisabled(options.disabled);
    PushStyleInput(options.color);
    float width = (options.size == ImVec2(0, 0)) ? ImGui::GetContentRegionAvail().x : options.size.x;
    if (options.alignment == ComponentAlignments::Left) {
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::Text(label, *value);
        }
    } else if (options.alignment == ComponentAlignments::Right) {
        if (options.labelPosition == LabelPositions::Above) {
            ImGui::NewLine();
            ImGui::SameLine(width - ImGui::CalcTextSize(label).x);
            ImGui::Text(label, *value);
        }
    }
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputScalar(options.labelPosition == LabelPositions::Above ? hiddenIdStr.c_str() : label, ImGuiDataType_S32, value, nullptr, nullptr, nullptr, options.addedFlags)) {
        dirty = true;
    }
    if ((ImGui::GetItemStatusFlags() & ImGuiItemStatusFlags_Edited) && !options.placeholder.empty()) {
        ImGui::SameLine(17.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "%s", options.placeholder.c_str());
    }
    PopStyleInput();
    ImGui::EndDisabled();
    ImGui::EndGroup();
    if (options.disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
        !Ship_IsCStringEmpty(options.disabledTooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.disabledTooltip)).c_str());
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }
    ImGui::PopID();
    return dirty;
}

bool CVarInputInt(const char* label, const char* cvarName, const InputOptions& options) {
    bool dirty = false;
    int32_t defaultValue = std::stoi(options.defaultValue);
    int32_t value = CVarGetInteger(cvarName, defaultValue);
    if (InputInt(label, &value, options)) {
        CVarSetInteger(cvarName, value);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ShipInit::Init(cvarName);
        dirty = true;
    }
    return dirty;
}

bool CVarColorPicker(const char* label, const char* cvarName, Color_RGBA8 defaultColor, bool hasAlpha,
                     uint8_t modifiers, UIWidgets::Colors themeColor) {
    std::string valueCVar = std::string(cvarName) + ".Value";
    std::string rainbowCVar = std::string(cvarName) + ".Rainbow";
    std::string lockedCVar = std::string(cvarName) + ".Locked";
    Color_RGBA8 color = CVarGetColor(valueCVar.c_str(), defaultColor);
    ImVec4 colorVec = ImVec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    bool changed = false;
    bool showReset = modifiers & ColorPickerResetButton;
    bool showRandom = modifiers & ColorPickerRandomButton;
    bool showRainbow = modifiers & ColorPickerRainbowCheck;
    bool showLock = modifiers & ColorPickerLockCheck;
    bool locked = CVarGetInteger(lockedCVar.c_str(), 0);
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs;
    ImGui::BeginDisabled(locked);
    PushStyleCombobox(UIWidgets::Colors::DarkGray);
    // OOT D.E.: swatch with a hidden ID, then the translated name.
    const std::string colorId = "##" + std::string(label);
    const std::string colorName = TranslateText(label);
    const std::string colorNameVisible = colorName.substr(0, colorName.find("##"));
    if (hasAlpha) {
        changed = ImGui::ColorEdit4(colorId.c_str(), (float*)&colorVec,
                                    flags | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
    } else {
        changed = ImGui::ColorEdit3(colorId.c_str(), (float*)&colorVec, flags | ImGuiColorEditFlags_NoAlpha);
    }
    PopStyleCombobox();
    if (!colorNameVisible.empty()) {
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(colorNameVisible.c_str());
    }
    ImGui::AlignTextToFramePadding();
    if (showReset) {
        ImGui::SameLine();
        std::string uniqueTag = "Reset##" + std::string(label);
        if (UIWidgets::Button(uniqueTag.c_str(),
                              UIWidgets::ButtonOptions({ { .tooltip = "Resets this color to its default value" } })
                                  .Color(themeColor)
                                  .Size(UIWidgets::Sizes::Inline))) {
            // TODO: Remove for next minor or major version, temporary fix for already migrated configs to 3 for 9.0.0
            CVarClear((std::string(cvarName) + ".R").c_str());
            CVarClear((std::string(cvarName) + ".G").c_str());
            CVarClear((std::string(cvarName) + ".B").c_str());
            CVarClear((std::string(cvarName) + ".A").c_str());
            CVarClear((std::string(cvarName) + ".Type").c_str());
            CVarClearBlock(valueCVar.c_str());
            Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        }
    }
    if (showRandom) {
        ImGui::SameLine();
        std::string uniqueTag = "Random##" + std::string(label);
        if (UIWidgets::Button(uniqueTag.c_str(),
                              UIWidgets::ButtonOptions({ { .tooltip = "Generates a random color value to use" } })
                                  .Color(themeColor)
                                  .Size(UIWidgets::Sizes::Inline))) {
            colorVec = GetRandomValue();
            color.r = fmin(fmax(colorVec.x * 255, 0), 255);
            color.g = fmin(fmax(colorVec.y * 255, 0), 255);
            color.b = fmin(fmax(colorVec.z * 255, 0), 255);
            CVarSetColor(valueCVar.c_str(), color);
            CVarSetInteger(rainbowCVar.c_str(), 0); // On click disable rainbow mode.
            ShipInit::Init(rainbowCVar.c_str());
            Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        }
    }
    if (showRainbow) {
        ImGui::SameLine();
        std::string uniqueTag = "Rainbow##" + std::string(cvarName) + "Rainbow";

        UIWidgets::CVarCheckbox(
            uniqueTag.c_str(), rainbowCVar.c_str(),
            UIWidgets::CheckboxOptions(
                { { .tooltip = "Cycles through colors on a timer\nOverwrites previously chosen color" } })
                .Color(themeColor));
    }
    ImGui::EndDisabled();
    if (showLock) {
        ImGui::SameLine();
        std::string uniqueTag = "Lock##" + std::string(cvarName) + "Locked";

        UIWidgets::CVarCheckbox(
            uniqueTag.c_str(), lockedCVar.c_str(),
            UIWidgets::CheckboxOptions({ { .tooltip = "Prevents this color from being changed" } }).Color(themeColor));
    }
    if (changed) {
        color.r = (uint8_t)(colorVec.x * 255.0f);
        color.g = (uint8_t)(colorVec.y * 255.0f);
        color.b = (uint8_t)(colorVec.z * 255.0f);
        color.a = (uint8_t)(colorVec.w * 255.0f);
        CVarSetColor(valueCVar.c_str(), color);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ShipInit::Init(valueCVar.c_str());
        changed = true;
    }

    return changed;
}

bool RadioButton(const char* _labelId, bool active, const RadioButtonsOptions& options) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const std::string translatedLabelStr = TranslateText(_labelId);
    const char* label = translatedLabelStr.c_str();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(_labelId);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    const float square_sz = ImGui::GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
    const ImRect total_bb(
        pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
                          label_size.y + style.FramePadding.y * 2.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    ImVec2 center = check_bb.GetCenter();
    center.x = IM_ROUND(center.x);
    center.y = IM_ROUND(center.y);
    const float radius = (square_sz - 1.0f) * 0.5f;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        ImGui::MarkItemEdited(id);

    ImGui::RenderNavCursor(total_bb, id);
    const int num_segment = window->DrawList->_CalcCircleAutoSegmentCount(radius);
    window->DrawList->AddCircleFilled(center, radius,
                                      ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive
                                                         : hovered         ? ImGuiCol_FrameBgHovered
                                                                           : ImGuiCol_FrameBg),
                                      num_segment);
    if (active) {
        const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 6.0f));
        window->DrawList->AddCircleFilled(center, radius - pad, ImGui::GetColorU32(ImGuiCol_CheckMark));
    }

    if (style.FrameBorderSize > 0.0f) {
        window->DrawList->AddCircle(center + ImVec2(1, 1), radius, ImGui::GetColorU32(ImGuiCol_BorderShadow),
                                    num_segment, style.FrameBorderSize);
        window->DrawList->AddCircle(center, radius, ImGui::GetColorU32(ImGuiCol_Border), num_segment,
                                    style.FrameBorderSize);
    }

    ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    if (g.LogEnabled)
        ImGui::LogRenderedText(&label_pos, active ? "(x)" : "( )");
    if (label_size.x > 0.0f)
        RenderText(label_pos, label, ImGui::FindRenderedTextEnd(label), true);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool CVarRadioButton(const char* text, const char* cvarName, int32_t id, const RadioButtonsOptions& options) {
    std::string make_invisible = "##" + std::string(text) + std::string(cvarName);

    bool ret = false;
    int val = CVarGetInteger(cvarName, 0);
    PushStyleCheckbox(options.color);
    if (ImGui::RadioButton(make_invisible.c_str(), id == val)) {
        CVarSetInteger(cvarName, id);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        ret = true;
    }
    ImGui::SameLine();
    ImGui::TextWrapped("%s", TranslateText(text).c_str());
    PopStyleCheckbox();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !Ship_IsCStringEmpty(options.tooltip)) {
        ImGui::SetTooltip("%s", WrappedText(TranslateText(options.tooltip)).c_str());
    }

    return ret;
}

void DrawFlagArray32(const std::string& name, uint32_t& flags, Colors color) {
    ImGui::PushID(name.c_str());
    for (int32_t flagIndex = 0; flagIndex < 32; flagIndex++) {
        if ((flagIndex % 8) != 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(flagIndex);
        uint32_t bitMask = 1 << flagIndex;
        bool flag = (flags & bitMask) != 0;
        PushStyleCheckbox(color);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
        std::string id = fmt::format("##{}{}", name, flagIndex);
        if (ImGui::Checkbox(id.c_str(), &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleVar();
        PopStyleCheckbox();
        ImGui::PopID();
    }
    ImGui::PopID();
}

void DrawFlagArray16(const std::string& name, uint16_t& flags, Colors color) {
    ImGui::PushID(name.c_str());
    for (int16_t flagIndex = 0; flagIndex < 16; flagIndex++) {
        if ((flagIndex % 8) != 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(flagIndex);
        uint16_t bitMask = 1 << flagIndex;
        bool flag = (flags & bitMask) != 0;
        PushStyleCheckbox(color);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
        std::string id = fmt::format("##{}{}", name, flagIndex);
        if (ImGui::Checkbox(id.c_str(), &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleVar();
        PopStyleCheckbox();
        ImGui::PopID();
    }
    ImGui::PopID();
}

void DrawFlagArray8(const std::string& name, uint8_t& flags, Colors color) {
    ImGui::PushID(name.c_str());
    for (int8_t flagIndex = 0; flagIndex < 8; flagIndex++) {
        if ((flagIndex % 8) != 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(flagIndex);
        uint8_t bitMask = 1 << flagIndex;
        bool flag = (flags & bitMask) != 0;
        PushStyleCheckbox(color);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
        std::string id = fmt::format("##{}{}", name, flagIndex);
        if (ImGui::Checkbox(id.c_str(), &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleVar();
        PopStyleCheckbox();
        ImGui::PopID();
    }
    ImGui::PopID();
}

void DrawFlagArray8Mask(const std::string& name, uint8_t& flags, Colors color) {
    ImGui::PushID(name.c_str());
    for (int8_t flagIndex = 0; flagIndex < 8; flagIndex++) {
        if ((flagIndex % 8) != 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(flagIndex);
        uint8_t bitMask = 1 << flagIndex;
        bool flag = (flags & bitMask) != 0;
        PushStyleCheckbox(color);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
        std::string id = fmt::format("##{}{}", name, flagIndex);
        if (ImGui::Checkbox(id.c_str(), &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleVar();
        PopStyleCheckbox();
        ImGui::PopID();
    }
    ImGui::PopID();
}
} // namespace UIWidgets

ImVec4 GetRandomValue() {
#if !defined(__SWITCH__) && !defined(__WIIU__)
    std::random_device rd;
    std::mt19937 rng(rd());
#else
    size_t seed = std::hash<std::string>{}(std::to_string(rand()));
    std::mt19937_64 rng(seed);
#endif
    std::uniform_int_distribution<int> dist(0, 255 - 1);

    ImVec4 NewColor;
    NewColor.x = (float)(dist(rng)) / 255.0f;
    NewColor.y = (float)(dist(rng)) / 255.0f;
    NewColor.z = (float)(dist(rng)) / 255.0f;
    return NewColor;
}

Color_RGBA8 RGBA8FromVec(ImVec4 vec) {
    Color_RGBA8 color = { vec.x * 255, vec.y * 255, vec.z * 255, vec.w * 255 };
    return color;
}

ImVec4 VecFromRGBA8(Color_RGBA8 color) {
    ImVec4 vec = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
    return vec;
}
