#!/usr/bin/env python3
"""Build isolated external-ROM launcher using established owned-memory teardown."""
import build_pocketsnes as b
b.OUT=b.ROOT/'.context/amiga/pocketsnes-game-arm'
common=b.ROOT/'amiga/mesen_bench'
# Retain timer calibration, bounded mailbox requests, cancellation and restore checks.
host=(common/'host.c').read_text()
start=host.index('    if(!request(MB_VERIFY))')
end=host.index('#ifdef MB_MMU\n    if(!request(MB_CANCEL))',start)
host=host[:start]+'''    for(i=0;i<10;i++) {
        if(!request(i==0?MB_VERIFY:MB_BENCH))goto done;
        if(value(0)!=60 || value(1) || value(2)!=(i+1)*60 || value(4) || value(5) || !value(6))goto done;
        printf("SAMPLE n=%u warmup=%u frames=60 total_frames=%u ticks=%u core_us=%u video=%08x pcm=%08x samples=%u width=%u height=%u\\n",
            i,i==0,value(2),value(6),(ULONG)((uint64_t)value(6)*1000000u/rate),value(3),value(9),value(10),value(7),value(8));fflush(stdout);
    }
'''+host[end:]
host=host.replace('MESEN CPU-ONLY','POCKETSNES SMW CPU/PPU/SPC/DSP')
host=host.replace('MESEN_CPU PASS; CPU-only throughput, NOT emulated video FPS','POCKET_GAME PASS; 600 native-matched frames and active PCM, NOT display FPS')
host+='''
/* ROM lives only inside this Exec-owned allocation, loaded before cache sync
 * and Core1 startup. Reject size/content mismatches before any ARM execution. */
int pocket_load_rom(unsigned char *base) {
    FILE *f=fopen("SD032G:Dev/ZZSnes-Pocket/SMW-Test.sfc","rb");
    unsigned long n,i;unsigned long h=2166136261u;
    if(!f){puts("Missing private test ROM");return 0;}
    n=fread(base+0x190000,1,524288,f);
    if(n!=524288 || fgetc(f)!=EOF || ferror(f)){fclose(f);return 0;}
    fclose(f);
    for(i=0;i<n;i++)h=(h^base[0x190000+i])*16777619u;
    printf("ROM bytes=%lu fnv=%08lx\\n",n,h);
    return h==ROM_FNV;
}
'''
# Expected FNV checked against the private input at build time, no input embedded.
raw=(b.ROOT/'.context/amiga/pocketsnes-game/input.sfc').read_bytes()
import hashlib
assert hashlib.sha256(raw).hexdigest()=='0838e531fe22c077528febe14cb3ff7c492f1f5fa8de354192bdff7137c27f5b'
h=2166136261
for v in raw:h=((h^v)*16777619)&0xffffffff
host=host.replace('ROM_FNV',hex(h)+'u')
launcher=(b.ROOT/'amiga/arm_debug/zz9000/launcher.c').read_text()
launcher=launcher.replace('int main(int argc,char **argv)','extern int pocket_load_rom(unsigned char *base);\nint main(int argc,char **argv)')
launcher=launcher.replace('memset(mem,0,ZZ_BLOCK_SIZE);memcpy(mem,zz_image,sizeof(zz_image));','memset(mem,0,ZZ_BLOCK_SIZE);memcpy(mem,zz_image,sizeof(zz_image));\n    if(!pocket_load_rom(mem))goto done;')
(b.OUT/'launcher.c').write_text(launcher)
# Pack with identical layout/compiler settings; substitute only this frontend.
code=(b.ROOT/'scripts/build_pocketsnes_host.py').read_text()
a=code.index("host=(common/'host.c')");z=code.index("(out/'host.c').write_text(host)",a)
code=code[:a]+code[z:]
code=code.replace("appname='ZZSnes-Pocket-Bench'","appname='ZZSnes-Pocket-GameBench'")
code=code.replace("'amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c')","str(relative/'launcher.c'),str(relative/'host.c')")
code=code.replace('Original six-frame headless fixture; portable C CPU/PPU/SPC/DSP; no frameskip, idle shutdown or game speed patches. No display/audio-device FPS.','External SMW 600-frame headless startup/menu; active PCM, 60-frame batches; no display or audio playback.')
exec(compile(code,'build_pocketsnes_host.py','exec'))
