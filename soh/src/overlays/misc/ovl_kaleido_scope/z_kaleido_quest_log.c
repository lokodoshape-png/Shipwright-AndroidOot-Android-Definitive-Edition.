// OOT D.E.: the "Diário de Quests" (quest journal) of the pause menu.
//
// On the Quest Status page, C-Up turns the page and opens the journal over the whole screen (B or C-Up closes it):
// a carved stone frame with the list categories on the left, the lines of the current list with check boxes in the
// middle, and a parchment card on the right with the selected line: what to do, the progress (the six medallions on
// the main quest), the next objective or what the line needs, and the place on the pause world map with Link's head
// blinking on it. The lists, with the progress read from the save every frame:
//   - the main quest (Spiritual Stones, Ocarina of Time, Master Sword, Sages, Ganondorf) and a summary of the others;
//   - the 36 Pieces of Heart, with where they are and what they need (Magic Bean, Hookshot, ...);
//   - the 4 bottles;
//   - the mask quest (Weird Egg to Mask of Truth) and the Deku Theater;
//   - the adult trading sequence (Pocket Egg to Biggoron's Sword);
//   - the Gold Skulltulas per region and dungeon, and the rewards of the House of Skulltula.
// Up/down choose the line; L/R (also Z, left/right and C-Left/C-Right) choose the list.
//
// The texts are in Portuguese and use the font codes of the PT-BR translation mods (SolidOne / Letras), which redraw
// unused accented letters of the game font as Á Ã Í Õ Ó Ú ã í õ ó ú.
#include "z_kaleido_scope.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
#include "textures/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "textures/message_static/message_static.h"
#include "soh_assets.h"
#include "textures/icon_item_field_static/icon_item_field_static.h"
#include "soh/ShipUtils.h"
#include "soh/OTRGlobals.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

// ---------------------------------------------------------------------------------------------------------------------
// Data

typedef enum {
    QL_CHK_NONE,
    QL_CHK_HEART_PIECE,   // index into the world map counter table (z_kaleido_map_collect.c)
    QL_CHK_COLLECT,       // collectible flag of a scene
    QL_CHK_ITEMGETINF,
    QL_CHK_INFTABLE,
    QL_CHK_EVENTCHKINF,
    QL_CHK_POE_POINTS,    // Poe Collector bottle
    QL_CHK_CHILD_TRADE,   // child trading slot holds this item or a later one
    QL_CHK_ADULT_TRADE,   // adult trading slot holds this item or a later one
    QL_CHK_BIGGORON,      // Biggoron's Sword
    QL_CHK_GS_TOKENS,     // at least this many Gold Skulltula tokens
    QL_CHK_QUEST_ITEM,    // quest status item (Spiritual Stones, medallions)
    QL_CHK_ITEM,          // inventory slot of this item holds it
    QL_CHK_MASTER_SWORD,
    QL_CHK_MEDALLIONS,    // all six medallions
} QuestLogCheckType;

typedef enum {
    QL_AGE_ANY,
    QL_AGE_CHILD,
    QL_AGE_ADULT,
} QuestLogAge;

// What a line needs. Drawn as item icons at the right of the line.
#define QL_NEED_BEAN (1 << 0)
#define QL_NEED_HOOKSHOT (1 << 1)
#define QL_NEED_LONGSHOT (1 << 2)
#define QL_NEED_BOMB (1 << 3)
#define QL_NEED_BOW (1 << 4)
#define QL_NEED_IRON_BOOTS (1 << 5)
#define QL_NEED_HOVER_BOOTS (1 << 6)
#define QL_NEED_SILVER_SCALE (1 << 7)
#define QL_NEED_GOLD_SCALE (1 << 8)
#define QL_NEED_LENS (1 << 9)
#define QL_NEED_OCARINA (1 << 10)
#define QL_NEED_BOOMERANG (1 << 11)
#define QL_NEED_ZORA_TUNIC (1 << 12)
#define QL_NEED_BLUE_FIRE (1 << 13)
#define QL_NEED_DEKU_STICK (1 << 14)
#define QL_NEED_NIGHT (1 << 15) // moon: only at night
#define QL_NEED_DAY (1 << 16)   // sun: only by day
#define QL_NEED_COUNT 17

typedef struct {
    const char* name;   // UTF-8
    const char* detail; // UTF-8
    const char* icon;   // 32x32 RGBA32 item icon, or 24x24 when iconIs24
    u8 iconIs24;
    u8 age;
    u32 needs;
    u8 check;
    s16 scene;
    u16 flag;
} QuestLogEntry;

// World map points (the dots of the pause world map), used for the map of the journal card.
typedef enum {
    QL_PT_NONE = -1,
    QL_PT_WASTELAND,
    QL_PT_FORTRESS,
    QL_PT_VALLEY,
    QL_PT_LAKE,
    QL_PT_LON_LON,
    QL_PT_MARKET,
    QL_PT_FIELD,
    QL_PT_DEATH_MOUNTAIN,
    QL_PT_KAKARIKO,
    QL_PT_LOST_WOODS,
    QL_PT_KOKIRI,
    QL_PT_ZORA,
    QL_PT_COUNT,
} QuestLogPoint;

// Centers of the world map dots on gWorldMapImageTex (216x128), from the map page vertices of the game.
static const u8 sQuestLogPointPos[QL_PT_COUNT][2] = {
    { 19, 31 }, { 45, 35 }, { 56, 47 }, { 79, 111 }, { 102, 54 }, { 113, 24 },
    { 126, 55 }, { 136, 15 }, { 147, 32 }, { 170, 61 }, { 186, 71 }, { 201, 37 },
};

static const char* sQuestLogPointNames[QL_PT_COUNT] = {
    "Deserto Assombrado", "Fortaleza Gerudo",   "Vale Gerudo",      "Lago Hylia",
    "Rancho Lon Lon",     "Castelo de Hyrule",  "Campo de Hyrule",  "Montanha da Morte",
    "Vila Kakariko",      "Floresta Perdida",   "Floresta Kokiri",  "Domínio Zora",
};

// A titled group of lines, starting at line `start` of a table.
typedef struct {
    s16 start;
    u8 style; // 0: golden, with the winged Triforce; 1: blue, with the "!"
    const char* title;
} QuestLogSection;

static const QuestLogEntry sQuestLogMain[] = {
    { "Pedra Espiritual da Floresta", "Entre na Árvore Deku, na Floresta Kokiri, e derrote a Rainha Gohma. A Árvore Deku entrega a Esmeralda Kokiri.", gQuestIconKokiriEmeraldTex, 1, QL_AGE_CHILD, 0, QL_CHK_QUEST_ITEM, 0, QUEST_KOKIRI_EMERALD },
    { "Carta da Zelda", "Entre escondido no jardim do Castelo de Hyrule e fale com a Princesa Zelda. Depois Impa ensina a Canção de Ninar da Zelda.", gItemIconZeldasLetterTex, 0, QL_AGE_CHILD, 0, QL_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER },
    { "Pedra Espiritual do Fogo", "Na Montanha da Morte, abra a Caverna Dodongo e derrote o Rei Dodongo. Darunia entrega o Rubi Goron.", gQuestIconGoronRubyTex, 1, QL_AGE_CHILD, QL_NEED_BOMB, QL_CHK_QUEST_ITEM, 0, QUEST_GORON_RUBY },
    { "Pedra Espiritual da Água", "Leve um peixe numa garrafa ao Lorde Jabu-Jabu, na Fonte Zora, e proteja a Princesa Ruto lá dentro. Ela entrega a Safira Zora.", gQuestIconZoraSapphireTex, 1, QL_AGE_CHILD, 0, QL_CHK_QUEST_ITEM, 0, QUEST_ZORA_SAPPHIRE },
    { "Ocarina do Tempo", "Com as três Pedras, volte ao Castelo de Hyrule. Zelda foge com Impa e joga a Ocarina do Tempo no fosso, na frente da ponte.", gItemIconOcarinaOfTimeTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEM, 0, ITEM_OCARINA_TIME },
    { "Espada Mestra", "No Templo do Tempo, toque a Canção do Tempo diante do altar e puxe a Espada Mestra. Link acorda sete anos depois.", gItemIconSwordMasterTex, 0, QL_AGE_ANY, QL_NEED_OCARINA, QL_CHK_MASTER_SWORD, 0, 0 },
    { "Desperte os Sábios", "Purifique os templos e desperte os Sábios. Com o Medalhão da Luz são seis medalhões para selar Ganondorf.", gQuestIconMedallionLightTex, 1, QL_AGE_ADULT, 0, QL_CHK_MEDALLIONS, 0, 0 },
    { "Flechas de Luz", "Com os seis medalhões, volte ao Templo do Tempo. Sheik revela quem é e entrega as Flechas de Luz.", gItemIconArrowLightTex, 0, QL_AGE_ADULT, 0, QL_CHK_ITEM, 0, ITEM_ARROW_LIGHT },
    { "Derrote Ganondorf", "Atravesse o Castelo de Ganon, desfaça as seis barreiras dos Sábios e derrote Ganondorf no alto da torre.", gQuestIconDungeonBossKeyTex, 1, QL_AGE_ADULT, QL_NEED_BOW, QL_CHK_NONE, 0, 0 },
};
static const s8 sQuestLogMainPoints[] = {
    QL_PT_KOKIRI, QL_PT_MARKET, QL_PT_DEATH_MOUNTAIN, QL_PT_ZORA, QL_PT_MARKET,
    QL_PT_MARKET, QL_PT_NONE /* next temple */, QL_PT_MARKET, QL_PT_MARKET,
};

static const QuestLogEntry sQuestLogHearts[] = {
    // Same order as the Piece of Heart table of the world map counter.
    { "Colosso do Deserto", "Plante um Feijão Mágico no canteiro perto da Fonte da Grande Fada (criança). Volte adulto e suba na folha voadora até o arco de pedra.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_BEAN, QL_CHK_HEART_PIECE, 0, 0 },
    { "Fortaleza Gerudo: telhado", "Baú no ponto mais alto da Fortaleza Gerudo. Suba pelos telhados usando o Gancho.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_HOOKSHOT, QL_CHK_HEART_PIECE, 0, 1 },
    { "Fortaleza Gerudo: tiro a cavalo", "Com Epona e o Cartão Gerudo, faça 1000 pontos no tiro com arco a cavalo. Com 1500 pontos você ganha a aljava maior.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_BOW, QL_CHK_HEART_PIECE, 0, 2 },
    { "Vale Gerudo: cachoeira", "Numa saliência ao lado da cachoeira, abaixo da ponte. Desça pela beira do penhasco ou plane até lá com um Cuco.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 3 },
    { "Vale Gerudo: caixote", "No fundo do vale, perto do rio, dentro de um caixote. Desça até a margem do rio e quebre o caixote rolando nele.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 4 },
    { "Laboratório do Lago", "Com as Botas de Ferro, afunde até o fundo do tanque do laboratório do Lago Hylia. Depois fale com o cientista.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_IRON_BOOTS, QL_CHK_HEART_PIECE, 0, 5 },
    { "Lagoa de Pesca", "Lago Hylia, Lagoa de Pesca: como criança, pesque um peixe de 10 libras ou mais e mostre ao dono.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, 0, QL_CHK_HEART_PIECE, 0, 6 },
    { "Lago Hylia: topo do laboratório", "Plante um Feijão Mágico no canteiro ao lado do laboratório (criança). Volte adulto e suba na folha até o telhado.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_BEAN, QL_CHK_HEART_PIECE, 0, 7 },
    { "Rancho Lon Lon: silo", "Dentro do silo do Rancho Lon Lon, empurre as caixas e entre pela passagem estreita. Só a criança cabe.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, 0, QL_CHK_HEART_PIECE, 0, 8 },
    { "Mercado: boliche Bombchu", "Boliche Bombchu do Mercado (precisa da Bolsa de Bombas). O prêmio muda a cada partida: jogue até sair o Pedaço de Coração.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_BOMB, QL_CHK_HEART_PIECE, 0, 9 },
    { "Mercado: cachorro perdido", "No Mercado, à noite: ache o cachorro Richard (o que segue você) e leve-o à moça da casa no beco.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_NIGHT, QL_CHK_HEART_PIECE, 0, 10 },
    { "Mercado: jogo dos baús", "No Mercado, à noite: vença o jogo dos baús. Com a Lente da Verdade você enxerga o baú certo.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_NIGHT | QL_NEED_LENS, QL_CHK_HEART_PIECE, 0, 11 },
    { "Campo: gruta das Tektites", "Gruta escondida do Campo de Hyrule, aberta com bomba. O pedaço está no fundo da água: mergulhe com a Escama de Ouro ou desça com as Botas de Ferro.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, QL_NEED_BOMB | QL_NEED_GOLD_SCALE | QL_NEED_IRON_BOOTS, QL_CHK_HEART_PIECE, 0, 12 },
    { "Campo: Deku da gruta", "Gruta dentro do cercado de árvores perto da entrada do Lago Hylia (abra com bomba). Compre o pedaço do Deku por 10 rupias.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, QL_NEED_BOMB, QL_CHK_HEART_PIECE, 0, 13 },
    { "Trilha da Montanha da Morte", "Na Trilha da Montanha da Morte, no alto da parede acima da entrada da Caverna Dodongo.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 14 },
    { "Cidade Goron: vaso que gira", "No andar de baixo da Cidade Goron, jogue uma bomba acesa no vaso que gira quando ele mostrar a cara sorrindo.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, QL_NEED_BOMB, QL_CHK_HEART_PIECE, 0, 15 },
    { "Cratera: parede", "Entrando na Cratera pela trilha, desça pela parede da esquerda até a saliência onde está o pedaço.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, 0, QL_CHK_HEART_PIECE, 0, 16 },
    { "Cratera: vulcão", "No alto do pilar no meio da Cratera. Plante um Feijão Mágico no canteiro perto do teleporte (criança) e volte adulto, ou use as Botas Flutuantes.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_BEAN | QL_NEED_HOVER_BOOTS, QL_CHK_HEART_PIECE, 0, 17 },
    { "50 Skulltulas de Ouro", "Casa das Skulltulas, em Kakariko: entregue 50 fichas de Skulltula de Ouro.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 18 },
    { "Kakariko: homem do telhado", "Suba no telhado da casa da Impa e fale com o homem lá em cima. Criança: pule da torre de vigia. Adulto: use o Gancho.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 19 },
    { "Kakariko: vaca da casa da Impa", "No curral da vaca, no fundo da casa da Impa. Entre pela abertura no alto, nos fundos da casa, pulando do telhado.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 20 },
    { "Kakariko: moinho", "No alto, dentro do moinho. Suba nas plataformas que giram e pule até a saliência. Com o Bumerangue dá para pegar de longe.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, QL_NEED_BOOMERANG, QL_CHK_HEART_PIECE, 0, 21 },
    { "Cemitério: túmulo dos ReDeads", "Puxe a lápide do túmulo dos ReDeads e desça. Lá dentro, toque a Canção do Sol diante da parede para o baú aparecer.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, QL_NEED_OCARINA, QL_CHK_HEART_PIECE, 0, 22 },
    { "Cemitério: no muro", "Numa reentrância alta no muro do cemitério. Plante um Feijão Mágico no canteiro abaixo dela (criança) e volte adulto, ou use o Gancho Longo.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_BEAN | QL_NEED_LONGSHOT, QL_CHK_HEART_PIECE, 0, 23 },
    { "Cemitério: corrida do Dampé", "No túmulo do Dampé (adulto), chegue ao fim da corrida contra o fantasma dele em menos de 1 minuto.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, 0, QL_CHK_HEART_PIECE, 0, 24 },
    { "Cemitério: escavação do Dampé", "No cemitério, à noite (criança), pague o Dampé para cavar. O pedaço sai ao acaso: tente várias vezes.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_NIGHT, QL_CHK_HEART_PIECE, 0, 25 },
    { "Floresta Perdida: Skull Kid", "Perto da entrada da Floresta Perdida, toque a Canção de Saria para o Skull Kid em cima do toco.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_OCARINA, QL_CHK_HEART_PIECE, 0, 26 },
    { "Floresta Perdida: jogo musical", "Na clareira dos Skull Kids músicos, repita as melodias que eles tocam até o fim do jogo.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_OCARINA, QL_CHK_HEART_PIECE, 0, 27 },
    { "Rio Zora: rãs (Tempestades)", "No Rio Zora, no tronco das rãs, toque a Canção das Tempestades para elas (criança).", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_OCARINA, QL_CHK_HEART_PIECE, 0, 28 },
    { "Rio Zora: jogo das rãs", "Depois de tocar as canções para as rãs (criança), vença o jogo delas tocando as notas que aparecem.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_OCARINA, QL_CHK_HEART_PIECE, 0, 29 },
    { "Rio Zora: pilar", "No alto de um pilar perto da gruta aberta, no começo do Rio Zora. Suba pelas pedras ou plane até lá com um Cuco.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 30 },
    { "Rio Zora: perto da cachoeira", "Na saliência perto da cachoeira que leva ao Domínio Zora. Pule pelas pedras do rio ou plane até lá com um Cuco.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_HEART_PIECE, 0, 31 },
    { "Domínio Zora: tochas", "Acenda as tochas do Domínio Zora com um Bastão Deku aceso, uma atrás da outra, antes que apaguem. O baú aparece no alto.", gQuestIconHeartPieceTex, 1, QL_AGE_CHILD, QL_NEED_DEKU_STICK, QL_CHK_HEART_PIECE, 0, 32 },
    { "Fonte Zora: iceberg", "Na Fonte Zora congelada (adulto), pule pelos blocos de gelo até o iceberg mais distante.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, 0, QL_CHK_HEART_PIECE, 0, 33 },
    { "Fonte Zora: fundo", "No fundo da Fonte Zora (adulto). Desça com as Botas de Ferro; a Túnica Zora deixa respirar embaixo d'água.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_IRON_BOOTS | QL_NEED_ZORA_TUNIC, QL_CHK_HEART_PIECE, 0, 34 },
    { "Caverna de Gelo", "Dentro da Caverna de Gelo (adulto). Derreta o gelo vermelho com Fogo Azul guardado numa garrafa.", gQuestIconHeartPieceTex, 1, QL_AGE_ADULT, QL_NEED_BLUE_FIRE, QL_CHK_COLLECT, SCENE_ICE_CAVERN, 0x01 },
};

static const QuestLogEntry sQuestLogBottles[] = {
    { "Rancho Lon Lon: cucos do Talon", "Depois de acordar o Talon no castelo, vença o jogo dos Super Cucos na casa do rancho (criança). Vem com Leite Lon Lon.", gItemIconBottleMilkFullTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_TALON_BOTTLE },
    { "Kakariko: cucos da Anju", "Junte os 7 cucos da Anju no cercado dela, na entrada de Kakariko (criança). Um deles está atrás da cerca: plane com outro cuco.", gItemIconBottleEmptyTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, 0x0C },
    { "Carta de Ruto", "No fundo do Lago Hylia (criança), perto da margem. Precisa da Escama de Prata do jogo de mergulho do Domínio Zora. Mostre ao Rei Zora e a garrafa fica com você.", gItemIconBottleRutosLetterTex, 0, QL_AGE_CHILD, QL_NEED_SILVER_SCALE, QL_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_RUTOS_LETTER },
    { "Coletor de Poes", "Com Epona e o Arco, cace 10 Grandes Poes no Campo de Hyrule e venda ao homem da casa do guarda, no Mercado em ruínas (adulto).", gItemIconBottleBigPoeTex, 0, QL_AGE_ADULT, QL_NEED_BOW, QL_CHK_POE_POINTS, 0, 0 },
};

static const QuestLogEntry sQuestLogMasks[] = {
    { "Ovo Estranho", "Fale com a Malon na frente do Castelo de Hyrule. O ovo choca com o tempo e vira um Cuco.", gItemIconWeirdEggTex, 0, QL_AGE_CHILD, 0, QL_CHK_CHILD_TRADE, 0, ITEM_WEIRD_EGG },
    { "Acordar o Talon", "Use o Cuco no Talon, que dorme ao lado das caixas perto da entrada do castelo.", gItemIconChickenTex, 0, QL_AGE_CHILD, 0, QL_CHK_EVENTCHKINF, 0, EVENTCHKINF_TALON_WOKEN_IN_CASTLE },
    { "Carta de Zelda", "Entre escondido no jardim do castelo e fale com a Zelda.", gItemIconZeldasLetterTex, 0, QL_AGE_CHILD, 0, QL_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER },
    { "Abrir a Loja de Máscaras", "Mostre a Carta de Zelda ao guarda do portão da Montanha da Morte, em Kakariko. Aí a Loja da Máscara Feliz, no Mercado, abre.", gItemIconZeldasLetterTex, 0, QL_AGE_CHILD, 0, QL_CHK_INFTABLE, 0, INFTABLE_SHOWED_ZELDAS_LETTER_TO_GATE_GUARD },
    { "Máscara Keaton", "Pegue a Máscara Keaton emprestada na loja e venda ao guarda do portão da Montanha da Morte. Depois pague a loja.", gItemIconMaskKeatonTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_38 },
    { "Máscara de Caveira", "Venda ao Skull Kid do toco perto da entrada da Floresta Perdida. Depois pague a loja.", gItemIconMaskSkullTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_39 },
    { "Máscara Assustadora", "Venda ao menino do Cemitério de Kakariko, de dia. Ele quer ser igual ao Dampé. Depois pague a loja.", gItemIconMaskSpookyTex, 0, QL_AGE_CHILD, QL_NEED_DAY, QL_CHK_ITEMGETINF, 0, ITEMGETINF_3A },
    { "Capuz de Coelho", "Venda ao Corredor, o homem que vive correndo pelo Campo de Hyrule. Ele paga com a carteira inteira. Depois pague a loja.", gItemIconMaskBunnyHoodTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_3B },
    { "Máscara da Verdade", "Depois de vender as quatro, a loja empresta a Máscara da Verdade. Com ela as Pedras Fofoqueiras falam. As máscaras Goron, Zora e Gerudo também ficam disponíveis.", gItemIconMaskTruthTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_3F },
    { "Teatro Deku: Caveira", "No Teatro Deku (gruta da Floresta Perdida, perto do Prado Sagrado), use a Máscara de Caveira. Prêmio: carregar mais Bastões Deku.", gItemIconMaskSkullTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_OBTAINED_STICK_UPGRADE_FROM_STAGE },
    { "Teatro Deku: Verdade", "No Teatro Deku, use a Máscara da Verdade. Prêmio: carregar mais Nozes Deku.", gItemIconMaskTruthTex, 0, QL_AGE_CHILD, 0, QL_CHK_ITEMGETINF, 0, ITEMGETINF_OBTAINED_NUT_UPGRADE_FROM_STAGE },
};

static const QuestLogEntry sQuestLogTrade[] = {
    { "Ovo de Bolso", "Fale com a Anju (adulto), na frente do cercado dos cucos em Kakariko.", gItemIconPocketEggTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_POCKET_EGG },
    { "Cuco de Bolso", "O ovo choca depois de uma noite. Use o cuco para acordar o Talon, que dorme na casa da Impa.", gItemIconPocketCuccoTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_POCKET_CUCCO },
    { "Cojiro", "Depois de acordar o Talon, dê o cuco à Anju e ganhe o Cojiro, o cuco azul.", gItemIconCojiroTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_COJIRO },
    { "Cogumelo Estranho", "Na Floresta Perdida, mostre o Cojiro ao rapaz sentado perto do caminho do Prado.", gItemIconOddMushroomTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_ODD_MUSHROOM },
    { "Poção Estranha", "Leve o cogumelo à velha da loja de poções nos fundos de Kakariko. Ele estraga se demorar!", gItemIconOddPotionTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_ODD_POTION },
    { "Serrote", "Volte à Floresta Perdida e dê a poção à garota Kokiri que está no lugar do rapaz.", gItemIconPoachersSawTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_SAW },
    { "Espada Goron Quebrada", "Dê o serrote ao chefe dos carpinteiros, perto da tenda no Vale Gerudo.", gItemIconBrokenGoronsSwordTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_SWORD_BROKEN },
    { "Receita", "Leve a espada ao Biggoron, no topo da Montanha da Morte.", gItemIconPrescriptionTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_PRESCRIPTION },
    { "Sapo Olhudo", "Mostre a receita ao Rei Zora, depois de descongelá-lo com Fogo Azul.", gItemIconEyeballFrogTex, 0, QL_AGE_ADULT, QL_NEED_BLUE_FIRE, QL_CHK_ADULT_TRADE, 0, ITEM_FROG },
    { "Colírio", "Leve o sapo ao cientista do laboratório do Lago Hylia, antes do tempo acabar.", gItemIconEyeDropsTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_EYEDROPS },
    { "Recibo", "Corra até o Biggoron com o colírio antes do tempo acabar. Use a ocarina para viajar rápido.", gItemIconClaimCheckTex, 0, QL_AGE_ADULT, 0, QL_CHK_ADULT_TRADE, 0, ITEM_CLAIM_CHECK },
    { "Espada do Biggoron", "Mostre o recibo ao Biggoron. A espada fica pronta depois de alguns dias (durma ou toque a Canção do Sol).", gItemIconSwordBiggoronTex, 0, QL_AGE_ADULT, 0, QL_CHK_BIGGORON, 0, 0 },
};

typedef struct {
    const char* name;
    const char* icon; // 24x24 quest icon or 32x32 item icon (see iconIs24)
    u8 iconIs24;
    s8 point;    // world map point, or -1 for a dungeon
    s8 dungeon;  // index into the Gold Skulltula flags of the dungeons
    s8 total;
} QuestLogSkullArea;

static const QuestLogSkullArea sQuestLogSkullAreas[] = {
    { "Floresta Kokiri", gQuestIconGoldSkulltulaTex, 1, 10, 0, 0 },
    { "Floresta Perdida e Prado", gQuestIconGoldSkulltulaTex, 1, 9, 0, 0 },
    { "Campo de Hyrule", gQuestIconGoldSkulltulaTex, 1, 6, 0, 0 },
    { "Mercado e Castelo", gQuestIconGoldSkulltulaTex, 1, 5, 0, 0 },
    { "Rancho Lon Lon", gQuestIconGoldSkulltulaTex, 1, 4, 0, 0 },
    { "Kakariko e Cemitério", gQuestIconGoldSkulltulaTex, 1, 8, 0, 0 },
    { "Montanha da Morte", gQuestIconGoldSkulltulaTex, 1, 7, 0, 0 },
    { "Rio, Domínio e Fonte Zora", gQuestIconGoldSkulltulaTex, 1, 11, 0, 0 },
    { "Lago Hylia", gQuestIconGoldSkulltulaTex, 1, 3, 0, 0 },
    { "Vale Gerudo", gQuestIconGoldSkulltulaTex, 1, 2, 0, 0 },
    { "Fortaleza Gerudo", gQuestIconGoldSkulltulaTex, 1, 1, 0, 0 },
    { "Deserto e Colosso", gQuestIconGoldSkulltulaTex, 1, 0, 0, 0 },
    { "Árvore Deku", gQuestIconKokiriEmeraldTex, 1, -1, 0, 4 },
    { "Caverna Dodongo", gQuestIconGoronRubyTex, 1, -1, 1, 4 },
    { "Barriga do Jabu-Jabu", gQuestIconZoraSapphireTex, 1, -1, 2, 4 },
    { "Templo da Floresta", gQuestIconMedallionForestTex, 1, -1, 3, 5 },
    { "Templo do Fogo", gQuestIconMedallionFireTex, 1, -1, 4, 5 },
    { "Templo da Água", gQuestIconMedallionWaterTex, 1, -1, 5, 5 },
    { "Templo do Espírito", gQuestIconMedallionSpiritTex, 1, -1, 6, 5 },
    { "Templo das Sombras", gQuestIconMedallionShadowTex, 1, -1, 7, 5 },
    { "Fundo do Poço", gItemIconLensOfTruthTex, 0, -1, 8, 3 },
    { "Caverna de Gelo", gItemIconBootsIronTex, 0, -1, 9, 3 },
};

static const QuestLogEntry sQuestLogSkullRewards[] = {
    { "10 fichas: Carteira de Adulto", "Casa das Skulltulas, em Kakariko. A carteira guarda até 200 rupias.", gItemIconAdultsWalletTex, 0, QL_AGE_ANY, 0, QL_CHK_EVENTCHKINF, 0, 0xDA },
    { "20 fichas: Pedra da Agonia", "A pedra treme o controle perto de grutas e segredos escondidos.", gQuestIconStoneOfAgonyTex, 1, QL_AGE_ANY, 0, QL_CHK_EVENTCHKINF, 0, 0xDB },
    { "30 fichas: Carteira de Gigante", "A carteira guarda até 500 rupias.", gItemIconGiantsWalletTex, 0, QL_AGE_ANY, 0, QL_CHK_EVENTCHKINF, 0, 0xDC },
    { "40 fichas: Bombchus", "Prêmio de 10 Bombchus.", gItemIconBombchuTex, 0, QL_AGE_ANY, 0, QL_CHK_EVENTCHKINF, 0, 0xDD },
    { "50 fichas: Pedaço de Coração", "Prêmio de um Pedaço de Coração.", gQuestIconHeartPieceTex, 1, QL_AGE_ANY, 0, QL_CHK_EVENTCHKINF, 0, 0xDE },
    { "100 fichas: Rupia Gigante", "A família volta ao normal. Depois disso, cada visita dá uma Rupia Gigante (200 rupias).", gQuestIconGoldSkulltulaTex, 1, QL_AGE_ANY, 0, QL_CHK_GS_TOKENS, 0, 100 },
};

static const s8 sQuestLogHeartPoints[] = {
    QL_PT_WASTELAND, QL_PT_FORTRESS, QL_PT_FORTRESS, QL_PT_VALLEY, QL_PT_VALLEY, QL_PT_LAKE, QL_PT_LAKE, QL_PT_LAKE,
    QL_PT_LON_LON, QL_PT_MARKET, QL_PT_MARKET, QL_PT_MARKET, QL_PT_FIELD, QL_PT_FIELD, QL_PT_DEATH_MOUNTAIN,
    QL_PT_DEATH_MOUNTAIN, QL_PT_DEATH_MOUNTAIN, QL_PT_DEATH_MOUNTAIN, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_KAKARIKO,
    QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_LOST_WOODS, QL_PT_LOST_WOODS,
    QL_PT_ZORA, QL_PT_ZORA, QL_PT_ZORA, QL_PT_ZORA, QL_PT_ZORA, QL_PT_ZORA, QL_PT_ZORA, QL_PT_ZORA,
};
static const s8 sQuestLogBottlePoints[] = { QL_PT_LON_LON, QL_PT_KAKARIKO, QL_PT_LAKE, QL_PT_MARKET };
static const s8 sQuestLogMaskPoints[] = {
    QL_PT_MARKET, QL_PT_MARKET, QL_PT_MARKET, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_LOST_WOODS,
    QL_PT_KAKARIKO, QL_PT_FIELD, QL_PT_MARKET, QL_PT_LOST_WOODS, QL_PT_LOST_WOODS,
};
static const s8 sQuestLogTradePoints[] = {
    QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_LOST_WOODS, QL_PT_KAKARIKO, QL_PT_LOST_WOODS,
    QL_PT_VALLEY, QL_PT_DEATH_MOUNTAIN, QL_PT_ZORA, QL_PT_LAKE, QL_PT_DEATH_MOUNTAIN, QL_PT_DEATH_MOUNTAIN,
};
// World map point of each dungeon, in the order of the Gold Skulltula flags.
static const s8 sQuestLogDungeonPoints[] = {
    QL_PT_KOKIRI, QL_PT_DEATH_MOUNTAIN, QL_PT_ZORA, QL_PT_LOST_WOODS, QL_PT_DEATH_MOUNTAIN,
    QL_PT_LAKE, QL_PT_WASTELAND, QL_PT_KAKARIKO, QL_PT_KAKARIKO, QL_PT_ZORA,
};

static const QuestLogSection sQuestLogMainSections[] = { { 0, 0, "Missões Principais" }, { 9, 1, "Missões Secundárias" }, { -1 } };
static const QuestLogSection sQuestLogHeartSections[] = {
    { 0, 0, "Deserto e Gerudo" }, { 5, 0, "Lago Hylia" }, { 8, 0, "Rancho e Mercado" }, { 12, 0, "Campo de Hyrule" },
    { 14, 0, "Montanha da Morte" }, { 18, 0, "Kakariko e Cemitério" }, { 26, 0, "Floresta Perdida" },
    { 28, 0, "Rio e Domínio Zora" }, { 35, 0, "Caverna de Gelo" }, { -1 },
};
static const QuestLogSection sQuestLogBottleSections[] = { { 0, 0, "Garrafas" }, { -1 } };
static const QuestLogSection sQuestLogMaskSections[] = {
    { 0, 0, "Abrir a Loja de Máscaras" }, { 4, 0, "Loja da Máscara Feliz" }, { 9, 1, "Teatro Deku" }, { -1 },
};
static const QuestLogSection sQuestLogTradeSections[] = { { 0, 0, "Troca de Adulto" }, { -1 } };
static const QuestLogSection sQuestLogSkullSections[] = {
    { 0, 0, "Regiões" }, { 12, 0, "Masmorras" }, { 22, 1, "Casa das Skulltulas" }, { -1 },
};

// The tables that run in parallel must have the same length.
typedef char QuestLogCheckMain[(ARRAY_COUNT(sQuestLogMainPoints) == ARRAY_COUNT(sQuestLogMain)) ? 1 : -1];
typedef char QuestLogCheckHearts[(ARRAY_COUNT(sQuestLogHeartPoints) == ARRAY_COUNT(sQuestLogHearts)) ? 1 : -1];
typedef char QuestLogCheckBottles[(ARRAY_COUNT(sQuestLogBottlePoints) == ARRAY_COUNT(sQuestLogBottles)) ? 1 : -1];
typedef char QuestLogCheckMasks[(ARRAY_COUNT(sQuestLogMaskPoints) == ARRAY_COUNT(sQuestLogMasks)) ? 1 : -1];
typedef char QuestLogCheckTrade[(ARRAY_COUNT(sQuestLogTradePoints) == ARRAY_COUNT(sQuestLogTrade)) ? 1 : -1];

#define QL_SKULL_AREA_DETAIL "Skulltulas de Ouro desta região. Muitas só aparecem à noite: siga o barulho delas. Use o Bumerangue ou o Gancho para pegar as fichas de longe."
#define QL_SKULL_DUNGEON_DETAIL "Skulltulas de Ouro dentro desta masmorra. Procure atrás de paredes que racham, no alto e em salas escondidas."

typedef enum {
    QL_CAT_MAIN,
    QL_CAT_HEARTS,
    QL_CAT_BOTTLES,
    QL_CAT_MASKS,
    QL_CAT_TRADE,
    QL_CAT_SKULLS,
    QL_CAT_COUNT,
} QuestLogCategory;

static const char* sQuestLogCategoryNames[QL_CAT_COUNT] = {
    "Missões", "Corações", "Garrafas", "Máscaras", "Trocas", "Skulltulas",
};

// Text of the plaque at the bottom of the journal.
static const char* sQuestLogCategoryHints[QL_CAT_COUNT] = {
    "Acompanhe o progresso das missões.",
    "4 Pedaços de Coração formam 1 Coração.",
    "Garrafas guardam fadas, poções e peixes.",
    "Venda as máscaras para liberar a próxima.",
    "Cada troca leva à Espada de Biggoron.",
    "Troque as fichas na Casa das Skulltulas.",
};

// One line of the current list, built every frame from the tables above.
typedef struct {
    const char* name;
    const char* detail;
    const char* icon;
    u8 iconIs24;
    u8 age;
    u32 needs;
    u8 done;
    u8 hasCount;
    s16 got;
    s16 total;
    s8 point;             // world map point, or QL_PT_NONE
    u8 sectionStyle;
    const char* section;  // title of the group the line belongs to
} QuestLogRow;

#define QL_MAX_ROWS 40

static QuestLogRow sQuestLogRows[QL_MAX_ROWS];
static s32 sQuestLogCategory = QL_CAT_MAIN;
static s32 sQuestLogCursor[QL_CAT_COUNT];
static s32 sQuestLogTop[QL_CAT_COUNT];
static s32 sQuestLogFrames = 0;
static s16 sQuestLogSavedCursorItem = PAUSE_ITEM_NONE;
static s8 sQuestLogHeldX = 0;
static s8 sQuestLogHeldY = 0;
static s16 sQuestLogRepeat = 0;

// Page flip when C-Up changes the mode: the page narrows to its edge, the mode changes, and it opens again.
#define QL_FLIP_FRAMES 12
static s16 sQuestLogFlipTimer = 0;
static u8 sQuestLogFlipTarget = PAUSE_QUEST_MODE_STATUS;

// ---------------------------------------------------------------------------------------------------------------------
// Save checks

static bool QuestLog_SceneCollect(PlayState* play, s16 scene, u16 flag) {
    if (scene == play->sceneNum) {
        return Flags_GetCollectible(play, flag);
    }
    return (gSaveContext.sceneFlags[scene].collect & (1 << flag)) != 0;
}

static bool QuestLog_IsDone(PlayState* play, const QuestLogEntry* e) {
    u8 item;

    switch (e->check) {
        case QL_CHK_HEART_PIECE:
            return KaleidoScope_MapCollectHeartPieceTaken(play, e->flag);
        case QL_CHK_COLLECT:
            return QuestLog_SceneCollect(play, e->scene, e->flag);
        case QL_CHK_ITEMGETINF:
            return GET_ITEMGETINF(e->flag) != 0;
        case QL_CHK_INFTABLE:
            return GET_INFTABLE(e->flag) != 0;
        case QL_CHK_EVENTCHKINF:
            return GET_EVENTCHKINF(e->flag) != 0;
        case QL_CHK_POE_POINTS:
            return (HIGH_SCORE(HS_POE_POINTS) >= 1000) || Flags_GetRandomizerInf(RAND_INF_10_BIG_POES);
        case QL_CHK_CHILD_TRADE:
            item = INV_CONTENT(ITEM_TRADE_CHILD);
            return ((item != ITEM_NONE) && (item >= e->flag) && (item <= ITEM_SOLD_OUT)) ||
                   GET_EVENTCHKINF(EVENTCHKINF_TALON_WOKEN_IN_CASTLE);
        case QL_CHK_ADULT_TRADE:
            item = INV_CONTENT(ITEM_TRADE_ADULT);
            return ((item != ITEM_NONE) && (item >= e->flag) && (item <= ITEM_CLAIM_CHECK)) ||
                   (gSaveContext.bgsFlag != 0);
        case QL_CHK_BIGGORON:
            return gSaveContext.bgsFlag != 0;
        case QL_CHK_GS_TOKENS:
            return gSaveContext.inventory.gsTokens >= e->flag;
        case QL_CHK_QUEST_ITEM:
            return CHECK_QUEST_ITEM(e->flag) != 0;
        case QL_CHK_ITEM:
            return INV_CONTENT(e->flag) == e->flag;
        case QL_CHK_MASTER_SWORD:
            return CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER) != 0;
        case QL_CHK_MEDALLIONS:
            return CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST) && CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE) &&
                   CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER) && CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT) &&
                   CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW) && CHECK_QUEST_ITEM(QUEST_MEDALLION_LIGHT);
    }
    return false;
}

static bool QuestLog_HasBottleOf(u8 item) {
    s32 slot;

    for (slot = SLOT_BOTTLE_1; slot <= SLOT_BOTTLE_4; slot++) {
        if (gSaveContext.inventory.items[slot] == item) {
            return true;
        }
    }
    return false;
}

static const char* QuestLog_NeedIcon(s32 need) {
    switch (1 << need) {
        case QL_NEED_BEAN: return gItemIconMagicBeanTex;
        case QL_NEED_HOOKSHOT: return gItemIconHookshotTex;
        case QL_NEED_LONGSHOT: return gItemIconLongshotTex;
        case QL_NEED_BOMB: return gItemIconBombTex;
        case QL_NEED_BOW: return gItemIconBowTex;
        case QL_NEED_IRON_BOOTS: return gItemIconBootsIronTex;
        case QL_NEED_HOVER_BOOTS: return gItemIconBootsHoverTex;
        case QL_NEED_SILVER_SCALE: return gItemIconScaleSilverTex;
        case QL_NEED_GOLD_SCALE: return gItemIconScaleGoldenTex;
        case QL_NEED_LENS: return gItemIconLensOfTruthTex;
        case QL_NEED_OCARINA: return gItemIconOcarinaFairyTex;
        case QL_NEED_BOOMERANG: return gItemIconBoomerangTex;
        case QL_NEED_ZORA_TUNIC: return gItemIconTunicZoraTex;
        case QL_NEED_BLUE_FIRE: return gItemIconBottleBlueFireTex;
        case QL_NEED_DEKU_STICK: return gItemIconDekuStickTex;
        case QL_NEED_NIGHT: return gMoonIconTex;
        case QL_NEED_DAY: return gSunIconTex;
    }
    return NULL;
}

static bool QuestLog_HasNeed(s32 need) {
    switch (1 << need) {
        case QL_NEED_BEAN: return INV_CONTENT(ITEM_BEAN) == ITEM_BEAN;
        case QL_NEED_HOOKSHOT: return INV_CONTENT(ITEM_HOOKSHOT) != ITEM_NONE;
        case QL_NEED_LONGSHOT: return INV_CONTENT(ITEM_LONGSHOT) == ITEM_LONGSHOT;
        case QL_NEED_BOMB: return INV_CONTENT(ITEM_BOMB) == ITEM_BOMB;
        case QL_NEED_BOW: return INV_CONTENT(ITEM_BOW) == ITEM_BOW;
        case QL_NEED_IRON_BOOTS: return CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON) != 0;
        case QL_NEED_HOVER_BOOTS: return CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_HOVER) != 0;
        case QL_NEED_SILVER_SCALE: return CUR_UPG_VALUE(UPG_SCALE) >= 1;
        case QL_NEED_GOLD_SCALE: return CUR_UPG_VALUE(UPG_SCALE) >= 2;
        case QL_NEED_LENS: return INV_CONTENT(ITEM_LENS) == ITEM_LENS;
        case QL_NEED_OCARINA: return INV_CONTENT(ITEM_OCARINA_FAIRY) != ITEM_NONE;
        case QL_NEED_BOOMERANG: return INV_CONTENT(ITEM_BOOMERANG) == ITEM_BOOMERANG;
        case QL_NEED_ZORA_TUNIC: return CHECK_OWNED_EQUIP(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_ZORA) != 0;
        case QL_NEED_BLUE_FIRE: return QuestLog_HasBottleOf(ITEM_BLUE_FIRE);
        case QL_NEED_DEKU_STICK: return INV_CONTENT(ITEM_STICK) == ITEM_STICK;
        // Lit when it is the right time now.
        case QL_NEED_NIGHT: return IS_NIGHT;
        case QL_NEED_DAY: return IS_DAY;
    }
    return false;
}

static void QuestLog_SetSection(QuestLogRow* row, const QuestLogSection* sections, s32 index) {
    row->section = NULL;
    row->sectionStyle = 0;
    for (; (sections != NULL) && (sections->start >= 0) && (sections->start <= index); sections++) {
        row->section = sections->title;
        row->sectionStyle = sections->style;
    }
}

static s32 QuestLog_AddEntries(PlayState* play, s32 n, const QuestLogEntry* entries, s32 count, const s8* points,
                               const QuestLogSection* sections, s32 sectionBase) {
    s32 i;

    for (i = 0; (i < count) && (n < QL_MAX_ROWS); i++, n++) {
        QuestLogRow* row = &sQuestLogRows[n];

        row->name = entries[i].name;
        row->detail = entries[i].detail;
        row->icon = entries[i].icon;
        row->iconIs24 = entries[i].iconIs24;
        row->age = entries[i].age;
        row->needs = entries[i].needs;
        row->done = QuestLog_IsDone(play, &entries[i]);
        row->hasCount = false;
        row->got = row->total = 0;
        row->point = (points != NULL) ? points[i] : QL_PT_NONE;
        QuestLog_SetSection(row, sections, sectionBase + i);
    }
    return n;
}

static s32 QuestLog_PopCount(u32 v) {
    s32 n = 0;

    while (v != 0) {
        n += v & 1;
        v >>= 1;
    }
    return n;
}

static s16 QuestLog_CountDone(PlayState* play, const QuestLogEntry* entries, s32 count) {
    s16 n = 0;
    s32 i;

    for (i = 0; i < count; i++) {
        if (QuestLog_IsDone(play, &entries[i])) {
            n++;
        }
    }
    return n;
}

// The temple of the first medallion still missing, for the map of "Desperte os Sábios".
static s8 QuestLog_NextTemplePoint(void) {
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST)) {
        return QL_PT_LOST_WOODS;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE)) {
        return QL_PT_DEATH_MOUNTAIN;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER)) {
        return QL_PT_LAKE;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW)) {
        return QL_PT_KAKARIKO;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT)) {
        return QL_PT_WASTELAND;
    }
    return QL_PT_MARKET;
}

// A line of the "Missões Secundárias" group: the progress of one of the other lists.
static s32 QuestLog_AddSummary(s32 n, const char* name, const char* detail, const char* icon, u8 iconIs24, s16 got,
                               s16 total) {
    QuestLogRow* row;

    if (n >= QL_MAX_ROWS) {
        return n;
    }
    row = &sQuestLogRows[n];
    row->name = name;
    row->detail = detail;
    row->icon = icon;
    row->iconIs24 = iconIs24;
    row->age = QL_AGE_ANY;
    row->needs = 0;
    row->hasCount = true;
    row->got = MIN(got, total);
    row->total = total;
    row->done = got >= total;
    row->point = QL_PT_NONE;
    row->section = sQuestLogMainSections[1].title;
    row->sectionStyle = sQuestLogMainSections[1].style;
    return n + 1;
}

// Fills sQuestLogRows with the lines of a category; returns the number of lines and the category's progress.
static s32 QuestLog_BuildRows(PlayState* play, s32 category, s16* got, s16* total) {
    s32 n = 0;
    s32 i;

    switch (category) {
        case QL_CAT_MAIN:
            n = QuestLog_AddEntries(play, 0, sQuestLogMain, ARRAY_COUNT(sQuestLogMain), sQuestLogMainPoints,
                                    sQuestLogMainSections, 0);
            sQuestLogRows[6].point = QuestLog_NextTemplePoint();
            *total = n;
            *got = 0;
            for (i = 0; i < n; i++) {
                if (sQuestLogRows[i].done) {
                    (*got)++;
                }
            }
            n = QuestLog_AddSummary(n, "Pedaços de Coração", "Os 36 Pedaços de Coração do jogo. A lista completa, com onde fica cada um, está na aba Corações.",
                                    gQuestIconHeartPieceTex, 1,
                                    QuestLog_CountDone(play, sQuestLogHearts, ARRAY_COUNT(sQuestLogHearts)),
                                    ARRAY_COUNT(sQuestLogHearts));
            n = QuestLog_AddSummary(n, "Garrafas", "As quatro garrafas que dá para ganhar. Veja onde fica cada uma na aba Garrafas.",
                                    gItemIconBottleEmptyTex, 0,
                                    QuestLog_CountDone(play, sQuestLogBottles, ARRAY_COUNT(sQuestLogBottles)),
                                    ARRAY_COUNT(sQuestLogBottles));
            n = QuestLog_AddSummary(n, "Máscaras da Loja Feliz", "Do Ovo Estranho à Máscara da Verdade. O passo a passo, com quem compra cada máscara, está na aba Máscaras.",
                                    gItemIconMaskKeatonTex, 0,
                                    QuestLog_CountDone(play, sQuestLogMasks, ARRAY_COUNT(sQuestLogMasks)),
                                    ARRAY_COUNT(sQuestLogMasks));
            n = QuestLog_AddSummary(n, "Espada de Biggoron", "A troca de adulto, do Ovo de Bolso até a Espada de Biggoron. Cada passo está na aba Trocas.",
                                    gItemIconSwordBiggoronTex, 0,
                                    QuestLog_CountDone(play, sQuestLogTrade, ARRAY_COUNT(sQuestLogTrade)),
                                    ARRAY_COUNT(sQuestLogTrade));
            n = QuestLog_AddSummary(n, "100 Skulltulas de Ouro", "As fichas das Skulltulas de Ouro. Quantas faltam em cada região está na aba Skulltulas.",
                                    gQuestIconGoldSkulltulaTex, 1, gSaveContext.inventory.gsTokens, 100);
            return n;
        case QL_CAT_HEARTS:
            n = QuestLog_AddEntries(play, 0, sQuestLogHearts, ARRAY_COUNT(sQuestLogHearts), sQuestLogHeartPoints,
                                    sQuestLogHeartSections, 0);
            break;
        case QL_CAT_BOTTLES:
            n = QuestLog_AddEntries(play, 0, sQuestLogBottles, ARRAY_COUNT(sQuestLogBottles), sQuestLogBottlePoints,
                                    sQuestLogBottleSections, 0);
            break;
        case QL_CAT_MASKS:
            n = QuestLog_AddEntries(play, 0, sQuestLogMasks, ARRAY_COUNT(sQuestLogMasks), sQuestLogMaskPoints,
                                    sQuestLogMaskSections, 0);
            break;
        case QL_CAT_TRADE:
            n = QuestLog_AddEntries(play, 0, sQuestLogTrade, ARRAY_COUNT(sQuestLogTrade), sQuestLogTradePoints,
                                    sQuestLogTradeSections, 0);
            break;
        case QL_CAT_SKULLS:
            for (i = 0; (i < ARRAY_COUNT(sQuestLogSkullAreas)) && (n < QL_MAX_ROWS); i++, n++) {
                const QuestLogSkullArea* area = &sQuestLogSkullAreas[i];
                QuestLogRow* row = &sQuestLogRows[n];

                row->name = area->name;
                row->icon = area->icon;
                row->iconIs24 = area->iconIs24;
                row->age = QL_AGE_ANY;
                row->needs = 0;
                row->hasCount = true;
                if (area->point >= 0) {
                    KaleidoScope_MapCollectSkulltulas(play, area->point, &row->got, &row->total);
                    row->detail = QL_SKULL_AREA_DETAIL;
                    row->point = area->point;
                } else {
                    row->total = area->total;
                    row->got = MIN(QuestLog_PopCount(GET_GS_FLAGS(area->dungeon)), area->total);
                    row->detail = QL_SKULL_DUNGEON_DETAIL;
                    row->point = sQuestLogDungeonPoints[area->dungeon];
                }
                row->done = row->got >= row->total;
                QuestLog_SetSection(row, sQuestLogSkullSections, i);
            }
            n = QuestLog_AddEntries(play, n, sQuestLogSkullRewards, ARRAY_COUNT(sQuestLogSkullRewards), NULL,
                                    sQuestLogSkullSections, n);
            for (i = n - ARRAY_COUNT(sQuestLogSkullRewards); i < n; i++) {
                sQuestLogRows[i].point = QL_PT_KAKARIKO;
            }
            *got = MIN(gSaveContext.inventory.gsTokens, 100);
            *total = 100;
            return n;
    }

    *got = 0;
    *total = n;
    for (i = 0; i < n; i++) {
        if (sQuestLogRows[i].done) {
            (*got)++;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------------------------------------------------
// Text

// Button glyphs of the game font, written in the texts as control characters.
#define QL_GLYPH_CUP "\x01"
#define QL_GLYPH_CLEFT "\x03"
#define QL_GLYPH_CRIGHT "\x04"
#define QL_GLYPH_B "\x05"
#define QL_GLYPH_L "\x06"
#define QL_GLYPH_R "\x07"

// Converts UTF-8 to the codes of the game font, as redrawn by the PT-BR translation mods.
static s32 QuestLog_Encode(const char* text, u8* out, s32 max) {
    const u8* s = (const u8*)text;
    s32 n = 0;

    while ((*s != '\0') && (n < max)) {
        u32 cp = *s++;

        if ((cp >= 0xC0) && (cp < 0xE0) && ((*s & 0xC0) == 0x80)) {
            cp = ((cp & 0x1F) << 6) | (*s++ & 0x3F);
        } else if (cp >= 0x80) {
            // Longer sequences are not used by these texts.
            while ((*s & 0xC0) == 0x80) {
                s++;
            }
            cp = '?';
        }

        switch (cp) {
            case 0x01: out[n++] = 0xA5; continue; // C-Up
            case 0x02: out[n++] = 0xA6; continue; // C-Down
            case 0x03: out[n++] = 0xA7; continue; // C-Left
            case 0x04: out[n++] = 0xA8; continue; // C-Right
            case 0x05: out[n++] = 0xA0; continue; // B
            case 0x06: out[n++] = 0xA2; continue; // L
            case 0x07: out[n++] = 0xA3; continue; // R
            case 0xC0: out[n++] = 0x80; continue; // À
            case 0xC1: out[n++] = 0x81; continue; // Á (mod)
            case 0xC2: out[n++] = 0x82; continue; // Â
            case 0xC3: out[n++] = 0x83; continue; // Ã (mod)
            case 0xC7: out[n++] = 0x84; continue; // Ç
            case 0xC9: out[n++] = 0x86; continue; // É
            case 0xCA: out[n++] = 0x87; continue; // Ê
            case 0xCD: out[n++] = 0x89; continue; // Í (mod)
            case 0xD4: out[n++] = 0x8A; continue; // Ô
            case 0xD5: out[n++] = 0x8B; continue; // Õ (mod)
            case 0xD3: out[n++] = 0x8C; continue; // Ó (mod)
            case 0xDC: out[n++] = 0x8D; continue; // Ü (mod)
            case 0xDA: out[n++] = 0x8E; continue; // Ú (mod)
            case 0xE0: out[n++] = 0x90; continue; // à
            case 0xE1: out[n++] = 0x91; continue; // á
            case 0xE2: out[n++] = 0x92; continue; // â
            case 0xE3: out[n++] = 0x93; continue; // ã (mod)
            case 0xE7: out[n++] = 0x94; continue; // ç
            case 0xE9: out[n++] = 0x96; continue; // é
            case 0xEA: out[n++] = 0x97; continue; // ê
            case 0xED: out[n++] = 0x99; continue; // í (mod)
            case 0xF4: out[n++] = 0x9A; continue; // ô
            case 0xF5: out[n++] = 0x9B; continue; // õ (mod)
            case 0xF3: out[n++] = 0x9C; continue; // ó (mod)
            case 0xFC: out[n++] = 0x9D; continue; // ü (mod)
            case 0xFA: out[n++] = 0x9E; continue; // ú (mod)
        }
        out[n++] = ((cp >= ' ') && (cp < 0x7F)) ? (u8)cp : '?';
    }
    return n;
}

// ---------------------------------------------------------------------------------------------------------------------
// Drawing
//
// Everything is drawn in "local units", with y growing downwards: half a pixel of the page for the hint on the Quest
// Status page, and half a pixel of a 427 x 240 screen for the journal.

#define QL_GFX_MAX 32000
#define QL_VTX_MAX 16000

static Gfx sQuestLogGfx[QL_GFX_MAX];
static Vtx sQuestLogVtx[QL_VTX_MAX];
static Gfx sQuestLogHintGfx[400];
static Vtx sQuestLogHintVtx[200];
static Vtx sQuestLogTitleVtx[4];

typedef struct {
    Gfx* gfx;
    Gfx* gfxEnd;
    Vtx* vtx;
    Vtx* vtxEnd;
    u8 alpha;
    u8 combine; // 0 = none set, 1 = textured, 2 = solid, 3 = vertex colors
} QuestLogDraw;

static bool QuestLog_HasRoom(QuestLogDraw* d) {
    return (d->gfx + 24 < d->gfxEnd) && (d->vtx + 4 <= d->vtxEnd);
}

static void QuestLog_Quad(QuestLogDraw* d, s16 x, s16 y, s16 w, s16 h, s16 texW, s16 texH) {
    Vtx* v = d->vtx;
    s32 i;

    for (i = 0; i < 4; i++) {
        v[i].v.ob[0] = (i & 1) ? (x + w) : x;
        v[i].v.ob[1] = (i & 2) ? (y + h) : y;
        v[i].v.ob[2] = 0;
        v[i].v.flag = 0;
        v[i].v.tc[0] = (i & 1) ? (texW << 5) : 0;
        v[i].v.tc[1] = (i & 2) ? (texH << 5) : 0;
        v[i].v.cn[0] = v[i].v.cn[1] = v[i].v.cn[2] = 255;
        v[i].v.cn[3] = 255;
    }
    gSPVertex(d->gfx++, v, 4, 0);
    gSP1Quadrangle(d->gfx++, 0, 2, 3, 1, 0);
    d->vtx += 4;
}

static void QuestLog_SetColor(QuestLogDraw* d, u8 r, u8 g, u8 b, u8 a) {
    gDPSetPrimColor(d->gfx++, 0, 0, r, g, b, (a * d->alpha) / 255);
}

static void QuestLog_Combine(QuestLogDraw* d, u8 combine) {
    if (d->combine != combine) {
        gDPPipeSync(d->gfx++);
        if (combine == 1) {
            gDPSetCombineMode(d->gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        } else if (combine == 3) {
            gDPSetCombineMode(d->gfx++, G_CC_SHADE, G_CC_SHADE);
        } else {
            gDPSetCombineMode(d->gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        }
        d->combine = combine;
    }
}

static void QuestLog_Rect(QuestLogDraw* d, s16 x, s16 y, s16 w, s16 h, u8 r, u8 g, u8 b, u8 a) {
    if (!QuestLog_HasRoom(d)) {
        return;
    }
    QuestLog_Combine(d, 2);
    QuestLog_SetColor(d, r, g, b, a);
    QuestLog_Quad(d, x, y, w, h, 0, 0);
}

// Draws a texture on a quad. Gray (and dimmer) when not "lit".
static void QuestLog_Tex(QuestLogDraw* d, const char* tex, u8 fmt, u8 siz, s16 texW, s16 texH, s16 x, s16 y, s16 w,
                         s16 h, bool lit, u8 alpha) {
    if ((tex == NULL) || !QuestLog_HasRoom(d)) {
        return;
    }
    QuestLog_Combine(d, 1);
    if (!lit) {
        gDPSetGrayscaleColor(d->gfx++, 109, 109, 109, 255);
        gSPGrayscale(d->gfx++, true);
        QuestLog_SetColor(d, 255, 255, 255, alpha * 150 / 255);
    } else {
        QuestLog_SetColor(d, 255, 255, 255, alpha);
    }
    if (siz == G_IM_SIZ_32b) {
        gDPLoadTextureBlock(d->gfx++, tex, fmt, G_IM_SIZ_32b, texW, texH, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(d->gfx++, tex, fmt, G_IM_SIZ_16b, texW, texH, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    }
    QuestLog_Quad(d, x, y, w, h, texW, texH);
    if (!lit) {
        gSPGrayscale(d->gfx++, false);
    }
}

// Item icons are 32x32 RGBA32, quest icons 24x24; the Sun and Moon of the time display are 16x16.
static void QuestLog_Icon(QuestLogDraw* d, const char* tex, bool is24, s16 x, s16 y, s16 size, bool lit) {
    s16 texSize = is24 ? 24 : 32;

    if ((tex == gSunIconTex) || (tex == gMoonIconTex)) {
        texSize = 16;
    }
    QuestLog_Tex(d, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, texSize, texSize, x, y, size, size, lit, 255);
}

static f32 QuestLog_CharAdvance(u8 c, s16 size) {
    return Ship_GetCharFontWidth(c) * size / 16.0f;
}

static f32 QuestLog_TextWidth(const u8* text, s32 len, s16 size) {
    f32 w = 0.0f;
    s32 i;

    for (i = 0; i < len; i++) {
        w += QuestLog_CharAdvance(text[i], size);
    }
    return w;
}

// Draws encoded text with glyphs of `size` units; returns the x after the text. Button glyphs keep their colors.
static f32 QuestLog_DrawCodes(QuestLogDraw* d, f32 x, s16 y, const u8* text, s32 len, s16 size, u8 r, u8 g, u8 b,
                              u8 a, f32 maxX) {
    s32 i;
    s32 colorSet = -1;

    QuestLog_Combine(d, 1);
    for (i = 0; i < len; i++) {
        u8 c = text[i];
        f32 adv = QuestLog_CharAdvance(c, size);
        // Button glyphs keep their colors, except in shadows.
        s32 color = ((r | g | b) == 0) ? 0 : (((c >= 0xA5) && (c <= 0xA8)) ? 1 : ((c == 0xA0) ? 2 : 0));

        if (x + adv > maxX + 0.5f) {
            break;
        }
        if ((c != ' ') && QuestLog_HasRoom(d)) {
            if (color != colorSet) {
                if (color == 1) {
                    QuestLog_SetColor(d, 255, 255, 50, a);
                } else if (color == 2) {
                    QuestLog_SetColor(d, 100, 255, 120, a);
                } else {
                    QuestLog_SetColor(d, r, g, b, a);
                }
                colorSet = color;
            }
            gDPLoadTextureBlock_4b(d->gfx++, Ship_GetCharFontTexture(c), G_IM_FMT_I, 16, 16, 0,
                                   G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                                   G_TX_NOLOD, G_TX_NOLOD);
            QuestLog_Quad(d, (s16)(x + 0.5f), y, size, size, 16, 16);
        }
        x += adv;
    }
    return x;
}

static f32 QuestLog_DrawText(QuestLogDraw* d, f32 x, s16 y, const char* text, s16 size, u8 r, u8 g, u8 b, u8 a,
                             f32 maxX) {
    u8 codes[128];
    s32 len = QuestLog_Encode(text, codes, ARRAY_COUNT(codes));

    return QuestLog_DrawCodes(d, x, y, codes, len, size, r, g, b, a, maxX);
}

// Draws text wrapped at word boundaries inside [x, x + width], at most maxLines lines.
static void QuestLog_DrawWrapped(QuestLogDraw* d, s16 x, s16 y, s16 width, const char* text, s16 size, s16 lineH,
                                 s32 maxLines, u8 r, u8 g, u8 b, u8 a) {
    u8 codes[320];
    s32 len = QuestLog_Encode(text, codes, ARRAY_COUNT(codes));
    s32 start = 0;
    s32 line;

    for (line = 0; (line < maxLines) && (start < len); line++) {
        f32 w = 0.0f;
        s32 end = start;
        s32 lastSpace = -1;

        while (end < len) {
            f32 adv = QuestLog_CharAdvance(codes[end], size);

            if ((w + adv > width) && (end > start)) {
                break;
            }
            if (codes[end] == ' ') {
                lastSpace = end;
            }
            w += adv;
            end++;
        }
        if ((end < len) && (lastSpace > start)) {
            end = lastSpace;
        }
        QuestLog_DrawCodes(d, x, y, &codes[start], end - start, size, r, g, b, a, x + width);
        start = end;
        while ((start < len) && (codes[start] == ' ')) {
            start++;
        }
        y += lineH;
    }
}

// Page background: the save page, with its title tile replaced by a plain band taken from the tile next to it, so the
// quest log can write its own title.
Gfx* KaleidoScope_DrawQuestLogPageSections(Gfx* gfx, Vtx* pageVtx, void** pageTexs) {
    s32 i;

    for (i = 0; i < 15; i++) {
        Vtx* v = &pageVtx[i * 4];
        void* tex = pageTexs[i];

        if (i == 5) {
            s32 j;

            // Top center tile: stretch a narrow strip of the inner edge of the top left tile, which has the same top
            // border and shading but no text.
            for (j = 0; j < 4; j++) {
                sQuestLogTitleVtx[j] = pageVtx[i * 4 + j];
                sQuestLogTitleVtx[j].v.tc[0] = (pageVtx[i * 4 + j].v.tc[0] == 0) ? (72 << 5) : (76 << 5);
            }
            v = sQuestLogTitleVtx;
            tex = pageTexs[0];
        }
        gDPPipeSync(gfx++);
        gSPVertex(gfx++, v, 4, 0);
        gDPLoadTextureBlock(gfx++, tex, G_IM_FMT_IA, G_IM_SIZ_8b, 80, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSP1Quadrangle(gfx++, 0, 2, 3, 1, 0);
    }
    return gfx;
}

static void QuestLog_ChangeCategory(s32 step) {
    sQuestLogCategory = (sQuestLogCategory + step + QL_CAT_COUNT) % QL_CAT_COUNT;
    Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

static void QuestLog_MoveCursor(s32* cursor, s32 step, s32 rowCount) {
    s32 next = CLAMP(*cursor + step, 0, MAX(rowCount - 1, 0));

    if (next != *cursor) {
        *cursor = next;
        Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    }
}

// Up/down (stick or D-pad, held to repeat) choose the line. L/R, Z, left/right and C-Left/C-Right choose the list.
// B goes back (see Update).
static void QuestLog_HandleInput(PlayState* play, s32 rowCount) {
    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = &play->state.input[0];
    s32* cursor = &sQuestLogCursor[sQuestLogCategory];
    s8 dirX;
    s8 dirY;

    if (sQuestLogFlipTimer > 0) {
        return;
    }
    if ((pauseCtx->state != 6) || !((pauseCtx->unk_1E4 == 0) || (pauseCtx->unk_1E4 == 5) || (pauseCtx->unk_1E4 == 8))) {
        sQuestLogHeldX = sQuestLogHeldY = 0;
        return;
    }
    // The page arrows are not used here.
    pauseCtx->cursorSpecialPos = 0;

    if (CHECK_BTN_ALL(input->press.button, BTN_R) || CHECK_BTN_ALL(input->press.button, BTN_CRIGHT)) {
        QuestLog_ChangeCategory(1);
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_Z) || CHECK_BTN_ALL(input->press.button, BTN_L) ||
        CHECK_BTN_ALL(input->press.button, BTN_CLEFT)) {
        QuestLog_ChangeCategory(-1);
        return;
    }

    dirX = 0;
    if ((pauseCtx->stickRelX < -30) || CHECK_BTN_ALL(input->cur.button, BTN_DLEFT)) {
        dirX = -1;
    } else if ((pauseCtx->stickRelX > 30) || CHECK_BTN_ALL(input->cur.button, BTN_DRIGHT)) {
        dirX = 1;
    }
    if ((dirX != 0) && (dirX != sQuestLogHeldX)) {
        sQuestLogHeldX = dirX;
        QuestLog_ChangeCategory(dirX);
        return;
    }
    sQuestLogHeldX = dirX;

    dirY = 0;
    if ((pauseCtx->stickRelY > 30) || CHECK_BTN_ALL(input->cur.button, BTN_DUP)) {
        dirY = -1;
    } else if ((pauseCtx->stickRelY < -30) || CHECK_BTN_ALL(input->cur.button, BTN_DDOWN)) {
        dirY = 1;
    }
    if (dirY == 0) {
        sQuestLogHeldY = 0;
    } else if (dirY != sQuestLogHeldY) {
        sQuestLogHeldY = dirY;
        sQuestLogRepeat = 10;
        QuestLog_MoveCursor(cursor, dirY, rowCount);
    } else if (--sQuestLogRepeat <= 0) {
        sQuestLogRepeat = 3;
        QuestLog_MoveCursor(cursor, dirY, rowCount);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Shapes of the journal: polygons (stone, plaques), lines, gradients and circles, all in local units.

static void QuestLog_SetVtx(Vtx* v, f32 x, f32 y, u8 r, u8 g, u8 b, u8 a) {
    v->v.ob[0] = (s16)(x + ((x < 0.0f) ? -0.5f : 0.5f));
    v->v.ob[1] = (s16)(y + ((y < 0.0f) ? -0.5f : 0.5f));
    v->v.ob[2] = 0;
    v->v.flag = 0;
    // Tiled textures: one texel every two units, in s10.5.
    v->v.tc[0] = (s16)(x * 16.0f);
    v->v.tc[1] = (s16)(y * 16.0f);
    v->v.cn[0] = r;
    v->v.cn[1] = g;
    v->v.cn[2] = b;
    v->v.cn[3] = a;
}

// Loads a 32x32 RGBA32 texture that repeats over the shapes drawn after it.
static void QuestLog_LoadTile(QuestLogDraw* d, const char* tex) {
    if (!QuestLog_HasRoom(d)) {
        return;
    }
    gDPLoadTextureBlock(d->gfx++, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
}

// A convex polygon, as a fan around its center. Textured with the last tile loaded when `textured`.
static void QuestLog_Fan(QuestLogDraw* d, const f32 (*pts)[2], s32 n, bool textured, u8 r, u8 g, u8 b, u8 a) {
    Vtx* v = d->vtx;
    f32 cx = 0.0f;
    f32 cy = 0.0f;
    s32 i;

    if ((n < 3) || (n > 24) || (d->vtx + n + 1 > d->vtxEnd) || (d->gfx + n + 16 >= d->gfxEnd)) {
        return;
    }
    QuestLog_Combine(d, textured ? 1 : 2);
    QuestLog_SetColor(d, r, g, b, a);
    for (i = 0; i < n; i++) {
        cx += pts[i][0];
        cy += pts[i][1];
    }
    QuestLog_SetVtx(&v[0], cx / n, cy / n, 255, 255, 255, 255);
    for (i = 0; i < n; i++) {
        QuestLog_SetVtx(&v[i + 1], pts[i][0], pts[i][1], 255, 255, 255, 255);
    }
    gSPVertex(d->gfx++, v, n + 1, 0);
    for (i = 0; i < n; i++) {
        gSP1Triangle(d->gfx++, 0, i + 1, ((i + 1) % n) + 1, 0);
    }
    d->vtx += n + 1;
}

static void QuestLog_Line(QuestLogDraw* d, f32 x0, f32 y0, f32 x1, f32 y1, f32 width, u8 r, u8 g, u8 b, u8 a) {
    f32 dx = x1 - x0;
    f32 dy = y1 - y0;
    f32 len = sqrtf(dx * dx + dy * dy);
    f32 nx;
    f32 ny;

    if (len < 0.01f) {
        return;
    }
    nx = -dy / len * width * 0.5f;
    ny = dx / len * width * 0.5f;
    {
        f32 pts[4][2] = { { x0 + nx, y0 + ny }, { x1 + nx, y1 + ny }, { x1 - nx, y1 - ny }, { x0 - nx, y0 - ny } };
        QuestLog_Fan(d, pts, 4, false, r, g, b, a);
    }
}

static void QuestLog_Outline(QuestLogDraw* d, const f32 (*pts)[2], s32 n, f32 width, u8 r, u8 g, u8 b, u8 a) {
    s32 i;

    for (i = 0; i < n; i++) {
        QuestLog_Line(d, pts[i][0], pts[i][1], pts[(i + 1) % n][0], pts[(i + 1) % n][1], width, r, g, b, a);
    }
}

// Rectangle with cut corners, clockwise on screen.
static s32 QuestLog_Chamfer(f32 (*pts)[2], f32 x0, f32 y0, f32 x1, f32 y1, f32 c) {
    f32 p[8][2] = { { x0 + c, y0 }, { x1 - c, y0 }, { x1, y0 + c }, { x1, y1 - c },
                    { x1 - c, y1 }, { x0 + c, y1 }, { x0, y1 - c }, { x0, y0 + c } };

    memcpy(pts, p, sizeof(p));
    return 8;
}

// Vertical (top to bottom) or horizontal (left to right) gradient between two colors.
static void QuestLog_Gradient(QuestLogDraw* d, f32 x, f32 y, f32 w, f32 h, const u8* c0, const u8* c1, bool vertical) {
    Vtx* v = d->vtx;
    s32 i;

    if (!QuestLog_HasRoom(d)) {
        return;
    }
    QuestLog_Combine(d, 3);
    for (i = 0; i < 4; i++) {
        bool second = vertical ? ((i & 2) != 0) : ((i & 1) != 0);
        const u8* c = second ? c1 : c0;

        QuestLog_SetVtx(&v[i], (i & 1) ? (x + w) : x, (i & 2) ? (y + h) : y, c[0], c[1], c[2], (c[3] * d->alpha) / 255);
    }
    gSPVertex(d->gfx++, v, 4, 0);
    gSP1Quadrangle(d->gfx++, 0, 2, 3, 1, 0);
    d->vtx += 4;
}

static void QuestLog_Circle(QuestLogDraw* d, f32 cx, f32 cy, f32 radius, u8 r, u8 g, u8 b, u8 a) {
    f32 pts[16][2];
    s32 i;

    for (i = 0; i < 16; i++) {
        pts[i][0] = cx + cosf(i * (2.0f * (f32)M_PI / 16)) * radius;
        pts[i][1] = cy + sinf(i * (2.0f * (f32)M_PI / 16)) * radius;
    }
    QuestLog_Fan(d, pts, 16, false, r, g, b, a);
}

// Carved stone: the stone texture tinted, a light edge on the sides facing the light (top, left), a dark edge on the
// others, and a dark outline.
static void QuestLog_Stone(QuestLogDraw* d, const f32 (*pts)[2], s32 n, u8 r, u8 g, u8 b, f32 bevel) {
    s32 i;

    QuestLog_LoadTile(d, gQJStoneTex);
    QuestLog_Fan(d, pts, n, true, r, g, b, 255);
    for (i = 0; i < n; i++) {
        const f32* p = pts[i];
        const f32* q = pts[(i + 1) % n];
        f32 dx = q[0] - p[0];
        f32 dy = q[1] - p[1];
        f32 len = sqrtf(dx * dx + dy * dy);
        f32 nx;
        f32 ny;
        f32 o;

        if (len < 0.01f) {
            continue;
        }
        // Outward normal of a clockwise polygon on screen.
        nx = dy / len;
        ny = -dx / len;
        o = bevel * 0.5f;
        if (nx + ny < -0.1f) {
            QuestLog_Line(d, p[0] - nx * o, p[1] - ny * o, q[0] - nx * o, q[1] - ny * o, bevel, 200, 210, 230, 80);
        } else {
            QuestLog_Line(d, p[0] - nx * o, p[1] - ny * o, q[0] - nx * o, q[1] - ny * o, bevel, 0, 0, 0, 120);
        }
    }
    QuestLog_Outline(d, pts, n, 1.5f, 10, 12, 18, 255);
}

// A recessed area: darker stone, shadow on the top and left edges, a thin light rim.
static void QuestLog_Recess(QuestLogDraw* d, const f32 (*pts)[2], s32 n, u8 r, u8 g, u8 b) {
    s32 i;

    QuestLog_LoadTile(d, gQJStoneTex);
    QuestLog_Fan(d, pts, n, true, r, g, b, 255);
    for (i = 0; i < n; i++) {
        const f32* p = pts[i];
        const f32* q = pts[(i + 1) % n];
        f32 dx = q[0] - p[0];
        f32 dy = q[1] - p[1];
        f32 len = sqrtf(dx * dx + dy * dy);
        f32 nx;
        f32 ny;

        if (len < 0.01f) {
            continue;
        }
        nx = dy / len;
        ny = -dx / len;
        if (nx + ny < -0.1f) {
            QuestLog_Line(d, p[0] - nx * 2.5f, p[1] - ny * 2.5f, q[0] - nx * 2.5f, q[1] - ny * 2.5f, 5.0f, 0, 0, 0, 110);
        }
    }
    QuestLog_Outline(d, pts, n, 1.5f, 84, 94, 112, 255);
}

// Golden glow around a shape, pulsing with `pulse` (0..1).
static void QuestLog_Glow(QuestLogDraw* d, const f32 (*pts)[2], s32 n, f32 pulse) {
    QuestLog_Outline(d, pts, n, 10.0f, 255, 190, 50, (u8)(25 + 20 * pulse));
    QuestLog_Outline(d, pts, n, 6.0f, 255, 200, 60, (u8)(55 + 40 * pulse));
    QuestLog_Outline(d, pts, n, 2.5f, 255, 228, 130, 255);
}

// The Triforce, `size` wide with its top at (x, y), in a color.
static void QuestLog_TriforceColor(QuestLogDraw* d, f32 x, f32 y, f32 size, u8 r, u8 g, u8 b, u8 a) {
    f32 half = size / 2;
    f32 h = size * 0.87f;
    f32 hh = h / 2;
    f32 t0[3][2] = { { x + half, y }, { x + half * 1.5f, y + hh }, { x + half * 0.5f, y + hh } };
    f32 t1[3][2] = { { x + half * 0.5f, y + hh }, { x + half, y + h }, { x, y + h } };
    f32 t2[3][2] = { { x + half * 1.5f, y + hh }, { x + size, y + h }, { x + half, y + h } };

    QuestLog_Fan(d, t0, 3, false, r, g, b, a);
    QuestLog_Fan(d, t1, 3, false, r, g, b, a);
    QuestLog_Fan(d, t2, 3, false, r, g, b, a);
}

// Golden Triforce with a dark rim.
static void QuestLog_GoldTriforce(QuestLogDraw* d, f32 x, f32 y, f32 size, bool lit) {
    QuestLog_TriforceColor(d, x - 2.0f, y - 2.0f, size + 4.0f, 40, 24, 4, 255);
    if (lit) {
        QuestLog_TriforceColor(d, x, y, size, 250, 200, 60, 255);
    } else {
        QuestLog_TriforceColor(d, x, y, size, 196, 200, 210, 255);
    }
}

// The winged Triforce of the title, `size` wide (the Triforce itself is a third of it).
static void QuestLog_WingedTriforce(QuestLogDraw* d, f32 cx, f32 y, f32 size) {
    f32 tri = size * 0.36f;
    f32 wing = size * 0.42f;

    QuestLog_Tex(d, gQJWingLTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, (s16)(cx - tri * 0.4f - wing), (s16)(y - wing * 0.3f),
                 (s16)wing, (s16)wing, true, 255);
    QuestLog_Tex(d, gQJWingRTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, (s16)(cx + tri * 0.4f), (s16)(y - wing * 0.3f),
                 (s16)wing, (s16)wing, true, 255);
    QuestLog_GoldTriforce(d, cx - tri / 2, y, tri, true);
}

// Text helpers in local units.
static f32 QuestLog_StrWidth(const char* text, s16 size) {
    u8 codes[160];
    s32 len = QuestLog_Encode(text, codes, ARRAY_COUNT(codes));

    return QuestLog_TextWidth(codes, len, size);
}

static f32 QuestLog_Print(QuestLogDraw* d, f32 x, f32 y, const char* text, s16 size, u8 r, u8 g, u8 b, bool shadow,
                          f32 maxX) {
    if (shadow) {
        QuestLog_DrawText(d, x + 1.5f, (s16)(y + 1.5f), text, size, 0, 0, 0, 200, maxX + 1.5f);
    }
    return QuestLog_DrawText(d, x, (s16)y, text, size, r, g, b, 255, maxX);
}

static void QuestLog_PrintCentered(QuestLogDraw* d, f32 cx, f32 y, const char* text, s16 size, u8 r, u8 g, u8 b,
                                   bool shadow) {
    f32 w = QuestLog_StrWidth(text, size);

    QuestLog_Print(d, cx - w / 2, y, text, size, r, g, b, shadow, cx + w / 2 + 2);
}

// The part of the pause world map around a point, `scale` units per texel.
static void QuestLog_MapCrop(QuestLogDraw* d, s16 x, s16 y, s16 texW, s16 texH, s16 u0, s16 v0, s16 scale) {
    if (!QuestLog_HasRoom(d)) {
        return;
    }
    QuestLog_Combine(d, 1);
    QuestLog_SetColor(d, 255, 255, 255, 255);
    gDPLoadTLUT_pal256(d->gfx++, gWorldMapImageTLUT);
    gDPSetTextureLUT(d->gfx++, G_TT_RGBA16);
    gDPLoadMultiTile(d->gfx++, gWorldMapImageTex, 0, G_TX_RENDERTILE, G_IM_FMT_CI, G_IM_SIZ_8b, 216, 128, u0, v0,
                     u0 + texW - 1, v0 + texH - 1, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                     G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetTileSize(d->gfx++, G_TX_RENDERTILE, 0, 0, (texW - 1) << G_TEXTURE_IMAGE_FRAC,
                   (texH - 1) << G_TEXTURE_IMAGE_FRAC);
    QuestLog_Quad(d, x, y, texW * scale, texH * scale, texW, texH);
    gDPPipeSync(d->gfx++);
    gDPSetTextureLUT(d->gfx++, G_TT_NONE);
}

// ---------------------------------------------------------------------------------------------------------------------
// The journal: a full screen panel drawn over the pause menu while the quest log is open.
//
// Layout in local units: a 854 x 480 canvas (half a pixel of a 427 x 240 screen), centered on the screen and scaled
// down on screens narrower than 16:9.

#define QJ_W 854
#define QJ_H 480
#define QJ_OPEN_FRAMES 8

#define QJ_LIST_X0 224
#define QJ_LIST_X1 496
#define QJ_LIST_Y0 131
#define QJ_LIST_Y1 425
#define QJ_HEADER_H 26
#define QJ_ROW_H 24

#define QJ_CARD_X0 507
#define QJ_CARD_Y0 131
#define QJ_CARD_X1 779
#define QJ_CARD_Y1 432

#define QJ_MAP_X 520
#define QJ_MAP_Y 368
#define QJ_MAP_TEX_W 82
#define QJ_MAP_TEX_H 17
#define QJ_MAP_SCALE 3

static s16 sQuestJournalAnim = 0;

static const char* sQuestLogTabIcons[QL_CAT_COUNT] = {
    NULL, gQuestIconHeartPieceTex, gItemIconBottleEmptyTex, gItemIconMaskKeatonTex, gItemIconPocketEggTex,
    gQuestIconGoldSkulltulaTex,
};
static const u8 sQuestLogTabIconIs24[QL_CAT_COUNT] = { 0, 1, 0, 0, 0, 1 };

// One line of the list on screen: a section title or a row.
typedef struct {
    s16 row; // -1 for a section title
    const char* title;
    u8 style;
} QuestJournalLine;

static QuestJournalLine sQuestJournalLines[QL_MAX_ROWS * 2];

static s32 QuestJournal_BuildLines(s32 rowCount, s32 cursor, s32* cursorLine) {
    s32 n = 0;
    s32 i;
    const char* section = NULL;

    *cursorLine = 0;
    for (i = 0; i < rowCount; i++) {
        if ((sQuestLogRows[i].section != NULL) && (sQuestLogRows[i].section != section)) {
            section = sQuestLogRows[i].section;
            sQuestJournalLines[n].row = -1;
            sQuestJournalLines[n].title = section;
            sQuestJournalLines[n].style = sQuestLogRows[i].sectionStyle;
            n++;
        }
        if (i == cursor) {
            *cursorLine = n;
        }
        sQuestJournalLines[n].row = i;
        sQuestJournalLines[n].title = NULL;
        sQuestJournalLines[n].style = 0;
        n++;
    }
    return n;
}

static s32 QuestJournal_LineH(s32 line) {
    return (sQuestJournalLines[line].row < 0) ? QJ_HEADER_H : QJ_ROW_H;
}

static s32 QuestJournal_Height(s32 from, s32 to) {
    s32 h = 0;

    for (; from <= to; from++) {
        h += QuestJournal_LineH(from);
    }
    return h;
}

static void QuestJournal_Begin(PlayState* play, QuestLogDraw* d, f32 k, u8 alpha) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Mtx* proj;

    d->gfx = sQuestLogGfx;
    d->gfxEnd = &sQuestLogGfx[QL_GFX_MAX];
    d->vtx = sQuestLogVtx;
    d->vtxEnd = &sQuestLogVtx[QL_VTX_MAX];
    d->alpha = alpha;
    d->combine = 0;

    OPEN_DISPS(gfxCtx);
    // Screen space: a 320 x 240 orthographic projection (the sides of wide screens are outside 0..320), and the
    // canvas centered on the screen.
    proj = Graph_Alloc(gfxCtx, sizeof(Mtx));
    guOrtho(proj, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -10.0f, 10.0f, 1.0f);
    gSPMatrix(POLY_OPA_DISP++, proj, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    Matrix_Push();
    Matrix_Translate(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.0f, MTXMODE_NEW);
    Matrix_Scale(k, k, 1.0f, MTXMODE_APPLY);
    Matrix_Translate(-QJ_W / 2.0f, -QJ_H / 2.0f, 0.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);

    gDPPipeSync(d->gfx++);
    gDPSetCycleType(d->gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(d->gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureFilter(d->gfx++, G_TF_BILERP);
    gDPSetTexturePersp(d->gfx++, G_TP_PERSP);
    gDPSetTextureLUT(d->gfx++, G_TT_NONE);
    gSPClearGeometryMode(d->gfx++, G_CULL_BOTH | G_FOG | G_LIGHTING | G_ZBUFFER | G_TEXTURE_GEN);
    gSPSetGeometryMode(d->gfx++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(d->gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
}

static void QuestJournal_End(PlayState* play, QuestLogDraw* d) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    gDPPipeSync(d->gfx++);
    gSPEndDisplayList(d->gfx++);

    OPEN_DISPS(gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sQuestLogGfx);
    CLOSE_DISPS(gfxCtx);

    // Back to the pause view and its render state.
    KaleidoScope_SetView(&play->pauseCtx, 0.0f, 0.0f, 64.0f);
    Gfx_SetupDL_42Opa(gfxCtx);
}

static void QuestJournal_DrawSidebar(QuestLogDraw* d, f32 pulse) {
    f32 pts[8][2];
    s32 i;

    for (i = 0; i < QL_CAT_COUNT; i++) {
        bool cur = i == sQuestLogCategory;
        f32 y0 = 131 + i * 49;
        s32 n = QuestLog_Chamfer(pts, 80, y0, 213, y0 + 42, 5);

        QuestLog_Stone(d, pts, n, cur ? 86 : 64, cur ? 84 : 72, cur ? 82 : 88, 2.5f);
        if (cur) {
            QuestLog_Glow(d, pts, n, pulse);
        }
        if (i == QL_CAT_MAIN) {
            QuestLog_GoldTriforce(d, 89, y0 + 9, 25, cur);
        } else {
            s16 size = 26;
            s16 texSize = sQuestLogTabIconIs24[i] ? 24 : 32;

            if (cur) {
                QuestLog_Icon(d, sQuestLogTabIcons[i], sQuestLogTabIconIs24[i], 88, (s16)y0 + 8, size, true);
            } else {
                // Silver, like the unselected buttons of the reference.
                QuestLog_Combine(d, 1);
                gDPSetGrayscaleColor(d->gfx++, 205, 210, 222, 255);
                gSPGrayscale(d->gfx++, true);
                QuestLog_SetColor(d, 255, 255, 255, 235);
                gDPLoadTextureBlock(d->gfx++, sQuestLogTabIcons[i], G_IM_FMT_RGBA, G_IM_SIZ_32b, texSize, texSize, 0,
                                    G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                                    G_TX_NOLOD, G_TX_NOLOD);
                QuestLog_Quad(d, 88, (s16)y0 + 8, size, size, texSize, texSize);
                gSPGrayscale(d->gfx++, false);
            }
        }
        QuestLog_Print(d, 122, y0 + 13, sQuestLogCategoryNames[i], 17, cur ? 255 : 226, cur ? 226 : 228,
                       cur ? 130 : 232, true, 210);
    }
}

static void QuestJournal_DrawList(QuestLogDraw* d, s32 rowCount, s32 cursor, f32 pulse) {
    f32 pts[8][2];
    s32 n;
    s32 lineCount;
    s32 cursorLine;
    s32* top = &sQuestLogTop[sQuestLogCategory];
    s32 avail = QJ_LIST_Y1 - QJ_LIST_Y0 - 8;
    s32 line;
    f32 y;

    n = QuestLog_Chamfer(pts, QJ_LIST_X0, QJ_LIST_Y0, QJ_LIST_X1, QJ_LIST_Y1, 6);
    QuestLog_Recess(d, pts, n, 40, 44, 56);

    lineCount = QuestJournal_BuildLines(rowCount, cursor, &cursorLine);
    if (lineCount == 0) {
        return;
    }

    // Scroll so that the cursor (and the title of its section, when it is the first line) is visible, without empty
    // space at the end.
    *top = CLAMP(*top, 0, lineCount - 1);
    if (cursorLine < *top) {
        *top = cursorLine;
    }
    if ((*top == cursorLine) && (cursorLine > 0) && (sQuestJournalLines[cursorLine - 1].row < 0)) {
        *top = cursorLine - 1;
    }
    while ((*top < cursorLine) && (QuestJournal_Height(*top, cursorLine) > avail)) {
        (*top)++;
    }
    while ((*top > 0) && (QuestJournal_Height(*top - 1, lineCount - 1) <= avail)) {
        (*top)--;
    }

    y = QJ_LIST_Y0 + 4;
    for (line = *top; line < lineCount; line++) {
        const QuestJournalLine* l = &sQuestJournalLines[line];
        s32 h = QuestJournal_LineH(line);

        if (y + h > QJ_LIST_Y1 - 3) {
            break;
        }
        if (l->row < 0) {
            static const u8 bandTop[4] = { 58, 66, 82, 255 };
            static const u8 bandBottom[4] = { 40, 46, 58, 255 };

            QuestLog_Gradient(d, QJ_LIST_X0 + 4, y + 1, QJ_LIST_X1 - QJ_LIST_X0 - 8, h - 3, bandTop, bandBottom, true);
            QuestLog_Line(d, QJ_LIST_X0 + 4, y + h - 1.5f, QJ_LIST_X1 - 4, y + h - 1.5f, 1.0f, 96, 106, 128, 255);
            if (l->style == 0) {
                QuestLog_WingedTriforce(d, 243, y + 6, 36);
                QuestLog_Print(d, 264, y + 5, l->title, 17, 250, 214, 110, true, QJ_LIST_X1 - 8);
            } else {
                QuestLog_Circle(d, 243, y + 13, 10.5f, 10, 20, 60, 255);
                QuestLog_Circle(d, 243, y + 13, 9.0f, 60, 110, 230, 255);
                QuestLog_Circle(d, 243, y + 11, 6.0f, 110, 160, 255, 255);
                QuestLog_PrintCentered(d, 243, y + 5, "!", 16, 255, 255, 255, true);
                QuestLog_Print(d, 264, y + 5, l->title, 17, 150, 200, 255, true, QJ_LIST_X1 - 8);
            }
        } else {
            const QuestLogRow* row = &sQuestLogRows[l->row];
            f32 rightX = QJ_LIST_X1 - 10;

            if (l->row == cursor) {
                s32 cn = QuestLog_Chamfer(pts, QJ_LIST_X0 + 6, y, QJ_LIST_X1 - 6, y + h - 1, 4);

                QuestLog_Fan(d, pts, cn, false, 120, 90, 20, (u8)(90 + 30 * pulse));
                QuestLog_Glow(d, pts, cn, pulse);
            } else {
                QuestLog_Line(d, QJ_LIST_X0 + 12, y + h - 0.5f, QJ_LIST_X1 - 12, y + h - 0.5f, 1.0f, 62, 68, 84, 255);
            }

            // Check box.
            QuestLog_Rect(d, 238, (s16)y + 6, 12, 12, 190, 192, 200, 255);
            QuestLog_Rect(d, 240, (s16)y + 8, 8, 8, 16, 18, 24, 255);
            if (row->done) {
                QuestLog_Tex(d, gQJCheckTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 236, (s16)y - 1, 20, 20, true, 255);
            }
            QuestLog_Icon(d, row->icon, row->iconIs24, 258, (s16)y + 3, 18, true);

            if (row->hasCount) {
                char text[16];

                snprintf(text, sizeof(text), "%d/%d", row->got, row->total);
                rightX -= QuestLog_StrWidth(text, 15);
                QuestLog_Print(d, rightX, y + 5, text, 15, 255, row->done ? 214 : 240, row->done ? 90 : 230, true,
                               QJ_LIST_X1 - 6);
                rightX -= 6;
            }
            {
                // Smaller letters for long names.
                s16 size = 15;

                while ((size > 12) && (QuestLog_StrWidth(row->name, size) > rightX - 282)) {
                    size--;
                }
                QuestLog_Print(d, 282, y + 5 + (15 - size) / 2, row->name, size, row->done ? 172 : 240,
                               row->done ? 170 : 234, row->done ? 162 : 216, true, rightX);
            }
        }
        y += h;
    }

    // Scroll arrows, like the ones of the item lists of the game.
    if (*top > 0) {
        QuestLog_Tex(d, gArrowUpTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, QJ_LIST_X1 - 18, QJ_LIST_Y0 - 6, 12, 12, true,
                     (u8)(150 + 100 * pulse));
    }
    if (line < lineCount) {
        QuestLog_Tex(d, gArrowDownTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, QJ_LIST_X1 - 18, QJ_LIST_Y1 - 6, 12, 12, true,
                     (u8)(150 + 100 * pulse));
    }
}

static void QuestJournal_DrawCard(QuestLogDraw* d, PlayState* play, s32 rowCount, s32 cursor, s16 got, s16 total,
                                  f32 pulse) {
    static const u8 burn[4] = { 112, 70, 30, 150 };
    static const u8 clear[4] = { 112, 70, 30, 0 };
    static const u8 barTop[4] = { 255, 226, 120, 255 };
    static const u8 barBottom[4] = { 190, 120, 20, 255 };
    const u8 ink[3] = { 58, 38, 20 };
    const u8 blue[3] = { 30, 40, 110 };
    const QuestLogRow* row = (rowCount > 0) ? &sQuestLogRows[cursor] : NULL;
    f32 pts[4][2] = { { QJ_CARD_X0, QJ_CARD_Y0 }, { QJ_CARD_X1, QJ_CARD_Y0 }, { QJ_CARD_X1, QJ_CARD_Y1 }, { QJ_CARD_X0, QJ_CARD_Y1 } };
    f32 y;
    s16 titleSize;
    char text[96];

    // Shadow, parchment, burnt edges and a thin brown rim.
    QuestLog_Rect(d, QJ_CARD_X0 + 4, QJ_CARD_Y0 + 5, QJ_CARD_X1 - QJ_CARD_X0, QJ_CARD_Y1 - QJ_CARD_Y0, 0, 0, 0, 120);
    QuestLog_LoadTile(d, gQJParchmentTex);
    QuestLog_Fan(d, pts, 4, true, 255, 246, 226, 255);
    QuestLog_Gradient(d, QJ_CARD_X0, QJ_CARD_Y0, QJ_CARD_X1 - QJ_CARD_X0, 14, burn, clear, true);
    QuestLog_Gradient(d, QJ_CARD_X0, QJ_CARD_Y1 - 14, QJ_CARD_X1 - QJ_CARD_X0, 14, clear, burn, true);
    QuestLog_Gradient(d, QJ_CARD_X0, QJ_CARD_Y0, 14, QJ_CARD_Y1 - QJ_CARD_Y0, burn, clear, false);
    QuestLog_Gradient(d, QJ_CARD_X1 - 14, QJ_CARD_Y0, 14, QJ_CARD_Y1 - QJ_CARD_Y0, clear, burn, false);
    QuestLog_Outline(d, pts, 4, 1.5f, 110, 74, 40, 255);
    // Faded Triforce in the corner, like a watermark.
    QuestLog_TriforceColor(d, QJ_CARD_X1 - 58, QJ_CARD_Y0 + 10, 44, 120, 88, 46, 60);

    if (row == NULL) {
        return;
    }

    // Title and description.
    titleSize = (QuestLog_StrWidth(row->name, 20) > QJ_CARD_X1 - QJ_CARD_X0 - 60) ? 16 : 20;
    QuestLog_Print(d, QJ_CARD_X0 + 14, QJ_CARD_Y0 + 12, row->name, titleSize, 52, 30, 14, false, QJ_CARD_X1 - 12);
    QuestLog_Line(d, QJ_CARD_X0 + 14, QJ_CARD_Y0 + 36, QJ_CARD_X1 - 60, QJ_CARD_Y0 + 36, 1.2f, 150, 110, 70, 255);
    QuestLog_DrawWrapped(d, QJ_CARD_X0 + 14, QJ_CARD_Y0 + 40, QJ_CARD_X1 - QJ_CARD_X0 - 28, row->detail, 13, 15, 6,
                         ink[0], ink[1], ink[2], 255);
    y = QJ_CARD_Y0 + 132;
    QuestLog_Line(d, QJ_CARD_X0 + 8, y, QJ_CARD_X1 - 8, y, 1.2f, 150, 112, 70, 200);

    // Progress: the six medallions on the main quest, a bar on the other lists.
    if (sQuestLogCategory == QL_CAT_MAIN) {
        static const u8 medallions[6] = { QUEST_MEDALLION_LIGHT, QUEST_MEDALLION_FOREST, QUEST_MEDALLION_FIRE,
                                          QUEST_MEDALLION_WATER, QUEST_MEDALLION_SHADOW, QUEST_MEDALLION_SPIRIT };
        static const char* medallionTexs[6] = { gQuestIconMedallionLightTex,  gQuestIconMedallionForestTex,
                                                gQuestIconMedallionFireTex,   gQuestIconMedallionWaterTex,
                                                gQuestIconMedallionShadowTex, gQuestIconMedallionSpiritTex };
        s32 owned = 0;
        s32 i;

        for (i = 0; i < 6; i++) {
            owned += CHECK_QUEST_ITEM(medallions[i]) ? 1 : 0;
        }
        snprintf(text, sizeof(text), "Progresso:  %d / 6", owned);
        QuestLog_Print(d, QJ_CARD_X0 + 16, y + 5, text, 15, blue[0], blue[1], blue[2], false, QJ_CARD_X1 - 10);
        for (i = 0; i < 6; i++) {
            bool has = CHECK_QUEST_ITEM(medallions[i]) != 0;
            s16 mx = QJ_CARD_X0 + 16 + i * 38;

            QuestLog_Circle(d, mx + 14, y + 36, 15, 60, 40, 20, 90);
            QuestLog_Icon(d, medallionTexs[i], true, mx, (s16)y + 22, 28, has);
            if (has) {
                QuestLog_Tex(d, gQJCheckTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, mx + 16, (s16)y + 36, 16, 16, true,
                             255);
            }
        }
    } else {
        f32 frac = (total > 0) ? CLAMP((f32)got / total, 0.0f, 1.0f) : 0.0f;
        f32 barW = QJ_CARD_X1 - QJ_CARD_X0 - 32;

        snprintf(text, sizeof(text), "Progresso:  %d / %d", got, total);
        QuestLog_Print(d, QJ_CARD_X0 + 16, y + 5, text, 15, blue[0], blue[1], blue[2], false, QJ_CARD_X1 - 10);
        QuestLog_Rect(d, QJ_CARD_X0 + 15, (s16)y + 25, (s16)barW + 2, 14, 70, 44, 20, 220);
        if (frac > 0.0f) {
            QuestLog_Gradient(d, QJ_CARD_X0 + 16, y + 26, barW * frac, 12, barTop, barBottom, true);
        }
        if (row->hasCount) {
            snprintf(text, sizeof(text), "Aqui: %d / %d", row->got, row->total);
            QuestLog_Print(d, QJ_CARD_X0 + 16, y + 42, text, 13, ink[0], ink[1], ink[2], false, QJ_CARD_X1 - 10);
        }
    }

    // Next objective on the main quest; what the line needs on the other lists.
    y = QJ_CARD_Y0 + 186;
    {
        f32 dm[4][2] = { { QJ_CARD_X0 + 20, y + 2 }, { QJ_CARD_X0 + 26, y + 8 }, { QJ_CARD_X0 + 20, y + 14 },
                         { QJ_CARD_X0 + 14, y + 8 } };
        QuestLog_Fan(d, dm, 4, false, blue[0], blue[1], blue[2], 255);
    }
    if (sQuestLogCategory == QL_CAT_MAIN) {
        s32 i;
        s32 next = -1;

        QuestLog_Print(d, QJ_CARD_X0 + 32, y, "Próximo Objetivo", 15, blue[0], blue[1], blue[2], false,
                       QJ_CARD_X1 - 10);
        for (i = 0; i < ARRAY_COUNT(sQuestLogMain); i++) {
            if (!sQuestLogRows[i].done) {
                next = i;
                break;
            }
        }
        if (next < 0) {
            snprintf(text, sizeof(text), "Hyrule está em paz. Parabéns!");
        } else if (sQuestLogRows[next].point >= 0) {
            snprintf(text, sizeof(text), "%s: %s.", sQuestLogRows[next].name,
                     sQuestLogPointNames[sQuestLogRows[next].point]);
        } else {
            snprintf(text, sizeof(text), "%s.", sQuestLogRows[next].name);
        }
        QuestLog_DrawWrapped(d, QJ_CARD_X0 + 32, (s16)y + 17, QJ_CARD_X1 - QJ_CARD_X0 - 44, text, 13, 15, 2, ink[0],
                             ink[1], ink[2], 255);
    } else {
        s32 need;
        s16 x = QJ_CARD_X0 + 32;
        bool any = false;

        QuestLog_Print(d, QJ_CARD_X0 + 32, y, "Requisitos", 15, blue[0], blue[1], blue[2], false, QJ_CARD_X1 - 10);
        if (row->age != QL_AGE_ANY) {
            bool adult = row->age == QL_AGE_ADULT;

            QuestLog_Icon(d, adult ? gItemIconSwordMasterTex : gItemIconSwordKokiriTex, false, x, (s16)y + 18, 22,
                          adult == LINK_IS_ADULT);
            QuestLog_Print(d, x + 24, y + 22, adult ? "Adulto" : "Criança", 13, ink[0], ink[1], ink[2], false,
                           QJ_CARD_X1 - 10);
            x += 24 + (s16)QuestLog_StrWidth(adult ? "Adulto" : "Criança", 13) + 8;
            any = true;
        }
        for (need = 0; need < QL_NEED_COUNT; need++) {
            if ((row->needs & (1 << need)) && (x + 22 < QJ_CARD_X1 - 8)) {
                QuestLog_Icon(d, QuestLog_NeedIcon(need), false, x, (s16)y + 18, 22, QuestLog_HasNeed(need));
                x += 24;
                any = true;
            }
        }
        if (!any) {
            QuestLog_Print(d, QJ_CARD_X0 + 32, y + 21, "Nada especial.", 13, ink[0], ink[1], ink[2], false,
                           QJ_CARD_X1 - 10);
        }
    }

    // The place on the pause world map, with Link's head blinking on it (or the Spiritual Stone / medallion of a
    // dungeon).
    {
        s16 boxW = QJ_MAP_TEX_W * QJ_MAP_SCALE;
        s16 boxH = QJ_MAP_TEX_H * QJ_MAP_SCALE;

        QuestLog_Rect(d, QJ_MAP_X - 2, QJ_MAP_Y - 2, boxW + 4, boxH + 4, 60, 40, 20, 255);
        if (row->point >= 0) {
            s16 px = sQuestLogPointPos[row->point][0];
            s16 py = sQuestLogPointPos[row->point][1];
            s16 u0 = CLAMP(px - QJ_MAP_TEX_W / 2, 0, 216 - QJ_MAP_TEX_W);
            s16 v0 = CLAMP(py - QJ_MAP_TEX_H / 2, 0, 128 - QJ_MAP_TEX_H);
            f32 mx = QJ_MAP_X + (px - u0) * QJ_MAP_SCALE;
            f32 my = QJ_MAP_Y + (py - v0) * QJ_MAP_SCALE;
            bool dungeonIcon = row->iconIs24 && (row->icon != gQuestIconHeartPieceTex) &&
                               (row->icon != gQuestIconGoldSkulltulaTex);
            static const u8 shadeTop[4] = { 0, 0, 0, 0 };
            static const u8 shadeBottom[4] = { 0, 0, 0, 170 };

            QuestLog_MapCrop(d, QJ_MAP_X, QJ_MAP_Y, QJ_MAP_TEX_W, QJ_MAP_TEX_H, u0, v0, QJ_MAP_SCALE);
            QuestLog_Circle(d, mx, my, 15, 255, 230, 120, (u8)(40 + 70 * pulse));
            QuestLog_Circle(d, mx, my, 9, 255, 240, 160, (u8)(50 + 80 * pulse));
            if (dungeonIcon) {
                QuestLog_Icon(d, row->icon, true, (s16)mx - 11, (s16)my - 11, 22, true);
            } else {
                QuestLog_Tex(d, gDungeonMapLinkHeadTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, (s16)mx - 10,
                             (s16)my - 10, 20, 20, true, (u8)(120 + 135 * pulse));
            }
            QuestLog_Gradient(d, QJ_MAP_X, QJ_MAP_Y + boxH - 22, boxW, 22, shadeTop, shadeBottom, true);
            QuestLog_PrintCentered(d, QJ_MAP_X + boxW / 2.0f, QJ_MAP_Y + boxH - 18, sQuestLogPointNames[row->point],
                                   14, 255, 246, 214, true);
        } else {
            // A summary line: its icon on a dark plate.
            QuestLog_Rect(d, QJ_MAP_X, QJ_MAP_Y, boxW, boxH, 44, 34, 24, 255);
            QuestLog_Circle(d, QJ_MAP_X + boxW / 2.0f, QJ_MAP_Y + boxH / 2.0f, 24, 255, 210, 90, (u8)(30 + 40 * pulse));
            QuestLog_Icon(d, row->icon, row->iconIs24, QJ_MAP_X + boxW / 2 - 18, QJ_MAP_Y + boxH / 2 - 18, 36, true);
        }
    }
}

void KaleidoScope_DrawQuestJournal(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    QuestLogDraw draw;
    QuestLogDraw* d = &draw;
    bool closing = (sQuestLogFlipTimer > 0) && (sQuestLogFlipTarget != PAUSE_QUEST_MODE_LOG);
    bool open = (pauseCtx->randoQuestMode == PAUSE_QUEST_MODE_LOG) && (pauseCtx->pageIndex == PAUSE_QUEST) && !closing;
    f32 t;
    f32 ease;
    f32 k;
    f32 left;
    f32 right;
    f32 pulse;
    s16 got;
    s16 total;
    s32 rowCount;
    s32* cursor;
    u8 alpha;

    if (open) {
        sQuestJournalAnim = MIN(sQuestJournalAnim + 1, QJ_OPEN_FRAMES);
    } else {
        sQuestJournalAnim = MAX(sQuestJournalAnim - 2, 0);
    }
    if (sQuestJournalAnim == 0) {
        return;
    }
    t = (f32)sQuestJournalAnim / QJ_OPEN_FRAMES;
    ease = t * t * (3.0f - 2.0f * t);

    sQuestLogFrames++;
    pulse = (sinf(sQuestLogFrames * 0.16f) + 1.0f) * 0.5f;

    rowCount = QuestLog_BuildRows(play, sQuestLogCategory, &got, &total);
    if (open) {
        QuestLog_HandleInput(play, rowCount);
        rowCount = QuestLog_BuildRows(play, sQuestLogCategory, &got, &total);
        // No item name in the panel below the page while the journal is shown.
        pauseCtx->cursorItem[PAUSE_QUEST] = PAUSE_ITEM_NONE;
    }
    cursor = &sQuestLogCursor[sQuestLogCategory];
    *cursor = CLAMP(*cursor, 0, MAX(rowCount - 1, 0));

    // Fit the 427 x 240 canvas on the screen, and grow it a little while it opens.
    left = OTRGetRectDimensionFromLeftEdge(0.0f);
    right = OTRGetRectDimensionFromRightEdge((f32)SCREEN_WIDTH);
    k = MIN(1.0f, (right - left) / (QJ_W / 2.0f)) * 0.5f;
    alpha = (u8)(pauseCtx->alpha * ease);

    QuestJournal_Begin(play, d, k * (0.92f + 0.08f * ease), alpha);

    // Darken everything behind the journal, the sides of wide screens too.
    {
        f32 sk = k * (0.92f + 0.08f * ease);
        f32 x0 = QJ_W / 2.0f + (left - 4.0f - SCREEN_WIDTH / 2.0f) / sk;
        f32 x1 = QJ_W / 2.0f + (right + 4.0f - SCREEN_WIDTH / 2.0f) / sk;
        f32 y0 = QJ_H / 2.0f - (SCREEN_HEIGHT / 2.0f + 4.0f) / sk;
        f32 y1 = QJ_H / 2.0f + (SCREEN_HEIGHT / 2.0f + 4.0f) / sk;
        f32 scrim[4][2] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };

        QuestLog_Fan(d, scrim, 4, false, 4, 6, 12, 170);
    }

    // Stone frame.
    {
        f32 pts[8][2];
        s32 n = QuestLog_Chamfer(pts, 59, 80, 795, 443, 21);

        QuestLog_Rect(d, 64, 86, 736, 363, 0, 0, 0, 120);
        QuestLog_Stone(d, pts, n, 74, 84, 104, 4.0f);
        n = QuestLog_Chamfer(pts, 69, 90, 785, 433, 15);
        QuestLog_Recess(d, pts, n, 30, 34, 44);
    }

    // L and R tabs at the sides of the title plaque.
    {
        static const f32 tabX[2] = { 251, 603 };
        s32 i;

        for (i = 0; i < 2; i++) {
            f32 cx = tabX[i];
            f32 tab[4][2] = { { cx - 27, 96 }, { cx - 17, 74 }, { cx + 17, 74 }, { cx + 27, 96 } };

            QuestLog_Stone(d, tab, 4, 80, 90, 110, 2.5f);
            QuestLog_PrintCentered(d, cx, 77, (i == 0) ? "L" : "R", 18, 200, 208, 224, true);
        }
    }

    // Title plaque with the winged Triforce.
    {
        f32 plaque[6][2] = { { 285, 88 }, { 308, 58 }, { 546, 58 }, { 569, 88 }, { 546, 118 }, { 308, 118 } };

        QuestLog_Stone(d, plaque, 6, 88, 98, 120, 3.0f);
        QuestLog_Outline(d, plaque, 6, 1.0f, 150, 160, 180, 120);
        QuestLog_WingedTriforce(d, QJ_W / 2.0f, 40, 64);
        QuestLog_PrintCentered(d, QJ_W / 2.0f, 76, "Diário de Quests", 24, 236, 224, 192, true);
    }

    // Controls, in the corners above the content.
    QuestLog_Print(d, 82, 99, QL_GLYPH_L "/" QL_GLYPH_R " Categoria", 14, 210, 214, 224, true, 290);
    QuestLog_Print(d, 772 - QuestLog_StrWidth(QL_GLYPH_B " Voltar", 14), 99, QL_GLYPH_B " Voltar", 14, 210, 214, 224,
                   true, 790);

    QuestJournal_DrawSidebar(d, pulse);
    QuestJournal_DrawList(d, rowCount, *cursor, pulse);
    QuestJournal_DrawCard(d, play, rowCount, *cursor, got, total, pulse);

    // Plaque at the bottom with a hint about the list.
    {
        f32 plaque[6][2] = { { 232, 442 }, { 250, 427 }, { 604, 427 }, { 622, 442 }, { 604, 458 }, { 250, 458 } };

        QuestLog_Stone(d, plaque, 6, 70, 80, 98, 2.5f);
        QuestLog_PrintCentered(d, QJ_W / 2.0f, 435, sQuestLogCategoryHints[sQuestLogCategory], 15, 226, 224, 214, true);
    }

    QuestJournal_End(play, d);
}

// Small "C-Up Missões" hint at the bottom right of the normal Quest Status page, drawn with the page matrix.
void KaleidoScope_DrawQuestLogHint(PlayState* play, GraphicsContext* gfxCtx) {
    PauseContext* pauseCtx = &play->pauseCtx;
    QuestLogDraw draw;
    QuestLogDraw* d = &draw;
    u8 codes[32];
    s32 len;
    f32 w;

    d->gfx = sQuestLogHintGfx;
    d->gfxEnd = &sQuestLogHintGfx[ARRAY_COUNT(sQuestLogHintGfx)];
    d->vtx = sQuestLogHintVtx;
    d->vtxEnd = &sQuestLogHintVtx[ARRAY_COUNT(sQuestLogHintVtx)];
    d->alpha = pauseCtx->alpha;
    d->combine = 0;

    OPEN_DISPS(gfxCtx);
    // The page matrix is loaded; move the origin to the top left of the page content, in half pixels, y down.
    Matrix_Push();
    Matrix_Translate(-108.0f, 58.0f + pauseCtx->offsetY, 0.0f, MTXMODE_APPLY);
    Matrix_Scale(0.5f, -0.5f, 1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);

    gDPPipeSync(d->gfx++);
    gDPSetTextureFilter(d->gfx++, G_TF_BILERP);
    len = QuestLog_Encode(QL_GLYPH_CUP "Missões", codes, ARRAY_COUNT(codes));
    w = QuestLog_TextWidth(codes, len, 16);
    // Page y -71 to -79 is free below the Spiritual Stones and the song staff.
    QuestLog_DrawCodes(d, 432 + 6 - w, 258, codes, len, 16, 220, 220, 220, 230, 432 + 12);
    gDPPipeSync(d->gfx++);
    gDPSetCombineMode(d->gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetPrimColor(d->gfx++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gSPEndDisplayList(d->gfx++);

    OPEN_DISPS(gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sQuestLogHintGfx);
    // Back to the page matrix for whatever the page draws next.
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISPS(gfxCtx);
}

static void QuestLog_SetMode(PauseContext* pauseCtx, u8 mode) {
    if (mode == pauseCtx->randoQuestMode) {
        return;
    }
    if (mode == PAUSE_QUEST_MODE_LOG) {
        sQuestLogSavedCursorItem = pauseCtx->cursorItem[PAUSE_QUEST];
    } else if (pauseCtx->randoQuestMode == PAUSE_QUEST_MODE_LOG) {
        pauseCtx->cursorItem[PAUSE_QUEST] = sQuestLogSavedCursorItem;
    }
    pauseCtx->randoQuestMode = mode;
}

// C-Up on the Quest Status page: Quest Status -> Missões -> (randomizer collectibles) -> Quest Status.
// The change happens halfway through the page flip.
void KaleidoScope_CycleQuestMode(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;

    if (sQuestLogFlipTimer > 0) {
        return;
    }
    switch (pauseCtx->randoQuestMode) {
        case PAUSE_QUEST_MODE_STATUS:
            sQuestLogFlipTarget = PAUSE_QUEST_MODE_LOG;
            break;
        case PAUSE_QUEST_MODE_LOG:
            sQuestLogFlipTarget = IS_RANDO ? PAUSE_QUEST_MODE_RANDO : PAUSE_QUEST_MODE_STATUS;
            break;
        default:
            sQuestLogFlipTarget = PAUSE_QUEST_MODE_STATUS;
            break;
    }
    sQuestLogFlipTimer = QL_FLIP_FRAMES;
    Audio_PlaySoundGeneral(NA_SE_SY_WIN_SCROLL_RIGHT, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

// B on the quest log: flip back to the Quest Status page.
void KaleidoScope_CloseQuestLog(PlayState* play) {
    if ((sQuestLogFlipTimer > 0) || (play->pauseCtx.randoQuestMode != PAUSE_QUEST_MODE_LOG)) {
        return;
    }
    sQuestLogFlipTarget = PAUSE_QUEST_MODE_STATUS;
    sQuestLogFlipTimer = QL_FLIP_FRAMES;
    Audio_PlaySoundGeneral(NA_SE_SY_WIN_SCROLL_LEFT, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

// Called with the Quest Status page matrix, before it is loaded: narrows the page while it flips.
void KaleidoScope_ApplyQuestPageFlip(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    f32 t;
    f32 scale;

    if (sQuestLogFlipTimer <= 0) {
        return;
    }
    if ((pauseCtx->state != 6) || (pauseCtx->pageIndex != PAUSE_QUEST)) {
        // The menu closed or moved on: finish at once.
        QuestLog_SetMode(pauseCtx, sQuestLogFlipTarget);
        sQuestLogFlipTimer = 0;
        return;
    }
    sQuestLogFlipTimer--;
    if (sQuestLogFlipTimer == QL_FLIP_FRAMES / 2) {
        QuestLog_SetMode(pauseCtx, sQuestLogFlipTarget);
    }
    t = (f32)(QL_FLIP_FRAMES - sQuestLogFlipTimer) / QL_FLIP_FRAMES;
    scale = fabsf(cosf(t * (f32)M_PI));
    Matrix_Scale(MAX(scale, 0.02f), 1.0f, 1.0f, MTXMODE_APPLY);
}
