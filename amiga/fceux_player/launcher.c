/* GPL-3.0-or-later. Native MUI 3.8 launcher for ZZNes ARM.
 * MUI construction/notification/event-loop patterns adapted from xSysInfo,
 * Copyright (c) 2025 xsysinfo contributors, BSD-2-Clause; see LICENSE-xSysInfo.
 * This process never owns Core1 or touches the display card. */
#include <exec/memory.h>
#include <devices/timer.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <libraries/lowlevel.h>
#include <dos/dostags.h>
#include <dos/datetime.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/lowlevel.h>
#include <clib/alib_protos.h>
#include <errno.h>
#include "ui_model.h"
#pragma GCC diagnostic ignored "-Wpointer-sign"
struct Library *MUIMasterBase,*AslBase,*LowLevelBase;
static Object *app,*win,*list,*filter,*selected_text,*status_text,*monitor,*theme,*mask,*vmask,*slot,*mute,*fire,*joytext,*statetext;
static struct ui_model model;
static char appdir[UI_PATH];
static int visible[UI_MAX],visible_count,selected=-1;
static char rows[UI_MAX][600],status[240],joy[400],stateinfo[1000];
static const char version[]="$VER: ZZNes Library 0.3.0-dev (10.10.2026)";
static const char *tabs[]={"Library","Display","Controllers","Save slots",NULL};
static const char *filters[]={"All ROMs","Recently played","Favourites",NULL};
static const char *monitors[]={"4:3 monitor","16:9 widescreen monitor",NULL};
static const char *themes[]={"Black bars","White background","Console grey / red","Cream / burgundy","Starfield wallpaper",NULL};
static const char *masks[]={"None (show all pixels)","8 NES pixels per edge","16 NES pixels per edge",NULL};
static const char *slots[]={"Slot 1","Slot 2","Slot 3","Slot 4","Slot 5",NULL};
static const char *fires[]={"Fire = NES A","Fire = NES B",NULL};
enum {ID_ADD=1,ID_SCAN,ID_SELECT,ID_FILTER,ID_FAV,ID_REMOVE,ID_PLAY,ID_RESUME,ID_SAVE,ID_SLOT};
static void notify(Object *o,ULONG attr,ULONG val,ULONG id){DoMethod(o,MUIM_Notify,attr,val,(ULONG)app,2,MUIM_Application_ReturnID,id);}
static ULONG value(Object *o,ULONG attr){ULONG n=0;get(o,attr,&n);return n;}
static Object *textline(const char *s){return TextObject,MUIA_Text_Contents,(ULONG)s,End;}
static Object *cycle(const char **items){return CycleObject,MUIA_Cycle_Entries,(ULONG)items,End;}
static void message(const char *s){snprintf(status,sizeof(status),"%s",s);set(status_text,MUIA_Text_Contents,(ULONG)status);}
static void collect(void){model.monitor=value(monitor,MUIA_Cycle_Active);model.theme=value(theme,MUIA_Cycle_Active);model.mask=value(mask,MUIA_Cycle_Active);model.vmask=value(vmask,MUIA_Cycle_Active);model.slot=value(slot,MUIA_Cycle_Active);model.mute=!!value(mute,MUIA_Selected);model.fire=value(fire,MUIA_Cycle_Active);}
static int prefs_write(void){FILE *f;int ok;BPTR lock;collect();f=fopen("PROGDIR:Library.tmp","w");if(!f)return 0;
 ok=ui_write(f,&model);if(fclose(f))ok=0;if(!ok)return 0;
 lock=Lock("PROGDIR:Library.cfg",ACCESS_READ);if(lock){UnLock(lock);DeleteFile("PROGDIR:Library.bak");if(!Rename("PROGDIR:Library.cfg","PROGDIR:Library.bak"))return 0;}
 if(!Rename("PROGDIR:Library.tmp","PROGDIR:Library.cfg")){Rename("PROGDIR:Library.bak","PROGDIR:Library.cfg");return 0;}return 1;}
static void slotpath(char *p,unsigned slotnum){snprintf(p,UI_PATH,"PROGDIR:States/%08x-%u.zzs",(ULONG)ui_hash(model.entry[selected].path),slotnum+1);}
static void update_slots(void){unsigned i;char p[UI_PATH],line[160];struct FileInfoBlock *fib;
 strcpy(stateinfo,"Select a ROM in Library to inspect its save slots.");
 if(selected>=0){strcpy(stateinfo,"Save states for the selected ROM:\n\n");fib=AllocDosObject(DOS_FIB,NULL);
  for(i=0;i<5;i++){BPTR lock;slotpath(p,i);lock=Lock(p,ACCESS_READ);
   if(lock&&fib&&Examine(lock,fib)){struct DateTime dt;char date[32],time[32];memset(&dt,0,sizeof(dt));dt.dat_Stamp=fib->fib_Date;dt.dat_Format=FORMAT_DOS;dt.dat_StrDate=date;dt.dat_StrTime=time;DateToStr(&dt);
    snprintf(line,sizeof(line),"Slot %u   Saved %s %s   (%d bytes)\n",i+1,date,time,fib->fib_Size);
   }else snprintf(line,sizeof(line),"Slot %u   Empty\n",i+1);
   if(lock)UnLock(lock);strcat(stateinfo,line);
  }if(fib)FreeDosObject(DOS_FIB,fib);
 }set(statetext,MUIA_Text_Contents,(ULONG)stateinfo);
}
static void selection(void){LONG n=(LONG)value(list,MUIA_List_Active);selected=n>=0&&n<visible_count?visible[n]:-1;
 set(selected_text,MUIA_Text_Contents,(ULONG)(selected>=0?model.entry[selected].path:"Select a game, or add a .nes file."));update_slots();}
static void refresh(void){unsigned i,j,f=value(filter,MUIA_Cycle_Active);int old=selected,active=-1;visible_count=0;
 DoMethod(list,MUIM_List_Clear);
 for(i=0;i<model.count;i++)if((f!=2||model.entry[i].favorite)&&(f!=1||model.entry[i].played))visible[visible_count++]=i;
 if(f==1)for(i=0;i<(unsigned)visible_count;i++)for(j=i+1;j<(unsigned)visible_count;j++)if(model.entry[visible[j]].played>model.entry[visible[i]].played){int t=visible[i];visible[i]=visible[j];visible[j]=t;}
 for(i=0;i<(unsigned)visible_count;i++){int n=visible[i];const char *name=FilePart(model.entry[n].path);
  snprintf(rows[i],sizeof(rows[i]),"%s  %s",model.entry[n].favorite?"*":" ",name);
  DoMethod(list,MUIM_List_InsertSingle,(ULONG)rows[i],MUIV_List_Insert_Bottom);if(n==old)active=i;
 }set(list,MUIA_List_Active,active);selection();}
static void browse(int drawer){struct FileRequester *r;char p[UI_PATH];unsigned before=model.count;
 set(app,MUIA_Application_Sleep,TRUE);
 r=AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)(drawer?"Add a folder of NES cartridges":"Add a NES cartridge"),ASLFR_InitialDrawer,(ULONG)"PROGDIR:ROMs",ASLFR_InitialPattern,(ULONG)"#?.nes",ASLFR_DoPatterns,TRUE,ASLFR_DrawersOnly,drawer,TAG_DONE);
 if(r){if(AslRequestTags(r,TAG_DONE)&&strlen(r->fr_Drawer)<sizeof(p)){
   strcpy(p,r->fr_Drawer);
   if(drawer){BPTR lock=Lock(p,ACCESS_READ);struct FileInfoBlock *fib=AllocDosObject(DOS_FIB,NULL);
    if(lock&&fib&&Examine(lock,fib))while(ExNext(lock,fib)){unsigned n=strlen(fib->fib_FileName);char full[UI_PATH];
     if(fib->fib_DirEntryType<0&&n>4&&!strcasecmp(fib->fib_FileName+n-4,".nes")){strcpy(full,p);if(AddPart(full,fib->fib_FileName,sizeof(full)))ui_add(&model,full);}}
    if(lock)UnLock(lock);if(fib)FreeDosObject(DOS_FIB,fib);
   }else if(AddPart(p,r->fr_File,sizeof(p)))selected=ui_add(&model,p);
  }FreeAslRequest(r);}
 set(app,MUIA_Application_Sleep,FALSE);refresh();
 if(model.count>before){if(!prefs_write())message("Added to this session; preferences could not be saved.");else message("ROM library saved. Select a game and press Play.");}
}
static int session_write(int resume){FILE *f;char p[UI_PATH];int ok;
 f=fopen("PROGDIR:Session.cfg","w");if(!f)return 0;
 fprintf(f,"ZZSESSION1\n%u %u %u %u\n",model.monitor|(model.theme<<1)|(model.mask<<4)|(model.vmask<<6),model.mute,model.fire,resume);
 ui_hex(f,model.entry[selected].path);fputc('\n',f);slotpath(p,model.slot);ui_hex(f,p);fputc('\n',f);ok=!ferror(f);if(fclose(f))ok=0;return ok;}
static void play(int resume){char path[UI_PATH],session[UI_PATH],qplayer[UI_PATH*2+4],qsession[UI_PATH*2+4],command[UI_PATH*4+40];LONG rc;BPTR out,dir;
 if(selected<0){message("Select a ROM first.");return;}collect();
 if(resume){BPTR lock;slotpath(path,model.slot);lock=Lock(path,ACCESS_READ);if(!lock){message("That slot is empty. Play the game and use F5 to save.");return;}UnLock(lock);}
 dir=CreateDir("PROGDIR:States");if(dir)UnLock(dir);
 if(!session_write(resume)){message("Cannot write launch settings. Check that the drawer is writable.");return;}
 model.entry[selected].played=++model.clock;if(!prefs_write()){message("Cannot save library settings. Check drawer permissions.");return;}
 strcpy(path,appdir);strcpy(session,appdir);
 if(!AddPart(path,"ZZFceux-ui",sizeof(path))||!AddPart(session,"Session.cfg",sizeof(session))||
    !ui_quote(qplayer,sizeof(qplayer),path)||!ui_quote(qsession,sizeof(qsession),session)){message("Application path is too long.");return;}
 snprintf(command,sizeof(command),"%s --session %s",qplayer,qsession);
 /* Synchronous child: hidden MUI window, no concurrent joystick polling. */
 set(win,MUIA_Window_Open,FALSE);
 out=Open("PROGDIR:Last-session.log",MODE_NEWFILE);
 rc=SystemTags(command,SYS_Input,0,SYS_Output,out,SYS_Asynch,FALSE,NP_StackSize,65536,TAG_DONE);
 if(out)Close(out);
 set(win,MUIA_Window_Open,TRUE);refresh();
 message(rc==0?"Returned from game. F5 saves / F8 loads the chosen slot.":"Player could not complete the session. See Last-session.log.");
}
static void joystick(void){ULONG s=LowLevelBase?ReadJoyPort(1):0,type=s&JP_TYPE_MASK;
 const char *kind=!LowLevelBase?"lowlevel.library unavailable":type==JP_TYPE_JOYSTK?"Standard joystick":type==JP_TYPE_GAMECTLR?"Game controller":type==JP_TYPE_MOUSE?"Mouse detected":"Move the stick or press fire to detect it";
 snprintf(joy,sizeof(joy),"Physical port 2 (lowlevel port 1)\n%s\n\nUp: %s    Down: %s    Left: %s    Right: %s\n\nFire: %s    Second button: %s\n\nRaw input: %08x",kind,s&JPF_JOY_UP?"ON":"--",s&JPF_JOY_DOWN?"ON":"--",s&JPF_JOY_LEFT?"ON":"--",s&JPF_JOY_RIGHT?"ON":"--",s&JPF_BUTTON_RED?"ON":"--",s&JPF_BUTTON_BLUE?"ON":"--",s);
 set(joytext,MUIA_Text_Contents,(ULONG)joy);
}
static int build(void){Object *add,*scan,*fav,*remove,*start,*resume,*save,*exit,*listview;
 app=ApplicationObject,MUIA_Application_Title,(ULONG)"ZZNes Library",MUIA_Application_Version,(ULONG)version,
 MUIA_Application_Base,(ULONG)"ZZNES",MUIA_Application_Description,(ULONG)"NES games on the ZZ9000 Cortex-A9",
 SubWindow,win=WindowObject,MUIA_Window_Title,(ULONG)"ZZNes - ROM Library",MUIA_Window_ID,0x4e455355,MUIA_Window_Width,680,
 WindowContents,VGroup,
  Child,textline("\33bZZNes ARM\33n   /   Nintendo Entertainment System"),
  Child,RegisterGroup(tabs),MUIA_Register_Frame,TRUE,
   Child,VGroup,
    Child,filter=cycle(filters),
    Child,listview=ListviewObject,MUIA_Listview_List,list=ListObject,InputListFrame,MUIA_List_ConstructHook,MUIV_List_ConstructHook_String,MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,End,
    Child,selected_text=TextObject,TextFrame,MUIA_Text_Contents,(ULONG)"Select a game, or add a .nes file.",End,
    Child,HGroup,Child,add=SimpleButton("Add ROM..."),Child,scan=SimpleButton("Add folder..."),Child,fav=SimpleButton("Toggle favourite"),Child,remove=SimpleButton("Remove from list"),End,
   End,
   Child,VGroup,
    Child,textline("Monitor shape (set the monitor itself to fill the screen):"),Child,monitor=cycle(monitors),
    Child,textline("Border / wallpaper:"),Child,theme=cycle(themes),
    Child,textline("Left / right edge mask (hide flashing pixels):"),Child,mask=cycle(masks),
    Child,textline("Top / bottom edge mask:"),Child,vmask=cycle(masks),
    Child,textline("The game keeps a 4:3 shape. Widescreen adds side panels.\nBlack, white and three original console-inspired designs.\nNearest-neighbour scaling; no smoothing or frameskip."),
    Child,HGroup,Child,mute=CheckMark(FALSE),Child,textline("Start muted (M toggles sound while playing)"),End,
    Child,VSpace(0),
   End,
   Child,VGroup,Child,joytext=TextObject,TextFrame,MUIA_Text_Contents,(ULONG)"Joystick test",End,
    Child,fire=cycle(fires),
    Child,textline("A C64 stick has one fire button. Use the keyboard for the other.\n\nArrows = directions   Z = A   X = B\nEnter = Start   Left Shift = Select\nP = pause   M = mute   Q / Escape = return to Library"),Child,VSpace(0),End,
   Child,VGroup,Child,slot=cycle(slots),Child,statetext=TextObject,TextFrame,MUIA_Text_Contents,(ULONG)"Select a ROM in Library.",End,
    Child,textline("F5 saves the selected slot during play; F8 restores it.\nResume slot starts the selected ROM from that save.\nFive slots per ROM; previous saves kept as .bak files.\nStates belong to this player version, not desktop Mesen/FCEUX."),Child,VSpace(0),End,
  End,
  Child,status_text=TextObject,TextFrame,MUIA_Text_Contents,(ULONG)"Ready. Add a ROM or a ROM folder.",End,
  Child,HGroup,Child,start=SimpleButton("Play"),Child,resume=SimpleButton("Resume slot"),Child,save=SimpleButton("Save settings"),Child,exit=SimpleButton("Quit"),End,
 End,End,End;
 if(!app)return 0;
 notify(win,MUIA_Window_CloseRequest,TRUE,MUIV_Application_ReturnID_Quit);notify(exit,MUIA_Pressed,FALSE,MUIV_Application_ReturnID_Quit);
 notify(add,MUIA_Pressed,FALSE,ID_ADD);notify(scan,MUIA_Pressed,FALSE,ID_SCAN);notify(fav,MUIA_Pressed,FALSE,ID_FAV);notify(remove,MUIA_Pressed,FALSE,ID_REMOVE);
 notify(start,MUIA_Pressed,FALSE,ID_PLAY);notify(resume,MUIA_Pressed,FALSE,ID_RESUME);notify(save,MUIA_Pressed,FALSE,ID_SAVE);
 notify(list,MUIA_List_Active,MUIV_EveryTime,ID_SELECT);notify(filter,MUIA_Cycle_Active,MUIV_EveryTime,ID_FILTER);notify(slot,MUIA_Cycle_Active,MUIV_EveryTime,ID_SLOT);
 notify(listview,MUIA_Listview_DoubleClick,TRUE,ID_PLAY);
 set(monitor,MUIA_Cycle_Active,model.monitor);set(theme,MUIA_Cycle_Active,model.theme);set(mask,MUIA_Cycle_Active,model.mask);set(vmask,MUIA_Cycle_Active,model.vmask);set(slot,MUIA_Cycle_Active,model.slot);set(mute,MUIA_Selected,model.mute);set(fire,MUIA_Cycle_Active,model.fire);
 return 1;
}
int main(void){struct MsgPort *port=NULL,*owner=NULL;struct timerequest *timer=NULL;FILE *f;ULONG signals=0,id;int running=1,opened=0,pending=0,rc=20;
 if(!NameFromLock(GetProgramDir(),appdir,sizeof(appdir)))goto done;
 owner=CreateMsgPort();if(!owner)goto done;owner->mp_Node.ln_Name="ZZNES_LIBRARY";
 Forbid();if(FindPort(owner->mp_Node.ln_Name)){Permit();DeleteMsgPort(owner);owner=NULL;goto done;}AddPort(owner);Permit();
 ui_defaults(&model);f=fopen("PROGDIR:Library.cfg","r");if(f){if(!ui_read(f,&model))ui_defaults(&model);fclose(f);}
 MUIMasterBase=OpenLibrary("muimaster.library",15);AslBase=OpenLibrary("asl.library",38);LowLevelBase=OpenLibrary("lowlevel.library",40);
 if(!MUIMasterBase||!AslBase)goto done;
 if(!build())goto done;refresh();set(win,MUIA_Window_Open,TRUE);if(!value(win,MUIA_Window_Open))goto done;
 port=CreateMsgPort();if(!port)goto done;timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));if(!timer)goto done;
 if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0))goto done;opened=1;
 while(running){
  id=DoMethod(app,MUIM_Application_NewInput,(ULONG)&signals);
  switch(id){case (ULONG)MUIV_Application_ReturnID_Quit:running=0;break;
   case ID_ADD:browse(0);break;case ID_SCAN:browse(1);break;case ID_SELECT:selection();break;case ID_FILTER:refresh();break;
   case ID_FAV:if(selected>=0){model.entry[selected].favorite^=1;refresh();if(!prefs_write())message("Could not save favourites.");}break;
   case ID_REMOVE:if(selected>=0){memmove(&model.entry[selected],&model.entry[selected+1],(--model.count-selected)*sizeof(model.entry[0]));selected=-1;refresh();if(!prefs_write())message("Could not save library.");else message("Removed from the list. The ROM file remains on disk.");}break;
   case ID_PLAY:play(0);break;case ID_RESUME:play(1);break;case ID_SLOT:update_slots();break;
   case ID_SAVE:message(prefs_write()?"Settings saved.":"Could not save settings.");break;}
  if(!running)break;
  if(pending&&CheckIO((struct IORequest *)timer)){WaitIO((struct IORequest *)timer);pending=0;}
  if(!pending){joystick();timer->tr_node.io_Command=TR_ADDREQUEST;timer->tr_time.tv_secs=0;timer->tr_time.tv_micro=50000;SendIO((struct IORequest *)timer);pending=1;}
  if(!id){ULONG got=Wait(signals|(1UL<<port->mp_SigBit)|SIGBREAKF_CTRL_C);if(got&SIGBREAKF_CTRL_C)running=0;}
 }
 rc=prefs_write()?0:20;
done:
 if(pending){AbortIO((struct IORequest *)timer);WaitIO((struct IORequest *)timer);}if(opened)CloseDevice((struct IORequest *)timer);if(timer)DeleteIORequest((struct IORequest *)timer);if(port)DeleteMsgPort(port);
 if(app)MUI_DisposeObject(app);if(LowLevelBase)CloseLibrary(LowLevelBase);if(AslBase)CloseLibrary(AslBase);if(MUIMasterBase)CloseLibrary(MUIMasterBase);
 if(owner){RemPort(owner);DeleteMsgPort(owner);}return rc;
}
