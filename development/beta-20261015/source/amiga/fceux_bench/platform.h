// GPL-3.0-or-later. Headless platform declarations; emulation stays upstream.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#ifndef FC_ARM
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#else
extern "C" void *memcpy(void *,const void *,size_t);
extern "C" void *memset(void *,int,size_t);
extern "C" void *memmove(void *,const void *,size_t);
extern "C" int memcmp(const void *,const void *,size_t);
extern "C" int printf(const char *,...);
extern "C" void free(void *);
#endif
#include "types.h"
// Upstream PPU uses word casts over byte-aligned scanlines. Preserve values
// without promising natural alignment or violating byte-buffer aliasing.
typedef uint32 FC_Unaligned32 __attribute__((aligned(1),may_alias));
#include "fceu.h"
#include "x6502.h"
#include "ppu.h"
#include "sound.h"
#include "cart.h"
#include "filter.h"
struct SFORMAT { void *v;uint32 s;const char *desc; };
#define FCEUSTATE_RLSB 0x80000000u
#define FCEUSTATE_INDIRECT 0x40000000u
void AddExState(void *,uint32,int,const char *);
void ResetExState(void (*)(void),void (*)(void));
void FCEU_dwmemset(void *,uint32,int);
void *FCEU_malloc(uint32);
void *FCEU_gmalloc(uint32);
void FCEU_gfree(void *);
void FCEU_CheatAddRAM(int,uint32,uint8 *);
void InputScanlineHook(uint8 *,uint8 *,uint32,int);
void DoNSFFrame(void);
void MMC5_hb(int);
void FCEU_WriteWaveData(int32 *,int);
void SetNESDeemph_OldHacky(uint8,int);
extern uint8 *XBuf,*XBackBuf;
extern uint8 **VPageR;
extern uint8 *vnapage[4];
extern int NSFReload;
extern int PPUView;
extern int FC_Error;
void Mapper90_Init(CartInfo *);
void FCEUI_Sound(int);
void FCEUI_SetSoundQuality(int);
void FCEUI_DisableSpriteLimitation(int);

inline void IncrementInstructionsCounters(){}

extern int debug_loggingCD;
typedef uint8 u8;
extern uint8 *XDBuf;
int GetPRGAddress(int);
extern unsigned char *cdloggerdata;
extern volatile int datacount,undefinedcount;
