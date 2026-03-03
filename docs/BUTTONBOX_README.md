# ESP-ButtonBox-WHEEL — Firmware para Volante SimRacing

Firmware HID Gamepad completo para Button Box de volante de corrida.
Aparece no Windows como **"ESP-ButtonBox-WHEEL"** no joy.cpl.

## Hardware

| Item | Especificação |
|------|--------------|
| **MCU** | ESP32-S3-WROOM1 N8R8 (16MB Flash, 8MB PSRAM) |
| **Matriz de Botões** | 8x8 via MCP23017 (I2C) — 27 slots usados + 1 SHIFT |
| **Encoders** | 9x EC11 (1 MFC + 8 para eixos/botões) |
| **Sensores Hall** | 2x analógicos (embreagens esquerda/direita) |
| **5-Way Joystick** | HAT/POV (8 direções) + botão central |
| **Comunicação** | USB HID Gamepad (nativo) + UART para tela WT32 |

## Identidade USB

| Campo | Valor |
|-------|-------|
| VID | `0x303A` (Espressif) |
| PID | `0x8172` |
| Product Name | ESP-ButtonBox-WHEEL |
| Manufacturer | SimRacing_DIY |
| USB Mode | TinyUSB OTG (`USB_MODE=0`) |

## HID Report — 64 Botões + 10 Eixos + 1 HAT

### GamepadReport (19 bytes)

```c
typedef struct __attribute__((packed)) {
    uint64_t buttons;   // 64 botões (8 bytes)
    uint8_t  hat;       // HAT/POV: 4 bits + 4 padding (1 byte)
    int8_t   x;         // Encoder 2
    int8_t   y;         // Encoder 3
    int8_t   z;         // Hall A (Clutch L)
    int8_t   rz;        // Hall B (Clutch R)
    int8_t   rx;        // Encoder 4
    int8_t   ry;        // Encoder 5
    int8_t   slider;    // Encoder 6
    int8_t   dial;      // Encoder 7
    int8_t   vx;        // Encoder 8
    int8_t   vy;        // Encoder 9
} GamepadReport;
```

### 10 Eixos HID

| # | Eixo HID | Variável | Fonte | Função típica |
|---|----------|----------|-------|---------------|
| 1 | X | axisX | Encoder 2 | BB (Brake Bias) |
| 2 | Y | axisY | Encoder 3 | MAP |
| 3 | Z | axisZ | Hall A | Clutch Esquerda |
| 4 | Rz | axisRZ | Hall B | Clutch Direita |
| 5 | Rx | axisRX | Encoder 4 | TC |
| 6 | Ry | axisRY | Encoder 5 | ABS |
| 7 | Slider | axisSlider | Encoder 6 | Lateral 1 |
| 8 | Dial | axisDial | Encoder 7 | Lateral 2 |
| 9 | Vx | axisVx | Encoder 8 | Lateral 3 |
| 10 | Vy | axisVy | Encoder 9 | Lateral 4 |

> **Nota:** joy.cpl do Windows mostra apenas 8 eixos e 32 botões (limitação visual).
> Use https://gamepad-tester.com, JoyToKey ou SimHub para ver todos os 64 botões e 10 eixos.

### Botões HID

| Range | Fonte | Descrição |
|-------|-------|-----------|
| 1–27 | Matriz MCP23017 | Botões físicos (slot 28 = SHIFT interno, não reportado) |
| 23–26 | 5-Way Joystick | Direções consumidas pelo HAT (não aparecem como botões) |
| 27 | 5-Way Center | Botão OK/confirm |
| 40–55 | Encoders 2-9 | Botões virtuais (modo encoder-button): CW/CCW |
| 60–69 | MFC Menu | Botões virtuais: TC2, TC3, TYRE, VOL_A, VOL_B up/down |

### HAT/POV Switch

| Valor | Direção |
|-------|---------|
| 0 | Null (solto) |
| 1 | N (Up) |
| 2 | NE |
| 3 | E (Right) |
| 4 | SE |
| 5 | S (Down) |
| 6 | SW |
| 7 | W (Left) |
| 8 | NW |

## Pinagem ESP32-S3-WROOM1 N8R8

### I2C (MCP23017)

| Função | GPIO |
|--------|------|
| SDA | 8 |
| SCL | 9 |

### Sensores Hall (Embreagens)

| Sensor | GPIO | Eixo HID |
|--------|------|----------|
| Hall A (Clutch L) | 1 | Z |
| Hall B (Clutch R) | 2 | Rz |

### Encoders (9x)

| Encoder | Pin A | Pin B | Função |
|---------|-------|-------|--------|
| ENC1 (MFC) | 14 | 15 | Menu rotativo |
| ENC2 | 16 | 17 | Eixo X / BB |
| ENC3 | 18 | 21 | Eixo Y / MAP |
| ENC4 | 38 | 39 | Eixo Rx / TC |
| ENC5 | 40 | 41 | Eixo Ry / ABS |
| ENC6 | 42 | 47 | Eixo Slider |
| ENC7 | 48 | 35 | Eixo Dial |
| ENC8 | 36 | 37 | Eixo Vx |
| ENC9 | 3 | 46 | Eixo Vy |

### UART (para tela WT32)

| Função | GPIO | Baud |
|--------|------|------|
| TX | 43 | 115200 |

> Debug também sai pela UART TX (GPIO 43) via CH340 no Mac.

## Menu MFC (Encoder 1)

O encoder principal (MFC) navega por 15 itens de menu:

| # | Item | O que faz | Tipo |
|---|------|-----------|------|
| 0 | CLUTCH | Cicla entre 6 modos de embreagem | Config |
| 1 | BITE | Ajusta bite point (0-100%) | Config |
| 2 | CALIB | Inicia/para calibração dos Hall sensors | Config |
| 3 | ENC MODE | Alterna encoders entre eixos ↔ botões | Config |
| 4 | BRIGHT | Ajusta brilho (15-255) | UART → tela |
| 5 | PAGE | Troca página da tela | UART → tela |
| 6 | VOL_SYS | Volume do sistema (Consumer Control) | Multimídia |
| 7 | VOL_A | Volume app A | Botão virtual 66-67 |
| 8 | VOL_B | Volume app B | Botão virtual 68-69 |
| 9 | TC2 | Traction Control 2 | Botão virtual 60-61 |
| 10 | TC3 | Traction Control 3 | Botão virtual 62-63 |
| 11 | TYRE | Tipo de pneu | Botão virtual 64-65 |
| 12 | ERS | Modo ERS (Balanced/Harvest/Deploy/Hotlap) | Config |
| 13 | FUEL | Mix de combustível (0-100) | Config |
| 14 | RESET | Reseta todas as configs para padrão | Config |

**Navegação:** Gire o encoder MFC para selecionar item. Pressione botão 1 (MFC push) para entrar em modo ajuste. Gire para ajustar valor. Pressione novamente para sair.

## 6 Modos de Embreagem

| # | Modo | Comportamento |
|---|------|---------------|
| 0 | DUAL | Ambas as paddles independentes (Z e Rz) |
| 1 | MIRROR | Média das duas paddles |
| 2 | BITE | F1 Style com bite point configurável |
| 3 | PROGRESSIVE | Paddle esq limita a dir |
| 4 | SINGLE_L | Só paddle esquerda (Z) |
| 5 | SINGLE_R | Só paddle direita (Rz) |

## Compilação e Upload

### Compilar

```bash
pio run -e wroom1-n8r8-wheel
```

### Upload (via CH340)

```bash
pio run -e wroom1-n8r8-wheel --target upload --upload-port /dev/cu.usbmodem5AF61103441
```

### Monitor Serial (debug via CH340)

```bash
pio device monitor -e wroom1-n8r8-wheel -p /dev/cu.usbmodem5AF61103441
```

## Arquitetura do Build

| Arquivo | Função |
|---------|--------|
| `src/main_wheel.cpp` | Firmware principal (1183 linhas) |
| `platformio.ini` | Env `wroom1-n8r8-wheel` |
| `variants/wroom1_wheel/pins_arduino.h` | Variante customizada (PID=0x8172) |
| `scripts/patch_hid_name.py` | Patch pré-build: muda "TinyUSB HID" → "ESP-ButtonBox-WHEEL" no framework |

### Por que precisamos de cada arquivo extra

- **Custom variant**: O `pins_arduino.h` padrão do ESP32-S3 define `USB_PID 0x1001` SEM `#ifndef` guard, impedindo override via `-D`. A variante customizada define `USB_PID 0x8172`.

- **Patch script**: O joy.cpl do Windows usa o HID interface string descriptor (não o USB product string). O framework Arduino-ESP32 hardcoda "TinyUSB HID" em `USBHID.cpp`. O script pré-build substitui por "ESP-ButtonBox-WHEEL".

## Uso no Windows

1. Conecte o cabo USB **nativo** (não o CH340) ao PC Windows
2. O dispositivo aparece automaticamente como **"ESP-ButtonBox-WHEEL"** em:
   - **Gerenciador de Dispositivos** → Dispositivos de Interface Humana
   - **joy.cpl** → Controladores de Jogo
3. Todos os 64 botões + 10 eixos + HAT são acessíveis via DirectInput
4. SimHub detecta automaticamente como gamepad HID

## Limitações do joy.cpl

| O que o joy.cpl mostra | O que realmente existe |
|------------------------|----------------------|
| 32 botões | 64 botões |
| 8 eixos (sem Vx/Vy) | 10 eixos |
| 1 HAT/POV | 1 HAT/POV |

Para verificar tudo: https://gamepad-tester.com ou **SimHub → Controles → Input Detection**.

## Documentação Relacionada

| Doc | Conteúdo |
|-----|----------|
| [MANUAL_BUTTONBOX.md](MANUAL_BUTTONBOX.md) | Manual completo com combos e atalhos |
| [MFC_MENU_IMPLEMENTATION.md](MFC_MENU_IMPLEMENTATION.md) | Implementação detalhada do menu MFC |
| [PINMAP_SOLDERING_GUIDE.md](PINMAP_SOLDERING_GUIDE.md) | Guia de soldagem passo a passo |
| [MCP23017_MATRIZ_8x8.md](MCP23017_MATRIZ_8x8.md) | Detalhes da matriz 8x8 via I2C |
| [COMO_CONFIGURAR_SIMHUB.md](COMO_CONFIGURAR_SIMHUB.md) | Configuração do SimHub (dashboard + wheel) |
| [BUTTONBOX_README_S3ZERO_LEGACY.md](BUTTONBOX_README_S3ZERO_LEGACY.md) | Legacy: firmware antigo para ESP32-S3-Zero |

---

**Firmware: main_wheel.cpp | Env: wroom1-n8r8-wheel | Device: ESP-ButtonBox-WHEEL**
