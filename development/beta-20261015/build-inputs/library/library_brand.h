/* GPL-3.0-or-later. Native embedded branding; same classic artwork path as ZZDosBox. */
#include "sidebar-art.h"
#include "splash-art.h"
static const ULONG brand_colours[]={0,0,0,0,0,0,0xffffffff,0xffffffff,0xffffffff,0,0,0};
static Object*library_logo(unsigned size){return BodychunkObject,
 MUIA_FixWidth,size,MUIA_FixHeight,size,MUIA_Bitmap_Width,size,MUIA_Bitmap_Height,size,
 MUIA_Bodychunk_Depth,2,MUIA_Bodychunk_Body,(ULONG)(size==80?sidebar_body:splash_body),
 MUIA_Bodychunk_Compression,0,MUIA_Bodychunk_Masking,0,
 MUIA_Bitmap_SourceColors,(ULONG)brand_colours,MUIA_Bitmap_Transparent,0,End;}
