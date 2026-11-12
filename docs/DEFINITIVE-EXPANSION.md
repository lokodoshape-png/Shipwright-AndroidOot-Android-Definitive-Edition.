# OOT D.E. — Definitive Expansion

Plano, auditoria e estado dos dez sistemas da expansão. Branch: `feature/ootde-definitive-expansion` (a `main` não
é alterada diretamente). Este documento é atualizado ao fim de cada fase.

## Fase 0 — Auditoria (12/11/2026)

### Origem
- Commit de origem: `c28198a` (`main`, "0.7.1: Diário de Quests corrigido (#7)"), versão 0.7.1, `versionCode` 17.
- Base: Shipwright-Android (Ship of Harkinian 9.0.2), submódulos `libultraship` (branch `mm` do Waterdish),
  `OTRExporter`, `ZAPDTR`.
- Branches ativas além da `main`:
  - `fix/ootde-0.8-stability` (PR #8, 0.8.0, `versionCode` 18): inicialização, importação de ROM, mods, Diário de
    Quests. Reescreve boa parte do `MainActivity.java`.
  - `feature/ootde-menu-definitivo` (0.9-menu, `versionCode` 19): menu principal Definitive Edition em
    `soh/soh/Enhancements/DefinitiveUI/`.
- A Fase 1 não toca em nenhum arquivo alterado por essas duas branches, exceto `CHANGELOG.md`,
  `Android/app/build.gradle` e `AndroidManifest.xml` (número de versão).

### O que já existe e será reaproveitado

| Sistema pedido | O que o código já tem | Decisão |
| --- | --- | --- |
| Configurações | CVars do SoH em `shipofharkinian.json` | Arquivo próprio `ootde/config.json` (pedido: separado e versionado). CVars continuam para o que já é do SoH. |
| Touch | Overlay Android em `MainActivity.java` + `touchcontrol_overlay.xml`, que alimenta um controle virtual SDL (`libultraship/src/port/mobile/MobileImpl.cpp`). Posições fixas. | Fase 2: tornar o overlay configurável, sem criar outro caminho de entrada. Depende do `MainActivity` da 0.8. |
| Giroscópio | Já funciona: `SDLGyroMapping` lê o sensor do celular no Android e o `z_player.c` soma à mira em primeira pessoa. Sensibilidade e calibração no editor de controles. | Fase 2: faltam suavização, inversão por eixo e escolha de itens. Estender o mapeamento existente. |
| Museu 3D | Nada pronto. Há desenho de modelos de itens (`GetItem_Draw`) e o visualizador de atores de depuração. | Fase 3: sistema novo, reaproveitando os modelos do jogo. |
| Boss Rush | Completo no SoH (`Enhancements/boss-rush`): seleção de chefes, opções, cronômetro, save isolado (`fileNum 0xFE`, nunca gravado). | Fase 6: acrescentar recordes, histórico e ranking; não reescrever. |
| Aparência de Link | Editor de personalização do SoH (cores de túnica, escudo, espada, efeitos). | Fase 3: tela de pré-visualização e perfis por cima dele. |
| Álbum / Explorador | Estatísticas (`gameplaystats`), contagem por cena, rastreadores do randomizer, flags de cena no save. | Fase 4: ler esses dados reais, sem gravar nada na campanha. |
| Conquistas | Nada. Há ganchos de eventos (`GameInteractor`) e notificações na tela. | Fase 5: sistema novo sobre os ganchos, dados em `ootde/`. |
| Modo Master | Master Quest e modificadores (multiplicador de dano, sem corações, perda permanente de coração, apagar save ao morrer). | Fase 6: agrupar em presets e desafios; não duplicar. |
| Saves | `SaveManager` (JSON por slot, seções versionadas). Sem backup; a gravação apagava o arquivo antes de copiar o novo. | Fase 1: feito (abaixo). |
| Randomizer | Completo no SoH (sementes, opções, rastreadores, saves marcados como randomizados). | Fase 7: interface de sementes e histórico; a geração continua a do SoH. |
| HUD | Posições, âncoras e margens de cada elemento no Editor de personalização. | Fase 2: perfis (Clássico, Minimalista, Definitive, Personalizado) e editor por cima desses CVars. |

### Arquitetura adotada
- Código novo em `soh/soh/Enhancements/DefinitiveExpansion/` (o CMake já inclui a pasta por `GLOB_RECURSE`).
- O núcleo (`DEStorage`, `OOTDEConfig`, `DESaveManager`) é C++17 puro, sem depender do jogo, e por isso tem testes
  que rodam no computador (`scripts/tests/run_de_expansion_tests.sh`).
- A ligação com o jogo fica em `DefinitiveExpansion.cpp` e nas páginas do menu (`SohGui/SohMenuDefinitive.cpp`).
- Dados da expansão em `<pasta do app>/ootde/` (no Android, `/storage/emulated/0/OOT-DE/ootde/`): `config.json`,
  `backups/`, `exports/`. Nada disso entra nos arquivos de save.

## Fase 1 — Saves, configurações e estrutura de dados (feita)

### OOTDEConfig
- `ootde/config.json`, com `schemaVersion` (hoje 1). Layouts antigos são migrados ao abrir, com cópia do original
  (`config.json.v<N>.bak`). Arquivo ilegível é guardado como `config.json.corrompido-<data>` e os padrões são usados.
  Arquivo de versão mais nova é copiado antes e as chaves desconhecidas são preservadas.
- Cada configuração tem padrão registrado; "restaurar padrões" apaga só as escolhas do jogador.
- Perfis: cópias nomeadas das configurações (de todas ou de um grupo, como `touch.`).
- Gravação atômica (arquivo temporário, `fsync`, `rename`).

### Gerenciador de saves
- Menu do jogo > Definitive > Saves.
- Por slot: estado (vazio, válido, inválido e por quê), nome, corações, mortes, Skulltulas, Randomizer/Master Quest.
- Backup manual; backup automático antes de o jogo gravar por cima (intervalo e quantidade configuráveis); cópia
  de segurança antes de apagar ou copiar por cima na seleção de arquivos.
- Exportar para `ootde/exports/`; importar arquivos `.sav` colocados nessa pasta.
- Restaurar/importar: valida o arquivo, guarda o save atual, grava cópia temporária, valida de novo e troca por
  `rename`. Só fora da partida; depois o jogo volta à tela inicial para recarregar os slots.
- Formato dos saves inalterado.

### Correção no jogo original
`SaveManager::SaveFileThreaded` apagava o save antigo e só depois copiava o novo. Agora o novo é gravado por
completo e renomeado por cima do antigo; se a gravação falhar, o antigo fica.

### Arquivos
Novos: `soh/soh/Enhancements/DefinitiveExpansion/{DEStorage,OOTDEConfig,DESaveManager,DefinitiveExpansion}.{h,cpp}`,
`soh/soh/SohGui/SohMenuDefinitive.cpp`, `scripts/tests/de_expansion_test.cpp`,
`scripts/tests/run_de_expansion_tests.sh`, este documento.
Modificados: `soh/soh/SaveManager.cpp`, `soh/soh/SohGui/SohMenu.{cpp,h}`, `CHANGELOG.md`, versão em
`build.gradle` e `AndroidManifest.xml`.

### Testes feitos
- `scripts/tests/run_de_expansion_tests.sh`: 94 verificações, 0 falhas, com AddressSanitizer e UBSan. Cobre escrita
  atômica, configurações (tipos, perfis, padrões, arquivo corrompido, migração v0 para v1, arquivo mais novo) e
  saves (validação, backups, rotação, restauração com origem inválida e válida, exportação, exclusão).
- Checagem de sintaxe dos arquivos do jogo alterados contra os headers do libultraship no computador.
- Compilação Android: pelo GitHub Actions (ver o PR).

### Ainda não testado
- No aparelho: as telas novas, backup automático durante o jogo, restaurar e importar. Precisa do APK instalado.

## Fase 2 — Touch 2.0, giroscópio e HUD (feita; falta testar no aparelho)

A expansão foi trazida para cima da 0.9 (`main` em `ebfd06e`), que já inclui as correções da 0.8.

### Touch 2.0
- Menu do jogo > Definitive > Touch & Motion > "Editar controles de toque" abre o editor sobre o jogo.
- Arrastar move cada controle (analógico, botões C, A, B, X, Y, L, R, Z, Start, Menu); tamanho individual (50% a
  250%); distância entre A/B/X/Y; transparência; 3 perfis; "Padrão" volta ao layout original; "Cancelar" desfaz.
- Os controles continuam sendo os mesmos botões do overlay, com os mesmos listeners e o mesmo controle virtual do
  SDL: o layout só aplica translação, escala e alfa nas views. Controles Bluetooth e USB não são afetados.
- Layouts em `OOT-DE/ootde/touch_layouts.properties` (versionado; posições em fração da tela, então valem em
  qualquer resolução; arquivo corrompido é guardado à parte). Reaplicado quando a tela muda de tamanho.
- Código: `TouchLayoutStore.java` (dados, testado no computador) e `TouchLayoutEditor.java` (views).
- Orientação: o app é fixo em paisagem (`screenOrientation="landscape"`); o layout se adapta ao tamanho da tela,
  mas não há modo retrato.

### Giroscópio
- O caminho de entrada é o que já existia (`SDLGyroMapping` do libultraship, sensor do aparelho, somado à mira em
  `z_player.c`). Entre os dois entra `DefinitiveExpansion_FilterGyro` (`DEControls.cpp`).
- Opções: ativar/desativar, sensibilidade, calibrar, suavização (0 a 90%), ligar e inverter cada eixo, e em quais
  itens usar (estilingue, arco, gancho, bumerangue, olhar em primeira pessoa). O analógico continua valendo junto.

### HUD
- Definitive > HUD: perfis Clássico, Minimalista, Definitive e Personalizado (salva o HUD atual), e por elemento:
  visível, tipo de posição, X/Y e margens. Escreve os mesmos CVars do Editor de personalização do SoH.
- **Parcial:** tamanho e transparência por elemento não foram feitos; o HUD original não tem essas opções (o
  controle de escala do SoH está desativado no código por estar incompleto).

### Arquivos
Novos: `Android/.../TouchLayoutStore.java`, `Android/.../TouchLayoutEditor.java`,
`soh/soh/Enhancements/DefinitiveExpansion/DEControls.cpp`, `scripts/tests/java/TouchLayoutStoreTest.java`.
Modificados: `MainActivity.java` (cria o editor, mantém os controles visíveis ao editar), `z_player.c` (filtro do
giroscópio), `SohMenuDefinitive.cpp`, `DefinitiveExpansion.{h,cpp}`, `scripts/tests/run_java_tests.sh`.

### Testes feitos
- `scripts/tests/run_java_tests.sh`: testes do `TouchLayoutStore` (salvar, reabrir, limites, arquivo corrompido,
  arquivo de versão mais nova), todos passando.
- Checagem de sintaxe dos arquivos C++ novos contra os headers do libultraship.
- Não testado no computador: `TouchLayoutEditor.java` (precisa do SDK Android; é compilado no GitHub Actions) e o
  comportamento no aparelho.

## Fase 8 — Definitive Features no menu principal (feita)

- Novo botão **Definitive Features** no menu principal Definitive Edition (entre Diário de Quests e Mods), com
  duas telas para manter os botões altos o bastante para o toque: Save Manager, Touch & Motion, HUD Editor, Link
  Customization, 3D Museum, "Mais recursos" e Voltar; depois Hyrule Explorer, Achievements, Boss Rush, Master
  Mode, Randomizer e Voltar.
- Mesmo visual, animações, cartão de descrição e navegação (toque, teclado, controle e B para voltar) dos outros
  botões do menu: é o mesmo código de desenho (`DeTitleMenuView`), só com itens novos.
- Cada botão abre a página correspondente do menu do jogo (cabeçalho "Definitive"). Nenhum botão é decorativo.
  Museu e Explorer explicam que mostram o save quando abertos dentro de um jogo.
- A ferramenta de prévia (`scripts/ootde_ui_preview`) ganhou as telas novas; as imagens geradas por ela estão no
  relatório.
- Arquivos: `DeTitleMenu.cpp`, `DeTitleMenuText.h`, `scripts/ootde_ui_preview/preview.cpp`.

## Fase 7 — Randomizer (feita)

- A auditoria confirmou: o Randomizer completo do Ship of Harkinian já existe (gerador, lógica de acesso,
  configurações de dificuldade, itens e progressão, truques, spoiler log, assinatura de 5 ícones na seleção de
  arquivos, saves separados e identificados). Nada disso foi reescrito; não há algoritmo novo.
- Novo em Definitive > Randomizer: semente pronta (código e assinatura), botão para copiar e compartilhar, campo
  de semente e botão Gerar (chama a mesma função `GenerateRandomizer` da janela de configurações, com as mesmas
  restrições: só na seleção de arquivos, sem save carregado), atalho para as configurações do Randomizer e
  histórico de partidas.
- Reprodutível: mesma semente + mesmas configurações = mesmo jogo; a assinatura confirma. O texto copiado avisa
  que as configurações precisam ser as mesmas.
- Histórico em `ootde/randomizer_history.json` (versionado, gravação atômica): slot, semente, assinatura, datas e
  se a partida foi concluída (com o tempo de jogo), gravado ao carregar um save do Randomizer e ao concluí-lo.
- Arquivo novo: `DERandomizer.cpp`.
- Testes: conferência de sintaxe. Não testado em jogo real.

## Fases 3 e 4 — Museu, aparência do Link, Hyrule Explorer e Álbum (feitas, com o Museu parcial)

### 3D Museum (Definitive > 3D Museum) — parcial

- Catálogo de 49 peças em Armas, Escudos, Equipamentos, Máscaras, Instrumentos e Itens especiais, com nome,
  categoria, descrição, zoom e estado de desbloqueio.
- O desbloqueio é o estado real do save carregado (inventário, equipamentos, melhorias, flags de evento). Item
  não encontrado fica bloqueado e aparece como "???" com uma pista.
- Imagem: os ícones que o próprio port já registra para os menus (`RegisterImGuiItemIcons`); nenhuma textura nova
  ou copiada. Se o arquivo do jogo não tiver o ícone, o quadro diz "sem ícone".
- **Não feito:** modelo 3D com rotação. O port não tem como desenhar um modelo do jogo dentro de uma janela do
  menu; fazer isso exige renderizar o modelo numa textura à parte, o que fica como pendência. A tela diz isso.
- Personagens, chefes e inimigos estão no Álbum (Hyrule Explorer), com os dados reais; sem modelo 3D.

### Link Customization (Definitive > Link Customization)

- Cores de túnica Kokiri, Goron e Zora, cabelo, botas, Mirror Shield, lâminas das três espadas e rastro da Master
  Sword. São opções do Editor de personalização do Ship of Harkinian, mudadas pelas funções do próprio editor
  (`CosmeticsEditor_SetColor/ResetColor`, novas e só aditivas), então o editor e o jogo ficam iguais.
- Prévia antes de aplicar (figura 2D com as cores escolhidas; a prévia 3D real é a tela Equipamento do menu de
  pausa, depois de aplicar), Aplicar, Descartar, Restaurar originais e 3 visuais guardados em `ootde/config.json`.
- Visuais desbloqueáveis: Sombra (conquista Dark Link), Sábio (seis medalhões), Herói Dourado (derrotar Ganon).
- Só cor: hitbox, atributos, dano, velocidade, física e animações não mudam. Nenhuma textura é substituída.

### Hyrule Explorer e Álbum (Definitive > Hyrule Explorer)

- Mapa: as 22 áreas do mapa-múndi do menu de pausa, nas posições e tamanhos que o jogo usa para a caixa "posição
  atual" (`areaBoxPosX/Y` em `z_kaleido_map_PAL.c`). Visitada = o bit da área em `worldMapAreaData`, o mesmo que
  tira a nuvem do mapa de pausa; vale para saves antigos também. Toque numa área para ver o estado.
- Regiões: os 12 pontos do mapa de pausa com áreas visitadas, Gold Skulltulas e Pedaços de Coração, contados pela
  mesma função do painel do mapa (`KaleidoScope_MapCollectRegionCounts`, nova, só lê).
- Álbum: personagens encontrados (19, por flags de conversa e evento), chefes enfrentados e derrotados (flag de
  início de batalha e horário da vitória nas estatísticas), inimigos derrotados (70 tipos, contadores do Ship of
  Harkinian), itens, eventos marcantes e lugares visitados.
- Lugares visitados usam os bits `scenesDiscovered` que o save já tem; só são marcados a partir desta versão.
- Nada é gravado no save pelo Explorer.

### Arquivos

- Novos: `DEMuseum.cpp`, `DEExplorer.cpp`.
- Alterados: `CosmeticsEditor.{h,cpp}` (3 funções novas), `z_kaleido_map_collect.c` e `z_kaleido_scope.h` (1 função
  nova que só lê), `DefinitiveExpansion.{h,cpp}`, `DEAchievements.cpp`, `SohMenuDefinitive.cpp`.

### Testes feitos

- Conferência de sintaxe de `DEMuseum.cpp`, `DEExplorer.cpp` e da integração contra os cabeçalhos do jogo.
  `CosmeticsEditor.cpp` e os arquivos C do menu de pausa dependem de cabeçalhos gerados na compilação: conferidos
  pela compilação do Android.
- Não testado: em jogo real.

## Fases 5 e 6 — Conquistas, recordes do Boss Rush e Master Mode (feitas; falta testar no aparelho)

### Conquistas (Definitive > Conquistas)

- 42 conquistas em 7 categorias (História, Exploração, Combate, Colecionáveis, Masmorras, Desafios, Segredos),
  com descrição, progresso, data de desbloqueio, percentual, histórico e aviso na tela.
- Cada uma é conferida contra o que o próprio jogo registra: inventário e itens de missão do save em memória,
  flags de evento, estatísticas do Ship of Harkinian (chefes derrotados, contadores) ou um evento do jogo
  (inimigo derrotado, ovo chocado, Boss Rush concluído). Abrir uma tela não desbloqueia nada.
- As de campanha só contam em saves normais ou Master Quest dos slots 1 a 3. Randomizer fica de fora (o
  inventário inicial é uma opção) e tem a conquista própria. A demo da tela de título e o save de depuração usam o
  arquivo 0xFF e nunca contam.
- Progresso em `ootde/achievements.json`, versionado, gravação atômica, separado dos saves. O progresso só sobe.
  Um save importado com progresso real desbloqueia as conquistas correspondentes: é o estado do save que vale.
- Ícones: ainda não (o campo existe na definição). Fica como pendência.
- "Lugares visitados": o save já tem um conjunto de bits `scenesDiscovered` nas estatísticas do Ship of Harkinian,
  que o jogo só preenchia no Randomizer de entradas. A expansão marca o bit ao entrar numa cena. Isso não grava o
  save: vai para o disco quando o jogador salva, junto com o resto das estatísticas. O formato não muda.

### Boss Rush (Definitive > Boss Rush)

- O modo é o Boss Rush do Ship of Harkinian, intocado: seleção de chefes, sequência, estado próprio que nunca é
  gravado (arquivo 0xFE/0xFF), então a campanha e o inventário dela não são tocados.
- Novo: ao terminar uma corrida (o jogo marca `gameComplete`), o resultado vai para `ootde/records.json`: tempo,
  mortes, vida restante, pontuação, data e opções usadas. Telas de ranking (por conjunto de chefes e dificuldade),
  histórico e regras.
- Tempo: relógio monotônico (`steady_clock`), somado quadro a quadro só enquanto o cronômetro do Boss Rush está
  andando (a sala entre chefes não conta). Não depende da taxa de quadros; um intervalo maior que 0,5 s entre
  quadros (app em segundo plano) é descartado.
- Dificuldades do ranking, tiradas das opções reais do modo: Normal; Difícil (Hyper Bosses); Sobrevivência (cura:
  nunca); Difícil + Sobrevivência. "Desafio de tempo" não é oferecido: o jogo não tem regra de limite de tempo
  durante a luta.
- Pontuação: 10000 − 5 por segundo − 500 por morte + até 2000 pela vida restante, mínimo 100; Difícil +50%,
  Sobrevivência +25%.

### Master Mode (Definitive > Master Mode)

- Presets Clássico, Herói, Mestre e Lendário, feitos só de modificadores que o jogo já tem (Aprimoramentos >
  Dificuldade): multiplicadores de dano, de queda e de abismo, sem corações, sem itens aleatórios, Hyper Enemies e
  Hyper Bosses. A tela mostra exatamente o que cada preset liga e o que está ligado agora.
- Não cria comportamento novo de inimigo. Nada vai para o save; "Clássico" desliga tudo. Se o jogador tinha uma
  combinação própria, ela é guardada e pode ser restaurada.
- Master Quest continua sendo escolhido ao criar o arquivo (progressão própria no save dele).
- Desafios com resultado registrado: corridas do Boss Rush e conquistas de desafio (história sem morrer, com três
  corações, no Master Quest; Boss Rush sem cura, sem morrer, com Hyper Bosses). Desafios com regra imposta durante
  o jogo (tempo limite, equipamento obrigatório) ainda não existem.

### Arquivos

- Novos: `DEAchievementsCore.{h,cpp}`, `DEAchievements.cpp`, `DERecords.{h,cpp}`, `DEChallenges.cpp`.
- Alterados: `DefinitiveExpansion.{h,cpp}`, `SohMenuDefinitive.cpp`, testes.

### Testes feitos

- Testes de computador (`scripts/tests/run_de_expansion_tests.sh`): conquistas (progresso só sobe, desbloqueio no
  alvo, arquivo corrompido e de versão mais nova, chaves desconhecidas preservadas) e recordes (pontuação,
  ordenação do ranking, histórico, limite de 200 corridas).
- Conferência de sintaxe dos arquivos de integração contra os cabeçalhos do jogo.
- Não testado: em jogo real (depende do APK no aparelho).

## Matriz de validação (preenchida a cada fase)

| Área | Item | Estado |
| --- | --- | --- |
| Saves | Instalação limpa | A testar no aparelho |
| Saves | Atualização da versão anterior | A testar no aparelho |
| Saves | Backup, restauração, importação, exportação | Testes automáticos OK; aparelho a testar |
| Saves | Recuperação após interrupção | Testes automáticos OK (origem inválida, sem arquivo temporário sobrando) |
| Android | Inicialização | A testar no aparelho |
| Android | Touch (editor de layout), giroscópio, Bluetooth | A testar no aparelho |
| Android | Mudança de orientação | Não se aplica: app fixo em paisagem |
| Gameplay | Campanha original | A testar no aparelho (gravação de save alterada) |
| Gameplay | Boss Rush, Master Mode, Randomizer, Conquistas, HUD | Fases 2 a 7 |
| Gráficos | Museu 3D | Fase 3 |

## Próximas fases e dependências
- Versão: publicada como `0.10.0` / `versionCode` 22 (escolha do Rafael, 12/11/2026).

## Riscos
- A gravação de save mudou de "apagar e copiar" para "gravar e renomear". É mais segura, mas é o caminho de todo
  save do jogo e precisa de teste no aparelho antes de ir para a `main`.
- Com "apagar save ao morrer" ligado, o save apagado também vai para os backups (dá para desligar em Definitive >
  Recursos).
