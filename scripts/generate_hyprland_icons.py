#!/usr/bin/env python3
"""GPL-3.0-only. Hyprland-inspired variant of our own Midnight UI artwork.
No Hyprland logos or external artwork. Run from any directory to regenerate.
"""
from pathlib import Path
import generate_midnight_icons as source

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'launcher/resources/hyprland'

def generate():
    icons = dict(source.icons)
    icons.update({
        'launch': '<path d="m6 5 7 7-7 7M14 19h5"/>',
        'new': '<rect x="3" y="3" width="18" height="18" rx="5"/><path d="M12 7v10M7 12h10"/>',
        'settings': '<path d="M4 7h16M4 17h16"/><rect x="7" y="4" width="4" height="6" rx="1"/><rect x="14" y="14" width="4" height="6" rx="1"/>',
        'news': '<rect x="3" y="4" width="18" height="16" rx="3"/><path d="M11 4v16M11 12h10"/>',
        'appearance': '<rect x="3" y="4" width="18" height="16" rx="4"/><path d="M3 9h18M9 9v11"/><path d="M6 6.5h.1m3 0h.1"/>',
    })
    for name, target in source.aliases.items():
        icons[name] = icons[target]
    (OUT / 'scalable').mkdir(parents=True, exist_ok=True)
    colors = {'status-good': '#86efac', 'status-bad': '#fb7185', 'status-yellow': '#fbbf24',
              'dialog-ok': '#86efac', 'dialog-error': '#fb7185', 'dialog-cancel': '#fb7185',
              'dialog-warning': '#fbbf24', 'status-running': '#74c7ff'}
    for name, body in sorted(icons.items()):
        color = colors.get(name, '#68e8da')
        svg = f'''<!-- SPDX-License-Identifier: GPL-3.0-only; Elysium Hyprland-inspired artwork -->
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
 <g fill="none" stroke="{color}" stroke-width="1.55" stroke-linecap="round" stroke-linejoin="round">{body}</g>
</svg>
'''
        (OUT / 'scalable' / f'{name}.svg').write_text(svg, encoding='utf-8')
    (OUT / 'index.theme').write_text('''[Icon Theme]
Name=Elysium Hyprland
Comment=Thin cyan outline icons, tiling and terminal-inspired controls
Inherits=multimc
Directories=scalable

[scalable]
Size=24
Type=Scalable
MinSize=16
MaxSize=256
''', encoding='utf-8')
    entries = '\n'.join(f'  <file>scalable/{name}.svg</file>' for name in sorted(icons))
    (OUT / 'hyprland.qrc').write_text(f'<RCC>\n <qresource prefix="/icons/hyprland">\n  <file>index.theme</file>\n{entries}\n </qresource>\n</RCC>\n', encoding='utf-8')
    return len(icons)

if __name__ == '__main__':
    print(f'Generated {generate()} Hyprland-inspired UI icons')
