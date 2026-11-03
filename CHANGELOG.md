# Changelog do OOT D.E.

Mudanças deste fork (lokodoshape-png/Shipwright-AndroidOot-Android-Definitive-Edition) em relação ao
[Shipwright-Android](https://github.com/Waterdish/Shipwright-Android) (Ship of Harkinian 9.0.2 para Android).
As builds saem na página [Releases](https://github.com/lokodoshape-png/Shipwright-AndroidOot-Android-Definitive-Edition./releases)
como `android-<versão>-build-N`. O APK se instala por cima do anterior porque todas as versões usam o mesmo
identificador (`com.ootde.soh`) e a mesma chave de assinatura.

O jogo não vem com a ROM nem com assets da Nintendo. Você gera o seu `oot.otr` a partir da sua própria cópia.

Como as versões são organizadas está em [docs/VERSOES.md](docs/VERSOES.md).

## Base Ship of Harkinian 9.0.2 (branch `main`)

## [0.4] — Identidade visual, progresso da preparação e correções

### Android
- **Ícone próprio** com Triforce e espada, em fundo verde inspirado em Hyrule.
- **Progresso real ao importar a ROM.** A cópia roda em segundo plano e mostra a porcentagem pelos bytes copiados
  (ou os MB copiados, quando o seletor de arquivos não informa o tamanho).
- **Progresso real ao preparar os arquivos do jogo** (pasta `assets` e `soh.otr`).
- **Arquivos do app atualizados em cada versão nova.** Ao atualizar, o app troca o `soh.otr` e a pasta `assets`
  (usada pelo extrator da ROM) pelos da versão nova. Saves, mods e o `oot.otr` gerado da sua ROM continuam lá.
  Antes ficavam os arquivos da versão antiga, que podem não combinar com o app novo.
- **O `soh.otr` é gravado primeiro num arquivo temporário**, então uma cópia interrompida não deixa um arquivo
  cortado para trás. Se a cópia falhar, o app tenta de novo na próxima abertura.
- **Correção: o app travava ao cancelar a escolha da ROM** ou quando a importação falhava. O extrator ficava
  esperando para sempre; agora ele volta para a pergunta anterior.
- **Correção: saves antigos podiam ser copiados por cima dos atuais.** A migração da pasta antiga
  `Android/data/.../files` agora só acontece quando ainda não existe `SOH/Save`.
- **Correção: o botão de esconder os controles de toque** precisava de dois toques depois de reabrir o app com os
  controles escondidos.
- Mensagens da preparação e da importação em português.
- `versionCode 12` e `versionName 0.4`, iguais no Gradle e no Manifest.

### Menu
- **Listas de opções traduzidas por inteiro.** A largura das listas usa o texto traduzido (antes os nomes em
  português podiam ficar cortados) e as listas que ainda mostravam as opções em inglês agora passam pela tradução.
- Dicas (tooltips) das listas de opções também passam pela tradução.

### Builds
- **Uma build por push na `main`**, publicada em Releases como `android-0.4-build-N`, com as mudanças desta seção
  nas notas. Pull requests só compilam o APK e o deixam como artefato do GitHub Actions.
- A build confere que Gradle e Manifest têm a mesma versão antes de compilar.
- Corrigida a publicação automática, que falhava em todo push desde a 0.3 ("GitHub Releases requires a tag").

### Pendências
- A tradução integral dos menus e de todo o conteúdo do jogo ainda precisa de revisão; esta versão não é
  totalmente traduzida.
- A extração da ROM pelo extrator nativo ainda não mostra porcentagem.

## [0.3] — Melhorias no processo de configuração

### Android
- Buffer de cópia de arquivos ampliado para 256 KB, com menos leituras e escritas durante a preparação.
- Primeira versão do acompanhamento das etapas da configuração.
- `versionCode 11` e `versionName 0.3`.

### Limitações conhecidas
- O indicador mostrava etapas da configuração, não a porcentagem real (resolvido na 0.4).

## [0.2] — Atualização segura de arquivos do usuário

### Android
- O app deixou de apagar `SOH/assets`, `soh.otr`, `oot.otr` e `oot-mq.otr` ao atualizar.
- Atualizações não apagam saves, mods, assets personalizados nem arquivos OTR.
- Chave de assinatura fixa nas builds do GitHub Actions, para cada APK instalar por cima do anterior.
- `versionCode 10` e `versionName 0.2`.

### Menu
- Abas superiores do menu mais compactas para caber o texto em português.

## [0.1] — Primeira versão da edição

### Identidade e Android
- Identificador próprio `com.ootde.soh`, que permite instalar o OOT D.E. ao lado do Ship of Harkinian original.
- Nome no Android: **Ocarina of Time – Definitive Edition**.
- APKs com o nome `ootde_<tipo>_v<versão>.apk`.
- `versionCode 9` e `versionName 0.1`.

### Menu em português do Brasil
- Abas principais e categorias do menu traduzidas.
- Camada central de tradução pt-BR para rótulos de widgets, botões e dicas.
- Tradução das categorias e opções de melhorias, correções, dificuldade, minijogos, modos extras, trapaças e
  ferramentas de desenvolvedor.
- A pesquisa do menu considera os rótulos e as descrições traduzidos.
- Opções das listas de seleção passam pela tradução sem mudar os valores internos usados pelo jogo.
- Identificadores internos do ImGui mantidos em inglês para preservar a compatibilidade.
