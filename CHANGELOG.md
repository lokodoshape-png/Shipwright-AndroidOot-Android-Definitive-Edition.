# Changelog do OOT D.E.

Mudanças deste fork (lokodoshape-png/Shipwright-AndroidOot-Android-Definitive-Edition) em relação ao
[Shipwright-Android](https://github.com/Waterdish/Shipwright-Android) (Ship of Harkinian 9.0.2 para Android).
As builds saem na página [Releases](https://github.com/lokodoshape-png/Shipwright-AndroidOot-Android-Definitive-Edition./releases)
como `android-<versão>-build-N`. O APK se instala por cima do anterior porque todas as versões usam o mesmo
identificador (`com.ootde.soh`) e a mesma chave de assinatura.

O jogo não vem com a ROM nem com assets da Nintendo. Você gera o seu `oot.otr` a partir da sua própria cópia.

Como as versões são organizadas está em [docs/VERSOES.md](docs/VERSOES.md).

## Base Ship of Harkinian 9.0.2 (branch `main`)

## [0.9-menu] — Interface Definitive Edition: menu principal (prévia)

Primeira etapa da nova interface (branch `feature/ootde-menu-definitivo`, ainda fora da `main`).

- **Novo menu principal na tela de título**, no lugar do "PRESS START", feito com a arte da imagem de referência
  (`docs/interface-de/referencia.png`), ampliada 4x com a rede de super-resolução EDSR (`scripts/ootde_ui_art.py`):
  fundo de Hyrule ao entardecer com o castelo, Link com o Escudo Hylian e Navi (com zoom lento), logo com
  "DEFINITIVE EDITION", painel com a moldura dourada e os botões azul e ouro (normal, selecionado e pressionado).
  O texto dos botões é escrito pelo jogo. Navi voa ao lado da opção escolhida, com partículas douradas.
  O fundo cobre a abertura do jogo; o "© Nintendo" fica no rodapé.
- **Menu animado**: abertura em sequência (cenário saindo do escuro, logo crescendo com um clarão, painel e botões
  deslizando um após o outro), raios de sol e vaga-lumes sobre a paisagem, luz passando pelo logo e pelo botão
  escolhido, estrelas piscando no logo, brilhos correndo pela moldura, botão escolhido maior e flutuando, faíscas e
  salto ao confirmar, troca de telas deslizando para o lado certo, descrição e corações entrando, último coração
  batendo e Navi voando do cenário até o cursor. Prévia em vídeo: `scripts/ootde_ui_preview/build.sh ... video`.
- **Todos os botões fazem a função real**, pelo caminho original do jogo:
  - *Continuar* carrega o último arquivo jogado (passa pela seleção de arquivos e confirma sozinho).
  - *Novo Jogo* abre a criação de arquivo no primeiro espaço livre (escolha de missão, nome etc. originais).
  - *Selecionar Arquivo* abre a seleção de arquivos original (copiar e apagar continuam lá).
  - *Diário de Quests* carrega o último arquivo e abre o pause já no Diário, assim que o jogo permite pausar.
  - *Mods* e *Configurações* abrem o menu do port na página certa.
  - *Extras*: mods instalados, informações da versão; Créditos aparece como "em breve".
- Funciona com controle (D-pad/analógico, A, Start, B), teclado (pelo mapeamento do controle) e toque. Os controles
  de toque somem enquanto o menu está na tela. Sons de cursor, confirmação e erro do próprio jogo.
- Dá para voltar ao "PRESS START" original em Configurações > Geral > *Menu principal Definitive Edition*.
- Fontes novas Cinzel e EB Garamond (licença OFL) e texturas geradas por `scripts/ootde_ui_assets.py`
  (`soh/assets/custom/ootde/ui/`), já incluídas no `soh.otr`. `scripts/otr_add_custom.py` agora também adiciona
  fontes.
- `scripts/ootde_ui_preview/`: desenha as telas no computador com o mesmo código do jogo, para conferir o visual.

## [0.7.1] — Diário de Quests corrigido

- **As texturas do Diário não apareciam.** O `soh.otr` que vai dentro do APK não tinha as texturas novas (pedra,
  pergaminho, asas da Triforce, ícones das categorias, marca, cadeado e ampulheta). Sem elas, os painéis ficavam
  transparentes e riscados, e o menu de pausa aparecia por trás (as letras L/R e o "R" no pergaminho). Agora elas estão
  no `soh.otr`, e os painéis têm um fundo opaco por baixo.
- **Acentos:** Á, Ã, Í, Ó, Õ, Ú, Ü, ã, í, ó, õ, ú e ü são desenhados com a letra do jogo e o acento por cima, sem o
  espaço quebrado depois da letra ("Á rvore", "histó ria"). Funciona com ou sem o mod de tradução. As letras ficaram
  um pouco mais próximas.
- **Nomes longos:** "Personagens" e "Secundárias" cabem no botão, e o título do pergaminho não passa por cima da
  Triforce do canto.
- O número das missões começa em 1 (por exemplo #1001).
- Novo `scripts/otr_add_custom.py`: põe no `soh.otr` as texturas de `soh/assets/custom/` que faltam. O build confere
  isso antes de compilar.

## [0.7] — Diário de Quests no pause

- **Diário de Quests no menu de pausa.** Na tela Status de Missão, aperte **C-Cima**: a página vira e o Diário abre
  na tela toda, por cima do menu, com moldura de pedra, a placa com a Triforce alada e o fundo escurecido. **B**,
  C-Cima ou o botão **Voltar** fecham e voltam para o Status. Tudo é desenhado dentro do jogo, com a fonte, os ícones
  e o mapa de Hyrule do próprio jogo, sem janelas flutuantes.
  - **À esquerda**, os seis botões de pedra das categorias (a escolhida brilha em dourado).
  - **No meio**, a lista da categoria, separada em grupos com título e a contagem do grupo. Cada linha mostra o
    estado: caixinha vazia (disponível), ampulheta (em andamento), cadeado (bloqueada, falta a etapa anterior da
    mesma história) ou marcada (concluída).
  - **À direita**, o pergaminho da linha escolhida: descrição, estado, progresso (os seis medalhões na história
    principal, uma barra nas outras), o próximo objetivo e os objetivos em ícones, marcados quando feitos, e o
    pedaço do mapa de Hyrule onde fica, com a cabeça do Link piscando no lugar (nas masmorras aparece a Pedra
    Espiritual ou o Medalhão). Cada missão tem um número próprio no canto do pergaminho.
  - **Embaixo**, uma placa com uma dica da categoria.
- As categorias:
  - **Principais:** a história (Pedras Espirituais, Carta da Zelda, Ocarina do Tempo, Espada Mestra, Sábios,
    Flechas de Luz e Ganondorf), cada etapa com seus objetivos;
  - **Secundárias:** a Loja da Máscara Feliz e o Teatro Deku, as trocas do Ovo de Bolso até a Espada do Biggoron, as
    4 garrafas, os 36 Pedaços de Coração (com onde fica cada um e o que precisa) e as Skulltulas de Ouro por região,
    masmorra e prêmio;
  - **Personagens:** quem o Link já conheceu e ajudou;
  - **Locais:** as regiões de Hyrule (as mesmas nuvens do mapa da pausa) e as masmorras já visitadas;
  - **Itens:** os itens que abrem caminho (estilingue, bombas, magia, gancho, botas, Túnica Goron...);
  - **Registros:** as 12 partituras e os documentos do Link (Carta da Zelda, Carta de Ruto, Cartão Gerudo...).
  Tudo é lido do save na hora; nada é marcado sem uma checagem real do jogo.
- **Controles:**
  - **Controle / teclado** (pelo mapeamento de botões do jogo): **cima / baixo** escolhem a linha (segurando, ela
    anda sozinha); **L / R**, Z, esquerda / direita e C-Esquerda / C-Direita trocam a categoria.
  - **Toque (Android) e mouse:** toque numa categoria, numa linha, nas abas L / R ou em Voltar; arraste a lista para
    rolar, ou toque nas setas; a roda do mouse também rola.
- Um aviso pequeno "C-Cima Missões" no canto de baixo da tela Status de Missão mostra o atalho.
- Os acentos usam as mesmas letras do mod de tradução (SolidOne / Letras). Sem o mod, Ã, Õ, Í, Ó e Ú aparecem
  com outros acentos.
- No randomizador, o C-Cima passa por Status → Diário → colecionáveis do randomizador → Status. Com o teleporte pelas
  canções da pausa ligado, apertar A no Diário não teleporta mais por engano.
- Código: o Diário é o módulo **QuestMenu** (`soh/src/overlays/misc/ovl_kaleido_scope/questmenu/`), separado em
  dados, estado, desenho e interface, com as texturas em `soh/assets/custom/overlays/questmenu/`. Veja
  [docs/QUESTMENU.md](docs/QUESTMENU.md).

## [0.6.1] — Correção do travamento no menu de pausa

- **Correção: o jogo fechava ao pausar.** Os contadores de Skulltulas e Pedaços de Coração do mapa-múndi usavam
  nomes de textura guardados em endereços ímpares, que o desenho do jogo confunde com outro tipo de endereço. Isso
  fazia o app fechar ao abrir o menu de pausa, com ou sem mods.
- Os APKs de teste não são mais publicados nas releases: só as versões finais da main aparecem lá.

## [0.6] — Randomizador em português e aviso do B em todas as cenas

- **Aviso "Segure B para acelerar" em todas as cenas.** Antes ele só aparecia nas cenas roteirizadas. A maioria das
  cenas do jogo é conduzida pelos próprios personagens e objetos com o Link parado (apresentação dos chefes, abrir
  portas, pegar itens, Navi...), e agora essas também mostram o aviso e podem ser aceleradas segurando B.
- **Randomizador traduzido.** As descrições longas das opções, os nomes e explicações dos truques de lógica e os
  nomes dos locais no rastreador de locais (e na lista de locais excluídos) agora aparecem em português. A busca do
  rastreador aceita tanto o nome em português quanto o original.
- **Ícone novo.** O app agora usa a arte OOT D.E. — Definitive Edition como ícone, tanto na instalação quanto na tela
  inicial.
- **Correção: travamento ao abrir o menu de pausa depois de ligar ou desligar um mod pelo menu.** As texturas
  trocadas eram apagadas enquanto o minimapa e o menu de pausa ainda apontavam para elas.

## [0.5] — Menu em português, mods pelo menu e cenas aceleradas

### Menu do Ship
- **Menu traduzido por inteiro.** Nomes das opções, dicas (tooltips), opções das listas e páginas agora passam
  pela tradução pt-BR. Antes as caixas de seleção, o tipo de opção mais comum, continuavam em inglês, e várias
  opções de Configurações só eram traduzidas nas abas. Todo o texto fica em `soh/soh/SohGui/MenuTranslations.cpp`.
- **Abas e barra lateral sem cortes no celular.** As abas de cima usam nomes curtos (Ajustes, Melhorias,
  Randomizador, Dev) e cabem sem a barra de rolagem que cobria o texto. A barra lateral fica larga o bastante para o
  nome mais longo da aba aberta ("Editor de personalização" aparecia cortado dos dois lados).
- **Iluminação estilo 3DS** (Ajustes > Gráficos): filtro de imagem com cores mais vivas, um pouco mais de luz e
  contraste, tom mais quente e brilho suave nas partes claras, com intensidade de 10% a 200%. Vale na hora.
- **Controles de toque voltam sempre que o menu fecha.** Antes eles só voltavam quando o menu era fechado pelo
  botão de menu; fechando pelo X, ficavam escondidos. Agora o app segue o estado real do menu a cada quadro.
- **Menu sob medida para a tela do celular.** O número de colunas agora depende de quanto texto cabe, e não da
  resolução: num celular as opções ficam em uma ou duas colunas largas em vez de três estreitas. Nomes longos de
  caixas de seleção quebram em mais de uma linha, o nome de um controle deslizante sobe para cima dele quando não
  sobra espaço, e as listas de escolha nunca passam da borda. As tabelas de ajustes do randomizador viram seções
  empilhadas quando a tela é estreita.
- **Janelas internas em português.** Editor de cosméticos (inclusive os nomes de cada cor), editor de áudio,
  controles, visualizador de comandos, estatísticas, parciais de tempo, rastreadores, randomizador (nomes das opções,
  valores e grupos), plandomizer, editor de resolução e ferramentas de desenvolvedor. As descrições longas das
  opções do randomizador e os nomes dos locais nos rastreadores continuam em inglês por enquanto.
- **Página de Mods** (Configurações > Modificações): lista os mods da pasta, liga e desliga cada um e instala novos
  pelo botão "Adicionar mod...", que abre o seletor de arquivos do Android. Aceita `.otr`, `.o2r` e `.zip`; os
  `.otr`/`.o2r` de dentro de um `.zip` são extraídos. Um mod desligado ganha o final `.desativado` e não é carregado.
  Ligar, desligar ou instalar um mod vale na hora para texturas e textos (inclusive tradução). Modelos, sons e o resto
  do mod passam a valer quando o jogo é aberto de novo, e a página avisa quando é o caso.
- **Controles de toque somem com o menu aberto** e voltam sozinhos ao fechar, para todo toque ir para o menu.
  O menu fecha com o botão Voltar do Android.

### Jogo
- **Segure B para acelerar as cenas** (Aprimoramentos > Pulos e acelerações > Cenas, ligado por padrão). Depois de
  segurar B por um instante a cena roda 5 vezes mais rápido (ajustável de 2x a 10x), sem pular nada. Vale para as
  cenas do jogo depois da seleção de arquivo, não para a abertura da tela de título, e para quando há uma pergunta
  na tela.
- **Opcional: passar diálogos ao segurar B** (desligado por padrão). Com ele ligado, as caixas de diálogo das cenas
  passam sozinhas enquanto B está segurado. Se "Pular texto" também estiver ligado, as duas telas mostram um aviso
  de incompatibilidade, porque "Pular texto" já faz isso em todos os diálogos.
- **Aviso "Segure B para acelerar"** no canto inferior direito quando a cena começa; enquanto acelera, mostra
  "Acelerando >>". Pode ser desligado na mesma tela.

- **Skulltulas e Pedaços de Coração no mapa** (Aprimoramentos > Qualidade de vida > Menu de pausa, ligado por padrão). No mapa do
  menu de pausa, ao escolher uma região aparece ao lado do ponto dela o ícone da Skulltula dourada e o do Pedaço de
  Coração com a conta, por exemplo `1/4`. A conta vem do save e muda assim que você pega um. Região com tudo pego fica
  com o ponto e os números em dourado e a carinha do Link piscando ao lado. Valem as 56 Skulltulas e os 35 Pedaços de
  Coração fora das masmorras.
- **Correção: letras grudadas na tradução.** O jogo reservava para cada letra a largura da letra original daquela
  posição da fonte. A tradução põe o Á no lugar do Î e o Ó no lugar do Ù, que são mais estreitos, então o Á e o Ó
  ficavam por cima da letra seguinte ("Á" ganhou a largura do A e "Ó" a do O). O "f" da fonte traduzida também é mais
  largo que o original e ganhou um pouco mais de espaço.

### Android
- **Barra de porcentagem ao criar os arquivos do jogo a partir da ROM** (a extração que gera o `oot.otr`). Antes
  aparecia só uma mensagem em inglês e a tela ficava parada; agora mostra quantos grupos de arquivos já foram
  extraídos e avisa quando está gravando o `oot.otr`. A importação da ROM e a preparação dos arquivos já tinham barra
  desde a 0.4.
- **Preparação e criação dos arquivos mais rápidas.** A criação do `oot.otr` copiava os arquivos de extração das 14
  versões da ROM (cerca de 7700 arquivos pequenos) para uma pasta temporária; agora copia só os da sua ROM (cerca de
  550) e usa todos os núcleos do celular menos um (antes, metade). A preparação na primeira abertura e depois de
  atualizar copia os recursos em várias tarefas ao mesmo tempo, sem abrir cada arquivo duas vezes.
- **Pasta própria `OOT-DE`** na raiz do armazenamento, no lugar da `SOH` que era dividida com o Ship of Harkinian
  original. Na primeira abertura da 0.5, o app copia saves, mods, o `oot.otr` e as configurações da pasta `SOH`
  para a `OOT-DE`. A pasta `SOH` não é apagada.
- `versionCode 13` e `versionName 0.5`.

### Builds
- O nome da pasta fica no libultraship (submódulo) e é trocado pelo `patches/libultraship-ootde.patch`, aplicado
  pelo script de build antes de compilar.

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
  `Android/data/.../files` agora só acontece quando ainda não existe a pasta `Save`.
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
