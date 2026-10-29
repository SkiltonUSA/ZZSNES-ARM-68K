"""Four-frame AHI batch experiment; no ARM or global AHI changes."""
from pathlib import Path
import difflib,json,hashlib,re

def prepare(stage):
 p=stage/'player_audio.h';old=p.read_text();new=old;stereo='AHIST_S16S' in old;old_size='12288' if stereo else '6144';new_size='16384' if stereo else '8192'
 for a,b in [('available>=3','available>=4'),('blocks=available>=4?3:available','blocks=available>=4?4:available'),(old_size,new_size),('Coalesce three immutable NES blocks','Coalesce four immutable console blocks')]:
  assert a in new,a;new=new.replace(a,b)
 new=re.sub(r'#define AUDIO_PREBUFFER [46]u[^\n]*', '#define AUDIO_PREBUFFER 6u /* Two frames beyond one four-frame batch. */',new)
 new=new.replace('8 queue slots, 4-frame prebuffer','8 queue slots, 6-frame prebuffer')
 if stereo:new=new.replace('(AUDIO_SLOTS+6u)','(AUDIO_SLOTS+8u)')
 p.write_text(new)
 (stage/'audio-batch.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='original/player_audio.h',tofile='batch4/player_audio.h')))
 (stage/'audio-batch.json').write_text(json.dumps({'batch_frames':4,'prebuffer_frames':6,'buffer_bytes':int(new_size),'original_sha256':hashlib.sha256(old.encode()).hexdigest(),'candidate_sha256':hashlib.sha256(new.encode()).hexdigest()},indent=2)+'\n')
