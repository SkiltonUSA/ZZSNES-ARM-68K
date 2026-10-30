/* Exercise the generated production player with mocked Amiga input APIs. */
#define main transport_tests
#include "transport.c"
#undef main
static const ULONG JP_TYPE_MASK=0xf0000000,JP_TYPE_GAMECTLR=0x10000000,JP_TYPE_JOYSTK=0x20000000;
#define JPF_BUTTON_RED (1ul<<0)
#define JPF_BUTTON_BLUE (1ul<<1)
#define JPF_BUTTON_GREEN (1ul<<2)
#define JPF_BUTTON_YELLOW (1ul<<3)
#define JPF_BUTTON_FORWARD (1ul<<4)
#define JPF_BUTTON_REVERSE (1ul<<5)
#define JPF_BUTTON_PLAY (1ul<<6)
#define JPF_JOY_UP (1ul<<7)
#define JPF_JOY_DOWN (1ul<<8)
#define JPF_JOY_LEFT (1ul<<9)
#define JPF_JOY_RIGHT (1ul<<10)
#include "input_config.h"
static struct ps_controls fc_controls;
static unsigned fc_fire,buttons,reads[2];
static ULONG states[2];
static ULONG ReadJoyPort(unsigned port){assert(port<2);reads[port]++;return states[port];}
static ULONG us(void){return 1;}
#include "joystick.h"
int main(void){
 const ULONG flags[]={JPF_BUTTON_RED,JPF_BUTTON_BLUE,JPF_BUTTON_GREEN,JPF_BUTTON_YELLOW,JPF_BUTTON_FORWARD,JPF_BUTTON_REVERSE,JPF_BUTTON_PLAY};
 /* Same lowlevel input path for USB mapping and physical CD32. */
 ps_controls_default(&fc_controls);
 LowLevelBase=&library;
 for(unsigned mode=3;mode<=4;mode++){
  assert(pad_open(mode));
  for(unsigned i=0;i<7;i++){
   states[1]=JP_TYPE_GAMECTLR|flags[i];
   assert(fc_read_buttons()==(1u<<fc_controls.joy[0][i]));
  }
  const ULONG dirs[]={JPF_JOY_UP,JPF_JOY_DOWN,JPF_JOY_LEFT,JPF_JOY_RIGHT};
  for(unsigned i=0;i<4;i++){states[1]=JP_TYPE_GAMECTLR|dirs[i];assert(fc_read_buttons()==(16u<<i));}
  states[1]=JP_TYPE_JOYSTK|JPF_BUTTON_RED;assert(fc_read_buttons()==1);
  states[1]=JP_TYPE_JOYSTK|JPF_BUTTON_PLAY;assert(fc_read_buttons()==0);
  states[1]=0;buttons=8;assert(fc_read_buttons()==8);buttons=0;
  states[1]=0x30000000|JPF_BUTTON_RED;assert(fc_read_buttons()==0);
  assert(reads[0]==0); /* Never read the mouse port by default. */
  assert(!PsdBase&&!pad_binding&&!pad_pot&&claims==0);
 }
 fc_controls.joy[0][0]=8;states[1]=JP_TYPE_GAMECTLR|JPF_BUTTON_RED;
 assert(fc_read_buttons()==256); /* Custom SNES A mapping. */
 LowLevelBase=NULL;assert(fc_read_buttons()==0);pad_close();
 puts("PASS player: USB/CD32 and native CD32 mappings, directions, two-button pads, keyboard, no mouse-port reads or USB claims");
}
