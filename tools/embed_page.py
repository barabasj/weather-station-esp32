"""Convierte web/index.html en src/web_page.h (ejecutar despues de editar la pagina)."""
import pathlib
root = pathlib.Path(__file__).resolve().parent.parent
html = (root / 'web' / 'index.html').read_text(encoding='utf-8')
assert ')html"' not in html
(root / 'src' / 'web_page.h').write_text(
    '// Generado por tools/embed_page.py a partir de web/index.html - no editar a mano\n'
    '#pragma once\n#include <pgmspace.h>\n\n'
    'const char WEB_PAGE[] PROGMEM = R"html(' + html + ')html";\n', encoding='utf-8')
print('web_page.h:', len(html.encode()), 'bytes')
