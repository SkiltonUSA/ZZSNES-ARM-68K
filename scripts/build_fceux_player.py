#!/usr/bin/env python3
"""Build the FCEUX frontend candidate; never deploys or launches hardware."""
from pathlib import Path
import argparse,difflib,hashlib,json,re,subprocess,sys
import build_fceux_bench as bench
from build_zz9000_debug import unpack_elf
from amiga import PIN,container_command
ROOT=bench.ROOT;SRC=ROOT/'amiga/fceux_player';FRONT=ROOT/'amiga/mesen_nes'
OUT=ROOT/'.context/amiga/fceux-player'

def prepare_frontend(scheduling="baseline"):
 ui=scheduling=="release-ui"
 if ui:scheduling="release-profile"
 release=scheduling in ("release-profile","release-flip","release-noaudio")
 flip_paced=scheduling=="release-flip"
 noaudio=scheduling=="release-noaudio"
 pipeline=scheduling in ("pipeline","pipeline-fast")
 display_paced=scheduling=="display-paced"
 fast=release or scheduling in ("optimized-fast","pipeline-fast","display-paced")
 if release or scheduling in ("optimized","pipeline","optimized-fast","pipeline-fast","display-paced"):scheduling="deadline-cadence3"
 # Same display palette/emphasis approximation as the existing player.
 colors=[int(x,16) for x in re.findall(r'0x([0-9A-Fa-f]{8})',(FRONT/'palette.h').read_text())]
 full=[]
 for emphasis in range(8):
  for color in colors:
   rgb=[float((color>>shift)&255) for shift in (16,8,0)]
   for bit in range(3):
    if emphasis&(1<<bit):rgb=[v*(1.1 if ch==bit else 0.9) for ch,v in enumerate(rgb)]
   full.append(0xff000000|(int(min(255,rgb[0]))<<16)|(int(min(255,rgb[1]))<<8)|int(min(255,rgb[2])))
 (OUT/'player_palette.h').write_text('static const uint32_t nes_player_palette[512]={'+','.join(hex(v) for v in full)+'};\n')
 patch=''
 for name in ['host.c','player_frontend.h','player_audio.h','fullscreen.h','wire.h']:
  original=(FRONT/name).read_text();s=original
  if name=='host.c':
   s=s.replace('static int core_only;', 'static int core_only;\nstatic int player_audit;\nstatic int verify_frame(unsigned n){return n==1 || (player_audit && n%60==0);}')
   s=s.replace('int verify=frame==1 || frame%60==0;', 'int verify=verify_frame(frame);')
   s=s.replace('request(NES_FRAME,buttons)', 'request(NES_FRAME,buttons | (verify_frame(frames+1)?256u:0u))')
   start=s.index('int nes_arguments(');end=s.index('static void paint(',start)
   s=s[:start]+'''int nes_arguments(int argc,char **argv){
 if(argc<=1)return argc;
 if(argc==2){rom_path=argv[1];return 1;}
 if(argc<4||argc>6){puts("Usage: ZZFceux [ROM.nes] or <nonce> RUN <ROM.nes> [frame_limit] [VERIFY]");return -1;}
 rom_path=argv[3];
 if(argc>=5){char *end;errno=0;frame_limit=strtoul(argv[4],&end,10);if(errno||*end||!frame_limit||frame_limit>36000)return -1;}
 if(argc==6){if(strcmp(argv[5],"VERIFY"))return -1;player_audit=1;}
 return 3;
}
'''+s[end:]
   s=s.replace('mapper 0/1/2/3/4/7/90','mapper 0/90, CHR ROM, no battery/trainer/four-screen')
   s=s.replace('Only NTSC iNES NROM, 16/32 KiB PRG, 0/8 KiB CHR; no trainer/battery supported','FCEUX core rejected cartridge: NTSC mapper 0/90 CHR ROM only')
   s=s.replace('PLAYER Mesen CPU/PPU/APU, NTSC; P pause, M mute, Q/Esc quit; no timing hacks','PLAYER FCEUX CPU/old PPU/APU, NTSC 32kHz quality0; P pause, M mute, Q/Esc quit; no frameskip')
  if name=='host.c':
   s=s.replace('static int player_audit;', 'static int player_audit;\nstatic ULONG present_pre_us,present_request_us,present_post_us;')
   s=s.replace('if(!pause_us(1000))return 0;', '/* PRESENT holds a bitmap lock: bounded polling avoids yielding here. */\n  if(op!=NES_PRESENT && !pause_us(1000))return 0;')
   s=s.replace('static uint32_t sequence;', 'static uint32_t sequence;\nstatic uint64_t frame_reply_ticks,present_reply_ticks;')
   s=s.replace('   return 1;}','   if(op==NES_FRAME)frame_reply_ticks+=ad_get(reply,48);\n   if(op==NES_PRESENT)present_reply_ticks+=ad_get(reply,48);\n   return 1;}')
   s=s.replace('static int present(ULONG raw_hash,ULONG frame){', 'static int present(ULONG raw_hash,ULONG frame){\n ULONG profile_start=us(),profile_request,profile_after;')
   s=s.replace('if(!request(NES_PRESENT,addr))return 0;', 'profile_request=us();present_pre_us+=profile_request-profile_start;\n if(!request(NES_PRESENT,addr))return 0;')
   s=s.replace('if(ad_get(shared+MB_REPLY,24)!=raw_hash)return 0;', 'profile_after=us();present_request_us+=profile_after-profile_request;\n if(ad_get(shared+MB_REPLY,24)!=raw_hash)return 0;')
   s=s.replace(' direct_ready=1;\n return 1;', ' present_post_us+=us()-profile_after;\n direct_ready=1;\n return 1;')
   s=s.replace(' printf("PROFILE frames=', ' printf("PRESENT_PROFILE pre_us=%u request_us=%u post_us=%u\\n",present_pre_us,present_request_us,present_post_us);\n printf("PROFILE frames=')
  if name=='host.c':
   s=s.replace(' printf("PRESENT_PROFILE', ' printf("ARM_REQUEST_PROFILE frame_us=%u present_us=%u\\n",rate?(ULONG)(frame_reply_ticks*1000000u/rate):0,rate?(ULONG)(present_reply_ticks*1000000u/rate):0);\n printf("PRESENT_PROFILE')
  if scheduling!='baseline':
   if name=='host.c':
    s=s.replace('#include "fullscreen.h"', '#include "wait_profile.h"\n#include "fullscreen.h"')
    s=s.replace('if(op!=NES_PRESENT && !pause_us(1000))return 0;',
      'if(op!=NES_PRESENT && '+('op!=NES_FRAME && ' if scheduling.startswith('deadline') else '!direct_locked && ' if scheduling=='locked-poll' else '')+'!profile_pause(1,1000))return 0;')
    s=s.replace('!pause_us(target-elapsed)', '!profile_pause(4,target-elapsed)')
    if scheduling.startswith('deadline'):
     s=s.replace('if(elapsed<target && !profile_pause(4,target-elapsed))goto done;',
       'while(elapsed<target){\n'
       '     if(!full_poll())goto done;\n'
       '     if(!direct_locked){events();if(quit)break;}\n'
       '     if(!direct_locked && target-elapsed>4000u && !profile_pause(4,1000))goto done;\n'
       '     elapsed=us()-frame_start;\n'
       '    }')
    if scheduling in ('deadline-cadence','deadline-cadence3'):
     s=s.replace('static int player_audit;', 'static ULONG next_frame_us;\nstatic int player_audit;')
     s=s.replace('if(player_paused){\n', 'if(player_paused){\n   next_frame_us=0;\n')
     s=s.replace('frame_start=us();before=frame_start;',
       'frame_start=us();next_frame_us=fc_next_deadline(frame_start,next_frame_us);before=frame_start;')
     s=s.replace('target=16639u;', 'target=(LONG)(next_frame_us-frame_start)>0?next_frame_us-frame_start:0;')
    if scheduling=='deadline-noaudio':s=s.replace('if(!core_only)player_audio_open();', 'if(!core_only && 0)player_audio_open();')
    s=s.replace(' printf("PROFILE frames=', ' print_wait_profile();\n printf("PROFILE frames=')
   if name=='fullscreen.h':
    s=s.replace('!pause_us(1000)', '!profile_pause(3,1000)')
    if scheduling.startswith('deadline'):
     s=s.replace('if(!profile_pause(3,1000)){full_fault=1;return 0;}', '/* Bounded full_poll deadline; do not sleep under bitmap lock. */')
    elif scheduling=='locked-poll':
     s=s.replace('if(!profile_pause(3,1000))', 'if(full_slots.pending>=0 && !profile_pause(3,1000))')
   if name=='player_audio.h':
    s=s.replace('!pause_us(1000)', '!profile_pause(5,1000)')
    if scheduling in ('deadline-audio','deadline-overlap','deadline-copy','deadline-batch','deadline-coalesce','deadline-cadence','deadline-cadence3'):s=s.replace('NP_Priority,0,', 'NP_Priority,5,')
    if scheduling in ('deadline-copy','deadline-batch','deadline-coalesce','deadline-cadence','deadline-cadence3'):
     start=s.index(' req[1]->ahir_Std.io_Device=');end=s.index(' audio_opened=1;',start)
     s=s[:start]+' CopyMem(req[0],req[1],sizeof(*req[1])); /* AHI documented clone after OpenDevice. */\n'+s[end:]
    s=s.replace('SendIO(', 'profile_send_io(').replace('WaitIO(', 'profile_wait_io(')
    if scheduling in ('deadline-overlap','deadline-copy','deadline-batch','deadline-coalesce','deadline-cadence','deadline-cadence3'):
     s=s.replace('static int player_audio_submit(void){return !audio_error;}', 'static int player_audio_submit(void){audio_wake();return !audio_error;}')
     s=s.replace('Permit();audio_samples+=n;audio_wake();return 1;', 'Permit();audio_samples+=n;return 1;')
    if scheduling in ('deadline-coalesce','deadline-cadence','deadline-cadence3'):
     s=s.replace('data[i]=AllocVec(2048,', 'data[i]=AllocVec(4096,')
     s=s.replace('available=audio_write-audio_read;Permit();', 'available=audio_write-audio_read;Permit();\n  if(!available)audio_force=0;',1)
     s=s.replace('if(primed&&available&&', 'if(primed&&available&&(available>=2||audio_stop==2||audio_force)&&')
     s=s.replace('ULONG available,n,index;', 'ULONG available,n,index,blocks,k;')
     begin=s.index('   index=audio_read%AUDIO_SLOTS;');end=s.index('   req[slot]->ahir_Std.io_Command',begin)
     s=s[:begin]+'''   /* Coalesce two immutable NES blocks without dropping/resampling PCM.
    * A short final block is flushed by stop/pause. Never exceed 4096 bytes. */
   blocks=available>=2?2:1;n=0;
   for(k=0;k<blocks;k++){
    ULONG count;
    index=(audio_read+k)%AUDIO_SLOTS;count=audio_queue[index].count;
    if(count>1024){audio_error=1;break;}
    CopyMem(audio_queue[index].data,data[slot]+n*2,count*2);n+=count;
   }
   if(audio_error)break;
   Forbid();audio_read+=blocks;Permit();
'''+s[end:]
    if scheduling=='deadline-cadence3':
     s=s.replace('AllocVec(4096,', 'AllocVec(6144,')
     s=s.replace('available>=2||', 'available>=3||')
     s=s.replace('blocks=available>=2?2:1;', 'blocks=available>=3?3:available;')
     s=s.replace('two immutable', 'three immutable').replace('4096 bytes', '6144 bytes')
    if scheduling=='deadline-batch':
     s=s.replace('int active[2]={0,0},opened=0,slot=0,primed=0;', 'int active[2]={0,0},opened=0,slot=0,primed=0,submit_forbidden=0;')
     s=s.replace('submitted_count[slot]=n;profile_send_io', 'if(!submit_forbidden){Forbid();submit_forbidden=1;}\n   submitted_count[slot]=n;profile_send_io')
     s=s.replace('  Wait(mask|', '  if(submit_forbidden){submit_forbidden=0;Permit();}\n  Wait(mask|')
     s=s.replace('finish:\n for', 'finish:\n if(submit_forbidden){submit_forbidden=0;Permit();}\n for')
  if display_paced and name=='host.c':
   s=s.replace(' if(!full_queue())return 0;',
     ' /* Pace completed frames before queuing; begin the next ARM frame immediately. */\n'
     ' while((LONG)(next_frame_us-us())>0){if(!full_poll())return 0;}\n'
     ' if(!full_queue())return 0;')
   start=s.index('  /* Bound the emulation rate without busy spinning.')
   end=s.index('  #ifndef NES_PLAYER',start)
   s=s[:start]+'  /* Output cadence is enforced before full_queue, overlapping next emulation with flip. */\n'+s[end:]
  if release:
   from fceux_release import frontend as release_frontend
   s=release_frontend(name,s,flip_paced,noaudio)
  if ui:
   from fceux_ui import frontend as ui_frontend
   s=ui_frontend(name,s)
  if pipeline:
   from fceux_pipeline import frontend
   s=frontend(name,s)
  s=s.replace('ZZNes','ZZFceux').replace('ZZNES','ZZFCEUX')
  (OUT/name).write_text(s)
  patch+=''.join(difflib.unified_diff(original.splitlines(True),s.splitlines(True),'mesen_nes/'+name,'fceux-player/'+name))
 original=(FRONT/'worker.cpp').read_text();worker=original.replace('#include "core.h"','#include "nes_api.h"').replace('if(!error)publish_frame();','if(!error && (arg&256u))publish_frame();').replace('if(op==NES_FRAME && !error)for','if(op==NES_FRAME && !error && (arg&256u))for')
 worker=worker.replace('barrier();uint32_t op=', 'barrier();uint32_t request_start=(uint32_t)ticks();uint32_t op=')
 worker=worker.replace('ad_put(reply,32,hash);', 'ad_put(reply,48,(uint32_t)ticks()-request_start);ad_put(reply,32,hash);')
 if pipeline:
  from fceux_pipeline import worker as pipeline_worker
  worker=pipeline_worker(worker)
 if ui:
  from fceux_ui import worker as ui_worker
  worker=ui_worker(worker)
 (OUT/'worker.cpp').write_text(worker)
 direct_original=(FRONT/'direct.cpp').read_text()
 direct=direct_original.replace('#include "core.h"','#include "nes_api.h"')
 if fast:direct=direct.replace('#include "direct_pixels.h"','#include "fast_pixels.h"')
 if ui:
  from fceux_ui import direct as ui_direct
  direct=ui_direct(direct)
 (OUT/'direct.cpp').write_text(direct)
 (OUT/'fast_pixels.h').write_bytes((SRC/'fast_pixels.h').read_bytes())
 patch+=''.join(difflib.unified_diff(original.splitlines(True),worker.splitlines(True),'mesen_nes/worker.cpp','fceux-player/worker.cpp'))
 patch+=''.join(difflib.unified_diff(direct_original.splitlines(True),direct.splitlines(True),'mesen_nes/direct.cpp','fceux-player/direct.cpp'))
 (OUT/'frontend.patch').write_text(patch)
 (OUT/'nes_api.h').write_bytes((FRONT/'core.h').read_bytes())
 (OUT/'cartridge.h').write_bytes((SRC/'cartridge.h').read_bytes())
 (OUT/'pace.h').write_bytes((SRC/'pace.h').read_bytes())
 for header in ("frame_profile.h","joystick.h"):(OUT/header).write_bytes((SRC/header).read_bytes())
 if ui:
  from fceux_ui import frontend as ui_frontend
  (OUT/'joystick.h').write_text(ui_frontend('joystick.h',(SRC/'joystick.h').read_text()))
  for header in ('view.h','ui_model.h','session.h','state_host.h','state_core.h'):(OUT/header).write_bytes((SRC/header).read_bytes())
  (OUT/'direct_plan.h').write_text((FRONT/'direct_plan.h').read_text().replace('256*bpp','320*bpp'))
 else:
  (OUT/'direct_plan.h').unlink(missing_ok=True)
 (OUT/'wait_profile.h').write_bytes((SRC/'wait_profile.h').read_bytes())
 (OUT/'demo.h').write_text('/* ROM selection is required; no built-in game. */\nstatic const uint8_t demo_rom[1]={0};\n')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--container-command');p.add_argument('--scheduling',choices=['release-ui','release-profile','release-flip','release-noaudio','baseline','optimized','optimized-fast','display-paced','pipeline','pipeline-fast','profile','locked-poll','deadline','deadline-audio','deadline-overlap','deadline-copy','deadline-batch','deadline-coalesce','deadline-cadence','deadline-cadence3','deadline-noaudio'],default='baseline');a=p.parse_args()
 bench.OUT=OUT;sources=bench.prepare();prepare_frontend(a.scheduling)
 ui=a.scheduling=='release-ui'
 if ui:
  from fceux_states import prepare as prepare_states
  prepare_states(OUT)
 # State support is opt-in; the bench and previous accepted modes retain their ABI.
 state_flags=['-DFC_STATES'] if ui else []
 if not ui:bench.build_native(sources)
 flags=state_flags+['-std=c++14','-DDINGUX','-DNES_PLAYER','-O3','-g','-fwrapv','-fno-exceptions','-fno-rtti','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage','-I'+str(bench.SRC),'-I'+str(OUT)]
 for name,extra in [('player-native',[]),('player-sanitize',['-fsanitize=address,undefined','-fno-sanitize=array-bounds,pointer-overflow','-fno-sanitize-recover=all'])]:
  subprocess.run(['clang++',*flags,*extra,*sources,bench.SRC/'core.cpp',SRC/'adapter.cpp',SRC/'native.cpp','-o',OUT/name],cwd=ROOT,check=True)
 direct_source=OUT/'direct.cpp' if a.scheduling in ('release-ui','release-profile','release-flip','release-noaudio','optimized-fast','pipeline-fast','display-paced') else FRONT/'direct.cpp'
 flags=bench.arm_objects(sources,worker=OUT/'worker.cpp',extras=[SRC/'adapter.cpp',direct_source],defines=state_flags+['-DNES_PLAYER','-DNES_DIRECT','-I'+str(FRONT)])
 sys.path.insert(0,str(bench.SRC));from elf import unpack
 image,rel,entry=unpack((OUT/'fceux.elf').read_bytes());ld='/opt/homebrew/opt/lld/bin/ld.lld'
 subprocess.run(['clang',*flags,'-c',ROOT/'amiga/arm_debug/zz9000/mapping_probe.S','-o',OUT/'mapping.o'],check=True)
 subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T',ROOT/'amiga/arm_debug/zz9000/payload.ld',OUT/'mapping.o','-o',OUT/'mapping.elf'],check=True)
 mapping,mrel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
 def array(name,data):return 'static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};\n'
 (OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
 command=json.loads(a.container_command) if a.container_command else container_command();relative=OUT.relative_to(ROOT)
 subprocess.run([*command,'run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],
 'm68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror',
 '-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000',
 '-DZZ_APP_NAME="ZZFceux"',('-DZZ_APP_VERSION="0.3.0-dev"' if ui else '-DZZ_APP_VERSION="0.2.0-rc1"' if a.scheduling.startswith('release-') else '-DZZ_APP_VERSION="0.1-dev"'),'-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x400000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE',
 '-Wno-unused-function','-Wno-misleading-indentation','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN','-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000',
 'amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/'ZZFceux')],cwd=ROOT,check=True)
 if ui:
  subprocess.run([*command,'run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],
   'm68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',
   'amiga/fceux_player/launcher.c','-lamiga','-o',str(relative/'ZZNesLibrary')],cwd=ROOT,check=True)
 binary=OUT/'ZZFceux';assert binary.read_bytes()[:4]==b'\0\0\x03\xf3'
 recorded=[*SRC.rglob('*'),*bench.SRC.rglob('*'),*FRONT.glob('*.h'),FRONT/'worker.cpp',FRONT/'direct.cpp',FRONT/'host.c',
 ROOT/'scripts/build_fceux_player.py',ROOT/'scripts/fceux_pipeline.py',ROOT/'scripts/fceux_release.py',ROOT/'scripts/fceux_ui.py',ROOT/'scripts/fceux_states.py',ROOT/'scripts/build_fceux_bench.py',*ROOT.joinpath('amiga/mesen_bench').glob('mmu*'),ROOT/'amiga/mesen_bench/runtime.c',
 ROOT/'amiga/arm_debug/zz9000/launcher.c',ROOT/'amiga/arm_debug/zz9000/layout.h',ROOT/'amiga/arm_debug/zz9000/mapping_probe.S',ROOT/'amiga/arm_debug/protocol.h']
 record={'scheduling':a.scheduling,'upstream':bench.COMMIT,'scope':'FCEUX NTSC mapper0/90 player; old PPU quality0 32kHz mono; every frame displayed; XACP XX19c fullscreen; AHI0 DOS Process',
 'image_bytes':len(image),'relocations':len(rel),'flags':[x for x in flags if not x.startswith('-I')],'compiler':subprocess.check_output(['clang','--version'],text=True),
 'sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in recorded if p.is_file() and '__pycache__' not in p.parts},
 'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [binary,OUT/'fceux.elf',OUT/'integration.patch',OUT/'frontend.patch']},'executable_md5':hashlib.md5(binary.read_bytes()).hexdigest()}
 if ui:record['outputs']['ZZNesLibrary']=hashlib.sha256((OUT/'ZZNesLibrary').read_bytes()).hexdigest()
 (OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n');print(f'Built {binary}: {binary.stat().st_size} bytes; ARM image {len(image)} bytes')
if __name__=='__main__':main()
