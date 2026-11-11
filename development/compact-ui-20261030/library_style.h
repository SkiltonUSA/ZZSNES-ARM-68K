/* GPL-3.0-or-later. Match ZZ8-Bit's launcher presentation; never change global preferences. */
#include "library_layout.h"
#include <proto/diskfont.h>
struct Library *DiskfontBase;
static struct TextFont *console_font;
static ULONG ui_font(void){return console_font?(ULONG)console_font:(ULONG)MUIV_Font_Normal;}
static ULONG ui_title_font(void){return (ULONG)MUIV_Font_Normal;}
static ULONG ui_code_font(void){return (ULONG)MUIV_Font_Fixed;}
static void style_open(void){
 struct Screen *screen=LockPubScreen(NULL);
 ui_layout=ui_layout_for_screen(screen?screen->Width:640,screen?screen->Height:480,screen?screen->BarHeight:20);
 if(screen)UnlockPubScreen(NULL,screen);
#ifdef ZZ_UI_TEST_WIDTH
 ui_layout=ui_layout_for_screen(ZZ_UI_TEST_WIDTH,ZZ_UI_TEST_HEIGHT,20);
#endif
 ui_compact=ui_layout.compact;
 struct TextAttr attr={(STRPTR)"helvetica.font",13,FS_NORMAL,FPF_DISKFONT};
 if(!ui_compact){DiskfontBase=OpenLibrary("diskfont.library",36);if(DiskfontBase)console_font=OpenDiskFont(&attr);}
 if(!console_font){attr.ta_Name=(STRPTR)"topaz.font";attr.ta_YSize=8;console_font=OpenFont(&attr);}
}
static void style_close(void){if(console_font)CloseFont(console_font);if(DiskfontBase)CloseLibrary(DiskfontBase);}
static Object *ui_button(const char *text){return TextObject,ButtonFrame,
 MUIA_Font,ui_font(),MUIA_InputMode,MUIV_InputMode_RelVerify,MUIA_Background,MUII_ButtonBack,
 MUIA_CycleChain,1,MUIA_Text_SetMax,TRUE,MUIA_HorizWeight,0,MUIA_InnerTop,1,MUIA_InnerBottom,1,
 MUIA_Text_PreParse,(ULONG)"\33c",MUIA_Text_Contents,(ULONG)text,End;}
#define UI_LIGHT_BACKGROUND "2:rdddddddd,dddddddd,dddddddd"
#define UI_PAGE MUIA_Group_Spacing,4,InnerSpacing(4,4)
#define UI_PANEL(name) GroupFrameT(name),MUIA_Background,MUII_GroupBack,MUIA_Group_Spacing,4,InnerSpacing(4,4)
static Object *ui_launch_button(void){return TextObject,ButtonFrame,
 MUIA_Font,ui_font(),MUIA_InputMode,MUIV_InputMode_RelVerify,
 MUIA_Background,(ULONG)"2:rb0b0b0b0,44444444,33333333",
 MUIA_CycleChain,1,MUIA_Text_SetMax,TRUE,MUIA_HorizWeight,0,MUIA_InnerTop,1,MUIA_InnerBottom,1,
 MUIA_Text_PreParse,(ULONG)"\33c\33b",MUIA_Text_Contents,(ULONG)"Launch",End;}

static Object *ui_tab(const char *text){return TextObject,ButtonFrame,MUIA_Font,ui_font(),MUIA_Background,MUII_ButtonBack,MUIA_InputMode,MUIV_InputMode_RelVerify,MUIA_CycleChain,1,MUIA_Text_PreParse,(ULONG)"\33c",MUIA_Text_Contents,(ULONG)text,End;}

#include "about-art.h"
static Object *ui_about(void){return BodychunkObject,
 MUIA_FixWidth,ABOUT_WIDTH,MUIA_FixHeight,ABOUT_HEIGHT,InnerSpacing(0,0),
 MUIA_InputMode,MUIV_InputMode_RelVerify,MUIA_CycleChain,TRUE,
 MUIA_ControlChar,'a',MUIA_ShortHelp,(ULONG)"About",
 MUIA_Bitmap_Width,ABOUT_WIDTH,MUIA_Bitmap_Height,ABOUT_HEIGHT,MUIA_Bodychunk_Depth,4,
 MUIA_Bodychunk_Body,(ULONG)about_body,MUIA_Bodychunk_Compression,0,
 MUIA_Bodychunk_Masking,0,MUIA_Bitmap_SourceColors,(ULONG)about_colours,
 MUIA_Bitmap_Transparent,0,End;}
