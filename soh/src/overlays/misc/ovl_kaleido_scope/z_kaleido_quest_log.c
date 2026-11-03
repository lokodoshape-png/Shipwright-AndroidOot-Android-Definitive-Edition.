// OOT D.E.: "Missões", a quest log page inside the pause menu.
//
// On the Quest Status page, C-Up flips the page over to the quest log (C-Up or B flips it back). The page uses the game's own
// page background, font and item icons, and turns with the pause cube like the other pages. It lists, with the
// progress read from the save every frame:
//   - the 36 Pieces of Heart, with where they are and what they need (Magic Bean, Hookshot, ...);
//   - the 4 bottles;
//   - the mask quest (Weird Egg to Mask of Truth) and the Deku Theater;
//   - the adult trading sequence (Pocket Egg to Biggoron's Sword);
//   - the Gold Skulltulas per region and dungeon, and the rewards of the House of Skulltula.
// Left/right (stick, D-pad or C-Left/C-Right) change the list, up/down move the cursor, B goes back, and the box at
// the bottom explains the selected line. The icons on the right of a line are what it needs: in color when Link already has it.
//
// The texts are in Portuguese and use the font codes of the PT-BR translation mods (SolidOne / Letras), which redraw
// unused accented letters of the game font as Á Ã Í Õ Ó Ú ã í õ ó ú.
#include "z_kaleido_scope.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
#include "textures/icon_item_dungeon_static/icon_item_dungeon_static.h"
#include "textures/message_static/message_static.h"
#include "soh_assets.h"
#include "soh/ShipUtils.h"
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

#define QL_SKULL_AREA_DETAIL "Skulltulas de Ouro desta região. Muitas só aparecem à noite: siga o barulho delas. Use o Bumerangue ou o Gancho para pegar as fichas de longe."
#define QL_SKULL_DUNGEON_DETAIL "Skulltulas de Ouro dentro desta masmorra. Procure atrás de paredes que racham, no alto e em salas escondidas."

typedef enum {
    QL_CAT_HEARTS,
    QL_CAT_BOTTLES,
    QL_CAT_MASKS,
    QL_CAT_TRADE,
    QL_CAT_SKULLS,
    QL_CAT_COUNT,
} QuestLogCategory;

static const char* sQuestLogCategoryNames[QL_CAT_COUNT] = {
    "Pedaços de Coração", "Garrafas", "Máscaras", "Troca de Adulto", "Skulltulas de Ouro",
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
} QuestLogRow;

#define QL_MAX_ROWS 40

static QuestLogRow sQuestLogRows[QL_MAX_ROWS];
static s32 sQuestLogCategory = QL_CAT_HEARTS;
static s32 sQuestLogCursor[QL_CAT_COUNT];
static s32 sQuestLogTop[QL_CAT_COUNT];
static s32 sQuestLogFrames = 0;
static s16 sQuestLogSavedCursorItem = PAUSE_ITEM_NONE;

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

static s32 QuestLog_AddEntries(PlayState* play, s32 n, const QuestLogEntry* entries, s32 count) {
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

// Fills sQuestLogRows with the lines of a category; returns the number of lines and the category's progress.
static s32 QuestLog_BuildRows(PlayState* play, s32 category, s16* got, s16* total) {
    s32 n = 0;
    s32 i;

    switch (category) {
        case QL_CAT_HEARTS:
            n = QuestLog_AddEntries(play, 0, sQuestLogHearts, ARRAY_COUNT(sQuestLogHearts));
            break;
        case QL_CAT_BOTTLES:
            n = QuestLog_AddEntries(play, 0, sQuestLogBottles, ARRAY_COUNT(sQuestLogBottles));
            break;
        case QL_CAT_MASKS:
            n = QuestLog_AddEntries(play, 0, sQuestLogMasks, ARRAY_COUNT(sQuestLogMasks));
            break;
        case QL_CAT_TRADE:
            n = QuestLog_AddEntries(play, 0, sQuestLogTrade, ARRAY_COUNT(sQuestLogTrade));
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
                } else {
                    row->total = area->total;
                    row->got = MIN(QuestLog_PopCount(GET_GS_FLAGS(area->dungeon)), area->total);
                    row->detail = QL_SKULL_DUNGEON_DETAIL;
                }
                row->done = row->got >= row->total;
            }
            n = QuestLog_AddEntries(play, n, sQuestLogSkullRewards, ARRAY_COUNT(sQuestLogSkullRewards));
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
// Everything is drawn in "local units": half a pixel of the page, with y growing downwards from the top left corner
// of the page's content area (x -108, y 58 of the page). The content area is 432 x 270 units; the title band is above
// it, from y -44 to 0.

#define QL_W 432
#define QL_H 270

#define QL_GFX_MAX 14000
#define QL_VTX_MAX 6000

static Gfx sQuestLogGfx[QL_GFX_MAX];
static Vtx sQuestLogVtx[QL_VTX_MAX];
static Vtx sQuestLogTitleVtx[4];

typedef struct {
    Gfx* gfx;
    Vtx* vtx;
    u8 alpha;
    u8 combine; // 0 = none set, 1 = textured, 2 = solid
} QuestLogDraw;

static bool QuestLog_HasRoom(QuestLogDraw* d) {
    return (d->gfx + 24 < &sQuestLogGfx[QL_GFX_MAX]) && (d->vtx + 4 <= &sQuestLogVtx[QL_VTX_MAX]);
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

// A flat golden Triforce, `size` units wide, with its top at (x, y).
static void QuestLog_Triforce(QuestLogDraw* d, s16 x, s16 y, s16 size, u8 alpha) {
    s16 half = size / 2;
    s16 h = size * 7 / 8;
    s16 hh = h / 2;
    Vtx* v = d->vtx;
    s32 i;
    // Three triangles: top, bottom left, bottom right.
    s16 pts[9][2] = {
        { x + half, y }, { x + half / 2, y + hh }, { x + half + half / 2, y + hh },
        { x + half / 2, y + hh }, { x, y + h }, { x + half, y + h },
        { x + half + half / 2, y + hh }, { x + half, y + h }, { x + size, y + h },
    };

    if ((d->vtx + 9 > &sQuestLogVtx[QL_VTX_MAX]) || (d->gfx + 24 >= &sQuestLogGfx[QL_GFX_MAX])) {
        return;
    }
    for (i = 0; i < 9; i++) {
        v[i].v.ob[0] = pts[i][0];
        v[i].v.ob[1] = pts[i][1];
        v[i].v.ob[2] = 0;
        v[i].v.flag = 0;
        v[i].v.tc[0] = v[i].v.tc[1] = 0;
        v[i].v.cn[0] = v[i].v.cn[1] = v[i].v.cn[2] = v[i].v.cn[3] = 255;
    }
    QuestLog_Combine(d, 2);
    QuestLog_SetColor(d, 255, 200, 40, alpha);
    gSPVertex(d->gfx++, v, 9, 0);
    gSP1Triangle(d->gfx++, 0, 1, 2, 0);
    gSP1Triangle(d->gfx++, 3, 4, 5, 0);
    gSP1Triangle(d->gfx++, 6, 7, 8, 0);
    d->vtx += 9;
}

// The game's text box (the black box of the dialogs), stretched over the rectangle.
static void QuestLog_TextBox(QuestLogDraw* d, s16 x, s16 y, s16 w, s16 h) {
    if (!QuestLog_HasRoom(d)) {
        return;
    }
    QuestLog_Combine(d, 1);
    QuestLog_SetColor(d, 0, 0, 0, 190);
    // 128x64 IA4, mirrored horizontally to make the whole box, like the dialogs do.
    gDPLoadTextureBlock_4b(d->gfx++, gDefaultMessageBackgroundTex, G_IM_FMT_IA, 128, 64, 0,
                           G_TX_MIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_CLAMP, 7, G_TX_NOMASK, G_TX_NOLOD,
                           G_TX_NOLOD);
    QuestLog_Quad(d, x, y, w, h, 256, 64);
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
        s32 color = ((c >= 0xA5) && (c <= 0xA8)) ? 1 : 0;

        if (x + adv > maxX + 0.5f) {
            break;
        }
        if ((c != ' ') && QuestLog_HasRoom(d)) {
            if (color != colorSet) {
                if (color == 1) {
                    QuestLog_SetColor(d, 255, 255, 50, a);
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

// Draws "got/total" right aligned at rightX.
static void QuestLog_DrawCount(QuestLogDraw* d, s16 rightX, s16 y, s16 got, s16 total, s16 size, bool gold) {
    char text[16];
    u8 codes[16];
    s32 len;

    snprintf(text, sizeof(text), "%d/%d", got, total);
    len = QuestLog_Encode(text, codes, ARRAY_COUNT(codes));
    QuestLog_DrawCodes(d, rightX - QuestLog_TextWidth(codes, len, size), y, codes, len, size, 255, gold ? 210 : 255,
                       gold ? 60 : 255, 255, rightX + 1);
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

// Up/down (stick or D-pad) choose the line, left/right (or C-Left/C-Right) the list. B goes back (see Update).
// Z and R still turn the pause pages.
static void QuestLog_HandleInput(PlayState* play, s32 rowCount) {
    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = &play->state.input[0];
    s32* cursor = &sQuestLogCursor[sQuestLogCategory];

    if (sQuestLogFlipTimer > 0) {
        return;
    }
    if ((pauseCtx->state != 6) || !((pauseCtx->unk_1E4 == 0) || (pauseCtx->unk_1E4 == 5) || (pauseCtx->unk_1E4 == 8))) {
        return;
    }
    // The page arrows are not used here: left/right change the list.
    pauseCtx->cursorSpecialPos = 0;

    if (CHECK_BTN_ALL(input->press.button, BTN_CLEFT) || CHECK_BTN_ALL(input->press.button, BTN_DLEFT) ||
        (pauseCtx->stickRelX < -30)) {
        QuestLog_ChangeCategory(-1);
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_CRIGHT) || CHECK_BTN_ALL(input->press.button, BTN_DRIGHT) ||
        (pauseCtx->stickRelX > 30)) {
        QuestLog_ChangeCategory(1);
        return;
    }

    if ((pauseCtx->stickRelY > 30) || CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
        if (*cursor > 0) {
            (*cursor)--;
            Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                   &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        }
    } else if ((pauseCtx->stickRelY < -30) || CHECK_BTN_ALL(input->press.button, BTN_DDOWN)) {
        if (*cursor < rowCount - 1) {
            (*cursor)++;
            Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                   &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        }
    }
}

// Layout of the content area (local units).
#define QL_TABS_Y 0
#define QL_TAB 22
#define QL_TAB_SEL 28
#define QL_TAB_STEP 40
#define QL_NAME_Y 32
#define QL_NAME_SIZE 20
#define QL_LIST_Y 56
#define QL_ROW_H 22
#define QL_ROWS 5
#define QL_ROW_SIZE 19
#define QL_ICON 18
#define QL_TEXT_X 46
#define QL_DETAIL_Y 172
#define QL_DETAIL_SIZE 18
#define QL_DETAIL_LINE 22

static const char* sQuestLogTabIcons[QL_CAT_COUNT] = {
    gQuestIconHeartPieceTex, gItemIconBottleEmptyTex, gItemIconMaskKeatonTex, gItemIconSwordBiggoronTex,
    gQuestIconGoldSkulltulaTex,
};
static const u8 sQuestLogTabIconIs24[QL_CAT_COUNT] = { 1, 0, 0, 0, 1 };

static void QuestLog_BeginDraw(PlayState* play, GraphicsContext* gfxCtx, QuestLogDraw* d) {
    PauseContext* pauseCtx = &play->pauseCtx;

    d->gfx = sQuestLogGfx;
    d->vtx = sQuestLogVtx;
    d->alpha = pauseCtx->alpha;
    d->combine = 0;

    OPEN_DISPS(gfxCtx);
    // The page matrix is loaded; move the origin to the top left of the content area, in half pixels, y down.
    Matrix_Push();
    Matrix_Translate(-108.0f, 58.0f + pauseCtx->offsetY, 0.0f, MTXMODE_APPLY);
    Matrix_Scale(0.5f, -0.5f, 1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);

    gDPPipeSync(d->gfx++);
    gDPSetTextureFilter(d->gfx++, G_TF_BILERP);
}

static void QuestLog_EndDraw(PlayState* play, GraphicsContext* gfxCtx, QuestLogDraw* d) {
    gDPPipeSync(d->gfx++);
    gDPSetCombineMode(d->gfx++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetPrimColor(d->gfx++, 0, 0, 255, 255, 255, play->pauseCtx.alpha);
    gSPEndDisplayList(d->gfx++);

    OPEN_DISPS(gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sQuestLogGfx);
    // Back to the page matrix for whatever the page draws next.
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISPS(gfxCtx);
}

void KaleidoScope_DrawQuestLog(PlayState* play, GraphicsContext* gfxCtx, bool active) {
    PauseContext* pauseCtx = &play->pauseCtx;
    QuestLogDraw draw;
    QuestLogDraw* d = &draw;
    s16 got;
    s16 total;
    s32 rowCount;
    s32 i;
    s32 phase;
    s32 pulse;

    sQuestLogFrames++;
    phase = sQuestLogFrames % 40;
    pulse = (phase < 20) ? phase : (40 - phase); // 0..20..0

    rowCount = QuestLog_BuildRows(play, sQuestLogCategory, &got, &total);
    if (active) {
        QuestLog_HandleInput(play, rowCount);
        // The category may have changed.
        rowCount = QuestLog_BuildRows(play, sQuestLogCategory, &got, &total);
        // No item name in the panel below the page while the log is shown.
        pauseCtx->cursorItem[PAUSE_QUEST] = PAUSE_ITEM_NONE;
    }

    s32* cursor = &sQuestLogCursor[sQuestLogCategory];
    s32* top = &sQuestLogTop[sQuestLogCategory];
    *cursor = CLAMP(*cursor, 0, MAX(rowCount - 1, 0));
    if (*cursor < *top) {
        *top = *cursor;
    } else if (*cursor >= *top + QL_ROWS) {
        *top = *cursor - QL_ROWS + 1;
    }
    *top = CLAMP(*top, 0, MAX(rowCount - QL_ROWS, 0));

    QuestLog_BeginDraw(play, gfxCtx, d);

    // Title in the band at the top of the page, between two Triforces.
    {
        u8 codes[32];
        s32 len = QuestLog_Encode("MISSÕES", codes, ARRAY_COUNT(codes));
        s16 size = 26;
        f32 w = QuestLog_TextWidth(codes, len, size);
        f32 x = (QL_W - w) / 2.0f;

        QuestLog_Triforce(d, (s16)x - 32, -37, 24, 255);
        QuestLog_Triforce(d, (s16)(x + w) + 8, -37, 24, 255);
        QuestLog_DrawCodes(d, x + 2, -36 + 2, codes, len, size, 0, 0, 0, 160, QL_W);
        QuestLog_DrawCodes(d, x, -36, codes, len, size, 255, 255, 255, 255, QL_W);

        // How to go back, at the right of the title band.
        len = QuestLog_Encode("B: voltar", codes, ARRAY_COUNT(codes));
        w = QuestLog_TextWidth(codes, len, 16);
        QuestLog_DrawCodes(d, QL_W + 6 - w, -30, codes, len, 16, 200, 200, 200, 220, QL_W + 12);
    }

    // Tabs: one icon per list, the current one bigger and lit, between the C-Left and C-Right buttons.
    {
        u8 codes[64];
        s32 len;
        s16 rowW = (QL_CAT_COUNT - 1) * QL_TAB_STEP;
        s16 cx0 = (QL_W - rowW) / 2;

        for (i = 0; i < QL_CAT_COUNT; i++) {
            bool cur = i == sQuestLogCategory;
            s16 size = cur ? QL_TAB_SEL : QL_TAB;
            s16 cx = cx0 + i * QL_TAB_STEP;

            if (cur) {
                // A golden glow behind the current tab.
                QuestLog_Rect(d, cx - 17, QL_TABS_Y, 34, 30, 255, 200, 40, 40 + pulse * 2);
            }
            QuestLog_Icon(d, sQuestLogTabIcons[i], sQuestLogTabIconIs24[i], cx - size / 2,
                          QL_TABS_Y + 15 - size / 2, size, cur);
        }
        len = QuestLog_Encode(QL_GLYPH_CLEFT, codes, ARRAY_COUNT(codes));
        QuestLog_DrawCodes(d, cx0 - QL_TAB_STEP + 4, QL_TABS_Y + 4, codes, len, 22, 255, 255, 255, 255, QL_W);
        len = QuestLog_Encode(QL_GLYPH_CRIGHT, codes, ARRAY_COUNT(codes));
        QuestLog_DrawCodes(d, cx0 + rowW + QL_TAB_STEP - 26, QL_TABS_Y + 4, codes, len, 22, 255, 255, 255, 255, QL_W);

        // Name of the list and its progress.
        {
            char countText[16];
            u8 countCodes[16];
            s32 countLen;
            bool full = got >= total;
            f32 w;
            f32 x;

            len = QuestLog_Encode(sQuestLogCategoryNames[sQuestLogCategory], codes, ARRAY_COUNT(codes));
            snprintf(countText, sizeof(countText), "  %d/%d", got, total);
            countLen = QuestLog_Encode(countText, countCodes, ARRAY_COUNT(countCodes));
            w = QuestLog_TextWidth(codes, len, QL_NAME_SIZE) + QuestLog_TextWidth(countCodes, countLen, QL_NAME_SIZE);
            x = (QL_W - w) / 2.0f;
            x = QuestLog_DrawCodes(d, x, QL_NAME_Y, codes, len, QL_NAME_SIZE, 255, 255, 255, 255, QL_W);
            QuestLog_DrawCodes(d, x, QL_NAME_Y, countCodes, countLen, QL_NAME_SIZE, 255, full ? 210 : 255,
                               full ? 60 : 255, 255, QL_W);
        }
    }

    // List.
    for (i = 0; (i < QL_ROWS) && (*top + i < rowCount); i++) {
        s32 index = *top + i;
        const QuestLogRow* row = &sQuestLogRows[index];
        s16 y = QL_LIST_Y + i * QL_ROW_H;
        s16 iconY = y + (QL_ROW_H - QL_ICON) / 2 - 1;
        s16 rightX = QL_W - 10;
        s32 need;

        if (index == *cursor) {
            // Selection: a soft golden band, and Navi floating at its left like when she points at something.
            s16 bob = (s16)(sinf(sQuestLogFrames * 0.2f) * 2.0f);

            QuestLog_Rect(d, 20, y - 1, QL_W - 30, QL_ROW_H, 255, 220, 120, 40 + pulse * 2);
            QuestLog_Tex(d, gNaviIconTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 1, iconY + bob, QL_ICON, QL_ICON,
                         true, 255);
        }

        QuestLog_Icon(d, row->icon, row->iconIs24, 22, iconY, QL_ICON, row->done);

        // Right side: progress count, or the age and what the line needs.
        if (row->hasCount) {
            QuestLog_DrawCount(d, rightX, y + 2, row->got, row->total, QL_ROW_SIZE, row->done);
            rightX -= 52;
        } else {
            for (need = QL_NEED_COUNT - 1; need >= 0; need--) {
                if (row->needs & (1 << need)) {
                    QuestLog_Icon(d, QuestLog_NeedIcon(need), false, rightX - QL_ICON, iconY, QL_ICON,
                                  QuestLog_HasNeed(need));
                    rightX -= QL_ICON + 2;
                }
            }
            if (row->age != QL_AGE_ANY) {
                bool adult = row->age == QL_AGE_ADULT;
                QuestLog_Icon(d, adult ? gItemIconSwordMasterTex : gItemIconSwordKokiriTex, false, rightX - QL_ICON,
                              iconY, QL_ICON, adult == LINK_IS_ADULT);
                rightX -= QL_ICON + 2;
            }
        }

        QuestLog_DrawText(d, QL_TEXT_X, y + 2, row->name, QL_ROW_SIZE, 255, row->done ? 210 : 255,
                          row->done ? 60 : 255, 255, rightX - 4);
    }

    // Scroll arrows, like the ones of the item lists of the game.
    if (*top > 0) {
        QuestLog_Tex(d, gArrowUpTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, QL_W - 14, QL_LIST_Y - 12, 12, 12, true,
                     120 + pulse * 6);
    }
    if (*top + QL_ROWS < rowCount) {
        QuestLog_Tex(d, gArrowDownTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, QL_W - 14, QL_LIST_Y + QL_ROWS * QL_ROW_H,
                     12, 12, true, 120 + pulse * 6);
    }

    // Detail of the selected line, in a dialog text box, with Link's map head when the line is done.
    QuestLog_TextBox(d, 0, QL_DETAIL_Y - 6, QL_W, QL_H - QL_DETAIL_Y + 6);
    if (rowCount > 0) {
        const QuestLogRow* row = &sQuestLogRows[*cursor];

        QuestLog_DrawWrapped(d, 14, QL_DETAIL_Y + 2, QL_W - (row->done ? 52 : 28), row->detail, QL_DETAIL_SIZE,
                             QL_DETAIL_LINE, 4, 255, 255, 255, 255);
        if (row->done) {
            QuestLog_Tex(d, gDungeonMapLinkHeadTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, QL_W - 34, QL_H - 30, 22,
                         22, true, 155 + pulse * 5);
        }
    }

    QuestLog_EndDraw(play, gfxCtx, d);
}

// Small "C-Up Missões" hint at the bottom right of the normal Quest Status page.
void KaleidoScope_DrawQuestLogHint(PlayState* play, GraphicsContext* gfxCtx) {
    QuestLogDraw draw;
    QuestLogDraw* d = &draw;
    u8 codes[32];
    s32 len;
    f32 w;

    QuestLog_BeginDraw(play, gfxCtx, d);
    len = QuestLog_Encode(QL_GLYPH_CUP "Missões", codes, ARRAY_COUNT(codes));
    w = QuestLog_TextWidth(codes, len, 16);
    // Page y -71 to -79 is free below the Spiritual Stones and the song staff.
    QuestLog_DrawCodes(d, QL_W + 6 - w, 258, codes, len, 16, 220, 220, 220, 230, QL_W + 12);
    QuestLog_EndDraw(play, gfxCtx, d);
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
