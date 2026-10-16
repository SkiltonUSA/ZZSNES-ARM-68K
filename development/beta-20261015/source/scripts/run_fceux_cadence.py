#!/usr/bin/env python3
"""Run explicitly selected bounded cadence diagnostics on the configured Amiga.
Uses one dashboard stream, verifies uploaded files and teardown, then collects
logs. Never overwrites the normal player, ROMs or user save-state files.
"""
import argparse,asyncio,functools,hashlib,http.server,importlib.util,json,re,threading,time,uuid
from datetime import timedelta
from pathlib import Path
from mcp import ClientSession
from mcp.client.streamable_http import streamablehttp_client
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'.context/amiga/fceux-cadence'
REMOTE='SD032G:Dev/ZZNes-Cadence'
FETCH_SPEC=importlib.util.spec_from_file_location('fetch',ROOT/'amiga/records/2026-10-07/inventory/fetch.py')
fetch=importlib.util.module_from_spec(FETCH_SPEC);FETCH_SPEC.loader.exec_module(fetch)
async def main(a):
 meta=json.loads((OUT/'builds.json').read_text());dest=OUT/'runs'/a.series
 dest.mkdir(parents=True,exist_ok=False);transfer=dest/'transfer';transfer.mkdir()
 configs={};nonce=uuid.uuid4().hex[:12]
 for label in dict.fromkeys(a.variants):
  m=meta[label];exe='ZZNes-'+label;p=OUT/label/exe
  assert hashlib.md5(p.read_bytes()).hexdigest()==m['md5'],label
  (transfer/exe).write_bytes(p.read_bytes())
  lines=['ZZSESSION1','16 0 0 0','SD032G:Emulator/ZZNES/ROMs/Contra (USA).nes'.encode().hex(),'unused'.encode().hex(),'unused'.encode().hex()]
  if a.rom:lines[2]=a.rom.encode('latin1').hex()
  lines[3]=(REMOTE+'/unused.zzs').encode().hex()
  snap=REMOTE+'/AutoTitle-'+nonce+'.zzp' if label.startswith('Thumbnail') else ''
  lines[4]=(snap or REMOTE+'/'+a.series+'-unused.zzp').encode().hex();cfg=a.series+'-'+label+'.cfg';configs[label]=(cfg,snap)
  (transfer/cfg).write_text('\n'.join(lines)+'\n')
  (transfer/('Start-'+a.series+'-'+label)).write_text(f'; Isolated {label}; Q/Esc quits.\nStack 65536\n{REMOTE}/{exe} --session {REMOTE}/{cfg} >{REMOTE}/Visual-{a.series}-{label}.log\n')
 (dest/'builds.json').write_text(json.dumps({n:meta[n] for n in a.variants},indent=2)+'\n')
 server=http.server.ThreadingHTTPServer(('10.0.0.21',55032),functools.partial(http.server.SimpleHTTPRequestHandler,directory=str(transfer)))
 thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
 events=[];results=[];reference={}
 try:
  async with streamablehttp_client('http://127.0.0.1:55010/mcp') as (r,w,_):
   async with ClientSession(r,w) as session:
    await session.initialize()
    async def call(name,args=None):
     res=await session.call_tool(name,args or {},read_timeout_seconds=timedelta(seconds=55))
     text='\n'.join(getattr(x,'text','') for x in res.content)
     events.append({'tool':name,'args':args,'text':text});(dest/'events.json').write_text(json.dumps(events,indent=2))
     assert not res.isError,text;return text
    async def run(script,timeout=40):return await call('amiga_run_script',{'script':script,'timeout':timeout})
    async def owned():return 'Sixies.ARM.Debug.Owner' in await call('amiga_list_ports')
    async def idle():
     assert not await owned(),'Core1 already owned; no further test'
     text=await call('amiga_list_screens');assert 'Screens (1)' in text and 'Workbench' in text,text
    await idle();await run(f'If NOT EXISTS {REMOTE}\nMakeDir {REMOTE}\nEndIf\n')
    for p in sorted(transfer.iterdir()):
     text=await run(f'C:wget -q -T 20 -O {REMOTE}/{p.name} http://10.0.0.21:55032/{p.name}\nC:MD5Sum {REMOTE}/{p.name}\n')
     assert hashlib.md5(p.read_bytes()).hexdigest() in text.lower(),p.name
     if p.name.startswith('ZZNes-'):await run(f'Protect {REMOTE}/{p.name} +e\n')
     if p.name.startswith('Start-'):await run(f'Protect {REMOTE}/{p.name} +se\n')
    print('Staged and hash-verified',','.join(a.variants),flush=True)
    for index,label in enumerate([] if a.stage_only else a.variants):
     await idle();tag=f'{a.series}-{index}-{label}';logpath=f'{REMOTE}/{tag}.log';cfg,snap=configs[label]
     if snap:
      text=await run(f'If EXISTS {snap}\nEcho THUMBNAIL_ALREADY_EXISTS\nEndIf\n')
      assert 'THUMBNAIL_ALREADY_EXISTS' not in text,'Choose a fresh thumbnail path'
     await run(f'Echo >{REMOTE}/Test-Run "Stack 65536"\nEcho >>{REMOTE}/Test-Run "{REMOTE}/ZZNes-{label} --session {REMOTE}/{cfg} >{logpath}"\nRun >NIL: Execute {REMOTE}/Test-Run\n',10)
     print('START',tag,flush=True)
     for _ in range(30):
      if await owned():break
      await asyncio.sleep(1)
     else:raise RuntimeError('No Core1 owner acquired')
     started=time.monotonic();tick=started;watchdog=meta[label]['loop_cap_us']/1e6+60
     while await owned():
      if time.monotonic()-started>watchdog:
       await call('amiga_input_key',{'rawkey':'10','direction':'down'})
       await call('amiga_input_key',{'rawkey':'10','direction':'up'})
       raise RuntimeError('Watchdog: requested Q, stopped sequence; inspect cleanup before further work')
      if time.monotonic()-tick>=30:
       print('RUNNING',tag,round(time.monotonic()-started),'seconds',flush=True);tick=time.monotonic()
      await asyncio.sleep(2)
     log=fetch.fetch(logpath,dest/(tag+'.log')).decode('latin1')
     for token in ['register_mismatches=00000000','witness_bad=0','LAUNCHER exit=0; owned memory released','fault=0']:
      assert token in log,(tag,token)
     n=meta[label]['frames'];c=re.search(r'CADENCE_RUN frames=(\d+) elapsed_us=(\d+) shown=(\d+) omitted=(\d+) rebases=(\d+) late_max=(\d+) max_streak=(\d+) period_us=(\d+)',log)
     assert c and int(c[1])==n and int(c[3])+int(c[4])==n and int(c[7])<=2,tag
     flips=re.search(r'FULLSCREEN_END queued=(\d+) flips=(\d+) pending=(-?\d+)',log)
     assert flips and flips[1]==flips[2]==c[3] and flips[3]=='-1',tag
     audio=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+) highwater=(\d+) error=(\d+) stopped=(\d+)',log)
     assert audio and audio[1]==audio[2]==audio[3] and int(audio[1])>0 and audio[5]=='0' and audio[6]=='1',tag
     hashes=[tuple(re.search(r'n=(\d+).*core_hash=([0-9a-f]+) output_hash=([0-9a-f]+)',l).groups()) for l in log.splitlines() if l.startswith('FRAME n=')]
     if n in reference:assert hashes==reference[n],(tag,'game hashes differ')
     reference[n]=hashes
     if snap:
      assert 'SNAPSHOT saved '+snap in log and 'THUMBNAIL continued past capture frame' in log,tag
      data=fetch.fetch(snap,dest/'automatic-thumbnail.zzp');assert len(data)>1000
     else:assert 'SNAPSHOT saved' not in log,tag
     summary=[l for l in log.splitlines() if any(t in l for t in ['CADENCE_RUN','FULLSCREEN_END','AUDIO_TASK','AUDIO_END','FRAME_TIMING stage=5','LAUNCHER exit','SNAPSHOT saved','THUMBNAIL continued'])]
     results.append({'label':label,'frames':n,'emulated_fps':n*1e6/int(c[2]),'shown':int(c[3]),'omitted':int(c[4]),'verified':True,'summary':summary,'visual_result':'not evaluated'})
     (dest/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('PASS',tag,'\n'+'\n'.join(summary),flush=True)
    await idle()
    text=await run('C:MD5Sum SD032G:Emulator/ZZNES/ZZFceux-ui\nC:MD5Sum SD032G:Emulator/ZZNES/ZZNES\n')
    assert 'd8793363fa6b5a281471da85c158b024' in text and 'a910355e15dbfa8b61219538e915507a' in text
    print('DONE: Workbench, no Core1 owner, normal player/library unchanged.',flush=True)
 finally:
  server.shutdown();server.server_close();thread.join();print('Transfer server stopped.',flush=True)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--series',required=True);p.add_argument('--stage-only',action='store_true',help='Install isolated launchers without starting an emulator');p.add_argument('--rom',help='Existing Amiga ROM path; never transferred');p.add_argument('variants',nargs='+')
 a=p.parse_args()
 if not re.fullmatch('[a-zA-Z0-9_-]+',a.series) or any(not re.fullmatch('(Baseline|Cadence|Interactive)-(600|1200|18000)',v) for v in a.variants):p.error('Invalid series or variant')
 asyncio.run(main(a))
