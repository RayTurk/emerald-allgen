#include "global.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "field_weather.h"
#include "overworld.h"
#include "script.h"
#include "battle_util.h"
#include "move.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "list_menu.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "starter_choose.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
#include "trig.h"
#include "window.h"
#include "constants/songs.h"
#include "constants/rgb.h"

#define STARTER_MON_COUNT   STARTERS_PER_GROUP

// Position of the sprite of the selected starter Pokémon
#define STARTER_PKMN_POS_X (DISPLAY_WIDTH / 2)
#define STARTER_PKMN_POS_Y 64

#define TAG_POKEBALL_SELECT 0x1000
#define TAG_STARTER_CIRCLE  0x1001

static void CB2_StarterChoose(void);
static void ClearStarterLabel(void);
static void Task_StarterChoose(u8 taskId);
static void Task_HandleStarterChooseInput(u8 taskId);
static void Task_WaitForStarterSprite(u8 taskId);
static void Task_AskConfirmStarter(u8 taskId);
static void Task_HandleConfirmStarterInput(u8 taskId);
static void Task_DeclineStarter(u8 taskId);
static void Task_MoveStarterChooseCursor(u8 taskId);
static void Task_CreateStarterLabel(u8 taskId);
static void CreateStarterPokemonLabel(u8 selection, u8 group);
static void Task_ShowGroupMenu(u8 taskId);
static void Task_HandleGroupMenuInput(u8 taskId);
static void Task_FadeOutAndExit(u8 taskId);
static u8 CreatePokemonFrontSprite(enum Species species, u8 x, u8 y);
static void SpriteCB_SelectionHand(struct Sprite *sprite);
static void SpriteCB_Pokeball(struct Sprite *sprite);
static void SpriteCB_StarterPokemon(struct Sprite *sprite);

static u16 sStarterLabelWindowId;

const u16 gBirchBagGrass_Pal[] = INCGFX_U16("graphics/starter_choose/tiles.png", ".gbapal");
static const u16 sPokeballSelection_Pal[] = INCGFX_U16("graphics/starter_choose/pokeball_selection.png", ".gbapal");
static const u16 sStarterCircle_Pal[] = INCGFX_U16("graphics/starter_choose/starter_circle.png", ".gbapal");
const u32 gBirchBagTilemap[] = INCGFX_U32("graphics/starter_choose/birch_bag.bin", ".smolTM");
const u32 gBirchGrassTilemap[] = INCGFX_U32("graphics/starter_choose/birch_grass.bin", ".smolTM");
const u32 gBirchBagGrass_Gfx[] = INCGFX_U32("graphics/starter_choose/tiles.png", ".4bpp.smol");
const u32 gPokeballSelection_Gfx[] = INCGFX_U32("graphics/starter_choose/pokeball_selection.png", ".4bpp.smol");
static const u32 sStarterCircle_Gfx[] = INCGFX_U32("graphics/starter_choose/starter_circle.png", ".4bpp.smol");

static const struct WindowTemplate sWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 15,
        .width = 24,
        .height = 4,
        .paletteNum = 14,
        .baseBlock = 0x0200
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sWindowTemplate_ConfirmStarter =
{
    .bg = 0,
    .tilemapLeft = 24,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0x0260
};

static const struct WindowTemplate sWindowTemplate_StarterLabel =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 13,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0x0274
};

static const u8 sPokeballCoords[STARTER_MON_COUNT][2] =
{
    {60, 64},
    {120, 88},
    {180, 64},
};

static const u8 sStarterLabelCoords[STARTER_MON_COUNT][2] =
{
    {0, 9},
    {16, 10},
    {8, 4},
};

// emerald-allgen: every starter group, in Poké Ball order (left, middle, right).
// For Gen groups the order is always grass, fire, water, which the rival logic relies on.
// SPECIES_NONE leaves that Poké Ball empty (hidden and skipped by the cursor).
static const u16 sStarterMon[(NUM_STARTER_GROUPS + NUM_REWARD_GROUPS) * STARTERS_PER_GROUP] =
{
    SPECIES_BULBASAUR,  SPECIES_CHARMANDER, SPECIES_SQUIRTLE,  // Gen 1
    SPECIES_CHIKORITA,  SPECIES_CYNDAQUIL,  SPECIES_TOTODILE,  // Gen 2
    SPECIES_TREECKO,    SPECIES_TORCHIC,    SPECIES_MUDKIP,    // Gen 3
    SPECIES_TURTWIG,    SPECIES_CHIMCHAR,   SPECIES_PIPLUP,    // Gen 4
    SPECIES_SNIVY,      SPECIES_TEPIG,      SPECIES_OSHAWOTT,  // Gen 5
    SPECIES_CHESPIN,    SPECIES_FENNEKIN,   SPECIES_FROAKIE,   // Gen 6
    SPECIES_ROWLET,     SPECIES_LITTEN,     SPECIES_POPPLIO,   // Gen 7
    SPECIES_GROOKEY,    SPECIES_SCORBUNNY,  SPECIES_SOBBLE,    // Gen 8
    SPECIES_SPRIGATITO, SPECIES_FUECOCO,    SPECIES_QUAXLY,    // Gen 9
    SPECIES_PIKACHU,    SPECIES_EEVEE,      SPECIES_ABRA,      // Specials 1
    SPECIES_RIOLU,      SPECIES_TOGEPI,     SPECIES_DRATINI,   // Specials 2
    SPECIES_MEW,        SPECIES_MEWTWO,     SPECIES_CELEBI,    // Mythics
    SPECIES_RAIKOU,     SPECIES_ENTEI,      SPECIES_SUICUNE,   // Beasts
    // Post-game reward groups (Birch's lab after the National Dex upgrade)
    SPECIES_MEW,        SPECIES_MEWTWO,     SPECIES_CELEBI,    // Mythic
    SPECIES_RAIKOU,     SPECIES_ENTEI,      SPECIES_SUICUNE,   // Beasts
};

#define FIRST_REWARD_GROUP NUM_STARTER_GROUPS
#define REWARD_LEVEL 50

// Full three-stage lines for the Gen starters: [gen][grass/fire/water][stage].
// Used to swap the rival's Hoenn starter for the same-gen equivalent.
static const u16 sStarterLines[NUM_GEN_STARTER_GROUPS][STARTERS_PER_GROUP][3] =
{
    {{SPECIES_BULBASAUR, SPECIES_IVYSAUR, SPECIES_VENUSAUR}, {SPECIES_CHARMANDER, SPECIES_CHARMELEON, SPECIES_CHARIZARD}, {SPECIES_SQUIRTLE, SPECIES_WARTORTLE, SPECIES_BLASTOISE}},
    {{SPECIES_CHIKORITA, SPECIES_BAYLEEF, SPECIES_MEGANIUM}, {SPECIES_CYNDAQUIL, SPECIES_QUILAVA, SPECIES_TYPHLOSION}, {SPECIES_TOTODILE, SPECIES_CROCONAW, SPECIES_FERALIGATR}},
    {{SPECIES_TREECKO, SPECIES_GROVYLE, SPECIES_SCEPTILE}, {SPECIES_TORCHIC, SPECIES_COMBUSKEN, SPECIES_BLAZIKEN}, {SPECIES_MUDKIP, SPECIES_MARSHTOMP, SPECIES_SWAMPERT}},
    {{SPECIES_TURTWIG, SPECIES_GROTLE, SPECIES_TORTERRA}, {SPECIES_CHIMCHAR, SPECIES_MONFERNO, SPECIES_INFERNAPE}, {SPECIES_PIPLUP, SPECIES_PRINPLUP, SPECIES_EMPOLEON}},
    {{SPECIES_SNIVY, SPECIES_SERVINE, SPECIES_SERPERIOR}, {SPECIES_TEPIG, SPECIES_PIGNITE, SPECIES_EMBOAR}, {SPECIES_OSHAWOTT, SPECIES_DEWOTT, SPECIES_SAMUROTT}},
    {{SPECIES_CHESPIN, SPECIES_QUILLADIN, SPECIES_CHESNAUGHT}, {SPECIES_FENNEKIN, SPECIES_BRAIXEN, SPECIES_DELPHOX}, {SPECIES_FROAKIE, SPECIES_FROGADIER, SPECIES_GRENINJA}},
    {{SPECIES_ROWLET, SPECIES_DARTRIX, SPECIES_DECIDUEYE}, {SPECIES_LITTEN, SPECIES_TORRACAT, SPECIES_INCINEROAR}, {SPECIES_POPPLIO, SPECIES_BRIONNE, SPECIES_PRIMARINA}},
    {{SPECIES_GROOKEY, SPECIES_THWACKEY, SPECIES_RILLABOOM}, {SPECIES_SCORBUNNY, SPECIES_RABOOT, SPECIES_CINDERACE}, {SPECIES_SOBBLE, SPECIES_DRIZZILE, SPECIES_INTELEON}},
    {{SPECIES_SPRIGATITO, SPECIES_FLORAGATO, SPECIES_MEOWSCARADA}, {SPECIES_FUECOCO, SPECIES_CROCALOR, SPECIES_SKELEDIRGE}, {SPECIES_QUAXLY, SPECIES_QUAXWELL, SPECIES_QUAQUAVAL}},
};

#define HOENN_GEN_GROUP 2

// For special starters the rival uses a fixed counter line, indexed like the special slots.
static const u8 sSpecialRivalLines[NUM_SPECIAL_GROUPS * STARTERS_PER_GROUP][2] =
{
    {3, 0}, // Pikachu -> Turtwig line (Torterra is Ground)
    {5, 0}, // Eevee   -> Chespin line (Chesnaught is Fighting)
    {5, 2}, // Abra    -> Froakie line (Greninja is Dark)
    {5, 1}, // Riolu   -> Fennekin line (Delphox is Psychic)
    {3, 2}, // Togepi  -> Piplup line (Empoleon is Steel)
    {6, 2}, // Dratini -> Popplio line (Primarina is Fairy)
    {5, 2}, // Mew     -> Froakie line (Greninja is Dark)
    {6, 1}, // Mewtwo  -> Litten line (Incineroar is Dark)
    {5, 1}, // Celebi  -> Fennekin line (Fire)
    {3, 0}, // Raikou  -> Turtwig line (Torterra is Ground)
    {4, 2}, // Entei   -> Oshawott line (Water)
    {4, 0}, // Suicune -> Snivy line (Grass)
};

static const struct ListMenuItem sStarterGroupItems[NUM_STARTER_GROUPS] =
{
    {COMPOUND_STRING("Kanto"),      0},
    {COMPOUND_STRING("Johto"),      1},
    {COMPOUND_STRING("Hoenn"),      2},
    {COMPOUND_STRING("Sinnoh"),     3},
    {COMPOUND_STRING("Unova"),      4},
    {COMPOUND_STRING("Kalos"),      5},
    {COMPOUND_STRING("Alola"),      6},
    {COMPOUND_STRING("Galar"),      7},
    {COMPOUND_STRING("Paldea"),     8},
    {COMPOUND_STRING("Specials 1"), 9},
    {COMPOUND_STRING("Specials 2"), 10},
    {COMPOUND_STRING("Mythics"),    11},
    {COMPOUND_STRING("Beasts"),     12},
};

static const struct ListMenuItem sRewardGroupItems[NUM_REWARD_GROUPS] =
{
    {COMPOUND_STRING("Mythic"), FIRST_REWARD_GROUP},
    {COMPOUND_STRING("Beasts"), FIRST_REWARD_GROUP + 1},
};

static const u8 sText_ChooseLegendGroup[] = _("PROF. BIRCH: Pick any one of these.\nPress B if you need more time.");
static const u8 sText_ChooseLegend[] = _("Choose carefully, {PLAYER}.\nYou can only take one!");

// TRUE while the screen is being used for Birch's post-game legendary pick
static bool8 sRewardMode;

static const u8 sText_ChooseRegion[] = _("PROF. BIRCH is in trouble!\nPick a region, then a POKéMON.");

static const struct WindowTemplate sWindowTemplate_GroupList =
{
    .bg = 0,
    .tilemapLeft = 10,
    .tilemapTop = 1,
    .width = 10,
    .height = 12,
    .paletteNum = 14,
    .baseBlock = 0x02C0
};

static const struct ListMenuTemplate sListMenuTemplate_Groups =
{
    .items = sStarterGroupItems,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = NUM_STARTER_GROUPS,
    .maxShowed = 6,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NORMAL,
    .cursorKind = CURSOR_BLACK_ARROW,
};

static const struct BgTemplate sBgTemplates[3] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 6,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
};

static const u8 sTextColors[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY};

static const struct OamData sOam_Hand =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct OamData sOam_Pokeball =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct OamData sOam_StarterCircle =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const u8 sCursorCoords[][2] =
{
    {60, 32},
    {120, 56},
    {180, 32},
};

static const union AnimCmd sAnim_Hand[] =
{
    ANIMCMD_FRAME(48, 30),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_Pokeball_Still[] =
{
    ANIMCMD_FRAME(0, 30),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_Pokeball_Moving[] =
{
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(0, 32),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_StarterCircle[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_Hand[] =
{
    sAnim_Hand,
};

static const union AnimCmd *const sAnims_Pokeball[] =
{
    sAnim_Pokeball_Still,
    sAnim_Pokeball_Moving,
};

static const union AnimCmd *const sAnims_StarterCircle[] =
{
    sAnim_StarterCircle,
};

static const union AffineAnimCmd sAffineAnim_StarterPokemon[] =
{
    AFFINEANIMCMD_FRAME(16, 16, 0, 0),
    AFFINEANIMCMD_FRAME(16, 16, 0, 15),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_StarterCircle[] =
{
    AFFINEANIMCMD_FRAME(20, 20, 0, 0),
    AFFINEANIMCMD_FRAME(20, 20, 0, 15),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sAffineAnims_StarterPokemon = {sAffineAnim_StarterPokemon};
static const union AffineAnimCmd *const sAffineAnims_StarterCircle[] = {sAffineAnim_StarterCircle};

static const struct CompressedSpriteSheet sSpriteSheet_PokeballSelect[] =
{
    {
        .data = gPokeballSelection_Gfx,
        .size = 0x0800,
        .tag = TAG_POKEBALL_SELECT
    },
    {}
};

static const struct CompressedSpriteSheet sSpriteSheet_StarterCircle[] =
{
    {
        .data = sStarterCircle_Gfx,
        .size = 0x0800,
        .tag = TAG_STARTER_CIRCLE
    },
    {}
};

static const struct SpritePalette sSpritePalettes_StarterChoose[] =
{
    {
        .data = sPokeballSelection_Pal,
        .tag = TAG_POKEBALL_SELECT
    },
    {
        .data = sStarterCircle_Pal,
        .tag = TAG_STARTER_CIRCLE
    },
    {},
};

static const struct SpriteTemplate sSpriteTemplate_Hand =
{
    .tileTag = TAG_POKEBALL_SELECT,
    .paletteTag = TAG_POKEBALL_SELECT,
    .oam = &sOam_Hand,
    .anims = sAnims_Hand,
    .callback = SpriteCB_SelectionHand
};

static const struct SpriteTemplate sSpriteTemplate_Pokeball =
{
    .tileTag = TAG_POKEBALL_SELECT,
    .paletteTag = TAG_POKEBALL_SELECT,
    .oam = &sOam_Pokeball,
    .anims = sAnims_Pokeball,
    .callback = SpriteCB_Pokeball
};

static const struct SpriteTemplate sSpriteTemplate_StarterCircle =
{
    .tileTag = TAG_STARTER_CIRCLE,
    .paletteTag = TAG_STARTER_CIRCLE,
    .oam = &sOam_StarterCircle,
    .anims = sAnims_StarterCircle,
    .affineAnims = sAffineAnims_StarterCircle,
    .callback = SpriteCB_StarterPokemon
};

// .text
// Any slot in the table, including the post-game reward groups. SPECIES_NONE for empty slots.
static u16 GetGroupSpecies(u32 flatIndex)
{
    if (flatIndex >= ARRAY_COUNT(sStarterMon))
        return SPECIES_NONE;
    return sStarterMon[flatIndex];
}

u16 GetLegendaryRewardPokemon(u16 index)
{
    return GetGroupSpecies(FIRST_REWARD_GROUP * STARTERS_PER_GROUP + index);
}

u16 GetStarterPokemon(u16 chosenStarterId)
{
    if (chosenStarterId >= NUM_STARTER_CHOICES)
        chosenStarterId = 0;
    return sStarterMon[chosenStarterId];
}

// VAR_STARTER_CHOICE holds the flat index + 1, so 0 means no starter has been picked yet.
static s32 GetChosenStarterIndex(void)
{
    u16 value = VarGet(VAR_STARTER_CHOICE);
    if (value == 0 || value > NUM_STARTER_CHOICES)
        return -1;
    return value - 1;
}

u16 GetChosenStarterPokemon(void)
{
    s32 index = GetChosenStarterIndex();
    return GetStarterPokemon(index < 0 ? 0 : index);
}

bool32 IsChosenStarterSpecial(void)
{
    return GetChosenStarterIndex() >= NUM_GEN_STARTER_GROUPS * STARTERS_PER_GROUP;
}

// The vanilla scripts use VAR_STARTER_MON (0 grass, 1 fire, 2 water) to pick which rival
// trainer to fight. Gen starters keep their element slot. Specials all use slot 0; their
// rival team gets swapped to a fixed counter line in GetRivalStarterSpecies.
u16 GetStarterRivalSlot(u16 chosenStarterId)
{
    if (chosenStarterId >= NUM_GEN_STARTER_GROUPS * STARTERS_PER_GROUP)
        return 0;
    return chosenStarterId % STARTERS_PER_GROUP;
}

// Called for each mon in a rival's party. If the mon is part of a Hoenn starter line,
// return the same evolution stage of the line that counters the player's actual pick.
u16 GetRivalStarterSpecies(u16 species)
{
    s32 choice = GetChosenStarterIndex();
    u32 element, stage;

    if (choice < 0)
        return species;

    for (element = 0; element < STARTERS_PER_GROUP; element++)
    {
        for (stage = 0; stage < 3; stage++)
        {
            if (sStarterLines[HOENN_GEN_GROUP][element][stage] != species)
                continue;

            if (choice >= NUM_GEN_STARTER_GROUPS * STARTERS_PER_GROUP)
            {
                const u8 *line = sSpecialRivalLines[choice - NUM_GEN_STARTER_GROUPS * STARTERS_PER_GROUP];
                return sStarterLines[line[0]][line[1]][stage];
            }
            return sStarterLines[choice / STARTERS_PER_GROUP][element][stage];
        }
    }
    return species;
}

static bool32 CanMoveHitNormalType(enum Move move)
{
    return move != MOVE_NONE
        && GetMovePower(move) > 0
        && GetTypeModifier(GetMoveType(move), TYPE_NORMAL) > UQ_4_12(0.0);
}

// Some starters can't damage Birch's Zigzagoon at level 5 (Abra only knows Teleport,
// Cosmog only Splash and Teleport), which makes the first battle unwinnable.
// Give those a basic attack so every pick can actually fight.
void EnsureStarterCanFight(struct Pokemon *mon)
{
    u32 i;
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Move fallback;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (CanMoveHitNormalType(GetMonData(mon, MON_DATA_MOVE1 + i)))
            return;
    }

    if (GetSpeciesType(species, 0) == TYPE_PSYCHIC || GetSpeciesType(species, 1) == TYPE_PSYCHIC)
        fallback = MOVE_CONFUSION;
    else
        fallback = MOVE_TACKLE;

    if (GiveMoveToMon(mon, fallback) == MON_HAS_MAX_MOVES)
        SetMonMoveSlot(mon, fallback, MAX_MON_MOVES - 1);
}

// Special: Birch's post-game legendary pick. Returns the chosen species in VAR_RESULT,
// or SPECIES_NONE if the player backed out.
static void Task_OpenLegendaryReward(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        sRewardMode = TRUE;
        gMain.savedCallback = CB2_ReturnToFieldContinueScript;
        SetMainCallback2(CB2_ChooseStarter);
        DestroyTask(taskId);
    }
}

void ChooseLegendaryReward(void)
{
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    CreateTask(Task_OpenLegendaryReward, 10);
}

static void VblankCB_StarterChoose(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Data for Task_StarterChoose
#define tStarterSelection   data[0]
#define tPkmnSpriteId       data[1]
#define tCircleSpriteId     data[2]
#define tGroup              data[3]
#define tListTaskId         data[4]
#define tShowBalls          data[5]
#define tListScroll         data[6]
#define tListRow            data[7]
#define tListWindowId       data[8]

#define CURRENT_STARTER(taskId) (gTasks[taskId].tGroup * STARTERS_PER_GROUP + gTasks[taskId].tStarterSelection)

// Data for sSpriteTemplate_Pokeball
#define sTaskId data[0]
#define sBallId data[1]

void CB2_ChooseStarter(void)
{
    u8 taskId;
    u8 spriteId;

    SetVBlankCallback(NULL);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);

    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);

    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);

    DecompressDataWithHeaderVram(gBirchBagGrass_Gfx, (void *)VRAM);
    DecompressDataWithHeaderVram(gBirchBagTilemap, (void *)(BG_SCREEN_ADDR(6)));
    DecompressDataWithHeaderVram(gBirchGrassTilemap, (void *)(BG_SCREEN_ADDR(7)));

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    InitWindows(sWindowTemplates);

    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 0x2A8, BG_PLTT_ID(13));
    ClearScheduledBgCopiesToVram();
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetAllPicSprites();

    LoadPalette(GetOverworldTextboxPalettePtr(), BG_PLTT_ID(14), PLTT_SIZE_4BPP);
    LoadPalette(gBirchBagGrass_Pal, BG_PLTT_ID(0), sizeof(gBirchBagGrass_Pal));
    LoadCompressedSpriteSheet(&sSpriteSheet_PokeballSelect[0]);
    LoadCompressedSpriteSheet(&sSpriteSheet_StarterCircle[0]);
    LoadSpritePalettes(sSpritePalettes_StarterChoose);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);

    EnableInterrupts(DISPSTAT_VBLANK);
    SetVBlankCallback(VblankCB_StarterChoose);
    SetMainCallback2(CB2_StarterChoose);

    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT1_BG2 | BLDCNT_TGT1_BG3 | BLDCNT_TGT1_OBJ | BLDCNT_TGT1_BD | BLDCNT_EFFECT_DARKEN);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 7);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);

    ShowBg(0);
    ShowBg(2);
    ShowBg(3);

    taskId = CreateTask(Task_ShowGroupMenu, 0);
    gTasks[taskId].tStarterSelection = 1;
    gTasks[taskId].tGroup = HOENN_GEN_GROUP;
    gTasks[taskId].tShowBalls = FALSE;
    gTasks[taskId].tListScroll = 0;
    gTasks[taskId].tListRow = sRewardMode ? 0 : HOENN_GEN_GROUP;

    // Create hand sprite
    spriteId = CreateSprite(&sSpriteTemplate_Hand, 120, 56, 2);
    gSprites[spriteId].data[0] = taskId;

    // Create three Poké Ball sprites
    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[0][0], sPokeballCoords[0][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 0;

    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[1][0], sPokeballCoords[1][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 1;

    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[2][0], sPokeballCoords[2][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 2;

    sStarterLabelWindowId = WINDOW_NONE;
}

static void CB2_StarterChoose(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

// emerald-allgen: step 1, pick a region (or a specials group) from a list
static void Task_ShowGroupMenu(u8 taskId)
{
    struct ListMenuTemplate menu = sListMenuTemplate_Groups;
    u8 windowId;

    if (sRewardMode)
    {
        menu.items = sRewardGroupItems;
        menu.totalItems = NUM_REWARD_GROUPS;
    }

    DrawStdFrameWithCustomTileAndPalette(0, FALSE, 0x2A8, 0xD);
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized(0, FONT_NORMAL, sRewardMode ? sText_ChooseLegendGroup : sText_ChooseRegion, 0, 1, 0, NULL);
    PutWindowTilemap(0);

    windowId = AddWindow(&sWindowTemplate_GroupList);
    DrawStdFrameWithCustomTileAndPalette(windowId, FALSE, 0x2A8, 0xD);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    menu.windowId = windowId;
    gTasks[taskId].tListWindowId = windowId;
    gTasks[taskId].tListTaskId = ListMenuInit(&menu, gTasks[taskId].tListScroll, gTasks[taskId].tListRow);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Task_HandleGroupMenuInput;
}

static void Task_HandleGroupMenuInput(u8 taskId)
{
    s32 input = ListMenu_ProcessInput(gTasks[taskId].tListTaskId);
    u16 scroll, row;

    if (input == LIST_NOTHING_CHOSEN)
        return;
    if (input == LIST_CANCEL && !sRewardMode)
        return; // the starter pick can't be backed out of

    PlaySE(SE_SELECT);
    DestroyListMenuTask(gTasks[taskId].tListTaskId, &scroll, &row);
    gTasks[taskId].tListScroll = scroll;
    gTasks[taskId].tListRow = row;
    ClearStdWindowAndFrameToTransparent(gTasks[taskId].tListWindowId, TRUE);
    RemoveWindow(gTasks[taskId].tListWindowId);

    if (input == LIST_CANCEL)
    {
        // Leaving Birch's legendary pick without choosing
        gSpecialVar_Result = SPECIES_NONE;
        gTasks[taskId].func = Task_FadeOutAndExit;
        return;
    }

    gTasks[taskId].tGroup = input;
    gTasks[taskId].tStarterSelection = 1;
    gTasks[taskId].tShowBalls = TRUE;
    gTasks[taskId].func = Task_StarterChoose;
}

// Step 2, pick one of the three Poké Balls for that group
static void Task_StarterChoose(u8 taskId)
{
    CreateStarterPokemonLabel(gTasks[taskId].tStarterSelection, gTasks[taskId].tGroup);
    DrawStdFrameWithCustomTileAndPalette(0, FALSE, 0x2A8, 0xD);
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized(0, FONT_NORMAL, sRewardMode ? sText_ChooseLegend : gText_BirchInTrouble, 0, 1, 0, NULL);
    PutWindowTilemap(0);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static void Task_HandleStarterChooseInput(u8 taskId)
{
    u8 selection = gTasks[taskId].tStarterSelection;

    if (JOY_NEW(A_BUTTON))
    {
        u8 spriteId;

        ClearStarterLabel();

        // Create white circle background
        spriteId = CreateSprite(&sSpriteTemplate_StarterCircle, sPokeballCoords[selection][0], sPokeballCoords[selection][1], 1);
        gTasks[taskId].tCircleSpriteId = spriteId;

        // Create Pokémon sprite
        spriteId = CreatePokemonFrontSprite(GetGroupSpecies(CURRENT_STARTER(taskId)), sPokeballCoords[selection][0], sPokeballCoords[selection][1]);
        gSprites[spriteId].affineAnims = &sAffineAnims_StarterPokemon;
        gSprites[spriteId].callback = SpriteCB_StarterPokemon;

        gTasks[taskId].tPkmnSpriteId = spriteId;
        gTasks[taskId].func = Task_WaitForStarterSprite;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        // Back to the region list
        PlaySE(SE_SELECT);
        ClearStarterLabel();
        gTasks[taskId].tShowBalls = FALSE;
        gTasks[taskId].func = Task_ShowGroupMenu;
    }
    else if (JOY_NEW(DPAD_LEFT) && selection > 0
          && GetGroupSpecies(gTasks[taskId].tGroup * STARTERS_PER_GROUP + selection - 1) != SPECIES_NONE)
    {
        gTasks[taskId].tStarterSelection--;
        gTasks[taskId].func = Task_MoveStarterChooseCursor;
    }
    else if (JOY_NEW(DPAD_RIGHT) && selection < STARTER_MON_COUNT - 1
          && GetGroupSpecies(gTasks[taskId].tGroup * STARTERS_PER_GROUP + selection + 1) != SPECIES_NONE)
    {
        gTasks[taskId].tStarterSelection++;
        gTasks[taskId].func = Task_MoveStarterChooseCursor;
    }
}

static void Task_WaitForStarterSprite(u8 taskId)
{
    if (gSprites[gTasks[taskId].tCircleSpriteId].affineAnimEnded &&
        gSprites[gTasks[taskId].tCircleSpriteId].x == STARTER_PKMN_POS_X &&
        gSprites[gTasks[taskId].tCircleSpriteId].y == STARTER_PKMN_POS_Y)
    {
        gTasks[taskId].func = Task_AskConfirmStarter;
    }
}

static void Task_AskConfirmStarter(u8 taskId)
{
    PlayCry_Normal(GetGroupSpecies(CURRENT_STARTER(taskId)), 0);
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized(0, FONT_NORMAL, gText_ConfirmStarterChoice, 0, 1, 0, NULL);
    ScheduleBgCopyTilemapToVram(0);
    CreateYesNoMenu(&sWindowTemplate_ConfirmStarter, 0x2A8, 0xD, 0);
    gTasks[taskId].func = Task_HandleConfirmStarterInput;
}

static void Task_HandleConfirmStarterInput(u8 taskId)
{
    u8 spriteId;

    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:  // YES
        if (sRewardMode)
        {
            // Birch's legendary pick returns the species itself to the script
            gSpecialVar_Result = GetGroupSpecies(CURRENT_STARTER(taskId));
            gTasks[taskId].func = Task_FadeOutAndExit;
            break;
        }
        // Return the starter choice and exit.
        gSpecialVar_Result = CURRENT_STARTER(taskId);
        ResetAllPicSprites();
        SetMainCallback2(gMain.savedCallback);
        break;
    case 1:  // NO
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        spriteId = gTasks[taskId].tPkmnSpriteId;
        FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
        FreeAndDestroyMonPicSprite(spriteId);

        spriteId = gTasks[taskId].tCircleSpriteId;
        FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
        DestroySprite(&gSprites[spriteId]);
        gTasks[taskId].func = Task_DeclineStarter;
        break;
    }
}

static void Task_FadeOutAndExit(u8 taskId)
{
    if (gTasks[taskId].tShowBalls != 2)
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].tShowBalls = 2; // keeps whatever is on screen visible during the fade
        return;
    }
    if (!gPaletteFade.active)
    {
        sRewardMode = FALSE;
        ResetAllPicSprites();
        FreeAllWindowBuffers();
        DestroyTask(taskId);
        SetMainCallback2(gMain.savedCallback);
    }
}

static void Task_DeclineStarter(u8 taskId)
{
    gTasks[taskId].func = Task_StarterChoose;
}

static void CreateStarterPokemonLabel(u8 selection, u8 group)
{
    u8 categoryText[32];
    struct WindowTemplate winTemplate;
    const u8 *speciesName;
    s32 width;
    u8 labelLeft, labelRight, labelTop, labelBottom;

    enum Species species = GetGroupSpecies(group * STARTERS_PER_GROUP + selection);
    CopyMonCategoryText(species, categoryText);
    speciesName = GetSpeciesName(species);

    winTemplate = sWindowTemplate_StarterLabel;
    winTemplate.tilemapLeft = sStarterLabelCoords[selection][0];
    winTemplate.tilemapTop = sStarterLabelCoords[selection][1];

    sStarterLabelWindowId = AddWindow(&winTemplate);
    FillWindowPixelBuffer(sStarterLabelWindowId, PIXEL_FILL(0));

    width = GetStringCenterAlignXOffset(FONT_NARROW, categoryText, 0x68);
    AddTextPrinterParameterized3(sStarterLabelWindowId, FONT_NARROW, width, 1, sTextColors, 0, categoryText);

    width = GetStringCenterAlignXOffset(FONT_NORMAL, speciesName, 0x68);
    AddTextPrinterParameterized3(sStarterLabelWindowId, FONT_NORMAL, width, 17, sTextColors, 0, speciesName);

    PutWindowTilemap(sStarterLabelWindowId);
    ScheduleBgCopyTilemapToVram(0);

    labelLeft = sStarterLabelCoords[selection][0] * 8 - 4;
    labelRight = (sStarterLabelCoords[selection][0] + 13) * 8 + 4;
    labelTop = sStarterLabelCoords[selection][1] * 8;
    labelBottom = (sStarterLabelCoords[selection][1] + 4) * 8;
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(labelLeft, labelRight));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(labelTop, labelBottom));
}

static void ClearStarterLabel(void)
{
    FillWindowPixelBuffer(sStarterLabelWindowId, PIXEL_FILL(0));
    ClearWindowTilemap(sStarterLabelWindowId);
    RemoveWindow(sStarterLabelWindowId);
    sStarterLabelWindowId = WINDOW_NONE;
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    ScheduleBgCopyTilemapToVram(0);
}

static void Task_MoveStarterChooseCursor(u8 taskId)
{
    ClearStarterLabel();
    gTasks[taskId].func = Task_CreateStarterLabel;
}

static void Task_CreateStarterLabel(u8 taskId)
{
    CreateStarterPokemonLabel(gTasks[taskId].tStarterSelection, gTasks[taskId].tGroup);
    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static u8 CreatePokemonFrontSprite(enum Species species, u8 x, u8 y)
{
    u8 spriteId;

    spriteId = CreateMonPicSprite_Affine(species, FALSE, 0, MON_PIC_AFFINE_FRONT, x, y, 14, TAG_NONE);
    gSprites[spriteId].oam.priority = 0;
    return spriteId;
}

static void SpriteCB_SelectionHand(struct Sprite *sprite)
{
    // Float up and down above selected Poké Ball
    sprite->invisible = !gTasks[sprite->data[0]].tShowBalls;
    sprite->x = sCursorCoords[gTasks[sprite->data[0]].tStarterSelection][0];
    sprite->y = sCursorCoords[gTasks[sprite->data[0]].tStarterSelection][1];
    sprite->y2 = Sin(sprite->data[1], 8);
    sprite->data[1] = (u8)(sprite->data[1]) + 4;
}

static void SpriteCB_Pokeball(struct Sprite *sprite)
{
    // Animate Poké Ball if currently selected
    sprite->invisible = !gTasks[sprite->sTaskId].tShowBalls
                     || GetGroupSpecies(gTasks[sprite->sTaskId].tGroup * STARTERS_PER_GROUP + sprite->sBallId) == SPECIES_NONE;
    if (gTasks[sprite->sTaskId].tStarterSelection == sprite->sBallId)
        StartSpriteAnimIfDifferent(sprite, 1);
    else
        StartSpriteAnimIfDifferent(sprite, 0);
}

static void SpriteCB_StarterPokemon(struct Sprite *sprite)
{
    // Move sprite to upper center of screen
    if (sprite->x > STARTER_PKMN_POS_X)
        sprite->x -= 4;
    if (sprite->x < STARTER_PKMN_POS_X)
        sprite->x += 4;
    if (sprite->y > STARTER_PKMN_POS_Y)
        sprite->y -= 2;
    if (sprite->y < STARTER_PKMN_POS_Y)
        sprite->y += 2;
}
