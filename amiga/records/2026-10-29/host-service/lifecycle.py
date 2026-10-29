import asyncio,json,re
from pathlib import Path
from datetime import timedelta
from mcp import ClientSession
from mcp.client.streamable_http import streamablehttp_client
out=Path('.context/amiga/ahi-lock-1029/lifecycle');out.mkdir(exist_ok=True)
D='SD032G:Dev/ZZCost1029'
async def main():
 async with streamablehttp_client('http://127.0.0.1:55010/mcp') as (r,w,_):
  async with ClientSession(r,w) as s:
   await s.initialize()
   async def call(n,a={}):
    v=await s.call_tool(n,a,read_timeout_seconds=timedelta(seconds=60));t='\n'.join(getattr(c,'text','')for c in v.content)
    with (out/'calls.jsonl').open('a')as f:f.write(json.dumps(dict(tool=n,args=a,result=t))+'\n')
    assert not v.isError and '[ERROR]' not in t,t
    return t
   async def run(t):return await call('amiga_run_script',dict(script=t,timeout=40))
   async def active():assert 'Sixies.ARM.Debug.Owner' in await call('amiga_list_ports')
   async def key(code):
    await active()
    try:await call('amiga_input_key',dict(rawkey=code,direction='down'))
    finally:await call('amiga_input_key',dict(rawkey=code,direction='up'))
   async def switch():
    await active()
    try:
     await call('amiga_input_key',dict(rawkey='64',direction='down'));await key('44')
    finally:await call('amiga_input_key',dict(rawkey='64',direction='up'))
    await asyncio.sleep(3)
   for tag,rom,magic in [('nes','mario.nes','ZZSESSION1'),('snes','smw.sfc','ZZSNESSESSION2')]:
    assert 'Sixies.ARM.Debug.Owner' not in await call('amiga_list_ports')
    name=tag+'-batch4-lifecycle';cfg=magic+'\n16 0 0 0\n'+(D+'/'+rom).encode().hex()+'\n'+(D+'/'+name+'.state').encode().hex()+'\n'+(D+'/'+name+'.ppm').encode().hex()+'\nDISPLAY 0 0 0 1\n'
    await call('amiga_write_file',dict(path=D+'/test.session',offset=0,hex_data=cfg.encode().hex()))
    script=f'Stack 65536\n{D}/{tag}-batch4 --session {D}/test.session --frames 6000 >{D}/{name}.log\nEcho $RC >{D}/{name}.done\n'
    await call('amiga_write_file',dict(path=D+'/start',offset=0,hex_data=script.encode().hex()))
    await run(f'If EXISTS {D}/Player.ready\nDelete {D}/Player.ready\nEndIf\nRun >NIL: Execute {D}/start\n')
    await asyncio.sleep(18)
    for attempt in range(12):
     if 'PLAYER_READY' in await run(f'If EXISTS {D}/Player.ready\nEcho PLAYER_READY\nEndIf\n'):break
     await asyncio.sleep(3)
    else:raise RuntimeError('Player not ready: no input injected')
    await active()
    print(tag,'running: pause, switch to window, resume, switch to fullscreen, exit',flush=True)
    await key('19');await asyncio.sleep(2);await switch()
    await key('19');await asyncio.sleep(3);await switch();await asyncio.sleep(3)
    await key('59');await asyncio.sleep(5)
    assert 'Sixies.ARM.Debug.Owner' not in await call('amiga_list_ports')
    await call('amiga_pull_file',dict(amiga_path=f'{D}/{name}.log',local_path=str((out/(name+'.log')).resolve())))
    t=(out/(name+'.log')).read_text(encoding='latin1')
    for word in ['END exit=0','owned memory released','register_mismatches=00000000','witness_bad=0','SCANLINE_BACKEND window=1','SCANLINE_BACKEND window=0']:assert word in t,(word,t[-2500:])
    assert t.count('SCANLINE_BACKEND window=0')==2
    a=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+).*error=0 stopped=1',t);assert a and a[1]==a[2]==a[3]
    print('PASS',name,'window/fullscreen, pause/resume and clean quit; all PCM completed',flush=True)
asyncio.run(main())
