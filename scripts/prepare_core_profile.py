"""Opt-in stage timing using the already mapped, read-only A9 global timer."""
from pathlib import Path


def once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Unexpected profile source: ' + old[:80])
    return text.replace(old, new, 1)


def prepare(core):
    core = Path(core)
    (core / 'stage_profile.h').write_text('''#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
extern uint32_t ps_stage_ticks[3], ps_stage_calls[3];
#ifdef __cplusplus
}
#endif
static inline uint32_t ps_stage_clock(void) {
#ifdef __arm__
    return *(volatile uint32_t *)0xf8f00200u;
#else
    return 0; /* Native runs verify output, not A9 timing. */
#endif
}
''')
    targets = [('ppu_.c', 'S9xSuperFXExec', 'void', '', 0),
               ('gfx.c', 'S9xUpdateScreen', 'void', '', 1),
               ('soundux.c', 'S9xMixSamplesO',
                'signed short* buffer, int sample_count, int sample_offset',
                'buffer, sample_count, sample_offset', 2)]
    for file, name, args, values, index in targets:
        path = core / 'src' / file
        s = once(path.read_text(), 'void ' + name + '(' + args + ')',
                 'static void ' + name + '_profile_body(' + args + ')')
        s = '#include "stage_profile.h"\n' + s
        s += f'''
void {name}({args}) {{
    uint32_t start = ps_stage_clock();
    {name}_profile_body({values});
    ps_stage_ticks[{index}] += ps_stage_clock() - start;
    ps_stage_calls[{index}]++;
}}
'''
        path.write_text(s)
    path = core / 'player.c'
    path.write_text('#include "stage_profile.h"\nuint32_t ps_stage_ticks[3], ps_stage_calls[3];\n' + path.read_text())
    path = core / 'worker.cpp'
    s = '#include "stage_profile.h"\n' + path.read_text()
    s = once(s, '   ps_set_pad2(ad_get(cmd,88));',
             '   for(unsigned i=0;i<3;i++)ps_stage_ticks[i]=ps_stage_calls[i]=0;\n   ps_set_pad2(ad_get(cmd,88));')
    s = once(s, '   if(!error && (arg&65536u))publish_frame();',
             '   for(unsigned i=0;i<3;i++){ad_put(reply,96+i*4,ps_stage_ticks[i]);ad_put(reply,108+i*4,ps_stage_calls[i]);}\n   if(!error && (arg&65536u))publish_frame();')
    path.write_text(s)


def host(stage):
    path = Path(stage) / 'host.c'
    s = 'static unsigned long long ps_stage_total[3],ps_stage_count[3];\n' + path.read_text()
    s = once(s, 'syncio->pull(reply,96,NULL);', 'syncio->pull(reply,120,NULL);')
    s = once(s, 'if(op==NES_FRAME){frame_reply_ticks+=',
             'if(op==NES_FRAME){for(unsigned i=0;i<3;i++){ps_stage_total[i]+=ad_get(reply,96+i*4);ps_stage_count[i]+=ad_get(reply,108+i*4);}frame_reply_ticks+=')
    marker = ' printf("ARM_REQUEST_PROFILE '
    start = s.index(marker)
    s = s[:start] + ''' for(unsigned i=0;i<3;i++)printf("CORE_STAGE id=%u us=%u calls=%u\\n",i,rate?(ULONG)(ps_stage_total[i]*1000000u/rate):0,(ULONG)ps_stage_count[i]);
''' + s[start:]
    path.write_text(s)
