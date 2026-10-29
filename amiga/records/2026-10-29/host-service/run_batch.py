import os,sys,asyncio,functools,hashlib,http.server,json,threading,shutil
from pathlib import Path
from datetime import timedelta
from mcp import ClientSession
from mcp.client.streamable_http import streamablehttp_client
out=Path('.context/amiga/ahi-lock-1029/batch-results-'+os.environ.get('ZZ_SCHEDULE_DISPLAY','0'));stage=out/'stage';stage.mkdir(parents=True,exist_ok=True)
files={
 'nes-ref':'.context/amiga/ahi-lock-1029/nes-ref/Player',
 'nes-batch4':'.context/amiga/ahi-lock-1029/nes-batch4/Player',
 'snes-ref':'.context/amiga/ahi-lock-1029/snes-ref/Player',
 'snes-batch4':'.context/amiga/ahi-lock-1029/snes-batch4/Player',
 'Modes':'.context/amiga/ahi-lock-1029/Modes',
 'mario.nes':'.context/attachments/yuC3Ij/011 Mario Bros.nes',
 'smw.sfc':'.context/amiga/jit-test-smw.sfc'}
selected=sys.argv[1:] or ['nes-ref-1','nes-batch4-1','nes-batch4-2','nes-ref-2','snes-ref-1','snes-batch4-1','snes-batch4-2','snes-ref-2']
if os.environ.get('ZZ_AUDIO_REUSE_FILES'):files={}
for name,p in files.items():shutil.copy2(p,stage/name)
async def main():
 server=http.server.ThreadingHTTPServer(('10.0.0.21',55032),functools.partial(http.server.SimpleHTTPRequestHandler,directory=str(stage.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  async with streamablehttp_client('http://127.0.0.1:55010/mcp') as (r,w,_):
   async with ClientSession(r,w) as s:
    await s.initialize()
    async def c(n,a={}):
     v=await s.call_tool(n,a,read_timeout_seconds=timedelta(seconds=60));t='\n'.join(getattr(c,'text','')for c in v.content)
     with (out/'calls.jsonl').open('a') as f:f.write(json.dumps(dict(tool=n,args=a,result=t))+'\n')
     assert not v.isError and '[ERROR]' not in t,t
     return t
    async def run(script):return await c('amiga_run_script',dict(script=script,timeout=40))
    assert 'Sixies.ARM.Debug.Owner' not in await c('amiga_list_ports')
    await run('If NOT EXISTS SD032G:Dev/ZZCost1029\nMakeDir SD032G:Dev/ZZCost1029\nEndIf\n')
    for name in files:
     t=await run(f'C:wget -q -T 30 -O SD032G:Dev/ZZCost1029/{name} http://10.0.0.21:55032/{name}\nC:MD5Sum SD032G:Dev/ZZCost1029/{name}\n')
     assert hashlib.md5((stage/name).read_bytes()).hexdigest() in t.lower(),t
     print('verified',name,flush=True)
    if files:
     await run('Protect SD032G:Dev/ZZCost1029/Modes +e\nSD032G:Dev/ZZCost1029/Modes >SD032G:Dev/ZZCost1029/modes.log\n')
     await c('amiga_pull_file',dict(amiga_path='SD032G:Dev/ZZCost1029/modes.log',local_path=str((out/'modes.txt').resolve())))
    for name in selected:
     assert 'Sixies.ARM.Debug.Owner' not in await c('amiga_list_ports')
     binary=name.rsplit('-',1)[0]
     # writable temporary SRAM/state paths; no installed library/config touched.
     rom='mario.nes' if name.startswith('nes') else 'smw.sfc'
     session=('ZZSESSION1' if name.startswith('nes') else 'ZZSNESSESSION2')+'\n16 0 0 0\n'+f'SD032G:Dev/ZZCost1029/{rom}'.encode().hex()+'\n'+f'SD032G:Dev/ZZCost1029/{name}.state'.encode().hex()+'\n'+f'SD032G:Dev/ZZCost1029/{name}.ppm'.encode().hex()+'\nDISPLAY '+os.environ.get('ZZ_SCHEDULE_DISPLAY','0')+' 0 0 1\n'
     await c('amiga_write_file',dict(path='SD032G:Dev/ZZCost1029/test.session',offset=0,hex_data=session.encode().hex()))
     count=120 if 'verify' in name else 660
     script=f'FailAt 21\nStack 65536\nProtect SD032G:Dev/ZZCost1029/{binary} +e\nSD032G:Dev/ZZCost1029/{binary} --session SD032G:Dev/ZZCost1029/test.session --frames {count} >SD032G:Dev/ZZCost1029/{name}.log\nEcho $RC >SD032G:Dev/ZZCost1029/{name}.done\n'
     await c('amiga_write_file',dict(path='SD032G:Dev/ZZCost1029/start',offset=0,hex_data=script.encode().hex()))
     await run(f'If EXISTS SD032G:Dev/ZZCost1029/{name}.done\nDelete SD032G:Dev/ZZCost1029/{name}.done\nEndIf\nRun >NIL: Execute SD032G:Dev/ZZCost1029/start\n')
     await asyncio.sleep(55)
     for retry in range(6):
      if 'Sixies.ARM.Debug.Owner' not in await c('amiga_list_ports'):break
      await asyncio.sleep(10)
     else:raise RuntimeError('Run remains active; no further launch')
     await c('amiga_pull_file',dict(amiga_path=f'SD032G:Dev/ZZCost1029/{name}.log',local_path=str((out/(name+'.log')).resolve())))
     text=(out/(name+'.log')).read_text(encoding='latin1')
     assert 'owned memory released' in text and 'register_mismatches=00000000' in text and 'witness_bad=0' in text,text[-2000:]
     assert f'END exit=0 frames={count}' in text,text[-2000:]
     if 'verify' in name:assert 'verified_frames=3' in text,text[-2000:]
     else:assert 'SCHEDULE fixed=1 audit=0 capture=0 warmup=60 frames=600' in text,text[-2000:]
     if name.startswith('snes'):assert f'rendered={count} skipped=0' in text,text[-2000:]
     print(name,'completed',flush=True)
 finally:server.shutdown();server.server_close()
asyncio.run(main())
