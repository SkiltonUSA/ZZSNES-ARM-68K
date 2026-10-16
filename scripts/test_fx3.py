from pathlib import Path
import subprocess
from compatibility_native import build
r=Path(__file__).resolve().parents[1]
exe=build(r/'tests/amiga/pocketsnes/fx3.c',r/'.context/fx3-tests',fx3=True)
subprocess.run([exe],check=True)
