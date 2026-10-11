#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "starter_choose.h"
#include "test/test.h"

// emerald-allgen: checks for the all-gen starter table and the rival swap logic.

TEST("(Starters) Every starter choice is a real species")
{
    u32 i;
    for (i = 0; i < NUM_STARTER_CHOICES; i++)
    {
        u32 species = GetStarterPokemon(i);
        EXPECT_NE(species, SPECIES_NONE);
        EXPECT(species < NUM_SPECIES);
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
