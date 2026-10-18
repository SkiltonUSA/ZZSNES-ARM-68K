#!/usr/bin/env python3
"""Build bundled Xanxi controls and the install-missing helper. No deployment."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def build(out):
 out=out.resolve();(out/'Scanlines').mkdir(parents=True,exist_ok=True)
 m=json.loads((ROOT/'docs/current-build.json').read_text())
 runtime=json.loads(os.environ.get('BETA_CONTAINER_COMMAND','["podman","--connection","nuflix-converter-root","run","--rm","--platform","linux/amd64"]'))
 cc=runtime+['-v',str(ROOT)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=gnu99','-noixemul','-m68020','-O2','-Wall','-Wextra']
 for name,source,target in [('ZZScanlines','amiga/scanlines/vendor/ZZScanlines.c',out/'Scanlines/ZZScanlines'),('ZZScanlinesGUI','amiga/scanlines/vendor/ZZScanlinesGUI.c',out/'Scanlines/ZZScanlinesGUI'),('Install-Scanlines','amiga/scanlines/install.c',out/'Install-Scanlines')]:
  subprocess.run(cc+(['-Werror']if name=='Install-Scanlines'else[])+[source,'-lamiga','-o',str(target.relative_to(ROOT))],cwd=ROOT,check=True)
  assert target.read_bytes()[:4]==bytes.fromhex('000003f3')
 print('PASS bundled scanline controls and installer',out)
if __name__=='__main__':build(ROOT/sys.argv[1])
