import json,subprocess,hashlib,shutil,sys
from pathlib import Path

root=Path.cwd();work=root/'.context/amiga/ahi-lock-1029'
for name,build,block,control,begin,version in [('ZZNes-ARM-68K','jit-switch32-build',0x3000000,0x80000,0x600000,'0.8.2-dev'),('ZZSNES-ARM-68K','jit-dispatch32-build',0x6000000,0x180000,0x3400000,'0.7.2-dev')]:
 if '--snes-only' in sys.argv and name!='ZZSNES-ARM-68K':continue
 repo=root/'.context'/name;m=json.loads((repo/'docs/current-build.json').read_text());source=repo/m['snapshot_dir']/'source';stage=repo/'.context'/build/'player';tag='snes' if 'SNES' in name else 'nes'
 ref=root/'.context/amiga/ack-poll-1029'/(tag+'-plain-ref')
 for required in ('mmu.h','mmu_plan.h','zz_payload.h','frame_profile.h'):assert (stage/required).is_file()
 for variant in ('profile',):
  out=work/(tag+'-'+variant);out.mkdir(exist_ok=True)
  for file in ('host.c','fullscreen.h'):shutil.copy2(ref/file,out/file)
  for file in ('player_audio.h','player_frontend.h','wait_profile.h'):
   shutil.copy2(stage/file,out/file)
  for file in ('profile.h','cost.h'):shutil.copy2(work/file,out/file)
  h=out/'host.c';t=h.read_text().replace('#include "wait_profile.h"','#include "profile.h"\n#include "wait_profile.h"')
  t=t.replace('  fc_profile_frame=frames;','  hc_active=frames>=60;fc_profile_frame=frames;').replace('done:\n','done:\n hc_active=0;\n').replace(' print_wait_profile();',' hc_print();\n print_wait_profile();');h.write_text(t)
  p=out/'player_audio.h';a=p.read_text();needle='CopyMem(audio_queue[index].data,data[slot]+n*'+('4,count*4);' if tag=='snes' else '2,count*2);');assert a.count(needle)==1;p.write_text(a.replace(needle,needle.replace('CopyMem','hc_pcm_copy')))
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+hex(block),'ZZ_CONTROL='+hex(control),'ZZ_APP_NAME="'+m['console']+'"','ZZ_APP_VERSION="'+version+'"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+hex(block)+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN','CJ_BEGIN='+hex(begin)]
  rel=lambda p:str(p.relative_to(root))
  cmd=['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',str(root)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+d for d in defs],'-I'+rel(out),'-I'+rel(stage),*['-I'+rel(source/'amiga'/d) for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']],rel(source/'amiga/arm_debug/zz9000/launcher.c'),rel(out/'host.c'),rel(stage/'zip/puff.c'),'-lamiga','-o',rel(out/'Player')]
  subprocess.run(cmd,check=True)
  (out/'build.json').write_text(json.dumps({'version':version,'variant':variant,'ack_profile':False,'fixed_presentation':True,'audit':variant=='verify','conversion_profile':variant=='verify','capture':False,'warmup':60,'hashes':{rel(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (out/'Player',out/'host.c',stage/'mmu.h',stage/'mmu_plan.h',stage/'zz_payload.h',out/'player_audio.h',out/'fullscreen.h')}},indent=2)+'\n')
  print('PASS',tag,variant,flush=True)
