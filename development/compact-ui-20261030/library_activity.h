/* GPL-3.0-or-later. Bounded, RAM-only MUI activity history. */
#define ACTIVITY_LIMIT 200u
#define ACTIVITY_SETTING_BASE 200u
static int ui_log_ready;
static Object *settings_list;
static unsigned activity_sequence,activity_watch_count;
struct activity_watch { Object *object; const char *label; ULONG attribute,last; const char **choices; };
static struct activity_watch activity_watches[72];
static void message(const char *text){
 char line[300];
 if(!ui_log_ready||!text||!*text)return;
 snprintf(line,sizeof(line),"%04u  %s",++activity_sequence,text);
 if(value(status_text,MUIA_List_Entries)>=ACTIVITY_LIMIT)DoMethod(status_text,MUIM_List_Remove,0);
 DoMethod(status_text,MUIM_List_InsertSingle,(ULONG)line,MUIV_List_Insert_Bottom);
 set(status_text,MUIA_List_Active,MUIV_List_Active_Bottom);
}
static void activity_selection(void){
 static char last[UI_PATH];char text[UI_TITLE+20],title[UI_TITLE];
 if(selected<0){last[0]=0;return;}
 const char *path=model.entry[selected].path;
 if(!strcmp(last,path))return;
 snprintf(last,sizeof(last),"%s",path);
 ui_title(&model.entry[selected],title);snprintf(text,sizeof(text),"Selected: %s",title);message(text);
}
static void activity_search(const char *query){
 static char last[UI_TITLE];char text[UI_TITLE+16];
 if(!strcmp(last,query))return;
 snprintf(last,sizeof(last),"%s",query);snprintf(text,sizeof(text),"Search: %s",*query?query:"cleared");message(text);
}
static void activity_watch(Object *object,const char *label,ULONG attribute,const char **choices){
 if(activity_watch_count>=sizeof(activity_watches)/sizeof(activity_watches[0]))return;
 struct activity_watch *w=&activity_watches[activity_watch_count];
 w->object=object;w->label=label;w->attribute=attribute;w->last=value(object,attribute);w->choices=choices;
 notify(object,attribute,MUIV_EveryTime,ACTIVITY_SETTING_BASE+activity_watch_count++);
}
static void setting_text(struct activity_watch *w,char *text,size_t size){
 ULONG n=value(w->object,w->attribute);
 if(w->choices)snprintf(text,size,"%s: %s",w->label,w->choices[n]);
 else if(w->attribute==MUIA_Selected)snprintf(text,size,"%s: %s",w->label,n?"on":"off");
 else snprintf(text,size,"%s: %lu",w->label,(unsigned long)n);
}
static void settings_refresh(void){
 char text[200];
 set(settings_list,MUIA_List_Quiet,TRUE);
 DoMethod(settings_list,MUIM_List_Clear);
 for(unsigned i=0;i<activity_watch_count;i++){
  setting_text(&activity_watches[i],text,sizeof(text));
  DoMethod(settings_list,MUIM_List_InsertSingle,(ULONG)text,MUIV_List_Insert_Bottom);
 }
 set(settings_list,MUIA_List_Quiet,FALSE);
}
static void activity_setting(ULONG id){
 if(id<ACTIVITY_SETTING_BASE||id>=ACTIVITY_SETTING_BASE+activity_watch_count)return;
 struct activity_watch*w=&activity_watches[id-ACTIVITY_SETTING_BASE];ULONG n=value(w->object,w->attribute);
 char text[200];
 if(n==w->last)return;w->last=n;
 setting_text(w,text,sizeof(text));message(text);settings_refresh();
}
static void activity_action(ULONG id){
 const char *text=NULL;
 switch(id){
 case ID_ADD:text="Add ROM: choose a file";break;
 case ID_REMOVE:text="Delete game requested";break;
 case ID_RENAME:text="Rename game requested";break;
 case ID_PLAY:text="Launch requested";break;
 case ID_RESUME:case ID_LATEST:text="Resume requested";break;
 case ID_SAVE:text="Saving settings";break;
 case ID_ART:text="Choose artwork";break;
 case ID_CLEAR_ART:text="Clear artwork requested";break;
 case ID_MANAGER_IMPORT:text="Import ROM folder requested";break;
 case ID_MANAGER_DUPLICATES:text="Duplicate scan requested";break;
 case ID_MANAGER_BACKUP:text="Save-state backup requested";break;
 case ID_MANAGER_CANCEL:text="Cancellation requested";break;
 case ID_MANAGER_BROWSE_IMPORT:text="Choose import directory";break;
 case ID_MANAGER_BROWSE_BACKUP:text="Choose backup directory";break;
#ifdef LIBRARY_SNES
 case ID_PROFILE_SAVE:text="Saving settings for selected game";break;
 case ID_PROFILE_RESET:text="Restore game defaults requested";break;
 case ID_PROFILE_GLOBAL:text="Editing global defaults";break;
#endif
 default:
  if(id>=ID_SLOT_RESUME&&id<ID_SLOT_RESUME+5)text="Resume saved slot requested";
  else if(id>=ID_SLOT_DELETE&&id<ID_SLOT_DELETE+5)text="Delete saved slot requested";
  break;
 }
 if(text)message(text);
}
