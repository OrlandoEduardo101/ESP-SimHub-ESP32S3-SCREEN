#!/usr/bin/env python3
"""
Converte imagem PNG para array RGB565 que pode ser usado no ESP32
"""
import sys
import os

try:
    from PIL import Image
except ImportError:
    print("Instalando Pillow...")
    os.system("pip3 install Pillow")
    from PIL import Image

def rgb_to_rgb565(r, g, b):
    """Converte RGB (0-255) para RGB565 (16-bit)"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_png_to_rgb565_array(png_path, max_width=480, max_height=320, maintain_aspect=True):
    """Converte PNG para array RGB565 mantendo proporções"""
    # Abre a imagem
    img = Image.open(png_path)
    original_width, original_height = img.size

    # Preserva transparência - converte para RGBA se necessário
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    # Mantém o canal alpha para processar transparência
    has_alpha = True

    # Calcula tamanho mantendo proporção
    if maintain_aspect:
        aspect_ratio = original_width / original_height
        max_aspect = max_width / max_height

        if aspect_ratio > max_aspect:
            # Imagem é mais larga - limita pela largura
            output_width = max_width
            output_height = int(max_width / aspect_ratio)
        else:
            # Imagem é mais alta - limita pela altura
            output_height = max_height
            output_width = int(max_height * aspect_ratio)
    else:
        output_width = max_width
        output_height = max_height

    # Redimensiona mantendo proporção
    img = img.resize((output_width, output_height), Image.Resampling.LANCZOS)

    return img, output_width, output_height, max_width, max_height

    # Converte para array RGB565
    pixels = []
    for y in range(output_height):
        row = []
        for x in range(output_width):
            r, g, b = img.getpixel((x, y))
            rgb565 = rgb_to_rgb565(r, g, b)
            row.append(rgb565)
        pixels.append(row)

    return pixels, output_width, output_height

def generate_header_file(pixels, width, height, max_width, max_height, output_path):
    """Gera arquivo header C++ com o array RGB565"""
    with open(output_path, 'w') as f:
        f.write("#ifndef LOGO_IMAGE_H\n")
        f.write("#define LOGO_IMAGE_H\n\n")
        f.write(f"#define LOGO_WIDTH {width}\n")
        f.write(f"#define LOGO_HEIGHT {height}\n")
        f.write(f"#define LOGO_MAX_WIDTH {max_width}\n")
        f.write(f"#define LOGO_MAX_HEIGHT {max_height}\n\n")
        f.write("const uint16_t logo_image[LOGO_HEIGHT][LOGO_WIDTH] PROGMEM = {\n")

        for y, row in enumerate(pixels):
            f.write("  {")
            f.write(", ".join([f"0x{rgb565:04X}" for rgb565 in row]))
            f.write("}")
            if y < height - 1:
                f.write(",")
            f.write("\n")

        f.write("};\n\n")
        f.write("#endif // LOGO_IMAGE_H\n")

if __name__ == "__main__":
    png_path = "img/logo.png"
    output_header = "src/logo_image.h"

    if not os.path.exists(png_path):
        print(f"Erro: Arquivo {png_path} não encontrado!")
        sys.exit(1)

    print(f"Convertendo {png_path} para array RGB565 (mantendo proporções)...")
    # Reduzir tamanho máximo para deixar espaço para o texto "Loading" abaixo
    # Usar ~75% da altura da tela para a logo, deixando espaço para texto
    max_display_width = 480
    max_display_height = 320
    logo_max_height = int(max_display_height * 0.75)  # 75% da altura para logo
    logo_max_width = int(max_display_width * 0.90)    # 90% da largura para logo

    img_resized, width, height, max_width, max_height = convert_png_to_rgb565_array(
        png_path, logo_max_width, logo_max_height, maintain_aspect=True
    )

    # Converte para array RGB565, preservando transparência
    # Pixels transparentes serão convertidos para preto (0x0000) para fundo preto
    pixels = []
    for y in range(height):
        row = []
        for x in range(width):
            pixel = img_resized.getpixel((x, y))
            if len(pixel) == 4:  # RGBA
                r, g, b, a = pixel
                # Se o pixel é transparente (alpha < 128), usa preto
                if a < 128:
                    rgb565 = 0x0000  # Preto para fundo transparente
                else:
                    rgb565 = rgb_to_rgb565(r, g, b)
            else:  # RGB
                r, g, b = pixel
                rgb565 = rgb_to_rgb565(r, g, b)
            row.append(rgb565)
        pixels.append(row)

    print(f"Gerando arquivo header {output_header}...")
    generate_header_file(pixels, width, height, max_width, max_height, output_header)

    print(f"✅ Conversão concluída!")
    print(f"   Tamanho original: {Image.open(png_path).size[0]}x{Image.open(png_path).size[1]}")
    print(f"   Tamanho final: {width}x{height} (mantendo proporção)")
    print(f"   Área máxima: {max_width}x{max_height}")
    print(f"   Tamanho do array: {width * height * 2} bytes ({width * height * 2 / 1024:.2f} KB)")

