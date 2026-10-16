/* GPL-3.0-or-later. Standard lowlevel.library flags; no hardware pokes. */
#include <libraries/lowlevel.h>
#include <proto/lowlevel.h>
struct Library *LowLevelBase;
static UBYTE fc_joy_buttons;
static ULONG fc_joy_type,fc_joy_polls,fc_joy_nonzero;
static UBYTE fc_joy_decode(ULONG state){
 ULONG type=state&JP_TYPE_MASK;UBYTE b=0;
 if(type!=JP_TYPE_JOYSTK&&type!=JP_TYPE_GAMECTLR)return 0;
 if(state&JPF_JOY_UP)b|=16;
 if(state&JPF_JOY_DOWN)b|=32;
 if(state&JPF_JOY_LEFT)b|=64;
 if(state&JPF_JOY_RIGHT)b|=128;
 if(state&JPF_BUTTON_RED)b|=1;
 if(state&JPF_BUTTON_BLUE)b|=2;
 if(type==JP_TYPE_GAMECTLR){if(state&JPF_BUTTON_GREEN)b|=4;
 if(state&JPF_BUTTON_PLAY)b|=8;}
 return b;
}
static UWORD fc_read_buttons(void){
 UBYTE b=0;
 /* Only sample once per emulated frame, outside every bitmap lock. Port 1
  * is the physical joystick port (port 0 remains the mouse). */
 if(LowLevelBase){ULONG state=ReadJoyPort(1);fc_joy_type=state&JP_TYPE_MASK;fc_joy_polls++;b=fc_joy_decode(state);
 if(b)fc_joy_nonzero++;}
 if(fc_fire){UBYTE a=b&1;b=(b&~3u)|((b&2)>>1)|(a<<1);}
 fc_joy_buttons=b;return buttons|b;
}
static void fc_joy_open(void){LowLevelBase=OpenLibrary("lowlevel.library",40);printf("JOYSTICK available=%u port=1; red=B blue=Y green=Select play=Start\n",(ULONG)!!LowLevelBase);}
static void fc_joy_close(void){printf("JOYSTICK_END polls=%u active=%u last_type=%08x\n",fc_joy_polls,fc_joy_nonzero,fc_joy_type);
 if(LowLevelBase){CloseLibrary(LowLevelBase);LowLevelBase=NULL;}}
