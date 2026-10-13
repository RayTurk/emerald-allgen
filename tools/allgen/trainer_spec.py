"""emerald-allgen: hand-picked teams for the story's key battles.

Each entry: trainer id -> list of (species, level[, held item]).
Moves, natures and IVs are filled in by build_trainers.py. Levels already include the
difficulty bump over vanilla. The last Pokemon listed is the ace.
"""

SMART = 'Smart Trainer'
BASIC = 'Basic Trainer'

# Gym leaders' first battles. Rematches (_2 to _5) are built from these teams.
GYM_TYPES = {
    'ROXANNE': 'ROCK', 'BRAWLY': 'FIGHTING', 'WATTSON': 'ELECTRIC', 'FLANNERY': 'FIRE',
    'NORMAN': 'NORMAL', 'WINONA': 'FLYING', 'TATE_AND_LIZA': 'PSYCHIC', 'JUAN': 'WATER',
}

BOSSES = {
    'TRAINER_ROXANNE_1': [('GEODUDE', 13), ('ROGGENROLA', 13), ('ROCKRUFF', 14), ('NOSEPASS', 16, 'ORAN_BERRY')],
    'TRAINER_BRAWLY_1': [('TIMBURR', 17), ('CROAGUNK', 17), ('MEDITITE', 18), ('MAKUHITA', 20, 'SITRUS_BERRY')],
    'TRAINER_WATTSON_1': [('LUXIO', 22), ('FLAAFFY', 22), ('PACHIRISU', 23), ('MAGNETON', 23), ('MANECTRIC', 25, 'SITRUS_BERRY')],
    'TRAINER_FLANNERY_1': [('GROWLITHE', 26), ('HOUNDOUR', 26), ('PONYTA', 27), ('CAMERUPT', 27), ('TORKOAL', 30, 'WHITE_HERB')],
    'TRAINER_NORMAN_1': [('BIBAREL', 29), ('GUMSHOOS', 29), ('VIGOROTH', 30), ('KANGASKHAN', 30), ('SLAKING', 33, 'SITRUS_BERRY')],
    'TRAINER_WINONA_1': [('TOUCANNON', 31), ('TROPIUS', 31), ('CRAMORANT', 32), ('SKARMORY', 33), ('ALTARIA', 35, 'SITRUS_BERRY')],
    'TRAINER_TATE_AND_LIZA_1': [('BRONZONG', 43), ('MALAMAR', 43), ('XATU', 43), ('CLAYDOL', 43), ('LUNATONE', 45, 'SITRUS_BERRY'), ('SOLROCK', 45, 'SITRUS_BERRY')],
    'TRAINER_JUAN_1': [('FLOATZEL', 44), ('JELLICENT', 44), ('DEWGONG', 45), ('GASTRODON', 45), ('CRAWDAUNT', 45), ('KINGDRA', 48, 'SITRUS_BERRY')],

    # Elite Four and Champion
    'TRAINER_SIDNEY': [('MIGHTYENA', 49), ('ZOROARK', 49), ('KROOKODILE', 50), ('WEAVILE', 50), ('BISHARP', 51), ('ABSOL', 53, 'SITRUS_BERRY')],
    'TRAINER_PHOEBE': [('BANETTE', 51), ('JELLICENT', 51), ('TREVENANT', 52), ('MISMAGIUS', 52), ('GENGAR', 53), ('DUSKNOIR', 55, 'SITRUS_BERRY')],
    'TRAINER_GLACIA': [('FROSLASS', 53), ('ABOMASNOW', 53), ('AVALUGG', 54), ('GLALIE', 54), ('MAMOSWINE', 55), ('WALREIN', 57, 'SITRUS_BERRY')],
    'TRAINER_DRAKE': [('ALTARIA', 55), ('DRAGALGE', 55), ('HAXORUS', 56), ('GOODRA', 56), ('GARCHOMP', 57), ('SALAMENCE', 59, 'SITRUS_BERRY')],
    'TRAINER_WALLACE': [('GYARADOS', 59), ('LUDICOLO', 59), ('GASTRODON', 59), ('LAPRAS', 59), ('GOLISOPOD', 60), ('MILOTIC', 62, 'SITRUS_BERRY')],
    'TRAINER_STEVEN': [('SKARMORY', 77), ('EXCADRILL', 77), ('AGGRON', 78), ('AEGISLASH_SHIELD', 78), ('CRADILY', 78), ('METAGROSS', 80, 'SITRUS_BERRY')],

    # Team leaders
    'TRAINER_MAXIE_MT_CHIMNEY': [('MIGHTYENA', 25), ('HOUNDOUR', 25), ('SANDILE', 25), ('CAMERUPT', 27, 'ORAN_BERRY')],
    'TRAINER_MAXIE_MAGMA_HIDEOUT': [('MIGHTYENA', 38), ('HOUNDOOM', 38), ('CROBAT', 39), ('KROOKODILE', 39), ('CAMERUPT', 41, 'SITRUS_BERRY')],
    'TRAINER_MAXIE_MOSSDEEP': [('MIGHTYENA', 43), ('HOUNDOOM', 43), ('CROBAT', 44), ('KROOKODILE', 44), ('CAMERUPT', 46, 'SITRUS_BERRY')],
    'TRAINER_ARCHIE': [('MIGHTYENA', 42), ('CROBAT', 42), ('TOXICROAK', 42), ('SEISMITOAD', 43), ('SHARPEDO', 45, 'SITRUS_BERRY')],

    # Wally at Victory Road and his rematches
    'TRAINER_WALLY_VR_1': [('ALTARIA', 45), ('WHIMSICOTT', 44), ('ROSERADE', 45), ('MAGNEZONE', 44), ('AROMATISSE', 44), ('GARDEVOIR', 47, 'SITRUS_BERRY')],
}

# Wally's Victory Road rematches keep his team and scale the levels.
WALLY_REMATCH_LEVEL_BONUS = {'TRAINER_WALLY_VR_2': 3, 'TRAINER_WALLY_VR_3': 6, 'TRAINER_WALLY_VR_4': 9, 'TRAINER_WALLY_VR_5': 12}

# Trainers left exactly as vanilla (Battle Frontier brains, placeholders).
UNTOUCHED_CLASSES = {'Salon Maiden', 'Pyramid King', 'Pike Queen', 'Palace Maven', 'Factory Head', 'Dome Ace',
                     'Arena Tycoon', 'RS Protag', 'Pkmn Trainer 1'}
UNTOUCHED_TRAINERS = {'TRAINER_RED', 'TRAINER_LEAF', 'TRAINER_WALLY_MAUVILLE'}
