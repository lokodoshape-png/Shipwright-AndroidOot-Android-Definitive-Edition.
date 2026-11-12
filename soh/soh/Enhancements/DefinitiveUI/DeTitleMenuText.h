#pragma once

// OOT D.E.: texts of the main menu (PT-BR). Shared by the game (DeTitleMenu.cpp) and the preview tool, so the
// screenshots show exactly what the game shows.

namespace DefinitiveUI {

enum TitleMainItem {
    MAIN_CONTINUE,
    MAIN_NEW_GAME,
    MAIN_FILE_SELECT,
    MAIN_JOURNAL,
    MAIN_MODS,
    MAIN_SETTINGS,
    MAIN_EXTRAS,
    MAIN_COUNT,
};

enum TitleExtrasItem {
    EXTRAS_MODS,
    EXTRAS_VERSION,
    EXTRAS_CREDITS,
    EXTRAS_BACK,
    EXTRAS_COUNT,
};

namespace Strings {
constexpr const char* kMainLabels[MAIN_COUNT] = {
    "Continuar", "Novo Jogo", "Selecionar Arquivo", "Diário de Quests", "Mods", "Configurações", "Extras",
};
constexpr const char* kExtrasTitle = "Extras";
constexpr const char* kExtrasLabels[EXTRAS_COUNT] = {
    "Mods Instalados",
    "Informações da Versão",
    "Créditos",
    "Voltar",
};

constexpr const char* kContinue = "Retome a aventura do último arquivo jogado.";
constexpr const char* kContinueNone = "Ainda não há nenhum arquivo salvo. Escolha Novo Jogo para começar.";
constexpr const char* kNewGame = "Começa uma nova aventura no primeiro arquivo livre (Arquivo %d).";
constexpr const char* kNewGameFull =
    "Os três arquivos já estão em uso. Apague um deles em Selecionar Arquivo para começar outra aventura.";
constexpr const char* kFileSelect = "Escolha, copie ou apague os arquivos salvos.";
constexpr const char* kJournal = "Continua o último arquivo e abre o Diário de Quests no menu de pausa.";
constexpr const char* kJournalNone = "O Diário de Quests acompanha um arquivo salvo. Comece um jogo primeiro.";
constexpr const char* kMods = "Ative, desative e instale mods de texturas, textos e traduções.";
constexpr const char* kSettings = "Jogo, gráficos, áudio, controles, interface e acessibilidade.";
constexpr const char* kExtras = "Mods instalados, informações da versão e créditos.";

constexpr const char* kExtrasMods = "%d mod(s) ativo(s) de %d na pasta mods. Abre a lista para ativar, desativar ou "
                                    "instalar.";
constexpr const char* kExtrasVersion =
    "Ocarina of Time: Definitive Edition %s\nBase: Ship of Harkinian %s (Android).\nOs dados do jogo vêm da sua "
    "própria ROM.";
constexpr const char* kExtrasCredits = "Tela de créditos: chega na próxima etapa da nova interface.";
constexpr const char* kExtrasBack = "Volta ao menu principal.";

constexpr const char* kSaveTitle = "Arquivo %d · %s";
} // namespace Strings

} // namespace DefinitiveUI
