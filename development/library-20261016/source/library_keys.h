/* GPL-3.0-or-later. Focus-local keyboard navigation for program lists. */
#include <devices/inputevent.h>
#include <proto/keymap.h>
struct Library *KeymapBase;
static struct MUI_CustomClass *program_list_class;
struct program_list_data {struct MUI_EventHandlerNode keys;};

static ULONG program_list_dispatch(struct IClass *cl __asm("a0"),
 Object *obj __asm("a2"),Msg msg __asm("a1")){
 struct program_list_data *d=INST_DATA(cl,obj);
 switch(msg->MethodID){
 case MUIM_Setup:
  if(!DoSuperMethodA(cl,obj,msg))return FALSE;
  memset(&d->keys,0,sizeof(d->keys));
  d->keys.ehn_Object=obj;d->keys.ehn_Class=cl;
  d->keys.ehn_Events=IDCMP_RAWKEY;d->keys.ehn_Priority=10;
  d->keys.ehn_Flags=MUI_EHF_GUIMODE;
  DoMethod(_win(obj),MUIM_Window_AddEventHandler,(ULONG)&d->keys);
  return TRUE;
 case MUIM_Cleanup:
  DoMethod(_win(obj),MUIM_Window_RemEventHandler,(ULONG)&d->keys);
  break;
 case MUIM_HandleEvent:{
  struct IntuiMessage *im=((struct MUIP_HandleEvent*)msg)->imsg;
  ULONG active=0,inner=0,count=0;LONG row=-1;unsigned code;char key;
  struct InputEvent event={0};
  if(!im||im->Class!=IDCMP_RAWKEY)break;
  get(_win(obj),MUIA_Window_ActiveObject,&active);
  get(obj,MUIA_Listview_List,&inner);
  /* Do not intercept typing in editors, shortcuts, or another list. */
  if(active!=(ULONG)obj&&active!=inner)break;
  if(im->Code&IECODE_UP_PREFIX)break;
  if(im->Qualifier&(IEQUALIFIER_CONTROL|IEQUALIFIER_LALT|IEQUALIFIER_RALT|
                     IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND))break;
  code=im->Code;get(obj,MUIA_List_Entries,&count);get(obj,MUIA_List_Active,&row);
  if(code==0x4c||code==0x4d){
   if(count){
    if(row<0)row=code==0x4c?(LONG)count-1:0;
    else if(code==0x4c&&row>0)row--;
    else if(code==0x4d&&row<(LONG)count-1)row++;
    set(obj,MUIA_List_Active,row);
   }
   return MUI_EventHandlerRC_Eat;
  }
  event.ie_Class=IECLASS_RAWKEY;event.ie_Code=im->Code;
  event.ie_Qualifier=im->Qualifier;
  if(MapRawKey(&event,&key,1,NULL)==1){
   unsigned ch=(unsigned char)key;
   if(ch>='A'&&ch<='Z')ch+='a'-'A';
   if((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')){
    ULONG i;
    for(i=0;i<count;i++){
     char *entry=NULL;unsigned first;
     DoMethod(obj,MUIM_List_GetEntry,i,(ULONG)&entry);
     if(!entry)continue;
     while(*entry==' '||*entry=='*')entry++;first=(unsigned char)*entry;
     if(first>='A'&&first<='Z')first+='a'-'A';
     if(first==ch){set(obj,MUIA_List_Active,i);break;}
    }
    return MUI_EventHandlerRC_Eat;
   }
  }
  break;
 }
 }
 return DoSuperMethodA(cl,obj,msg);
}
static Object *program_list_new(Object **list_out){
 return NewObject(program_list_class->mcc_Class,NULL,MUIA_CycleChain,1,
  MUIA_Listview_Input,TRUE,
  MUIA_ShortHelp,(ULONG)"Up/Down: select a game. Letter or number: jump to the first matching title.",
  MUIA_Listview_List,*list_out=ListObject,MUIA_List_AutoVisible,TRUE,
   MUIA_Font,ui_font(),InputListFrame,
   MUIA_List_ConstructHook,MUIV_List_ConstructHook_String,
   MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,TAG_DONE);
}
