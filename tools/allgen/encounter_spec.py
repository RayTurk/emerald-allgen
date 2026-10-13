# emerald-allgen encounter additions.
# MAP -> field -> list of (species, weight). Weight is the target % for that species in the table.
# Fishing uses rod-specific fields: old_rod, good_rod, super_rod.
C, U, R, VR = 12, 6, 2, 1   # common / uncommon / rare / very rare target %
SEA_ROUTES_A = ["ROUTE124","ROUTE125","ROUTE126","ROUTE127"]
SEA_ROUTES_B = ["ROUTE128","ROUTE129","ROUTE130","ROUTE131"]
SEA_ROUTES_C = ["ROUTE132","ROUTE133","ROUTE134"]
EARLY_SEA = ["DEWFORD_TOWN","ROUTE106","ROUTE107","ROUTE108","ROUTE109"]
SPEC = {}
def add(maps, field, *mons):
    if isinstance(maps, str): maps=[maps]
    for m in maps:
        SPEC.setdefault(m, {}).setdefault(field, []).extend(mons)

# Early game
add("ROUTE101", "land", ("STARLY", C), ("BIDOOF", C))
add("ROUTE102", "land", ("SEWADDLE", C))
add("ROUTE103", "land", ("YUNGOOS", C), ("ROCKRUFF", U))
add("ROUTE103", "water", ("BUIZEL", 5))
add("ROUTE103", "super_rod", ("FINIZEN", 4))
add("ROUTE104", "land", ("PIKIPEK", C), ("FLABEBE", U), ("WIMPOD", U))
add("ROUTE104", "good_rod", ("SHELLOS", 20))
add("PETALBURG_WOODS", "land", ("CATERPIE", C), ("SCATTERBUG_MEADOW", C), ("FOONGUS", U), ("SKWOVET", U), ("CUTIEFLY", R))
add("ROUTE116", "land", ("SHINX", C))
add("RUSTURF_TUNNEL", "land", ("ROGGENROLA", C*2), ("DRILBUR", C*2), ("TIMBURR", C*2))
add(["GRANITE_CAVE_1F","GRANITE_CAVE_STEVENS_ROOM"], "land", ("ONIX", U), ("NOIBAT", U))
add(["GRANITE_CAVE_B1F","GRANITE_CAVE_B2F"], "land", ("BRONZOR", U), ("NOIBAT", U), ("ONIX", R))
add(EARLY_SEA, "water", ("MANTYKE", 4))
add(EARLY_SEA, "super_rod", ("KRABBY", 20))
add(EARLY_SEA, "super_rod", ("SHELLDER", 15))

# Mid game
add("ROUTE110", "land", ("MAREEP", C), ("PACHIRISU", U))
add("NEW_MAUVILLE_ENTRANCE", "land", ("KLINK", 20))
add("NEW_MAUVILLE_INSIDE", "land", ("KLINK", 20), ("ROTOM", VR))
add("ROUTE117", "land", ("PETILIL", C), ("COTTONEE", C))
add("ROUTE117", "super_rod", ("TYMPOLE", 20))
add("ROUTE111", "land", ("SANDILE", C), ("HIPPOPOTAS", U), ("SKORUPI", U), ("SILICOBRA", C))
add(["MIRAGE_TOWER_1F","MIRAGE_TOWER_2F","MIRAGE_TOWER_3F","MIRAGE_TOWER_4F"], "land", ("GIBLE", VR))
add("DESERT_UNDERPASS", "land", ("GIBLE", R))
add("ROUTE112", "land", ("GROWLITHE", C), ("PONYTA", C))
add("FIERY_PATH", "land", ("SIZZLIPEDE", C), ("GROWLITHE", U), ("PONYTA", U))
add("JAGGED_PASS", "land", ("HOUNDOUR", C), ("MANKEY", C), ("LARVESTA", VR))
add("ROUTE113", "land", ("TRUBBISH", C))
add("ROUTE114", "land", ("CROAGUNK", C))
add("ROUTE114", "super_rod", ("BASCULIN", 15))
add(["METEOR_FALLS_1F_1R"], "land", ("CLEFAIRY", C), ("AXEW", U), ("MINIOR", VR))
add(["METEOR_FALLS_1F_2R","METEOR_FALLS_B1F_1R","METEOR_FALLS_B1F_2R","METEOR_FALLS_STEVENS_CAVE"], "land", ("CLEFAIRY", C), ("AXEW", U), ("MINIOR", R))

# Late game
add("ROUTE118", "land", ("SCYTHER", U), ("BOUNSWEET", C), ("FOMANTIS", C), ("FERROSEED", U))
add("ROUTE119", "land", ("SCYTHER", U), ("STEENEE", U), ("FERROSEED", U), ("GOOMY", R))
add("ROUTE119", "water", ("DEWPIDER", 5))
add("ROUTE120", "land", ("PAWNIARD", U), ("ZORUA", R))
add(["ROUTE121","ROUTE123"], "land", ("INKAY", C), ("SPRITZEE", U))
add("ROUTE115", "land", ("SANDYGAST", U))
add(["MT_PYRE_1F","MT_PYRE_2F","MT_PYRE_3F"], "land", ("GASTLY", 20), ("YAMASK", 20), ("HONEDGE", 10))
add(["MT_PYRE_4F","MT_PYRE_5F","MT_PYRE_6F"], "land", ("MISDREAVUS", 20), ("HAUNTER", 10), ("HONEDGE", 10))
add("MT_PYRE_EXTERIOR", "land", ("PHANTUMP", C), ("MISDREAVUS", U), ("SINISTEA", R))
add("MT_PYRE_SUMMIT", "land", ("PHANTUMP", U), ("HONEDGE", U), ("SINISTEA", R))
add(["SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM","SHOAL_CAVE_LOW_TIDE_INNER_ROOM","SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM","SHOAL_CAVE_LOW_TIDE_LOWER_ROOM"], "land", ("SWINUB", C), ("SNOVER", U), ("BERGMITE", U))
add("SHOAL_CAVE_LOW_TIDE_ICE_ROOM", "land", ("SNEASEL", U), ("SNOVER", U), ("BERGMITE", U))
add(SEA_ROUTES_A, "water", ("SEEL", 5), ("FRILLISH", 4))
add([r for r in SEA_ROUTES_B if r != "ROUTE129"], "water", ("-PELIPPER", 0), ("FRILLISH", 5), ("CRAMORANT", 4), ("LAPRAS", 1))
add("ROUTE129", "water", ("-PELIPPER", 0), ("FRILLISH", 5), ("CRAMORANT", 4))
add(SEA_ROUTES_C, "water", ("-PELIPPER", 0), ("SEEL", 5), ("CRAMORANT", 4), ("LAPRAS", 1))
add(SEA_ROUTES_A + SEA_ROUTES_C, "super_rod", ("WIGLETT", 15))
add(SEA_ROUTES_B, "super_rod", ("ARROKUDA", 15))
add(SEA_ROUTES_A + SEA_ROUTES_C, "super_rod", ("ARROKUDA", 15))
add("UNDERWATER_ROUTE124", "water", ("QWILFISH", 5), ("SKRELP", 4))
add("UNDERWATER_ROUTE126", "water", ("SKRELP", 5), ("ALOMOMOLA", 4))
add(["SAFARI_ZONE_SOUTH","SAFARI_ZONE_SOUTHWEST"], "land", ("TAUROS", U), ("KANGASKHAN", U))
add(["SAFARI_ZONE_NORTH","SAFARI_ZONE_NORTHWEST"], "land", ("CHANSEY", VR), ("TAUROS", U))
add("VICTORY_ROAD_1F", "land", ("PUPITAR", R))
add(["VICTORY_ROAD_B1F","VICTORY_ROAD_B2F"], "land", ("DEINO", R), ("PUPITAR", VR))
