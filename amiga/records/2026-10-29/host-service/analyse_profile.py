from pathlib import Path
import json,re,statistics
w=Path(__file__).resolve().parent;rows=[]
for tag,ids in [('nes',[1,2]),('snes',[3,4])]:
 for i in ids:
  t=(w/'results-0'/f'{tag}-profile-{i}.log').read_text(encoding='latin1');costs=[]
  for m in re.finditer(r'HOST_COST kind=(\d+) calls=(\d+) total_us=(\d+) maximum_us=(\d+) bins=([\d,]+)',t):
   d=dict(zip(['kind','calls','total_us','maximum_us'],map(int,m.groups()[:4])));d['bins']=list(map(int,m[5].split(',')));assert sum(d['bins'])==d['calls'];costs.append(d)
  assert len(costs)==6 and costs[5]['calls']>=597
  assert 'END exit=0 frames=660' in t and 'owned memory released' in t
  a=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+).*error=0 stopped=1',t);assert a and a[1]==a[2]==a[3]
  rows.append(dict(console=tag,run=i,costs=costs))
for tag in ['nes','snes']:
 r=[x for x in rows if x['console']==tag]
 print(tag,{k:round(statistics.mean(sum(x['costs'][i]['total_us'] for i in idx) for x in r)/1000,2) for k,idx in [('locks',[0,1]),('send',[2,3]),('wait',[4]),('copy',[5])]})
(w/'profile-results.json').write_text(json.dumps(rows,indent=2)+'\n')
