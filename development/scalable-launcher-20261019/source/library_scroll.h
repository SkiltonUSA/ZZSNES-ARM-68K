/* GPL-3.0-or-later. MUI 3.8 built-in scrolling; no external MCC required. */
static struct MUI_CustomClass *viewport_class;
static ULONG viewport_dispatch(struct IClass *cl __asm("a0"),Object *obj __asm("a2"),Msg msg __asm("a1")){
 ULONG result=DoSuperMethodA(cl,obj,msg);
 if(msg->MethodID==MUIM_AskMinMax){
  struct MUIP_AskMinMax *m=(struct MUIP_AskMinMax*)msg;
  /* A short/fixed-width page must not cap the whole window's maximum size.
   * The inherited minimum and virtual content layout remain MUI-owned. */
  m->MinMaxInfo->MaxWidth=MUI_MAXMAX;m->MinMaxInfo->MaxHeight=MUI_MAXMAX;
 }
 return result;
}
static int ui_scroll_pages(Object *pages){
 Object *children[LIB_PAGE_COUNT],*child,*scroll;struct List *list;APTR state;
 unsigned count=0,i;
 get(pages,MUIA_Group_ChildList,&list);state=list->lh_Head;
 while((child=NextObject(&state))&&count<LIB_PAGE_COUNT)children[count++]=child;
 /* Wrap separately: a wide developer page must not widen the library. */
 for(i=0;i<count;i++){
  DoMethod(pages,OM_REMMEMBER,(ULONG)children[i]);
  scroll=ScrollgroupObject,
   MUIA_Scrollgroup_FreeHoriz,TRUE,MUIA_Scrollgroup_FreeVert,TRUE,
   MUIA_Scrollgroup_Contents,NewObject(viewport_class->mcc_Class,NULL,
    MUIA_Group_Spacing,0,MUIA_InnerLeft,0,MUIA_InnerRight,0,
    MUIA_InnerTop,0,MUIA_InnerBottom,0,MUIA_Group_Child,children[i],TAG_DONE),End;
  if(!scroll)return 0;
  DoMethod(pages,OM_ADDMEMBER,(ULONG)scroll);
 }
 return 1;
}
