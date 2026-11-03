// OOT D.E.: QuestMenu data: the quests, how each one is checked against the save, and its state.
//
// Every line reads the real progress from the save. A quest the game does not record (like beating Ganondorf) is never
// marked done. To add a quest, add a line to a table below and, where the table has them, to its parallel tables
// (world map points); the build fails if their lengths differ.
#include "questmenu.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
#include "soh_assets.h"
#include <string.h>

// ---------------------------------------------------------------------------------------------------------------------
// World map

// Centers of the world map dots on gWorldMapImageTex (216x128), from the map page vertices of the game.
const u8 gQuestMenuPointPos[QM_PT_COUNT][2] = {
    { 19, 31 }, { 45, 35 }, { 56, 47 }, { 79, 111 }, { 102, 54 }, { 113, 24 },
    { 126, 55 }, { 136, 15 }, { 147, 32 }, { 170, 61 }, { 186, 71 }, { 201, 37 },
};

const char* gQuestMenuPointNames[QM_PT_COUNT] = {
    "Deserto Assombrado", "Fortaleza Gerudo",   "Vale Gerudo",      "Lago Hylia",
    "Rancho Lon Lon",     "Castelo de Hyrule",  "Campo de Hyrule",  "Montanha da Morte",
    "Vila Kakariko",      "Floresta Perdida",   "Floresta Kokiri",  "Domínio Zora",
};

// ---------------------------------------------------------------------------------------------------------------------
// Missões principais

static const QuestMenuObjective sQuestMenuGoalsEmerald[] = {
    { "Espada Kokiri", gItemIconSwordKokiriTex, 0, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_KOKIRI) },
    { "Escudo Deku", gItemIconShieldDekuTex, 0, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_DEKU) },
    { "Estilingue das Fadas", gItemIconSlingshotTex, 0, QM_CHK_ITEM, 0, ITEM_SLINGSHOT },
    { "Esmeralda Kokiri", gQuestIconKokiriEmeraldTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_KOKIRI_EMERALD },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsLetter[] = {
    { "Acordar o Talon", gItemIconChickenTex, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_TALON_WOKEN_IN_CASTLE },
    { "Carta da Zelda", gItemIconZeldasLetterTex, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER },
    { "Canção de Ninar", gQMIconSongTex, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_LULLABY },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsRuby[] = {
    { "Canção de Saria", gQMIconSongTex, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_SARIA },
    { "Bolsa de Bombas", gItemIconBombTex, 0, QM_CHK_ITEM, 0, ITEM_BOMB },
    { "Rubi Goron", gQuestIconGoronRubyTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_GORON_RUBY },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsSapphire[] = {
    { "Carta de Ruto", gItemIconBottleRutosLetterTex, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_RUTOS_LETTER },
    { "Bumerangue", gItemIconBoomerangTex, 0, QM_CHK_ITEM, 0, ITEM_BOOMERANG },
    { "Safira Zora", gQuestIconZoraSapphireTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_ZORA_SAPPHIRE },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsOcarina[] = {
    { "Esmeralda Kokiri", gQuestIconKokiriEmeraldTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_KOKIRI_EMERALD },
    { "Rubi Goron", gQuestIconGoronRubyTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_GORON_RUBY },
    { "Safira Zora", gQuestIconZoraSapphireTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_ZORA_SAPPHIRE },
    { "Ocarina do Tempo", gItemIconOcarinaOfTimeTex, 0, QM_CHK_ITEM, 0, ITEM_OCARINA_TIME },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsSword[] = {
    { "Ocarina do Tempo", gItemIconOcarinaOfTimeTex, 0, QM_CHK_ITEM, 0, ITEM_OCARINA_TIME },
    { "Canção do Tempo", gQMIconSongTex, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_TIME },
    { "Espada Mestra", gItemIconSwordMasterTex, 0, QM_CHK_MASTER_SWORD, 0, 0 },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsSages[] = {
    { "Medalhão da Luz", gQuestIconMedallionLightTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_LIGHT },
    { "Medalhão da Floresta", gQuestIconMedallionForestTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_FOREST },
    { "Medalhão do Fogo", gQuestIconMedallionFireTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_FIRE },
    { "Medalhão da Água", gQuestIconMedallionWaterTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_WATER },
    { "Medalhão das Sombras", gQuestIconMedallionShadowTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_SHADOW },
    { "Medalhão do Espírito", gQuestIconMedallionSpiritTex, 1, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_SPIRIT },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsLightArrows[] = {
    { "Os seis medalhões", gQuestIconMedallionLightTex, 1, QM_CHK_MEDALLIONS, 0, 0 },
    { "Flechas de Luz", gItemIconArrowLightTex, 0, QM_CHK_ITEM, 0, ITEM_ARROW_LIGHT },
    { NULL },
};
static const QuestMenuObjective sQuestMenuGoalsGanon[] = {
    { "Flechas de Luz", gItemIconArrowLightTex, 0, QM_CHK_ITEM, 0, ITEM_ARROW_LIGHT },
    { "Luvas de Ouro", gItemIconGoldenGauntletsTex, 0, QM_CHK_UPGRADE, 0, QM_UPGRADE(UPG_STRENGTH, 3) },
    { "Chave da Torre de Ganon", gQuestIconDungeonBossKeyTex, 1, QM_CHK_BOSS_KEY, SCENE_GANONS_TOWER, 0 },
    { "Derrotar Ganondorf", gItemIconSwordMasterTex, 0, QM_CHK_NONE, 0, 0 },
    { NULL },
};

static const QuestMenuEntry sQuestMenuMain[] = {
    { "Pedra Espiritual da Floresta", "Entre na Árvore Deku, na Floresta Kokiri, e derrote a Rainha Gohma. A Árvore Deku entrega a Esmeralda Kokiri.", gQuestIconKokiriEmeraldTex, 1, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_KOKIRI_EMERALD, sQuestMenuGoalsEmerald },
    { "Carta da Zelda", "Entre escondido no jardim do Castelo de Hyrule e fale com a Princesa Zelda. Depois Impa ensina a Canção de Ninar da Zelda.", gItemIconZeldasLetterTex, 0, QM_AGE_CHILD, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER, sQuestMenuGoalsLetter },
    { "Pedra Espiritual do Fogo", "Na Montanha da Morte, abra a Caverna Dodongo e derrote o Rei Dodongo. Darunia entrega o Rubi Goron.", gQuestIconGoronRubyTex, 1, QM_AGE_CHILD, QM_NEED_BOMB, QM_CHK_QUEST_ITEM, 0, QUEST_GORON_RUBY, sQuestMenuGoalsRuby },
    { "Pedra Espiritual da Água", "Leve um peixe numa garrafa ao Lorde Jabu-Jabu, na Fonte Zora, e proteja a Princesa Ruto lá dentro. Ela entrega a Safira Zora.", gQuestIconZoraSapphireTex, 1, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_ZORA_SAPPHIRE, sQuestMenuGoalsSapphire },
    { "Ocarina do Tempo", "Com as três Pedras, volte ao Castelo de Hyrule. Zelda foge com Impa e joga a Ocarina do Tempo no fosso, na frente da ponte.", gItemIconOcarinaOfTimeTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEM, 0, ITEM_OCARINA_TIME, sQuestMenuGoalsOcarina },
    { "Espada Mestra", "No Templo do Tempo, toque a Canção do Tempo diante do altar e puxe a Espada Mestra. Link acorda sete anos depois.", gItemIconSwordMasterTex, 0, QM_AGE_ANY, QM_NEED_OCARINA, QM_CHK_MASTER_SWORD, 0, 0, sQuestMenuGoalsSword },
    { "Desperte os Sábios", "Purifique os templos e desperte os Sábios. Com o Medalhão da Luz são seis medalhões para selar Ganondorf.", gQuestIconMedallionLightTex, 1, QM_AGE_ADULT, 0, QM_CHK_MEDALLIONS, 0, 0, sQuestMenuGoalsSages },
    { "Flechas de Luz", "Com os seis medalhões, volte ao Templo do Tempo. Sheik revela quem é e entrega as Flechas de Luz.", gItemIconArrowLightTex, 0, QM_AGE_ADULT, 0, QM_CHK_ITEM, 0, ITEM_ARROW_LIGHT, sQuestMenuGoalsLightArrows },
    { "Derrote Ganondorf", "Atravesse o Castelo de Ganon, desfaça as seis barreiras dos Sábios e derrote Ganondorf no alto da torre. O jogo não guarda essa vitória no save, então ela nunca aparece como concluída aqui.", gQuestIconDungeonBossKeyTex, 1, QM_AGE_ADULT, QM_NEED_BOW, QM_CHK_NONE, 0, 0, sQuestMenuGoalsGanon },
};
static const s8 sQuestMenuMainPoints[] = {
    QM_PT_KOKIRI, QM_PT_MARKET, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_MARKET,
    QM_PT_MARKET, QM_PT_NONE /* next temple */, QM_PT_MARKET, QM_PT_MARKET,
};
static const QuestMenuSection sQuestMenuMainSections[] = {
    { 0, 0, "Infância" }, { 5, 0, "Sete anos depois" }, { -1 },
};

// ---------------------------------------------------------------------------------------------------------------------
// Missões secundárias

static const QuestMenuEntry sQuestMenuHearts[] = {
    // Same order as the Piece of Heart table of the world map counter.
    { "Colosso do Deserto", "Plante um Feijão Mágico no canteiro perto da Fonte da Grande Fada (criança). Volte adulto e suba na folha voadora até o arco de pedra.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_BEAN, QM_CHK_HEART_PIECE, 0, 0 },
    { "Fortaleza Gerudo: telhado", "Baú no ponto mais alto da Fortaleza Gerudo. Suba pelos telhados usando o Gancho.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_HOOKSHOT, QM_CHK_HEART_PIECE, 0, 1 },
    { "Fortaleza Gerudo: tiro a cavalo", "Com Epona e o Cartão Gerudo, faça 1000 pontos no tiro com arco a cavalo. Com 1500 pontos você ganha a aljava maior.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_BOW, QM_CHK_HEART_PIECE, 0, 2 },
    { "Vale Gerudo: cachoeira", "Numa saliência ao lado da cachoeira, abaixo da ponte. Desça pela beira do penhasco ou plane até lá com um Cuco.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 3 },
    { "Vale Gerudo: caixote", "No fundo do vale, perto do rio, dentro de um caixote. Desça até a margem do rio e quebre o caixote rolando nele.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 4 },
    { "Laboratório do Lago", "Com as Botas de Ferro, afunde até o fundo do tanque do laboratório do Lago Hylia. Depois fale com o cientista.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_IRON_BOOTS, QM_CHK_HEART_PIECE, 0, 5 },
    { "Lagoa de Pesca", "Lago Hylia, Lagoa de Pesca: como criança, pesque um peixe de 10 libras ou mais e mostre ao dono.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, 0, QM_CHK_HEART_PIECE, 0, 6 },
    { "Lago Hylia: topo do laboratório", "Plante um Feijão Mágico no canteiro ao lado do laboratório (criança). Volte adulto e suba na folha até o telhado.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_BEAN, QM_CHK_HEART_PIECE, 0, 7 },
    { "Rancho Lon Lon: silo", "Dentro do silo do Rancho Lon Lon, empurre as caixas e entre pela passagem estreita. Só a criança cabe.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, 0, QM_CHK_HEART_PIECE, 0, 8 },
    { "Mercado: boliche Bombchu", "Boliche Bombchu do Mercado (precisa da Bolsa de Bombas). O prêmio muda a cada partida: jogue até sair o Pedaço de Coração.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_BOMB, QM_CHK_HEART_PIECE, 0, 9 },
    { "Mercado: cachorro perdido", "No Mercado, à noite: ache o cachorro Richard (o que segue você) e leve-o à moça da casa no beco.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_NIGHT, QM_CHK_HEART_PIECE, 0, 10 },
    { "Mercado: jogo dos baús", "No Mercado, à noite: vença o jogo dos baús. Com a Lente da Verdade você enxerga o baú certo.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_NIGHT | QM_NEED_LENS, QM_CHK_HEART_PIECE, 0, 11 },
    { "Campo: gruta das Tektites", "Gruta escondida do Campo de Hyrule, aberta com bomba. O pedaço está no fundo da água: mergulhe com a Escama de Ouro ou desça com as Botas de Ferro.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, QM_NEED_BOMB | QM_NEED_GOLD_SCALE | QM_NEED_IRON_BOOTS, QM_CHK_HEART_PIECE, 0, 12 },
    { "Campo: Deku da gruta", "Gruta dentro do cercado de árvores perto da entrada do Lago Hylia (abra com bomba). Compre o pedaço do Deku por 10 rupias.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, QM_NEED_BOMB, QM_CHK_HEART_PIECE, 0, 13 },
    { "Trilha da Montanha da Morte", "Na Trilha da Montanha da Morte, no alto da parede acima da entrada da Caverna Dodongo.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 14 },
    { "Cidade Goron: vaso que gira", "No andar de baixo da Cidade Goron, jogue uma bomba acesa no vaso que gira quando ele mostrar a cara sorrindo.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, QM_NEED_BOMB, QM_CHK_HEART_PIECE, 0, 15 },
    { "Cratera: parede", "Entrando na Cratera pela trilha, desça pela parede da esquerda até a saliência onde está o pedaço.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, 0, QM_CHK_HEART_PIECE, 0, 16 },
    { "Cratera: vulcão", "No alto do pilar no meio da Cratera. Plante um Feijão Mágico no canteiro perto do teleporte (criança) e volte adulto, ou use as Botas Flutuantes.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_BEAN | QM_NEED_HOVER_BOOTS, QM_CHK_HEART_PIECE, 0, 17 },
    { "50 Skulltulas de Ouro", "Casa das Skulltulas, em Kakariko: entregue 50 fichas de Skulltula de Ouro.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 18 },
    { "Kakariko: homem do telhado", "Suba no telhado da casa da Impa e fale com o homem lá em cima. Criança: pule da torre de vigia. Adulto: use o Gancho.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 19 },
    { "Kakariko: vaca da casa da Impa", "No curral da vaca, no fundo da casa da Impa. Entre pela abertura no alto, nos fundos da casa, pulando do telhado.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 20 },
    { "Kakariko: moinho", "No alto, dentro do moinho. Suba nas plataformas que giram e pule até a saliência. Com o Bumerangue dá para pegar de longe.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, QM_NEED_BOOMERANG, QM_CHK_HEART_PIECE, 0, 21 },
    { "Cemitério: túmulo dos ReDeads", "Puxe a lápide do túmulo dos ReDeads e desça. Lá dentro, toque a Canção do Sol diante da parede para o baú aparecer.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, QM_NEED_OCARINA, QM_CHK_HEART_PIECE, 0, 22 },
    { "Cemitério: no muro", "Numa reentrância alta no muro do cemitério. Plante um Feijão Mágico no canteiro abaixo dela (criança) e volte adulto, ou use o Gancho Longo.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_BEAN | QM_NEED_LONGSHOT, QM_CHK_HEART_PIECE, 0, 23 },
    { "Cemitério: corrida do Dampé", "No túmulo do Dampé (adulto), chegue ao fim da corrida contra o fantasma dele em menos de 1 minuto.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, 0, QM_CHK_HEART_PIECE, 0, 24 },
    { "Cemitério: escavação do Dampé", "No cemitério, à noite (criança), pague o Dampé para cavar. O pedaço sai ao acaso: tente várias vezes.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_NIGHT, QM_CHK_HEART_PIECE, 0, 25 },
    { "Floresta Perdida: Skull Kid", "Perto da entrada da Floresta Perdida, toque a Canção de Saria para o Skull Kid em cima do toco.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_HEART_PIECE, 0, 26 },
    { "Floresta Perdida: jogo musical", "Na clareira dos Skull Kids músicos, repita as melodias que eles tocam até o fim do jogo.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_HEART_PIECE, 0, 27 },
    { "Rio Zora: rãs (Tempestades)", "No Rio Zora, no tronco das rãs, toque a Canção das Tempestades para elas (criança).", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_HEART_PIECE, 0, 28 },
    { "Rio Zora: jogo das rãs", "Depois de tocar as canções para as rãs (criança), vença o jogo delas tocando as notas que aparecem.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_HEART_PIECE, 0, 29 },
    { "Rio Zora: pilar", "No alto de um pilar perto da gruta aberta, no começo do Rio Zora. Suba pelas pedras ou plane até lá com um Cuco.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 30 },
    { "Rio Zora: perto da cachoeira", "Na saliência perto da cachoeira que leva ao Domínio Zora. Pule pelas pedras do rio ou plane até lá com um Cuco.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_HEART_PIECE, 0, 31 },
    { "Domínio Zora: tochas", "Acenda as tochas do Domínio Zora com um Bastão Deku aceso, uma atrás da outra, antes que apaguem. O baú aparece no alto.", gQuestIconHeartPieceTex, 1, QM_AGE_CHILD, QM_NEED_DEKU_STICK, QM_CHK_HEART_PIECE, 0, 32 },
    { "Fonte Zora: iceberg", "Na Fonte Zora congelada (adulto), pule pelos blocos de gelo até o iceberg mais distante.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, 0, QM_CHK_HEART_PIECE, 0, 33 },
    { "Fonte Zora: fundo", "No fundo da Fonte Zora (adulto). Desça com as Botas de Ferro; a Túnica Zora deixa respirar embaixo d'água.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_IRON_BOOTS | QM_NEED_ZORA_TUNIC, QM_CHK_HEART_PIECE, 0, 34 },
    { "Caverna de Gelo", "Dentro da Caverna de Gelo (adulto). Derreta o gelo vermelho com Fogo Azul guardado numa garrafa.", gQuestIconHeartPieceTex, 1, QM_AGE_ADULT, QM_NEED_BLUE_FIRE, QM_CHK_COLLECT, SCENE_ICE_CAVERN, 0x01 },
};

static const QuestMenuEntry sQuestMenuBottles[] = {
    { "Rancho Lon Lon: cucos do Talon", "Depois de acordar o Talon no castelo, vença o jogo dos Super Cucos na casa do rancho (criança). Vem com Leite Lon Lon.", gItemIconBottleMilkFullTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_TALON_BOTTLE },
    { "Kakariko: cucos da Anju", "Junte os 7 cucos da Anju no cercado dela, na entrada de Kakariko (criança). Um deles está atrás da cerca: plane com outro cuco.", gItemIconBottleEmptyTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, 0x0C },
    { "Carta de Ruto", "No fundo do Lago Hylia (criança), perto da margem. Precisa da Escama de Prata do jogo de mergulho do Domínio Zora. Mostre ao Rei Zora e a garrafa fica com você.", gItemIconBottleRutosLetterTex, 0, QM_AGE_CHILD, QM_NEED_SILVER_SCALE, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_RUTOS_LETTER },
    { "Coletor de Poes", "Com Epona e o Arco, cace 10 Grandes Poes no Campo de Hyrule e venda ao homem da casa do guarda, no Mercado em ruínas (adulto).", gItemIconBottleBigPoeTex, 0, QM_AGE_ADULT, QM_NEED_BOW, QM_CHK_POE_POINTS, 0, 0 },
};

static const QuestMenuEntry sQuestMenuMasks[] = {
    { "Ovo Estranho", "Fale com a Malon na frente do Castelo de Hyrule. O ovo choca com o tempo e vira um Cuco.", gItemIconWeirdEggTex, 0, QM_AGE_CHILD, 0, QM_CHK_CHILD_TRADE, 0, ITEM_WEIRD_EGG },
    { "Acordar o Talon", "Use o Cuco no Talon, que dorme ao lado das caixas perto da entrada do castelo.", gItemIconChickenTex, 0, QM_AGE_CHILD, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_TALON_WOKEN_IN_CASTLE },
    { "Carta de Zelda", "Entre escondido no jardim do castelo e fale com a Zelda.", gItemIconZeldasLetterTex, 0, QM_AGE_CHILD, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER },
    { "Abrir a Loja de Máscaras", "Mostre a Carta de Zelda ao guarda do portão da Montanha da Morte, em Kakariko. Aí a Loja da Máscara Feliz, no Mercado, abre.", gItemIconZeldasLetterTex, 0, QM_AGE_CHILD, 0, QM_CHK_INFTABLE, 0, INFTABLE_SHOWED_ZELDAS_LETTER_TO_GATE_GUARD },
    { "Máscara Keaton", "Pegue a Máscara Keaton emprestada na loja e venda ao guarda do portão da Montanha da Morte. Depois pague a loja.", gItemIconMaskKeatonTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_38 },
    { "Máscara de Caveira", "Venda ao Skull Kid do toco perto da entrada da Floresta Perdida. Depois pague a loja.", gItemIconMaskSkullTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_39 },
    { "Máscara Assustadora", "Venda ao menino do Cemitério de Kakariko, de dia. Ele quer ser igual ao Dampé. Depois pague a loja.", gItemIconMaskSpookyTex, 0, QM_AGE_CHILD, QM_NEED_DAY, QM_CHK_ITEMGETINF, 0, ITEMGETINF_3A },
    { "Capuz de Coelho", "Venda ao Corredor, o homem que vive correndo pelo Campo de Hyrule. Ele paga com a carteira inteira. Depois pague a loja.", gItemIconMaskBunnyHoodTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_3B },
    { "Máscara da Verdade", "Depois de vender as quatro, a loja empresta a Máscara da Verdade. Com ela as Pedras Fofoqueiras falam. As máscaras Goron, Zora e Gerudo também ficam disponíveis.", gItemIconMaskTruthTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_3F },
    { "Teatro Deku: Caveira", "No Teatro Deku (gruta da Floresta Perdida, perto do Prado Sagrado), use a Máscara de Caveira. Prêmio: carregar mais Bastões Deku.", gItemIconMaskSkullTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_OBTAINED_STICK_UPGRADE_FROM_STAGE },
    { "Teatro Deku: Verdade", "No Teatro Deku, use a Máscara da Verdade. Prêmio: carregar mais Nozes Deku.", gItemIconMaskTruthTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEMGETINF, 0, ITEMGETINF_OBTAINED_NUT_UPGRADE_FROM_STAGE },
};

static const QuestMenuEntry sQuestMenuTrade[] = {
    { "Ovo de Bolso", "Fale com a Anju (adulto), na frente do cercado dos cucos em Kakariko.", gItemIconPocketEggTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_POCKET_EGG },
    { "Cuco de Bolso", "O ovo choca depois de uma noite. Use o cuco para acordar o Talon, que dorme na casa da Impa.", gItemIconPocketCuccoTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_POCKET_CUCCO },
    { "Cojiro", "Depois de acordar o Talon, dê o cuco à Anju e ganhe o Cojiro, o cuco azul.", gItemIconCojiroTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_COJIRO },
    { "Cogumelo Estranho", "Na Floresta Perdida, mostre o Cojiro ao rapaz sentado perto do caminho do Prado.", gItemIconOddMushroomTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_ODD_MUSHROOM },
    { "Poção Estranha", "Leve o cogumelo à velha da loja de poções nos fundos de Kakariko. Ele estraga se demorar!", gItemIconOddPotionTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_ODD_POTION },
    { "Serrote", "Volte à Floresta Perdida e dê a poção à garota Kokiri que está no lugar do rapaz.", gItemIconPoachersSawTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_SAW },
    { "Espada Goron Quebrada", "Dê o serrote ao chefe dos carpinteiros, perto da tenda no Vale Gerudo.", gItemIconBrokenGoronsSwordTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_SWORD_BROKEN },
    { "Receita", "Leve a espada ao Biggoron, no topo da Montanha da Morte.", gItemIconPrescriptionTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_PRESCRIPTION },
    { "Sapo Olhudo", "Mostre a receita ao Rei Zora, depois de descongelá-lo com Fogo Azul.", gItemIconEyeballFrogTex, 0, QM_AGE_ADULT, QM_NEED_BLUE_FIRE, QM_CHK_ADULT_TRADE, 0, ITEM_FROG },
    { "Colírio", "Leve o sapo ao cientista do laboratório do Lago Hylia, antes do tempo acabar.", gItemIconEyeDropsTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_EYEDROPS },
    { "Recibo", "Corra até o Biggoron com o colírio antes do tempo acabar. Use a ocarina para viajar rápido.", gItemIconClaimCheckTex, 0, QM_AGE_ADULT, 0, QM_CHK_ADULT_TRADE, 0, ITEM_CLAIM_CHECK },
    { "Espada do Biggoron", "Mostre o recibo ao Biggoron. A espada fica pronta depois de alguns dias (durma ou toque a Canção do Sol).", gItemIconSwordBiggoronTex, 0, QM_AGE_ADULT, 0, QM_CHK_BIGGORON, 0, 0 },
};

typedef struct {
    const char* name;
    const char* icon; // 24x24 quest icon or 32x32 item icon (see iconIs24)
    u8 iconIs24;
    s8 point;    // world map point, or -1 for a dungeon
    s8 dungeon;  // index into the Gold Skulltula flags of the dungeons
    s8 total;
} QuestMenuSkullArea;

static const QuestMenuSkullArea sQuestMenuSkullAreas[] = {
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

static const QuestMenuEntry sQuestMenuSkullRewards[] = {
    { "10 fichas: Carteira de Adulto", "Casa das Skulltulas, em Kakariko. A carteira guarda até 200 rupias.", gItemIconAdultsWalletTex, 0, QM_AGE_ANY, 0, QM_CHK_EVENTCHKINF, 0, 0xDA },
    { "20 fichas: Pedra da Agonia", "A pedra treme o controle perto de grutas e segredos escondidos.", gQuestIconStoneOfAgonyTex, 1, QM_AGE_ANY, 0, QM_CHK_EVENTCHKINF, 0, 0xDB },
    { "30 fichas: Carteira de Gigante", "A carteira guarda até 500 rupias.", gItemIconGiantsWalletTex, 0, QM_AGE_ANY, 0, QM_CHK_EVENTCHKINF, 0, 0xDC },
    { "40 fichas: Bombchus", "Prêmio de 10 Bombchus.", gItemIconBombchuTex, 0, QM_AGE_ANY, 0, QM_CHK_EVENTCHKINF, 0, 0xDD },
    { "50 fichas: Pedaço de Coração", "Prêmio de um Pedaço de Coração.", gQuestIconHeartPieceTex, 1, QM_AGE_ANY, 0, QM_CHK_EVENTCHKINF, 0, 0xDE },
    { "100 fichas: Rupia Gigante", "A família volta ao normal. Depois disso, cada visita dá uma Rupia Gigante (200 rupias).", gQuestIconGoldSkulltulaTex, 1, QM_AGE_ANY, 0, QM_CHK_GS_TOKENS, 0, 100 },
};

static const s8 sQuestMenuHeartPoints[] = {
    QM_PT_WASTELAND, QM_PT_FORTRESS, QM_PT_FORTRESS, QM_PT_VALLEY, QM_PT_VALLEY, QM_PT_LAKE, QM_PT_LAKE, QM_PT_LAKE,
    QM_PT_LON_LON, QM_PT_MARKET, QM_PT_MARKET, QM_PT_MARKET, QM_PT_FIELD, QM_PT_FIELD, QM_PT_DEATH_MOUNTAIN,
    QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_KAKARIKO,
    QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_LOST_WOODS, QM_PT_LOST_WOODS,
    QM_PT_ZORA, QM_PT_ZORA, QM_PT_ZORA, QM_PT_ZORA, QM_PT_ZORA, QM_PT_ZORA, QM_PT_ZORA, QM_PT_ZORA,
};
static const s8 sQuestMenuBottlePoints[] = { QM_PT_LON_LON, QM_PT_KAKARIKO, QM_PT_LAKE, QM_PT_MARKET };
static const s8 sQuestMenuMaskPoints[] = {
    QM_PT_MARKET, QM_PT_MARKET, QM_PT_MARKET, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_LOST_WOODS,
    QM_PT_KAKARIKO, QM_PT_FIELD, QM_PT_MARKET, QM_PT_LOST_WOODS, QM_PT_LOST_WOODS,
};
static const s8 sQuestMenuTradePoints[] = {
    QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_LOST_WOODS, QM_PT_KAKARIKO, QM_PT_LOST_WOODS,
    QM_PT_VALLEY, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_LAKE, QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN,
};
// World map point of each dungeon, in the order of the Gold Skulltula flags.
static const s8 sQuestMenuDungeonPoints[] = {
    QM_PT_KOKIRI, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_LOST_WOODS, QM_PT_DEATH_MOUNTAIN,
    QM_PT_LAKE, QM_PT_WASTELAND, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_ZORA,
};

static const QuestMenuSection sQuestMenuHeartSections[] = {
    { 0, 0, "Deserto e Gerudo" }, { 5, 0, "Lago Hylia" }, { 8, 0, "Rancho e Mercado" }, { 12, 0, "Campo de Hyrule" },
    { 14, 0, "Montanha da Morte" }, { 18, 0, "Kakariko e Cemitério" }, { 26, 0, "Floresta Perdida" },
    { 28, 0, "Rio e Domínio Zora" }, { 35, 0, "Caverna de Gelo" }, { -1 },
};
static const QuestMenuSection sQuestMenuBottleSections[] = { { 0, 0, "Garrafas" }, { -1 } };
static const QuestMenuSection sQuestMenuMaskSections[] = {
    { 0, 0, "Abrir a Loja de Máscaras" }, { 4, 0, "Loja da Máscara Feliz" }, { 9, 1, "Teatro Deku" }, { -1 },
};
static const QuestMenuSection sQuestMenuTradeSections[] = { { 0, 0, "Troca de Adulto" }, { -1 } };
static const QuestMenuSection sQuestMenuSkullSections[] = {
    { 0, 0, "Regiões" }, { 12, 0, "Masmorras" }, { 22, 1, "Casa das Skulltulas" }, { -1 },
};


// ---------------------------------------------------------------------------------------------------------------------
// Personagens e NPCs

static const QuestMenuEntry sQuestMenuPeople[] = {
    { "Saria", "Amiga de infância do Link. No Prado Sagrado, no fundo da Floresta Perdida, ela ensina a Canção de Saria.", gItemIconOcarinaFairyTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_SARIA },
    { "Princesa Zelda", "Espera o Link no jardim do Castelo de Hyrule. Conta da ameaça de Ganondorf e entrega a sua carta.", gItemIconZeldasLetterTex, 0, QM_AGE_CHILD, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER },
    { "Impa", "Guardiã da Zelda. Na saída do jardim do castelo ela ensina a Canção de Ninar da Zelda.", gQMIconSongTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_LULLABY },
    { "Malon", "Filha do Talon, no Rancho Lon Lon. Cante com ela (ocarina) e ela ensina a Canção de Epona.", gItemIconBottleMilkFullTex, 0, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_EPONA },
    { "Darunia", "Chefe dos Gorons. Toque a Canção de Saria para ele dançar e ganhe o Bracelete Goron.", gItemIconGoronsBraceletTex, 0, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_UPGRADE, 0, QM_UPGRADE(UPG_STRENGTH, 1) },
    { "Princesa Ruto", "Princesa Zora, perdida na barriga do Lorde Jabu-Jabu. Leve-a até o fim e ela entrega a Safira Zora.", gQuestIconZoraSapphireTex, 1, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_ZORA_SAPPHIRE },
    { "Grande Fada do Poder", "No alto da Montanha da Morte, atrás da parede que se quebra com bomba. Toque a Canção de Ninar e ganhe a magia.", gQuestIconMagicJarSmallTex, 1, QM_AGE_CHILD, QM_NEED_BOMB | QM_NEED_OCARINA, QM_CHK_MAGIC, 0, 0 },
    { "Dampé", "O coveiro do Cemitério de Kakariko. Adulto, vença a corrida contra o fantasma dele e ganhe o Gancho.", gItemIconHookshotTex, 0, QM_AGE_ADULT, 0, QM_CHK_ITEM, 0, ITEM_HOOKSHOT },
    { "Sheik", "Um misterioso Sheikah. Ensina o Minueto da Floresta no Prado Sagrado e as outras canções de teletransporte.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_MINUET },
    { "Rauru", "O Sábio da Luz, na Câmara dos Sábios. Entrega o Medalhão da Luz quando Link desperta como adulto.", gQuestIconMedallionLightTex, 1, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_LIGHT },
    { "Rei Zora", "Preso no gelo do Domínio Zora. Descongele-o com Fogo Azul e ele dá a Túnica Zora.", gItemIconTunicZoraTex, 0, QM_AGE_ADULT, QM_NEED_BLUE_FIRE, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_ZORA) },
    { "Nabooru", "Líder das Gerudo, no Templo do Espírito. Ao ser libertada, desperta como Sábia do Espírito.", gQuestIconMedallionSpiritTex, 1, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_MEDALLION_SPIRIT },
    { "Biggoron", "O Goron gigante do topo da Montanha da Morte. No fim da troca de adulto ele forja a Espada de Biggoron.", gItemIconSwordBiggoronTex, 0, QM_AGE_ADULT, 0, QM_CHK_BIGGORON, 0, 0 },
};
static const s8 sQuestMenuPeoplePoints[] = {
    QM_PT_LOST_WOODS, QM_PT_MARKET, QM_PT_MARKET, QM_PT_LON_LON, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_DEATH_MOUNTAIN,
    QM_PT_KAKARIKO, QM_PT_LOST_WOODS, QM_PT_MARKET, QM_PT_ZORA, QM_PT_WASTELAND, QM_PT_DEATH_MOUNTAIN,
};
static const QuestMenuSection sQuestMenuPeopleSections[] = { { 0, 0, "Infância" }, { 7, 0, "Sete anos depois" }, { -1 } };

// ---------------------------------------------------------------------------------------------------------------------
// Locais descobertos

static const QuestMenuEntry sQuestMenuPlaces[] = {
    { "Floresta Kokiri", "A vila das crianças da floresta, onde Link cresceu.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 4 },
    { "Floresta Perdida", "Um labirinto de árvores. Siga a música de Saria para não se perder.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 10 },
    { "Prado Sagrado", "A clareira no fundo da Floresta Perdida, na frente do Templo da Floresta.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 5 },
    { "Campo de Hyrule", "O grande campo no centro do reino. Liga todas as regiões.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 0 },
    { "Rancho Lon Lon", "A fazenda do Talon e da Malon, no meio do Campo de Hyrule.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 19 },
    { "Mercado de Hyrule", "A praça da cidade do castelo, com lojas e minijogos.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 14 },
    { "Castelo de Hyrule", "O castelo da família real, ao norte do Mercado.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 15 },
    { "Vila Kakariko", "A vila dos Sheikah, no pé da Montanha da Morte.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 1 },
    { "Cemitério de Kakariko", "Onde descansam a família real e os Sheikah. O Dampé cuida dele.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 2 },
    { "Trilha da Montanha da Morte", "O caminho até a Cidade Goron e o topo do vulcão.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 16 },
    { "Cidade Goron", "A cidade dos Gorons, escavada dentro da montanha.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 18 },
    { "Cratera da Montanha da Morte", "O interior do vulcão, quente demais sem a Túnica Goron.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 17 },
    { "Rio Zora", "O rio que desce do Domínio Zora até o Campo de Hyrule.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 3 },
    { "Domínio Zora", "O reino dos Zoras, atrás da cachoeira.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 7 },
    { "Fonte Zora", "A fonte onde vive o Lorde Jabu-Jabu.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 8 },
    { "Lago Hylia", "O grande lago no sul do reino, com o laboratório e a Lagoa de Pesca.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 6 },
    { "Vale Gerudo", "O desfiladeiro com a ponte que leva à Fortaleza Gerudo.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 9 },
    { "Fortaleza Gerudo", "A fortaleza das ladras Gerudo.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 12 },
    { "Deserto Assombrado", "Um mar de areia. Siga as bandeiras e o fantasma para não se perder.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 13 },
    { "Colosso do Deserto", "A estátua gigante que guarda o Templo do Espírito.", gQMIconPlacesTex, 0, QM_AGE_ANY, 0, QM_CHK_AREA, 0, 11 },
    { "Árvore Deku", "Dentro do guardião da floresta.", gQuestIconKokiriEmeraldTex, 1, QM_AGE_CHILD, 0, QM_CHK_VISITED, SCENE_DEKU_TREE, 0 },
    { "Caverna Dodongo", "A caverna dos Dodongos, na Trilha da Montanha da Morte.", gQuestIconGoronRubyTex, 1, QM_AGE_CHILD, QM_NEED_BOMB, QM_CHK_VISITED, SCENE_DODONGOS_CAVERN, 0 },
    { "Barriga do Jabu-Jabu", "Dentro do peixe guardião dos Zoras.", gQuestIconZoraSapphireTex, 1, QM_AGE_CHILD, 0, QM_CHK_VISITED, SCENE_JABU_JABU, 0 },
    { "Templo da Floresta", "No Prado Sagrado. A entrada fica no alto: use o Gancho.", gQuestIconMedallionForestTex, 1, QM_AGE_ADULT, QM_NEED_HOOKSHOT, QM_CHK_VISITED, SCENE_FOREST_TEMPLE, 0 },
    { "Templo do Fogo", "Dentro da Cratera da Montanha da Morte.", gQuestIconMedallionFireTex, 1, QM_AGE_ADULT, 0, QM_CHK_VISITED, SCENE_FIRE_TEMPLE, 0 },
    { "Templo da Água", "No fundo do Lago Hylia. Afunde com as Botas de Ferro.", gQuestIconMedallionWaterTex, 1, QM_AGE_ADULT, QM_NEED_IRON_BOOTS, QM_CHK_VISITED, SCENE_WATER_TEMPLE, 0 },
    { "Templo das Sombras", "Embaixo do Cemitério de Kakariko.", gQuestIconMedallionShadowTex, 1, QM_AGE_ADULT, QM_NEED_LENS, QM_CHK_VISITED, SCENE_SHADOW_TEMPLE, 0 },
    { "Templo do Espírito", "Dentro do Colosso do Deserto.", gQuestIconMedallionSpiritTex, 1, QM_AGE_ANY, 0, QM_CHK_VISITED, SCENE_SPIRIT_TEMPLE, 0 },
    { "Fundo do Poço", "Embaixo do poço de Kakariko, que seca com a Canção das Tempestades.", gItemIconLensOfTruthTex, 0, QM_AGE_CHILD, 0, QM_CHK_VISITED, SCENE_BOTTOM_OF_THE_WELL, 0 },
    { "Caverna de Gelo", "Atrás da Fonte Zora congelada.", gItemIconBootsIronTex, 0, QM_AGE_ADULT, 0, QM_CHK_VISITED, SCENE_ICE_CAVERN, 0 },
    { "Campo de Treino Gerudo", "As provas das Gerudo, dentro da fortaleza.", gQuestIconGerudosCardTex, 1, QM_AGE_ADULT, 0, QM_CHK_VISITED, SCENE_GERUDO_TRAINING_GROUND, 0 },
    { "Castelo de Ganon", "Onde ficava o Castelo de Hyrule, sete anos depois.", gQuestIconDungeonBossKeyTex, 1, QM_AGE_ADULT, 0, QM_CHK_VISITED, SCENE_INSIDE_GANONS_CASTLE, 0 },
};
static const s8 sQuestMenuPlacesPoints[] = {
    QM_PT_KOKIRI, QM_PT_LOST_WOODS, QM_PT_LOST_WOODS, QM_PT_FIELD, QM_PT_LON_LON, QM_PT_MARKET, QM_PT_MARKET,
    QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA,
    QM_PT_ZORA, QM_PT_ZORA, QM_PT_LAKE, QM_PT_VALLEY, QM_PT_FORTRESS, QM_PT_WASTELAND, QM_PT_WASTELAND,
    QM_PT_KOKIRI, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_LOST_WOODS, QM_PT_DEATH_MOUNTAIN, QM_PT_LAKE, QM_PT_KAKARIKO,
    QM_PT_WASTELAND, QM_PT_KAKARIKO, QM_PT_ZORA, QM_PT_FORTRESS, QM_PT_MARKET,
};
static const QuestMenuSection sQuestMenuPlacesSections[] = { { 0, 0, "Regiões de Hyrule" }, { 20, 0, "Masmorras" }, { -1 } };

// ---------------------------------------------------------------------------------------------------------------------
// Itens importantes

static const QuestMenuEntry sQuestMenuItems[] = {
    { "Estilingue das Fadas", "Dentro da Árvore Deku, num baú no alto.", gItemIconSlingshotTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEM, 0, ITEM_SLINGSHOT },
    { "Bolsa de Bombas", "Na Caverna Dodongo, depois de vencer um Rei Lagarto.", gItemIconBombBag20Tex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEM, 0, ITEM_BOMB },
    { "Bracelete Goron", "Presente do Darunia, depois de você tocar a Canção de Saria para ele.", gItemIconGoronsBraceletTex, 0, QM_AGE_CHILD, QM_NEED_OCARINA, QM_CHK_UPGRADE, 0, QM_UPGRADE(UPG_STRENGTH, 1) },
    { "Escama de Prata", "Vença o jogo de mergulho do Domínio Zora (criança).", gItemIconScaleSilverTex, 0, QM_AGE_CHILD, 0, QM_CHK_UPGRADE, 0, QM_UPGRADE(UPG_SCALE, 1) },
    { "Bumerangue", "Na barriga do Lorde Jabu-Jabu.", gItemIconBoomerangTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEM, 0, ITEM_BOOMERANG },
    { "Magia", "Presente da Grande Fada do Poder, no alto da Montanha da Morte.", gQuestIconMagicJarSmallTex, 1, QM_AGE_ANY, QM_NEED_BOMB | QM_NEED_OCARINA, QM_CHK_MAGIC, 0, 0 },
    { "Fogo de Din", "Grande Fada perto do Castelo de Hyrule (abra o caminho com bomba e toque a Canção de Ninar).", gItemIconDinsFireTex, 0, QM_AGE_CHILD, QM_NEED_BOMB | QM_NEED_OCARINA, QM_CHK_ITEM, 0, ITEM_DINS_FIRE },
    { "Vento de Farore", "Grande Fada da Fonte Zora, atrás das pedras que se explodem.", gItemIconFaroresWindTex, 0, QM_AGE_CHILD, QM_NEED_BOMB | QM_NEED_OCARINA, QM_CHK_ITEM, 0, ITEM_FARORES_WIND },
    { "Lente da Verdade", "No Fundo do Poço, na sala do Coração Morto.", gItemIconLensOfTruthTex, 0, QM_AGE_CHILD, 0, QM_CHK_ITEM, 0, ITEM_LENS },
    { "Gancho", "Corrida do Dampé, dentro do túmulo dele (adulto).", gItemIconHookshotTex, 0, QM_AGE_ADULT, 0, QM_CHK_ITEM, 0, ITEM_HOOKSHOT },
    { "Arco das Fadas", "No Templo da Floresta.", gItemIconBowTex, 0, QM_AGE_ADULT, 0, QM_CHK_ITEM, 0, ITEM_BOW },
    { "Túnica Goron", "Presente do filho do Darunia, na Cidade Goron (adulto).", gItemIconTunicGoronTex, 0, QM_AGE_ADULT, 0, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_GORON) },
    { "Martelo Megaton", "No Templo do Fogo.", gItemIconHammerTex, 0, QM_AGE_ADULT, 0, QM_CHK_ITEM, 0, ITEM_HAMMER },
    { "Botas de Ferro", "No fim da Caverna de Gelo.", gItemIconBootsIronTex, 0, QM_AGE_ADULT, 0, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON) },
    { "Gancho Longo", "No Templo da Água, depois de vencer o Link Sombrio.", gItemIconLongshotTex, 0, QM_AGE_ADULT, QM_NEED_IRON_BOOTS, QM_CHK_ITEM, 0, ITEM_LONGSHOT },
    { "Botas Flutuantes", "No Templo das Sombras.", gItemIconBootsHoverTex, 0, QM_AGE_ADULT, QM_NEED_LENS, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_HOVER) },
    { "Luvas de Prata", "No Templo do Espírito, pela entrada da criança.", gItemIconSilverGauntletsTex, 0, QM_AGE_CHILD, 0, QM_CHK_UPGRADE, 0, QM_UPGRADE(UPG_STRENGTH, 2) },
    { "Escudo Espelhado", "No Templo do Espírito, nas mãos do Colosso (adulto).", gItemIconShieldMirrorTex, 0, QM_AGE_ADULT, 0, QM_CHK_EQUIP, 0, QM_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_MIRROR) },
    { "Amor de Nayru", "Grande Fada do Colosso do Deserto (criança, abra com bomba).", gItemIconNayrusLoveTex, 0, QM_AGE_CHILD, QM_NEED_BOMB | QM_NEED_OCARINA, QM_CHK_ITEM, 0, ITEM_NAYRUS_LOVE },
};
static const s8 sQuestMenuItemsPoints[] = {
    QM_PT_KOKIRI, QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_ZORA, QM_PT_DEATH_MOUNTAIN, QM_PT_MARKET,
    QM_PT_ZORA, QM_PT_KAKARIKO, QM_PT_KAKARIKO, QM_PT_LOST_WOODS, QM_PT_DEATH_MOUNTAIN, QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA,
    QM_PT_LAKE, QM_PT_KAKARIKO, QM_PT_WASTELAND, QM_PT_WASTELAND, QM_PT_WASTELAND,
};
static const QuestMenuSection sQuestMenuItemsSections[] = { { 0, 0, "Infância" }, { 9, 0, "Sete anos depois" }, { -1 } };

// ---------------------------------------------------------------------------------------------------------------------
// Registros e documentos

static const QuestMenuEntry sQuestMenuRecords[] = {
    { "Canção de Ninar da Zelda", "Impa ensina, na saída do jardim do castelo. Abre portas com o símbolo da família real.", gQMIconSongTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_LULLABY },
    { "Canção de Epona", "Malon ensina, no Rancho Lon Lon. Chama a Epona quando Link é adulto.", gQMIconSongTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_EPONA },
    { "Canção de Saria", "Saria ensina, no Prado Sagrado. Fale com ela de qualquer lugar.", gQMIconSongTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_SARIA },
    { "Canção do Sol", "No Túmulo Real do Cemitério de Kakariko. Faz o dia virar noite e paralisa os ReDeads.", gQMIconSongTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_SUN },
    { "Canção do Tempo", "Zelda ensina, na ponte do castelo, junto com a Ocarina do Tempo.", gQMIconSongTex, 0, QM_AGE_CHILD, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_TIME },
    { "Canção das Tempestades", "O homem do moinho de Kakariko ensina (adulto). Criança, toque-a lá para secar o poço.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_STORMS },
    { "Minueto da Floresta", "Sheik ensina, no Prado Sagrado. Leva ao Templo da Floresta.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_MINUET },
    { "Bolero do Fogo", "Sheik ensina, na entrada da Cratera da Montanha da Morte.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_BOLERO },
    { "Serenata da Água", "Sheik ensina, no fim da Caverna de Gelo.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_SERENADE },
    { "Noturno das Sombras", "Sheik ensina, em Kakariko, depois do Templo do Fogo, da Água e da Floresta.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_NOCTURNE },
    { "Réquiem do Espírito", "Sheik ensina, na entrada do Colosso do Deserto.", gQMIconSongTex, 0, QM_AGE_ANY, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_REQUIEM },
    { "Prelúdio da Luz", "Sheik ensina, no Templo do Tempo, depois do Templo da Floresta.", gQMIconSongTex, 0, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_SONG_PRELUDE },
    { "Carta da Zelda", "Mostre ao guarda do portão da Montanha da Morte para passar e abrir a Loja de Máscaras.", gItemIconZeldasLetterTex, 0, QM_AGE_CHILD, 0, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_ZELDAS_LETTER },
    { "Carta de Ruto", "Pedido de socorro da Princesa Ruto, numa garrafa no Lago Hylia. Mostre ao Rei Zora.", gItemIconBottleRutosLetterTex, 0, QM_AGE_CHILD, QM_NEED_SILVER_SCALE, QM_CHK_EVENTCHKINF, 0, EVENTCHKINF_OBTAINED_RUTOS_LETTER },
    { "Cartão Gerudo", "Liberte os carpinteiros presos na Fortaleza Gerudo. Abre o portão e o Campo de Treino.", gQuestIconGerudosCardTex, 1, QM_AGE_ADULT, 0, QM_CHK_QUEST_ITEM, 0, QUEST_GERUDO_CARD },
    { "Pedra da Agonia", "Prêmio de 20 fichas da Casa das Skulltulas. Treme perto de segredos escondidos.", gQuestIconStoneOfAgonyTex, 1, QM_AGE_ANY, 0, QM_CHK_QUEST_ITEM, 0, QUEST_STONE_OF_AGONY },
};
static const s8 sQuestMenuRecordsPoints[] = {
    QM_PT_MARKET, QM_PT_LON_LON, QM_PT_LOST_WOODS, QM_PT_KAKARIKO, QM_PT_MARKET, QM_PT_KAKARIKO, QM_PT_LOST_WOODS,
    QM_PT_DEATH_MOUNTAIN, QM_PT_ZORA, QM_PT_KAKARIKO, QM_PT_WASTELAND, QM_PT_MARKET, QM_PT_DEATH_MOUNTAIN, QM_PT_LAKE,
    QM_PT_FORTRESS, QM_PT_KAKARIKO,
};
static const QuestMenuSection sQuestMenuRecordsSections[] = { { 0, 0, "Partituras" }, { 12, 1, "Documentos" }, { -1 } };

// The tables that run in parallel must have the same length.
typedef char QuestMenuCheckMain[(ARRAY_COUNT(sQuestMenuMainPoints) == ARRAY_COUNT(sQuestMenuMain)) ? 1 : -1];
typedef char QuestMenuCheckHearts[(ARRAY_COUNT(sQuestMenuHeartPoints) == ARRAY_COUNT(sQuestMenuHearts)) ? 1 : -1];
typedef char QuestMenuCheckBottles[(ARRAY_COUNT(sQuestMenuBottlePoints) == ARRAY_COUNT(sQuestMenuBottles)) ? 1 : -1];
typedef char QuestMenuCheckMasks[(ARRAY_COUNT(sQuestMenuMaskPoints) == ARRAY_COUNT(sQuestMenuMasks)) ? 1 : -1];
typedef char QuestMenuCheckTrade[(ARRAY_COUNT(sQuestMenuTradePoints) == ARRAY_COUNT(sQuestMenuTrade)) ? 1 : -1];
typedef char QuestMenuCheckPeople[(ARRAY_COUNT(sQuestMenuPeoplePoints) == ARRAY_COUNT(sQuestMenuPeople)) ? 1 : -1];
typedef char QuestMenuCheckPlaces[(ARRAY_COUNT(sQuestMenuPlacesPoints) == ARRAY_COUNT(sQuestMenuPlaces)) ? 1 : -1];
typedef char QuestMenuCheckItems[(ARRAY_COUNT(sQuestMenuItemsPoints) == ARRAY_COUNT(sQuestMenuItems)) ? 1 : -1];
typedef char QuestMenuCheckRecords[(ARRAY_COUNT(sQuestMenuRecordsPoints) == ARRAY_COUNT(sQuestMenuRecords)) ? 1 : -1];

#define QM_SKULL_AREA_DETAIL "Skulltulas de Ouro desta região. Muitas só aparecem à noite: siga o barulho delas. Use o Bumerangue ou o Gancho para pegar as fichas de longe."
#define QM_SKULL_DUNGEON_DETAIL "Skulltulas de Ouro dentro desta masmorra. Procure atrás de paredes que racham, no alto e em salas escondidas."

// ---------------------------------------------------------------------------------------------------------------------
// Categories

const char* gQuestMenuCategoryNames[QM_CAT_COUNT] = {
    "Principais", "Secundárias", "Personagens", "Locais", "Itens", "Registros",
};

// Text of the plaque at the bottom of the journal.
const char* gQuestMenuCategoryHints[QM_CAT_COUNT] = {
    "Acompanhe a história principal.",
    "Corações, garrafas, máscaras e Skulltulas.",
    "Quem o Link já conheceu e ajudou.",
    "Regiões e masmorras já visitadas.",
    "Os itens que abrem caminho em Hyrule.",
    "Partituras e documentos do Link.",
};

// NULL: the Triforce, drawn by the interface.
const char* gQuestMenuCategoryIcons[QM_CAT_COUNT] = {
    NULL, gQMIconSideTex, gQMIconPeopleTex, gQMIconPlacesTex, gQMIconItemsTex, gQMIconRecordsTex,
};

// ---------------------------------------------------------------------------------------------------------------------
// Save checks

static bool QuestMenu_SceneCollect(PlayState* play, s16 scene, u16 flag) {
    if (scene == play->sceneNum) {
        return Flags_GetCollectible(play, flag);
    }
    return (gSaveContext.sceneFlags[scene].collect & (1 << flag)) != 0;
}

static bool QuestMenu_HasBottleOf(u8 item) {
    s32 slot;

    for (slot = SLOT_BOTTLE_1; slot <= SLOT_BOTTLE_4; slot++) {
        if (gSaveContext.inventory.items[slot] == item) {
            return true;
        }
    }
    return false;
}

static const char* QuestMenu_NeedIcon(s32 need) {
    switch (1 << need) {
        case QM_NEED_BEAN: return gItemIconMagicBeanTex;
        case QM_NEED_HOOKSHOT: return gItemIconHookshotTex;
        case QM_NEED_LONGSHOT: return gItemIconLongshotTex;
        case QM_NEED_BOMB: return gItemIconBombTex;
        case QM_NEED_BOW: return gItemIconBowTex;
        case QM_NEED_IRON_BOOTS: return gItemIconBootsIronTex;
        case QM_NEED_HOVER_BOOTS: return gItemIconBootsHoverTex;
        case QM_NEED_SILVER_SCALE: return gItemIconScaleSilverTex;
        case QM_NEED_GOLD_SCALE: return gItemIconScaleGoldenTex;
        case QM_NEED_LENS: return gItemIconLensOfTruthTex;
        case QM_NEED_OCARINA: return gItemIconOcarinaFairyTex;
        case QM_NEED_BOOMERANG: return gItemIconBoomerangTex;
        case QM_NEED_ZORA_TUNIC: return gItemIconTunicZoraTex;
        case QM_NEED_BLUE_FIRE: return gItemIconBottleBlueFireTex;
        case QM_NEED_DEKU_STICK: return gItemIconDekuStickTex;
        case QM_NEED_NIGHT: return gMoonIconTex;
        case QM_NEED_DAY: return gSunIconTex;
    }
    return NULL;
}

static bool QuestMenu_HasNeed(s32 need) {
    switch (1 << need) {
        case QM_NEED_BEAN: return INV_CONTENT(ITEM_BEAN) == ITEM_BEAN;
        case QM_NEED_HOOKSHOT: return INV_CONTENT(ITEM_HOOKSHOT) != ITEM_NONE;
        case QM_NEED_LONGSHOT: return INV_CONTENT(ITEM_LONGSHOT) == ITEM_LONGSHOT;
        case QM_NEED_BOMB: return INV_CONTENT(ITEM_BOMB) == ITEM_BOMB;
        case QM_NEED_BOW: return INV_CONTENT(ITEM_BOW) == ITEM_BOW;
        case QM_NEED_IRON_BOOTS: return CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON) != 0;
        case QM_NEED_HOVER_BOOTS: return CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_HOVER) != 0;
        case QM_NEED_SILVER_SCALE: return CUR_UPG_VALUE(UPG_SCALE) >= 1;
        case QM_NEED_GOLD_SCALE: return CUR_UPG_VALUE(UPG_SCALE) >= 2;
        case QM_NEED_LENS: return INV_CONTENT(ITEM_LENS) == ITEM_LENS;
        case QM_NEED_OCARINA: return INV_CONTENT(ITEM_OCARINA_FAIRY) != ITEM_NONE;
        case QM_NEED_BOOMERANG: return INV_CONTENT(ITEM_BOOMERANG) == ITEM_BOOMERANG;
        case QM_NEED_ZORA_TUNIC: return CHECK_OWNED_EQUIP(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_ZORA) != 0;
        case QM_NEED_BLUE_FIRE: return QuestMenu_HasBottleOf(ITEM_BLUE_FIRE);
        case QM_NEED_DEKU_STICK: return INV_CONTENT(ITEM_STICK) == ITEM_STICK;
        // Lit when it is the right time now.
        case QM_NEED_NIGHT: return IS_NIGHT;
        case QM_NEED_DAY: return IS_DAY;
    }
    return false;
}


static bool QuestMenu_Check(PlayState* play, u8 check, s16 scene, u16 flag) {
    u8 item;

    switch (check) {
        case QM_CHK_HEART_PIECE:
            return KaleidoScope_MapCollectHeartPieceTaken(play, flag);
        case QM_CHK_COLLECT:
            return QuestMenu_SceneCollect(play, scene, flag);
        case QM_CHK_ITEMGETINF:
            return GET_ITEMGETINF(flag) != 0;
        case QM_CHK_INFTABLE:
            return GET_INFTABLE(flag) != 0;
        case QM_CHK_EVENTCHKINF:
            return GET_EVENTCHKINF(flag) != 0;
        case QM_CHK_POE_POINTS:
            return (HIGH_SCORE(HS_POE_POINTS) >= 1000) || Flags_GetRandomizerInf(RAND_INF_10_BIG_POES);
        case QM_CHK_CHILD_TRADE:
            item = INV_CONTENT(ITEM_TRADE_CHILD);
            return ((item != ITEM_NONE) && (item >= flag) && (item <= ITEM_SOLD_OUT)) ||
                   GET_EVENTCHKINF(EVENTCHKINF_TALON_WOKEN_IN_CASTLE);
        case QM_CHK_ADULT_TRADE:
            item = INV_CONTENT(ITEM_TRADE_ADULT);
            return ((item != ITEM_NONE) && (item >= flag) && (item <= ITEM_CLAIM_CHECK)) ||
                   (gSaveContext.bgsFlag != 0);
        case QM_CHK_BIGGORON:
            return gSaveContext.bgsFlag != 0;
        case QM_CHK_GS_TOKENS:
            return gSaveContext.inventory.gsTokens >= flag;
        case QM_CHK_QUEST_ITEM:
            return CHECK_QUEST_ITEM(flag) != 0;
        case QM_CHK_ITEM:
            return INV_CONTENT(flag) == flag;
        case QM_CHK_MASTER_SWORD:
            return CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER) != 0;
        case QM_CHK_MEDALLIONS:
            return CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST) && CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE) &&
                   CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER) && CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT) &&
                   CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW) && CHECK_QUEST_ITEM(QUEST_MEDALLION_LIGHT);
        case QM_CHK_EQUIP:
            return CHECK_OWNED_EQUIP(flag >> 4, flag & 0xF) != 0;
        case QM_CHK_UPGRADE:
            return CUR_UPG_VALUE(flag >> 4) >= (flag & 0xF);
        case QM_CHK_MAGIC:
            return gSaveContext.isMagicAcquired != 0;
        case QM_CHK_AREA:
            return (gSaveContext.worldMapAreaData & gBitFlags[flag]) != 0;
        case QM_CHK_VISITED:
            return (play->sceneNum == scene) || (gSaveContext.sceneFlags[scene].rooms != 0);
        case QM_CHK_BOSS_KEY:
            return CHECK_DUNGEON_ITEM(DUNGEON_KEY_BOSS, scene) != 0;
    }
    return false;
}

static bool QuestMenu_IsDone(PlayState* play, const QuestMenuEntry* e) {
    return QuestMenu_Check(play, e->check, e->scene, e->flag);
}

static const char* QuestMenu_NeedName(s32 need) {
    switch (1 << need) {
        case QM_NEED_BEAN: return "Feijão Mágico";
        case QM_NEED_HOOKSHOT: return "Gancho";
        case QM_NEED_LONGSHOT: return "Gancho Longo";
        case QM_NEED_BOMB: return "Bombas";
        case QM_NEED_BOW: return "Arco";
        case QM_NEED_IRON_BOOTS: return "Botas de Ferro";
        case QM_NEED_HOVER_BOOTS: return "Botas Flutuantes";
        case QM_NEED_SILVER_SCALE: return "Escama de Prata";
        case QM_NEED_GOLD_SCALE: return "Escama de Ouro";
        case QM_NEED_LENS: return "Lente da Verdade";
        case QM_NEED_OCARINA: return "Ocarina";
        case QM_NEED_BOOMERANG: return "Bumerangue";
        case QM_NEED_ZORA_TUNIC: return "Túnica Zora";
        case QM_NEED_BLUE_FIRE: return "Fogo Azul";
        case QM_NEED_DEKU_STICK: return "Bastão Deku";
        case QM_NEED_NIGHT: return "À noite";
        case QM_NEED_DAY: return "De dia";
    }
    return "";
}

// ---------------------------------------------------------------------------------------------------------------------
// Lines

static void QuestMenu_SetSection(QuestMenuRow* row, const QuestMenuSection* sections, s32 index) {
    row->section = NULL;
    row->sectionStyle = 0;
    for (; (sections != NULL) && (sections->start >= 0) && (sections->start <= index); sections++) {
        row->section = sections->title;
        row->sectionStyle = sections->style;
    }
}

// Adds the lines of a table. In a `chained` table (a story), a line is locked until the one before it is done.
static s32 QuestMenu_AddEntries(PlayState* play, QuestMenuRow* rows, s32 n, s32 max, s32 category,
                                const QuestMenuEntry* entries, s32 count, const s8* points,
                                const QuestMenuSection* sections, s32 sectionBase, bool chained) {
    s32 i;

    for (i = 0; (i < count) && (n < max); i++, n++) {
        QuestMenuRow* row = &rows[n];

        memset(row, 0, sizeof(*row));
        row->id = category * 1000 + n;
        row->entry = &entries[i];
        row->name = entries[i].name;
        row->detail = entries[i].detail;
        row->icon = entries[i].icon;
        row->iconIs24 = entries[i].iconIs24;
        row->age = entries[i].age;
        row->needs = entries[i].needs;
        row->done = QuestMenu_IsDone(play, &entries[i]);
        row->point = (points != NULL) ? points[i] : QM_PT_NONE;
        QuestMenu_SetSection(row, sections, sectionBase + i);
        // State: done, locked behind the previous step of a story, started (some objectives done), or available.
        if (row->done) {
            row->state = QM_STATE_DONE;
        } else if (chained && (i > 0) && !rows[n - 1].done) {
            row->state = QM_STATE_LOCKED;
        } else {
            const QuestMenuObjective* o;
            bool started = false;

            for (o = entries[i].objectives; (o != NULL) && (o->label != NULL); o++) {
                started |= QuestMenu_Check(play, o->check, o->scene, o->flag);
            }
            row->state = started ? QM_STATE_IN_PROGRESS : QM_STATE_AVAILABLE;
        }
    }
    return n;
}

static s32 QuestMenu_PopCount(u32 v) {
    s32 n = 0;

    while (v != 0) {
        n += v & 1;
        v >>= 1;
    }
    return n;
}

// The temple of the first medallion still missing, for the map of "Desperte os Sábios".
static s8 QuestMenu_NextTemplePoint(void) {
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST)) {
        return QM_PT_LOST_WOODS;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE)) {
        return QM_PT_DEATH_MOUNTAIN;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER)) {
        return QM_PT_LAKE;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW)) {
        return QM_PT_KAKARIKO;
    }
    if (!CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT)) {
        return QM_PT_WASTELAND;
    }
    return QM_PT_MARKET;
}

static s32 QuestMenu_AddSkulltulas(PlayState* play, QuestMenuRow* rows, s32 n, s32 max) {
    s32 i;
    s32 first = n;

    for (i = 0; (i < ARRAY_COUNT(sQuestMenuSkullAreas)) && (n < max); i++, n++) {
        const QuestMenuSkullArea* area = &sQuestMenuSkullAreas[i];
        QuestMenuRow* row = &rows[n];

        memset(row, 0, sizeof(*row));
        row->id = QM_CAT_SIDE * 1000 + n;
        row->name = area->name;
        row->icon = area->icon;
        row->iconIs24 = area->iconIs24;
        row->age = QM_AGE_ANY;
        row->hasCount = true;
        if (area->point >= 0) {
            KaleidoScope_MapCollectSkulltulas(play, area->point, &row->got, &row->total);
            row->detail = QM_SKULL_AREA_DETAIL;
            row->point = area->point;
        } else {
            row->total = area->total;
            row->got = MIN(QuestMenu_PopCount(GET_GS_FLAGS(area->dungeon)), area->total);
            row->detail = QM_SKULL_DUNGEON_DETAIL;
            row->point = sQuestMenuDungeonPoints[area->dungeon];
        }
        row->done = row->got >= row->total;
        row->state = row->done ? QM_STATE_DONE : ((row->got > 0) ? QM_STATE_IN_PROGRESS : QM_STATE_AVAILABLE);
        QuestMenu_SetSection(row, sQuestMenuSkullSections, i);
    }
    n = QuestMenu_AddEntries(play, rows, n, max, QM_CAT_SIDE, sQuestMenuSkullRewards, ARRAY_COUNT(sQuestMenuSkullRewards),
                             NULL, sQuestMenuSkullSections, n - first, false);
    for (i = n - ARRAY_COUNT(sQuestMenuSkullRewards); i < n; i++) {
        rows[i].point = QM_PT_KAKARIKO;
    }
    return n;
}

s32 QuestMenu_BuildRows(PlayState* play, s32 category, QuestMenuRow* rows, s32 max) {
    s32 n = 0;

    switch (category) {
        case QM_CAT_MAIN:
            n = QuestMenu_AddEntries(play, rows, 0, max, category, sQuestMenuMain, ARRAY_COUNT(sQuestMenuMain),
                                     sQuestMenuMainPoints, sQuestMenuMainSections, 0, true);
            rows[6].point = QuestMenu_NextTemplePoint();
            break;
        case QM_CAT_SIDE:
            n = QuestMenu_AddEntries(play, rows, n, max, category, sQuestMenuMasks, ARRAY_COUNT(sQuestMenuMasks),
                                     sQuestMenuMaskPoints, sQuestMenuMaskSections, 0, true);
            n = QuestMenu_AddEntries(play, rows, n, max, category, sQuestMenuTrade, ARRAY_COUNT(sQuestMenuTrade),
                                     sQuestMenuTradePoints, sQuestMenuTradeSections, 0, true);
            n = QuestMenu_AddEntries(play, rows, n, max, category, sQuestMenuBottles, ARRAY_COUNT(sQuestMenuBottles),
                                     sQuestMenuBottlePoints, sQuestMenuBottleSections, 0, false);
            n = QuestMenu_AddEntries(play, rows, n, max, category, sQuestMenuHearts, ARRAY_COUNT(sQuestMenuHearts),
                                     sQuestMenuHeartPoints, sQuestMenuHeartSections, 0, false);
            n = QuestMenu_AddSkulltulas(play, rows, n, max);
            break;
        case QM_CAT_PEOPLE:
            n = QuestMenu_AddEntries(play, rows, 0, max, category, sQuestMenuPeople, ARRAY_COUNT(sQuestMenuPeople),
                                     sQuestMenuPeoplePoints, sQuestMenuPeopleSections, 0, false);
            break;
        case QM_CAT_PLACES:
            n = QuestMenu_AddEntries(play, rows, 0, max, category, sQuestMenuPlaces, ARRAY_COUNT(sQuestMenuPlaces),
                                     sQuestMenuPlacesPoints, sQuestMenuPlacesSections, 0, false);
            break;
        case QM_CAT_ITEMS:
            n = QuestMenu_AddEntries(play, rows, 0, max, category, sQuestMenuItems, ARRAY_COUNT(sQuestMenuItems),
                                     sQuestMenuItemsPoints, sQuestMenuItemsSections, 0, false);
            break;
        case QM_CAT_RECORDS:
            n = QuestMenu_AddEntries(play, rows, 0, max, category, sQuestMenuRecords, ARRAY_COUNT(sQuestMenuRecords),
                                     sQuestMenuRecordsPoints, sQuestMenuRecordsSections, 0, false);
            break;
    }
    return n;
}

// The objectives of a line: its own list, or else the age and the items it needs, then the line itself.
s32 QuestMenu_GetGoals(PlayState* play, const QuestMenuRow* row, QuestMenuGoal* goals, s32 max) {
    s32 n = 0;
    s32 need;

    if ((row->entry != NULL) && (row->entry->objectives != NULL)) {
        const QuestMenuObjective* o;

        for (o = row->entry->objectives; (o->label != NULL) && (n < max); o++, n++) {
            goals[n].label = o->label;
            goals[n].icon = o->icon;
            goals[n].iconIs24 = o->iconIs24;
            goals[n].done = QuestMenu_Check(play, o->check, o->scene, o->flag);
        }
        return n;
    }
    if (row->hasCount) {
        return 0;
    }
    if ((row->age != QM_AGE_ANY) && (n < max)) {
        bool adult = row->age == QM_AGE_ADULT;

        goals[n].label = adult ? "Link adulto" : "Link criança";
        goals[n].icon = adult ? gItemIconSwordMasterTex : gItemIconSwordKokiriTex;
        goals[n].iconIs24 = false;
        goals[n].done = row->done || (adult == (LINK_IS_ADULT != 0));
        n++;
    }
    for (need = 0; (need < QM_NEED_COUNT) && (n < max - 1); need++) {
        if (row->needs & (1 << need)) {
            goals[n].label = QuestMenu_NeedName(need);
            goals[n].icon = QuestMenu_NeedIcon(need);
            goals[n].iconIs24 = false;
            goals[n].done = row->done || QuestMenu_HasNeed(need);
            n++;
        }
    }
    if (n < max) {
        goals[n].label = row->name;
        goals[n].icon = row->icon;
        goals[n].iconIs24 = row->iconIs24;
        goals[n].done = row->done;
        n++;
    }
    return n;
}

const char* QuestMenu_StateName(u8 state) {
    switch (state) {
        case QM_STATE_LOCKED: return "Bloqueada";
        case QM_STATE_AVAILABLE: return "Disponível";
        case QM_STATE_IN_PROGRESS: return "Em andamento";
        case QM_STATE_DONE: return "Concluída";
    }
    return "";
}

// Spiritual Stones, medallions and the other dungeon icons are drawn on the map instead of Link's head.
bool QuestMenu_IsDungeonIcon(const char* icon) {
    return (icon == gQuestIconKokiriEmeraldTex) || (icon == gQuestIconGoronRubyTex) ||
           (icon == gQuestIconZoraSapphireTex) || (icon == gQuestIconMedallionForestTex) ||
           (icon == gQuestIconMedallionFireTex) || (icon == gQuestIconMedallionWaterTex) ||
           (icon == gQuestIconMedallionShadowTex) || (icon == gQuestIconMedallionSpiritTex) ||
           (icon == gQuestIconMedallionLightTex);
}
