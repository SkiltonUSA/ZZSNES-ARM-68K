#!/usr/bin/env python3
"""Check visual indicators against the actual generated player joystick mapping."""
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts'))
from fceux_ui import frontend
OUT=ROOT/'.context/amiga/controller-ui'
OUT.mkdir(parents=True,exist_ok=True)
# Same lowlevel flag definitions used in existing host compatibility checks.
prefix=(ROOT/'tests/amiga/fceux_player/check_release.py').read_text().split("prefix='''",1)[1].split("'''",1)[0]
source=frontend('joystick.h',(ROOT/'amiga/fceux_player/joystick.h').read_text())
source=source[:source.index('static void fc_joy_open')]
source='\n'.join(s for s in source.splitlines() if not s.startswith('#include'))
test=prefix+'''
static ULONG input;static UBYTE buttons;static unsigned fc_fire;
static ULONG ReadJoyPort(unsigned port){assert(port==1);return input;}
#define CONTROLLER_MODEL_ONLY
#include "library_controller.h"
'''+source+'''
int main(void){
 const ULONG flags[8]={JPF_BUTTON_RED,JPF_BUTTON_BLUE,JPF_BUTTON_GREEN,JPF_BUTTON_PLAY,JPF_JOY_UP,JPF_JOY_DOWN,JPF_JOY_LEFT,JPF_JOY_RIGHT};
 LowLevelBase=(struct Library*)1;
 for(unsigned type=0;type<16;type++)for(unsigned n=0;n<256;n++)for(fc_fire=0;fc_fire<2;fc_fire++){
  input=type<<28;for(unsigned bit=0;bit<8;bit++)if(n&(1u<<bit))input|=flags[bit];
  assert(controller_bits(input,fc_fire)==fc_read_buttons());
 }
 assert(controller_bits(0,0)==0);
 puts("PASS 8192 controller states match player: directions/diagonals, releases, fire swaps, gamepad Select/Start, mouse/unknown rejection");
}
'''
p=OUT/'controller-test.c';p.write_text(test)
subprocess.run(['clang','-std=c99','-O2','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'amiga/fceux_player'),str(p),'-o',str(OUT/'controller-test')],check=True)
r=subprocess.run([OUT/'controller-test'],capture_output=True,text=True)
(OUT/'tests.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='');r.check_returncode()
