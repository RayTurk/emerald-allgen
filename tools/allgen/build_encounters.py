# emerald-allgen: rebuilds src/data/wild_encounters.json from the vanilla tables plus encounter_spec.py.
# Always starts from the upstream (vanilla) file in git, so it is safe to rerun after editing the spec.
# Usage (from the repo root): python3 tools/allgen/build_encounters.py
import json, os, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from encounter_spec import SPEC
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
P = os.path.join(ROOT, 'src/data/wild_encounters.json')
VANILLA_COMMIT = '7b4344c7'  # last upstream commit before emerald-allgen changes
d = json.loads(subprocess.check_output(['git', '-C', ROOT, 'show', VANILLA_COMMIT + ':src/data/wild_encounters.json']))
grp=d['wild_encounter_groups'][0]
rates={f['type']:f['encounter_rates'] for f in grp['fields']}
fish_groups={k:v for f in grp['fields'] if f['type']=='fishing_mons' for k,v in f.get('groups',{}).items()}

def assign(slot_rates, targets):
    """targets: list of (species, target%, meta). Returns list of species index per slot."""
    order=sorted(range(len(slot_rates)), key=lambda i:-slot_rates[i])
    got={i:0.0 for i in range(len(targets))}; cnt={i:0 for i in range(len(targets))}
    out=[None]*len(slot_rates)
    for n,slot in enumerate(order):
        left=len(order)-n
        unassigned=[i for i in got if cnt[i]==0]
        pool=unassigned if len(unassigned)>=left else list(got)
        # pick highest remaining deficit; unassigned breaks ties
        best=max(pool, key=lambda i:(targets[i][1]-got[i], cnt[i]==0, targets[i][1]))
        out[slot]=best; got[best]+=slot_rates[slot]; cnt[best]+=1
    missing=[targets[i][0] for i in got if cnt[i]==0]
    if missing: raise SystemExit(f'not enough slots for {missing}')
    return out

def rebuild(mons, slot_rates, additions, label, new_levels='first'):
    drops={'SPECIES_'+sp[1:] for sp,_ in additions if sp.startswith('-')}
    additions=[(sp,w) for sp,w in additions if not sp.startswith('-')]
    # vanilla species -> share and level range
    van={}
    for i,m in enumerate(mons):
        if m['species'] in drops: continue
        v=van.setdefault(m['species'], {'share':0,'min':m['min_level'],'max':m['max_level']})
        v['share']+=slot_rates[i]; v['min']=min(v['min'],m['min_level']); v['max']=max(v['max'],m['max_level'])
    new_total=sum(w for _,w in additions)
    scale=(100-new_total)/100.0
    first=mons[0]
    if new_levels=='late':
        first={'min_level':max(x['min_level'] for x in mons),'max_level':max(x['max_level'] for x in mons)}
    targets=[(sp, v['share']*scale, (v['min'],v['max'])) for sp,v in van.items()]
    for sp,w in additions:
        sp='SPECIES_'+sp
        if sp in van: continue
        targets.append((sp, float(w), (first['min_level'], first['max_level'])))
    if len(targets)>len(slot_rates):
        # drop the smallest vanilla species that share a table with many others
        raise SystemExit(f'{label}: {len(targets)} species for {len(slot_rates)} slots')
    pick=assign(slot_rates, targets)
    # keep per-slot vanilla levels where the vanilla species stays in a slot it used, else use its range
    res=[]
    for i,t in enumerate(pick):
        sp,_,(lo,hi)=targets[t]
        if i < len(mons) and mons[i]['species']==sp:
            lo,hi=mons[i]['min_level'],mons[i]['max_level']
        res.append({'min_level':lo,'max_level':hi,'species':sp})
    return res

ROD_FIELDS={'old_rod':'old_rod','good_rod':'good_rod','super_rod':'super_rod'}
touched=set()
for e in grp['encounters']:
    m=e['map'].replace('MAP_','')
    if m not in SPEC: continue
    for field, adds in SPEC[m].items():
        if field in ('land','water','rock_smash'):
            key=field+'_mons'
            if key not in e: raise SystemExit(f'{m} has no {key}')
            e[key]['mons']=rebuild(e[key]['mons'], rates[key], adds, f'{m}.{field}', 'first' if field=='land' else 'late')
        else:
            key='fishing_mons'
            if key not in e: raise SystemExit(f'{m} has no fishing')
            idx=fish_groups[field]
            mons=e[key]['mons']; sub=[mons[i] for i in idx]
            subr=[rates[key][i] for i in idx]
            tot=sum(subr); subr_pct=[r*100.0/tot for r in subr]
            new=rebuild(sub, subr_pct, adds, f'{m}.{field}', 'late')
            for j,i in enumerate(idx): mons[i]=new[j]
        touched.add(m)
missing=set(SPEC)-touched
if missing: raise SystemExit(f'maps not found: {missing}')
json.dump(d, open(P,'w'), indent=2, ensure_ascii=False); open(P,'a').write('\n')
print('updated', len(touched), 'maps')
