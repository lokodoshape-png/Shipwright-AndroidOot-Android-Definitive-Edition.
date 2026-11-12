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
| Estado e configurações | CVars (`CVAR_SETTING(...)`) | `DefinitiveUI.TitleMenu` (liga/desliga o menu), `DefinitiveUI.LastFile` (último arquivo jogado). |
| Saves | `soh/soh/SaveManager.cpp` (`Save_GetSaveMetaInfo`) | Lidos só para mostrar (nome, corações). Carregar e criar arquivos continua sendo feito pela seleção de arquivos original. |
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

- `z_en_mag.c` — com o logo na tela, entrega o controle ao menu; esconde o "PRESS START"; desloca o logo 56 px (N64)
  para a esquerda.
- `z_file_choose.c` — *Continuar*/*Novo Jogo*: escolhe o arquivo como se o jogador tivesse apertado A nele; *Continuar*
  responde "Sim" na confirmação.
- `z_kaleido_setup.c` — *Diário de Quests*: abre o pause no Diário quando o jogo permite pausar (mesmas condições do
  Start).
- `OTRGlobals.cpp` — carrega as fontes; esconde os controles de toque enquanto o menu está na tela.
- `SohGui.cpp`, `SohMenuSettings.cpp` — registra a tela; opção para voltar ao "PRESS START".

## Prévia no computador

`scripts/ootde_ui_preview/build.sh <pasta do imgui v1.91.6-docking> <saída>` desenha as telas com o mesmo código do
jogo (sem ROM nem GPU). O fundo da prévia é uma imagem gerada; no jogo o fundo é a abertura rodando.

## Próximas fases

1. Seleção de arquivos (tela 2) e Novo Jogo (tela 3) — sobre `ovl_file_choose`, mantendo copiar/apagar/criar originais.
2. Configurações e Controles (telas 4 e 5) — categorias sobre as opções existentes do `SohMenu` e do editor de controles.
3. Diário, Inventário e Mapa (telas 6–8) — dentro do pause; dependem da versão 0.8 (que mexe no Diário).
4. Galeria, Extras e Créditos (telas 9–11).
