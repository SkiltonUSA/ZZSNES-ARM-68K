#!/usr/bin/env python3
"""Host-only blanking observations and bounded post-ACK delay comparisons.
Uses C's four slots and unchanged ARM image. Never deploys or launches.
"""
from pathlib import Path
import hashlib,json,shutil,subprocess
from amiga import PIN
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/pocketsnes-tearing/C'
OUT=ROOT/'.context/amiga/pocketsnes-scanout'
def replace(s,a,b):
 assert s.count(a)==1,(a,s.count(a))
 return s.replace(a,b)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 baseline=json.loads((BASE/'trial-build.json').read_text())
 for n,h in baseline['sha256'].items():assert sha(BASE/n)==h,n
 records={}
 for name,mode in [('C0',0),('E',1),('F',2)]:
  out=OUT/name;out.mkdir(parents=True,exist_ok=True)
  for p in BASE.iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.elf'):shutil.copy2(p,out/p.name)
  for n in ['scanout_gate.h','scanout_measure.h']:shutil.copy2(ROOT/'amiga/pocketsnes/library'/n,out/n)
  full=(out/'fullscreen.h').read_text();host=(out/'host.c').read_text()
  full=replace(full,'static ULONG trial_ack_blank,trial_ack_active;',f'''static ULONG trial_ack_blank,trial_ack_active;
#include "scanout_gate.h"
static struct sg_gate full_gate;
static ULONG gate_count,gate_total_us,gate_max_us,gate_timeouts;
#define SCANOUT_GATE_MODE {mode}u
static int full_blank(void){{return (*(volatile UWORD *)(full_board+0x4c)&0x20u)!=0;}}''')
  start=full.index('static int full_poll(void)');end=full.index('static int full_drain(void)',start)
  full=full[:start]+'''static int full_gate_error(void){
 full_pan(full_origin,0);full_slots.pending=-1;full_gate.phase=0;
 full_fault=1;full_unlock();puts("SCANOUT gate/ACK failed; pending cancelled");return 0;
}
static int full_poll(void){
 int status;
 if(full_fault)return 0;
 if(full_gate.phase){
  ULONG now=us();
  if(full_ack_count()!=(UWORD)(full_slots.ack+1))return full_gate_error();
  status=sg_poll(&full_gate,full_blank(),now);
  if(status<0){gate_timeouts++;return full_gate_error();}
  if(!status)return 1;
  {ULONG dt=now-full_gate.start;gate_count++;gate_total_us+=dt;if(dt>gate_max_us)gate_max_us=dt;}
  full_wait_us+=now-full_pending_us;full_unlock();return 1;
 }
 if(full_slots.pending<0)return 1;
 status=nf_ack(&full_slots,full_ack_count());
 if(status==1){
  int blank=full_blank();ULONG now=us();
  if(blank)trial_ack_blank++;else trial_ack_active++;
  fc_flip_sample();sg_begin(&full_gate,SCANOUT_GATE_MODE,blank,now);
  if(!full_gate.phase){full_wait_us+=now-full_pending_us;full_unlock();}
  return 1;
 }
 if(status<0 || (ULONG)(us()-full_pending_us)>100000u)return full_gate_error();
 return 1;
}
'''+full[end:]
  full=replace(full,'while(full_slots.pending>=0){','while(full_slots.pending>=0 || full_gate.phase){')
  full=replace(full,'if(!direct_locked || !full_attached || !nf_queue','if(full_fault || full_gate.phase || !direct_locked || !full_attached || !nf_queue')
  full=replace(full,'static int full_open(void){','#include "scanout_measure.h"\nstatic int full_open(void){')
  full=replace(full,'full_slots.pending=-1;full_unlock();','full_slots.pending=-1;full_gate.phase=0;full_unlock();')
  # Measurement after opening the screen, before emulator timing starts.
  host=replace(host,'if(!full_open())goto done;','if(!full_open() || !full_measure_refresh())goto done;')
  host=host.replace('TEARING_TRIAL variant=C ',f'TEARING_TRIAL variant={name} ')
  host=replace(host,'fc_profile_print();fc_joy_close();',f'''printf("SCANOUT_GATE variant={name} mode={mode} delayed=%u total_us=%u max_us=%u timeouts=%u phase=%u\\n",gate_count,gate_total_us,gate_max_us,gate_timeouts,full_gate.phase);
 fc_profile_print();fc_joy_close();''')
  (out/'fullscreen.h').write_text(full);(out/'host.c').write_text(host)
  rel=out.relative_to(ROOT)
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x2000000','ZZ_CONTROL=0x180000',f'ZZ_APP_NAME="ZZSnes-{name}"','ZZ_APP_VERSION="scanout-trial"','ZZ_MIN_STACK=65536','MB_ALLOCATION=0x2000000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
  subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+d for d in defs],'-I'+str(rel),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(rel/'host.c'),'-lamiga','-o',str(rel/f'ZZSnes-{name}')],cwd=ROOT,check=True)
  for n in ['pocket.elf','zz_payload.h']:assert sha(out/n)==sha(BASE/n)
  records[name]={'gate_mode':mode,'arm_unchanged_from_C':True,'sha256':{n:sha(out/n) for n in ['host.c','fullscreen.h','scanout_gate.h','scanout_measure.h','pocket.elf','zz_payload.h',f'ZZSnes-{name}']},'md5':hashlib.md5((out/f'ZZSnes-{name}').read_bytes()).hexdigest()}
  print('Built',name,flush=True)
 (OUT/'builds.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
