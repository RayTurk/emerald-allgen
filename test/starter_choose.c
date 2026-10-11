#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "battle_util.h"
#include "move.h"
#include "starter_choose.h"
#include "test/test.h"

// emerald-allgen: checks for the all-gen starter table and the rival swap logic.

TEST("(Starters) Every starter choice is a real species")
{
    u32 i;
    for (i = 0; i < NUM_STARTER_CHOICES; i++)
    {
        u32 species = GetStarterPokemon(i);
        if (species == SPECIES_NONE)
            continue; // intentionally empty Poké Ball slot
        EXPECT(species < NUM_SPECIES);
        EXPECT(gSpeciesInfo[species].baseHP > 0);
    }
}

TEST("(Starters) Out of range choice falls back to the first starter")
{
    EXPECT_EQ(GetStarterPokemon(NUM_STARTER_CHOICES), GetStarterPokemon(0));
}

TEST("(Starters) Rival slot keeps grass/fire/water position for Gen starters")
{
    EXPECT_EQ(GetStarterRivalSlot(0), 0);  // Bulbasaur
    EXPECT_EQ(GetStarterRivalSlot(4), 1);  // Cyndaquil
    EXPECT_EQ(GetStarterRivalSlot(26), 2); // Quaxly
    EXPECT_EQ(GetStarterRivalSlot(27), 0); // Pikachu (special)
}

TEST("(Starters) Rival keeps Hoenn starters when no starter has been picked")
{
    VarSet(VAR_STARTER_CHOICE, 0);
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_TORCHIC), SPECIES_TORCHIC);
}

TEST("(Starters) Rival swaps to the same-gen line at the same stage")
{
    // Player picked Chikorita (Gen 2 grass). The vanilla rival for slot 0 uses the Torchic line.
    VarSet(VAR_STARTER_CHOICE, 3 + 1);
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_TORCHIC), SPECIES_CYNDAQUIL);
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_COMBUSKEN), SPECIES_QUILAVA);
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_BLAZIKEN), SPECIES_TYPHLOSION);
    // Non-starter rival mons are untouched
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_WINGULL), SPECIES_WINGULL);
    VarSet(VAR_STARTER_CHOICE, 0);
}

TEST("(Starters) Special starters give the rival a fixed counter line")
{
    // Player picked Abra, rival gets the Froakie line
    VarSet(VAR_STARTER_CHOICE, 29 + 1);
    EXPECT_EQ(IsChosenStarterSpecial(), TRUE);
    EXPECT_EQ(GetChosenStarterPokemon(), SPECIES_ABRA);
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_TORCHIC), SPECIES_FROAKIE);
    EXPECT_EQ(GetRivalStarterSpecies(SPECIES_GROVYLE), SPECIES_FROGADIER);
    VarSet(VAR_STARTER_CHOICE, 0);
}

TEST("(Starters) Every starter can damage Birch's Zigzagoon at level 5")
{
    u32 i, j;
    for (i = 0; i < NUM_STARTER_CHOICES; i++)
    {
        struct Pokemon mon;
        bool32 canFight = FALSE;
        u32 species = GetStarterPokemon(i);

        if (species == SPECIES_NONE)
            continue;
        CreateMon(&mon, species, 5, 0, OTID_STRUCT_PRESET(0));
        EnsureStarterCanFight(&mon);
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            enum Move move = GetMonData(&mon, MON_DATA_MOVE1 + j);
            if (move != MOVE_NONE && GetMovePower(move) > 0
             && GetTypeModifier(GetMoveType(move), TYPE_NORMAL) > UQ_4_12(0.0))
                canFight = TRUE;
        }
        EXPECT(canFight);
    }
}

TEST("(Starters) Birch's post-game legendaries are all real species")
{
    u32 i;
    for (i = 0; i < NUM_REWARD_GROUPS * STARTERS_PER_GROUP; i++)
    {
        u32 species = GetLegendaryRewardPokemon(i);
        if (species == SPECIES_NONE)
            continue; // intentionally empty Poké Ball slot
        EXPECT(species < NUM_SPECIES);
        EXPECT(gSpeciesInfo[species].baseHP > 0);
    }
}

TEST("(Starters) Baby legendary starters evolve by level")
{
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_POIPOLE)[0].method, EVO_LEVEL);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_POIPOLE)[0].param, 40);
    EXPECT_EQ(GetSpeciesEvolutions(SPECIES_MELTAN)[0].targetSpecies, SPECIES_MELMETAL);
}
