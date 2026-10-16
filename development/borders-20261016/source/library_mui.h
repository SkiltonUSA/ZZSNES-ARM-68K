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
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/lowlevel.h>
#include <clib/alib_protos.h>
#include <errno.h>
#ifdef LIBRARY_SNES
#define UI_CONFIG_PREFIX "ZZSNESUI"
#endif
#include "ui_model.h"
#include "cartridge.h"
#ifdef LIBRARY_SNES
#include "cartridge_warning.h"
#endif
#include "library_preview.h"
#include "library_saves.h"
#include "library_controller.h"
#pragma GCC diagnostic ignored "-Wpointer-sign"
#include "library_style.h"
#include "zip/rom_archive.h"
#include "library_keys.h"
#ifdef LIBRARY_SNES
#define LIB_TITLE "ZZSNES"
#define LIB_PORT "ZZSNES_LIBRARY"
#define LIB_PLAYER "ZZSnes-ui"
#define LIB_SESSION "ZZSNESSESSION2"
#define LIB_VERSION "0.6.1 Beta"
#define LIB_PATTERN "#?.(sfc|smc|zip)"
#define LIB_SNES 1
#define LIB_SYSTEM "Super Nintendo Entertainment System"
#define LIB_LIMIT 0x400200u
#else
#define LIB_TITLE "ZZNES"
#define LIB_PORT "ZZNES_LIBRARY"
#define LIB_PLAYER "ZZFceux-ui"
#define LIB_SESSION "ZZSESSION1"
#define LIB_VERSION "0.7.1 Beta"
#define LIB_PATTERN "#?.(nes|zip)"
#define LIB_SNES 0
#define LIB_SYSTEM "Nintendo Entertainment System"
#define LIB_LIMIT 0xc0010u
#endif
#ifdef LIBRARY_PREVIEW
#undef LIB_PORT
#ifdef LIBRARY_SNES
#define LIB_PORT "ZZSNES_LIBRARY_PREVIEW"
#else
#define LIB_PORT "ZZNES_LIBRARY_PREVIEW"
#endif
#define LIB_WINDOW " - " LIB_VERSION
#else
#define LIB_WINDOW " - " LIB_VERSION
#endif
#pragma GCC diagnostic ignored "-Wpointer-sign"
struct Library *MUIMasterBase,*AslBase,*LowLevelBase;
static Object *app,*win,*list,*pages,*search,*title_edit,*selected_text,*status_text,*monitor,*theme,*mask,*vmask,*slot,*mute,*fire,*joytext;
static Object *start,*resume,*latest,*fav,*remove_button,*rename_button,*art_button,*clear_art,*hero,*summary,*count_text,*save_title;
static Object *scan_drives,*change_list,*navigation,*heading,*display_mode,*scanline_mode,*fps_option,*optimise_option,*autoscan_option;
static Object *cards[5],*card_text[5],*card_resume[5],*card_delete[5];
static char card_labels[5][180],details[400],summary_line[180],count_line[80];
static char save_heading[UI_TITLE+24],selected_path[UI_PATH];
static int present_slots[5],valid_slots[5],latest_slot=-1,rom_valid,selection_busy,needs_details,legacy_config;
static uint32_t rom_id,rom_id2,rom_bytes;
static ULONG previous_joy;
static struct ui_model model;
static char appdir[UI_PATH];
static int *visible,visible_count,selected=-1;
static unsigned visible_capacity,search_ticks;
static char search_seen[UI_TITLE];
static char status[240],joy[400];
static const char version[]="$VER: " LIB_TITLE " Library " LIB_VERSION " (16.10.2026)";
static const char *tabs[]={"Games","Display","Controllers","Save states","About",NULL};
static const char *display_choices[]={"Fullscreen","Workbench window",NULL};
static const char *scanline_choices[]={"Off","Classic","Soft","Gradient",NULL};
static const char *filters[]={"All ROMs","Recently played","Favourites",NULL};
static const char *monitors[]={"4:3 monitor","16:9 widescreen monitor",NULL};
#ifdef LIBRARY_SNES
static const char *themes[]={"Black bars","White background","SNES console / purple","Super Famicom / four colours","SNES console / charcoal","Nintendo artwork","Super Nintendo artwork","EmuELEC bezel",NULL};
#else
static const char *themes[]={"Black bars","White background","Console grey / red","Cream / burgundy","Starfield wallpaper","Nintendo artwork","Super Nintendo artwork","EmuELEC bezel",NULL};
#endif
static const char *masks[]={"None (show all pixels)","8 pixels per edge","16 pixels per edge",NULL};
static const char *slots[]={"Slot 1","Slot 2","Slot 3","Slot 4","Slot 5",NULL};
#ifdef LIBRARY_SNES
static const char *fires[]={"Fire = SNES B","Fire = SNES Y",NULL};
#else
static const char *fires[]={"Fire = NES A","Fire = NES B",NULL};
#endif
enum {ID_ADD=1,ID_SCAN,ID_SELECT,ID_FILTER,ID_FAV,ID_REMOVE,ID_PLAY,ID_RESUME,ID_SAVE,ID_SLOT,ID_LATEST,ID_SAVES,ID_RENAME,ID_ART,ID_CLEAR_ART,ID_SEARCH,ID_ALL,ID_RECENT,ID_FAVOURITES,ID_NAV=50,ID_SCAN_DRIVES,ID_SLOT_RESUME=110,ID_SLOT_DELETE=120};
static void notify(Object *o,ULONG attr,ULONG val,ULONG id){DoMethod(o,MUIM_Notify,attr,val,(ULONG)app,2,MUIM_Application_ReturnID,id);}
static ULONG value(Object *o,ULONG attr){ULONG n=0;get(o,attr,&n);return n;}
/* MUI 3.8 does not relayout a Text when its content grows to more lines.
 * Reserve a font-relative height independent of current dynamic contents. */
static struct MUI_CustomClass *multiline_class;
static ULONG multiline_dispatch(struct IClass*cl __asm("a0"),Object*obj __asm("a2"),Msg msg __asm("a1")){
 ULONG result=DoSuperMethodA(cl,obj,msg);
 if(msg->MethodID==MUIM_AskMinMax){struct MUIP_AskMinMax*m=(struct MUIP_AskMinMax*)msg;
  ULONG lines=value(obj,MUIA_UserData);LONG height=(LONG)lines*_font(obj)->tf_YSize+4;
  if(m->MinMaxInfo->MinHeight<height)m->MinMaxInfo->MinHeight=height;
  m->MinMaxInfo->DefHeight=m->MinMaxInfo->MaxHeight=m->MinMaxInfo->MinHeight;
 }return result;
}
static Object *multiline(const char*s,unsigned lines){return NewObject(multiline_class->mcc_Class,NULL,MUIA_Font,ui_font(),MUIA_UserData,lines,MUIA_Text_Contents,(ULONG)s,TAG_DONE);}
static Object *textline(const char *s){return TextObject,MUIA_Font,ui_font(),MUIA_Text_Contents,(ULONG)s,End;}
static Object *cycle(const char **items){return CycleObject,MUIA_Font,ui_font(),MUIA_Cycle_Entries,(ULONG)items,End;}
static void message(const char *s){snprintf(status,sizeof(status),"%s",s);set(status_text,MUIA_Text_Contents,(ULONG)status);}
static void collect(void){if(selected>=0)model.last=selected+1;model.monitor=value(monitor,MUIA_Cycle_Active);model.theme=value(theme,MUIA_Cycle_Active);model.mask=value(mask,MUIA_Cycle_Active);model.vmask=value(vmask,MUIA_Cycle_Active);model.slot=value(slot,MUIA_Cycle_Active);model.mute=!!value(mute,MUIA_Selected);model.fire=value(fire,MUIA_Cycle_Active);model.windowed=value(display_mode,MUIA_Cycle_Active);model.scanlines=value(scanline_mode,MUIA_Cycle_Active);model.fps=!!value(fps_option,MUIA_Selected);model.optimise=!!value(optimise_option,MUIA_Selected);model.autoscan=!!value(autoscan_option,MUIA_Selected);}
static int preserve_legacy(void){FILE*src,*dst;char buf[1024];size_t n;int ok=1;BPTR lock;
 if(!legacy_config)return 1;lock=Lock("PROGDIR:Library.pre-browser",ACCESS_READ);if(lock){UnLock(lock);legacy_config=0;return 1;}
 src=fopen("PROGDIR:Library.cfg","rb");if(!src)return 0;dst=fopen("PROGDIR:Library.pre-browser","wb");if(!dst){fclose(src);return 0;}
 while((n=fread(buf,1,sizeof(buf),src)))if(fwrite(buf,1,n,dst)!=n){ok=0;break;}
 if(ferror(src))ok=0;if(fclose(src))ok=0;if(fclose(dst))ok=0;
 if(!ok)DeleteFile("PROGDIR:Library.pre-browser");else legacy_config=0;return ok;
}
static int prefs_write(void){FILE *f;int ok;BPTR lock;if(!preserve_legacy())return 0;collect();f=fopen("PROGDIR:Library.tmp","w");if(!f)return 0;
 ok=ui_write(f,&model);if(fclose(f))ok=0;if(!ok)return 0;
 lock=Lock("PROGDIR:Library.cfg",ACCESS_READ);if(lock){UnLock(lock);DeleteFile("PROGDIR:Library.bak");if(!Rename("PROGDIR:Library.cfg","PROGDIR:Library.bak"))return 0;}
 if(!Rename("PROGDIR:Library.tmp","PROGDIR:Library.cfg")){Rename("PROGDIR:Library.bak","PROGDIR:Library.cfg");return 0;}return 1;}
static int ui_extension(const char*p){return em_rom_zip(p)||em_rom_extension(p,LIB_SNES);}
/* 1 raw ROM, 2 matching single-ROM ZIP, 0 rejected. Only reads ZIP directory. */
static int library_candidate(const char*p){
 if(em_rom_zip(p))return em_rom_probe(p,LIB_LIMIT,LIB_SNES)==EM_ROM_OK?2:0;
 return em_rom_extension(p,LIB_SNES)?1:0;
}
static char cartridge_name[512];
static void identify(void){unsigned n;UBYTE*p;rom_valid=0;rom_id=rom_id2=rom_bytes=0;
 strcpy(details,"Select a game to see its details.");if(selected<0)return;
 p=AllocVec(LIB_LIMIT,MEMF_PUBLIC);if(!p){strcpy(details,"Not enough memory to inspect this cartridge.");return;}
 int error=em_rom_load(model.entry[selected].path,p,LIB_LIMIT,&n,LIB_SNES,cartridge_name,sizeof(cartridge_name));
 if(error){snprintf(details,sizeof(details),"%s",em_rom_message(error));FreeVec(p);return;}
#ifdef LIBRARY_SNES
 {unsigned skip=snes_skip(n);int h=snes_header(p+skip,n-skip);if(snes_fx3(p+skip,n-skip))strcpy(details,SNES_FX3_WARNING);else if(h>=0){rom_id=snes_hash(p+skip,n-skip);rom_id2=snes_hash2(p+skip,n-skip);rom_bytes=n-skip;rom_valid=1;
  snprintf(details,sizeof(details),"Header supported: %s %s\nGameplay compatibility varies by title.",snes_pal(p+skip,n-skip)?"PAL":"NTSC",h==0x7fc0?"LoROM":"HiROM");}
 else strcpy(details,"Unsupported cartridge: requires NTSC/PAL LoROM/HiROM\nup to 4 MiB, without enhancement chips.");}
#else
 {int mapper=nes_cartridge(p,n);if(mapper>=0){rom_id=bytes_hash(p,n);rom_bytes=n;rom_valid=1;snprintf(details,sizeof(details),"Header supported: %s, mapper %d\nGameplay compatibility varies by title.",fc_rom_pal(p,n,cartridge_name)?"PAL":"NTSC",mapper);}
 else strcpy(details,"Unsupported cartridge: requires supported iNES\nmapper 0/1/2/4/5/90; no trainer or battery.");}
#endif
 FreeVec(p);
}
static void slotpath(char*p,unsigned slotnum){
#ifdef LIBRARY_SNES
 snprintf(p,UI_PATH,"PROGDIR:States/%08lx%08lx-%u.zzs",(unsigned long)rom_id,(unsigned long)rom_id2,slotnum+1);
#else
 snprintf(p,UI_PATH,"PROGDIR:States/%08lx-%u.zzs",(unsigned long)ui_hash(model.entry[selected].path),slotnum+1);
#endif
}
static void snapshotpath(char*p){snprintf(p,UI_PATH,"PROGDIR:Snapshots/%08lx%08lx.zzp",(unsigned long)rom_id,(unsigned long)rom_id2);}
/* The player still validates its complete core layout on
 * Resume; filenames or screenshots alone must never establish compatibility. */
static int read_state(unsigned slotnum,Object*preview){char path[UI_PATH];FILE*f;UBYTE*p;unsigned n;int ok=0;
 const unsigned cap=LIB_STATE_CAP;
 if(selected<0||!rom_valid)return 0;slotpath(path,slotnum);f=fopen(path,"rb");if(!f)return 0;p=AllocVec(cap+1,MEMF_PUBLIC);if(!p){fclose(f);return 0;}
 n=fread(p,1,cap+1,f);if(!ferror(f)){ok=library_state_valid(p,n,rom_id,rom_id2,rom_bytes);
#ifdef LIBRARY_SNES
 if(ok&&preview)preview_set(preview,p+PS_THUMB_OFFSET,NULL);
#else
 if(ok&&preview)preview_set(preview,NULL,"No screenshot");
#endif
 }fclose(f);FreeVec(p);return ok;
}
static int newer(const struct DateStamp*a,const struct DateStamp*b){int32_t aa[3]={a->ds_Days,a->ds_Minute,a->ds_Tick},bb[3]={b->ds_Days,b->ds_Minute,b->ds_Tick};return library_newer(aa,bb);}
static void update_slots(void){unsigned i;char path[UI_PATH],backup[UI_PATH+8];struct FileInfoBlock*fib=AllocDosObject(DOS_FIB,NULL);struct DateStamp best={0};
 latest_slot=-1;set(selected_text,MUIA_Text_Contents,(ULONG)details);
 for(i=0;i<5;i++){BPTR lock=0;present_slots[i]=valid_slots[i]=0;preview_set(cards[i],NULL,"Empty slot");snprintf(card_labels[i],sizeof(card_labels[i]),"Slot %u%s\nEmpty",i+1,value(slot,MUIA_Cycle_Active)==i?" *":"");
  if(selected>=0&&rom_valid&&fib){slotpath(path,i);lock=Lock(path,ACCESS_READ);
   if(lock&&Examine(lock,fib)&&fib->fib_DirEntryType<0){present_slots[i]=1;valid_slots[i]=read_state(i,cards[i]);
    if(valid_slots[i]){struct DateTime dt;char date[32]={0},time[32]={0};memset(&dt,0,sizeof(dt));dt.dat_Stamp=fib->fib_Date;dt.dat_Format=FORMAT_DOS;dt.dat_StrDate=date;dt.dat_StrTime=time;DateToStr(&dt);
     snprintf(card_labels[i],sizeof(card_labels[i]),"Slot %u%s\n%s\n%s",i+1,value(slot,MUIA_Cycle_Active)==i?" *":"",date,time);
     if(latest_slot<0||newer(&fib->fib_Date,&best)){best=fib->fib_Date;latest_slot=i;}
    }else {snprintf(card_labels[i],sizeof(card_labels[i]),"Slot %u\nUnreadable or\nincompatible",i+1);preview_set(cards[i],NULL,"Unavailable");}
   }
   if(lock)UnLock(lock);
   snprintf(backup,sizeof(backup),"%s.bak",path);lock=Lock(backup,ACCESS_READ);
   if(lock){if(Examine(lock,fib)&&fib->fib_DirEntryType<0){
     if(!present_slots[i]){snprintf(card_labels[i],sizeof(card_labels[i]),"Slot %u\nBackup only",i+1);preview_set(cards[i],NULL,"Backup only");}
     present_slots[i]=1;
    }UnLock(lock);}
  }
  set(card_text[i],MUIA_Text_Contents,(ULONG)card_labels[i]);set(card_resume[i],MUIA_Disabled,!valid_slots[i]);set(card_delete[i],MUIA_Disabled,!present_slots[i]);
 }
 if(fib)FreeDosObject(DOS_FIB,fib);
 preview_set(hero,NULL,"Play for art");
 if(selected>=0){snapshotpath(path);if(!preview_art(hero,model.entry[selected].art)&&!(rom_valid&&preview_snapshot(hero,path,rom_id,rom_id2,rom_bytes))&&latest_slot>=0)preview_copy(hero,cards[latest_slot]);}
 if(latest_slot>=0)snprintf(summary_line,sizeof(summary_line),"Latest save: slot %d\nF5/F8 use slot %lu during play.",latest_slot+1,(unsigned long)value(slot,MUIA_Cycle_Active)+1);
 else snprintf(summary_line,sizeof(summary_line),"No save states.\nSelected slot: %lu",(unsigned long)value(slot,MUIA_Cycle_Active)+1);
 set(summary,MUIA_Text_Contents,(ULONG)summary_line);set(start,MUIA_Disabled,!rom_valid);set(resume,MUIA_Disabled,!valid_slots[value(slot,MUIA_Cycle_Active)]);set(latest,MUIA_Disabled,latest_slot<0);
}
/* DOS reports missing separately from denied access; do not mistake failure
 * for an empty slot. Only regular files may be removed. */
static int delete_state_file(const char*path){BPTR lock;struct FileInfoBlock*fib;int regular;
 lock=Lock(path,ACCESS_READ);if(!lock)return IoErr()==ERROR_OBJECT_NOT_FOUND?0:-1;
 fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return -1;}
 regular=Examine(lock,fib)&&fib->fib_DirEntryType<0;FreeDosObject(DOS_FIB,fib);UnLock(lock);
 if(!regular)return -1;if(DeleteFile(path))return 1;
 return IoErr()==ERROR_OBJECT_NOT_FOUND?0:-1;
}
static void delete_slot(unsigned n){char path[UI_PATH],backup[UI_PATH+8],title[UI_TITLE];int result;
 struct EasyStruct request={sizeof(struct EasyStruct),0,"Delete save state",
  "Delete slot %lu for\n%s?\n\nThe save and its backup will be removed.","Delete|Cancel"};
 ULONG args[2];struct Window*confirm;LONG answer;
 if(n>=5||selected<0||!rom_valid)return;
 update_slots();if(!present_slots[n])return;
 slotpath(path,n);snprintf(backup,sizeof(backup),"%s.bak",path);ui_title(&model.entry[selected],title);
 args[0]=n+1;args[1]=(ULONG)title;
 /* Intuition's last gadget returns 0: Cancel. Refuse the special 0/1
  * failure values from BuildEasyRequestArgs; require an actual requester.
  * See intuition.library/EasyRequestArgs and BuildEasyRequestArgs autodocs. */
 confirm=BuildEasyRequestArgs(_window(win),&request,0,args);
 if((ULONG)confirm<=1){message("Could not open deletion confirmation.");return;}
 do{answer=SysReqHandler(confirm,NULL,TRUE);}while(answer==-2);
 FreeSysRequest(confirm);if(answer!=1)return;
 result=library_delete_pair(path,backup,delete_state_file);update_slots();
 message(result==1?"Save state deleted.":result==-2?"Save removed; backup could not be deleted.":"Could not delete save state; backup retained.");
}
static void refresh(void);
static void delete_game(void){
 if(selected<0)return;
 char title[UI_TITLE],path[UI_PATH],backup[UI_PATH+8];ui_title(&model.entry[selected],title);
 struct EasyStruct request={sizeof(struct EasyStruct),0,"Delete game",(em_rom_zip(model.entry[selected].path)?"Delete %s?\nThe ZIP archive, battery saves and library save states will be removed.":"Delete %s?\nThe ROM file, battery saves and library save states will be removed."),"Yes|No"};
 ULONG args[1]={(ULONG)title};struct Window*confirm=BuildEasyRequestArgs(_window(win),&request,0,args);LONG answer;
 if((ULONG)confirm<=1)return;do{answer=SysReqHandler(confirm,NULL,TRUE);}while(answer==-2);FreeSysRequest(confirm);if(answer!=1)return;
 identify();if(delete_state_file(model.entry[selected].path)<0){message("Could not delete ROM. Library entry retained.");return;}
 int incomplete=0;
 {char battery[UI_PATH+12];const char*suffix[]={".sav",".sav.bak",".sav.tmp"};
 for(unsigned i=0;i<3;i++){snprintf(battery,sizeof(battery),"%s%s",model.entry[selected].path,suffix[i]);if(delete_state_file(battery)<0)incomplete=1;}}

 if(rom_valid){for(unsigned i=0;i<5;i++){slotpath(path,i);snprintf(backup,sizeof(backup),"%s.bak",path);if(library_delete_pair(path,backup,delete_state_file)<0)incomplete=1;snprintf(backup,sizeof(backup),"%s.tmp",path);if(delete_state_file(backup)<0)incomplete=1;}
 snapshotpath(path);snprintf(backup,sizeof(backup),"%s.bak",path);if(library_delete_pair(path,backup,delete_state_file)<0)incomplete=1;
 snprintf(backup,sizeof(backup),"%s.tmp",path);if(delete_state_file(backup)<0)incomplete=1;}
 ui_remove(&model,selected);selected=-1;model.last=0;
 refresh();if(!prefs_write())message("Files deleted, but library settings could not be saved.");else message(incomplete?"ROM deleted; some save files could not be removed.":"");
}
static void selection(void){LONG n=(LONG)value(list,MUIA_List_Active);char title[UI_TITLE];if(selection_busy)return;
 selected=n>=0&&n<visible_count?visible[n]:-1;rom_valid=0;latest_slot=-1;needs_details=1;
 title[0]=0;if(selected>=0){ui_title(&model.entry[selected],title);model.last=selected+1;}
 snprintf(save_heading,sizeof(save_heading),"Save states: %s",selected>=0?title:"Select a game first");set(save_title,MUIA_Text_Contents,(ULONG)save_heading);
 ui_clean(selected_path,sizeof(selected_path),selected>=0?model.entry[selected].path:"");set(selected_text,MUIA_ShortHelp,(ULONG)selected_path);
 set(title_edit,MUIA_String_Contents,(ULONG)title);
 set(selected_text,MUIA_Text_Contents,(ULONG)(selected>=0?"Inspecting cartridge...":"Add games, or clear the search/filter."));
 set(start,MUIA_Disabled,TRUE);set(resume,MUIA_Disabled,TRUE);set(latest,MUIA_Disabled,TRUE);
 set(fav,MUIA_Disabled,selected<0);set(remove_button,MUIA_Disabled,selected<0);set(rename_button,MUIA_Disabled,selected<0);set(art_button,MUIA_Disabled,selected<0);set(clear_art,MUIA_Disabled,selected<0);
 for(unsigned i=0;i<5;i++){set(card_resume[i],MUIA_Disabled,TRUE);set(card_delete[i],MUIA_Disabled,TRUE);}
}
static void refresh(void){unsigned i;int old=selected,active=-1;const char*q=(const char*)value(search,MUIA_String_Contents);
 snprintf(search_seen,sizeof(search_seen),"%s",q);search_ticks=0;
 if(model.count>visible_capacity){int*p=realloc(visible,(size_t)model.capacity*sizeof(*visible));
  if(!p){selection_busy=1;DoMethod(list,MUIM_List_Clear);visible_count=0;selection_busy=0;selection();message("Not enough memory to display the library.");return;}visible=p;visible_capacity=model.capacity;}
 selection_busy=1;set(list,MUIA_List_Quiet,TRUE);DoMethod(list,MUIM_List_Clear);visible_count=ui_visible(&model,model.filter,q,visible);
 for(i=0;i<(unsigned)visible_count;i++){int n=visible[i];char title[UI_TITLE],row[UI_TITLE+8];ui_title(&model.entry[n],title);
  snprintf(row,sizeof(row),"%s %s",model.entry[n].favorite?"*":" ",title);DoMethod(list,MUIM_List_InsertSingle,(ULONG)row,MUIV_List_Insert_Bottom);if(n==old)active=i;}
 int complete=value(list,MUIA_List_Entries)==(ULONG)visible_count;
 if(!complete){DoMethod(list,MUIM_List_Clear);visible_count=0;active=-1;}
 if(active<0&&visible_count)active=0;set(list,MUIA_List_Active,active);set(list,MUIA_List_Quiet,FALSE);selection_busy=0;selection();
 snprintf(count_line,sizeof(count_line),"%s - %d / %u games",filters[model.filter],visible_count,model.count);set(count_text,MUIA_Text_Contents,(ULONG)count_line);if(!complete)message("Not enough memory to display the library. Saved games are retained.");
}
/* Some MUI string classes notify Contents only on programmatic changes. */
static void search_poll(void){const char*q=(const char*)value(search,MUIA_String_Contents);
 if(strcmp(q,search_seen)){snprintf(search_seen,sizeof(search_seen),"%s",q);search_ticks=4;}
 else if(search_ticks&&!--search_ticks)refresh();
}
static void scan_caption(int active){set(scan_drives,MUIA_Text_Contents,(ULONG)(active?"Stop scan":"Scan drives"));}
#include "library_scan.h"
static void artwork(void){struct FileRequester*r;char path[UI_PATH];if(selected<0)return;set(app,MUIA_Application_Sleep,TRUE);
 r=AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)"Choose local artwork (up to 512 x 512)",ASLFR_InitialDrawer,(ULONG)"PROGDIR:Artwork",TAG_DONE);
 if(r){if(AslRequestTags(r,TAG_DONE)&&strlen(r->fr_Drawer)<sizeof(path)){strcpy(path,r->fr_Drawer);
  if(AddPart(path,r->fr_File,sizeof(path))){if(preview_art(hero,path)){strcpy(model.entry[selected].art,path);message(prefs_write()?"Artwork linked; original image unchanged.":"Could not save artwork link.");}
   else message("Cannot decode picture. Use <=512x512, <=1 MiB; a V43 picture datatype is required.");}}
  FreeAslRequest(r);}set(app,MUIA_Application_Sleep,FALSE);
}

static void browse(int drawer){struct FileRequester *r;char p[UI_PATH];unsigned before=model.count,rejected=0;
 set(app,MUIA_Application_Sleep,TRUE);
 r=AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)(drawer?"Add a folder of games":"Add a game"),ASLFR_InitialDrawer,(ULONG)"PROGDIR:ROMs",ASLFR_InitialPattern,(ULONG)LIB_PATTERN,ASLFR_DoPatterns,TRUE,ASLFR_DrawersOnly,drawer,TAG_DONE);
 if(r){if(AslRequestTags(r,TAG_DONE)&&strlen(r->fr_Drawer)<sizeof(p)){
   strcpy(p,r->fr_Drawer);
   if(drawer){BPTR lock=Lock(p,ACCESS_READ);struct FileInfoBlock *fib=AllocDosObject(DOS_FIB,NULL);
    if(lock&&fib&&Examine(lock,fib))while(ExNext(lock,fib)){char full[UI_PATH];
     if(fib->fib_DirEntryType<0&&ui_extension(fib->fib_FileName)){strcpy(full,p);if(AddPart(full,fib->fib_FileName,sizeof(full))&&library_candidate(full))if(ui_add(&model,full)<0)rejected++;}}
    if(lock)UnLock(lock);if(fib)FreeDosObject(DOS_FIB,fib);
   }else if(AddPart(p,r->fr_File,sizeof(p))){
    if(em_rom_zip(p)){int e=em_rom_probe(p,LIB_LIMIT,LIB_SNES);if(e){FreeAslRequest(r);set(app,MUIA_Application_Sleep,FALSE);message(em_rom_message(e));return;}}
    int added=ui_add(&model,p);if(added<0)rejected++;else selected=added;}
  }FreeAslRequest(r);}
 set(app,MUIA_Application_Sleep,FALSE);refresh();
 if(model.count>before){if(!prefs_write())message("Added to this session; preferences could not be saved.");else message("ROM library saved. Select a game and press Play.");}
 if(rejected)message("Some files could not be added. Check free memory and path lengths.");
}
static int session_write(int resume){FILE *f;char p[UI_PATH];int ok;
 f=fopen("PROGDIR:Session.cfg","w");if(!f)return 0;
 fprintf(f,LIB_SESSION "\n%u %u %u %u\n",model.monitor|(model.theme<<1)|(model.mask<<4)|(model.vmask<<6),model.mute,model.fire,resume);
 ui_hex(f,model.entry[selected].path);fputc('\n',f);slotpath(p,model.slot);ui_hex(f,p);fputc('\n',f);snapshotpath(p);ui_hex(f,p);fputc('\n',f);fprintf(f,"DISPLAY %u %u %u %u\n",model.windowed,model.scanlines,model.fps,model.optimise);ok=!ferror(f);if(fclose(f))ok=0;return ok;}
#include "library_launch.h"
static void play(int resume){char path[UI_PATH],session[UI_PATH],qplayer[UI_PATH*2+4],qsession[UI_PATH*2+4],command[UI_PATH*4+40];BPTR dir;
 if(selected<0){message("Select a game first.");return;}identify();if(!rom_valid){message(details);return;}collect();
 if(resume&&!read_state(model.slot,NULL)){message("That save is missing, damaged or for another cartridge.");update_slots();return;}
 dir=CreateDir("PROGDIR:States");if(dir)UnLock(dir);
 dir=CreateDir("PROGDIR:Snapshots");if(dir)UnLock(dir);
 if(!session_write(resume)){message("Cannot write launch settings. Check that the drawer is writable.");return;}
 ui_played(&model,selected);if(!prefs_write()){message("Cannot save library settings. Check drawer permissions.");return;}
 strcpy(path,appdir);strcpy(session,appdir);
 if(!AddPart(path,LIB_PLAYER,sizeof(path))||!AddPart(session,"Session.cfg",sizeof(session))||
    !ui_quote(qplayer,sizeof(qplayer),path)||!ui_quote(qsession,sizeof(qsession),session)){message("Application path is too long.");return;}
 snprintf(command,sizeof(command),"%s --session %s",qplayer,qsession);
 if(!launch_child(command))message("Could not start the player.");
}
static void joystick(void){ULONG s=LowLevelBase?ReadJoyPort(1):0,type=s&JP_TYPE_MASK;
 const char *kind=!LowLevelBase?"lowlevel.library unavailable":type==JP_TYPE_JOYSTK?"Standard joystick":type==JP_TYPE_GAMECTLR?"Game controller":type==JP_TYPE_MOUSE?"Mouse detected":"Move the stick or press fire to detect it";
 snprintf(joy,sizeof(joy),"Port 2: %s  |  Raw: %08lx",kind,(unsigned long)s);
 set(joytext,MUIA_Text_Contents,(ULONG)joy);
 controller_update(s,value(fire,MUIA_Cycle_Active),value(pages,MUIA_Group_ActivePage)==2);
 if(value(win,MUIA_Window_Activate)&&value(pages,MUIA_Group_ActivePage)==0&&(type==JP_TYPE_JOYSTK||type==JP_TYPE_GAMECTLR)){
  ULONG edge=s&~previous_joy;LONG active=(LONG)value(list,MUIA_List_Active);
  if((edge&JPF_JOY_UP)&&active>0)set(list,MUIA_List_Active,active-1);
  else if((edge&JPF_JOY_DOWN)&&active+1<visible_count)set(list,MUIA_List_Active,active+1);
  if((edge&JPF_BUTTON_RED)&&rom_valid)set(win,MUIA_Window_ActiveObject,(ULONG)start);
 }previous_joy=s;
}
static Object*save_card(unsigned i){Object*o;static const char*labels[]={"Slot 1","Slot 2","Slot 3","Slot 4","Slot 5"};
 o=VGroup,GroupFrameT(labels[i]),
  Child,cards[i]=preview_new(),
  Child,card_text[i]=multiline("Empty",3),
  Child,card_resume[i]=ui_button("Resume"),Child,card_delete[i]=ui_button("Delete..."),End;return o;
}
#include "library_brand.h"
static int build(void){Object *add,*scan,*save,*exit,*listview,*all,*recent,*favourites,*saves;
 program_list_class=MUI_CreateCustomClass(NULL,MUIC_Listview,NULL,sizeof(struct program_list_data),(APTR)program_list_dispatch);if(!program_list_class)return 0;
 controller_class=MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct controller_instance),(APTR)controller_dispatch);if(!controller_class)return 0;
 multiline_class=MUI_CreateCustomClass(NULL,MUIC_Text,NULL,0,(APTR)multiline_dispatch);if(!multiline_class)return 0;
 preview_class=MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct preview_instance),(APTR)preview_dispatch);if(!preview_class)return 0;
 app=ApplicationObject,MUIA_Application_Title,(ULONG)LIB_TITLE " Library",MUIA_Application_Version,(ULONG)version,
 MUIA_Application_Base,(ULONG)LIB_TITLE,MUIA_Application_Description,(ULONG)"Native ZZ9000 game library",
 SubWindow,win=WindowObject,MUIA_Window_Title,(ULONG)"",MUIA_Window_ID,
#ifdef LIBRARY_SNES
 0x534e5549,
#else
 0x4e455355,
#endif
 MUIA_Window_Width,900,
 WindowContents,VGroup,MUIA_Group_Spacing,8,InnerSpacing(10,8),
  Child,HGroup,MUIA_Group_Spacing,12,
   Child,VGroup,MUIA_HorizWeight,0,MUIA_FixWidth,148,
    Child,HGroup,Child,HSpace(0),Child,library_logo(80),Child,HSpace(0),End,
    Child,ListviewObject,MUIA_CycleChain,1,MUIA_FixWidth,148,MUIA_Listview_List,navigation=ListObject,MUIA_Font,ui_font(),InputListFrame,MUIA_List_MinLineHeight,30,End,End,
   End,
   Child,VGroup,MUIA_Group_Spacing,8,
    Child,heading=TextObject,MUIA_Font,ui_title_font(),MUIA_Text_Contents,(ULONG)"Games",End,
    Child,pages=VGroup,MUIA_Group_PageMode,TRUE,
   Child,VGroup,
    Child,HGroup,Child,TextObject,MUIA_Font,ui_font(),MUIA_HorizWeight,0,MUIA_Text_Contents,(ULONG)"Search:",End,Child,search=StringObject,MUIA_Font,ui_font(),StringFrame,MUIA_String_MaxLen,UI_TITLE,End,End,
    Child,HGroup,Child,all=ui_button("All games"),Child,recent=ui_button("Recently played"),Child,favourites=ui_button("Favourites"),End,
    Child,count_text=textline("Your game library"),
    Child,HGroup,Child,autoscan_option=CheckMark(FALSE),Child,textline("Find ROMs on mounted drives automatically"),End,
    Child,HGroup,
     Child,VGroup,MUIA_Weight,45,
      Child,listview=program_list_new(&list),
      Child,HGroup,Child,add=ui_button("Add..."),Child,scan=ui_button("Folder..."),Child,scan_drives=ui_button("Scan drives"),End,
      Child,HGroup,Child,fav=ui_button("Favourite on/off"),Child,remove_button=ui_button("Delete..."),End,
     End,
     Child,VGroup,GroupFrameT("Selected game"),MUIA_Weight,55,
      Child,HGroup,Child,title_edit=StringObject,MUIA_Font,ui_font(),StringFrame,MUIA_String_MaxLen,UI_TITLE,End,Child,rename_button=ui_button("Rename"),End,
      Child,selected_text=multiline("Select a game.",2),
      Child,HGroup,Child,hero=preview_new(),Child,VGroup,
       Child,summary=multiline("No saves yet.",2),
       Child,art_button=ui_button("Artwork..."),Child,clear_art=ui_button("Clear artwork"),End,End,
      Child,textline("F9 in game: capture library picture."),
      Child,start=ui_button("Play from start"),Child,latest=ui_button("Continue latest save"),Child,saves=ui_button("Choose save state..."),
      Child,VSpace(0),
     End,
    End,
   End,
   Child,VGroup,MUIA_Group_Spacing,8,
    Child,ColGroup(2),GroupFrameT("Presentation"),
     Child,TextObject,MUIA_Font,ui_font(),MUIA_FixWidth,180,MUIA_Text_Contents,(ULONG)"Display",End,Child,display_mode=cycle(display_choices),
     Child,textline("Scanlines"),Child,scanline_mode=cycle(scanline_choices),
     Child,textline("On-screen FPS"),Child,HGroup,Child,fps_option=CheckMark(FALSE),Child,HSpace(0),End,
     Child,textline("Optimised presenter"),Child,HGroup,Child,optimise_option=CheckMark(TRUE),Child,HSpace(0),End,End,
    Child,textline("Alt + Return switches display during play. F10 returns to the library."),
    Child,textline("Monitor shape (set the monitor itself to fill the screen):"),Child,monitor=cycle(monitors),
    Child,textline("Border / wallpaper:"),Child,theme=cycle(themes),
    Child,textline("Left / right edge mask (hide flashing pixels):"),Child,mask=cycle(masks),
    Child,textline("Top / bottom edge mask:"),Child,vmask=cycle(masks),
    Child,textline("The game keeps a physical 4:3 shape.\nDisplay and controller settings apply to all games."),
    Child,HGroup,Child,mute=CheckMark(FALSE),Child,textline("Start muted (M toggles sound while playing)"),End,Child,VSpace(0),End,
   Child,VGroup,
    Child,joytext=textline("Move the joystick or press Fire on physical port 2"),
    Child,controller=controller_new(),
    Child,HGroup,Child,textline("Primary fire button:"),Child,fire=cycle(fires),End,
    Child,textline("Button2 needs a second physical button; Green/Play need a gamepad.\nKeyboard labels show in-game controls; this panel tests the joystick."),
    Child,textline("P=pause; M=mute; Q/Escape=return; F9=capture game picture."),Child,VSpace(0),End,
   Child,VGroup,
    Child,save_title=textline("Save states: Select a game first"),
    Child,HGroup,Child,save_card(0),Child,save_card(1),Child,save_card(2),Child,save_card(3),Child,save_card(4),End,
    Child,HGroup,Child,textline("Active save slot:"),Child,slot=cycle(slots),Child,resume=ui_button("Resume chosen slot"),End,
    Child,textline("F5 saves; F8 restores, including while paused. Previous saves kept as .bak.\nContinue uses the newest readable save by the Amiga file date."),
    Child,VSpace(0),End,
   Child,VGroup,
    Child,textline("\33b" LIB_TITLE " " LIB_VERSION " for AmigaOS / ZZ9000\33n"),
    Child,VGroup,GroupFrameT("Amiga development"),
     Child,textline("SkiltonUSA - 68K frontend, Cortex-A9 integration and optimisation.\nDevelopment: Codex assisted, Amiga-Bridge MCP"),
#ifdef LIBRARY_SNES
     Child,textline("github.com/SkiltonUSA/ZZSNES-ARM-68K"),
#else
     Child,textline("github.com/SkiltonUSA/ZZNes-ARM-68K"),
#endif
    End,
    Child,VGroup,GroupFrameT("Emulation core"),
#ifdef LIBRARY_SNES
     Child,textline("Snes9x2002 / PocketSNES - Snes9x and libretro contributors.\nOriginal Snes9x: Gary Henderson and Jerremy Koot."),
     Child,textline("github.com/libretro/snes9x2002\nCore: Snes9x non-commercial licence."),
#else
     Child,textline("FCEUX / FCE Ultra - Xodnizel and the upstream contributors.\nARM integration based on the Rezzy-dev/fceux source tree."),
     Child,textline("github.com/Rezzy-dev/fceux\nCore: GPL-2.0-or-later."),
#endif
    End,
    Child,VGroup,GroupFrameT("Platform and interface"),
     Child,textline("Xanxi (Xanxi-Amiga) - XACP platform.\ngithub.com/Xanxi-Amiga/XACP-ZZ9000"),
     Child,textline("MUI (Magic User Interface) - native Amiga interface toolkit."),
    End,
    Child,textline("Interface: GPL-3.0-or-later. Credits.txt contains the full acknowledgements."),
    Child,ListviewObject,MUIA_Font,ui_font(),MUIA_FixHeightTxt,(ULONG)"\n\n\n\n",MUIA_Listview_List,change_list=ListObject,MUIA_Font,ui_font(),ReadListFrame,
     MUIA_List_ConstructHook,MUIV_List_ConstructHook_String,MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,End,
    Child,VSpace(0),End,
  End,
   End,
  End,
  Child,status_text=TextObject,MUIA_Font,ui_font(),TextFrame,MUIA_Text_SetMin,FALSE,MUIA_FixHeightTxt,(ULONG)"Ag",MUIA_Text_Contents,(ULONG)"",End,
  Child,HGroup,Child,save=ui_button("Save settings"),Child,exit=ui_button("Quit"),Child,HSpace(0),Child,textline(LIB_VERSION),End,
 End,End,End;
 if(!app)return 0;
 {const char*changes[]={LIB_VERSION " - EmuELEC bezel",
   "Load ZIP cartridges directly",
   "Libraries grow with available memory",
   "Fast path lookup and sorting for large ROM collections",
 "Sidebar navigation; startup and loading screens; original logos.",
 "Fullscreen / window switching; optional FPS and software scanlines.",
 "Guarded NEON presentation with integer fallback; baseline comparison.",
 "Mounted-drive ROM discovery; Add, Rename and confirmed Delete.",
 "Previous beta - regional cadence, batched audio and save states.",NULL};
 for(unsigned i=0;changes[i];i++)DoMethod(change_list,MUIM_List_InsertSingle,(ULONG)changes[i],MUIV_List_Insert_Bottom);}

 for(unsigned i=0;tabs[i];i++)DoMethod(navigation,MUIM_List_InsertSingle,(ULONG)tabs[i],MUIV_List_Insert_Bottom);
 set(navigation,MUIA_List_Active,0);notify(navigation,MUIA_List_Active,MUIV_EveryTime,ID_NAV);
 notify(scan_drives,MUIA_Pressed,FALSE,ID_SCAN_DRIVES);
 notify(win,MUIA_Window_CloseRequest,TRUE,MUIV_Application_ReturnID_Quit);notify(exit,MUIA_Pressed,FALSE,MUIV_Application_ReturnID_Quit);
 notify(add,MUIA_Pressed,FALSE,ID_ADD);notify(scan,MUIA_Pressed,FALSE,ID_SCAN);notify(fav,MUIA_Pressed,FALSE,ID_FAV);notify(remove_button,MUIA_Pressed,FALSE,ID_REMOVE);
 notify(start,MUIA_Pressed,FALSE,ID_PLAY);notify(resume,MUIA_Pressed,FALSE,ID_RESUME);notify(save,MUIA_Pressed,FALSE,ID_SAVE);
 notify(list,MUIA_List_Active,MUIV_EveryTime,ID_SELECT);notify(slot,MUIA_Cycle_Active,MUIV_EveryTime,ID_SLOT);
 notify(listview,MUIA_Listview_DoubleClick,TRUE,ID_PLAY);notify(search,MUIA_String_Acknowledge,MUIV_EveryTime,ID_SEARCH);
 notify(all,MUIA_Pressed,FALSE,ID_ALL);notify(recent,MUIA_Pressed,FALSE,ID_RECENT);notify(favourites,MUIA_Pressed,FALSE,ID_FAVOURITES);
 notify(latest,MUIA_Pressed,FALSE,ID_LATEST);notify(saves,MUIA_Pressed,FALSE,ID_SAVES);
 notify(rename_button,MUIA_Pressed,FALSE,ID_RENAME);notify(art_button,MUIA_Pressed,FALSE,ID_ART);notify(clear_art,MUIA_Pressed,FALSE,ID_CLEAR_ART);
 for(unsigned i=0;i<5;i++){notify(card_resume[i],MUIA_Pressed,FALSE,ID_SLOT_RESUME+i);notify(card_delete[i],MUIA_Pressed,FALSE,ID_SLOT_DELETE+i);}
 set(display_mode,MUIA_Cycle_Active,model.windowed);set(scanline_mode,MUIA_Cycle_Active,model.scanlines);set(fps_option,MUIA_Selected,model.fps);set(optimise_option,MUIA_Selected,model.optimise);set(autoscan_option,MUIA_Selected,model.autoscan);
 set(monitor,MUIA_Cycle_Active,model.monitor);set(theme,MUIA_Cycle_Active,model.theme);set(mask,MUIA_Cycle_Active,model.mask);set(vmask,MUIA_Cycle_Active,model.vmask);set(slot,MUIA_Cycle_Active,model.slot);set(mute,MUIA_Selected,model.mute);set(fire,MUIA_Cycle_Active,model.fire);
 return 1;
}

int main(void){struct MsgPort *port=NULL,*owner=NULL;struct timerequest *timer=NULL;FILE *f;ULONG signals=0,id;int running=1,opened=0,pending=0,rc=20,config_bad=0;
 {struct Task*t=FindTask(NULL);if((ULONG)t->tc_SPUpper-(ULONG)t->tc_SPLower<65536){puts(LIB_TITLE " Library requires Stack 65536 (also set the Workbench icon stack).");return 20;}}
 if(!NameFromLock(GetProgramDir(),appdir,sizeof(appdir)))goto done;
 owner=CreateMsgPort();if(!owner)goto done;owner->mp_Node.ln_Name=LIB_PORT;
 Forbid();if(FindPort(owner->mp_Node.ln_Name)){Permit();DeleteMsgPort(owner);owner=NULL;goto done;}AddPort(owner);Permit();
 ui_defaults(&model);f=fopen("PROGDIR:Library.cfg","r");if(f){char header[32];if(fgets(header,sizeof(header),f))legacy_config=!strcmp(header,UI_CONFIG_PREFIX "1\n");rewind(f);if(!ui_read(f,&model)){ui_defaults(&model);config_bad=1;}fclose(f);}
 selected=model.last?(int)model.last-1:-1;
 GfxBase=(struct GfxBase*)OpenLibrary("graphics.library",39);DataTypesBase=OpenLibrary("datatypes.library",43);
 MUIMasterBase=OpenLibrary("muimaster.library",15);AslBase=OpenLibrary("asl.library",38);LowLevelBase=OpenLibrary("lowlevel.library",40);
 if(!MUIMasterBase||!AslBase||!GfxBase)goto done;
 if(config_bad){puts("Could not load Library.cfg. Check free memory and the settings file; no files changed.");goto done;}
 KeymapBase=OpenLibrary("keymap.library",37);style_open();
 if(!build())goto done;refresh();loading_window=splash_create(0);startup_ticks=20;if(!loading_window)set(win,MUIA_Window_Open,TRUE);
 port=CreateMsgPort();if(!port)goto done;timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));if(!timer)goto done;
 if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0))goto done;opened=1;
 while(running){
  id=DoMethod(app,MUIM_Application_NewInput,(ULONG)&signals);
  switch(id){case (ULONG)MUIV_Application_ReturnID_Quit:if(!child_running)running=0;break;
   case ID_ADD:browse(0);break;case ID_SCAN:browse(1);break;case ID_SELECT:selection();break;case ID_FILTER:refresh();break;
   case ID_NAV:{ULONG n=value(navigation,MUIA_List_Active);if(n<5){set(pages,MUIA_Group_ActivePage,n);set(heading,MUIA_Text_Contents,(ULONG)tabs[n]);}}break;
   case ID_SCAN_DRIVES:if(scan_active){scan_stop();message("Scan stopped.");}else scan_begin(1);break;
   case ID_SEARCH:refresh();break;
   case ID_ALL:model.filter=0;refresh();break;case ID_RECENT:model.filter=1;refresh();break;case ID_FAVOURITES:model.filter=2;refresh();break;
   case ID_RENAME:if(selected>=0){ui_clean(model.entry[selected].title,UI_TITLE,(const char*)value(title_edit,MUIA_String_Contents));refresh();message(prefs_write()?"Display title saved; ROM filename unchanged.":"Could not save title.");}break;
   case ID_ART:artwork();break;
   case ID_CLEAR_ART:if(selected>=0){model.entry[selected].art[0]=0;update_slots();message(prefs_write()?"Artwork removed from library; image file unchanged.":"Could not save artwork setting.");}break;
   case ID_SAVES:set(navigation,MUIA_List_Active,3);break;
   case ID_LATEST:if(latest_slot>=0){set(slot,MUIA_Cycle_Active,latest_slot);play(1);}break;
   case ID_FAV:if(selected>=0){model.entry[selected].favorite^=1;refresh();if(!prefs_write())message("Could not save favourites.");}break;
   case ID_REMOVE:delete_game();break;
   case ID_PLAY:play(0);break;case ID_RESUME:play(1);break;case ID_SLOT:update_slots();break;
   case ID_SAVE:message(prefs_write()?"Settings saved.":"Could not save settings.");break;
   default:if(id>=ID_SLOT_RESUME&&id<ID_SLOT_RESUME+5){set(slot,MUIA_Cycle_Active,id-ID_SLOT_RESUME);play(1);}
    else if(id>=ID_SLOT_DELETE&&id<ID_SLOT_DELETE+5)delete_slot(id-ID_SLOT_DELETE);break;}
  if(!id&&needs_details){needs_details=0;identify();update_slots();}
  if(!running)break;
  if(pending&&CheckIO((struct IORequest *)timer)){WaitIO((struct IORequest *)timer);pending=0;}
  if(!pending){launch_poll();if(!child_running&&!startup_ticks){search_poll();joystick();if(!scan_active&&value(autoscan_option,MUIA_Selected)&&++scan_ticks>=200){scan_ticks=0;scan_begin(0);}scan_step();}timer->tr_node.io_Command=TR_ADDREQUEST;timer->tr_time.tv_secs=0;timer->tr_time.tv_micro=50000;SendIO((struct IORequest *)timer);pending=1;}
  if(!id){ULONG got=Wait(signals|(1UL<<port->mp_SigBit)|SIGBREAKF_CTRL_C);if(got&SIGBREAKF_CTRL_C)running=0;}
 }
 scan_stop();rc=prefs_write()?0:20;
done:
 if(pending){AbortIO((struct IORequest *)timer);WaitIO((struct IORequest *)timer);}if(opened)CloseDevice((struct IORequest *)timer);if(timer)DeleteIORequest((struct IORequest *)timer);if(port)DeleteMsgPort(port);
 splash_close();scan_stop();if(app)MUI_DisposeObject(app);if(program_list_class)MUI_DeleteCustomClass(program_list_class);if(KeymapBase)CloseLibrary(KeymapBase);style_close();if(controller_class)MUI_DeleteCustomClass(controller_class);if(preview_class)MUI_DeleteCustomClass(preview_class);if(multiline_class)MUI_DeleteCustomClass(multiline_class);if(DataTypesBase)CloseLibrary(DataTypesBase);if(GfxBase)CloseLibrary((struct Library*)GfxBase);if(LowLevelBase)CloseLibrary(LowLevelBase);if(AslBase)CloseLibrary(AslBase);if(MUIMasterBase)CloseLibrary(MUIMasterBase);
 free(visible);ui_dispose(&model);if(owner){RemPort(owner);DeleteMsgPort(owner);}return rc;
}
