#include "global.h"
#include "pokemon.h"
#include "constants/items.h"
#include "test/test.h"

// emerald-allgen: Hoenn+ regional dex and the evolution fixes that keep it completable.

TEST("(Hoenn+) The regional dex has 401 entries")
{
    EXPECT_EQ(HOENN_DEX_COUNT - 1, 401);
}

TEST("(Hoenn+) Every regional dex entry maps to a distinct national dex number")
{
    u32 i, j;
    for (i = 1; i < HOENN_DEX_COUNT; i++)
    {
        enum NationalDexOrder nat = HoennToNationalOrder(i);
        EXPECT_NE(nat, NATIONAL_DEX_NONE);
        EXPECT_EQ(NationalToHoennOrder(nat), i);
        for (j = i + 1; j < HOENN_DEX_COUNT; j++)
            EXPECT_NE(nat, HoennToNationalOrder(j));
    }
}

TEST("(Hoenn+) New species are in the regional dex, legendaries stay last")
{
    EXPECT_NE(SpeciesToHoennPokedexNum(SPECIES_STARLY), 0);
    EXPECT_NE(SpeciesToHoennPokedexNum(SPECIES_HYDREIGON), 0);
    EXPECT_NE(SpeciesToHoennPokedexNum(SPECIES_FLORGES), 0);
    EXPECT_EQ(SpeciesToHoennPokedexNum(SPECIES_DEOXYS), HOENN_DEX_COUNT - 1);
    EXPECT_EQ(SpeciesToHoennPokedexNum(SPECIES_LILLIPUP), 0); // National Dex only
}

TEST("(Hoenn+) Leader's Crest evolves Bisharp when used from the bag")
{
    struct Pokemon mon;
    bool32 canStop;
    CreateMon(&mon, SPECIES_BISHARP, 60, 0, OTID_STRUCT_PRESET(0));
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, ITEM_LEADERS_CREST, NULL, &canStop, CHECK_EVO), SPECIES_KINGAMBIT);
}

TEST("(Hoenn+) Metal Coat evolves Scyther without a trade")
{
    struct Pokemon mon;
    bool32 canStop;
    CreateMon(&mon, SPECIES_SCYTHER, 30, 0, OTID_STRUCT_PRESET(0));
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, ITEM_METAL_COAT, NULL, &canStop, CHECK_EVO), SPECIES_SCIZOR);
}
