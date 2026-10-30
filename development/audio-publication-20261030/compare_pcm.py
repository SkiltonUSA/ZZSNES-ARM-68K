#!/usr/bin/env python3
"""Compare big-endian host PCM with a little-endian stereo ARM capture."""
import argparse,csv,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('hardware',type=Path);p.add_argument('reference',type=Path);p.add_argument('frames',type=Path);p.add_argument('--require-match',action='store_true');a=p.parse_args()
h=a.hardware.read_bytes();r=a.reference.read_bytes();assert len(h)==len(r) and len(h)%4==0
actual=struct.unpack('>'+'h'*(len(h)//2),h);expected=struct.unpack('<'+'h'*(len(r)//2),r)
pos=different=stale=0;previous=[]
for row in csv.DictReader(a.frames.open()):
 n=int(row['sample_frames'])*2
 for i,(x,y) in enumerate(zip(actual[pos:pos+n],expected[pos:pos+n])):
  if x!=y:
   different+=1;stale+=i<len(previous) and x==previous[i]
 previous=expected[pos:pos+n];pos+=n
assert pos==len(actual)
print(json.dumps(dict(sample_frames=len(actual)//2,individual_samples=len(actual),different=different,previous_frame_matches=stale,hardware_sha256=hashlib.sha256(h).hexdigest(),model_little_endian_sha256=hashlib.sha256(r).hexdigest()),indent=2))
if a.require_match:assert different==0
