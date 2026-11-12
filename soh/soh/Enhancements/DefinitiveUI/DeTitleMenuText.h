#pragma once

// OOT D.E.: texts of the main menu (PT-BR). Shared by the game (DeTitleMenu.cpp) and the preview tool, so the
// screenshots show exactly what the game shows.

namespace DefinitiveUI {

enum TitleMainItem {
    MAIN_CONTINUE,
    MAIN_NEW_GAME,
    MAIN_FILES,
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
    "Continuar", "Novo Jogo", "Arquivos de Jogo", "Diário de Quests", "Mods", "Configurações", "Extras",
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
    "Os três arquivos já estão em uso. Apague um deles em Arquivos de Jogo para começar outra aventura.";
constexpr const char* kFiles = "Jogue, copie ou apague os três arquivos salvos.";
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

// Arquivos de Jogo.
constexpr const char* kFilesTitle = "Arquivos de Jogo";
constexpr const char* kFileLabel = "Arquivo %d · %s";
constexpr const char* kFileEmptyLabel = "Arquivo %d · Vazio";
constexpr const char* kFileEmpty = "Arquivo vazio. Escolha para começar uma nova aventura aqui.";
constexpr const char* kFileInfo = "%s. Mortes: %d. Escolha para jogar, copiar ou apagar.";
constexpr const char* kFileIncompatible = "%s. Este arquivo precisa de dados do jogo que a sua ROM não tem.";
constexpr const char* kQuestNormal = "Aventura original";
constexpr const char* kQuestMaster = "Master Quest";
constexpr const char* kQuestRando = "Randomizer";
constexpr const char* kBack = "Voltar";
constexpr const char* kBackDetail = "Volta à tela anterior.";

constexpr const char* kPlay = "Jogar";
constexpr const char* kPlayDetail = "Carrega este arquivo e continua a aventura.";
constexpr const char* kCopy = "Copiar";
constexpr const char* kCopyDetail = "Faz uma cópia deste arquivo em um arquivo vazio.";
constexpr const char* kCopyNoRoom = "Não há arquivo vazio para receber a cópia.";
constexpr const char* kErase = "Apagar";
constexpr const char* kEraseDetail = "Apaga este arquivo para sempre. O jogo pergunta antes.";

constexpr const char* kEraseTitle = "Apagar o Arquivo %d?";
constexpr const char* kEraseNo = "Não, manter";
constexpr const char* kEraseNoDetail = "Nada é apagado.";
constexpr const char* kEraseYes = "Sim, apagar";
constexpr const char* kEraseYesDetail = "O arquivo \"%s\" será apagado para sempre. Não dá para desfazer.";

constexpr const char* kCopyTitle = "Copiar para...";
constexpr const char* kCopyTo = "Copia \"%s\" para o Arquivo %d.";

// Novo Jogo.
constexpr const char* kNewTitle = "Novo Jogo · Arquivo %d";
constexpr const char* kQuestNormalDetail = "A aventura de Ocarina of Time, com as masmorras originais.";
constexpr const char* kQuestMasterDetail = "As masmorras refeitas e mais difíceis da Master Quest.";
constexpr const char* kClassic = "Randomizer e Boss Rush";
constexpr const char* kClassicDetail =
    "Abre a tela clássica de arquivos, onde ficam as opções do Randomizer e o Boss Rush.";
constexpr const char* kNameTitle = "Nome do Herói";
constexpr const char* kNameHint = "Até 8 letras";
constexpr const char* kKeyCase = "Aa";
constexpr const char* kKeySpace = "Espaço";
constexpr const char* kKeyErase = "Apagar";
constexpr const char* kKeyBack = "Voltar";
constexpr const char* kKeyDone = "Começar";

constexpr const char* kLoading = "Carregando";
constexpr const char* kCreating = "Criando a aventura";
} // namespace Strings

} // namespace DefinitiveUI
