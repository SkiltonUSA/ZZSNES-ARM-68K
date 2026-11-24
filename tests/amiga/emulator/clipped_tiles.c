/* Compare the real clipped renderer with full-tile pixels and a clip oracle. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "gfx.h"
void pocket_heap_init(void) {}
#ifdef CLIPPED_ARM_PROBE
uint32 TileBlank;
#endif
void DrawTile16(uint32,int32,uint32,uint32);
void DrawClippedTile16(uint32,int32,uint32,uint32,uint32,uint32);
static uint8 tile[64] __attribute__((aligned(4))), cached[1]={1};
static uint32 colours[256];
static uint16 screen[GFX_PPL*10], full[GFX_PPL*10];
static uint8 depth[GFX_PPL*10], full_depth[GFX_PPL*10];
static void clear(unsigned pattern) {
    for(unsigned i=0;i<GFX_PPL*10;i++){screen[i]=0x1234;depth[i]=(i+pattern)%8;}
}
int main(void) {
#ifdef CLIPPED_ARM_PROBE
    const unsigned patterns=1;
#else
    const unsigned patterns=4;
#endif
    BG.TileAddress=0;BG.TileShift=5;BG.Buffer=tile;BG.Buffered=cached;
    GFX.S=(uint8*)screen;GFX.DB=depth;GFX.ScreenColorsPre=colours;
    GFX.PaletteMask=0;GFX.PaletteShift=0;GFX.Z1=5;GFX.Z2=7;
    unsigned cases=0;
    for(unsigned pattern=0;pattern<patterns;pattern++) {
        for(unsigned i=0;i<64;i++)tile[i]=(i+pattern)%16; /* includes transparent pixels */
        for(unsigned i=0;i<256;i++)colours[i]=0x4000+i*17+pattern;
        for(unsigned flip=0;flip<4;flip++)for(unsigned row=0;row<8;row++)
        for(unsigned lines=1;lines<=8-row;lines++) {
            unsigned flags=flip<<14,offset=GFX_PPL+16;
            clear(pattern);DrawTile16(flags,offset,row*8,lines);
            memcpy(full,screen,sizeof full);memcpy(full_depth,depth,sizeof depth);
            for(unsigned start=0;start<8;start++)for(unsigned width=0;width<=8-start;width++) {
                clear(pattern);DrawClippedTile16(flags,offset,start,width,row*8,lines);
                for(unsigned i=0;i<GFX_PPL*10;i++) {
                    unsigned y=i/GFX_PPL,x=i%GFX_PPL;
                    int inside=y>=1&&y<1+lines&&x>=16+start&&x<16+start+width;
                    unsigned pixel=inside?full[i]:0x1234;
                    unsigned z=inside?full_depth[i]:(i+pattern)%8;
                    if(screen[i]!=pixel||depth[i]!=z) {

#ifndef CLIPPED_ARM_PROBE
                        fprintf(stderr,"clip mismatch flip=%u row=%u lines=%u start=%u width=%u x=%u y=%u pixel=%x/%x depth=%u/%u\n",flip,row,lines,start,width,x,y,screen[i],pixel,depth[i],z);

#endif
                        return 1;
                    }
                }
                ++cases;
            }
        }
    }

#ifndef CLIPPED_ARM_PROBE
    printf("PASS clipped tile pixel/depth oracle: %u cases, all flips, rows, widths, transparency and depth\n",cases);
#endif
    return 0;
}
