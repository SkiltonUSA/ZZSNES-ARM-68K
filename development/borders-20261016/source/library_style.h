/* GPL-3.0-or-later. App-local MUI presentation; no global preference changes. */
#include <proto/diskfont.h>
#include <proto/graphics.h>
struct Library *DiskfontBase;

static struct TextFont *style_body,*style_title,*style_code;
static ULONG ui_font(void){return style_body?(ULONG)style_body:(ULONG)MUIV_Font_Normal;}
static ULONG ui_title_font(void){return style_title?(ULONG)style_title:(ULONG)MUIV_Font_Title;}
static ULONG ui_code_font(void){return style_code?(ULONG)style_code:(ULONG)MUIV_Font_Fixed;}
static void style_open(void){
 struct TextAttr normal={(STRPTR)"helvetica.font",13,FS_NORMAL,FPF_DISKFONT};
 struct TextAttr heading={(STRPTR)"helvetica.font",18,FSF_BOLD,FPF_DISKFONT};
 struct TextAttr code={(STRPTR)"courier.font",13,FS_NORMAL,FPF_DISKFONT};

 DiskfontBase=OpenLibrary("diskfont.library",36);
 if(GfxBase&&DiskfontBase){style_body=OpenDiskFont(&normal);style_title=OpenDiskFont(&heading);style_code=OpenDiskFont(&code);}
}
static void style_close(void){
 if(style_code)CloseFont(style_code);if(style_title)CloseFont(style_title);if(style_body)CloseFont(style_body);
 if(DiskfontBase)CloseLibrary(DiskfontBase);
}
static Object *ui_button(const char *text){return TextObject,ButtonFrame,
 MUIA_Font,ui_font(),MUIA_InputMode,MUIV_InputMode_RelVerify,MUIA_Background,MUII_ButtonBack,
 MUIA_CycleChain,1,MUIA_InnerTop,5,MUIA_InnerBottom,5,
 MUIA_Text_PreParse,(ULONG)"\33c",MUIA_Text_Contents,(ULONG)text,End;}
/* Shared rhythm keeps secondary tabs/forms visually consistent. */
#define UI_PAGE MUIA_Group_Spacing,8,InnerSpacing(10,8)
#define UI_PANEL(name) GroupFrameT(name),MUIA_Background,MUII_GroupBack,MUIA_Group_Spacing,6,InnerSpacing(10,8)
