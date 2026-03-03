#!/usr/bin/env python3
"""Valida se a cart ela de stickers cabe em A4 (210×297mm)"""

import xml.etree.ElementTree as ET

# Carregar SVG
tree = ET.parse('buttons_sticker.svg')
root = tree.getroot()

# Obter viewBox
viewBox = root.attrib['viewBox'].split()
width = float(viewBox[2])
height = float(viewBox[3])

print("="*60)
print("VALIDAÇÃO DE LIMITES DA CARTELA DE STICKERS")
print("="*60)
print(f"\nDimensões do viewBox: {width}mm × {height}mm")
print(f"Tamanho A4: 210mm × 297mm")

# Verificações
print("\n✓ Verificação de largura:")
if width <= 210:
    print(f"  ✅ OK - {width}mm cabe em A4 (margem: {210-width:.1f}mm)")
else:
    print(f"  ❌ OVERFLOW - {width}mm excede A4 em {width-210:.1f}mm")

print("\n✓ Verificação de altura:")
if height <= 297:
    print(f"  ✅ OK - {height}mm cabe em A4 (margem: {297-height:.1f}mm)")
else:
    print(f"  ❌ OVERFLOW - {height}mm excede A4 em {height-297:.1f}mm")

# Contar seções
sections = {
    "Botões Frontais (8mm)": 0,
    "Streaming/Utilities (retangulares)": 0,
    "Encoders Laterais (thumbs)": 0,
    "Extras/Tras": 0
}

# Botões 8mm: círculos
circles = root.findall('.//{http://www.w3.org/2000/svg}use[@href="#btn-8mm"]')
sections["Botões Frontais (8mm)"] = len(circles)

# Botões retangulares: rect elements
rects = root.findall('.//{http://www.w3.org/2000/svg}rect[@rx="0.5"]')
sections["Streaming/Utilities (retangulares)"] = len(rects)

# Encoders: thumb-label
thumbs = root.findall('.//{http://www.w3.org/2000/svg}use[@href="#thumb-label"]')
sections["Encoders Laterais (thumbs)"] = len(thumbs)

print("\n✓ Resumo de conteúdo:")
for section, count in sections.items():
    print(f"  • {section}: {count} items")

total = sum(sections.values())
print(f"\n  TOTAL: {total} stickers\n")
print("="*60)
