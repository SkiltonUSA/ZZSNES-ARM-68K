from pathlib import Path
import argparse,json,shutil,subprocess
r=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description='Build a private 600-frame NTSC stereo capture player; no deployment')
p.add_argument('build',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
source=a.build.resolve();out=a.output.resolve();stage=out/'player'
# Keep output under the repository cross-compiler mount.
out.relative_to(r)
shutil.copytree(source/'player',stage,dirs_exist_ok=True)
p=stage/'player_audio.h';s=p.read_text();needle='static volatile int audio_state_drain,audio_state_drained;';assert s.count(needle)==1
s=s.replace(needle,needle+'\n#define PCM_CAPACITY 1300000u\nstatic UBYTE *capture_pcm;static ULONG capture_size;')
needle=' audio_parent=FindTask(NULL);';assert s.count(needle)==1
s=s.replace(needle,' capture_pcm=AllocVec(PCM_CAPACITY,MEMF_PUBLIC);\n'+needle)
needle=' CopyMem((APTR)(shared+NES_AUDIO),audio_queue[index].data,n*4);';assert s.count(needle)==1
s=s.replace(needle,needle+'\n if(capture_pcm&&n*4<=PCM_CAPACITY-capture_size){CopyMem(audio_queue[index].data,capture_pcm+capture_size,n*4);capture_size+=n*4;}')
needle=' audio_opened=0;';assert needle in s
s=s.replace(needle,' if(capture_pcm){FILE*f=fopen("PROGDIR:snes-source.pcm","wb");if(f){ULONG written=fwrite(capture_pcm,1,capture_size,f);fclose(f);printf("PCM_CAPTURE bytes=%u written=%u\\n",capture_size,written);}FreeVec(capture_pcm);capture_pcm=NULL;}\n'+needle,1);p.write_text(s)
m=json.loads((r/'docs/current-build.json').read_text());src=r/m['snapshot_dir']/'source';rel=lambda p:str(p.relative_to(r))
cc=['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',str(r)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation']
defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x6000000','ZZ_CONTROL=0x180000','ZZ_APP_NAME="ZZSNES"','ZZ_APP_VERSION="0.7.11-dev PCM capture"','ZZ_MIN_STACK=65536','MB_ALLOCATION=0x6000000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN','CJ_BEGIN=0x3400000']
subprocess.run(cc+['-D'+d for d in defs]+['-I'+rel(stage)]+['-I'+rel(src/'amiga'/d) for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']]+[rel(src/'amiga/arm_debug/zz9000/launcher.c'),rel(stage/'host.c'),rel(stage/'zip/puff.c'),'-lamiga','-o',rel(out/'SNES')],cwd=r,check=True)
print('PASS private stereo capture',out,flush=True)
