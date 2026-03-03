#!/usr/bin/env python3
"""
Valida se a cartela de stickers cabe em A4 (210×297mm)
"""

import xml.etree.ElementTree as ET

SVG_FILE = "docs/svg/buttons_sticker.svg"
A4_WIDTH = 210  # mm
A4_HEIGHT = 297  # mm

def parse_svg():
    tree = ET.parse(SVG_FILE)
    root = tree.getroot()

    # Extract viewBox
    viewbox = root.get('viewBox', '0 0 210 297')
    _, _, width, height = map(float, viewbox.split())

    # Count buttons by section
    sections = {
        'Botões Frontais': 0,
        'Streaming/Utilities': 0,
        'Encoders': 0
    }

    for comment in root.iter():
        if comment.tag == '{http://www.w3.org/2000/svg}g':
            # Count <use> elements (8mm buttons)
            uses = comment.findall('.//{http://www.w3.org/2000/svg}use')
            # Count <rect> elements (6×4mm rectangular buttons)
            rects = comment.findall('.//{http://www.w3.org/2000/svg}rect')

            transform = comment.get('transform', '')
            if 'translate(15, 25)' in transform:
                sections['Botões Frontais'] += len(uses)
            elif 'translate(15, 146)' in transform:
                sections['Streaming/Utilities'] += len(rects) if rects else 0
            elif 'translate(18, 183)' in transform:
                sections['Encoders'] += len(uses)

    return width, height, sections

def main():
    width, height, sections = parse_svg()

    print("=" * 60)
    print("VALIDAÇÃO DE LIMITES DA CARTELA DE STICKERS")
    print("=" * 60)
    print(f"Dimensões do viewBox: {width}mm × {height}mm")
    print(f"Tamanho A4: {A4_WIDTH}mm × {A4_HEIGHT}mm")
    print()

    # Check width
    print("✓ Verificação de largura:")
    if width <= A4_WIDTH:
        print(f"  ✅ OK - {width}mm cabe em A4 (margem: {A4_WIDTH - width}mm)")
    else:
        print(f"  ❌ ERRO - {width}mm excede A4 por {width - A4_WIDTH}mm")
    print()

    # Check height
    print("✓ Verificação de altura:")
    if height <= A4_HEIGHT:
        print(f"  ✅ OK - {height}mm cabe em A4 (margem: {A4_HEIGHT - height}mm)")
    else:
        print(f"  ❌ ERRO - {height}mm excede A4 por {height - A4_HEIGHT}mm")
    print()

    # Show summary
    print("✓ Resumo de conteúdo:")
    total = 0
    for section, count in sections.items():
        if count > 0:
            print(f"  • {section}: {count} items")
            total += count

    print(f"\n  TOTAL: {total} stickers")
    print("=" * 60)

if __name__ == "__main__":
    main()
