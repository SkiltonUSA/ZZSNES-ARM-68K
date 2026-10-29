"""Optional 68k acknowledgement polling interval. No connection or deployment."""
import difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def prepare(stage):
    path=stage/'fullscreen.h';old=path.read_text()
    marker=' if(fc_windowed)return !full_fault;'
    assert old.count(marker)==1 and 'static UWORD *full_pointer;' in old
    if 'SCANOUT_GATE_MODE' in old:assert '#define SCANOUT_GATE_MODE 0u' in old
    assert old.count('full_pending_us=us();')==1
    new=old.replace('static UWORD *full_pointer;','#include "poll_interval.h"\nstatic uint32_t ap_last;\nstatic UWORD *full_pointer;')
    new=new.replace(marker,marker+'\n if(full_slots.pending>=0 && !ack_poll_due(us(),&ap_last,250))return !full_fault;')
    new=new.replace('full_pending_us=us();','full_pending_us=us();ap_last=full_pending_us-250u;')
    path.write_text(new)
    shutil.copy2(ROOT/'development/ack-poll-20261029/poll_interval.h',stage/'poll_interval.h')
    (stage/'ack-poll.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='original/fullscreen.h',tofile='candidate/fullscreen.h')))
    digest=lambda data:hashlib.sha256(data).hexdigest()
    (stage/'ack-poll.json').write_text(json.dumps(dict(interval_us=250,original_header=digest(old.encode()),candidate_header=digest(new.encode()),policy=digest((stage/'poll_interval.h').read_bytes()),arm_payload=digest((stage/'zz_payload.h').read_bytes())),indent=2)+'\n')
