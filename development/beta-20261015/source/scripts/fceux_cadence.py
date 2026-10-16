"""Host-only regional cadence adaptation; preserves FCEUX ARM/display/audio ABI."""
from pathlib import Path
from fceux_release import once

ROOT = Path(__file__).resolve().parents[1]

def apply(stage):
    for name in ('cadence.h', 'pace_target.h'):
        (stage / name).write_bytes((ROOT / 'amiga/pocketsnes/library' / name).read_bytes())
    p = stage / 'host.c'
    s = once(p.read_text(), '#include "wait_profile.h"',
             '#include "wait_profile.h"\n#include "cadence.h"\n#include "pace_target.h"\nstatic struct ps_cadence cadence;')
    s = once(s, 'next_frame_us=fc_next_deadline(frame_start,next_frame_us,fc_frame_us);',
             'if(!next_frame_us)cadence.valid=0;next_frame_us=ps_cadence_next(&cadence,frame_start,fc_frame_us);')
    s = once(s, '  if(!core_only){\n   before=us();if(!present(',
             '  if(!core_only && ps_cadence_show(&cadence,us(),fc_frame_us)){\n   before=us();if(!present(')
    s = once(s, 'target=(LONG)(next_frame_us-frame_start)>0?next_frame_us-frame_start:0;',
             'target=ps_pace_target(frame_start,next_frame_us,fc_frame_us);')
    s = once(s, '  /* Bound the emulation rate without busy spinning. Reset a late deadline\n   * to avoid a burst of catch-up frames after a window drag or task switch. */',
             '  /* Regional clock, two-frame lead, bounded catch-up; reset on pause/state/snapshot. */')
    s = s.replace('no frameskip', 'bounded picture omission; every core/APU frame retained')
    p.write_text(s)
