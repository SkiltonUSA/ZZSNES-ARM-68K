from pathlib import Path
import shutil,subprocess,json,hashlib,zipfile,argparse
parser=argparse.ArgumentParser(description="Native-only cartridge survey; never deploys or changes the installed player.")
parser.add_argument("roms",nargs="+",type=Path)
parser.add_argument("--allow-experimental-chips",action="store_true")
args=parser.parse_args()
ROOT=Path(__file__).resolve().parents[1];O=ROOT/'.context/cartridge-survey';O.mkdir(parents=True,exist_ok=True);r=ROOT;snap=r/'development/beta-20261015';core=O/'native-core'
if not core.exists():shutil.copytree(snap/'source/core',core)
inc=O/'include';shutil.copytree(snap/'build-inputs/player',inc,dirs_exist_ok=True)
p=inc/'cartridge.h';s=p.read_text()
if args.allow_experimental_chips:s=s.replace('(map==0x20+i||map==0x30+i||contra)&&type<=2','(map==0x20+i||map==0x30+i||contra||(!i&&map==0x23))&&(type<=2||(!i&&(type==0x13||type==0x14||type==0x15||type==0x1a||type==0x34||type==0x35)))')
p.write_text(s);(core/"cartridge.h").write_text(s)
inputs=O/'inputs';inputs.mkdir(exist_ok=True)
items=[]
for i,path in enumerate(args.roms):
 path=path.resolve()
 with zipfile.ZipFile(path)as z:
  members=[v for v in z.infolist()if v.filename.lower().endswith(('.sfc','.smc'))and not v.filename.startswith('__MACOSX/')]
  assert len(members)==1 and members[0].file_size<=0x400200
  data=z.read(members[0])
 if len(data)%32768==512:data=data[512:]
 (inputs/f'{i}.sfc').write_bytes(data);items.append({'archive':str(path)})
source=(O/'native.c');source.write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "state_format.h"
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);int nes_frame(uint16_t,int(*)(void));
uint32_t nes_hash(void);uint32_t nes_frames(void);unsigned nes_audio(const int16_t**);void pocket_player_close(void);
unsigned fc_state_store(uint8_t*,unsigned);int fc_state_load(const uint8_t*,unsigned);
void pocket_heap_init(void){}
static uint32_t ah(void){const int16_t*p;unsigned n=nes_audio(&p);uint32_t h=n;for(unsigned i=0;i<n*2;i++)h=(h^(uint16_t)p[i])*16777619u;return h;}
int main(int argc,char**argv){assert(argc>=2);unsigned count=argc>2?atoi(argv[2]):600;
 FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);assert(n>0&&n<=0x400000);uint8_t*rom=malloc(n);assert(fread(rom,1,n,f)==(unsigned)n);fclose(f);
 assert(!nes_init(rom,n,0,0));unsigned changes=0,last=0;
 for(unsigned i=0;i<count;i++){int e=nes_frame(i>=360&&i<368?8:0,0);if(e){printf("FRAME failure %u code=%d\n",i,e);return 2;}unsigned hash=nes_hash();changes+=last!=hash;last=hash;}
 printf("RUN frames=%u changed=%u last=%08x\n",count,changes,last);fflush(stdout);
 uint8_t*s=malloc(PS_STATE_MAX);unsigned size=fc_state_store(s,PS_STATE_MAX);assert(size&&ps_valid(s,size));printf("STATE bytes=%u frame=%u\n",size,nes_frames());fflush(stdout);
 uint32_t video[30],audio[30];for(unsigned i=0;i<30;i++){assert(!nes_frame(0,0));video[i]=nes_hash();audio[i]=ah();}
 assert(fc_state_load(s,size));assert(nes_frames()==count);
 for(unsigned i=0;i<30;i++){assert(!nes_frame(0,0));if(nes_hash()!=video[i]||ah()!=audio[i]){printf("REPLAY mismatch frame=%u video=%08x/%08x pcm=%08x/%08x\n",i,nes_hash(),video[i],ah(),audio[i]);return 3;}}
 puts("PASS exact 30-frame video/PCM replay");pocket_player_close();free(s);free(rom);return 0;}
''')
names='apu apuaux c4 c4emu cheats cheats2 clip data dsp1 fxemu fxinst globals dma memmap cpu cpuexec cpuops sa1 sa1cpu sdd1 sdd1emu snapshot soundux spc700 srtc ppu_ gfx tile'.split()
sources=[core/'src'/f'{n}.c'for n in names]+[core/'libretro/libretro.c',core/'libretro/libretro-common/streams/memory_stream.c',core/'player.c',source]
flags=['clang','-std=gnu99','-O2','-g','-fno-strict-aliasing','-fwrapv','-DHAVE_STRINGS_H','-DHAVE_STDINT_H','-DHAVE_INTTYPES_H','-D__LIBRETRO__','-DUSE_SA1','-D__OLD_RASTER_FX__','-DPOCKET_NATIVE','-Wno-incompatible-pointer-types','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all']
flags+=['-I'+str(p)for p in [inc,core,core/'src',core/'libretro',core/'libretro/libretro-common/include',snap/'source/amiga/mesen_bench']]
headers=hashlib.sha256(b''.join(p.read_bytes()for d in [inc,core]for p in sorted(d.rglob('*.h')))).digest();objects=[]
for i,p in enumerate(sources):
 obj=O/f'native-{i}.o';stamp=obj.with_suffix('.sha256');h=hashlib.sha256(headers+p.read_bytes()+str(flags).encode()).hexdigest();objects.append(obj)
 if not obj.exists()or not stamp.exists()or stamp.read_text()!=h:
  subprocess.run(flags+['-c',str(p),'-o',str(obj)],check=True);stamp.write_text(h)
subprocess.run(['clang','-fsanitize=address,undefined',*map(str,objects),'-lm','-o',str(O/'native')],check=True)
results=[]
for i,item in enumerate(items):
 try:
  run=subprocess.run([O/'native',inputs/f'{i}.sfc'],capture_output=True,text=True,timeout=40);status=run.returncode;log=run.stdout+run.stderr
 except subprocess.TimeoutExpired as e:status='timeout';log=str(e)
 (O/f'native-{i}.log').write_text(log);print(Path(item['archive']).name,status,log[-500:],flush=True);results.append(dict(rom=Path(item['archive']).name,status=status))
(O/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
