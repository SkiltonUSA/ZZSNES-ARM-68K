"""Verify current archived inputs/artifacts and private-file exclusions."""
import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
checks=json.loads((root/'docs/current-sha256.json').read_text())
for rel,h in checks.items():
 p=root/rel;assert hashlib.sha256(p.read_bytes()).hexdigest()==h,rel
 assert p.suffix.lower()not in ['.nes','.sfc','.smc','.sav','.zzs','.srm','.rom','.iso','.adf'],rel
 assert p.name not in ['Library.cfg','Session.cfg','Run-player'],rel
print('PASS',len(checks),'current source/artifact hashes and private-data exclusions')
