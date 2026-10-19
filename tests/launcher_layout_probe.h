#ifdef ZZ_UI_OPTIONS_PROBE
#include "expanded_options_ui_probe.h"
#endif
/* Included only by the diagnostic build. Never changes a screen mode or saves. */
#ifndef ZZ_UI_CAPTURE_TICKS
#define ZZ_UI_CAPTURE_TICKS 3000
#endif
static void layout_pump(void){
 ULONG signals;unsigned i;
 for(i=0;i<12;i++){DoMethod(app,MUIM_Application_NewInput,(ULONG)&signals);Delay(1);}
}
static void layout_page(unsigned page){
 set(pages,MUIA_Group_ActivePage,page);set(navigation,MUIA_List_Active,page);
 set(heading,MUIA_Text_Contents,(ULONG)tabs[page]);
}
static void layout_stage(const char *name){FILE*f=fopen("PROGDIR:layout-stage.txt","w");if(f){fputs(name,f);fclose(f);}}
static int layout_probe(void){
 FILE *report=freopen("PROGDIR:layout-report.txt","w",stdout);
 struct List *children;APTR state;Object *scroll,*virt;unsigned page=0;int errors=0;
 struct Window *native;
 layout_pump();native=(struct Window*)value(win,MUIA_Window_Window);
 if(!report)return 20;
 if(needs_details){needs_details=0;identify();update_slots();}
 printf("LIMITS %d %d / %d %d\n",native->MinWidth,native->MinHeight,native->MaxWidth,native->MaxHeight);
 printf("LAYOUT screen budget %u x %u compact %d window %u x %u\n",ZZ_UI_TEST_WIDTH,ZZ_UI_TEST_HEIGHT,ui_compact,native->Width,native->Height);
 if(native->Width>ZZ_UI_TEST_WIDTH||native->Height>ZZ_UI_TEST_HEIGHT-20)errors++;
 get(pages,MUIA_Group_ChildList,&children);state=children->lh_Head;
 while((scroll=NextObject(&state))){
  layout_page(page);layout_pump();virt=(Object*)value(scroll,MUIA_Scrollgroup_Contents);
  printf("PAGE %u %s viewport %ld x %ld content %lu x %lu\n",page,tabs[page],(long)_mwidth(virt),(long)_mheight(virt),(unsigned long)value(virt,MUIA_Virtgroup_Width),(unsigned long)value(virt,MUIA_Virtgroup_Height));
  if(_mwidth(virt)<180||_mheight(virt)<100)errors++;
  SetAttrs(virt,MUIA_Virtgroup_Left,10000,MUIA_Virtgroup_Top,10000,TAG_DONE);layout_pump();
  printf("SCROLL %lu %lu\n",(unsigned long)value(virt,MUIA_Virtgroup_Left),(unsigned long)value(virt,MUIA_Virtgroup_Top));
  SetAttrs(virt,MUIA_Virtgroup_Left,0,MUIA_Virtgroup_Top,0,TAG_DONE);
  fflush(stdout);page++;
 }
 if(page!=LIB_PAGE_COUNT)errors++;
 /* Exercise real MUI resizing, without injecting a held mouse gesture. */
 {const unsigned widths[]={480,608,768};unsigned i;
  for(i=0;i<3;i++){
   ChangeWindowBox(native,native->LeftEdge,native->TopEdge,widths[i],360);layout_pump();
   printf("RESIZE requested %u x 360 actual %u x %u\n",widths[i],native->Width,native->Height);
   if((unsigned)native->Width!=widths[i]||native->Height!=360)errors++;
  }
  ChangeWindowBox(native,native->LeftEdge,native->TopEdge,ui_layout.width,ui_layout.height);layout_pump();
 }
#ifdef ZZ_UI_OPTIONS_PROBE
 errors+=expanded_options_ui_probe();
#endif
 /* Keep a real small window available for a screenshot, without mouse drags. */
 layout_page(0);layout_pump();layout_stage("library");Delay(ZZ_UI_CAPTURE_TICKS);
 layout_page(1);layout_pump();layout_stage("display");Delay(ZZ_UI_CAPTURE_TICKS);
 layout_page(2);layout_pump();layout_stage("controllers");Delay(ZZ_UI_CAPTURE_TICKS);
 {Object *splash=splash_create(1);struct Window *w=splash?(struct Window*)value(splash,MUIA_Window_Window):NULL;
  if(!w||w->Width>ZZ_UI_TEST_WIDTH||w->Height>ZZ_UI_TEST_HEIGHT-20)errors++;
  if(w)printf("LOADING %u x %u\n",w->Width,w->Height);
  layout_pump();loading_window=splash;splash_close();
 }
 printf("LAYOUT %s errors=%d pages=%u\n",errors?"FAIL":"PASS",errors,page);fflush(stdout);
 layout_stage("done");return errors?20:0;
}
