#!/usr/bin/env python3
"""Convert supplied PNG or fallback SVG artwork to MUI and Workbench resources."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
GLYPHS={'Z':['11111','00001','00010','00100','01000','10000','11111'],
'N':['10001','11001','11001','10101','10011','10011','10001'],
'E':['11111','10000','10000','11110','10000','10000','11111'],
'S':['01111','10000','10000','01110','00001','00001','11110']}
def prepare_png(master,name,out):
 """Convert supplied artwork to embedded palette/planar resources, preserving alpha."""
 out.mkdir(parents=True,exist_ok=True)
 for size,kind in [(64,'icon'),(80,'sidebar'),(128,'splash')]:
  raw=subprocess.check_output(['magick',str(master),'-resize',f'{size}x{size}!','+dither','-colors','127','-depth','8','rgba:-'])
  palette=[(0,0,0)];lookup={};indices=[]
  for at in range(0,len(raw),4):
   r,g,b,a=raw[at:at+4];rgb=(r,g,b)
   if a<128:indices.append(0);continue
   if rgb not in lookup:lookup[rgb]=len(palette);palette.append(rgb)
   indices.append(lookup[rgb])
  assert len(palette)<=128;palette += [(0,0,0)]*(128-len(palette));depth=7
  body=bytearray(size*(size//8)*depth);planes=[0]*(2*size*(size//16))
  for y in range(size):
   for x in range(size):
    v=indices[y*size+x]
    for bit in range(depth):
     if v&(1<<bit):body[(y*depth+bit)*(size//8)+x//8]|=1<<(7-x%8)
    # Classic fallback retains a readable silhouette on old icon libraries.
    r,g,b=palette[v];classic=0 if not v else 1 if r+g+b<384 else 2
    for bit in range(2):
     if classic&(1<<bit):planes[bit*size*(size//16)+y*(size//16)+x//16]|=1<<(15-x%16)
  if kind=='icon':
   text='#define ICON_COLOR 1\nstatic UWORD icon_planes[]={'+','.join(map(str,planes))+'};\n'
   text+='static UBYTE icon_pixels[]={'+','.join(map(str,indices))+'};\n'
   text+='static struct ColorRegister icon_palette[]={'+','.join('{'+','.join(map(str,c))+'}'for c in palette)+'};\n'
  else:
   text=f'#define {kind.upper()}_DEPTH {depth}\nstatic const UBYTE {kind}_body[]={{'+','.join(map(str,body))+'};\n'
   text+=f'static const ULONG {kind}_colours[]={{'+','.join(hex(c*0x01010101)for rgb in palette for c in rgb)+'};\n'
  (out/(kind+'-art.h')).write_text('/* Generated from supplied '+master.name+'; do not edit. */\n'+text)
 subprocess.run(['magick',str(master),'-resize','256x256',str(out/(name+'.png'))],check=True)

def prepare(name,out):
 master=ROOT/'amiga/emulator/assets'/(name.lower()+'.png')
 if master.exists():return prepare_png(master,name,out)
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
