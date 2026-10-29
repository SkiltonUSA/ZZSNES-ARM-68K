"""Isolated timing overlay. The legacy opcode implementation stays untouched."""
from pathlib import Path
import re
import shutil

VERSION = '0.7.5-exp'
ROOT = Path(__file__).resolve().parents[1]
OVERLAY = ROOT / 'development/timed-superfx-20261029'


def replace(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Unexpected Super FX source: ' + old[:100])
    return text.replace(old, new, 1)


def options(directory):
    p = Path(directory) / 'core_options.h'
    p.write_text(replace(p.read_text(), '#pragma once', '#pragma once\n#define PS_TIMED_FX 0x800000u')
                 .replace('~0x7fffffu', '~0xffffffu'))


def prepare(core):
    core = Path(core)
    src = core / 'src'
    options(core)
    for name in ('timed_fx.h', 'timed_fx.c'):
        shutil.copy2(OVERLAY / name, src / name)
    header = (src / 'fxinst.h').read_text()
    # A separate opcode table avoids charging cycles or testing mode per
    # instruction in Legacy. Shared GSU register layout is unchanged.
    header = replace(header, '#define FETCHPIPE { PIPE = PRGBANK(R15); }', '''#define FETCHPIPE { PIPE = PRGBANK(R15); ps_fx_fetch(USEX16(R15)); }''')
    header += '''
#include "timed_fx.h"
#define FX_CYC(n) (ps_fx_timing.cycles += (n))
static INLINE void ps_fx_fetch(unsigned address) {
    unsigned offset=USEX16(address-GSU.vCacheBaseReg);
    if(GSU.bCacheActive && offset<512) {
        unsigned bit=1u<<(offset>>4);
        if(!(ps_fx_timing.cache&bit)) {
            ps_fx_timing.cache|=bit;
            FX_CYC(ps_fx_timing.memory*16);
        }
        FX_CYC(ps_fx_timing.cached);
    } else FX_CYC(ps_fx_timing.memory);
}
'''
    # GSU declaration must precede the inline fetch implementation.
    header = header.replace('#include "timed_fx.h"', '#include "timed_fx.h"\nextern FxRegs_s GSU;')
    (src / 'fxinst_timed.h').write_text(header)
    instructions = (src / 'fxinst.c').read_text().replace('"fxinst.h"', '"fxinst_timed.h"')
    instructions = replace(instructions, 'int gsu_bank [512] = {0};', '')
    for old, new in [('fx_apfFunctionTable', 'ps_timed_functions'),
                     ('fx_apfOpcodeTable', 'ps_timed_opcodes'),
                     ('fx_apfPlotTable', 'ps_timed_plots')]:
        instructions = instructions.replace(old, new)
    costs = {
        'STW':'memory*2', 'STB':'memory', 'LDW':'memory*2', 'LDB':'memory',
        'MULT':'mult', 'UMULT':'mult', 'MULT_I':'mult', 'UMULT_I':'mult',
        'LMS':'memory*2', 'SMS':'memory*2', 'LM':'memory*2', 'SM':'memory*2',
    }
    for name, cost in costs.items():
        pattern = r'(#define FX_' + name + r'\([^)]*\)[^\n]*\n)'
        instructions, count = re.subn(pattern, lambda m: m[0] + 'FX_CYC(ps_fx_timing.' + cost + '); \\\n', instructions)
        if count != 1:
            raise ValueError('Missing GSU timing macro: ' + name)
    functions = {
        'plot_2bit':'((ps_fx_timing.memory*2)>>3)+1', 'rpix_2bit':'ps_fx_timing.memory*2',
        'plot_4bit':'((ps_fx_timing.memory*4)>>3)+1', 'rpix_4bit':'ps_fx_timing.memory*4',
        'plot_8bit':'ps_fx_timing.memory+1', 'rpix_8bit':'ps_fx_timing.memory*8',
        'sbk':'ps_fx_timing.memory*2', 'fmult':'ps_fx_timing.fmult', 'lmult':'ps_fx_timing.fmult',
        'getb':'ps_fx_timing.memory', 'getbh':'ps_fx_timing.memory',
        'getbl':'ps_fx_timing.memory', 'getbs':'ps_fx_timing.memory',
    }
    for name, cost in functions.items():
        marker = 'static INLINE void fx_' + name + '()\n{'
        instructions = replace(instructions, marker, marker + '\n   FX_CYC(' + cost + ');')
    start = instructions.index('static uint32 fx_run(')
    end = instructions.index('#ifdef FX_FUNCTION_TABLE', start)
    instructions = instructions[:start] + '''static uint32 fx_run(uint32 budget)
{
   unsigned executed=0, fast=CLSR&1, ms0=CFGR&0x20;
   if (!TF(G)) { ps_fx_timing.debt=0; return 0; }
   if (budget<=ps_fx_timing.debt) { ps_fx_timing.debt-=budget; return 0; }
   budget-=ps_fx_timing.debt;
   ps_fx_timing.memory=fast?5:6;
   ps_fx_timing.cached=fast?1:2;
   ps_fx_timing.mult=ms0?1:2;
   ps_fx_timing.fmult=(ms0?3:7)*(fast?1:2);
   ps_fx_timing.cycles=0;
   READR14;
   while(TF(G) && ps_fx_timing.cycles<budget) { FX_STEP; executed++; }
   ps_fx_timing.debt=TF(G)?ps_fx_timing.cycles-budget:0;
   return executed;
}

''' + instructions[end:]
    (src / 'fxinst_timed.c').write_text(instructions)
    p = src / 'fxemu.c'
    text = '#include "timed_fx.h"\n' + p.read_text()
    text = replace(text, 'static INLINE void fx_readRegisterSpace()\n{',
                   'static INLINE void fx_readRegisterSpace()\n{\n   ps_fx_select_tables();')
    text = replace(text, '   memset((uint8*)&GSU, 0, sizeof(FxRegs_s));',
                   '   ps_fx_reset_timing();\n   memset((uint8*)&GSU, 0, sizeof(FxRegs_s));')
    text = text.replace('GSU.pfPlot = fx_apfPlotTable[', 'GSU.pfPlot = fx_ppfPlotTable[')
    text = text.replace('GSU.pfRpix = fx_apfPlotTable[', 'GSU.pfRpix = fx_ppfPlotTable[')
    # Both public and instruction-cache flush paths invalidate timing tags.
    text = text.replace('   GSU.vCacheFlags = 0;', '   ps_fx_timing.cache = 0;\n   GSU.vCacheFlags = 0;')
    p.write_text(text)
    p = src / 'ppu_.c'
    text = '#include "timed_fx.h"\n' + p.read_text()
    # GO/R15 writes arm execution. Timed mode runs once at HBLANK_END_EVENT.
    text = text.replace('S9xSuperFXExec();', 'if (!ps_fx_timed_active()) S9xSuperFXExec();')
    text = replace(text, '         if (Fx3Active)\n            FxEmulate',
                   '         if (ps_fx_timed_active())\n            FxEmulate(1364); /* SNES master clocks per nominal scanline. */\n         else if (Fx3Active)\n            FxEmulate')
    p.write_text(text)
    p = core / 'player.c'
    text = '#include "src/timed_fx.h"\n' + p.read_text()
    text = replace(text, 'options=v;return 1;', 'options=v;ps_fx_set_timed(v&PS_TIMED_FX);return 1;')
    text = replace(text, ' if(!retro_serialize(', ' ps_fx_store_timing(p);\n if(!retro_serialize(')
    text = replace(text, ' full_dirty=0;unsigned core=',
                   ' ps_fx_set_timed(next_options&PS_TIMED_FX);\n full_dirty=0;unsigned core=')
    text = replace(text, ' frames=ps_get(p,48);', ' ps_fx_load_timing(p);\n frames=ps_get(p,48);')
    text = replace(text, ' if(!ps_valid(p,n))return 0;',
                   ' if(!ps_valid(p,n)||!ps_fx_valid_timing(p))return 0;')
    p.write_text(text)


def prepare_launcher(directory):
    directory = Path(directory)
    options(directory)
    p = directory / 'library_mui.h'
    text = re.sub(r'#define LIB_VERSION "[^"]+"', '#define LIB_VERSION "'+VERSION+'"', p.read_text())
    text = replace(text, 'static void settings_get(', 'static Object *core_fx_timing;\nstatic const char *fx_timing_choices[]={"Legacy","Timed (experimental)",NULL};\nstatic void settings_get(')
    text = replace(text, ' p->controls.second=value(',
                   ' p->core|=value(core_fx_timing,MUIA_Cycle_Active)?PS_TIMED_FX:0;\n p->controls.second=value(')
    text = replace(text, ' set(second_player,MUIA_Cycle_Active,p->controls.second);',
                   ' set(core_fx_timing,MUIA_Cycle_Active,!!(p->core&PS_TIMED_FX));\n set(second_player,MUIA_Cycle_Active,p->controls.second);')
    text = replace(text, '     Child,textline("Emulated CPU"),Child,core_clock=cycle(clock_choices),End,',
                   '     Child,textline("Emulated CPU"),Child,core_clock=cycle(clock_choices),\n     Child,textline("Super FX"),Child,core_fx_timing=cycle(fx_timing_choices),End,\n    Child,textline("Timed Super FX is experimental. FX3 keeps legacy timing."),')
    p.write_text(text)
