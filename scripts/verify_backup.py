#!/usr/bin/env python3
"""Verify saved artifacts, source pins and the documented timing summary."""
from pathlib import Path
import hashlib,json,statistics,re
root=Path(__file__).resolve().parents[1]
checks=json.loads((root/'artifacts/SHA256.json').read_text())
checks.update(json.loads((root/'sources/SHA256.json').read_text()))
for name,want in checks.items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==want,name
for folder in ['amiga/pocketsnes/vendor','amiga/mesen_snes/vendor','amiga/mesen_snes/system/vendor']:
    p=root/folder;m=json.loads((p/'upstream.json').read_text())
    for name,want in m['files'].items():
        if isinstance(want,dict):want=want['sha256']
        assert hashlib.sha256((p/name).read_bytes()).hexdigest()==want,name
p=root/'amiga/records/2026-10-10/pocketsnes';log=(p/'benchmark.log').read_text()
rate=int(re.search(r'estimated_hz=(\d+)',log)[1])
samples=[int(m[1])*1000/rate/6 for m in re.finditer(r'SAMPLE n=\d+ warmup=0.*?ticks=(\d+)',log)]
assert len(samples)==10
assert abs(statistics.mean(samples)-json.loads((p/'results.json').read_text())['mean_ms_per_frame'])<1e-9
for marker in ['POCKETSNES PASS','CANCEL PASS active_work=1','witness_bad=0','register_mismatches=00000000','owned memory released']:
    assert marker in log,marker
print('PASS: saved artifacts, pinned core sources, measured mean and hardware completion markers')
