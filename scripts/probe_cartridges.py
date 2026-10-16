from pathlib import Path
import shutil,subprocess,json,hashlib,zipfile,argparse
parser=argparse.ArgumentParser(description="Native-only cartridge survey; never deploys or changes the installed player.")
parser.add_argument("roms",nargs="+",type=Path)
parser.add_argument("--allow-experimental-chips",action="store_true")
parser.add_argument("--frames",type=int,default=600)
parser.add_argument("--baseline",action="store_true",help="Use the unchanged 0.6.1 core")
args=parser.parse_args()
if not 1 <= args.frames <= 36000:parser.error("frames must be 1..36000")
ROOT=Path(__file__).resolve().parents[1];O=ROOT/'.context/cartridge-survey';O.mkdir(parents=True,exist_ok=True);r=ROOT;snap=r/'development/beta-20261015';core=O/'native-core'
shutil.copytree(snap/'source/core',core,dirs_exist_ok=True)
if not args.baseline:
 shutil.copytree(r/'development/compatibility-20261016/core',core,dirs_exist_ok=True)
inc=O/'include';shutil.copytree(snap/'build-inputs/player',inc,dirs_exist_ok=True)
p=inc/'cartridge.h';s=p.read_text()
if args.allow_experimental_chips:s=s.replace('(map==0x20+i||map==0x30+i||contra)&&type<=2','(((map==0x20+i||map==0x30+i||contra)&&type<=2)||(!i&&map==0x20&&(type==0x13||type==0x14||type==0x15||type==0x1a))||(!i&&map==0x23&&(type==0x34||type==0x35)))')
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
source=r/'tests/amiga/pocketsnes/cartridge_replay.c'

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
 work=O/f'run-{i}';work.mkdir(exist_ok=True)
 try:
  run=subprocess.run([O/'native',inputs/f'{i}.sfc',str(args.frames)],capture_output=True,text=True,timeout=120,cwd=work);status=run.returncode;log=run.stdout+run.stderr
 except subprocess.TimeoutExpired as e:status='timeout';log=str(e)
 (O/f'native-{i}.log').write_text(log);
 if (work/'last-frame.rgb565').exists():shutil.copy2(work/'last-frame.rgb565',O/f'frame-{i}.rgb565')
 fresh_status='not run'
 if status in (0,3):
  fresh=subprocess.run([O/'native',inputs/f'{i}.sfc',str(args.frames),'resume'],capture_output=True,text=True,cwd=work,timeout=30)
  fresh_status=fresh.returncode;(O/f'fresh-{i}.log').write_text(fresh.stdout+fresh.stderr)
 print(Path(item['archive']).name,status,log[-500:],flush=True);results.append(dict(rom=Path(item['archive']).name,frames=args.frames,baseline=args.baseline,status=status,fresh_status=fresh_status))
(O/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
