"""Isolated GSU fetch experiments; default builds and state formats are unchanged."""
from pathlib import Path


def prepare(core):
    core = Path(core)
    header = core / 'src/fxinst.h'
    old = '#define PRGBANK(idx) GSU.pvPrgBank[USEX16(idx) & (Fx3Active && GSU.vPrgBankReg < 0x40 ? 0x7fff : 0xffff)]'
    new = '''/* Rebuilt with the program pointer after register import, LJMP and restore.
 * No timing, bank layout or checkpoint fields change. */
extern uint32 fx_prg_mask;
#define FX_BIND_FETCH_MASK() (fx_prg_mask = (Fx3Active && GSU.vPrgBankReg < 0x40 ? 0x7fff : 0xffff))
#define PRGBANK(idx) GSU.pvPrgBank[(idx) & fx_prg_mask]'''
    text = header.read_text()
    assert text.count(old) == 1
    header.write_text(text.replace(old, new))
    for name in ('fxemu.c', 'fxinst.c', 'fx_state.h'):
        path = core / 'src' / name
        text = path.read_text()
        binding = 'GSU.pvPrgBank = GSU.apvRomBank[GSU.vPrgBankReg];' if name != 'fx_state.h' else 'GSU.pvPrgBank=GSU.apvRomBank[GSU.vPrgBankReg];'
        assert text.count(binding) == 1, name
        text = text.replace(binding, binding + ' FX_BIND_FETCH_MASK();')
        if name == 'fxemu.c':
            assert text.count('uint8 Fx3Active;') == 1
            text = text.replace('uint8 Fx3Active;', 'uint8 Fx3Active;\nuint32 fx_prg_mask = 0xffff;')
        path.write_text(text)


def prepare_loop(core):
    """Select ordinary/FX3 pipeline fetch once per GSU invocation."""
    path = Path(core) / 'src/fxinst.c'
    text = path.read_text()
    old = '''   while (TF(G) && (GSU.vCounter-- > 0))
      FX_STEP;'''
    new = '''   /* Cartridge type is immutable during this invocation. Keep the
    * original FX3 path, but ordinary GSU banks always use all 16 PC bits.
    * LJMP still reloads pvPrgBank; budget and STOP handling are unchanged. */
   if (Fx3Active)
   {
      while (TF(G) && (GSU.vCounter-- > 0))
         FX_STEP;
   }
   else
   {
      while (TF(G) && (GSU.vCounter-- > 0))
      {
         uint32 vOpcode = (uint32)PIPE;
         PIPE = GSU.pvPrgBank[USEX16(R15)];
#ifdef FX_ADDRESS_CHECK
         GSU.vPipeAdr = (GSU.vPrgBankReg << 16) + R15;
#endif
         (*fx_ppfOpcodeTable[(GSU.vStatusReg & 0x300) | vOpcode])();
      }
   }'''
    assert text.count(old) == 1
    path.write_text(text.replace(old, new))
