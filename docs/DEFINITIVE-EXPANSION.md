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
- **Fase 8** (menu Definitive): a categoria "Definitive Features" entra no menu principal da branch
  `feature/ootde-menu-definitivo`. Até lá os sistemas ficam no menu do jogo, em "Definitive".
- Versão: `0.9.1-expansao` / `versionCode` 22 é só para o APK de teste instalar por cima das outras builds de teste.
  O número oficial é decidido na hora de juntar à `main`.

## Riscos
- A gravação de save mudou de "apagar e copiar" para "gravar e renomear". É mais segura, mas é o caminho de todo
  save do jogo e precisa de teste no aparelho antes de ir para a `main`.
- Com "apagar save ao morrer" ligado, o save apagado também vai para os backups (dá para desligar em Definitive >
  Recursos).
