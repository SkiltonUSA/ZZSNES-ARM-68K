"""Pack the supplied About icon as a small, embedded MUI bitmap."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def build(out):
    out.mkdir(parents=True, exist_ok=True)
    # The supplied PNG has an opaque background. Flood only its connected
    # outer background, keeping the white information symbol intact.
    command = ['magick', str(ROOT / 'development/compact-ui-20261030/about-icon.png'),
               '-alpha', 'on', '-fuzz', '12%', '-fill', 'none',
               '-draw', 'alpha 0,0 floodfill', '-trim', '+repage',
               '-resize', '20x20!', '+dither', '-colors', '15']
    rgba = subprocess.check_output(command + ['-depth', '8', 'rgba:-'])
    assert len(rgba) == 20 * 20 * 4
    palette = [(0, 0, 0)]
    indices = []
    for i in range(0, len(rgba), 4):
        r, g, b, a = rgba[i:i+4]
        if a < 128:
            indices.append(0)
        else:
            rgb = (r, g, b)
            if rgb not in palette:
                palette.append(rgb)
            indices.append(palette.index(rgb))
    assert len(palette) <= 16
    palette += [(0, 0, 0)] * (16 - len(palette))
    body = bytearray(20 * 4 * 4)
    for i, value in enumerate(indices):
        y, x = divmod(i, 20)
        for bit in range(4):
            if value & (1 << bit):
                body[(y * 4 + bit) * 4 + x // 8] |= 1 << (7 - x % 8)
    text = '/* Generated from assets/about-icon.png; do not edit. */\n'
    text += '#define ABOUT_WIDTH 20\n#define ABOUT_HEIGHT 20\n'
    text += 'static const UBYTE about_body[]={' + ','.join(map(str, body)) + '};\n'
    text += 'static const ULONG about_colours[]={' + ','.join(
        hex(c * 0x01010101) for rgb in palette for c in rgb) + '};\n'
    (out / 'about-art.h').write_text(text)
    subprocess.run(command + [str(out / 'about-icon.png')], check=True)
