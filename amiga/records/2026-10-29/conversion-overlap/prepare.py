"""Isolated fixed-presentation experiment. No connection or deployment."""
import difflib,hashlib,json
from pathlib import Path

def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new)

def prepare(stage):
    original={name:(stage/name).read_text() for name in ('host.c','fullscreen.h','player_audio.h')}
    host=original['host.c'];full=original['fullscreen.h'];audio=original['player_audio.h']
    # Explicitly limited to controlled every-frame tests, not release cadence.
    assert 'if(!core_only) /* Diagnostic: present every completed frame. */' in host
    assert 'ack_poll_due' not in full
    if 'SCANOUT_GATE_MODE' in full:assert '#define SCANOUT_GATE_MODE 0u' in full
    full=once(full,'static UWORD *full_pointer;', '''static UWORD *full_pointer;
/* A PAN completion cannot release P96 while ARM writes another free slot. */
static int full_rendering;
static struct RenderInfo full_render_info;''')
    full=once(full,'static void full_unlock(void){','static void full_unlock(void){\n if(full_rendering)return;')
    at='static int full_queue(void){'
    helpers='''/* The cached geometry is valid only while this exact P96 lock is retained. */
static int full_render_begin(struct RenderInfo *ri){
 full_rendering=1;
 if(!direct_locked){
  full_origin_known=0;memset(&full_render_info,0,sizeof(full_render_info));
  direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&full_render_info,sizeof(full_render_info));direct_locked=1;
 }
 *ri=full_render_info;return 1;
}
static int full_render_finish(void){
 /* Call only after the PRESENT token/hash checks and optional readback. */
 if(!full_drain())return 0;
 full_rendering=0;full_unlock();return 1;
}
static int full_relock(void){
 struct RenderInfo ri;
 if(!full_front())return 0;
 /* Input needs an unlocked interval. Reject relocation during that interval:
  * no new ARM write or PAN may use a stale address. Cleanup retains this lock. */
 full_origin_known=0;memset(&ri,0,sizeof(ri));
 direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;
 if(ri.Memory!=full_render_info.Memory || ri.BytesPerRow!=full_render_info.BytesPerRow ||
    ri.RGBFormat!=full_render_info.RGBFormat){puts("CONVERSION bitmap relocated; stopping before PAN");return 0;}
 full_origin_known=1;return 1;
}
'''
    full=once(full,at,helpers+at)
    ending=' full_slots.pending=-1;'+('full_gate.phase=0;' if 'SCANOUT_GATE_MODE' in full else '')+'full_unlock();'
    full=once(full,ending,ending.replace('full_unlock();','full_rendering=0;full_unlock();'))
    old=''' memset(&ri,0,sizeof(ri));
#ifdef NES_FULLSCREEN
 full_origin_known=0;
#endif
 direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;'''
    new=''' if(!fc_windowed){if(!full_render_begin(&ri))return 0;}
 else {
  memset(&ri,0,sizeof(ri));full_origin_known=0;
  direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;
 }'''
    host=once(host,old,new)
    host=once(host,' if(!full_drain())return 0;\n if(!full_front())',' if(fc_windowed&&!full_drain())return 0;\n if(!full_front())')
    host=once(host,' if(!full_queue())return 0;', ''' if(!fc_windowed){
  if(!full_render_finish())return 0;
  events();fc_read_buttons();
  if(quit)return 1;
  if(!full_relock())return 0;
 }
 if(!full_queue())return 0;''')
    host=once(host,'  /* AHI may process input while waiting: release any outstanding bitmap lock first. */\n  before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;fc_sample(1,us()-before);fc_read_buttons();', '''  /* Sole producer: a free PCM slot cannot fill underneath us. Only the full
   * ring path can process input/yield; drain before allowing that path. PCM is
   * copied before PRESENT overwrites the reply metadata. */
  before=us();if((fc_windowed||core_only||!player_audio_room())&&!full_drain())goto done;flip_drain_us+=us()-before;fc_sample(1,us()-before);
  if(fc_windowed||core_only)fc_read_buttons();''')
    audio=once(audio,'static int player_audio_frame(void){','''static int player_audio_room(void){
 ULONG available;Forbid();available=audio_write-audio_read;Permit();
 return available<AUDIO_SLOTS;
}
static int player_audio_frame(void){''')
    hashes={}
    for name,new in [('host.c',host),('fullscreen.h',full),('player_audio.h',audio)]:
        old=original[name];(stage/name).write_text(new)
        (stage/(name+'.overlap.patch')).write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='original/'+name,tofile='overlap/'+name)))
        hashes[name]={'original':hashlib.sha256(old.encode()).hexdigest(),'candidate':hashlib.sha256(new.encode()).hexdigest()}
    (stage/'conversion-overlap.json').write_text(json.dumps({'fixed_presentation_only':True,'files':hashes},indent=2)+'\n')

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('stage',type=Path);prepare(p.parse_args().stage.resolve())
