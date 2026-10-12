"""Inject bounded screenshot hooks into the existing release-ui 68k host."""
from fceux_release import once

def frontend(s):
    if '#include "snapshot_host.h"' in s:
        raise ValueError('Snapshot hooks already present')
    snes = 'PocketSNES' in s
    s = once(s, 'if(argc==3&&!strcmp(argv[1],"--session"))return fc_session(argv[2])?1:-1;',
             '''if((argc==3||argc==5)&&!strcmp(argv[1],"--session")){
 if(argc==5){char*end;unsigned long n;if(strcmp(argv[3],"--frames"))return -1;errno=0;n=strtoul(argv[4],&end,10);if(errno||*end||!n||n>36000)return -1;frame_limit=n;}
 return fc_session(argv[2])?1:-1;}''')
    s = once(s, '#include "view.h"', '#include "view.h"\n' +
             ('#define SNAPSHOT_SNES 1\n' if snes else '') + '#include "snapshot_host.h"')
    s = once(s, 'else if(key==0x54||key==0x57)',
             'else if(key==0x58){if(!(code&128))fc_snapshot_action=1;}\n   else if(key==0x54||key==0x57)')
    s = once(s, ' int verify=verify_frame(frame);',
             ' int verify=verify_frame(frame),capture=fc_snapshot_due(frame);')
    s = once(s, ' if(verify){\n  syncio->pull(ri.Memory,span,NULL);',
             ' if(verify||capture){\n  syncio->pull(ri.Memory,span,NULL);')
    s = once(s, 'if(verify && !full_drain())return 0;',
             'if((verify||capture) && !full_drain())return 0;')
    s = once(s, ' present_post_us+=us()-profile_after;',
             ' if(capture)fc_snapshot_store(direct_readback,span,pitch,direct_format);\n present_post_us+=us()-profile_after;')
    s = once(s, ' memcpy((void *)(base+NES_ROM),rom,size);',
             ' fc_snapshot_init(rom,size);\n memcpy((void *)(base+NES_ROM),rom,size);')
    s = once(s, '   if(player_paused){if(!pause_us(10000))goto done;continue;}',
             '   if(fc_snapshot_action&&direct_ready){if(!present(ad_get(shared+MB_REPLY,24),frames))goto done;}\n'
             '   if(player_paused){if(!pause_us(10000))goto done;continue;}')
    return s
