#!/usr/bin/env python3
"""Audit Modrinth file availability, not a proof of JAR-level compatibility.
No downloads of mods, no installation and no mutation of existing instances.
"""
import datetime
import json
import os
from pathlib import Path
import re
import urllib.request

PROJECTS = ('sodium', 'sodium-extra', 'iris', 'zoomify', 'ok-zoomer', 'modmenu', 'fabric-api')
REQUIRED = ('sodium', 'iris')
BASE = 'https://api.modrinth.com/v2/'


def get(path):
    request = urllib.request.Request(BASE + path, headers={
        'User-Agent': 'ElysiumLauncher-CatalogAudit/1.0 (github.com/artemii14/ubiquitous-happiness)',
        'Accept': 'application/json',
    })
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)


def run():
    # Ignore snapshots/pre-releases of Minecraft, but allow beta/alpha mod files:
    # the user's rule requires the newest available compatible file, not release-only.
    releases = {v['version'] for v in get('tag/game_version') if v['version_type'] == 'release'}
    releases = {v for v in releases if (re.fullmatch(r'1\.\d+(?:\.\d+)?', v) and int(v.split('.')[1]) >= 14)
                or re.fullmatch(r'\d{2}\.\d+(?:\.\d+)?', v)}
    data = {slug: get('project/' + slug + '/version') for slug in PROJECTS}
    report = {'checked_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'scope': 'File availability only; dependencies and JAR constraints not yet resolved',
              'mod_channels': ['release', 'beta', 'alpha'], 'loaders': {}}
    for loader in ('fabric', 'neoforge'):
        available = {}
        for slug, versions in data.items():
            by_game = {}
            for version in sorted(versions, key=lambda v: v['date_published'], reverse=True):
                if loader not in version['loaders'] or not version.get('files'):
                    continue
                for game in version['game_versions']:
                    if game not in releases or game in by_game:
                        continue
                    by_game[game] = {
                        'id': version['id'], 'version': version['version_number'],
                        'type': version['version_type'], 'published': version['date_published'],
                        'dependencies': version['dependencies'],
                    }
            available[slug] = by_game
        required = REQUIRED + (('fabric-api',) if loader == 'fabric' else ())
        eligible = set.intersection(*(set(available[s]) for s in required))
        eligible &= set(available['zoomify']) | set(available['ok-zoomer'])
        rows = []
        for game in sorted(eligible, key=lambda s: tuple(map(int, s.split('.')))):
            zoom = 'zoomify' if game in available['zoomify'] else 'ok-zoomer'
            mods = {s: available[s][game] for s in required + (zoom,)}
            if game in available['sodium-extra']:
                mods['sodium-extra'] = available['sodium-extra'][game]
            if game in available['modmenu']:
                mods['modmenu'] = available['modmenu'][game]
            rows.append({'minecraft': game, 'zoom': zoom, 'extra': 'sodium-extra' in mods, 'mods': mods})
        report['loaders'][loader] = {'rows': rows, 'coverage': {s: sorted(v) for s, v in available.items()}}
        if os.environ.get('GITHUB_ACTIONS'):
            # Check annotations can be read without downloading redirected Azure logs.
            text = '; '.join(r['minecraft'] + ':' + r['zoom'] + ('+extra' if r['extra'] else '') for r in rows)
            print(f'::notice title=Sodium catalog {loader}::{text or "NONE"}')
            for slug in required:
                if not available[slug]:
                    print(f'::notice title=Missing {loader}::{slug} has no matching files')
        print(loader + ': ' + ', '.join(r['minecraft'] for r in rows))
    Path('sodium-catalog-audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    summary = ['# Sodium catalog availability', '', report['checked_utc'], '', report['scope'],
               '', 'Includes release, beta and alpha mod files; only stable Minecraft releases.', '']
    for loader, result in report['loaders'].items():
        summary += ['## ' + loader, '', '| Minecraft | Zoom | Sodium Extra |', '|---|---|---|']
        summary += [f'| {r["minecraft"]} | {r["zoom"]} | {r["extra"]} |' for r in result['rows']]
    if os.environ.get('GITHUB_STEP_SUMMARY'):
        Path(os.environ['GITHUB_STEP_SUMMARY']).write_text('\n'.join(summary) + '\n', encoding='utf-8')


if __name__ == '__main__':
    run()
