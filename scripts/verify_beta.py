#!/usr/bin/env python3
"""Check archived Beta bytes and recompute README rates from hardware evidence."""
from pathlib import Path
import hashlib,importlib.util,json
root=Path(__file__).resolve().parents[1]
meta=json.loads((root/'docs/beta-build.json').read_text())
checks=json.loads((root/'docs/beta-sha256.json').read_text())
for p,h in checks.items():assert hashlib.sha256((root/p).read_bytes()).hexdigest()==h,p
snapshot=root/meta['snapshot_dir']
spec=importlib.util.spec_from_file_location('cadence_analysis',snapshot/'source/scripts/analyze_pocketsnes_cadence.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
readme=(root/'README.md').read_text()
for system,series,rate in [('zznes','contra',32000),('zznes','pal',32000),('zzsnes','long',32040),('zzsnes','pal',32040)]:
 p=root/'amiga/records/2026-10-12'/(system+'-cadence')/series
 saved=json.loads((p/'analysis.json').read_text())
 for expected in saved:
  actual=m.analyze(p/expected['file'],audio_rate=rate)
  if system=='zznes':actual['mono_samples']=actual.pop('stereo_pairs');actual['audio_rate']=rate
  assert actual==expected,(system,series,'raw analysis mismatch')
  row=actual.get('steady',actual)
  for k in ['emulated_fps','presentations_per_second']:assert f'{row[k]:.3f}' in readme,(system,series,k)
for p in list(snapshot.rglob('*'))+list((root/meta['artifact_dir']).rglob('*')):
 if p.is_file():assert p.suffix.lower() not in ['.nes','.sfc','.smc','.zzs','.sav','.srm','.adf','.hdf','.rom','.iso','.zip'],p
assert not (root/meta['artifact_dir']/'Library.cfg').exists()
print('PASS:',len(checks),'archived file hashes; all four timing comparisons match raw hardware logs and README; private ROM/state/config exclusions')
