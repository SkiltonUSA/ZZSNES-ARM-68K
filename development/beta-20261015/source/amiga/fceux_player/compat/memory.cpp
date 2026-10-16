/* Mapper allocations belong to the one-game launcher allocation. */
#include "platform.h"
static uint64_t arena[16384];static unsigned used;
void *FCEU_gmalloc(uint32 n){unsigned size=(n+7u)&~7u;if(size<n||size>sizeof(arena)-used){FC_Error=11;return nullptr;}void*p=(uint8*)arena+used;used+=size;memset(p,0,size);return p;}
void *FCEU_malloc(uint32 n){return FCEU_gmalloc(n);}
void FCEU_gfree(void*){} // Released with the whole allocation after Core1 stops.
void FCEU_CheatAddRAM(int,uint32,uint8*){} // Cheat system is intentionally absent.
