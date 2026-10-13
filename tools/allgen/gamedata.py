"""emerald-allgen: read species, move and learnset data straight from the expansion's C sources.

Only meant for the build scripts in this folder. Where the sources use #if / ternaries for
older generations, the first (newest) value is taken, matching the GEN_LATEST configs.
"""
import glob
import json
import os
import re
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
VANILLA_COMMIT = '7b4344c7'  # last upstream commit before emerald-allgen changes

SPECIAL_FLAGS = ['isLegendary', 'isMythical', 'isUltraBeast', 'isParadox', 'isSubLegendary',
                 'isRestrictedLegendary', 'isTotem', 'isMegaEvolution', 'isPrimalReversion',
                 'isGigantamax', 'isTeraForm', 'cannotBeTraded']
BATTLE_ONLY_FLAGS = ['isMegaEvolution', 'isPrimalReversion', 'isGigantamax', 'isTeraForm', 'isTotem']


def read(path):
    with open(os.path.join(ROOT, path), encoding='utf-8') as f:
        return f.read()


def vanilla(path):
    return subprocess.check_output(['git', '-C', ROOT, 'show', f'{VANILLA_COMMIT}:{path}']).decode('utf-8')


def _num(expr):
    m = re.search(r'(?:\?\s*)?(-?\d+)', expr)
    return int(m.group(1)) if m else None


def _first_field(body, field):
    """Value of `.field = ...` taking the first #if branch / first ternary value."""
    m = re.search(r'\.%s\s*=\s*([^\n]+)' % field, body)
    if not m:
        return None
    val = m.group(1).strip().rstrip(',')
    if '?' in val:
        val = val.split('?', 1)[1].split(':', 1)[0]
    return val.strip()


def load_species():
    db = {}
    for f in sorted(glob.glob(os.path.join(ROOT, 'src/data/pokemon/species_info/gen_*_families.h'))):
        s = open(f, encoding='utf-8').read()
        gen = int(re.search(r'gen_(\d)', f).group(1))
        for m in re.finditer(r'\n    \[SPECIES_(\w+)\] =\n    \{(.*?)\n    \},', s, re.S):
            name, body = m.group(1), m.group(2)
            d = {'gen': gen}
            dn = re.search(r'\.natDexNum = NATIONAL_DEX_(\w+)', body)
            d['dex'] = dn.group(1) if dn else None
            stats = []
            for st in ['baseHP', 'baseAttack', 'baseDefense', 'baseSpeed', 'baseSpAttack', 'baseSpDefense']:
                v = _first_field(body, st)
                stats.append((_num(v) if v else 0) or 0)
            d['stats'] = stats
            d['bst'] = sum(stats)
            tm = re.search(r'\.types = MON_TYPES\(([^)]*)\)', body)
            types = []
            if tm:
                for t in tm.group(1).split(','):
                    t = t.strip()
                    if t.startswith('TYPE_') and t[5:] not in types:
                        types.append(t[5:])
            d['types'] = types
            ab = re.search(r'\.abilities = \{([^}]*)\}', body)
            d['abilities'] = [a.strip()[8:] for a in ab.group(1).split(',')] if ab else []
            ev = re.search(r'\.evolutions = EVOLUTION\((.*?)\)\s*,\s*(?:\n|$)', body + '\n', re.S)
            evos = []
            if ev:
                for e in re.finditer(r'\{(EVO_\w+),\s*([^,{}]*),\s*SPECIES_(\w+)((?:,\s*CONDITIONS\(.*?\}\))?)\}',
                                     ev.group(1).replace('\n', ' ')):
                    evos.append({'method': e.group(1), 'param': e.group(2).strip(), 'target': e.group(3),
                                 'conditions': e.group(4)})
            d['evos'] = evos
            lu = re.search(r'\.levelUpLearnset = (\w+)', body)
            d['levelup_sym'] = lu.group(1) if lu else None
            te = re.search(r'\.teachableLearnset = (\w+)', body)
            d['teach_sym'] = te.group(1) if te else None
            for flag in SPECIAL_FLAGS:
                d[flag] = ('.%s = TRUE' % flag) in body
            db[name] = d
    # pre-evolution links
    for name, d in db.items():
        for e in d['evos']:
            if e['target'] in db:
                db[e['target']].setdefault('preevo', name)
    return db


def load_levelup():
    s = read('src/data/pokemon/level_up_learnsets/gen_9.h')
    out = {}
    for m in re.finditer(r'static const struct LevelUpMove (\w+)\[\] = \{(.*?)\};', s, re.S):
        out[m.group(1)] = [(int(a), b) for a, b in re.findall(r'LEVEL_UP_MOVE\(\s*(\d+),\s*MOVE_(\w+)\)', m.group(2))]
    return out


def load_teachable():
    s = read('src/data/pokemon/teachable_learnsets.h')
    out = {}
    for m in re.finditer(r'static const u16 (\w+)\[\] = \{(.*?)\};', s, re.S):
        out[m.group(1)] = [x for x in re.findall(r'MOVE_(\w+)', m.group(2)) if x != 'UNAVAILABLE']
    return out


def load_moves():
    s = read('src/data/moves_info.h')
    out = {}
    for m in re.finditer(r'\n    \[MOVE_(\w+)\] =\n    \{(.*?)\n    \},', s, re.S):
        name, body = m.group(1), m.group(2)
        power = _first_field(body, 'power')
        acc = _first_field(body, 'accuracy')
        t = re.search(r'\.type = TYPE_(\w+)', body)
        cat = re.search(r'\.category = DAMAGE_CATEGORY_(\w+)', body)
        eff = re.search(r'\.effect = (\w+)', body)
        pri = _first_field(body, 'priority')
        out[name] = {
            'power': _num(power) if power else 0,
            'accuracy': _num(acc) if acc else 0,
            'type': t.group(1) if t else 'NORMAL',
            'category': cat.group(1) if cat else 'STATUS',
            'effect': eff.group(1) if eff else '',
            'priority': _num(pri) if pri else 0,
            'recharge': 'MOVE_EFFECT_RECHARGE' in body,
            'charge': ('EFFECT_TWO_TURNS_ATTACK' in body) or ('EFFECT_SOLAR_BEAM' in body),
            'selfKO': ('EFFECT_EXPLOSION' in body) or ('EFFECT_FINAL_GAMBIT' in body) or ('EFFECT_MEMENTO' in body),
            'z': '.zMove' not in body and False,
        }
    return out


def hoenn_dex():
    s = read('include/constants/pokedex.h')
    a = s.index('#define FOREACH_SPECIES_IN_HOENN_DEX_ORDER')
    b = s.index('\n\n', a)
    return re.findall(r'F\((\w+)\)', s[a:b])


def wild_levels(json_text=None):
    """species -> lowest wild level it can be found at in Hoenn (Emerald maps only)."""
    d = json.loads(json_text) if json_text else json.loads(read('src/data/wild_encounters.json'))
    out = {}
    for e in d['wild_encounter_groups'][0]['encounters']:
        if 'Frlg' in e.get('base_label', '') or 'FireRed' in e.get('base_label', '') or 'LeafGreen' in e.get('base_label', ''):
            continue
        if e['map'].startswith('MAP_SEVEN_ISLAND') or e['map'] in ('MAP_ALTERING_CAVE',):
            continue
        for k in ['land_mons', 'water_mons', 'rock_smash_mons', 'fishing_mons']:
            for mon in e.get(k, {}).get('mons', []):
                sp = mon['species'][8:]
                out[sp] = min(out.get(sp, 999), mon['min_level'])
    return out


# When the player can first reach an encounter of each kind (rough trainer-level equivalents).
SURF_LEVEL, OLD_ROD_LEVEL, GOOD_ROD_LEVEL, SUPER_ROD_LEVEL, ROCK_SMASH_LEVEL, DIVE_LEVEL = 30, 12, 26, 40, 20, 45
SAFARI_LEVEL = 36


def wild_progress(json_text=None):
    """species -> roughly the earliest point (as a level) the player can catch it in Hoenn."""
    d = json.loads(json_text) if json_text else json.loads(read('src/data/wild_encounters.json'))
    grp = d['wild_encounter_groups'][0]
    fish = [f for f in grp['fields'] if f['type'] == 'fishing_mons'][0]['groups']
    out = {}

    def note(sp, lv):
        out[sp] = min(out.get(sp, 999), lv)
    for e in grp['encounters']:
        lbl, m = e.get('base_label', ''), e['map']
        if any(x in lbl for x in ('Frlg', 'FireRed', 'LeafGreen')) or m.startswith('MAP_SEVEN_ISLAND') or m == 'MAP_ALTERING_CAVE':
            continue
        land = e.get('land_mons', {}).get('mons', [])
        base = min([x['min_level'] for x in land] or [0])
        if 'SAFARI' in m:
            base = max(base, SAFARI_LEVEL)
        for x in land:
            note(x['species'][8:], max(base, x['min_level']))
        if 'UNDERWATER' in m:
            for x in e.get('water_mons', {}).get('mons', []):
                note(x['species'][8:], DIVE_LEVEL)
            continue
        for x in e.get('water_mons', {}).get('mons', []):
            note(x['species'][8:], max(SURF_LEVEL, base))
        for x in e.get('rock_smash_mons', {}).get('mons', []):
            note(x['species'][8:], max(ROCK_SMASH_LEVEL, base))
        mons = e.get('fishing_mons', {}).get('mons', [])
        for rod, lv in (('old_rod', OLD_ROD_LEVEL), ('good_rod', GOOD_ROD_LEVEL), ('super_rod', SUPER_ROD_LEVEL)):
            for i in fish[rod]:
                if i < len(mons):
                    note(mons[i]['species'][8:], max(lv, base))
    return out
