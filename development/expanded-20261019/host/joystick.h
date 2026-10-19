/* GPL-3.0-or-later. Standard lowlevel.library flags; no hardware pokes. */
#include <libraries/lowlevel.h>
#include <proto/lowlevel.h>
struct Library *LowLevelBase;
static UWORD fc_joy_buttons,fc_joy_second;
static ULONG fc_joy_type,fc_joy_polls,fc_joy_nonzero;
static UWORD fc_joy_decode(ULONG state,unsigned player){
 ULONG type=state&JP_TYPE_MASK;UWORD b=0;
 static const ULONG flags[]={JPF_BUTTON_RED,JPF_BUTTON_BLUE,JPF_BUTTON_GREEN,JPF_BUTTON_YELLOW,JPF_BUTTON_FORWARD,JPF_BUTTON_REVERSE,JPF_BUTTON_PLAY};
 if(type!=JP_TYPE_JOYSTK&&type!=JP_TYPE_GAMECTLR)return 0;
 if(state&JPF_JOY_UP)b|=16;if(state&JPF_JOY_DOWN)b|=32;
 if(state&JPF_JOY_LEFT)b|=64;if(state&JPF_JOY_RIGHT)b|=128;
 for(unsigned i=0;i<(type==JP_TYPE_GAMECTLR?7u:2u);i++)if((state&flags[i])&&fc_controls.joy[player][i]<12)b|=1u<<fc_controls.joy[player][i];
 return b;
}
static UWORD fc_read_buttons(void){
 UWORD b=0;fc_joy_second=0;
 if(LowLevelBase){ULONG state=ReadJoyPort(1);fc_joy_type=state&JP_TYPE_MASK;fc_joy_polls++;b=fc_joy_decode(state,0);if(b)fc_joy_nonzero++;
  /* Port 1 remains the mouse unless the user explicitly enables player 2. */
  if(fc_controls.second==2)fc_joy_second=fc_joy_decode(ReadJoyPort(0),1);
 }
 if(fc_fire){UWORD a=b&1;b=(b&~3u)|((b&2)>>1)|(a<<1);}
 fc_joy_buttons=b;return buttons|b;
}
static void fc_joy_open(void){LowLevelBase=OpenLibrary("lowlevel.library",40);printf("JOYSTICK available=%u port=1; red=B blue=Y green=Select play=Start\n",(ULONG)!!LowLevelBase);}
static void fc_joy_close(void){printf("JOYSTICK_END polls=%u active=%u last_type=%08x\n",fc_joy_polls,fc_joy_nonzero,fc_joy_type);
 if(LowLevelBase){CloseLibrary(LowLevelBase);LowLevelBase=NULL;}}
