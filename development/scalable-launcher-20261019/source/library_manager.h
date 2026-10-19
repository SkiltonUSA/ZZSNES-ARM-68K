/* GPL-3.0-or-later. Incremental ROM management; no game file deletion. */
#include "rom_manager.h"
static Object *manager_controls,*manager_import_path,*manager_backup_path,*manager_cancel,*manager_settings_save;
static int manager_mode; /* 1 duplicates, 2 backup, 3 one-shot import */
static unsigned manager_at,manager_duplicates,manager_unreadable,*manager_keep,*manager_next;
static struct ui_path_index manager_paths,manager_content;
static unsigned char *manager_rom,*manager_compare;
static struct rm_copy *manager_copy;
static BPTR manager_lock;
static struct FileInfoBlock *manager_fib;
static FILE *manager_log;
static char manager_root[UI_PATH],manager_partial[UI_PATH];
static unsigned manager_phase,manager_battery,manager_suffix,manager_files;
static const char *manager_dirs[]={"States","Snapshots"};

/* Announce a file one event-loop turn before decoding so MUI can repaint. */
static Object *duplicate_window,*duplicate_file,*duplicate_progress;
static unsigned duplicate_announced;
static char duplicate_filename[UI_PATH],duplicate_count[80];
static void duplicate_splash_close(void){
 if(duplicate_window){set(duplicate_window,MUIA_Window_Open,FALSE);DoMethod(app,OM_REMMEMBER,(ULONG)duplicate_window);MUI_DisposeObject(duplicate_window);}
 duplicate_window=duplicate_file=duplicate_progress=NULL;duplicate_announced=0;
}
static void duplicate_splash_open(void){Object *cancel;
 duplicate_window=WindowObject,MUIA_Window_Title,(ULONG)"Duplicate scan",
 MUIA_Window_CloseGadget,FALSE,MUIA_Window_SizeGadget,FALSE,
 MUIA_Window_LeftEdge,MUIV_Window_LeftEdge_Centered,MUIA_Window_TopEdge,MUIV_Window_TopEdge_Centered,
 WindowContents,VGroup,GroupFrame,InnerSpacing(12,10),MUIA_Group_Spacing,8,
  Child,textline("Scanning for duplicates,\nzipped files take longer"),
  Child,duplicate_file=TextObject,MUIA_Font,ui_font(),MUIA_Text_SetMin,FALSE,
   MUIA_Text_Contents,(ULONG)"",MUIA_FixWidthTxt,(ULONG)"MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",End,
  Child,duplicate_progress=TextObject,MUIA_Font,ui_font(),MUIA_Text_SetMin,FALSE,MUIA_Text_Contents,(ULONG)"",End,
  Child,HGroup,Child,HSpace(0),Child,cancel=ui_button("Cancel"),Child,HSpace(0),End,
 End,End;
 if(duplicate_window){DoMethod(app,OM_ADDMEMBER,(ULONG)duplicate_window);notify(cancel,MUIA_Pressed,FALSE,ID_MANAGER_CANCEL);set(duplicate_window,MUIA_Window_Open,TRUE);}
}

static void manager_enable(int busy){
 set(manager_controls,MUIA_Disabled,busy);set(manager_cancel,MUIA_Disabled,!busy);
 set(navigation,MUIA_Disabled,busy);set(manager_settings_save,MUIA_Disabled,busy);set(start,MUIA_Disabled,busy||selected<0||!rom_valid);
}
static void manager_finish(const char *text){
 duplicate_splash_close();
 if(manager_copy){rm_copy_cancel(manager_copy);free(manager_copy);manager_copy=NULL;}
 if(manager_lock){UnLock(manager_lock);manager_lock=0;}
 if(manager_fib){FreeDosObject(DOS_FIB,manager_fib);manager_fib=NULL;}
 if(manager_log){fputs("INCOMPLETE\n",manager_log);fclose(manager_log);manager_log=NULL;}
 free(manager_keep);free(manager_next);free(manager_rom);free(manager_compare);
 manager_keep=manager_next=NULL;manager_rom=manager_compare=NULL;
 ui_index_free(&manager_paths);ui_index_free(&manager_content);
 if(manager_mode==3){scan_stop();scan_dirty=0;if(!prefs_write())text="Import stopped; library could not be saved.";}
 manager_mode=0;manager_enable(0);message(text);
}
static void manager_browse(Object *target,const char *title){
 struct FileRequester *r;set(app,MUIA_Application_Sleep,TRUE);
 r=AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)title,
  ASLFR_InitialDrawer,value(target,MUIA_String_Contents),ASLFR_DrawersOnly,TRUE,TAG_DONE);
 if(r){if(AslRequestTags(r,TAG_DONE))set(target,MUIA_String_Contents,(ULONG)r->fr_Drawer);FreeAslRequest(r);}
 set(app,MUIA_Application_Sleep,FALSE);
}
static void manager_duplicates_begin(void){
 if(scan_active){message("Finish the directory scan first.");return;}
 if(model.count<2){message("No duplicate entries.");return;}
 manager_keep=malloc(model.count*sizeof(*manager_keep));manager_next=malloc(model.count*sizeof(*manager_next));
 manager_rom=malloc(LIB_LIMIT);manager_compare=malloc(LIB_LIMIT);
 if(!manager_keep||!manager_next||!manager_rom||!manager_compare){manager_finish("Not enough memory to compare ROMs.");return;}
 manager_at=manager_duplicates=manager_unreadable=0;manager_mode=1;manager_enable(1);
 duplicate_announced=0;duplicate_splash_open();message("Scanning for duplicates, zipped files take longer");
}
static void manager_duplicates_apply(void){
 duplicate_splash_close();
 char note[200];struct ui_model next,old;int old_selection=selected,new_selection=selected;
 snprintf(note,sizeof(note),"Remove %u duplicate library entries?\nROM files and saves will be kept.\n%u unreadable entries will be kept.",manager_duplicates,manager_unreadable);
 if(!manager_duplicates){snprintf(note,sizeof(note),"No duplicates found; %u unreadable entries kept.",manager_unreadable);manager_finish(note);return;}
 struct EasyStruct request={sizeof(request),0,"Delete duplicates",note,"Remove|Cancel"};
 if(!EasyRequestArgs(NULL,&request,NULL,NULL)){manager_finish("Duplicate removal cancelled.");return;}
 if(!rm_compact(&model,manager_keep,&next,&new_selection)){manager_finish("Not enough memory; library unchanged.");return;}
 old=model;model=next;selected=new_selection;
 if(!prefs_write()){ui_dispose(&model);model=old;selected=old_selection;manager_finish("Could not save changes; library entries retained.");return;}
 ui_dispose(&old);refresh();
 snprintf(note,sizeof(note),"Removed %u duplicate entries; %u unreadable entries kept. ROMs and saves retained.",manager_duplicates,manager_unreadable);
 manager_finish(note);
}
static void manager_duplicate_step(void){
 unsigned i=manager_at,n=0;char canonical[UI_PATH],key[80],note[160];int prior=-1;
 if(i==model.count){manager_duplicates_apply();return;}
 if(!duplicate_announced){
  const char *name=FilePart(model.entry[i].path);
  snprintf(duplicate_filename,sizeof(duplicate_filename),"%s",name);
  snprintf(duplicate_count,sizeof(duplicate_count),"File %u of %u",i+1,model.count);
  if(duplicate_window){set(duplicate_file,MUIA_Text_Contents,(ULONG)duplicate_filename);set(duplicate_progress,MUIA_Text_Contents,(ULONG)duplicate_count);}
  message(duplicate_filename);duplicate_announced=1;return;
 }
 duplicate_announced=0;
 manager_keep[i]=i;manager_next[i]=~0u;
 BPTR lock=Lock(model.entry[i].path,ACCESS_READ);
 if(lock){if(NameFromLock(lock,canonical,sizeof(canonical)))prior=ui_index_find(&manager_paths,canonical);else canonical[0]=0;UnLock(lock);}else canonical[0]=0;
 if(prior>=0){manager_keep[i]=manager_keep[prior];manager_duplicates++;}
 else{
  int error=em_rom_load(model.entry[i].path,manager_rom,LIB_LIMIT,&n,LIB_SNES,NULL,0);
  if(error)manager_unreadable++;
  else{
   unsigned a=2166136261u,b=5381u;
   for(unsigned j=0;j<n;j++){a=(a^manager_rom[j])*16777619u;b=(b*33u)^manager_rom[j];}
   snprintf(key,sizeof(key),"%08x-%08x-%08x",n,a,b);prior=ui_index_find(&manager_content,key);
   unsigned match=prior<0?~0u:(unsigned)prior;
   while(match!=~0u){unsigned other=0;
    if(em_rom_load(model.entry[match].path,manager_compare,LIB_LIMIT,&other,LIB_SNES,NULL,0)==0&&other==n&&!memcmp(manager_rom,manager_compare,n))break;
    match=manager_next[match];
   }
   if(match!=~0u){manager_keep[i]=match;manager_duplicates++;}
   else if(prior<0){if(ui_index_add(&manager_content,key,i)<0){manager_finish("Not enough memory; library unchanged.");return;}}
   else{manager_next[i]=manager_next[prior];manager_next[prior]=i;}
  }
  if(!error&&*canonical&&ui_index_add(&manager_paths,canonical,i)<0){manager_finish("Not enough memory; library unchanged.");return;}
 }
 manager_at++;
 snprintf(note,sizeof(note),"Compared %u / %u ROMs - %u duplicate entries",manager_at,model.count,manager_duplicates);message(note);
}
static int manager_directory(const char *path){BPTR lock=CreateDir(path);if(!lock)return 0;UnLock(lock);return 1;}
static int manager_backup_file(const char *source,const char *relative){
 char target[UI_PATH];strcpy(target,manager_root);
 if(!AddPart(target,relative,sizeof(target))||!rm_copy_begin(manager_copy,source,target,manager_partial))return 0;
 message(relative);return 1;
}
static void manager_backup_begin(void){
 const char *dest=(const char*)value(manager_backup_path,MUIA_String_Contents);char path[UI_PATH],name[80];struct DateStamp stamp;int made=0;
 if(scan_active){message("Finish the directory scan first.");return;}
 if(!*dest||strlen(dest)>UI_PATH-100){message("Choose a backup destination.");return;}
 BPTR lock=Lock(dest,ACCESS_READ);struct FileInfoBlock *fib=AllocDosObject(DOS_FIB,NULL);
 int directory=lock&&fib&&Examine(lock,fib)&&fib->fib_DirEntryType>0;
 if(lock)UnLock(lock);if(fib)FreeDosObject(DOS_FIB,fib);
 if(!directory){message("Choose a mounted disk or directory.");return;}
 if(!prefs_write()){message("Cannot save current library settings; backup not started.");return;}
 DateStamp(&stamp);
 for(unsigned i=0;i<100;i++){
  snprintf(name,sizeof(name),LIB_TITLE "-%lx-%lx-%u",(unsigned long)stamp.ds_Days,(unsigned long)(stamp.ds_Minute*3000+stamp.ds_Tick),i);
  strcpy(manager_root,dest);if(!AddPart(manager_root,name,sizeof(manager_root)))break;
  if(manager_directory(manager_root)){made=1;break;}
  if(IoErr()!=ERROR_OBJECT_EXISTS)break;
 }
 if(!made){message("Cannot create backup folder on that disk.");return;}
 manager_mode=2;manager_enable(1);manager_phase=manager_battery=manager_suffix=manager_files=0;
 manager_copy=calloc(1,sizeof(*manager_copy));manager_fib=AllocDosObject(DOS_FIB,NULL);
 if(!manager_copy||!manager_fib){manager_finish("Not enough memory for backup.");return;}
 for(unsigned i=0;i<3;i++){
  strcpy(path,manager_root);if(!AddPart(path,i<2?manager_dirs[i]:"Battery",sizeof(path))||!manager_directory(path)){manager_finish("Cannot create backup subdirectory.");return;}
 }
 strcpy(path,manager_root);AddPart(path,"Backup.part",sizeof(path));manager_log=fopen(path,"w");
 if(!manager_log){manager_finish("Cannot create backup record.");return;}
 fprintf(manager_log,LIB_TITLE " save backup\nPaths below are hex-encoded. COMPLETE marks a verified backup.\n");
 strcpy(manager_partial,manager_root);AddPart(manager_partial,"Copy.part",sizeof(manager_partial));
 if(!manager_backup_file("PROGDIR:Library.cfg","Library.cfg"))manager_finish("Cannot back up library settings.");
}
static void manager_backup_step(void){
 char source[UI_PATH],relative[UI_PATH],note[240];
 if(manager_copy->in){
  int result=rm_copy_step(manager_copy);if(!result)return;
  if(result<0){manager_finish("Backup failed verification or disk I/O. Partial folder retained.");return;}
  manager_files++;fprintf(manager_log,"FILE ");ui_hex(manager_log,manager_copy->source);fputc(' ',manager_log);ui_hex(manager_log,manager_copy->target);fputc('\n',manager_log);
  if(ferror(manager_log)){manager_finish("Could not update backup record. Partial folder retained.");return;}
 }
 if(manager_phase<2){
  if(!manager_lock){
   snprintf(source,sizeof(source),"PROGDIR:%s",manager_dirs[manager_phase]);manager_lock=Lock(source,ACCESS_READ);
   if(!manager_lock){if(IoErr()==ERROR_OBJECT_NOT_FOUND){manager_phase++;return;}manager_finish("Cannot read save directory.");return;}
   if(!Examine(manager_lock,manager_fib)){manager_finish("Cannot examine save directory.");return;}
  }
  if(!ExNext(manager_lock,manager_fib)){
   LONG error=IoErr();UnLock(manager_lock);manager_lock=0;
   if(error!=ERROR_NO_MORE_ENTRIES){manager_finish("Error reading save directory.");return;}manager_phase++;return;
  }
  if(manager_fib->fib_DirEntryType!=ST_FILE)return;
  snprintf(relative,sizeof(relative),"%s/%s",manager_dirs[manager_phase],manager_fib->fib_FileName);
  strcpy(source,"PROGDIR:");if(!AddPart(source,relative,sizeof(source))||!manager_backup_file(source,relative))manager_finish("Cannot copy a save file. Partial backup retained.");
  return;
 }
 if(manager_battery<model.count){
  const char *suffix[]={".sav",".sav.bak"};unsigned index=manager_battery,kind=manager_suffix;
  if(++manager_suffix==2){manager_suffix=0;manager_battery++;}
  if(snprintf(source,sizeof(source),"%s%s",model.entry[index].path,suffix[kind])>=(int)sizeof(source)){manager_finish("Battery save path is too long.");return;}
  BPTR lock=Lock(source,ACCESS_READ);
  if(!lock){if(IoErr()!=ERROR_OBJECT_NOT_FOUND)manager_finish("Cannot read a battery save.");return;}
  int regular=Examine(lock,manager_fib)&&manager_fib->fib_DirEntryType<0;UnLock(lock);
  if(!regular){manager_finish("Battery save is not a regular file.");return;}
  snprintf(relative,sizeof(relative),"Battery/%08u%s",index+1,suffix[kind]);
  if(!manager_backup_file(source,relative))manager_finish("Cannot copy a battery save. Partial backup retained.");return;
 }
 int ok=fputs("COMPLETE\n",manager_log)>=0;if(fclose(manager_log))ok=0;manager_log=NULL;
 if(ok){char from[UI_PATH],to[UI_PATH];strcpy(from,manager_root);strcpy(to,manager_root);AddPart(from,"Backup.part",sizeof(from));AddPart(to,"Backup.txt",sizeof(to));ok=Rename(from,to)!=0;}
 if(ok)snprintf(note,sizeof(note),"Backed up %u files to %.175s",manager_files,manager_root);
 else snprintf(note,sizeof(note),"Could not finish backup record in %.175s",manager_root);
 manager_finish(note);
}
static void manager_step(void){
 if(manager_mode==1)manager_duplicate_step();else if(manager_mode==2)manager_backup_step();
 else if(manager_mode==3&&!scan_active){manager_mode=0;manager_enable(0);}
}
