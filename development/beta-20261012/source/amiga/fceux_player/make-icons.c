/* Create classic Workbench icons using our own two-bitplane controller artwork.
 * Build-time packaging helper only; not required or shipped with the app. */
#include <exec/types.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <stdio.h>
#include <string.h>
struct Library *IconBase;
static UWORD planes[2*3*32];
static struct Image image;
static struct DrawerData drawer;
static LONG icon_stack=65536;
static int make_icon(char *path,int type)
{
    struct DiskObject *d=NewDiskObject(type);APTR old;UWORD flags,width,height;BOOL ok;
    if(!d)return 0;
    if(type==WBDRAWER)d->do_DrawerData=&drawer;
    old=d->do_Gadget.GadgetRender;flags=d->do_Gadget.Flags;
    width=d->do_Gadget.Width;height=d->do_Gadget.Height;
    d->do_Gadget.GadgetRender=&image;d->do_Gadget.Flags=GFLG_GADGIMAGE|GFLG_GADGHCOMP;
    d->do_Gadget.Width=48;d->do_Gadget.Height=32;
    d->do_CurrentX=NO_ICON_POSITION;d->do_CurrentY=NO_ICON_POSITION;d->do_StackSize=icon_stack;
    ok=PutDiskObject(path,d);
    d->do_Gadget.GadgetRender=old;d->do_Gadget.Flags=flags;
    d->do_Gadget.Width=width;d->do_Gadget.Height=height;
    if(type==WBDRAWER)d->do_DrawerData=0;
    FreeDiskObject(d);
    if(!ok)return 0;
    d=GetDiskObject(path);if(!d)return 0;
    ok=d->do_Type==type && d->do_StackSize==icon_stack;
    printf("%s.info type=%d stack=%u image=%dx%d verified=%d\n",path,type,(unsigned)d->do_StackSize,d->do_Gadget.Width,d->do_Gadget.Height,ok);
    FreeDiskObject(d);return ok;
}
int main(int argc,char **argv)
{
    unsigned px,py,colour;char path[256];int ok;
    if(argc<2||argc>3||strlen(argv[1])>220)return 20;
    if(argc==3){if(strlen(argv[2])>30||strchr(argv[2],'/')||strchr(argv[2],':'))return 20;icon_stack=65536;}
    for(py=0;py<32;py++)for(px=0;px<48;px++) {
        /* Original stylised controller; four Workbench palette indices. */
        colour=0;
        if(px>=3&&px<45&&py>=7&&py<27)colour=2;
        if(px>=5&&px<43&&py>=9&&py<25)colour=1;
        if((px>=11&&px<15&&py>=12&&py<23)||(px>=8&&px<18&&py>=16&&py<20))colour=3;
        if((px>=31&&px<35&&py>=16&&py<20)||(px>=37&&px<41&&py>=12&&py<16))colour=3;
        if(px>=21&&px<28&&py==22)colour=2;
        if(colour&1)planes[py*3+px/16]|=1u<<(15-px%16);
        if(colour&2)planes[96+py*3+px/16]|=1u<<(15-px%16);
    }
    memset(&drawer,0,sizeof(drawer));
    drawer.dd_NewWindow.LeftEdge=80;drawer.dd_NewWindow.TopEdge=80;
    drawer.dd_NewWindow.Width=440;drawer.dd_NewWindow.Height=240;
    drawer.dd_NewWindow.DetailPen=0;drawer.dd_NewWindow.BlockPen=1;
    drawer.dd_NewWindow.Flags=WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SIZEGADGET;
    drawer.dd_NewWindow.MinWidth=100;drawer.dd_NewWindow.MinHeight=80;
    drawer.dd_NewWindow.MaxWidth=0xffff;drawer.dd_NewWindow.MaxHeight=0xffff;
    memset(&image,0,sizeof(image));image.Width=48;image.Height=32;image.Depth=2;
    image.ImageData=planes;image.PlanePick=3;
    IconBase=OpenLibrary("icon.library",44);if(!IconBase)return 20;
    ok=make_icon(argv[1],WBDRAWER);
    snprintf(path,sizeof(path),"%s/%s",argv[1],argc==3?argv[2]:"ZZFceux");ok=make_icon(path,WBTOOL)&&ok;
    CloseLibrary(IconBase);return ok?0:20;
}
