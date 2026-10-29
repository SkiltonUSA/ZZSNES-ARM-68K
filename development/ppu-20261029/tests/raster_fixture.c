/* Original randomized sprite/window/palette fixture; no commercial game data. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "gfx.h"
int nes_init(const uint8_t *, uint32_t, uint16_t *, uint16_t *);
void pocket_player_close(void);
void pocket_heap_init(void) {}
void DrawOBJS(bool8_32, uint8);
void S9xSetupOBJ(void);
static uint32_t rng = 0x5afef00d;
static uint32_t random_word(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static uint32_t hash(const void *data, unsigned n) {
    const uint8_t *p = data; uint32_t h = 2166136261u;
    while (n--) h = (h ^ *p++) * 16777619u;
    return h;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    uint8_t *rom = calloc(1, 0x10000); assert(rom);
    memset(rom + 0x7fc0, ' ', 21); memcpy(rom + 0x7fc0, "ZZ RASTER FIXTURE", 17);
    rom[0] = 0x80; rom[1] = 0xfe; rom[0x7fd5] = 0x20;
    rom[0x7fd7] = 6; rom[0x7fd9] = 1;
    rom[0x7fdc] = rom[0x7fdd] = 255; rom[0x7ffd] = 128;
    assert(!nes_init(rom, 0x10000, 0, 0));
    FILE *f = fopen(argv[1], "w"); assert(f);
    PPU.ScreenHeight = 224;
    /* Only visible clipping is compared: disabled layers are recomputed when
       enabled. Write through the real PPU interface, including irrelevant bits. */
    static const unsigned registers[] = {0x2123,0x2124,0x2125,0x2126,0x2127,
        0x2128,0x2129,0x212a,0x212b,0x212c,0x212d,0x212e,0x212f,0x2130};
    extern void ComputeClipWindows(void);
    for (unsigned i = 0; i < 8192; i++) {
        unsigned address = registers[random_word() % (sizeof(registers)/sizeof(registers[0]))];
        S9xSetPPU(random_word(), address);
        if (PPU.RecomputeClipWindows) ComputeClipWindows();
        uint32_t h = 2166136261u;
        for (unsigned screen = 0; screen < 2; screen++) for (unsigned layer = 0; layer < 6; layer++) {
            if (layer < 5 && !(Memory.FillRAM[0x212c + screen] & (1u << layer))) continue;
            unsigned count = IPPU.Clip[screen].Count[layer]; h = (h ^ count) * 16777619u;
            for (unsigned band = 0; band < count; band++) {
                h = (h ^ IPPU.Clip[screen].Left[band][layer]) * 16777619u;
                h = (h ^ IPPU.Clip[screen].Right[band][layer]) * 16777619u;
            }
        }
        fprintf(f, "window %u %08x\n", i, h);
    }
    for (unsigned test = 0; test < 2048; test++) {
        for (unsigned i = 0; i < 0x10000; i++) Memory.VRAM[i] = random_word();
        memset(IPPU.TileCached[TILE_4BIT], 0, 0x10000 >> 5);
        for (unsigned i = 0; i < 256; i++) IPPU.ScreenColors[i] = random_word() & 65535;
        for (unsigned i = 0; i < 128; i++) {
            PPU.OBJ[i].HPos = (int)(random_word() % 512) - 256;
            PPU.OBJ[i].VPos = random_word() % 256;
            PPU.OBJ[i].Name = random_word() % 512;
            PPU.OBJ[i].Palette = random_word() % 8;
            PPU.OBJ[i].Priority = random_word() % 4;
            PPU.OBJ[i].Size = random_word() & 1;
            PPU.OBJ[i].HFlip = random_word() & 1;
            PPU.OBJ[i].VFlip = random_word() & 1;
        }
        PPU.OBJSizeSelect = test % 6; PPU.FirstSprite = test % 128;
        PPU.OBJNameBase = (test % 8) * 0x2000; PPU.OBJNameSelect = 0x2000;
        GFX.StartY = random_word() % 200; GFX.EndY = GFX.StartY + random_word() % 24;
        GFX.S = GFX.Screen; GFX.DB = GFX.ZBuffer; GFX.Pseudo = 0;
        GFX.r2130 = (test & 1) ? 2 : 0;
        GFX.r2131 = 16 | ((test & 2) ? 0x40 : 0) | ((test & 4) ? 0x80 : 0);
        GFX.FixedColour = random_word() & 65535;
        memset(&IPPU.Clip[0], 0, sizeof(IPPU.Clip[0])); GFX.pCurrentClip = &IPPU.Clip[0];
        IPPU.Clip[0].Count[4] = test % 3;
        IPPU.Clip[0].Left[0][4] = random_word() % 64;
        IPPU.Clip[0].Right[0][4] = 64 + random_word() % 64;
        IPPU.Clip[0].Left[1][4] = 128 + random_word() % 64;
        IPPU.Clip[0].Right[1][4] = 192 + random_word() % 65;
        for (unsigned i = 0; i < GFX_PPL * 240; i++) {
            ((uint16_t *)GFX.Screen)[i] = 0x1234;
            ((uint16_t *)GFX.SubScreen)[i] = random_word();
            GFX.ZBuffer[i] = random_word() & 7;
            GFX.SubZBuffer[i] = random_word() & (GFX.r2130 & 2 ? 7 : 1);
        }
        S9xSetupOBJ(); DrawOBJS(1, 0);
        fprintf(f, "%u %08x %08x\n", test, hash(GFX.Screen, GFX_PITCH * 240), hash(GFX.ZBuffer, GFX_ZPITCH * 240));
    }
    assert(!fclose(f)); pocket_player_close(); free(rom); return 0;
}
