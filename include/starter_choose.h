#ifndef GUARD_STARTER_CHOOSE_H
#define GUARD_STARTER_CHOOSE_H

extern const u16 gBirchBagGrass_Pal[];
extern const u32 gBirchBagTilemap[];
extern const u32 gBirchGrassTilemap[];
extern const u32 gBirchBagGrass_Gfx[];
extern const u32 gPokeballSelection_Gfx[];

// emerald-allgen: starters are grouped in sets of 3 (one Poké Ball each).
// Groups 0-8 are Gens 1-9 (grass, fire, water). The rest are special starters.
#define STARTERS_PER_GROUP      3
#define NUM_GEN_STARTER_GROUPS  9
#define NUM_SPECIAL_GROUPS      2
#define NUM_STARTER_GROUPS      (NUM_GEN_STARTER_GROUPS + NUM_SPECIAL_GROUPS)
#define NUM_STARTER_CHOICES     (NUM_STARTER_GROUPS * STARTERS_PER_GROUP)

u16 GetStarterPokemon(u16 chosenStarterId);
u16 GetChosenStarterPokemon(void);
bool32 IsChosenStarterSpecial(void);
u16 GetStarterRivalSlot(u16 chosenStarterId);
u16 GetRivalStarterSpecies(u16 species);
void CB2_ChooseStarter(void);

#endif // GUARD_STARTER_CHOOSE_H
