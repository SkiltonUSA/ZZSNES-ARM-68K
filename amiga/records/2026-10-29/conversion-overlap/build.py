import json,subprocess,hashlib,shutil,sys
from pathlib import Path
from prepare import prepare
root=Path.cwd();work=root/'.context/amiga/conversion-1029'
for name,build,block,control,begin,version in [('ZZNes-ARM-68K','jit-switch32-build',0x3000000,0x80000,0x600000,'0.8.2-dev'),('ZZSNES-ARM-68K','jit-dispatch32-build',0x6000000,0x180000,0x3400000,'0.7.2-dev')]:
 repo=root/'.context'/name;m=json.loads((repo/'docs/current-build.json').read_text());source=repo/m['snapshot_dir']/'source';stage=repo/'.context'/build/'player';tag='snes' if 'SNES' in name else 'nes'
 ref=root/'.context/amiga/ack-poll-1029'/(tag+'-plain-ref')
 for required in ('mmu.h','mmu_plan.h','zz_payload.h','frame_profile.h'):assert (stage/required).is_file()
 for variant in (('verify',) if '--verify' in sys.argv else ('ref','overlap')):
  out=work/(tag+'-'+variant);out.mkdir(exist_ok=True)
  for file in ('host.c','fullscreen.h'):shutil.copy2(ref/file,out/file)
  shutil.copy2(stage/'player_audio.h',out/'player_audio.h')
  # Quoted include from a stage-local header otherwise bypasses our new audio.
  shutil.copy2(stage/'player_frontend.h',out/'player_frontend.h')
  for file in ('flip_plan.h','scanout_gate.h'):
   if (stage/file).exists():shutil.copy2(stage/file,out/file)
  if variant!='ref':prepare(out)
  if variant=='verify':
   f=out/'fullscreen.h';t=f.read_text().replace('static int full_rendering;','static int full_rendering;static ULONG co_pending_starts,co_acks;').replace(' full_rendering=1;',' if(full_slots.pending>=0)co_pending_starts++;\n full_rendering=1;').replace('fc_flip_sample();','if(full_rendering)co_acks++;fc_flip_sample();');f.write_text(t)
   h=out/'host.c';h.write_text(h.read_text().replace('player_audit=0;','player_audit=1;').replace('SCHEDULE fixed=1 audit=0','SCHEDULE fixed=1 audit=1').replace(' print_wait_profile();',' printf("CONVERSION pending_starts=%u held_acks=%u\\n",co_pending_starts,co_acks);\n print_wait_profile();'))
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+hex(block),'ZZ_CONTROL='+hex(control),'ZZ_APP_NAME="'+m['console']+'"','ZZ_APP_VERSION="'+version+'"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+hex(block)+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN','CJ_BEGIN='+hex(begin)]
  rel=lambda p:str(p.relative_to(root))
  cmd=['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',str(root)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+d for d in defs],'-I'+rel(out),'-I'+rel(stage),*['-I'+rel(source/'amiga'/d) for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']],rel(source/'amiga/arm_debug/zz9000/launcher.c'),rel(out/'host.c'),rel(stage/'zip/puff.c'),'-lamiga','-o',rel(out/'Player')]
  subprocess.run(cmd,check=True)
  (out/'build.json').write_text(json.dumps({'version':version,'variant':variant,'ack_profile':False,'fixed_presentation':True,'audit':variant=='verify','conversion_profile':variant=='verify','capture':False,'warmup':60,'hashes':{rel(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (out/'Player',out/'host.c',stage/'mmu.h',stage/'mmu_plan.h',stage/'zz_payload.h',out/'player_audio.h',out/'fullscreen.h')}},indent=2)+'\n')
  if variant!='ref':subprocess.run(['python3',str(work/'test_host.py'),str(repo),str(out)],check=True)
  print('PASS',tag,variant,flush=True)
