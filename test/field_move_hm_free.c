#include "global.h"
#include "field_move.h"
#include "item.h"
#include "party_menu.h"
#include "pokemon.h"
#include "test/test.h"

// emerald-allgen: HM-free field moves

static struct Pokemon *SetUpLeadMon(enum Species species)
{
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], species, 30, 0, OTID_STRUCT_PRESET(0));
    return &gParties[B_TRAINER_PLAYER][0];
}

TEST("(HM-free) A Pokemon that can learn Surf can use it once the player owns the HM")
{
    struct Pokemon *mon = SetUpLeadMon(SPECIES_LAPRAS);
    ASSUME(CanLearnTeachableMove(SPECIES_LAPRAS, MOVE_SURF));
    ASSUME(!MonKnowsMove(mon, MOVE_SURF));

    ClearBag();
    EXPECT(!MonCanUseFieldMove(mon, MOVE_SURF));
    AddBagItem(ITEM_HM_SURF, 1);
    EXPECT(MonCanUseFieldMove(mon, MOVE_SURF));
    ClearBag();
}

TEST("(HM-free) A Pokemon that can't learn the move can't use it, even with the HM")
{
    struct Pokemon *mon = SetUpLeadMon(SPECIES_CATERPIE);
    ASSUME(!CanLearnTeachableMove(SPECIES_CATERPIE, MOVE_SURF));

    ClearBag();
    AddBagItem(ITEM_HM_SURF, 1);
    EXPECT(!MonCanUseFieldMove(mon, MOVE_SURF));
    ClearBag();
}

TEST("(HM-free) Knowing the move still works without the HM")
{
    struct Pokemon *mon = SetUpLeadMon(SPECIES_LAPRAS);
    SetMonMoveSlot(mon, MOVE_SURF, 0);

    ClearBag();
    EXPECT(MonCanUseFieldMove(mon, MOVE_SURF));
}

TEST("(HM-free) Non-HM field moves still have to be known")
{
    struct Pokemon *mon = SetUpLeadMon(SPECIES_ABRA);
    ClearBag();
    EXPECT(!MonCanUseFieldMove(mon, MOVE_DIG));
    EXPECT(!MonCanUseFieldMove(mon, MOVE_NONE));
}
