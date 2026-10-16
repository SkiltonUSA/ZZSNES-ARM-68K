#!/usr/bin/env python3
"""Original code-native pixel logos; SVG masters and classic MUI/Workbench data."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
GLYPHS={'Z':['11111','00001','00010','00100','01000','10000','11111'],
'N':['10001','11001','11001','10101','10011','10011','10001'],
'E':['11111','10000','10000','11110','10000','10000','11111'],
'S':['01111','10000','10000','01110','00001','00001','11110']}
def prepare(name,out):
 out.mkdir(parents=True,exist_ok=True)
 paths=['<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">',
 '<path d="M18 25H220V218H18Z M220 25L242 10V200L220 218 M18 25L40 10H242" fill="white" stroke="black" stroke-width="9" stroke-linejoin="round"/>',
 '<rect x="30" y="38" width="178" height="168" rx="8" fill="black"/>']
 for row,text in enumerate(['ZZ',name[2:]]):
  scale=9 if row==0 else 7;w=(len(text)*6-1)*scale;x0=119-w/2;y0=48 if row==0 else 128
  for c,ch in enumerate(text):
   for y,line in enumerate(GLYPHS[ch]):
    for x,v in enumerate(line):
     if v=='1':paths.append(f'<rect x="{x0+(c*6+x)*scale}" y="{y0+y*scale}" width="{scale}" height="{scale}" fill="white"/>')
 paths.append('</svg>');master=ROOT/'amiga/emulator/assets'/(name.lower()+'.svg');master.write_text('\n'.join(paths)+'\n')
 for size,kind in [(64,'icon'),(80,'sidebar'),(128,'splash')]:
  data=subprocess.check_output(['magick','-background','none',str(master),'-resize',f'{size}x{size}!','-depth','8','rgba:-'])
  body=bytearray(size*(size//8)*2);planes=[0]*(2*size*(size//16))
  for y in range(size):
   for x in range(size):
    r,g,b,a=data[(y*size+x)*4:(y*size+x+1)*4];v=0 if a<128 else 1 if r+g+b<384 else 2
    for bit in range(2):
     if v&(1<<bit):
      body[(y*2+bit)*(size//8)+x//8]|=1<<(7-x%8)
      planes[bit*size*(size//16)+y*(size//16)+x//16]|=1<<(15-x%16)
  if kind=='icon':text='static UWORD icon_planes[]={'+','.join(map(str,planes))+'};\n'
  else:text='static const UBYTE '+kind+'_body[]={'+','.join(map(str,body))+'};\n'
  (out/(kind+'-art.h')).write_text('/* Generated from '+master.name+'; do not edit. */\n'+text)
 subprocess.run(['magick','-background','none',str(master),str(out/(name+'.png'))],check=True)
if __name__=='__main__':
 for name in ['ZZNES','ZZSNES']:prepare(name,ROOT/'.context/amiga/emulator-next'/name/'library')
