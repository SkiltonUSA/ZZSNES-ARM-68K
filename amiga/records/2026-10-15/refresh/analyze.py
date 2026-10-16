import re,json,statistics
from pathlib import Path
out=Path(__file__).parent
rows=[]
for name in ['ZZNES','ZZSNES']:
 for p in sorted(out.glob(name+'-bench-*.log')):
  s=p.read_text();assert 'LAUNCHER exit=0; owned memory released'in s and 'register_mismatches=00000000'in s and 'witness_bad=0'in s
  d=re.search(r'DIRECT_END challenges=(\d+).*copy_us=(\d+)',s);f=re.search(r'FRAME_TIMING stage=4 count=(\d+) mean_us=(\d+)',s);v=re.search(r'FRAME_TIMING stage=5 count=(\d+) mean_us=(\d+)',s)
  row=dict(console=name,file=p.name,case=p.stem.split('-bench-')[1][2:],presented=int(d[1]),arm_present_us=int(d[2])/int(d[1]),steady_emulation_fps=1e6/int(f[2]),steady_present_fps=1e6/int(v[2]));rows.append(row)
for name in ['ZZNES','ZZSNES']:
 vals={key:[r['arm_present_us']for r in rows if r['console']==name and r['case']==key]for key in ['baseline','optimised','soft-scalar','soft-neon']}
 if all(vals.values()):
  avg={k:statistics.median(v)for k,v in vals.items()}
  print(name,{k:round(v,2)for k,v in avg.items()},'gain',round((1-avg['optimised']/avg['baseline'])*100,2),'soft gain',round((1-avg['soft-neon']/avg['soft-scalar'])*100,2))
(out/'measurements.json').write_text(json.dumps(rows,indent=2)+'\n')
