"""SN30 USB and passive-adapter Mega Drive input, shared by tester/player."""
from pathlib import Path
import shutil
from prepare_audio_window import edit
SOURCE=Path(__file__).resolve().parents[1]/'development/controllers-20261030'

def headers(dest):
 for name in ('pad_decode.h','pad_amiga.h'):shutil.copy2(SOURCE/name,dest/name)

def prepare_launcher(lib):
 headers(lib)
 p=lib/'library_mui.h'
 edit(p,'static Object *audio_output;', '''static Object *audio_output;
#include "pad_amiga.h"
static Object *pad_source;
static unsigned pad_choice,pad_preview,pad_testing,pad_usb_available;
static const char *pad_choices[]={"Auto: USB / Mega Drive","USB: SN30 Pro","Mega Drive 6","Amiga / CD32",NULL};''')
 edit(p,'if(!em_audio_save(value(audio_output,MUIA_Cycle_Active)))return 0;',
      'if(!pad_write_config(value(pad_source,MUIA_Cycle_Active)))return 0;if(!em_audio_save(value(audio_output,MUIA_Cycle_Active)))return 0;')
 edit(p,'static unsigned controller_mapped_bits(ULONG s){','static unsigned controller_mapped_bits(ULONG s){\n if(value(pad_source,MUIA_Cycle_Active)!=3)return pad_preview;')
 edit(p,'static const char *controller_binding(unsigned button){','''static const char *controller_binding(unsigned button){
 if(value(pad_source,MUIA_Cycle_Active)!=3){
  static const char *usb[]={"B","Y","Select","Start","D-pad","D-pad","D-pad","D-pad","A","X","L","R"};
  static const char *md[]={"B","A","Mode","Start","D-pad","D-pad","D-pad","D-pad","C","X","Y","Z"};
  return (pad_binding||value(pad_source,MUIA_Cycle_Active)==1)?usb[button]:md[button];
 }''')
 edit(p,'static unsigned controller_binding_hash(void){unsigned h=0;',
      'static unsigned controller_binding_hash(void){unsigned h=value(pad_source,MUIA_Cycle_Active)+(pad_binding?8:0);')
 start=p.read_text().index('static void joystick(void)');end=p.read_text().index('\n}',start)+2
 s=p.read_text();s=s[:start]+'''static void joystick(void){
 unsigned source=value(pad_source,MUIA_Cycle_Active),visible=value(pages,MUIA_Group_ActivePage)==2;
 ULONG s=0;int blocked=0;
 Forbid();blocked=FindPort("ZZUAE.1")||FindPort("ZZNES.1")||FindPort("ZZDOSBOX.1")||FindPort("Sixies.ARM.Debug.Owner");Permit();
 for(unsigned i=0;i<7;i++)set(joymap[0][i],MUIA_Disabled,source!=3);
 if(!visible||blocked||source==3){if(pad_testing){pad_close();pad_testing=0;}pad_preview=0;}
 if(visible&&!blocked&&source!=3){
  if(!pad_testing||pad_mode!=source){pad_open(source);pad_testing=1;}
  ULONG sec,micro;CurrentTime(&sec,&micro);pad_preview=pad_poll(sec*1000000u+micro);
  snprintf(joy,sizeof(joy),"%s",pad_status);
 }else if(blocked)snprintf(joy,sizeof(joy),"Close the other emulator to test controllers");
 else if(source==3){s=LowLevelBase?ReadJoyPort(1):0;
  snprintf(joy,sizeof(joy),"Amiga / CD32 port 2: %s",!LowLevelBase?"lowlevel.library unavailable":(s&JP_TYPE_MASK)==JP_TYPE_GAMECTLR?"gamepad":(s&JP_TYPE_MASK)==JP_TYPE_JOYSTK?"joystick":"waiting for input");
 }else snprintf(joy,sizeof(joy),"%s",pad_usb_available?"USB ready; Mega Drive fallback":"USB unavailable; Mega Drive fallback");
 set(joytext,MUIA_Text_Contents,(ULONG)joy);
 controller_update(s,0,visible);
 if(source==3&&!blocked&&value(win,MUIA_Window_Activate)&&value(pages,MUIA_Group_ActivePage)==0){
  ULONG type=s&JP_TYPE_MASK,edge=s&~previous_joy;LONG active=(LONG)value(list,MUIA_List_Active);
  if(type==JP_TYPE_JOYSTK||type==JP_TYPE_GAMECTLR){
   if((edge&JPF_JOY_UP)&&active>0)set(list,MUIA_List_Active,active-1);
   else if((edge&JPF_JOY_DOWN)&&active+1<visible_count)set(list,MUIA_List_Active,active+1);
   if((edge&JPF_BUTTON_RED)&&rom_valid)set(win,MUIA_Window_ActiveObject,(ULONG)start);
  }
 }previous_joy=s;
}'''+s[end:];p.write_text(s)
 edit(p,'Child,joytext=textline("Move the joystick or press Fire on physical port 2"),', '''Child,HGroup,Child,textline("Player 1"),Child,pad_source=cycle(pad_choices),End,
    Child,joytext=textline("Detecting controller support..."),''')
 edit(p,'Child,mapping_grid(),','''Child,textline("SN30 USB: B+Start, then connect cable. Mega Drive: 5/7 adapter."),
    Child,mapping_grid(),''')
 edit(p,'set(audio_output,MUIA_Cycle_Active,em_audio_load());','''if(!pad_read_config(&pad_choice)){puts("Invalid Controller.cfg; no settings changed");return 0;}
 set(pad_source,MUIA_Cycle_Active,pad_choice);
 {struct Library*usb=OpenLibrary("poseidon.library",4);pad_usb_available=usb!=NULL;if(usb)CloseLibrary(usb);}
 set(pad_source,MUIA_ShortHelp,(ULONG)"Auto prefers SN30 Pro USB (D-input), then a Mega Drive six-button pad with a passive 5/7 adapter.\\nCD32 converters use Amiga/CD32 mapping. Start/Mode map to Start/Select.\\nUSB requires Poseidon 4+. No Bluetooth dongle or X-input support.");
 set(audio_output,MUIA_Cycle_Active,em_audio_load());''')
 edit(p,'splash_close();scan_stop();','pad_close();splash_close();scan_stop();')
 p=lib/'library_launch.h'
 edit(p,'loading_window=splash_create(1);set(win,MUIA_Window_Open,FALSE);',
      'pad_close();pad_testing=0;loading_window=splash_create(1);set(win,MUIA_Window_Open,FALSE);')

def prepare_player(stage):
 headers(stage);p=stage/'joystick.h'
 edit(p,'struct Library *LowLevelBase;','struct Library *LowLevelBase;\n#include "pad_amiga.h"')
 edit(p,'b=fc_joy_decode(state,0);','b=pad_mode==3?fc_joy_decode(state,0):pad_poll(us());')
 # Only the legacy route may clock lowlevel port 2 while we own the Sega pins.
 edit(p,'ULONG state=ReadJoyPort(1);','ULONG state=pad_mode==3?ReadJoyPort(1):0;')
 edit(p,'if(LowLevelBase){ULONG state=', 'if(LowLevelBase||pad_mode!=3){ULONG state=')
 edit(p,'if(fc_controls.second==2)', 'if(LowLevelBase&&fc_controls.second==2)')
 edit(p,'if(fc_fire){','if(pad_mode==3&&fc_fire){')
 edit(p,'static void fc_joy_open(void){', '''static void fc_joy_open(void){unsigned mode=0;
 if(!pad_read_config(&mode)){puts("Invalid Controller.cfg; using Amiga/CD32");mode=3;}
 pad_open(mode);printf("CONTROLLER mode=%u USB=%u\\n",mode,(unsigned)!!PsdBase);''')
 edit(p,'static void fc_joy_close(void){','static void fc_joy_close(void){pad_close();')
