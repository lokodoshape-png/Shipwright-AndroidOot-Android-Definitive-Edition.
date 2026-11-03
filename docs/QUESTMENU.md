# QuestMenu — Diário de Quests

O Diário de Quests é o modo `PAUSE_QUEST_MODE_LOG` da página Status de Missão do menu de pausa. Ele é desenhado
com as display lists do próprio jogo (Fast3D no PC e no Android), com a fonte, os ícones e o mapa do jogo. Não é uma
janela ImGui e não tem estado próprio salvo: tudo é lido do save a cada quadro.

## Arquivos

| Arquivo | Parte | O que faz |
|---|---|---|
| `soh/src/overlays/misc/ovl_kaleido_scope/questmenu/questmenu.h` | todas | Tipos, tabelas, contexto e funções do módulo |
| `.../questmenu/questmenu_data.c` | dados | Tabelas das missões, checagens no save, estados e objetivos |
| `.../questmenu/questmenu_state.c` | estado | Abrir e fechar (virada da página), categoria e linha, controle, teclado, toque e mouse |
| `.../questmenu/questmenu_gfx.c` | desenho | Formas, texturas e texto (fonte do jogo, letras do mod PT-BR) |
| `.../questmenu/questmenu_ui.c` | interface | Layout do Diário e as áreas de toque |
| `soh/soh/Enhancements/QuestMenu/QuestMenuPointer.cpp/.h` | estado | Posição do mouse / dedo em coordenadas do jogo (C++, `extern "C"`) |
| `soh/assets/custom/overlays/questmenu/*.rgba32.png` | texturas | Pedra, pergaminho, marca, asas, ícones das categorias, cadeado, ampulheta |
| `soh/assets/soh_assets.h` | texturas | Declara as texturas `gQM*Tex` (`__OTR__overlays/questmenu/...`) |
| `soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h` | integração | Funções `KaleidoScope_*` chamadas pelo menu de pausa |
| `soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c` | integração | Chama o Diário, C-Cima / B, e trava a troca de página com o Diário aberto |

Os arquivos novos entram no build sozinhos: o `soh/CMakeLists.txt` junta `src/*.c` e `soh/*.cpp` com
`GLOB_RECURSE`, e todo PNG em `soh/assets/custom/` é empacotado no `soh.otr`.

O arquivo antigo `z_kaleido_quest_log.c` foi dividido nesses módulos e removido.

## Integração com o menu de pausa

- `KaleidoScope_CycleQuestMode` (C-Cima) e `KaleidoScope_CloseQuestLog` (B ou "Voltar") começam a virada da página.
  `KaleidoScope_ApplyQuestPageFlip` estreita a página e troca o modo na metade.
- `KaleidoScope_DrawQuestJournal` é chamada depois do painel de informações. Ela chama `QuestMenu_Update`, que
  cuida da animação, da entrada e das linhas, e depois `QuestMenu_DrawJournal`.
- Com o Diário aberto, `KaleidoScope_HandlePageToggles` não troca de página.
- `KaleidoScope_DrawQuestLogHint` desenha o aviso "C-Cima Missões" na página Status.

## Dados

Cada categoria (`QuestMenuCategory`) é montada por `QuestMenu_BuildRows` a partir de tabelas `QuestMenuEntry`:

- `name`, `detail`, `icon`: nome, descrição e ícone (texturas do jogo);
- `age`, `needs`: idade e itens que a missão precisa;
- `check`, `scene`, `flag`: como saber se está concluída (`QuestMenuCheckType`: flags de evento, itens, medalhões,
  áreas do mapa visitadas, salas de masmorra visitadas...);
- `objectives`: lista opcional de objetivos, cada um com a própria checagem.

Para cada tabela também existem `...Points` (o ponto do mapa da pausa) e `...Sections` (os grupos com título). O
tamanho de `...Points` é conferido na compilação.

O estado (`QuestMenuState`) é calculado a cada quadro:

- **concluída**: a checagem da missão é verdadeira;
- **bloqueada**: em tabelas encadeadas (uma história: máscaras, trocas, principal), a etapa anterior não foi feita;
- **em andamento**: algum objetivo já foi feito (ou, nas Skulltulas, alguma ficha);
- **disponível**: nos outros casos.

Cada linha tem um id único (`categoria * 1000 + linha`), mostrado no pergaminho.

**Para adicionar uma missão**, acrescente uma linha na tabela da categoria e o ponto do mapa na tabela `...Points`
correspondente. **Para adicionar uma categoria**, acrescente o valor em `QuestMenuCategory`, o nome, a dica e o ícone
em `gQuestMenuCategory*` e o `case` em `QuestMenu_BuildRows`. O layout lateral tem lugar para seis botões.

## Entrada

- **Controle e teclado** (pelo mapeamento de botões do jogo): cima / baixo escolhem a linha (com repetição);
  L / R, Z, esquerda / direita e C-Esquerda / C-Direita trocam a categoria; B fecha.
- **Toque e mouse**: no Android, o SDL transforma o toque em eventos de mouse. A interface registra a cada quadro as
  áreas tocáveis (`QuestMenu_AddHit`): categorias, linhas, abas L / R, setas de rolagem, a lista toda (para
  arrastar) e "Voltar". Um toque vale quando o dedo sobe na mesma área onde desceu. Arrastar a lista rola sem mover
  a seleção, e a roda do mouse também rola. O ponteiro é ignorado quando um menu ImGui está usando o mouse.

## Desenho

O Diário usa uma tela de 854 x 480 unidades (meio pixel de uma tela 427 x 240), centralizada e reduzida em telas
mais estreitas que 16:9. As laterais das telas largas recebem o fundo escurecido. A projeção é ortográfica 320 x 240,
e o Fast3D estica o x conforme a proporção da tela. `QuestMenuPointer.cpp` faz o caminho inverso para o toque.
