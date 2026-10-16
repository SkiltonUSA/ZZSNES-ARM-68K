/* GPL-3.0-or-later. Native embedded branding; same classic artwork path as ZZDosBox. */
#include "sidebar-art.h"
#include "splash-art.h"
#ifndef SIDEBAR_DEPTH
#define SIDEBAR_DEPTH 2
#define SPLASH_DEPTH 2
#define sidebar_colours brand_colours
#define splash_colours brand_colours
static const ULONG brand_colours[]={0,0,0,0,0,0,0xffffffff,0xffffffff,0xffffffff,0,0,0};
#endif
static Object*library_logo(unsigned size){return BodychunkObject,
 MUIA_FixWidth,size,MUIA_FixHeight,size,MUIA_Bitmap_Width,size,MUIA_Bitmap_Height,size,
 MUIA_Bodychunk_Depth,(size==80?SIDEBAR_DEPTH:SPLASH_DEPTH),MUIA_Bodychunk_Body,(ULONG)(size==80?sidebar_body:splash_body),
 MUIA_Bodychunk_Compression,0,MUIA_Bodychunk_Masking,0,
 MUIA_Bitmap_SourceColors,(ULONG)(size==80?sidebar_colours:splash_colours),MUIA_Bitmap_Transparent,0,End;}

#include "launch-art.h"
/* Area's RelVerify input retains the existing MUI pressed notification,
 * keyboard focus, disabled overlay and raised/sunken button frame. */
static Object *ui_launch_button(void){return BodychunkObject,ButtonFrame,
 MUIA_InputMode,MUIV_InputMode_RelVerify,MUIA_CycleChain,1,
 MUIA_Background,MUII_ButtonBack,InnerSpacing(4,4),
 MUIA_FixWidth,LAUNCH_WIDTH+12,MUIA_FixHeight,LAUNCH_HEIGHT+12,
 MUIA_Bitmap_Width,LAUNCH_WIDTH,MUIA_Bitmap_Height,LAUNCH_HEIGHT,
 MUIA_Bodychunk_Depth,LAUNCH_DEPTH,MUIA_Bodychunk_Body,(ULONG)launch_body,
 MUIA_Bodychunk_Compression,0,MUIA_Bodychunk_Masking,0,
 MUIA_Bitmap_SourceColors,(ULONG)launch_colours,MUIA_Bitmap_Transparent,0,
 MUIA_ShortHelp,(ULONG)"Launch selected game",End;}
