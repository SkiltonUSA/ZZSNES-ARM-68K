"""Native checks of current window geometry, settings and cadence."""
from pathlib import Path
import json,subprocess
r=Path(__file__).resolve().parents[1];d=r/'development/audio-window-20261029';o=r/'.context/audio-window-tests';o.mkdir(parents=True,exist_ok=True)
for name,includes,flags in [('window',[d],[]),('cadence',[r/'.context/ax-release-build/player'],[]),('settings',[r/'.context/ax-release-build/library'],['-DTEST_SNES']if json.loads((r/'docs/current-build.json').read_text())['console']=='ZZSNES' else[])]:
 subprocess.run(['clang','-std=c99','-fsanitize=address,undefined',*flags,*['-I'+str(p)for p in includes],str(d/('test_'+name+'.c')),'-o',str(o/name)],check=True)
 subprocess.run([str(o/name)],check=True)

subprocess.run(['clang','-std=c99','-fsanitize=address,undefined','-I'+str(d),'-I'+str(r/'.context/ax-release-build/player'),str(d/'test_snapshot.c'),'-o',str(o/'snapshot')],check=True)
subprocess.run([str(o/'snapshot')],cwd=o,check=True)
