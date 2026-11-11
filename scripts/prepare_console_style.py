"""ZZ8-Bit presentation for the fixed-console libraries; no core changes."""
from pathlib import Path

def once(s,a,b):
    assert s.count(a)==1,a[:100]
    return s.replace(a,b,1)

def prepare(lib):
    from about_art import build
    build(lib)
    p=lib/'library_mui.h';s=p.read_text()
    s=s.replace('*navigation,*heading,','*heading,')
    s=once(s,'static char save_heading', 'static Object *root_group,*tab_buttons[7],*details_panel,*details_button,*status_line;\nstatic int details_visible;\nstatic char save_heading')
    begin=s.index('  Child,HGroup,MUIA_Group_Spacing,ui_compact?4:12,')
    end=s.index('    Child,pages=VGroup,MUIA_Group_PageMode,TRUE,',begin)
    s=s[:begin]+'''  Child,HGroup,MUIA_VertWeight,0,
   Child,heading=TextObject,MUIA_Font,ui_font(),MUIA_Text_SetMin,FALSE,MUIA_Text_PreParse,(ULONG)"\\33b",MUIA_Text_Contents,(ULONG)LIB_TITLE,End,
   Child,HSpace(0),Child,save=manager_settings_save=ui_button("Save settings"),Child,tab_buttons[6]=ui_about(),End,
  Child,ColGroup(ui_compact?3:6),MUIA_VertWeight,0,MUIA_Group_Spacing,3,
   Child,tab_buttons[0]=ui_tab("Games"),Child,tab_buttons[5]=ui_tab("ROM Manager"),
   Child,tab_buttons[1]=ui_tab("Display"),Child,tab_buttons[4]=ui_tab("Sound"),
   Child,tab_buttons[2]=ui_tab("Controllers"),Child,tab_buttons[3]=ui_tab("Save states"),
   End,
'''+s[end:]
    s=once(s,'  End,\n   End,\n  End,\n  Child,ListviewObject,GroupFrameT("Activity"),','  End,\n  Child,details_panel=VGroup,MUIA_ShowMe,FALSE,MUIA_VertWeight,0,\n  Child,ListviewObject,GroupFrameT("Details"),')
    s=once(s,'MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,End,\n  Child,HGroup,Child,start=ui_launch_button(),Child,save=manager_settings_save=ui_button("Save settings"),Child,exit=ui_button("Quit"),Child,HSpace(0),', 'MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,End,End,\n  Child,HGroup,MUIA_VertWeight,0,Child,status_line=TextObject,MUIA_Text_SetMin,FALSE,MUIA_Text_Contents,(ULONG)"Ready",End,Child,details_button=ui_button("Details"),Child,start=ui_launch_button(),Child,exit=ui_button("Close"),')
    s=s.replace('WindowContents,VGroup,UI_PAGE,','WindowContents,root_group=VGroup,UI_PAGE,MUIA_Font,ui_font(),MUIA_Background,(ULONG)UI_LIGHT_BACKGROUND,')
    a=' for(unsigned i=0;tabs[i];i++)DoMethod(navigation,MUIM_List_InsertSingle,(ULONG)tabs[i],MUIV_List_Insert_Bottom);\n set(navigation,MUIA_List_Active,0);notify(navigation,MUIA_List_Active,MUIV_EveryTime,ID_NAV);'
    s=once(s,a,' for(unsigned i=0;i<7;i++)notify(tab_buttons[i],MUIA_Pressed,FALSE,700+i);\n notify(details_button,MUIA_Pressed,FALSE,710);select_page(0);')
    a='case ID_NAV:{ULONG n=value(navigation,MUIA_List_Active);if(n<7){set(pages,MUIA_Group_ActivePage,n);set(heading,MUIA_Text_Contents,(ULONG)tabs[n]);message(tabs[n]);}}break;'
    s=once(s,a,'case ID_NAV:break;')
    s=s.replace('case ID_SAVES:set(navigation,MUIA_List_Active,3);break;','case ID_SAVES:select_page(3);break;')
    s=once(s,'  activity_action(id);','  if(id>=700&&id<707){select_page(id-700);id=0;}\n  if(id==710){show_details(!details_visible);id=0;}\n  activity_action(id);')
    pos=s.index('static int build(void)')
    s=s[:pos]+'''static void show_details(int visible){
 details_visible=visible;
 DoMethod(root_group,MUIM_Group_InitChange);
 set(details_panel,MUIA_ShowMe,visible);
 set(details_button,MUIA_Text_Contents,(ULONG)(visible?"Hide details":"Details"));
 DoMethod(root_group,MUIM_Group_ExitChange);
}
static void select_page(unsigned page){
 set(pages,MUIA_Group_ActivePage,page);
 for(unsigned i=0;i<6;i++)set(tab_buttons[i],MUIA_Background,i==page?(ULONG)"2:r77777777,99999999,bbbbbbbb":(ULONG)MUII_ButtonBack);
 message(tabs[page]);
}
'''+s[pos:]
    s=once(s,'Child,VGroup,GroupFrameT("Selected game"),MUIA_Weight,55,','Child,VGroup,GroupFrame,MUIA_HorizWeight,0,MUIA_FixWidth,ui_compact?230:270,')
    s=s.replace('Child,VGroup,GroupFrameT("Games"),MUIA_Weight,50,','Child,VGroup,MUIA_Weight,100,')
    s=s.replace('Child,HGroup,Child,hero=preview_new(),Child,VGroup,','Child,VGroup,Child,HGroup,Child,HSpace(0),Child,hero=preview_new(),Child,HSpace(0),End,Child,VGroup,')
    s=once(s,'Child,art_button=ui_button("Artwork..."),Child,clear_art=ui_button("Clear artwork"),End,End,','Child,HGroup,Child,art_button=ui_button("Artwork..."),Child,clear_art=ui_button("Clear"),Child,HSpace(0),End,End,End,')
    s=s.replace('MUIA_FixHeight,ui_compact?72:120,','MUIA_FixHeight,ui_compact?72:96,')
    s=s.replace('ui_compact?0x534e5531:0x534e5530','ui_compact?0x534e5535:0x534e5534').replace('ui_compact?0x4e455531:0x4e455530','ui_compact?0x4e455535:0x4e455534')
    s=s.replace('return CycleObject,MUIA_Font,ui_font(),','return CycleObject,MUIA_FixWidth,ui_compact?170:210,MUIA_HorizWeight,0,MUIA_CycleChain,TRUE,MUIA_Font,ui_font(),')
    p.write_text(s)
    p=lib/'library_activity.h';s=p.read_text().replace(' snprintf(line,sizeof(line),',' set(status_line,MUIA_Text_Contents,(ULONG)text);\n snprintf(line,sizeof(line),',1);p.write_text(s)

    p=lib/'library_manager.h';s=p.read_text();s=s.replace('set(navigation,MUIA_Disabled,busy);','for(unsigned i=0;i<7;i++)set(tab_buttons[i],MUIA_Disabled,busy);');p.write_text(s)

    # The Games page already scrolls its lists, so omit its outer Scrollgroup.
    p=lib/'library_scroll.h';s=p.read_text()
    s=once(s,'  scroll=ScrollgroupObject,','  if(i==0){\n   scroll=NewObject(viewport_class->mcc_Class,NULL,\n    MUIA_Group_Spacing,0,MUIA_InnerLeft,0,MUIA_InnerRight,0,\n    MUIA_InnerTop,0,MUIA_InnerBottom,0,MUIA_Group_Child,children[i],TAG_DONE);\n  }else scroll=ScrollgroupObject,')
    p.write_text(s)
