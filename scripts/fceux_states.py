"""Expose explicit emulated state, keeping pinned vendor files untouched."""
def prepare(out):
    def table(name, fields):
        return '\nSFORMAT '+name+'[]={\n'+''.join(
            ' {&'+f+',sizeof('+f+'),"ZZ"},\n' for f in fields.split())+' {0}};\n'
    p=out/'ppu.cpp';s=p.read_text()
    s+=table('FC_PPU_EXTRA','UPALRAM deemp deempcnt ppudead kook SpriteDMA DummyRead scanline g_rasterpos linestartts tofix firsttile sphitx sphitdata spork numsprites SpriteBlurp sprlinebuf SPRBUF')
    p.write_text(s)
    p=out/'sound.cpp';s=p.read_text()
    old='   static uint32 tcout=0;\n   static int32 triacc=0;\n   static int32 noiseacc=0;'
    assert s.count(old)==1
    s=s.replace(old,'')
    s='static unsigned tcout; static int triacc,noiseacc;\n'+s
    s+=table('FC_SOUND_EXTRA','tristep wlcount RectDutyCount soundtsoffs soundtsi sqacc DMCDMABuf ChannelBC tcout triacc noiseacc Wave inbuf')
    p.write_text(s)
    p=out/'filter.cpp';s=p.read_text();old=' static int64 acc1=0,acc2=0;'
    assert s.count(old)==1;s=s.replace(old,'')
    s=s.replace('void SexyFilter(int32 *in, int32 *out, int32 count)','static int64 acc1=0,acc2=0;\nvoid SexyFilter(int32 *in, int32 *out, int32 count)')
    s+=table('FC_FILTER_EXTRA','acc1 acc2');p.write_text(s)
    # Upstream mapper90 accidentally saves only half of its eight CHR low regs.
    p=out/'90.cpp';s=p.read_text();assert '{ chrlow, 4, "CHRL" }' in s
    p.write_text(s.replace('{ chrlow, 4, "CHRL" }','{ chrlow, 8, "CHRL" }'))
