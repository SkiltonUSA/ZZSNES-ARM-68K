from pathlib import Path
import shutil,subprocess,json,zipfile,argparse
parser=argparse.ArgumentParser(description="Native-only cartridge survey; never deploys or changes the installed player.")
parser.add_argument("roms",nargs="+",type=Path)
parser.add_argument("--allow-experimental-chips",action="store_true")
parser.add_argument("--frames",type=int,default=600)
parser.add_argument("--baseline",action="store_true",help="Use the unchanged 0.6.1 core")
args=parser.parse_args()
if not 1 <= args.frames <= 36000:parser.error("frames must be 1..36000")
from compatibility_native import build
ROOT=Path(__file__).resolve().parents[1];O=ROOT/'.context/cartridge-survey';O.mkdir(parents=True,exist_ok=True)
build(ROOT/'tests/amiga/pocketsnes/cartridge_replay.c',O,experimental=args.allow_experimental_chips,baseline=args.baseline)
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
