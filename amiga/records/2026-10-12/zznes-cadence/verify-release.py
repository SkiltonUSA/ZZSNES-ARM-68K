import asyncio,functools,hashlib,http.server,importlib.util,json,re,threading,uuid
from pathlib import Path
from datetime import timedelta
from mcp import ClientSession
from mcp.client.streamable_http import streamablehttp_client
ROOT=Path.cwd();OUT=ROOT/'.context/amiga/fceux-cadence';remote='SD032G:Dev/ZZNes-Cadence';target='SD032G:Dev/ZZNes-Borders'
spec=importlib.util.spec_from_file_location('fetch',ROOT/'amiga/records/2026-10-07/inventory/fetch.py');fetch=importlib.util.module_from_spec(spec);spec.loader.exec_module(fetch)
runid='life-'+uuid.uuid4().hex[:8];out=OUT/runid;out.mkdir();transfer=out/'transfer';transfer.mkdir();events=[]
rom=json.loads((ROOT/'.context/amiga/nes-rom-import/manifest.json').read_text())[3]['destination']
for resume in [0,1]:
 lines=['ZZSESSION1',f'16 0 0 {resume}',rom.encode().hex(),f'{remote}/{runid}.zzs'.encode().hex(),f'{remote}/{runid}.zzp'.encode().hex()]
 (transfer/f'{runid}-{resume}.cfg').write_text('\n'.join(lines)+'\n')
(transfer/'ZZFceux-ui').write_bytes((OUT/'Release/ZZFceux-ui').read_bytes())
server=http.server.ThreadingHTTPServer(('10.0.0.21',55032),functools.partial(http.server.SimpleHTTPRequestHandler,directory=str(transfer)))
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
async def main():
 async with streamablehttp_client('http://127.0.0.1:55010/mcp') as (r,w,_):
  async with ClientSession(r,w) as session:
   await session.initialize()
   async def call(n,a=None):
    res=await session.call_tool(n,a or {},read_timeout_seconds=timedelta(seconds=55));t='\n'.join(getattr(x,'text','') for x in res.content);events.append({'tool':n,'args':a,'text':t});(out/'events.json').write_text(json.dumps(events,indent=2));assert not res.isError,t;return t
   async def run(script):return await call('amiga_run_script',{'script':script,'timeout':35})
   async def owner():return 'Sixies.ARM.Debug.Owner' in await call('amiga_list_ports')
   async def idle():
    assert not await owner();t=await call('amiga_list_screens');assert 'Screens (1)' in t and 'Workbench' in t,t
   async def key(k):
    await call('amiga_input_key',{'rawkey':k,'direction':'down'});await asyncio.sleep(.2);await call('amiga_input_key',{'rawkey':k,'direction':'up'})
   async def wait_owner(want,seconds):
    for _ in range(seconds):
     if await owner()==want:return
     await asyncio.sleep(1)
    raise RuntimeError('Unexpected owner state; stop, never reset/free')
   def verify(log):
    for token in ['register_mismatches=00000000','witness_bad=0','LAUNCHER exit=0; owned memory released','fault=0']:assert token in log,token
    a=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+).*error=0 stopped=1',log);assert a and a[1]==a[2]==a[3]
   await idle()
   for p in transfer.iterdir():
    t=await run(f'C:wget -q -T 20 -O {remote}/{p.name} http://10.0.0.21:55032/{p.name}\nC:MD5Sum {remote}/{p.name}\n');assert hashlib.md5(p.read_bytes()).hexdigest() in t.lower()
   await run(f'Protect {remote}/ZZFceux-ui +e\n')
   await run(f'Stack 65536\nRun >NIL: {remote}/ZZFceux-ui --session {remote}/{runid}-0.cfg --frames 1800 >{remote}/{runid}.log\n')
   await wait_owner(True,30)
   for _ in range(30):
    if 'ZZFceux fullscreen' in await call('amiga_list_screens'):break
    await asyncio.sleep(1)
   else:raise RuntimeError('No fullscreen')
   print('START MMC5 release lifecycle',flush=True)
   await asyncio.sleep(8);await key('19')
   try:
    await asyncio.sleep(.5)
    for _ in range(3):await key('54');await asyncio.sleep(1)
    await asyncio.sleep(2)
    for _ in range(3):await key('57');await asyncio.sleep(1)
    await asyncio.sleep(2);await key('58');await asyncio.sleep(2)
   finally:
    await key('19');await asyncio.sleep(3);await key('10')
   await wait_owner(False,40)
   log=fetch.fetch(f'{remote}/{runid}.log',out/'lifecycle.log').decode('latin1');verify(log)
   for token in ['STATE save OK','STATE load OK','SNAPSHOT saved']:assert token in log,token
   n=int(re.search(r'ZZFCEUX END exit=0 frames=(\d+)',log)[1]);assert n>300,n
   print('PASS snapshot, paused save/load, resume beyond frame 300, normal quit and checked restoration',flush=True)
   await idle();await run(f'Stack 65536\nRun >NIL: {remote}/ZZFceux-ui --session {remote}/{runid}-1.cfg --frames 120 >{remote}/{runid}-resume.log\n')
   await wait_owner(True,30);await wait_owner(False,40)
   log=fetch.fetch(f'{remote}/{runid}-resume.log',out/'resume.log').decode('latin1');verify(log);assert 'STATE load OK' in log
   print('PASS fresh-process state resume',flush=True)
   await idle()
   # Only the player changes, after both lifecycle tests pass. Retain old binary.
   old='3b3e008483c21c6b0bb6704bd4bc3207';new=hashlib.md5((transfer/'ZZFceux-ui').read_bytes()).hexdigest()
   before=await run(f'C:MD5Sum {target}/ZZFceux-ui\nC:MD5Sum {target}/ZZNesLibrary\nC:MD5Sum {target}/Library.cfg\n')
   assert old in before and 'c628639da59e85c4392a997a9293d115' in before
   cfg=[l for l in before.splitlines() if 'Library.cfg' in l];assert len(cfg)==1
   backup=target+'/ZZFceux-ui.pre-cadence-20261012'
   t=await run(f'If NOT EXISTS {backup}\nCopy {target}/ZZFceux-ui {backup}\nEndIf\nC:MD5Sum {backup}\nCopy {remote}/ZZFceux-ui {target}/ZZFceux-ui.new\nC:MD5Sum {target}/ZZFceux-ui.new\n');assert old in t and new in t
   t=await run(f'Copy {target}/ZZFceux-ui.new {target}/ZZFceux-ui\nProtect {target}/ZZFceux-ui +e\nC:MD5Sum {target}/ZZFceux-ui\nC:MD5Sum {target}/ZZNesLibrary\nC:MD5Sum {target}/Library.cfg\n');assert new in t and 'c628639da59e85c4392a997a9293d115' in t and cfg[0] in t
   (out/'deployment.json').write_text(json.dumps({'player':target+'/ZZFceux-ui','md5':new,'previous_md5':old,'backup':backup,'unchanged_library_md5':'c628639da59e85c4392a997a9293d115','library_config_unchanged':True,'lifecycle_passed':True},indent=2)+'\n')
   print('INSTALLED',target+'/ZZFceux-ui','MD5',new,'backup',backup,flush=True)
   await idle();print('DONE Workbench, no Core1 owner',flush=True)
try:asyncio.run(main())
finally:server.shutdown();server.server_close();thread.join();print('Transfer server stopped',flush=True)
