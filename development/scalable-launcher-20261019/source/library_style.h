/* GPL-3.0-or-later. App-local MUI presentation; no global preference changes. */
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include "library_layout.h"
struct Library *DiskfontBase;
static struct TextFont *style_body,*style_title,*style_code;
static ULONG ui_font(void){return style_body?(ULONG)style_body:(ULONG)MUIV_Font_Normal;}
static ULONG ui_title_font(void){return style_title?(ULONG)style_title:(ULONG)MUIV_Font_Title;}
static ULONG ui_code_font(void){return style_code?(ULONG)style_code:(ULONG)MUIV_Font_Fixed;}
static void style_open(void){
 struct TextAttr normal={(STRPTR)"helvetica.font",13,FS_NORMAL,FPF_DISKFONT};
 struct TextAttr heading={(STRPTR)"helvetica.font",18,FSF_BOLD,FPF_DISKFONT};
 struct TextAttr code={(STRPTR)"courier.font",13,FS_NORMAL,FPF_DISKFONT};
 struct Screen *screen=LockPubScreen(NULL);
 ui_layout=ui_layout_for_screen(screen?screen->Width:640,screen?screen->Height:480,screen?screen->BarHeight:20);
 if(screen)UnlockPubScreen(NULL,screen);
#ifdef ZZ_UI_TEST_WIDTH
 ui_layout=ui_layout_for_screen(ZZ_UI_TEST_WIDTH,ZZ_UI_TEST_HEIGHT,20);
#endif
 ui_compact=ui_layout.compact;
 if(ui_compact){normal.ta_YSize=code.ta_YSize=11;heading.ta_YSize=13;}
 DiskfontBase=OpenLibrary("diskfont.library",36);
 if(GfxBase&&DiskfontBase){style_body=OpenDiskFont(&normal);style_title=OpenDiskFont(&heading);style_code=OpenDiskFont(&code);}
 if(ui_compact&&GfxBase){
  struct TextAttr fallback={(STRPTR)"topaz.font",8,FS_NORMAL,0};
  if(!style_body)style_body=OpenFont(&fallback);
  if(!style_title)style_title=OpenFont(&fallback);
  if(!style_code)style_code=OpenFont(&fallback);
 }
}
static void style_close(void){
 if(style_code)CloseFont(style_code);if(style_title)CloseFont(style_title);if(style_body)CloseFont(style_body);
 if(DiskfontBase)CloseLibrary(DiskfontBase);
}
static Object *ui_button(const char *text){return TextObject,ButtonFrame,
 MUIA_Font,ui_font(),MUIA_InputMode,MUIV_InputMode_RelVerify,MUIA_Background,MUII_ButtonBack,
 MUIA_CycleChain,1,MUIA_InnerTop,ui_compact?2:5,MUIA_InnerBottom,ui_compact?2:5,
 MUIA_Text_PreParse,(ULONG)"\33c",MUIA_Text_Contents,(ULONG)text,End;}
/* Shared rhythm keeps secondary tabs/forms visually consistent. */
#define UI_LIGHT_BACKGROUND "2:rdddddddd,dddddddd,dddddddd"
#define UI_PAGE MUIA_Group_Spacing,ui_compact?4:8,InnerSpacing(ui_compact?4:10,ui_compact?4:8)
#define UI_PANEL(name) GroupFrameT(name),MUIA_Background,MUII_GroupBack,MUIA_Group_Spacing,ui_compact?4:6,InnerSpacing(ui_compact?4:10,ui_compact?4:8)

/* MUI pen-spec background; scoped to this button, never global preferences. */
static Object *ui_launch_button(void){return TextObject,ButtonFrame,
 MUIA_Font,ui_font(),MUIA_InputMode,MUIV_InputMode_RelVerify,
 MUIA_Background,(ULONG)"2:re0e0e0e0,40404040,40404040",
 MUIA_CycleChain,1,MUIA_InnerTop,ui_compact?2:5,MUIA_InnerBottom,ui_compact?2:5,
 MUIA_Text_PreParse,(ULONG)"\33c\33b",MUIA_Text_Contents,(ULONG)"Launch",End;}
