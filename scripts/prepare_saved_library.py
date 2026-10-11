#!/usr/bin/env python3
"""Seed ignored build output from verified archived ARM artifacts; no Amiga I/O."""
from pathlib import Path
import hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'artifacts/pocketsnes-library';out=ROOT/'.context/amiga/pocketsnes-library'
checks=json.loads((ROOT/'artifacts/SHA256.json').read_text())
files=[source/'pocket.elf',*sorted((source/'build-support').iterdir())]
for p in files:
 assert p.is_file() and hashlib.sha256(p.read_bytes()).hexdigest()==checks[str(p.relative_to(ROOT))],p
out.mkdir(parents=True,exist_ok=True)
for p in files:shutil.copy2(p,out/p.name)
import build_mesen_snes_system as fixture
fixture.OUT=out
from pocketsnes_library import test_rom
test_rom(out)
print('Prepared verified ARM payload, generated source support and original test ROM in',out)
