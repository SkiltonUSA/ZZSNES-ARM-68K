"""Isolated single-core raster experiments; never modify archived core sources."""
from pathlib import Path

MODES = ('baseline', 'profile', 'fixed', 'sprite', 'ppu', 'all')
COUNTERS = ('updates', 'empty_updates', 'clip_rebuilds', 'sprite_rebuilds',
            'sprite_passes', 'sprites_visited', 'sprite_tile_rows', 'sprite_lines',
            'renderer_selections', 'normal_selections', 'fixed_full_selections',
            'math_selections', 'tile_rows', 'clipped_tile_rows', 'avoidable_clip_rebuild_marks',
            'avoidable_sprite_selections')


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Unexpected raster source: ' + old[:80])
    return text.replace(old, new, 1)


def fixed_colour(gfx, tile):
    """Specialise full-strength math; preserve colour-window depth 0/1."""
    start = tile.index('static INLINE void WRITE_4PIXELS16_ADDF1_2')
    end = tile.index('void DrawLargePixel16Add(', start)
    functions = tile[start:end]
    for old, new in [('FixedAdd1_2', 'FixedAddFull'), ('FixedSub1_2', 'FixedSubFull'),
                     ('ADDF1_2', 'ADDFULL'), ('SUBF1_2', 'SUBFULL'),
                     ('COLOR_ADD1_2', 'COLOR_ADD'), ('COLOR_SUB1_2', 'COLOR_SUB')]:
        functions = functions.replace(old, new)
    tile = tile[:end] + '/* Fixed full-strength colour math: no subscreen pixel fetch. */\n' + functions + tile[end:]
    declarations = ''
    for op in ('Add', 'Sub'):
        declarations += 'void DrawTile16Fixed%sFull(uint32, int32, uint32, uint32);\n' % op
        declarations += 'void DrawClippedTile16Fixed%sFull(uint32, int32, uint32, uint32, uint32, uint32);\n' % op
        for prefix, pointer in [('DrawTile', 'DrawTilePtr'), ('DrawClippedTile', 'DrawClippedTilePtr')]:
            old = '            %s = %s16%s;' % (pointer, prefix, op)
            new = '            %s = (GFX.r2130 & 2) ? %s16%s : %s16Fixed%sFull;' % (pointer, prefix, op, prefix, op)
            gfx = replace_once(gfx, old, new)
    marker = 'static INLINE void SelectTileRenderer('
    gfx = replace_once(gfx, marker, declarations + '\n' + marker)
    return gfx, tile


def sprite_setup(gfx):
    """Reuse horizontal clipping across the sprite's existing tile-row batches."""
    start = gfx.index('void DrawOBJS(')
    end = gfx.index('void DrawBackgroundMosaic(', start)
    before, body, after = gfx[:start], gfx[start:end], gfx[end:]
    marker = '         for (Y = 0; Y < Size; Y += 8)'
    setup = '''         /* Horizontal clipping is constant across this sprite/window span. */
         int hskip = PPU.OBJ[S].HPos < Left ? Left - PPU.OBJ[S].HPos : 0;
         int first_column = hskip >> 3;
         int first_pixel = hskip & 7;
         int draw_left = PPU.OBJ[S].HPos + hskip;
         int draw_right = PPU.OBJ[S].HPos + Size;
         if (draw_right > Right) draw_right = Right;
         int width = draw_right - draw_left;
         int first_width = first_pixel ? 8 - first_pixel : 0;
         if (first_width > width) first_width = width;
         int whole_tiles = (width - first_width) >> 3;
         int last_width = (width - first_width) & 7;

'''
    body = replace_once(body, marker, setup + marker)
    a = body.index('               Middle = Size >> 3;')
    b = body.index('            }\n         }\n      }', a)
    body = body[:a] + '''               Tile += first_column * TileInc;
               O += (draw_left - first_pixel) * GFX_PIXSIZE;
               if (first_pixel)
               {
                  (*DrawClippedTilePtr)(Tile, O, first_pixel, first_width,
                                        TileLine, LineCount);
                  Tile += TileInc;
                  O += 8 * GFX_PIXSIZE;
               }
               for (X = 0; X < whole_tiles; X++, O += 8 * GFX_PIXSIZE,
                     Tile += TileInc)
                  (*DrawTilePtr)(Tile, O, TileLine, LineCount);
               if (last_width)
                  (*DrawClippedTilePtr)(Tile, O, 0, last_width,
                                        TileLine, LineCount);
''' + body[b:]
    body = body.replace('      int clip, Offset;', '      int clip;').replace('               int Middle;\n', '')
    return before + body + after


def renderer_selection(gfx, profile=False):
    start = gfx.index('void DrawOBJS(')
    end = gfx.index('void DrawBackgroundMosaic(', start)
    body = gfx[start:end]
    body = replace_once(body, '   int I = 0;', '   int I = 0;\n   int previous_normal = -1;')
    old = '''      if (OnMain && SUB_OR_ADD(4))
         SelectTileRenderer(!GFX.Pseudo && PPU.OBJ [S].Palette < 4, false);'''
    new = '''      if (OnMain && SUB_OR_ADD(4))
      {
         int normal = !GFX.Pseudo && PPU.OBJ[S].Palette < 4;
         SELECT
         previous_normal = normal;
      }'''
    select = ('zz_ppu_profile[15] += normal == previous_normal;\n         SelectTileRenderer(normal, false);' if profile else
              'if (normal != previous_normal) SelectTileRenderer(normal, false);')
    body = replace_once(body, old, new.replace('SELECT', select))
    return gfx[:start] + body + gfx[end:]


def prepare(core, mode):
    if mode not in MODES:
        raise ValueError('Unknown PPU experiment: ' + mode)
    src = Path(core) / 'src'
    gfx = (src / 'gfx.c').read_text()
    tile = (src / 'tile.c').read_text()
    header = '#pragma once\n'
    if mode == 'profile':
        header += '#define ZZ_PPU_PROFILE 1\nextern unsigned zz_ppu_profile[16];\n'
        gfx += '\nunsigned zz_ppu_profile[16];\n'
        gfx = replace_once(gfx, '   GFX.S = GFX.Screen;\n   memoryfillram',
            '   zz_ppu_profile[0]++;\n   zz_ppu_profile[1] += IPPU.PreviousLine == IPPU.CurrentLine;\n   zz_ppu_profile[2] += !!PPU.RecomputeClipWindows;\n   zz_ppu_profile[3] += !!IPPU.OBJChanged;\n   GFX.S = GFX.Screen;\n   memoryfillram')
        gfx = replace_once(gfx, 'void DrawOBJS(bool8_32 OnMain, uint8 D)\n{',
            'void DrawOBJS(bool8_32 OnMain, uint8 D)\n{\n   zz_ppu_profile[4]++;')
        gfx = replace_once(gfx, '      int VPos = GFX.VPositions [S];',
            '      zz_ppu_profile[5]++;\n      int VPos = GFX.VPositions [S];')
        gfx = replace_once(gfx, '               TileLine = StartLine << 3;',
            '               zz_ppu_profile[6]++; zz_ppu_profile[7] += LineCount;\n               TileLine = StartLine << 3;')
        gfx = replace_once(gfx, 'static INLINE void SelectTileRenderer(bool8_32 normal, bool NoZ)\n{',
            'static INLINE void SelectTileRenderer(bool8_32 normal, bool NoZ)\n{\n   zz_ppu_profile[8]++;\n   zz_ppu_profile[9] += !!normal;\n   zz_ppu_profile[10] += !normal && !(GFX.r2130 & 2) && !(GFX.r2131 & 0x40);\n   zz_ppu_profile[11] += !normal;')
        import re
        def count_rows(match):
            name = match.group(1)
            index = 13 if 'Clipped' in name else 12
            return match.group(0) + '\n   zz_ppu_profile[%d] += LineCount;' % index
        tile = re.sub(r'void (Draw(?:Clipped)?Tile16\w*)\([^)]*\)\n\{', count_rows, tile)
    if mode in ('fixed', 'all'):
        gfx, tile = fixed_colour(gfx, tile)
    if mode in ('sprite', 'all'):
        gfx = sprite_setup(gfx)
    if mode in ('profile', 'ppu', 'all'):
        gfx = renderer_selection(gfx, profile=mode == 'profile')
    if mode in ('profile', 'ppu', 'all'):
        ppu = (src / 'ppu_.c').read_text()
        for addr, mask in [('212c', 'Memory.FillRAM[0x212e] & 0x1f'),
                           ('212d', 'Memory.FillRAM[0x212f] & 0x1f'),
                           ('212e', 'Memory.FillRAM[0x212c] & 0x1f'),
                           ('212f', 'Memory.FillRAM[0x212d] & 0x1f'),
                           ('2130', '0xf0')]:
            begin = ppu.index('      case 0x' + addr + ':')
            end = ppu.index('      case ', begin + 12)
            block = ppu[begin:end]
            condition = '(Byte ^ Memory.FillRAM[0x' + addr + ']) & (' + mask + ')'
            old = '            PPU.RecomputeClipWindows = TRUE;'
            new = ('            zz_ppu_profile[14] += !(' + condition + ');\n' + old
                   if mode == 'profile' else
                   '            if (' + condition + ') PPU.RecomputeClipWindows = TRUE;')
            ppu = ppu[:begin] + replace_once(block, old, new) + ppu[end:]
        (src / 'ppu_.c').write_text('#include "ppu_experiment.h"\n' + ppu)
    (src / 'ppu_experiment.h').write_text(header)
    for name, text in [('gfx.c', gfx), ('tile.c', tile)]:
        (src / name).write_text('#include "ppu_experiment.h"\n' + text)
