from pathlib import Path
import hashlib,json
from PIL import Image
ROOT=Path(__file__).resolve().parent
pictures=[Image.open(ROOT/'source'/n).convert('RGBA')for n in ['emuelec-1px.png','emuelec-2px.png']]
assert all(p.size==(1920,1080)for p in pictures)
boxes=[(0,0,240,1080),(1680,0,1920,1080)]
assert all(pictures[0].crop(b).tobytes()==pictures[1].crop(b).tobytes()for b in boxes)
panels=Image.new('RGB',(40,480))
for side,b in enumerate(boxes):
 p=pictures[0].crop(b);bg=Image.new('RGBA',p.size,(0,0,0,255));bg.alpha_composite(p)
 panels.paste(bg.convert('RGB').resize((40,240),Image.Resampling.LANCZOS),(0,side*240))
p=panels.quantize(colors=16,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
raw=p.tobytes();packed=bytes(raw[i]|raw[i+1]<<4 for i in range(0,len(raw),2));colors=p.getpalette()[:48]
palette=[((colors[i]>>3)<<11)|((colors[i+1]>>2)<<5)|(colors[i+2]>>3)for i in range(0,48,3)]
text='/* Generated from user-supplied EmuELEC side panels; see README.md. */\n#pragma once\n#include <stdint.h>\n'
text+='static const uint16_t border_emuelec_palette[16]={'+','.join(hex(v)for v in palette)+'};\n'
text+='static const uint8_t border_emuelec[9600]={\n'+''.join(','.join(hex(v)for v in packed[i:i+24])+',\n'for i in range(0,len(packed),24))+'};\n'
text+='static inline uint16_t border_emuelec_pixel(unsigned i){return border_emuelec_palette[(border_emuelec[i/2]>>(4*(i&1)))&15];}\n'
(ROOT/'emuelec_assets.h').write_text(text)
(ROOT/'emuelec.json').write_text(json.dumps({'sources':{n:hashlib.sha256((ROOT/'source'/n).read_bytes()).hexdigest()for n in ['emuelec-1px.png','emuelec-2px.png']},'rectangles':boxes,'identical_side_panels':True,'bytes':9632,'palette_colors':16,'centre_overlay_imported':False},indent=2)+'\n')
