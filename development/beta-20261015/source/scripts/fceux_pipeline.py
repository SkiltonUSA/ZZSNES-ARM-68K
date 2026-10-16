"""Optional combined FRAME/PRESENT experiment; baseline generators stay intact."""
def once(source, old, new):
    assert source.count(old) == 1, (old, source.count(old))
    return source.replace(old, new, 1)


def frontend(name, source):
    if name == 'wire.h':
        return source + '\n/* Private FCEUX command; same bounded mailbox and allocation. */\n#define NES_FRAME_PRESENT 10u\n'
    if name == 'player_audio.h':
        # Reserve a producer slot outside the bitmap lock. The worker is the
        # only other ring user and can only free slots, so the subsequent
        # player_audio_frame cannot block while copying PCM under the lock.
        index = source.index('static int player_audio_frame(void)')
        source = source[:index] + '''static int player_audio_reserve(void){
 ULONG start=us(),available;
 if(!audio_opened)return 1;
 for(;;){
  Forbid();available=audio_write-audio_read;Permit();
  if(available<AUDIO_SLOTS)return !audio_error;
  events();if(quit)return 1;
  if(audio_done||audio_error||(ULONG)(us()-start)>500000u)return 0;
  if(!profile_pause(5,1000))return 0;
 }
}
''' + source[index:]
        return source
    if name != 'host.c':
        return source
    source = once(source, 'if(op==NES_FRAME && !player_audio_submit())',
                  'if((op==NES_FRAME || op==NES_FRAME_PRESENT) && !player_audio_submit())')
    source = once(source, 'if(op==NES_FRAME)frame_reply_ticks+=ad_get(reply,48);',
                  'if(op==NES_FRAME)frame_reply_ticks+=ad_get(reply,48);\n'
                  '   if(op==NES_FRAME_PRESENT){frame_reply_ticks+=ad_get(reply,48)-ad_get(reply,52);present_reply_ticks+=ad_get(reply,52);}')
    source = once(source, 'op!=NES_FRAME && !profile_pause',
                  'op!=NES_FRAME && op!=NES_FRAME_PRESENT && !profile_pause')
    source = once(source, '(op==NES_PRESENT?50000u:10000000u)',
                  '((op==NES_PRESENT || op==NES_FRAME_PRESENT)?50000u:10000000u)')
    source = once(source, 'static int present(ULONG raw_hash,ULONG frame)',
                  'static int present(ULONG input,ULONG frame)')
    source = once(source, 'if(!request(NES_PRESENT,addr))return 0;',
                  'ad_put(shared+MB_HOST,28,addr);\n if(!request(NES_FRAME_PRESENT,input))return 0;')
    source = once(source, 'if(ad_get(shared+MB_REPLY,24)!=raw_hash)return 0;',
                  'if(ad_get(shared+MB_REPLY,24)!=ad_get(shared+MB_REPLY,56))return 0;')
    source = once(source, 'direct_ticks+=ad_get(shared+MB_REPLY,28);',
                  'direct_ticks+=ad_get(shared+MB_REPLY,52);')
    old = 'frame_start=us();next_frame_us=fc_next_deadline(frame_start,next_frame_us);before=frame_start;if(!request(NES_FRAME,buttons | (verify_frame(frames+1)?256u:0u)))goto done;request_us+=us()-before;'
    new = '''/* Finish the preceding flip and reserve PCM space before taking a
   * fresh bitmap lock. No timer or Intuition operation under the new lock. */
  before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;
  events();if(quit)break;if(player_paused)continue;
  if(!player_audio_reserve())goto done;
  if(quit)break;
  frame_start=us();next_frame_us=fc_next_deadline(frame_start,next_frame_us);before=frame_start;
  if(!present(buttons | (verify_frame(frames+1)?256u:0u),frames+1))goto done;
  request_us+=us()-before;'''
    source = once(source, old, new)
    source = once(source, '''  /* AHI may process input while waiting: release any outstanding bitmap lock first. */
  before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;''',
                  '''  /* A ring slot was reserved before locking. This bounded PCM copy
   * neither signals the worker nor calls Intuition/AHI. */''')
    source = once(source, '''  if(!core_only){
   before=us();if(!present(ad_get(base+MB_REPLY,24),frames+1))goto done;draw_us+=us()-before;
  }''', '  /* Combined command already populated and queued this frame. */')
    source = source.replace('PRESENT_PROFILE pre_us=', 'COMBINED_PROFILE pre_us=')
    return source


def worker(source):
    source = once(source, 'error=0,elapsed=0;', 'error=0,elapsed=0,present_elapsed=0,present_hash=0;')
    source = once(source, 'op==NES_FRAME && initialized',
                  '(op==NES_FRAME || op==NES_FRAME_PRESENT) && initialized')
    marker = '''#endif
  }
#ifdef NES_DIRECT
  else if(op==NES_PRESENT'''
    code = '''#endif
   if(op==NES_FRAME_PRESENT && !error){
    /* Emulation completes privately before any framebuffer mapping/write.
     * Host holds a fresh OS bitmap lock for this whole <=50ms request. */
    present_hash=nes_hash();uint32_t draw_start=(uint32_t)ticks();
    error=nes_present(base,ad_get(cmd,28),ad_get(cmd,16),ad_get(cmd,20),ad_get(cmd,24));
    present_elapsed=(uint32_t)ticks()-draw_start;
    if(!error && nes_hash()!=present_hash)error=10;
   }
  }
#ifdef NES_DIRECT
  else if(op==NES_PRESENT'''
    source = once(source, marker, code)
    source = once(source, 'if(op==NES_FRAME && !error && (arg&256u))for',
                  'if((op==NES_FRAME || op==NES_FRAME_PRESENT) && !error && (arg&256u))for')
    source = once(source, 'ad_put(reply,48,(uint32_t)ticks()-request_start);',
                  'ad_put(reply,52,present_elapsed);ad_put(reply,56,present_hash);\n  ad_put(reply,48,(uint32_t)ticks()-request_start);')
    return source
