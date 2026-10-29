/* User-supplied ROM input only; per-frame video/PCM comparison, no game data. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ppu_experiment.h"
int nes_init(const uint8_t *, uint32_t, uint16_t *, uint16_t *);
int nes_frame(uint16_t, int (*)(void));
uint32_t nes_hash(void);
unsigned nes_audio(const int16_t **);
void pocket_player_close(void);
void pocket_heap_init(void) {}
int main(int argc, char **argv) {
    assert(argc == 4);
    FILE *f = fopen(argv[1], "rb"); assert(f);
    assert(!fseek(f, 0, SEEK_END)); long n = ftell(f); rewind(f);
    assert(n > 0 && n <= 0x600000);
    uint8_t *rom = malloc(n); assert(rom);
    assert(fread(rom, 1, n, f) == (unsigned long)n); fclose(f);
    assert(!nes_init(rom, n, 0, 0));
    FILE *out = fopen(argv[3], "wb"); assert(out);
    for (unsigned i = 0; i < strtoul(argv[2], 0, 10); i++) {
        unsigned pad = i >= 360 && i < 2400 && i % 300 < 8 ? 8 : 0;
        /* After the menus, hold B and periodically steer. */
        if (i >= 2700) pad = 1 | ((i / 120) % 2 ? 128 : 0);
        assert(!nes_frame(pad, 0));
        const int16_t *pcm; unsigned samples = nes_audio(&pcm);
        uint32_t hash = 2166136261u;
        for (unsigned j = 0; j < samples * 2; j++) hash = (hash ^ (uint16_t)pcm[j]) * 16777619u;
        fprintf(out, "%u %08x %u %08x\n", i + 1, nes_hash(), samples, hash);
    }
    assert(!fclose(out));
#ifdef ZZ_PPU_PROFILE
    for (unsigned i = 0; i < 16; i++) printf("PROFILE %u %u\n", i, zz_ppu_profile[i]);
#endif
    pocket_player_close(); free(rom); return 0;
}
