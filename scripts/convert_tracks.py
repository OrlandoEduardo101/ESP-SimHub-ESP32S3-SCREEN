#!/usr/bin/env python3
"""
Convert SVG track circuit files to C header with PROGMEM arrays
for ESP32 minimap rendering on WT32-SC01 Plus display.

Usage:
    pip install svgpathtools
    python scripts/convert_tracks.py [--preview]

Input:  tracks_svg/*.svg
Output: src/TrackMaps.h
"""

import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

try:
    from svgpathtools import parse_path, Path as SvgPath
except ImportError:
    print("ERROR: svgpathtools required. Install with: pip install svgpathtools")
    sys.exit(1)

# --- Configuration ---
NUM_POINTS = 80       # Equidistant points per track
MAP_WIDTH = 220       # Target area width (pixels on ESP32 display)
MAP_HEIGHT = 160      # Target area height
PADDING = 5           # Margin inside the target area

SVG_NS = '{http://www.w3.org/2000/svg}'

# SVG filename stem -> (C_IDENTIFIER, [match_strings_for_SimHub_TrackId])
TRACKS = {
    'RaceCircuitAmericas':         ('AMERICAS',    ['americas', 'cota']),
    'RaceCircuitAutodromaDiMonza': ('MONZA',       ['monza']),
    'RaceCircuitGillesVilleneuve': ('VILLENEUVE',  ['villeneuve', 'montreal']),
    'RaceCircuitInterlagos2':      ('INTERLAGOS',  ['interlagos']),
    'RaceCircuitMonaco2':          ('MONACO',      ['monaco']),
    'RaceCircuitPaulRicard':       ('PAULRICARD',  ['paul_ricard', 'ricard', 'castellet']),
    'RaceCircuitRedBull2':         ('REDBULL',     ['red_bull', 'spielberg']),
    'RaceCircuitSepang2':          ('SEPANG',      ['sepang']),
    'RaceCircuitSilverstone':      ('SILVERSTONE', ['silverstone']),
    'RaceCircuitSochiAutodrom2':   ('SOCHI',       ['sochi']),
    'RaceCircuitSuzuka':           ('SUZUKA',      ['suzuka']),
}


def parse_translate(transform_str):
    """Extract translate(x, y) from SVG transform attribute."""
    if not transform_str:
        return 0.0, 0.0
    m = re.search(r'translate\(\s*([-.0-9eE]+)[\s,]+([-.0-9eE]+)\s*\)', transform_str)
    return (float(m.group(1)), float(m.group(2))) if m else (0.0, 0.0)


def extract_svg_paths(svg_file):
    """Parse SVG and return list of (d_string, tx, ty, has_fill) tuples."""
    tree = ET.parse(svg_file)
    root = tree.getroot()
    results = []

    def collect(parent, tx, ty):
        for p in parent.findall(f'{SVG_NS}path'):
            d = p.get('d', '').strip()
            fill = p.get('fill', '')
            if d:
                results.append((d, tx, ty, fill not in ('', 'none')))

    # Top-level <path> elements
    collect(root, 0, 0)

    # <g>-wrapped paths (up to 2 levels deep)
    for g in root.findall(f'{SVG_NS}g'):
        gtx, gty = parse_translate(g.get('transform', ''))
        collect(g, gtx, gty)
        for inner in g.findall(f'{SVG_NS}g'):
            collect(inner, gtx, gty)

    return results


def get_unique_paths(svg_file):
    """Get deduplicated track path(s) from SVG, preferring non-fill paths."""
    all_paths = extract_svg_paths(svg_file)
    if not all_paths:
        return []

    non_fill = [(d, tx, ty) for d, tx, ty, hf in all_paths if not hf]
    source = non_fill if non_fill else [(d, tx, ty) for d, tx, ty, _ in all_paths]

    seen = set()
    unique = []
    for d, tx, ty in source:
        if d not in seen:
            seen.add(d)
            unique.append((d, tx, ty))
    return unique


def sample_equidistant(path_obj, n):
    """Sample n equidistant points along path via arc-length parameterization."""
    total = path_obj.length()
    if total == 0:
        return []
    points = []
    for i in range(n):
        frac = i / n  # 0.0 to (n-1)/n — don't include 1.0 (same as 0.0 for closed)
        try:
            t = path_obj.ilength(frac * total)
        except Exception:
            t = frac  # fallback
        pt = path_obj.point(t)
        points.append((pt.real, pt.imag))
    return points


def normalize(points, w, h, pad):
    """Scale/center points to fit w×h with padding, maintaining aspect ratio."""
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    min_x, max_x, min_y, max_y = min(xs), max(xs), min(ys), max(ys)
    sw, sh = max_x - min_x, max_y - min_y
    if sw == 0 or sh == 0:
        return [(w // 2, h // 2)] * len(points)

    uw, uh = w - 2 * pad, h - 2 * pad
    scale = min(uw / sw, uh / sh)
    ox = pad + (uw - sw * scale) / 2
    oy = pad + (uh - sh * scale) / 2

    return [(int(round((x - min_x) * scale + ox)),
             int(round((y - min_y) * scale + oy))) for x, y in points]


def process_track(svg_file, num_pts):
    """Process one SVG file → list of normalized (x, y) points."""
    paths = get_unique_paths(svg_file)
    if not paths:
        return []

    # Build combined path from all unique sub-paths
    all_segments = []
    for d, tx, ty in paths:
        pobj = parse_path(d)
        if tx != 0 or ty != 0:
            pobj = pobj.translated(complex(tx, ty))
        all_segments.extend(pobj)

    combined = SvgPath(*all_segments)
    raw = sample_equidistant(combined, num_pts)
    return normalize(raw, MAP_WIDTH, MAP_HEIGHT, PADDING)


def generate_header(tracks_data, output_path):
    """Write src/TrackMaps.h with PROGMEM arrays and lookup table."""
    lines = [
        '#pragma once',
        '// Auto-generated by scripts/convert_tracks.py — DO NOT EDIT MANUALLY',
        '// Track map point arrays for ESP32 minimap rendering',
        '',
        '#include <Arduino.h>',
        '',
        f'#define TRACK_MAP_WIDTH  {MAP_WIDTH}',
        f'#define TRACK_MAP_HEIGHT {MAP_HEIGHT}',
        '',
    ]

    for name, c_id, _, points in tracks_data:
        coords = ', '.join(f'{x},{y}' for x, y in points)
        lines.append(f'// {name} ({len(points)} points)')
        lines.append(f'const int16_t TRACK_{c_id}[] PROGMEM = {{ {coords} }};')
        lines.append('')

    lines.append('struct TrackMapEntry {')
    lines.append('  const char* id;           // lowercase substring to match against trackId')
    lines.append('  const int16_t* points;    // PROGMEM array: {x0,y0, x1,y1, ...}')
    lines.append('  uint8_t numPoints;        // number of (x,y) pairs')
    lines.append('};')
    lines.append('')
    lines.append('const TrackMapEntry TRACK_MAP_TABLE[] = {')

    total_entries = 0
    for _, c_id, aliases, points in tracks_data:
        for alias in aliases:
            lines.append(f'  {{"{alias}", TRACK_{c_id}, {len(points)}}},')
            total_entries += 1

    lines.append('};')
    lines.append(f'const uint8_t TRACK_MAP_COUNT = {total_entries};')
    lines.append('')

    with open(output_path, 'w') as f:
        f.write('\n'.join(lines))


def text_preview(name, points, w=60, h=25):
    """Print ASCII art preview of a track shape."""
    grid = [[' '] * w for _ in range(h)]
    for x, y in points:
        gx = int(x * (w - 1) / MAP_WIDTH)
        gy = int(y * (h - 1) / MAP_HEIGHT)
        gx = max(0, min(w - 1, gx))
        gy = max(0, min(h - 1, gy))
        grid[gy][gx] = '#'

    print(f'\n  {name}:')
    print('  +' + '-' * w + '+')
    for row in grid:
        print('  |' + ''.join(row) + '|')
    print('  +' + '-' * w + '+')


def main():
    preview = '--preview' in sys.argv

    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    svg_dir = project_dir / 'tracks_svg'
    output = project_dir / 'src' / 'TrackMaps.h'

    if not svg_dir.exists():
        print(f'ERROR: SVG directory not found: {svg_dir}')
        sys.exit(1)

    print(f'SVG source:  {svg_dir}')
    print(f'Output:      {output}')
    print(f'Points/track: {NUM_POINTS}')
    print(f'Map size:    {MAP_WIDTH}x{MAP_HEIGHT} (pad={PADDING})')
    print()

    tracks_data = []
    for svg_file in sorted(svg_dir.glob('*.svg')):
        stem = svg_file.stem
        if stem not in TRACKS:
            print(f'  SKIP: {svg_file.name} (no mapping)')
            continue

        c_id, aliases = TRACKS[stem]
        print(f'  {svg_file.name} -> TRACK_{c_id} ...', end=' ')

        points = process_track(str(svg_file), NUM_POINTS)
        if not points:
            print('FAILED')
            continue

        print(f'OK ({len(points)} pts)')
        tracks_data.append((stem, c_id, aliases, points))

        if preview:
            text_preview(stem, points)

    if not tracks_data:
        print('\nERROR: No tracks processed!')
        sys.exit(1)

    generate_header(tracks_data, str(output))

    total_bytes = sum(len(pts) * 2 * 2 for _, _, _, pts in tracks_data)  # int16_t = 2 bytes
    print(f'\nGenerated: {output}')
    print(f'Tracks:    {len(tracks_data)}')
    print(f'PROGMEM:   ~{total_bytes} bytes ({total_bytes / 1024:.1f} KB)')


if __name__ == '__main__':
    main()
