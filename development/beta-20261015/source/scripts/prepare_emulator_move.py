#!/usr/bin/env python3
"""Plan portable ROM/art paths and NES save-key migration from captured configs."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'.context/amiga/emulator-beta'
# Leave eight characters for .sav.bak/.sav.tmp even on 28-character volumes.
SHORT_NAMES={
 '002 Super Mario World.nes':'SuperMarioWorld.nes',
 'Asterix (Europe) (En,Fr,De,Es,It).nes':'Asterix (E).nes',
 'Aussie Rules Footy (Australia).nes':'AussieRules (E).nes',
 "Castlevania III - Dracula's Curse (USA).nes":'Castlevania3 US.nes',
 'Contra Force (USA).nes':'Contra Force US.nes',
 'Donkey Kong 3 (World).nes':'Donkey Kong 3.nes',
 "Hammerin' Harry (Europe).nes":'HammerHarry (E).nes',
 'Library-System-Test.sfc':'Library-Test.sfc',
 'Aero Fighters (USA).sfc':'AeroFighters US.sfc',
 'Another World (Europe).sfc':'AnotherWorld EU.sfc',
 'Battletoads-Double Dragon (USA).sfc':'BattletoadsDD US.sfc',
 'Cannon Fodder (Europe).sfc':'CannonFodder EU.sfc',
 'Chaos Engine, The (Europe).sfc':'Chaos Engine EU.sfc',
 'Contra III - The Alien Wars (USA).sfc':'Contra III US.sfc',
}

def path_hash(value):
 h=2166136261
 for b in value.encode('latin1'):h=((h^b)*16777619)&0xffffffff
 return h
def prepare(console):
 short='nes' if console=='ZZNES' else 'snes';old='SD032G:Dev/'+('ZZNes-Borders' if short=='nes' else 'ZZSnes-Borders');new='SD032G:Emulator/'+console
 raw=(OUT/(short+'-Library.cfg')).read_bytes();lines=raw.decode('latin1').splitlines();assert lines[0]==('ZZNESUI2' if short=='nes' else 'ZZSNESUI2')
 rewritten=lines[:2];roms=[];art=[];keys={};names={};entries=[]
 def relocate(source,subdir,collection):
  if source.upper().startswith('PROGDIR:'):source=old+'/'+source[8:]
  name=source.rsplit('/',1)[-1].rsplit(':',1)[-1];assert name and name not in ('.','..')
  if subdir=='ROMs':
   name=SHORT_NAMES.get(name,name);assert len(name)<=20,name
  # Avoid collisions even on short-filename filesystems.
  key=(subdir,name[:30].casefold());assert key not in names or names[key]==source,(source,names.get(key));names[key]=source
  portable='PROGDIR:'+subdir+'/'+name;dest=new+'/'+subdir+'/'+name
  if not any(r['source']==source for r in collection):collection.append({'source':source,'destination':dest,'portable':portable})
  return portable
 for line in lines[2:]:
  fields=line.split();assert len(fields)==5
  original=bytes.fromhex(fields[2]).decode('latin1');portable=relocate(original,'ROMs',roms)
  fields[2]=portable.encode('latin1').hex()
  if fields[3]=='-':
   title=original.rsplit('/',1)[-1].rsplit(':',1)[-1]
   if title.lower().endswith(('.nes','.sfc','.smc')):title=title[:-4]
   fields[3]=title.replace('_',' ').encode('latin1').hex()
  if fields[4]!='-':fields[4]=relocate(bytes.fromhex(fields[4]).decode('latin1'),'Artwork',art).encode('latin1').hex()
  if short=='nes':
   key=f'{path_hash(original):08x}';assert key not in keys;keys[key]=f'{path_hash(portable):08x}'
  rewritten.append(' '.join(fields));entries.append({'old':original,'new':portable})
 rename=[]
 for item in json.loads((OUT/(short+'-States-dir.json')).read_text())['entries']:
  name=item['name']
  if short=='nes' and name[:8] in keys:
   assert name[8:10]=='-1' or name[8:10] in ['-2','-3','-4','-5'],name
   rename.append({'old':name,'new':keys[name[:8]]+name[8:]})
 assert len({r['new'] for r in rename})==len(rename)
 package=OUT/'build/transfer'/console;package.mkdir(parents=True,exist_ok=True)
 (package/'Library.cfg').write_bytes(('\n'.join(rewritten)+'\n').encode('latin1'))
 (package/'Library.bak').write_bytes((package/'Library.cfg').read_bytes())
 return {'old':old,'new':new,'original_config_md5':hashlib.md5(raw).hexdigest(),'roms':roms,'artwork':art,'entries':entries,'state_renames':rename,'states':json.loads((OUT/(short+'-States-dir.json')).read_text())['entries'],'snapshots':json.loads((OUT/(short+'-Snapshots-dir.json')).read_text())['entries']}
if __name__=='__main__':
 plan={n:prepare(n) for n in ['ZZNES','ZZSNES']};(OUT/'move-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
 for k,v in plan.items():print(k,len(v['roms']),'ROMs;',v['state_renames'])
