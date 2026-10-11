"""Asserted transformations for the profiled, joystick-enabled release candidate."""
def once(s, old, new):
    assert s.count(old)==1, (old,s.count(old))
    return s.replace(old,new)

def frontend(name,s,flip_paced=False,noaudio=False):
    if name=='host.c':
        s=once(s,'static int player_audit;', 'static int player_audit,fc_scripted_input;')
        s=once(s,'if(strcmp(argv[5],"VERIFY"))return -1;player_audit=1;', 'if(!strcmp(argv[5],"VERIFY"))player_audit=1;else if(!strcmp(argv[5],"INPUT"))fc_scripted_input=1;else return -1;')
        s=once(s,'[frame_limit] [VERIFY]', '[frame_limit] [VERIFY|INPUT]')
        s=once(s,'#include "wait_profile.h"','#include "wait_profile.h"\n#include "frame_profile.h"\n#include "joystick.h"')
        s=once(s,'if(!core_only)player_audio_open();','fc_joy_open();\n if(!core_only)player_audio_open();' if not noaudio else 'fc_joy_open();if(0)player_audio_open(); /* No-output diagnostic; APU runs. */')
        s=once(s,'frame_start=us();next_frame_us=', 'fc_profile_frame=frames;\n  frame_start=us();next_frame_us=')
        s=once(s,'before=frame_start;if(!request(NES_FRAME,buttons |', 'before=frame_start;{ UBYTE input=fc_read_buttons();static UBYTE last;\n  if(input!=last){fc_input_changes++;}last=input;\n  if(!request(NES_FRAME,input |')
        s=once(s,')goto done;request_us+=us()-before;', '){goto done;}request_us+=us()-before;fc_sample(0,us()-before);}')
        # Only the per-frame drain (the pause path keeps its separate accounting).
        old='before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;'
        assert s.count(old)==2
        where=s.index(old,s.index('fc_profile_frame=frames;'))
        s=s[:where]+s[where:].replace(old,old+'fc_sample(1,us()-before);fc_read_buttons();',1)
        s=once(s,'audio_us+=us()-before;', 'audio_us+=us()-before;fc_sample(2,us()-before);')
        s=once(s,'if(!present(ad_get(base+MB_REPLY,24),frames+1))goto done;draw_us+=us()-before;', 'if(!present(ad_get(base+MB_REPLY,24),frames+1))goto done;draw_us+=us()-before;fc_sample(3,us()-before);')
        s=once(s,'  #ifndef NES_PLAYER\n  if((ULONG)(us()-start)', '  fc_sample(4,us()-frame_start);\n  #ifndef NES_PLAYER\n  if((ULONG)(us()-start)')
        s=once(s,' print_wait_profile();',' fc_profile_print();fc_joy_close();\n print_wait_profile();')
        s=once(s,'next_frame_us=0;', 'next_frame_us=0;fc_last_flip=0;fc_joy_buttons=0;')
        # The previous flip may still be locked at frame-start. Sample joystick
        # after that flip drains and retain its state for the next NES frame.
        s=once(s,'UBYTE input=fc_read_buttons();', 'UBYTE input=(UBYTE)(buttons|fc_joy_buttons);if(fc_scripted_input)input=frames>=89&&frames<99?8:0;')
        if flip_paced:
            a=s.index('  /* Bound the emulation rate without busy spinning.')
            b=s.index('  fc_sample(4,',a)
            s=s[:a]+'  /* Deferred-PAN acknowledgements provide output cadence. */\n'+s[b:]
    elif name=='fullscreen.h':
        s=once(s,'if(status==1){full_wait_us', 'if(status==1){fc_flip_sample();full_wait_us')
    elif name=='player_frontend.h':
        s=once(s,'ASLFR_InitialPattern,', 'ASLFR_InitialDrawer,(ULONG)"PROGDIR:ROMs",ASLFR_InitialPattern,')
    return s
