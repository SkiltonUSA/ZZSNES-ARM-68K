/* GPL-3.0-or-later. Native MUI development launcher; no ARM ownership here.
 * MUI patterns from xSysInfo / our NES launcher; see LICENSE-xSysInfo. */
#include <libraries/mui.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#pragma GCC diagnostic ignored "-Wpointer-sign"
struct Library *MUIMasterBase;
static Object *app,*win,*result;
static char appdir[512],message[640];
static const char version[]="$VER: ZZSnes-Mesen 0.2.0-dev (10.10.2026)";
static Object *line(const char *s){return TextObject,MUIA_Text_Contents,(ULONG)s,End;}
static int quote(char *out,size_t cap,const char *in){size_t n=0;
 if(cap<3)return 0;out[n++]='"';
 while(*in){unsigned char c=*in++;if(c<32||c==127)return 0;
  if(c=='*'||c=='"'||c=='$'||c=='`'){if(n+1>=cap)return 0;out[n++]='*';}
  if(n+1>=cap)return 0;out[n++]=c;}
 if(n+2>cap)return 0;out[n++]='"';out[n]=0;return 1;}
static void show(const char *s){snprintf(message,sizeof(message),"%s",s);set(result,MUIA_Text_Contents,(ULONG)message);}
static void run(int system){char path[512],command[1050],row[512];BPTR out;LONG rc;FILE *f;int pass=0,restore=0,ret=0;unsigned samples=0,total=0;
 if(FindPort("Sixies.ARM.Debug.Owner")){show("Another ARM application owns Core1. Close it first.");return;}
 strcpy(path,appdir);
 if(!AddPart(path,system?"ZZSnes-Mesen-System":"ZZSnes-Mesen-CPU",sizeof(path))||!quote(command,sizeof(command),path)){show("Application path is too long.");return;}
 out=Open(system?"PROGDIR:Last-System.log":"PROGDIR:Last-CPU.log",MODE_NEWFILE);if(!out){show("Cannot create a diagnostic log in this drawer.");return;}
 set(app,MUIA_Application_Sleep,TRUE);
 rc=SystemTags(command,SYS_Input,0,SYS_Output,out,SYS_Asynch,FALSE,NP_StackSize,65536,TAG_DONE);
 Close(out);set(app,MUIA_Application_Sleep,FALSE);
 f=fopen(system?"PROGDIR:Last-System.log":"PROGDIR:Last-CPU.log","r");
 if(f){while(fgets(row,sizeof(row),f)){
  char *p;
  if(strstr(row,system?"MESEN_SNES_SYSTEM PASS":"MESEN_SNES_CPU PASS"))pass=1;
  if(strstr(row,"MMU_RESTORE phase=3 register_mismatches=00000000")&&strstr(row,"witness_bad=0"))restore=1;
  if(strstr(row,"EXIT epilogue=52455431"))ret=1;
  if(strstr(row,"SAMPLE n=")&&strstr(row,"warmup=0")&&(p=strstr(row,system?"core_us=":"cpu_us="))){total+=(unsigned)strtoul(p+(system?8:7),NULL,10);samples++;}
 }fclose(f);}
 if(!rc&&pass&&restore&&ret&&samples==10){
  if(system)snprintf(message,sizeof(message),"System diagnostic passed; clean ARM return.\nSix-frame test: mean %u us per frame.\nHeadless graphics and silent audio; not game FPS.\nFull results: Last-System.log",total/samples/6);
  else snprintf(message,sizeof(message),"CPU diagnostic passed; clean ARM return.\n5,144 instructions per sample; mean %u us.\nCPU only: this is not game speed.\nFull results: Last-CPU.log",total/samples);
  set(result,MUIA_Text_Contents,(ULONG)message);
 }else show("Diagnostic did not pass all checks.\nSee Last-CPU.log or Last-System.log in this drawer.\nNo SNES game was started.");
}
int main(void){Object *start,*system,*close;struct MsgPort *owner=NULL;ULONG signals,id,open=0;int running=1,rc=20;
 if(!NameFromLock(GetProgramDir(),appdir,sizeof(appdir)))goto done;
 owner=CreateMsgPort();if(!owner)goto done;owner->mp_Node.ln_Name="ZZSNES_MESEN_UI";
 Forbid();if(FindPort(owner->mp_Node.ln_Name)){Permit();DeleteMsgPort(owner);owner=NULL;goto done;}AddPort(owner);Permit();
 MUIMasterBase=OpenLibrary("muimaster.library",15);if(!MUIMasterBase)goto done;
 app=ApplicationObject,MUIA_Application_Title,(ULONG)"ZZSnes-Mesen",MUIA_Application_Version,(ULONG)version,
  MUIA_Application_Base,(ULONG)"ZZSNES",MUIA_Application_Description,(ULONG)"Mesen2 SNES development on ZZ9000",
  SubWindow,win=WindowObject,MUIA_Window_Title,(ULONG)"ZZSnes-Mesen - Development",MUIA_Window_ID,0x534e4553,MUIA_Window_Width,560,
  WindowContents,VGroup,
   Child,line("\33bZZSnes-Mesen\33n   /   Super Nintendo"),
   Child,line("Separate SNES project - 0.2.0 development build\n\nReady: CPU, DMA, PPU graphics and SPC700/DSP test.\nAn original cartridge runs six frames on the Cortex-A9.\nDisplay, audible sound and game loading are next."),
   Child,line("Close other ZZ9000 ARM games before running this test."),
   Child,result=TextObject,TextFrame,MUIA_Text_Contents,(ULONG)"Ready. The test takes a few seconds.\nNo ROM file is required.\nThe result will appear here.\nDetailed results are saved beside this application.",End,
   Child,HGroup,Child,start=SimpleButton("CPU test"),Child,system=SimpleButton("System test"),Child,close=SimpleButton("Close"),End,
  End,End,End;
 if(!app)goto done;
 DoMethod(win,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,(ULONG)app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
 DoMethod(close,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
 DoMethod(start,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)app,2,MUIM_Application_ReturnID,1);
 DoMethod(system,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)app,2,MUIM_Application_ReturnID,2);
 set(win,MUIA_Window_Open,TRUE);get(win,MUIA_Window_Open,&open);if(!open)goto done;
 while(running){id=DoMethod(app,MUIM_Application_NewInput,(ULONG)&signals);
  if(id==(ULONG)MUIV_Application_ReturnID_Quit)running=0;else if(id==1||id==2)run(id==2);
  if(running&&signals&&(Wait(signals|SIGBREAKF_CTRL_C)&SIGBREAKF_CTRL_C))running=0;
 }rc=0;
done:
 if(app)MUI_DisposeObject(app);if(MUIMasterBase)CloseLibrary(MUIMasterBase);
 if(owner){RemPort(owner);DeleteMsgPort(owner);}return rc;
}
