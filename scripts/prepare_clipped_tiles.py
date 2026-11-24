"""Correct portable clipped-tile masks in an isolated staged core."""
from pathlib import Path


def prepare(core):
    path = Path(core) / 'src/gfx.h'
    text = path.read_text()
    start = text.index('#define SWAP_DWORD(dword) ')
    end = text.index('\n#endif', start)
    expression = text[start:end].split(' ', 2)[2]
    # ARM_ASM already modifies its argument. Every portable call also expects
    # an in-place swap; a value-only expression silently drops the mask swap.
    text = text[:start] + '#define SWAP_DWORD(dword) do { (dword) = ' + expression + '; } while (0)' + text[end:]
    path.write_text(text)
