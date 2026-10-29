"""Four-frame AHI batch experiment; no ARM or global AHI changes."""
from pathlib import Path
import difflib,json,hashlib,re

def prepare_batch(stage):
 p=stage/'player_audio.h';old=p.read_text();new=old;stereo='AHIST_S16S' in old;old_size='12288' if stereo else '6144';new_size='16384' if stereo else '8192'
 for a,b in [('available>=3','available>=4'),('blocks=available>=4?3:available','blocks=available>=4?4:available'),(old_size,new_size),('Coalesce three immutable NES blocks','Coalesce four immutable console blocks')]:
  assert a in new,a;new=new.replace(a,b)
 new=re.sub(r'#define AUDIO_PREBUFFER [46]u[^\n]*', '#define AUDIO_PREBUFFER 6u /* Two frames beyond one four-frame batch. */',new)
 new=new.replace('8 queue slots, 4-frame prebuffer','8 queue slots, 6-frame prebuffer')
 if stereo:new=new.replace('(AUDIO_SLOTS+6u)','(AUDIO_SLOTS+8u)')
 p.write_text(new)
 (stage/'audio-batch.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='original/player_audio.h',tofile='batch4/player_audio.h')))
 (stage/'audio-batch.json').write_text(json.dumps({'batch_frames':4,'prebuffer_frames':6,'buffer_bytes':int(new_size),'original_sha256':hashlib.sha256(old.encode()).hexdigest(),'candidate_sha256':hashlib.sha256(new.encode()).hexdigest()},indent=2)+'\n')


def prepare_profile(stage):
 root=Path(__file__).resolve().parents[1]
 for name in ('cost.h','profile.h'):
  (stage/name).write_bytes((root/'development/host-service-20261029'/name).read_bytes())
 path=stage/'host.c';old=path.read_text();new=old
 for a,b in [('#include "wait_profile.h"','#include "profile.h"\n#include "wait_profile.h"'),
             ('  fc_profile_frame=frames;','  hc_active=frames>=60;fc_profile_frame=frames;'),
             ('done:\n','done:\n hc_active=0;\n'),
             (' print_wait_profile();',' hc_print();\n print_wait_profile();')]:
  assert new.count(a)==1,a;new=new.replace(a,b)
 path.write_text(new)
 path=stage/'player_audio.h';audio=path.read_text();width=4 if 'AHIST_S16S' in audio else 2
 needle=f'CopyMem(audio_queue[index].data,data[slot]+n*{width},count*{width});'
 assert audio.count(needle)==1
 path.write_text(audio.replace(needle,needle.replace('CopyMem','hc_pcm_copy')))
 (stage/'host-profile.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='original/host.c',tofile='profile/host.c')))
 (stage/'host-service.json').write_text(json.dumps({'kind':'elapsed-call-profile','warmup_frames':60,'sample_bytes':width,'hashes':{n:hashlib.sha256((stage/n).read_bytes()).hexdigest() for n in ('host.c','player_audio.h','profile.h','cost.h','zz_payload.h')}},indent=2)+'\n')


def prepare(stage,kind):
 if kind=='profile':prepare_profile(stage)
 elif kind=='batch4':prepare_batch(stage)
 else:raise ValueError(kind)
