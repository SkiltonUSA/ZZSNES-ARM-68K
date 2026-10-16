#!/usr/bin/env python3
"""NES cadence log metrics (32 kHz mono, not SNES stereo/32040 Hz)."""
import argparse,json
from pathlib import Path
from analyze_pocketsnes_cadence import analyze
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('logs',nargs='+',type=Path);p.add_argument('--output',type=Path)
a=p.parse_args();rows=[]
for path in a.logs:
 r=analyze(path,audio_rate=32000)
 r['mono_samples']=r.pop('stereo_pairs');r['audio_rate']=32000
 rows.append(r)
 print(f"{path.name}: emulation={r['emulated_fps']:.3f} presentation={r['presentations_per_second']:.3f} omitted={r['omitted']} gaps={r['gap_indicators']}")
if a.output:a.output.write_text(json.dumps(rows,indent=2)+'\n')
