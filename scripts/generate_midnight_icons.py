#!/usr/bin/env python3
"""Original Elysium Midnight UI glyphs. GPL-3.0-only. No external artwork.
24px grid, 1.7px rounded strokes, lavender foreground and semantic accents.
Run after editing paths, then commit generated SVGs and QRC.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'launcher/resources/midnight'

def path(d):
    return f'<path d="{d}"/>'
def rect(x,y,w,h,r=2):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}"/>'
def circle(x,y,r):
    return f'<circle cx="{x}" cy="{y}" r="{r}"/>'

folder = path('M3 7V5a2 2 0 0 1 2-2h5l3 4h6a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V7Z')
page = path('M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9ZM14 3v6h6')
box = path('m3 7 9-4 9 4v10l-9 4-9-4ZM3 7l9 4 9-4M12 11v10')
sliders = path('M3 6h5m4 0h9M3 12h11m4 0h3M3 18h4m4 0h10') + circle(10,6,2)+circle(16,12,2)+circle(9,18,2)
gear = path('m9 3-1 3-3 1-2 3 2 2-1 3 3 2 2-1 3 2 3-2v-3l3-2-1-4-3-1-1-3Z')+circle(11,11,3)
user = circle(12,7,4)+path('M4 21v-2a8 8 0 0 1 16 0v2')
cube = box
icons = {
 'new':rect(3,3,18,18,4)+path('M12 7v10M7 12h10'),
 'accounts':user,
 'noaccount':user+path('m3 3 18 18'),
 'viewfolder':folder,
 'help':circle(12,12,9)+path('M9 8a3 3 0 1 1 4 3c-1 1-1 1-1 3M12 17h.01'),
 'about':circle(12,12,9)+path('M12 11v6M12 7h.01'),
 'settings':sliders,
 'instance-settings':rect(3,3,18,18,4)+path('M7 8h10M7 12h5M7 16h8'),
 'launch':path('m8 4 13 8-13 8Z'),
 'stop':rect(5,5,14,14,3),
 'delete':path('M3 6h18M9 6V3h6v3M6 6l1 15h10l1-15M10 10v7M14 10v7'),
 'copy':rect(8,8,13,13)+path('M16 4V3H3v13h1'),
 'rename':path('m4 16 12-12 4 4-12 12-5 1ZM13 7l4 4M14 21h7'),
 'export':path('M13 3h8v8M21 3 10 14M10 5H5a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-5'),
 'import':path('M12 3v12m-4-4 4 4 4-4M4 16v5h16v-5'),
 'shortcut':path('M4 20v-7a5 5 0 0 1 5-5h11m-5-5 5 5-5 5'),
 'tag':path('M3 3h8l10 10-8 8L3 11Z')+circle(7,7,1),
 'refresh':path('M20 8a8 8 0 0 0-14-2L3 9m0-5v5h5M4 16a8 8 0 0 0 14 2l3-3m0 5v-5h-5'),
 'checkupdate':circle(12,12,9)+path('M12 6v12m-4-4 4 4 4-4'),
 'news':rect(3,4,18,16)+rect(6,7,5,5,1)+path('M15 8h3M15 11h3M6 16h12'),
 'log':rect(3,4,18,16)+path('m6 8 4 4-4 4M13 16h5'),
 'notes':page+path('M8 13h8M8 17h6'),
 'custom-commands':path('m8 5-6 7 6 7m8-14 6 7-6 7m-3-15-2 16'),
 'java':path('M6 10h12v5a6 6 0 0 1-12 0ZM18 11h2a3 3 0 0 1 0 6h-2M5 22h14M9 7c-3-2 3-3 0-5M14 7c-3-2 3-3 0-5'),
 'language':circle(12,12,9)+path('M3 12h18M12 3a18 18 0 0 1 0 18 18 18 0 0 1 0-18'),
 'proxy':circle(5,12,2)+circle(19,5,2)+circle(19,19,2)+path('M7 11 17 6M7 13l10 5'),
 'server':rect(3,3,18,7)+rect(3,14,18,7)+path('M7 6h.01M7 17h.01M11 6h6M11 17h6'),
 'unknown_server':rect(3,3,18,18,4)+path('M10 8a2 2 0 1 1 3 2c-1 1-1 1-1 3M12 17h.01'),
 'worlds':circle(12,12,9)+path('m6 5 4 3-1 4-4 1m15-6-5 2 1 5-3 2 1 5'),
 'minecraft':cube,
 'packages':box,
 'datapacks':box+path('m8 5 9 4M7 14v3'),
 'resourcepacks':rect(3,3,18,18,3)+circle(8,8,2)+path('m3 18 6-6 4 4 3-3 5 5'),
 'shaderpacks':path('m12 2 3 7 7 3-7 3-3 7-3-7-7-3Z'),
 'loadermods':path('M4 4h6V3a2 2 0 0 1 4 0v1h6v6h1a2 2 0 0 1 0 4h-1v6h-6v-1a2 2 0 0 0-4 0v1H4v-6h1a2 2 0 0 0 0-4H4Z'),
 'coremods':gear,
 'jarmods':page+path('m8 13 3 2-3 2m6-4-1 4'),
 'centralmods':rect(3,3,7,7)+rect(14,3,7,7)+rect(3,14,7,7)+rect(14,14,7,7),
 'quickmods':path('m14 2-10 12h7l-1 8 10-13h-7Z'),
 'screenshots':rect(3,5,18,15,3)+circle(12,12,4)+path('M8 5V3h8v2'),
 'screenshot-placeholder':rect(3,3,18,18)+path('m4 18 5-6 4 4 3-3 4 5')+circle(8,8,1),
 'appearance':path('M12 3a9 9 0 1 0 0 18h2a2 2 0 0 0 0-4h-1a2 2 0 0 1 0-4h5a3 3 0 0 0 3-3c0-4-4-7-9-7Z')+circle(7,10,1)+circle(10,6,1)+circle(16,7,1),
 'externaltools':path('m14 4-3 3 2 4 4 2 3-3a7 7 0 0 1-9 8l-6 4-3-3 5-6a7 7 0 0 1 7-9Z'),
 'bug':rect(7,6,10,15,5)+path('m8 7-3-4m11 4 3-4M3 10h4m10 0h4M2 15h5m10 0h5M4 21l4-3m8 0 4 3M12 7v13'),
 'cat':path('M4 12V3l6 4h4l6-4v9a8 8 0 0 1-16 0ZM8 12h.01M16 12h.01m-6 4 2 1 2-1'),
 'launcher':path('m12 3 9 5v8l-9 5-9-5V8ZM3 8l9 5 9-5M12 13v8')+path('M9 6h6'),
 'star':path('m12 3 3 6 7 1-5 5 1 7-6-3-6 3 1-7-5-5 7-1Z'),
 'status-good':circle(12,12,9)+path('m7 12 3 3 7-7'),
 'status-bad':circle(12,12,9)+path('m8 8 8 8m0-8-8 8'),
 'status-yellow':path('m12 3 10 18H2ZM12 9v5M12 17h.01'),
 'status-running':circle(12,12,9)+path('m10 8 6 4-6 4Z'),
}
# Extra freedesktop names used by Qt and launcher dialogs, sharing exact semantics.
aliases = {'document-new':'new','document-open':'viewfolder','folder':'viewfolder',
 'edit-copy':'copy','edit-delete':'delete','edit-rename':'rename','list-add':'new',
 'list-remove':'stop','go-next':'shortcut','go-previous':'shortcut',
 'dialog-information':'about','dialog-warning':'status-yellow','dialog-error':'status-bad',
 'dialog-ok':'status-good','dialog-cancel':'status-bad','view-refresh':'refresh',
 'download':'import','upload':'export','preferences-system':'settings','application-exit':'export'}
for name, target in aliases.items():
    icons[name] = icons[target]
colors={'status-good':'#7dd9bb','status-bad':'#f08fa4','status-yellow':'#eac58b','status-running':'#a997ff'}

def generate():
    (OUT/'scalable').mkdir(parents=True, exist_ok=True)
    for name, body in sorted(icons.items()):
        color=colors.get(name, '#b7a8ff')
        svg=f'''<!-- SPDX-License-Identifier: GPL-3.0-only; Original Elysium Midnight artwork -->
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
 <g fill="none" stroke="{color}" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">{body}</g>
</svg>
'''
        (OUT/'scalable'/f'{name}.svg').write_text(svg,encoding='utf-8')
    (OUT/'index.theme').write_text('''[Icon Theme]
Name=Elysium Midnight
Comment=Original rounded lavender outline icons
Inherits=multimc
Directories=scalable

[scalable]
Size=24
Type=Scalable
MinSize=16
MaxSize=256
''',encoding='utf-8')
    entries='\n'.join(f'  <file>scalable/{name}.svg</file>' for name in sorted(icons))
    (OUT/'midnight.qrc').write_text(f'<RCC>\n <qresource prefix="/icons/midnight">\n  <file>index.theme</file>\n{entries}\n </qresource>\n</RCC>\n',encoding='utf-8')

if __name__ == '__main__':
    generate()
    print(f'Generated {len(icons)} Midnight UI icons')
