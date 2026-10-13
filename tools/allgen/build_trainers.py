"""emerald-allgen: rebuild src/data/trainers.party with cross-generation teams.

Starts from the vanilla file in git every run, so it is safe to rerun after editing
trainer_spec.py or the rules below.

  * Key battles (gyms, Elite Four, Champion, team leaders, Wally, Steven) use the
    hand-picked teams in trainer_spec.py, with moves chosen here.
  * Every other trainer keeps its party size, levels (plus a small bump) and items,
    and each Pokemon is swapped for a Hoenn+ species that shares a type with the
    one it replaces, is about as strong, and is findable by that point in the game.
    About a third of the vanilla Pokemon are kept for Hoenn flavor.
  * The rival's starter slot is left alone: the game swaps it at battle time to
    the line that counters the player's starter.

Usage (from the repo root): python3 tools/allgen/build_trainers.py
"""
import hashlib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gamedata as gd
import trainer_spec as spec

KEEP_VANILLA_CHANCE = 0.2
USAGE = {}  # family -> times picked so far, to spread picks across more species
PSEUDO_ROOTS = {'LARVITAR', 'GIBLE', 'DEINO', 'GOOMY', 'BAGON', 'BELDUM', 'DRATINI', 'AXEW', 'LARVESTA'}
PSEUDO_CLASSES = {'Cooltrainer', 'Expert', 'Dragon Tamer', 'Pkmn Ranger', 'Cooltrainer 2'}
STARTER_ROOTS = {'TREECKO', 'TORCHIC', 'MUDKIP'}
HOENN_STARTER_LINES = {'TREECKO', 'GROVYLE', 'SCEPTILE', 'TORCHIC', 'COMBUSKEN', 'BLAZIKEN', 'MUDKIP', 'MARSHTOMP', 'SWAMPERT'}
NEVER_USE = {'SHEDINJA', 'DITTO', 'SMEARGLE', 'UNOWN', 'WYNAUT', 'WOBBUFFET', 'CHINGLING', 'MUNCHLAX', 'HAPPINY',
             'CLEFFA', 'BUDEW', 'AZURILL', 'PICHU', 'IGGLYBUFF', 'MANTYKE', 'FEEBAS', 'MAGIKARP', 'CASTFORM_NORMAL'}
FORM_CANON = {'CASTFORM': 'CASTFORM_NORMAL', 'LYCANROC': 'LYCANROC_MIDDAY', 'PALAFIN': 'PALAFIN_ZERO',
              'SHELLOS': 'SHELLOS_WEST', 'GASTRODON': 'GASTRODON_WEST', 'BASCULIN': 'BASCULIN_RED_STRIPED',
              'AEGISLASH': 'AEGISLASH_SHIELD', 'SINISTEA': 'SINISTEA_PHONY', 'POLTEAGEIST': 'POLTEAGEIST_PHONY',
              'DEOXYS': 'DEOXYS_NORMAL'}
GOOD_STATUS = ['SWORDS_DANCE', 'DRAGON_DANCE', 'NASTY_PLOT', 'CALM_MIND', 'BULK_UP', 'QUIVER_DANCE', 'SHELL_SMASH',
               'WILL_O_WISP', 'THUNDER_WAVE', 'SPORE', 'SLEEP_POWDER', 'STEALTH_ROCK', 'LEECH_SEED',
               'RECOVER', 'ROOST', 'SLACK_OFF', 'SOFT_BOILED', 'MOONLIGHT', 'SYNTHESIS', 'TRICK_ROOM', 'AGILITY',
               'IRON_DEFENSE', 'COTTON_GUARD', 'COIL', 'HONE_CLAWS', 'WORK_UP', 'GROWTH', 'TAIL_GLOW', 'SHIFT_GEAR',
               'ROCK_POLISH', 'STICKY_WEB', 'SPIKES', 'TOXIC_SPIKES', 'YAWN', 'HYPNOSIS', 'STUN_SPORE', 'GLARE',
               'PROTECT', 'LIGHT_SCREEN', 'REFLECT', 'TAUNT', 'TOXIC']
BAD_ATTACKS = {'FOCUS_PUNCH', 'SELF_DESTRUCT', 'EXPLOSION', 'MISTY_EXPLOSION', 'FINAL_GAMBIT', 'DREAM_EATER',
               'SNORE', 'FAKE_OUT', 'FIRST_IMPRESSION', 'BELCH', 'SYNCHRONOISE', 'LAST_RESORT', 'SKY_DROP',
               'FUTURE_SIGHT', 'DOOM_DESIRE', 'NATURAL_GIFT', 'SPIT_UP', 'BIDE', 'COUNTER', 'MIRROR_COAT',
               'METAL_BURST', 'ENDEAVOR', 'FLAIL', 'REVERSAL', 'PAYBACK', 'GYRO_BALL', 'ELECTRO_BALL', 'TRUMP_CARD',
               'HIDDEN_POWER', 'RETURN', 'FRUSTRATION', 'SECRET_POWER', 'PRESENT', 'MAGNITUDE', 'STEEL_BEAM',
               'HYPER_BEAM', 'GIGA_IMPACT', 'SOLAR_BEAM', 'SOLAR_BLADE', 'SKY_ATTACK', 'RAZOR_WIND', 'SKULL_BASH',
               'DIG', 'FLY', 'DIVE', 'BOUNCE', 'PHANTOM_FORCE', 'SHADOW_FORCE', 'METEOR_BEAM', 'BURN_UP',
               'DOUBLE_SHOCK', 'STRUGGLE', 'RAGE_FIST', 'MIRROR_MOVE', 'COPYCAT'}

db = gd.load_species()
moves = gd.load_moves()
levelup = gd.load_levelup()
teach = gd.load_teachable()
hoenn_plus = gd.hoenn_dex()
wild = gd.wild_progress()


def h(*parts):
    return int(hashlib.sha1('|'.join(map(str, parts)).encode()).hexdigest()[:8], 16) / 0xFFFFFFFF


def canon(name):
    return FORM_CANON.get(name, name)


def name_to_key(text):
    """Showdown-style species name -> SPECIES_ key (same rules as tools/trainerproc)."""
    t = text.strip()
    if t.startswith('SPECIES_'):
        return t[8:]
    t = t.replace('♂', '_M').replace('♀', '_F').replace('é', 'E').replace("'", '').replace('’', '')
    t = re.sub(r'[^A-Za-z0-9]+', '_', t).strip('_').upper()
    return t


# ---------------------------------------------------------------- species progression
def root_of(sp):
    seen = set()
    while db.get(sp, {}).get('preevo') and sp not in seen:
        seen.add(sp)
        sp = db[sp]['preevo']
    return sp


def family_members(root):
    out, stack = [], [root]
    while stack:
        x = stack.pop()
        if x in out or x not in db:
            continue
        out.append(x)
        stack += [e['target'] for e in db[x]['evos'] if e['target'] in db]
    return out


def dex_of(sp):
    return db.get(sp, {}).get('dex') or sp


HOENN_PLUS_DEX = set(hoenn_plus)
dex_wild = {}
for sp, lv in wild.items():
    key = canon(sp) if sp in db else sp
    d = dex_of(key) if key in db else sp.split('_')[0]
    dex_wild[d] = min(dex_wild.get(d, 999), lv)


def family_avail(root):
    lv = min([dex_wild.get(dex_of(m), 999) for m in family_members(root)] + [999])
    return lv if lv < 999 else 25  # gift / fossil / static Pokemon: assume mid-game


_req_cache = {}


def req_level(sp):
    """Lowest trainer level at which this species looks natural."""
    if sp in _req_cache:
        return _req_cache[sp]
    pre = db[sp].get('preevo')
    if not pre:
        r = family_avail(sp)
    else:
        parent = req_level(pre)
        thr = None
        for e in db[pre]['evos']:
            if e['target'] != sp:
                continue
            p = e['param']
            if e['method'] == 'EVO_LEVEL' and p.isdigit() and int(p) > 0:
                t = int(p)
            elif e['method'] == 'EVO_LEVEL':
                t = max(parent + 10, 20)
            elif e['method'] == 'EVO_ITEM':
                t = max(parent + 12, 25)
            else:
                t = max(parent + 15, 30)
            thr = t if thr is None else min(thr, t)
        r = max(parent + 2, thr if thr is not None else parent + 15)
    _req_cache[sp] = r
    return r


def stage_for(root, level, salt):
    """Most evolved member of the family a trainer would plausibly have at this level."""
    cur = root
    while True:
        kids = [e['target'] for e in db[cur]['evos']
                if e['target'] in db and not any(db[e['target']].get(f) for f in gd.BATTLE_ONLY_FLAGS)
                and 'IF_REGION' not in (e['conditions'] or '') and db[e['target']]['bst'] > 0]
        kids = sorted(set(kids))
        ok = [k for k in kids if req_level(k) <= level]
        if not ok:
            return cur
        cur = ok[int(h(salt, cur) * len(ok)) % len(ok)]


def usable(sp):
    d = db.get(sp)
    if not d or d['bst'] == 0:
        return False
    if any(d.get(f) for f in ['isLegendary', 'isMythical', 'isUltraBeast', 'isParadox', 'isSubLegendary',
                              'isRestrictedLegendary'] + gd.BATTLE_ONLY_FLAGS):
        return False
    return sp not in NEVER_USE


# Trainer pool: families from the Hoenn+ dex
POOL_ROOTS = []
for dexname in hoenn_plus:
    sp = canon(dexname)
    if sp not in db:
        continue
    r = root_of(sp)
    if r in POOL_ROOTS or r in STARTER_ROOTS or not usable(r) and not usable(sp):
        continue
    if any(m in HOENN_STARTER_LINES for m in family_members(r)):
        continue
    POOL_ROOTS.append(r)
VANILLA_HOENN = set(gd.vanilla('include/constants/pokedex.h').split('#define FOREACH_SPECIES_IN_HOENN_DEX_ORDER')[1].split('\n\n')[0].replace('F(', ' ').replace(')', ' ').split())


def pick_replacement(vsp, level, cls, salt, taken):
    vt = set(db[vsp]['types']) if vsp in db else set()
    vb = db[vsp]['bst'] if vsp in db else 300
    allow_pseudo = cls in PSEUDO_CLASSES and level >= 30
    for relax in range(3):
        cands = []
        for r in POOL_ROOTS:
            if r in taken or r == root_of(vsp):
                continue
            if r in PSEUDO_ROOTS and not allow_pseudo:
                continue
            if family_avail(r) > level + 2:
                continue
            sp = stage_for(r, level, salt)
            if not usable(sp):
                continue
            d = db[sp]
            if relax < 2 and vt and not (vt & set(d['types'])):
                continue
            lo, hi = (0.75 * vb, 1.2 * vb + 20) if relax == 0 else (0.6 * vb, 1.35 * vb + 30)
            if not (lo <= d['bst'] <= hi):
                continue
            w = 6.0 if dex_of(sp) not in VANILLA_HOENN else 1.0
            w /= 1.0 + USAGE.get(r, 0) / 3.0
            cands.append((sp, r, w))
        if cands:
            tot = sum(c[2] for c in cands)
            x = h(salt, 'pick') * tot
            for sp, r, w in cands:
                x -= w
                if x <= 0:
                    break
            USAGE[r] = USAGE.get(r, 0) + 1
            return sp, r
    return None, None


# ---------------------------------------------------------------- moves for key battles
def learnable(sp, level, allow_tm):
    out = {}
    chain = [sp]
    while db[chain[-1]].get('preevo'):
        chain.append(db[chain[-1]]['preevo'])
    for s in chain:
        for lv, mv in levelup.get(db[s]['levelup_sym'], []):
            if lv <= level and mv in moves:
                out[mv] = 'level'
    if allow_tm:
        for mv in teach.get(db[sp]['teach_sym'], []):
            if mv in moves:
                out.setdefault(mv, 'tm')
    return out


def choose_moves(sp, level):
    d = db[sp]
    atk, spa = d['stats'][1], d['stats'][4]
    phys = atk >= spa
    cap = 45 + int(level * 2.2)
    pool = learnable(sp, level, allow_tm=level >= 20)
    scored = []
    for mv in pool:
        m = moves[mv]
        if m['category'] == 'STATUS' or m['power'] <= 1 or mv in BAD_ATTACKS or m['recharge'] or m['charge'] or m['selfKO']:
            continue
        if m['power'] > cap:
            continue
        s = m['power'] * (m['accuracy'] or 100) / 100.0
        if m['type'] in d['types']:
            s *= 1.5
        s *= 1.15 if (m['category'] == 'PHYSICAL') == phys else 0.75
        if m['priority'] > 0:
            s *= 0.9
        if 'RECOIL' in m['effect'] or mv in ('DOUBLE_EDGE', 'TAKE_DOWN', 'WOOD_HAMMER', 'BRAVE_BIRD', 'FLARE_BLITZ', 'HEAD_SMASH', 'WAVE_CRASH', 'VOLT_TACKLE', 'HEAD_CHARGE', 'SUBMISSION', 'WILD_CHARGE'):
            s *= 0.85
        if m['type'] == 'NORMAL' and 'NORMAL' not in d['types']:
            s *= 0.8
        scored.append((s, mv, m['type']))
    scored.sort(reverse=True)
    picked, types = [], set()
    for s, mv, t in scored:           # best STAB of each type first
        if t in d['types'] and t not in types:
            picked.append(mv); types.add(t)
    for s, mv, t in scored:           # then coverage
        if len(picked) >= 3:
            break
        if t not in types:
            picked.append(mv); types.add(t)
    status = [mv for mv in GOOD_STATUS if mv in pool]
    if status and len(picked) >= 2:
        picked = picked[:3] + [status[0]]
    for s, mv, t in scored:           # fill remaining slots with the next best attacks
        if len(picked) >= 4:
            break
        if mv not in picked:
            picked.append(mv)
    return picked[:4]


def nature_for(sp):
    st = db[sp]['stats']
    if st[1] >= st[4]:
        return 'Jolly' if st[3] >= 100 and st[1] < 120 else 'Adamant'
    return 'Timid' if st[3] >= 100 and st[4] < 120 else 'Modest'


def ivs_for(level):
    v = 15 if level < 20 else 20 if level < 35 else 25 if level < 50 else 31
    return f'{v} HP / {v} Atk / {v} Def / {v} SpA / {v} SpD / {v} Spe'


# ---------------------------------------------------------------- party file io
def parse_party(text):
    head, _, rest = text.partition('\n=== ')
    blocks = ('=== ' + rest).split('\n=== ')
    trainers = []
    for i, b in enumerate(blocks):
        if not b.strip():
            continue
        b = b if b.startswith('=== ') else '=== ' + b
        chunks = b.strip('\n').split('\n\n')
        header = chunks[0].split('\n')
        tid = re.match(r'=== (\w+) ===', header[0]).group(1)
        fields = [ln for ln in header[1:]]
        mons = []
        for c in chunks[1:]:
            lines = c.split('\n')
            m = re.match(r'^(?P<sp>[^@()]+?)(?: \((?P<g>[MF])\))?(?: @ (?P<item>.+))?$', lines[0].strip())
            mon = {'species': name_to_key(m.group('sp')), 'gender': m.group('g'), 'item': m.group('item'),
                   'fields': [], 'moves': []}
            for ln in lines[1:]:
                if ln.startswith('- '):
                    mon['moves'].append(ln[2:].strip())
                elif ln.startswith('Level:'):
                    mon['level'] = int(ln.split(':')[1])
                else:
                    mon['fields'].append(ln)
            mons.append(mon)
        trainers.append({'id': tid, 'fields': fields, 'mons': mons})
    return head + '\n', trainers


def field(tr, key):
    for ln in tr['fields']:
        if ln.startswith(key + ':'):
            return ln.split(':', 1)[1].strip()
    return None


def set_field(tr, key, value):
    for i, ln in enumerate(tr['fields']):
        if ln.startswith(key + ':'):
            tr['fields'][i] = f'{key}: {value}'
            return
    tr['fields'].append(f'{key}: {value}')


def item_const(item):
    if not item:
        return None
    return item if item.startswith('ITEM_') else 'ITEM_' + re.sub(r'[^A-Za-z0-9]+', '_', item.replace("'", '')).strip('_').upper()


def write_party(head, trainers):
    out = [head.rstrip('\n') + '\n']
    for tr in trainers:
        out.append(f"=== {tr['id']} ===")
        out.extend(tr['fields'])
        for mon in tr['mons']:
            out.append('')
            sp = mon['species']
            line = sp if mon.get('raw_name') else 'SPECIES_' + sp
            if mon.get('raw_name'):
                line = mon['raw_name']
            if mon.get('gender'):
                line += f" ({mon['gender']})"
            if mon.get('item'):
                line += f" @ {mon['item']}"
            out.append(line)
            out.append(f"Level: {mon['level']}")
            out.extend(mon['fields'])
            out.extend('- ' + mv for mv in mon['moves'])
        out.append('')
    return '\n'.join(out)


# ---------------------------------------------------------------- build
def bump(level):
    if level <= 10:
        return level + 1
    if level <= 30:
        return level + 2
    return min(100, level + 3)


def boss_mon(sp, level, item, salt):
    sp = canon(sp)
    if sp not in db:
        raise SystemExit(f'unknown species {sp} in trainer_spec ({salt})')
    mon = {'species': sp, 'gender': None, 'item': item_const(item) if item else None, 'level': level,
           'fields': [f'IVs: {ivs_for(level)}', f'Nature: {nature_for(sp)}'],
           'moves': ['MOVE_' + m for m in choose_moves(sp, level)]}
    return mon


def evolve_fully(sp, level, salt):
    return stage_for(root_of(sp), level, salt)


def build():
    head, trainers = parse_party(gd.vanilla('src/data/trainers.party'))
    by_id = {t['id']: t for t in trainers}
    stats = {'kept': 0, 'swapped': 0, 'boss': 0, 'fallback': 0}
    for tr in trainers:
        tid, cls = tr['id'], field(tr, 'Class')
        if cls in spec.UNTOUCHED_CLASSES or tid in spec.UNTOUCHED_TRAINERS or tid == 'TRAINER_NONE':
            continue
        if tid in spec.BOSSES:
            tr['mons'] = [boss_mon(e[0], e[1], e[2] if len(e) > 2 else None, tid) for e in spec.BOSSES[tid]]
            set_field(tr, 'AI', spec.SMART if cls in ('Leader', 'Elite Four', 'Champion') or tid == 'TRAINER_STEVEN' else spec.BASIC)
            stats['boss'] += len(tr['mons'])
            continue
        if tid in spec.WALLY_REMATCH_LEVEL_BONUS:
            base = spec.BOSSES['TRAINER_WALLY_VR_1']
            bonus = spec.WALLY_REMATCH_LEVEL_BONUS[tid]
            tr['mons'] = [boss_mon(e[0], e[1] + bonus, e[2] if len(e) > 2 else None, tid) for e in base]
            set_field(tr, 'AI', spec.BASIC)
            stats['boss'] += len(tr['mons'])
            continue
        m = re.match(r'TRAINER_(\w+)_([2-5])$', tid)
        if cls == 'Leader' and m and f'TRAINER_{m.group(1)}_1' in spec.BOSSES:
            build_gym_rematch(tr, m.group(1), int(m.group(2)))
            stats['boss'] += len(tr['mons'])
            continue
        # regular trainer
        taken = set()
        for i, mon in enumerate(tr['mons']):
            mon['level'] = bump(mon['level'])
            vsp = mon['species']
            if cls == 'Rival' and vsp in HOENN_STARTER_LINES:
                taken.add(root_of(vsp))
                continue
            keep = h(tid, i, 'keep') < KEEP_VANILLA_CHANCE and vsp in db
            if not keep:
                sp, r = pick_replacement(vsp, mon['level'], cls, f'{tid}:{i}', taken)
                if sp:
                    mon['species'] = sp
                    mon['moves'] = []
                    mon['fields'] = [f for f in mon['fields'] if not f.startswith(('Ability:', 'Tera Type:'))]
                    taken.add(r)
                    stats['swapped'] += 1
                    continue
                stats['fallback'] += 1
            # kept vanilla species: evolve it if the new level calls for it
            if vsp in db:
                taken.add(root_of(vsp))
            stats['kept'] += 1
    return head, trainers, stats


def build_gym_rematch(tr, leader, n):
    gtype = spec.GYM_TYPES[leader]
    base = spec.BOSSES[f'TRAINER_{leader}_1']
    count = len(tr['mons'])
    levels = sorted(bump(m['level']) for m in tr['mons'])
    team, roots = [], set()
    for e in base:                                   # the first-battle team, evolved
        sp = evolve_fully(canon(e[0]), levels[-1], f'{leader}{n}')
        if root_of(sp) not in roots:
            team.append(sp); roots.add(root_of(sp))
    extras = []
    for r in POOL_ROOTS:                             # strongest same-type families to fill out the party
        if r in roots:
            continue
        sp = stage_for(r, levels[-1], f'{leader}{n}x')
        if usable(sp) and gtype in db[sp]['types'] and req_level(sp) <= levels[-1]:
            extras.append((db[sp]['bst'], sp))
    extras.sort(reverse=True)
    target = min(6, max(count, len(base) + n - 1))
    ace = team[-1]
    for _, sp in extras:
        if len(team) >= target:
            break
        team.insert(len(team) - 1, sp)
    team = team[:target - 1] + [ace] if len(team) > target else team
    lv_lo, lv_hi = levels[0], levels[-1]
    mons = []
    for i, sp in enumerate(team):
        lv = lv_hi if sp == ace else lv_lo + (i % 3)
        mons.append(boss_mon(sp, lv, 'SITRUS_BERRY' if sp == ace else None, tr['id']))
    tr['mons'] = mons
    set_field(tr, 'AI', spec.SMART)


def validate(trainers):
    """The test ROM doesn't load real trainer data, so the parties are checked here instead."""
    problems = []
    for tr in trainers:
        if tr['id'] in ('TRAINER_BRENDAN_PLACEHOLDER', 'TRAINER_MAY_PLACEHOLDER', 'TRAINER_NONE'):
            continue
        if len(tr['mons']) > 6:
            problems.append(f"{tr['id']}: {len(tr['mons'])} Pokemon")
        if field(tr, 'Double Battle') == 'Yes' and len(tr['mons']) < 2:
            problems.append(f"{tr['id']}: double battle with one Pokemon")
        for mon in tr['mons']:
            d = db.get(mon['species'])
            if not d or d['bst'] == 0:
                problems.append(f"{tr['id']}: unknown species {mon['species']}")
            elif any(d.get(f) for f in ('isRestrictedLegendary', 'isSubLegendary', 'isMythical', 'isUltraBeast', 'isParadox')):
                problems.append(f"{tr['id']}: legendary {mon['species']}")
            if not 1 <= mon['level'] <= 100:
                problems.append(f"{tr['id']}: level {mon['level']}")
            for mv in mon['moves']:
                key = mv[5:] if mv.startswith('MOVE_') else re.sub(r'[^A-Za-z0-9]+', '_', mv).upper()
                if key not in moves:
                    problems.append(f"{tr['id']}: unknown move {mv}")
    if problems:
        raise SystemExit('trainer data problems:\n  ' + '\n  '.join(problems))


if __name__ == '__main__':
    head, trainers, stats = build()
    validate(trainers)
    path = os.path.join(gd.ROOT, 'src/data/trainers.party')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(write_party(head, trainers))
    print('trainers:', len(trainers), stats)
