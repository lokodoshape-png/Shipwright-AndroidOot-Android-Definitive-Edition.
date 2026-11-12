# Interface Definitive Edition

Reformulação visual da interface do OOT D.E., em etapas, na branch `feature/ootde-menu-definitivo`.

## Fase 1 — Auditoria da arquitetura

| Sistema | Onde está | O que isso significa para a interface |
| --- | --- | --- |
| Menus do jogo (título, seleção de arquivos, pause) | `soh/src/overlays/actors/ovl_En_Mag` (logo e "PRESS START"), `soh/src/overlays/gamestates/ovl_file_choose` (arquivos), `soh/src/overlays/misc/ovl_kaleido_scope` (pause) | Desenhados com display lists do N64 em 320x240. É ali que estão as regras reais (saves, novo arquivo, pause). A nova interface manda nesses sistemas, não os substitui. |
| Menu do port (Configurações, Mods) | `soh/soh/SohGui` (`SohMenu`, `Menu.cpp`, `ModsMenu.cpp`) | Dear ImGui sobre o jogo. As opções existentes ficam onde estão; a nova interface abre a página certa (`Menu.ActiveHeader` / `Menu.SettingsSidebarSection`). |
| Renderização | libultraship (Fast3D + ImGui, OpenGL ES no Android) | A nova interface é desenhada com ImGui sobre a imagem do jogo, em resolução nativa (texto e molduras nítidos em qualquer tela), sem mexer no pipeline do jogo. |
| Texturas | `soh/assets/custom/` → `soh.otr` (o APK leva um `soh.otr` pronto; `scripts/otr_add_custom.py`) | Texturas novas em `soh/assets/custom/ootde/ui/` (RGBA32). Carregadas como texturas do ImGui só enquanto a tela está aberta e liberadas ao sair. |
| Fontes | `soh/assets/custom/fonts/` (recurso Font do libultraship) | Cinzel e EB Garamond adicionadas ao atlas do ImGui na inicialização. |
| Entrada | Controle do jogo (`play->state.input`), mapeado pelo libultraship (Bluetooth, teclado, controles de toque) | O menu lê o controle do próprio jogo, então qualquer controle já configurado funciona. Toque/mouse como no Diário de Quests (`Window::GetMousePos`). |
| Estado e configurações | CVars (`CVAR_SETTING(...)`) | `DefinitiveUI.TitleMenu` (liga/desliga o menu), `DefinitiveUI.Splash` (imagem de abertura), `DefinitiveUI.MenuMusicVolume` (0 = música original), `DefinitiveUI.LastFile` (último arquivo jogado). |
| Saves | `soh/soh/SaveManager.cpp` (`Save_GetSaveMetaInfo`) | Lidos para mostrar (nome, corações). Copiar e apagar chamam `Save_CopyFile`/`Save_DeleteFile`, como as telas originais. Criar (`Sram_InitSave`) e carregar continuam dentro da seleção de arquivos original, que trabalha escondida. |
| Diário de Quests | `ovl_kaleido_scope/questmenu/*` (página do pause, `PAUSE_QUEST_MODE_LOG`) | O menu principal só pede para o pause abrir nele (`z_kaleido_setup.c`), pelo mesmo caminho do Start. |

## Código

`soh/soh/Enhancements/DefinitiveUI/`

- `DeTheme.*` — paleta, fontes, texturas e formas (painel de pedra com moldura de ouro, botões, Triforce, medalhão com
  runas, Navi, corações, partículas). Só depende do ImGui.
- `DeTitleMenuView.*` — desenho do menu principal e áreas de toque. Não muda nada no jogo.
- `DeTitleMenuText.h` — textos em PT-BR.
- `DeTitleMenu.cpp` — modelo (itens, navegação, o que cada botão faz), carregamento/liberação de recursos e as funções
  C chamadas pelo jogo (`DefinitiveUI.h`).

Ganchos no código do jogo (pequenos e marcados com `OOT D.E.`):

- `z_en_mag.c` — pula a animação do logo do N64 e entrega o controle ao menu; esconde o "PRESS START".
- `z_file_choose.c` — trabalha debaixo da tela "Carregando", sem receber controle e sem música: cria o arquivo pedido
  pelo menu (nome e aventura já escolhidos, `Sram_InitSave`) ou escolhe o arquivo como se o jogador tivesse apertado A
  nele e responde "Sim". Se o pedido não puder ser atendido, a tela original aparece e assume. *Randomizer e Boss
  Rush* abrem essa tela de propósito.
- `z_kaleido_setup.c` — *Diário de Quests*: abre o pause no Diário quando o jogo permite pausar (mesmas condições do
  Start).
- `OTRGlobals.cpp` — carrega as fontes; esconde os controles de toque enquanto o menu está na tela.
- `SohGui.cpp`, `SohMenuSettings.cpp` — registra a tela; opção para voltar ao "PRESS START".

## Prévia no computador

`scripts/ootde_ui_preview/build.sh <pasta do imgui v1.91.6-docking> <saída>` desenha as telas com o mesmo código do
jogo (sem ROM nem GPU), com as mesmas texturas: abertura, menu principal, Extras, Arquivos de Jogo, opções do
arquivo, Novo Jogo, nome do herói e "Carregando". Com `video` no fim, grava as animações em `menu-animado.mp4`.

## Fluxo

1. Ao abrir o jogo: imagem "Definitive Edition" (1,5 s entrando, fica até completar 4 s e o menu estar pronto, sai em
   1 s; toque ou botão pula).
2. Menu principal. *Arquivos de Jogo* e *Novo Jogo* são telas do próprio menu (`SCREEN_FILES`, `SCREEN_FILE`,
   `SCREEN_ERASE`, `SCREEN_COPY`, `SCREEN_QUEST`, `SCREEN_NAME` em `DeTitleMenu.cpp`).
3. Ao jogar ou criar um arquivo, a tela "Carregando" cobre o jogo até o arquivo carregar (`OnLoadGame`); se algo
   falhar, ela sai sozinha depois de 25 s e a tela original fica visível.

O toque vale ao soltar o dedo sobre o item, sem ter arrastado (`PointerState::tap`).

## Arte

Fundo, logo, botões e moldura vêm da imagem de referência (`docs/interface-de/referencia.png`), recortados e
ampliados 4x com a rede EDSR por `scripts/ootde_ui_art.py` (instruções no próprio script). Os botões perdem o texto
original (o jogo escreve o seu) e ganham fundo transparente. Depois, `scripts/otr_add_custom.py` coloca tudo no
`soh.otr` do APK.

## Próximas fases

1. Opções do Randomizer e Boss Rush em telas novas (hoje abrem a tela clássica de arquivos).
2. Configurações e Controles (telas 4 e 5) — categorias sobre as opções existentes do `SohMenu` e do editor de controles.
3. Diário, Inventário e Mapa (telas 6–8) — dentro do pause; dependem da versão 0.8 (que mexe no Diário).
4. Galeria, Extras e Créditos (telas 9–11).
