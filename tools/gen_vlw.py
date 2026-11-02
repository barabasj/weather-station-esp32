"""Genera fuentes suavizadas formato VLW (TFT_eSPI) como arrays PROGMEM."""
import sys, struct
from PIL import Image, ImageDraw, ImageFont

CHARS = list(range(0x21, 0x7F)) + list(range(0xA1, 0x100)) + \
        [0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x20AC]

def make(ttf, size):
    f = ImageFont.truetype(ttf, size)
    ascent, descent = f.getmetrics()
    glyphs, bitmaps = [], []
    for cp in sorted(CHARS):
        ch = chr(cp)
        x0, y0, x1, y1 = f.getbbox(ch, anchor='ls')
        w, h = max(0, x1 - x0), max(0, y1 - y0)
        adv = int(round(f.getlength(ch)))
        if w == 0 or h == 0:
            glyphs.append((cp, 0, 0, adv, 0, 0)); bitmaps.append(b''); continue
        img = Image.new('L', (w, h), 0)
        ImageDraw.Draw(img).text((-x0, -y0), ch, font=f, fill=255, anchor='ls')
        glyphs.append((cp, h, w, adv, -y0, x0))
        bitmaps.append(img.tobytes())
    out = struct.pack('>6i', len(glyphs), 11, size, 0, ascent, descent)
    for g in glyphs:
        out += struct.pack('>7i', *g, 0)
    out += b''.join(bitmaps)
    return out

def to_header(name, data):
    lines = [f'const uint8_t {name}[] PROGMEM = {{']
    for i in range(0, len(data), 24):
        lines.append('  ' + ','.join(f'0x{b:02X}' for b in data[i:i+24]) + ',')
    lines.append('};')
    return '\n'.join(lines)

if __name__ == '__main__':
    out = ['// Fuentes generadas con fontgen/gen_vlw.py (Inter, licencia SIL OFL)',
           '// Incluyen ASCII, Latin-1 (tildes, enie, °, ¿¡) y comillas/guiones tipograficos.',
           '#pragma once', '#include <pgmspace.h>', '']
    total = 0
    for name, ttf, size in [
        ('FONT_S',  '/usr/share/fonts/opentype/inter/Inter-Medium.otf', 13),
        ('FONT_M',  '/usr/share/fonts/opentype/inter/Inter-Medium.otf', 17),
        ('FONT_N',  '/usr/share/fonts/opentype/inter/Inter-SemiBold.otf', 21),
    ]:
        d = make(ttf, size); total += len(d)
        open(f'/home/claude/fontgen/{name}.vlw', 'wb').write(d)
        out.append(f'// {name}: {ttf.split("/")[-1]} {size}px, {len(d)} bytes')
        out.append(to_header(name, d)); out.append('')
    open(sys.argv[1], 'w').write('\n'.join(out))
    print('total bytes', total)
