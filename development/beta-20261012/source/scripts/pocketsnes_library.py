"""SNES library preparation; called only for the opt-in standalone UI build."""
from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[1];LIB=ROOT/'amiga/pocketsnes/library'
def test_rom(out):
 import build_mesen_snes_system as fixture
 rom=bytearray(fixture.fixture())
 rom[0x7fd5:0x7fda]=bytes([0x20,0,5,0,0])
 rom[0x7fdc:0x7fe0]=bytes([0xfe,0xff,1,0])
 (out/"Library-System-Test.sfc").write_bytes(rom)

def core_patch(out):
 test_rom(out)
 # Preserve intended two's-complement arithmetic without signed-shift UB.
 f=out/'src/soundux.c';f.write_text(f.read_text().replace('ch->envx << ENVX_SHIFT','(int32)((uint32)ch->envx << ENVX_SHIFT)'))
 f=out/'src/gfx.c';s=f.read_text()
 for expr in ['LineData [Line].BG[0].HOffset','LineData [Line].BG[0].VOffset','l->CentreX','l->CentreY']:
  s=s.replace('((int32) '+expr+' << M7) >> M7','(((int32)((uint32)('+expr+') & 0x1fffu) ^ 0x1000) - 0x1000)')
 f.write_text(s)
 f=out/'src/soundux.c';f.write_text(f.read_text()+'\n'+(LIB/'mixer.inc').read_text())
 f=out/'libretro/libretro.c';f.write_text(f.read_text()+'''\nuint32_t ps_audio_phase(int load,uint32_t value){uint32_t bits;if(load)memcpy(&audio_samples_accumulator,&value,4);memcpy(&bits,&audio_samples_accumulator,4);return bits;}\n''')
 # Snapshot padding must be deterministic and cannot contain stale allocator data.
 f=out/'src/snapshot.c';s=f.read_text().replace('block = (uint8*)malloc(len);','block = (uint8*)calloc(1,len);');f.write_text(s)
 f=out/'src/snapshot.c';s=f.read_text().replace('*ptr++ << 24','(uint32)*ptr++ << 24').replace('(int64) * ptr++ <<','(uint64_t) * ptr++ <<');f.write_text(s)
def frontend(out):
 import fceux_ui as ui
 for name in ['border_art.h','border_assets.h']:
  shutil.copy2(ROOT/'amiga/borders'/name,out/name)
 for name in ['ui_model.h','library_mui.h','library_preview.h','library_saves.h','library_controller.h','session.h','state_host.h','snapshot_host.h','snapshot_format.h']:
  shutil.copy2(ROOT/'amiga/fceux_player'/name,out/name)
 for name in ['view.h','state_format.h','cartridge.h','thumbnail.h','launcher.c','player.c','mixer.inc','region.h']:
  shutil.copy2(LIB/name,out/name)
 f=out/'ui_model.h';f.write_text(f.read_text().replace('"ZZNESUI"','"ZZSNESUI"').replace('m->mask=1;','m->theme=2;'))
 f=out/'session.h';f.write_text(f.read_text().replace('fc_view=16','fc_view=4').replace('ZZSESSION1','ZZSNESSESSION2'))
 f=out/'launcher.c';f.write_text(f.read_text().replace('ZZSESSION1','ZZSNESSESSION2'))
 f=out/'wire.h';s=f.read_text().replace('0x190000u','0x200000u').replace('524288u','0x400000u').replace('0x171000u','0x600000u').replace('32768u','0x100000u');f.write_text(s)
 f=out/'player_audio.h';f.write_text(ui.frontend('player_audio.h',f.read_text()))
 f=out/'joystick.h';f.write_text(ui.frontend('joystick.h',f.read_text()))
 f=out/'worker.cpp';s=ui.worker(f.read_text());s=s.replace('extern unsigned fc_state_store','extern "C" unsigned fc_state_store').replace('extern int fc_state_load','extern "C" int fc_state_load');s=s.replace('result=fc_state_load((const uint8_t *)base+NES_SAVE,arg);','result=fc_state_load((const uint8_t *)base+NES_SAVE,arg);if(result)publish_frame();');f.write_text(s)
 f=out/'direct_plan.h';f.write_text((ROOT/'amiga/mesen_nes/direct_plan.h').read_text().replace('256*bpp','320*bpp'))
 f=out/'direct.cpp';s=f.read_text().replace('#include "pixels.h"','#include "fast_pixels.h"');s=ui.direct(s);s=s.replace('fv_row(row_words,src+y*256,y,opt,format)','fv_row(row_words,src,y,opt,format)');f.write_text(s)
 f=out/'host.c';s=f.read_text()
 current='{unsigned pixel=((unsigned)indices[2*(y*256+x)]<<8)|indices[2*(y*256+x)+1];c=pocket_rgb(pixel);}'
 original='{unsigned index=((unsigned)indices[2*(y*256+x)]<<8)|indices[2*(y*256+x)+1];if(index>=512){bad++;continue;}c=nes_player_palette[index];}'
 assert current in s;s=s.replace(current,original);s=ui.frontend('host.c',s)
 start=s.index('{int sx=fv_source(x,fc_view);');end=s.index('\n#else',start)
 s=s[:start]+'{int sx=fv_source(x,fc_view),sy=fv_y(y,fc_view);if(sx<0||sy<0)c=fv_border(x,y,fc_view);else {unsigned index=((unsigned)indices[2*(sy*256+sx)]<<8)|indices[2*(sy*256+sx)+1];c=pocket_rgb(index);}}'+s[end:]
 s=s.replace('rom=AllocVec(NES_ROM_LIMIT+1,','rom=AllocVec(NES_ROM_LIMIT+513,').replace('fread(rom,1,NES_ROM_LIMIT+1,f)','fread(rom,1,NES_ROM_LIMIT+513,f)')
 s=s.replace('if(size>NES_ROM_LIMIT)', '{unsigned skip=snes_skip(size);if(skip){memmove(rom,rom+skip,size-skip);size-=skip;}}\n if(size>NES_ROM_LIMIT)')
 s=s.replace('This preview requires the validated 512 KiB Super Mario World (USA) cartridge','Supported: NTSC LoROM/HiROM up to 4 MiB, no enhancement chips')
 s=s.replace('#include "state_host.h"','static int present(ULONG,ULONG);\n#include "state_host.h"')
 f.write_text(s)
 f=out/'state_host.h';s=f.read_text();s='#include "state_format.h"\n'+s;s=s.replace('n>=32&&n<=NES_SAVE_LIMIT','n>=PS_PAYLOAD_OFFSET&&n<=NES_SAVE_LIMIT&&ps_valid(p,n)')
 s=s.replace('ok=ad_get(shared+MB_REPLY,52);printf','ok=ad_get(shared+MB_REPLY,52);if(ok&&direct_bitmap){if(!present(ad_get(shared+MB_REPLY,24),0)||!full_drain()){FreeVec(p);return -1;}}printf')
 s=s.replace('ZZNes Library','ZZSnes Library').replace('32 KiB battery area (supported ROMs have none)','dedicated 1 MiB state area');f.write_text(s)
 # Native ABI and source-bound state build; no default binary changes.
 f=out/'payload.ld';f.write_text((ROOT/'amiga/pocketsnes/payload.ld').read_text().replace('0x220000','0x700000').replace('0x17d0000','0x1fd0000'))
 f=out/'elf.py';f.write_text((ROOT/'amiga/pocketsnes/elf.py').read_text().replace('0x220000','0x700000').replace('0x15b0000','0x18d0000'))
 from pocketsnes_regions import frontend as region_frontend
 region_frontend(out)

def manifest(out):
 import hashlib,json
 from build_pocketsnes import PIN
 sources=list((ROOT/'amiga/borders').rglob('*'))+list(LIB.glob('*'))+[ROOT/'scripts'/n for n in ['pocketsnes_library.py','build_pocketsnes_library.py','build_pocketsnes_player.py','build_pocketsnes_arm.py','build_pocketsnes.py','fceux_ui.py','fceux_states.py','game_snapshots.py']]
 sources += [ROOT/'tests/amiga/pocketsnes'/n for n in ['check_library_arm.py','check_library_host.py','library_native.c','library_view.c']]
 sources += [ROOT/'amiga/fceux_player'/n for n in ['ui_model.h','library_mui.h','library_preview.h','library_saves.h','library_controller.h','session.h','snapshot_format.h','snapshot_host.h']]
 record={'version':'0.2.0-dev','launcher_version':'0.3.0-ui-preview','upstream_commit':PIN,'scope':'MUI ROM library, five ROM-specific thumbnail save slots, ARM direct console bezel presentation, stereo AHI unit0','allocation_bytes':0x2000000,'state_version':2,'commercial_roms_included':False,'files':{},'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources if p.is_file()}}
 for name in ['ZZSnesLibrary','ZZSnes-Pocket','pocket.elf','Library-System-Test.sfc']:
  p=out/name;data=p.read_bytes();record['files'][name]={'bytes':len(data),'md5':hashlib.md5(data).hexdigest(),'sha256':hashlib.sha256(data).hexdigest()}
 (out/'library-build.json').write_text(json.dumps(record,indent=2)+'\n')
