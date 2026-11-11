"""Opt-in legacy GSU budget experiment; keep STOP and saved endpoints exact."""
from pathlib import Path


def prepare(core):
    path = Path(core) / 'src/fxinst.c'
    source = path.read_text()
    old = '''   GSU.vCounter = nInstructions;
   READR14;
   while (TF(G) && (GSU.vCounter-- > 0))
      FX_STEP;'''
    new = '''   /* Handlers do not inspect this budget. STOP alone writes it (zero),
    * and clears G. Keep its result; otherwise publish the exhausted counter,
    * including the original unsigned wrap when the budget reaches zero.
    * Snapshots are taken outside FxEmulate, never inside this loop. */
   uint32 remaining = nInstructions;
   GSU.vCounter = nInstructions;
   READR14;
   while (TF(G) && (remaining-- > 0))
      FX_STEP;
   if (TF(G)) GSU.vCounter = remaining;'''
    assert source.count(old) == 1
    uses = [line.strip() for line in source.splitlines() if 'GSU.vCounter' in line]
    assert uses == ['GSU.vCounter = 0;', 'GSU.vInstCount = GSU.vCounter;',
                    'GSU.vCounter = nInstructions;', 'while (TF(G) && (GSU.vCounter-- > 0))']
    path.write_text(source.replace(old, new))
