# Guia Completo de Soldagem — ESP32-S3 Button Box Wheel

> **LEIA TUDO antes de soldar.** Este documento mapeia cada fio, cada pino e cada componente.

---

## Índice

1. [Lista de Componentes](#1-lista-de-componentes)
2. [Diagrama de Blocos](#2-diagrama-de-blocos)
3. [GND e Alimentação](#3-gnd-e-alimentação)
4. [MCP23017 (I2C → Matriz)](#4-mcp23017-i2c--matriz)
5. [Matriz 8x8 — Mapa de Slots](#5-matriz-8x8--mapa-de-slots)
6. [Onde soldar cada botão na matriz](#6-onde-soldar-cada-botão-na-matriz)
7. [PCA9685 (I2C → LEDs Frontais)](#7-pca9685-i2c--leds-frontais)
8. [Encoders (A/B direto no ESP32)](#8-encoders-ab-direto-no-esp32)
9. [Hall Sensors (Embreagens)](#9-hall-sensors-embreagens)
10. [UART → WT32 (Tela)](#10-uart--wt32-tela)
11. [USB](#11-usb)
12. [Checklist de Soldagem](#12-checklist-de-soldagem)
13. [Dicas Práticas](#13-dicas-práticas)
14. [Troubleshooting](#14-troubleshooting)

---

## 1) Lista de Componentes

| Qty | Componente | Observação |
|-----|-----------|------------|
| 1 | ESP32-S3-WROOM1 N8R8 | MCU principal (USB HID) |
| 1 | MCP23017 (DIP ou SOIC) | Expansor I2C para matriz (0x20) |
| 1 | PCA9685 (módulo 16ch PWM I2C) | Driver PWM para LEDs frontais (0x40) |
| 1 | WT32-SC01 Plus (ou similar) | Tela/dashboard (recebe UART) |
| 9 | Encoder EC11 (com switch) | MFC + 4 frontais + 4 laterais |
| 2 | Sensor Hall analógico (49E/SS495A) | Embreagens (clutch paddles) |
| 28 | Diodo 1N4148 | 1 por botão/switch na matriz |
| 12 | Botões push com LED vermelho | Frontais (6 col. esq + 6 col. dir), 4 terminais |
| 2 | Botão push (traseiro) | Botões extras traseiros |
| 2 | Micro switch (borboleta) | Marcha UP/DOWN |
| 1 | 5-way joystick (push) | D-Pad + center click |
| 12 | Resistor 68Ω (¼W) | 1 em série por LED frontal |
| — | Fios, conectores, solda | — |

---

## 2) Diagrama de Blocos

```
                         ┌──────────────────────────────┐
                         │      ESP32-S3-WROOM1 N8R8     │
                         │                                │
  USB-C ◄───────────────►│  USB (HID Gamepad + Consumer)  │
                         │                                │
                         │  GPIO 8 (SDA) ──┬──► MCP23017  │
                         │  GPIO 9 (SCL) ──┤    (0x20)     │
                         │                 │      ↕        │
                         │                 │  Matriz 8x8   │
                         │                 │  (64 slots)   │
                         │                 │               │
                         │                 ├──► PCA9685    │
                         │                 │    (0x40)     │
                         │                 │      ↕        │
                         │                 │  12 LEDs      │
                         │                 │  (CH0–CH11)   │
                         │                                │
                         │  GPIO 14,15 ────► ENC1 (MFC)   │
                         │  GPIO 16,17 ────► ENC2 (BB)    │
                         │  GPIO 18,21 ────► ENC3 (MAP)   │
                         │  GPIO 38,39 ────► ENC4 (TC)    │
                         │  GPIO 40,41 ────► ENC5 (ABS)   │
                         │  GPIO 42,47 ────► ENC6 (Lat.1) │
                         │  GPIO 48,35 ────► ENC7 (Lat.2) │
                         │  GPIO 36,37 ────► ENC8 (Lat.3) │
                         │  GPIO 3,46  ────► ENC9 (Lat.4) │
                         │                                │
                         │  GPIO 1 ────► Hall A (Clutch L) │
                         │  GPIO 2 ────► Hall B (Clutch R) │
                         │                                │
                         │  GPIO 43 (TX) ──► WT32 GPIO11  │
                         └──────────────────────────────────┘
```

---

## 3) GND e Alimentação

### Regra de Ouro: GND COMUM

**TODOS** os módulos devem compartilhar o mesmo GND:

```
ESP32 GND ──┬── MCP23017 VSS (pin 10)
             ├── WT32 GND
             ├── Hall A GND
             ├── Hall B GND
             ├── Encoder 1 GND (C)
             ├── Encoder 2 GND (C)
             ├── Encoder 3 GND (C)
             ├── ... (todos os encoders)
             └── Encoder 9 GND (C)
```

### Alimentação 3.3V

```
ESP32 3.3V ──┬── MCP23017 VDD (pin 9)
              ├── MCP23017 RESET (pin 18)  ← ou use 10kΩ pullup
              ├── PCA9685 VCC
              ├── PCA9685 V+ (alimentação LEDs)
              ├── Hall A VCC
              └── Hall B VCC
```

> **Corrente dos LEDs:** 12 LEDs × 15mA ≈ 180mA. O regulador 3.3V do ESP32-S3 suporta \~600mA — dentro do limite. Se usar LEDs mais brilhantes (>20mA), use fonte externa no V+ do PCA9685.

> **⚠️ NUNCA 5V nos Halls ou nas entradas do ESP32. Sempre 3.3V.**

---

## 4) MCP23017 (I2C → Matriz)

### Pinout do MCP23017 (DIP-28)

```
         ┌────u────┐
  GPB0  1│         │28 GPA7
  GPB1  2│         │27 GPA6
  GPB2  3│         │26 GPA5
  GPB3  4│         │25 GPA4
  GPB4  5│         │24 GPA3
  GPB5  6│         │23 GPA2
  GPB6  7│         │22 GPA1
  GPB7  8│         │21 GPA0
   VDD  9│         │20 INTA
   VSS 10│         │19 INTB
    NC 11│         │18 RESET
   SCL 12│         │17 A2
   SDA 13│         │16 A1
    NC 14│         │15 A0
         └─────────┘
```

### Tabela de Conexões

| MCP23017 Pin | Nome | Conecta em | Observação |
|:---:|:---:|:---:|:---:|
| 9 | VDD | 3.3V | Alimentação |
| 10 | VSS | GND | Terra |
| 12 | SCL | ESP32 GPIO 9 | I2C Clock |
| 13 | SDA | ESP32 GPIO 8 | I2C Data |
| 15 | A0 | GND | Endereço bit 0 |
| 16 | A1 | GND | Endereço bit 1 |
| 17 | A2 | GND | Endereço bit 2 |
| 18 | RESET | 3.3V | Mantém ativo (ou 10kΩ p/ 3.3V) |
| 20 | INTA | — | Não usado |
| 19 | INTB | — | Não usado |
| 21–28 | GPA0–GPA7 | Colunas COL0–COL7 | OUTPUT (scan) |
| 1–8 | GPB0–GPB7 | Linhas ROW0–ROW7 | INPUT_PULLUP (leitura) |

**Endereço I2C = 0x20** (A0=A1=A2=GND)

---

## 5) Matriz 8x8 — Mapa de Slots

Cada cruzamento coluna × linha = 1 slot (botão).

Fórmula: **Slot = (ROW × 8) + COL + 1**

```
              COL0    COL1    COL2    COL3    COL4    COL5    COL6    COL7
              GPA0    GPA1    GPA2    GPA3    GPA4    GPA5    GPA6    GPA7
              (21)    (22)    (23)    (24)    (25)    (26)    (27)    (28)
         ┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
ROW0 GPB0│  Sl.1  │  Sl.2  │  Sl.3  │  Sl.4  │  Sl.5  │  Sl.6  │  Sl.7  │  Sl.8  │
🔵(pin 1)│ MFC SW │ ENC2 SW│ ENC3 SW│ ENC4 SW│ ENC5 SW│(livre) │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW1 GPB1│  Sl.9  │ Sl.10  │ Sl.11  │ Sl.12  │ Sl.13  │ Sl.14  │ Sl.15  │ Sl.16  │
🟠(pin 2)│⚡Ext.1 │⚡Ext.2 │⚡Ext.3 │⚡Ext.4 │⚡RADIO │⚡FLASH │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW2 GPB2│ Sl.17  │ Sl.18  │ Sl.19  │ Sl.20  │ Sl.21  │ Sl.22  │ Sl.23  │ Sl.24  │
⬜(pin 3)│⚡Frt.1 │⚡Frt.2 │⚡Frt.3 │⚡Frt.4 │⚡Frt.5 │⚡Frt.6 │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW3 GPB3│ Sl.25  │ Sl.26  │ Sl.27  │ Sl.28  │ Sl.29  │ Sl.30  │ Sl.31  │ Sl.32  │
🟢(pin 4)│ 5w UP  │5w DOWN │5w LEFT │5w RIGHT│5w CENT.│ SHIFT  │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW4 GPB4│ Sl.33  │ Sl.34  │ Sl.35  │ Sl.36  │ Sl.37  │ Sl.38  │ Sl.39  │ Sl.40  │
🟤(pin 5)│Borb.UP │BorbDWN │ Tras.1 │ Tras.2 │ Frnt7  │(livre) │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW5 GPB5│ Sl.41  │ Sl.42  │ Sl.43  │ Sl.44  │ Sl.45  │ Sl.46  │ Sl.47  │ Sl.48  │
 (pin 6) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW6 GPB6│ Sl.49  │ Sl.50  │ Sl.51  │ Sl.52  │ Sl.53  │ Sl.54  │ Sl.55  │ Sl.56  │
 (pin 7) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │
         ├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
ROW7 GPB7│ Sl.57  │ Sl.58  │ Sl.59  │ Sl.60  │ Sl.61  │ Sl.62  │ Sl.63  │ Sl.64  │
 (pin 8) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │(livre) │
         └────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘
```

### Legenda dos Slots Usados

> 💡 **Botões com LED** = têm 4 terminais físicos: 2 para switch (→ MCP23017) e 2 para LED (→ PCA9685).
> Botões **sem LED** = apenas 2 terminais, só vão para a matriz.

| Slot | Função | Tipo | LED PCA9685 | HID |
|:---:|:---:|:---:|:---:|:---:|
| 1 | MFC SW (press encoder MFC) | Momentâneo | — | Button 1 |
| 2 | ENC2 SW (BB switch) | Momentâneo | — | Button 2 |
| 3 | ENC3 SW (MAP switch) | Momentâneo | — | Button 3 |
| 4 | ENC4 SW (TC switch) | Momentâneo | — | Button 4 |
| 5 | ENC5 SW (ABS switch) | Momentâneo | — | Button 5 |
| 6–8 | Livres (GPB0) | — | — | — |
| 9 | ⚡ Extra 1 (coluna esquerda) | Momentâneo | **CH 0** | Button 9 |
| 10 | ⚡ Extra 2 (coluna esquerda) | Momentâneo | **CH 1** | Button 10 |
| 11 | ⚡ Extra 3 (coluna esquerda) | Momentâneo | **CH 2** | Button 11 |
| 12 | ⚡ Extra 4 (coluna esquerda) | Momentâneo | **CH 3** | Button 12 |
| 13 | ⚡ RADIO (Mute em VOL_SYS) | Momentâneo | **CH 4** | Button 13 |
| 14 | ⚡ FLASH (Play/Pause em VOL_SYS) | Momentâneo | **CH 5** | Button 14 |
| 15–16 | Livres (GPB1) | — | — | — |
| 17 | ⚡ Frontal 1 (coluna direita) | Momentâneo | **CH 6** | Button 17 |
| 18 | ⚡ Frontal 2 (coluna direita) | Momentâneo | **CH 7** | Button 18 |
| 19 | ⚡ Frontal 3 (coluna direita) | Momentâneo | **CH 8** | Button 19 |
| 20 | ⚡ Frontal 4 (coluna direita) | Momentâneo | **CH 9** | Button 20 |
| 21 | ⚡ Frontal 5 (coluna direita) | Momentâneo | **CH 10** | Button 21 |
| 22 | ⚡ Frontal 6 (coluna direita) | Momentâneo | **CH 11** | Button 22 |
| 23–24 | Livres (GPB2) | — | — | — |
| 25 | 5-way UP | Momentâneo | — | **HAT N** (não botão) |
| 26 | 5-way DOWN | Momentâneo | — | **HAT S** (não botão) |
| 27 | 5-way LEFT | Momentâneo | — | **HAT W** (não botão) |
| 28 | 5-way RIGHT | Momentâneo | — | **HAT E** (não botão) |
| 29 | 5-way CENTER (OK/confirm) | Momentâneo | — | Button 29 |
| 30 | SHIFT (modificador interno) | Momentâneo | — | **Interno** (não HID) |
| 31–32 | Livres (GPB3) | — | — | — |
| 33 | Borboleta UP (marcha +) | Momentâneo | — | Button 33 |
| 34 | Borboleta DOWN (marcha −) | Momentâneo | — | Button 34 |
| 35 | Botão traseiro 1 | Momentâneo | — | Button 35 |
| 36 | Botão traseiro 2 | Momentâneo | — | Button 36 |
| 37 | Frontal 7 (sem LED) | Momentâneo | — | Button 37 |
| 38–64 | **Livres** (expansão futura) | — | — | — |

> ⚡ = Botão com LED vermelho de 4 terminais (soldado também no PCA9685). **12 botões no total** (slots 9–14 e 17–22).

---

## 6) Onde Soldar Cada Botão na Matriz

### Regra do Diodo

**CADA botão na matriz precisa de um diodo 1N4148** para evitar ghost keys.

Sentido: **ROW → ▷| → Botão → COL**

```
  ROW (GPBx)──►|──[BOTÃO]── COL (GPAx)
           diodo 1N4148
           (banda no lado do ROW)
```

O lado com a **banda/faixa preta** do diodo fica no lado da **LINHA (ROW)**.

### Cores dos Fios — ROW (GPB) e COL (GPA)

| Linha (ROW) | Pino MCP | Cor do Fio |
|:---:|:---:|:---:|
| GPB0 | pin 1 | 🔵 Azul |
| GPB1 | pin 2 | 🟠 Laranja |
| GPB2 | pin 3 | ⬜ Cinza |
| GPB3 | pin 4 | 🟢 Verde |
| GPB4 | pin 5 | 🟤 Marrom |
| GPB5 | pin 6 | 🩷 Rosa |
| GPB6 | pin 7 | ⚪ Branco (marca preta) |
| GPB7 | pin 8 | ⚫ Preto (marca branca) |

| Coluna (COL) | Pino MCP | Cor do Fio |
|:---:|:---:|:---:|
| GPA0 | pin 21 | ⚪ Branco |
| GPA1 | pin 22 | 🟤 Marrom |
| GPA2 | pin 23 | 🔴 Vermelho |
| GPA3 | pin 24 | 🟡 Amarelo |
| GPA4 | pin 25 | ⚫ Preto |
| GPA5 | pin 26 | 🟣 Roxo |
| GPA6 | pin 27 | 🟩 Verde-claro |
| GPA7 | pin 28 | 🟧 Laranja-claro |

### Tabela de Soldagem — Botão por Botão

> **Para cada botão:** solde o diodo entre o pino ROW e um terminal do botão, depois o outro terminal do botão vai no pino COL.

| # | Componente | Pino A (ROW+diodo) | Pino B (COL) | Slot | LED (PCA9685) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| 1 | MFC SW (push encoder MFC) | 🔵 GPB0 (MCP pin 1) | ⚪ GPA0 (MCP pin 21) | 1 | — |
| 2 | ENC2 SW (BB push) | 🔵 GPB0 (MCP pin 1) | 🟤 GPA1 (MCP pin 22) | 2 | — |
| 3 | ENC3 SW (MAP push) | 🔵 GPB0 (MCP pin 1) | 🔴 GPA2 (MCP pin 23) | 3 | — |
| 4 | ENC4 SW (TC push) | 🔵 GPB0 (MCP pin 1) | 🟡 GPA3 (MCP pin 24) | 4 | — |
| 5 | ENC5 SW (ABS push) | 🔵 GPB0 (MCP pin 1) | ⚫ GPA4 (MCP pin 25) | 5 | — |
| 6 | ⚡ Extra 1 | 🟠 GPB1 (MCP pin 2) | ⚪ GPA0 (MCP pin 21) | 9 | **CH 0** |
| 7 | ⚡ Extra 2 | 🟠 GPB1 (MCP pin 2) | 🟤 GPA1 (MCP pin 22) | 10 | **CH 1** |
| 8 | ⚡ Extra 3 | 🟠 GPB1 (MCP pin 2) | 🔴 GPA2 (MCP pin 23) | 11 | **CH 2** |
| 9 | ⚡ Extra 4 | 🟠 GPB1 (MCP pin 2) | 🟡 GPA3 (MCP pin 24) | 12 | **CH 3** |
| 10 | ⚡ RADIO | 🟠 GPB1 (MCP pin 2) | ⚫ GPA4 (MCP pin 25) | 13 | **CH 4** |
| 11 | ⚡ FLASH | 🟠 GPB1 (MCP pin 2) | 🟣 GPA5 (MCP pin 26) | 14 | **CH 5** |
| 12 | ⚡ Frontal 1 | ⬜ GPB2 (MCP pin 3) | ⚪ GPA0 (MCP pin 21) | 17 | **CH 6** |
| 13 | ⚡ Frontal 2 | ⬜ GPB2 (MCP pin 3) | 🟤 GPA1 (MCP pin 22) | 18 | **CH 7** |
| 14 | ⚡ Frontal 3 | ⬜ GPB2 (MCP pin 3) | 🔴 GPA2 (MCP pin 23) | 19 | **CH 8** |
| 15 | ⚡ Frontal 4 | ⬜ GPB2 (MCP pin 3) | 🟡 GPA3 (MCP pin 24) | 20 | **CH 9** |
| 16 | ⚡ Frontal 5 | ⬜ GPB2 (MCP pin 3) | ⚫ GPA4 (MCP pin 25) | 21 | **CH 10** |
| 17 | ⚡ Frontal 6 | ⬜ GPB2 (MCP pin 3) | 🟣 GPA5 (MCP pin 26) | 22 | **CH 11** |
| 18 | 5-way UP | 🟢 GPB3 (MCP pin 4) | ⚪ GPA0 (MCP pin 21) | 25 | — |
| 19 | 5-way DOWN | 🟢 GPB3 (MCP pin 4) | 🟤 GPA1 (MCP pin 22) | 26 | — |
| 20 | 5-way LEFT | 🟢 GPB3 (MCP pin 4) | 🔴 GPA2 (MCP pin 23) | 27 | — |
| 21 | 5-way RIGHT | 🟢 GPB3 (MCP pin 4) | 🟡 GPA3 (MCP pin 24) | 28 | — |
| 22 | 5-way CENTER | 🟢 GPB3 (MCP pin 4) | ⚫ GPA4 (MCP pin 25) | 29 | — |
| 23 | SHIFT | 🟢 GPB3 (MCP pin 4) | 🟣 GPA5 (MCP pin 26) | 30 | — |
| 24 | Borboleta UP (+) | 🟤 GPB4 (MCP pin 5) | ⚪ GPA0 (MCP pin 21) | 33 | — |
| 25 | Borboleta DOWN (−) | 🟤 GPB4 (MCP pin 5) | 🟤 GPA1 (MCP pin 22) | 34 | — |
| 26 | Botão traseiro 1 | 🟤 GPB4 (MCP pin 5) | 🔴 GPA2 (MCP pin 23) | 35 | — |
| 27 | Botão traseiro 2 | 🟤 GPB4 (MCP pin 5) | 🟡 GPA3 (MCP pin 24) | 36 | — |
| 28 | Frontal 7 (sem LED) | 🟤 GPB4 (MCP pin 5) | ⚫ GPA4 (MCP pin 25) | 37 | — |

> ⚡ = Botão com LED vermelho (4 terminais). Os 2 terminais LED vão para o PCA9685 (ver seção 7). Os 2 terminais switch continuam aqui na matriz.

### Exemplo Visual — Um Botão

```
MCP23017                     Botão Frontal 1 (Slot 17)
                             ┌─────┐
GPB2 (pin 3) ──►|──────────┤ SW  ├────────── GPA0 (pin 21)
              diodo         └─────┘
              1N4148
              (banda no GPB2)
```

> **Terminais LED do mesmo botão** vão para o PCA9685 (ver seção 7), não para o MCP23017.

---

## 7) PCA9685 (I2C → LEDs Frontais)

O PCA9685 é um driver PWM I2C de 16 canais (12-bit, 4096 níveis). Ele controla os LEDs dos 12 botões frontais com breathing, sweep e flash — totalmente autônomo no firmware do wheel.

### Conexão I2C (paralelo ao MCP23017)

| PCA9685 Pin | Conecta em | Observação |
|:---:|:---:|:---:|
| VCC | ESP32 3.3V | Alimentação lógica |
| GND | GND comum | Terra |
| SDA | ESP32 GPIO 8 | Mesmo fio do MCP23017 |
| SCL | ESP32 GPIO 9 | Mesmo fio do MCP23017 |
| V+ | 3.3V | Alimentação dos LEDs |
| OE | — | Deixar desconectado (pull-down interno) |
| A0–A5 | GND | Endereço 0x40 (padrão) |

**Endereço I2C = 0x40** (A0–A5 = GND)

```
ESP32 GPIO 8 (SDA) ──┬── MCP23017 SDA (0x20)
                      └── PCA9685  SDA (0x40)

ESP32 GPIO 9 (SCL) ──┬── MCP23017 SCL
                      └── PCA9685  SCL
```

### Mapeamento Canais → LEDs

Cada botão frontal tem **4 terminais**: 2 para switch (→ MCP23017) e 2 para LED (→ PCA9685).

O LED é conectado com o **catodo no canal do PCA9685** e o **anodo no V+ (3.3V)**, com resistor 68Ω em série:

```
3.3V ──[68Ω]──[LED+]──[LED-]── PCA9685 CHx
```

| PCA9685 Canal | Botão | Slot Matriz | Coluna |
|:---:|:---:|:---:|:---:|
| CH 0 | Extra 1 | **9** | Esquerda (topo) |
| CH 1 | Extra 2 | **10** | Esquerda |
| CH 2 | Extra 3 | **11** | Esquerda |
| CH 3 | Extra 4 | **12** | Esquerda |
| CH 4 | RADIO | **13** | Esquerda |
| CH 5 | FLASH | **14** | Esquerda (base) |
| CH 6 | Frontal 1 | **17** | Direita (topo) |
| CH 7 | Frontal 2 | **18** | Direita |
| CH 8 | Frontal 3 | **19** | Direita |
| CH 9 | Frontal 4 | **20** | Direita |
| CH 10 | Frontal 5 | **21** | Direita |
| CH 11 | Frontal 6 | **22** | Direita (base) |
| CH 12–15 | — | — | Livres para expansão |

> **Nota:** Frontal 7 (slot 37) não tem LED no mapeamento padrão. Para adicionar, edite `SLOT_TO_LED_CH[]` em `main_wheel.cpp`.

### Efeitos Implementados no Firmware

| Evento | Efeito LED | Descrição |
|---|---|---|
| Boot | **Sweep + blink + fade** | Fecha de fora→dentro, 3 piscadas, fade para idle |
| Idle | **Breathing** | Onda senoidal suave (~2.5s ciclo, ±50% amplitude) |
| Botão pressionado | **Flash** | LED do botão vai ao máximo por 120ms |
| MFC girado CW | **Sweep L→R** | CH0→CH11 acendem em sequência |
| MFC girado CCW | **Sweep R→L** | CH11→CH0 acendem em sequência |
| SHIFT pressionado | **Blink alternado** | Pares e ímpares alternam a 200ms |
| BRIGHT ajustado | **Brilho proporcional** | Altera o nível base do breathing |

### Exemplo Visual — LED de um Botão

```
PCA9685                      Botão Frontal 1 (Slot 17)
                             ┌──────────────┐
3.3V ──[68Ω]────────────────►│ LED+ (anodo) │
                             │ LED- (catodo)│──── CH6 (pin 11 do PCA9685)
                             └──────────────┘

 (SW do mesmo botão vai normalmente para a matriz MCP23017 — seção 6)
```

---

## 8) Encoders (A/B Direto no ESP32)

Os encoders **A** e **B** vão direto no ESP32 (sem MCP23017).
O **SW** (push button) de cada encoder vai na **matriz** (com diodo, ver seção 6).
O **GND (C)** do encoder vai no **GND comum**.

### Pinagem Encoder EC11

```
        ┌─────┐
   A ───┤     ├─── B
        │ EC11│
   C ───┤     │
(GND)   │     │
        │ SW──┼── SW
        └─────┘    │
                   SW (outro terminal)
```

- **A** = sinal A (CLK)
- **B** = sinal B (DT)
- **C** = comum do encoder (GND)
- **SW** = push button (2 terminais, um vai no GND ou na matriz)

### Tabela de Soldagem — Encoders

| Encoder | Função | Pino A → ESP32 | Pino B → ESP32 | GND (C) | SW → Matriz |
|:---:|:---:|:---:|:---:|:---:|:---:|
| ENC1 | **MFC** (Menu) | GPIO 14 | GPIO 15 | GND | Slot 1 (ROW0/COL0) |
| ENC2 | **BB** (Brake Bias) | GPIO 16 | GPIO 17 | GND | Slot 2 (ROW0/COL1) |
| ENC3 | **MAP** (Engine Map) | GPIO 18 | GPIO 21 | GND | Slot 3 (ROW0/COL2) |
| ENC4 | **TC** (Traction Control) | GPIO 38 | GPIO 39 | GND | Slot 4 (ROW0/COL3) |
| ENC5 | **ABS** | GPIO 40 | GPIO 41 | GND | Slot 5 (ROW0/COL4) |
| ENC6 | **Lateral 1** | GPIO 42 | GPIO 47 | GND | — (SW não usado) |
| ENC7 | **Lateral 2** | GPIO 48 | GPIO 35 ⚠️ | GND | — (SW não usado) |
| ENC8 | **Lateral 3** | GPIO 36 ⚠️ | GPIO 37 ⚠️ | GND | — (SW não usado) |
| ENC9 | **Lateral 4** | GPIO 3 ⚠️ | GPIO 46 ⚠️ | GND | — (SW não usado) |

> **⚠️ GPIOs 35-37** podem ter conflito com PSRAM. **GPIO 3 e 46** são strapping pins.
> Teste esses encoders ANTES de soldar definitivamente. Se não funcionarem, precisará trocar os GPIOs.

### Exemplo Visual — Encoder MFC

```
ESP32-S3                     Encoder EC11 (MFC)
                             ┌──────────┐
GPIO 14 ────────────────────►│ A (CLK)  │
GPIO 15 ────────────────────►│ B (DT)   │
GND ────────────────────────►│ C (GND)  │
                             │          │
                             │ SW pin 1 │──►|── GPB0 (MCP pin 1)  ← Slot 1
                             │ SW pin 2 │──────── GPA0 (MCP pin 21)
                             └──────────┘
                                          diodo no SW!
```

### ENC6–ENC9: SW Não Utilizado

Os encoders laterais ficam na lateral do volante. Nessa posição, é **impossível** pressionar o push button (SW) durante a corrida. O firmware ignora esses SWs.

**Dica:** se quiser usar os slots livres do GPB0 (slots 6, 7, 8), conecte **botões extras** (tact switches separados) a esses slots em vez dos SWs dos encoders laterais.

---

## 9) Hall Sensors (Embreagens)

### Sensor Hall Analógico (ex: SS49E, SS495A)

```
┌──────────────┐
│  Hall Sensor  │
│               │
│  VCC ─────────│──► ESP32 3.3V
│  GND ─────────│──► GND comum
│  OUT ─────────│──► GPIO do ESP32
│               │
└──────────────┘
```

### Tabela de Soldagem — Halls

| Sensor | Função | VCC | GND | OUT → ESP32 | Eixo HID |
|:---:|:---:|:---:|:---:|:---:|:---:|
| Hall A | Clutch esquerda (L) | 3.3V | GND | **GPIO 1** | Z |
| Hall B | Clutch direita (R) | 3.3V | GND | **GPIO 2** | Rz |

> **⚠️ IMPORTANTE:** Alimentação **3.3V** (não 5V!). Se usar 5V, pode danificar o ESP32.

### Montagem na Borboleta

```
         Ímã (colado na borboleta)
            ↕  ← movimento linear
    ┌──────────────┐
    │  Hall Sensor  │ ← fixo no chassi do volante
    └──────────────┘
```

O Hall detecta a distância do ímã. Quanto mais perto, maior a tensão em OUT.

### Calibração

Após soldar, use o menu MFC:
1. Gire até **CALIB**, pressione MFC
2. Mova as borboletas do mínimo ao máximo
3. Pressione MFC para salvar
4. Se aparecer "CALIB ERR: HALL" no WT32, verifique as conexões

---

## 10) UART → WT32 (Tela)

Comunicação **unidirecional** do ESP32 para o WT32 (dashboard/display).

### Conexões

| ESP32-S3 | WT32-SC01 Plus | Função |
|:---:|:---:|:---:|
| GPIO 43 (TX) | GPIO 11 (RX) | Dados (115200 baud) |
| GND | GND | Terra comum |

> **Atenção:** TX → RX (cruzado!). O TX do ESP32 conecta no RX do WT32.

```
ESP32-S3                    WT32-SC01 Plus
┌──────────┐                ┌──────────┐
│ GPIO 43  │───────────────►│ GPIO 11  │
│  (TX)    │                │  (RX)    │
│          │                │          │
│   GND    │────────────────│   GND    │
└──────────┘                └──────────┘
```

> Não é necessário conectar RX do ESP32 ao TX do WT32 (comunicação unidirecional).

---

## 11) USB

O ESP32-S3 tem USB nativo. Basta conectar o cabo USB-C ao computador.

O firmware aparece como:
- **Gamepad**: 64 botões + 1 HAT/POV + 10 eixos
- **Consumer Control**: Volume +/−, Mute, Play/Pause

No Windows → "Dispositivos de Jogo" (joy.cpl), aparece como **"ESP-ButtonBox-WHEEL"**.

---

## 12) Checklist de Soldagem

### Fase 1: Alimentação e I2C
- [ ] GND comum conectado entre **todos** os módulos
- [ ] ESP32 3.3V → MCP23017 VDD (pin 9)
- [ ] MCP23017 VSS (pin 10) → GND
- [ ] MCP23017 RESET (pin 18) → 3.3V
- [ ] MCP23017 A0 (pin 15) → GND
- [ ] MCP23017 A1 (pin 16) → GND
- [ ] MCP23017 A2 (pin 17) → GND
- [ ] ESP32 GPIO 8 → MCP23017 SDA (pin 13)
- [ ] ESP32 GPIO 9 → MCP23017 SCL (pin 12)
- [ ] ESP32 3.3V → PCA9685 VCC
- [ ] ESP32 3.3V → PCA9685 V+
- [ ] PCA9685 GND → GND comum
- [ ] ESP32 GPIO 8 → PCA9685 SDA (paralelo ao MCP23017)
- [ ] ESP32 GPIO 9 → PCA9685 SCL (paralelo ao MCP23017)
- [ ] PCA9685 A0–A5 → GND (endereço 0x40)

### Fase 2: Teste I2C
- [ ] Upload do firmware e verificar **ambos** no Serial Monitor: "MCP23017 OK" e "PCA9685 LED driver OK"
- [ ] Se aparecer "MCP23017 not found", checar soldas SDA/SCL/VDD/VSS/endereço
- [ ] Se aparecer "PCA9685 not found", checar soldas SDA/SCL/VCC/A0-A5

### Fase 3: Encoders (A/B direto no ESP32)
- [ ] ENC1 (MFC): A→GPIO14, B→GPIO15, C→GND
- [ ] ENC2 (BB): A→GPIO16, B→GPIO17, C→GND
- [ ] ENC3 (MAP): A→GPIO18, B→GPIO21, C→GND
- [ ] ENC4 (TC): A→GPIO38, B→GPIO39, C→GND
- [ ] ENC5 (ABS): A→GPIO40, B→GPIO41, C→GND
- [ ] ENC6 (Lat.1): A→GPIO42, B→GPIO47, C→GND
- [ ] ENC7 (Lat.2): A→GPIO48, B→GPIO35⚠️, C→GND
- [ ] ENC8 (Lat.3): A→GPIO36⚠️, B→GPIO37⚠️, C→GND
- [ ] ENC9 (Lat.4): A→GPIO3⚠️, B→GPIO46⚠️, C→GND

### Fase 4: Teste Encoders
- [ ] Girar cada encoder e ver eixo mexer no Windows (Game Controllers)
- [ ] Se ENC7/8/9 não funcionar, trocar GPIOs (conflito PSRAM/strapping)

### Fase 5: Halls (Embreagens)
- [ ] Hall A: VCC→3.3V, GND→GND, OUT→GPIO1
- [ ] Hall B: VCC→3.3V, GND→GND, OUT→GPIO2
- [ ] Testar no Game Controllers: eixos Z e Rz devem se mover com o ímã

### Fase 5b: LEDs Frontais (PCA9685)
- [ ] Soldar 12 resistores 68Ω (um em série por LED)
- [ ] CH0: Extra1 LED → 3.3V–[68Ω]–LED+–LED-–CH0
- [ ] CH1: Extra2 LED → mesma lógica
- [ ] CH2: Extra3 LED
- [ ] CH3: Extra4 LED
- [ ] CH4: RADIO LED
- [ ] CH5: FLASH LED
- [ ] CH6: Frontal1 LED
- [ ] CH7: Frontal2 LED
- [ ] CH8: Frontal3 LED
- [ ] CH9: Frontal4 LED
- [ ] CH10: Frontal5 LED
- [ ] CH11: Frontal6 LED
- [ ] Verificar boot sweep: LEDs devem fechar de fora→dentro ao ligar
- [ ] Verificar breathing contínuo após boot

### Fase 6: Matriz de Botões (no MCP23017)
- [ ] Soldar todos os 28 diodos 1N4148 (banda/faixa no lado ROW)
- [ ] Slot 1: MFC SW → 🔵 GPB0(1) / ⚪ GPA0(21)
- [ ] Slots 2–5: Encoder SWs → 🔵 GPB0(1) / 🟤 GPA1(22), 🔴 GPA2(23), 🟡 GPA3(24), ⚫ GPA4(25)
- [ ] Slots 9–14: LEDs esquerda → 🟠 GPB1(2) / GPA0-5(21-26) — ver tabela seção 6
- [ ] Slots 17–22: LEDs direita → ⬜ GPB2(3) / GPA0-5(21-26) — ver tabela seção 6
- [ ] Slots 25–28: 5-way (UP/DOWN/LEFT/RIGHT) → 🟢 GPB3(4) / GPA0-3(21-24)
- [ ] Slot 29: 5-way CENTER → 🟢 GPB3(4) / ⚫ GPA4(25)
- [ ] Slot 30: SHIFT → 🟢 GPB3(4) / 🟣 GPA5(26)
- [ ] Slots 33–34: Borboletas → 🟤 GPB4(5) / ⚪ GPA0(21), 🟤 GPA1(22)
- [ ] Slots 35–36: Traseiros → 🟤 GPB4(5) / 🔴 GPA2(23), 🟡 GPA3(24)
- [ ] Slot 37: Frontal extra (sem LED) → 🟤 GPB4(5) / ⚫ GPA4(25)

### Fase 7: Teste Matriz
- [ ] Pressionar cada botão e verificar no Game Controllers (1–5, 9–14, 17–22, 29, 33–37 = botões; 25–28 = HAT/D-Pad)
- [ ] Verificar que SHIFT (30) **não** aparece no HID
- [ ] Verificar HAT/POV: círculo central no Game Controllers deve virar para N/S/E/W

### Fase 8: UART → WT32
- [ ] ESP32 GPIO 43 → WT32 GPIO 11 (TX→RX cruzado)
- [ ] GND comum
- [ ] Dashboard do WT32 deve mostrar dados após boot

### Fase 9: Teste Final Integrado
- [ ] Todos os 9 encoders giram e são detectados
- [ ] Todos os 28 botões (slots 1–5, 9–14, 17–22, 29, 33–37) aparecem no Game Controllers
- [ ] HAT/D-Pad funciona em 8 direções
- [ ] Embreagens Z/Rz respondem suavemente
- [ ] Menu MFC navega pelos 15 itens
- [ ] Consumer Control (volume/mute/play) funciona
- [ ] SHIFT combos funcionam (testar pelo menos 2)
- [ ] LEDs: boot sweep executa corretamente
- [ ] LEDs: breathing suave contínuo no idle
- [ ] LEDs: pressionar botão frontal → flash no LED correspondente
- [ ] LEDs: girar MFC → sweep L→R ou R→L
- [ ] LEDs: segurar SHIFT → padrão alternado

---

## 13) Dicas Práticas

### Ordem Recomendada de Soldagem

1. **MCP23017 + PCA9685** (alimentação + I2C) → testar comunicação de ambos
2. **LEDs frontais** (resistores + conexões PCA9685) → verificar boot sweep
3. **Encoders** (começar por MFC e frontais) → testar girar
4. **Halls** → testar eixos
5. **Botões da matriz** (começar com poucos) → testar conforme solda → verificar flash LED
6. **UART** → testar conexão WT32
7. **Últimos:** encoders laterais (GPIOs potencialmente problemáticos)

### Esquema de Cores de Fios (recomendado)

Use cores consistentes para identificar cada tipo de sinal. Sugestão para ribbon cable colorido (26 AWG):

| Cor | Tipo de Sinal | Destino |
|:---:|:---:|:---:|
| 🔴 Vermelho | Alimentação 3.3V | VDD/VCC de todos os módulos |
| ⚫ Preto | GND | GND comum (todos os módulos) |
| 🔵 Azul | I2C SDA | GPIO 8 → MCP23017 pin 13 + PCA9685 SDA |
| 🟡 Amarelo | I2C SCL | GPIO 9 → MCP23017 pin 12 + PCA9685 SCL |
| � Laranja | Encoder A (CLK) | GPIO 14/16/18/38/40/42/48/36/3 |
| 🟣 Roxo | Encoder B (DT) | GPIO 15/17/21/39/41/47/35/37/46 |
| 🩷 Rosa | Hall sensor OUT | GPIO 1 (Hall A) / GPIO 2 (Hall B) |
| 🟤 Marrom | UART TX | GPIO 43 → WT32 GPIO 11 |

**ROW (GPB) — matriz:**

| Fio | GPB | MCP Pin |
|:---:|:---:|:---:|
| 🔵 Azul | GPB0 | 1 |
| 🟠 Laranja | GPB1 | 2 |
| ⬜ Cinza | GPB2 | 3 |
| 🟢 Verde | GPB3 | 4 |
| 🟤 Marrom | GPB4 | 5 |
| 🩷 Rosa | GPB5 | 6 |
| ⚪ Branco (marca preta) | GPB6 | 7 |
| ⚫ Preto (marca branca) | GPB7 | 8 |

**COL (GPA) — matriz:**

| Fio | GPA | MCP Pin |
|:---:|:---:|:---:|
| ⚪ Branco | GPA0 | 21 |
| 🟤 Marrom | GPA1 | 22 |
| 🔴 Vermelho | GPA2 | 23 |
| 🟡 Amarelo | GPA3 | 24 |
| ⚫ Preto | GPA4 | 25 |
| 🟣 Roxo | GPA5 | 26 |
| 🟩 Verde-claro | GPA6 | 27 |
| 🟧 Laranja-claro | GPA7 | 28 |

**5-way + SHIFT (todos em ROW 🟢 verde / GPB3):**

| Função | Fio COL | GPA | Slot |
|:---:|:---:|:---:|:---:|
| 5-way UP | ⚪ Branco | GPA0 | 25 |
| 5-way DOWN | � Marrom | GPA1 | 26 |
| 5-way LEFT | 🔴 Vermelho | GPA2 | 27 |
| 5-way RIGHT | 🟡 Amarelo | GPA3 | 28 |
| 5-way CENTER | ⚫ Preto | GPA4 | 29 |
| SHIFT | 🟣 Roxo | GPA5 | 30 |

**Borboletas + Traseiros + Frontal extra (todos em ROW 🟤 marrom / GPB4):**

| Função | Fio COL | GPA | Slot |
|:---:|:---:|:---:|:---:|
| Borboleta UP | ⚪ Branco | GPA0 | 33 |
| Borboleta DOWN | 🟤 Marrom | GPA1 | 34 |
| Traseiro 1 | 🔴 Vermelho | GPA2 | 35 |
| Traseiro 2 | 🟡 Amarelo | GPA3 | 36 |
| Frontal 7 (sem LED) | ⚫ Preto | GPA4 | 37 |

> **Dica:** Ao usar ribbon cable, agrupe por função: 1 fita para COLs (8 fios), 1 fita para ROWs (8 fios), 1 fita para encoders (2 fios por encoder).

### Dicas de Solda

- Use fio 24-28 AWG (ribbon cable funciona bem)
- **Identifique cada fio com etiqueta** antes de soldar (slot #, GPIO #)
- Solde o diodo no fio ANTES de soldar no MCP23017
- Teste a continuidade com multímetro após cada solda
- Agrupe os fios: todas as COLs em um maço, todas as ROWs em outro
- Botões ⚡ com LED têm **4 fios saindo**: 2 terminais switch (→ MCP23017 via diodo) + 2 terminais LED (→ PCA9685)

### Possíveis Problemas com GPIOs

| GPIO | Problema | Solução |
|------|----------|---------|
| 35, 36, 37 | Pode conflitar com PSRAM (ESP32-S3 N8R8) | Testar antes; se falhar, mover encoder para outros GPIOs |
| 3 | Strapping pin (boot mode) | Testar; pode precisar ser substituído |
| 46 | Strapping pin | Testar; pode precisar ser substituído |
| 0 | Strapping pin | **NÃO USAR** (boot/download mode) |

### GPIOs Livres no ESP32-S3 (caso precise trocar)

GPIOs **seguros** que não estão em uso e não têm conflitos:
- GPIO 4, 5, 6, 7 (desde que não use outro barramento)
- GPIO 10, 11, 12, 13 (livres após migração para MCP23017)

---

## 14) Troubleshooting

| Problema | Causa Provável | Solução |
|----------|---------------|------|
| "MCP23017 not found" | SDA/SCL invertido, VDD faltando, endereço errado | Checar soldas, medir 3.3V no VDD, checar A0/A1/A2=GND |
| "PCA9685 not found" | SDA/SCL não conectado ao PCA9685, VCC faltando, A0-A5 não aterrados | Checar soldas, medir 3.3V no VCC, checar A0-A5=GND |
| LEDs não acendem | LED invertido (anodo/catodo trocado) ou resistor faltando | Verificar polaridade; anodo → 3.3V, catodo → CH do PCA9685 |
| LEDs muito fracos | Resistor muito alto | Usar 68Ω; para mais brilho, reduzir para 47Ω ou 33Ω |
| Todos LEDs acendem ao mesmo tempo (sem efeito) | I2C OK mas algum canal curto-circuito | Verificar soldas dos canais individuais |
| Boot sweep não executa | PCA9685 não reconhecido no I2C | Checar endereço 0x40 (A0-A5 = GND) |
| Botão não responde | Diodo invertido ou faltando | Verificar banda do diodo (faixa preta no lado ROW) |
| Dois botões ativam juntos | Diodo faltando (ghost key) | Adicionar diodo 1N4148 |
| Encoder gira para um lado só | A/B invertido | Trocar fios A↔B |
| Encoder pula valores | Ruído no sinal | Adicionar capacitor 100nF entre A-GND e B-GND |
| Hall não responde | VCC=5V (queimou) ou fio solto | Medir com multímetro; alimentar com 3.3V |
| HAT não funciona | 5-way nas posições erradas | Verificar UP=slot25, DOWN=26, LEFT=27, RIGHT=28 |
| SHIFT ativa como botão | Slot errado na matriz | SHIFT deve estar no slot 30 (ROW3/COL5) |
| WT32 não mostra dados | TX/RX não cruzado | GPIO43(TX) → GPIO11(RX) do WT32 |
| Volume/Mute não funciona | VOL_SYS não está em modo ajuste | Navegar até VOL_SYS no MFC e pressionar para entrar |

---

## Resumo Rápido — Todos os Fios

### ESP32-S3 GPIOs Utilizados

> **GPIO 8 e 9 (I2C)** agora têm **dois dispositivos** no barramento: MCP23017 (0x20) e PCA9685 (0x40).

| GPIO | Função | Destino |
|:---:|:---:|:---:|
| 1 | ADC (Hall A) | Sensor Hall esquerdo OUT |
| 2 | ADC (Hall B) | Sensor Hall direito OUT |
| 3 ⚠️ | ENC9_A | Encoder 9 pino A |
| 8 | I2C SDA | MCP23017 pin 13 |
| 9 | I2C SCL | MCP23017 pin 12 |
| 14 | ENC1_A | Encoder MFC pino A |
| 15 | ENC1_B | Encoder MFC pino B |
| 16 | ENC2_A | Encoder BB pino A |
| 17 | ENC2_B | Encoder BB pino B |
| 18 | ENC3_A | Encoder MAP pino A |
| 21 | ENC3_B | Encoder MAP pino B |
| 35 ⚠️ | ENC7_B | Encoder Lateral 2 pino B |
| 36 ⚠️ | ENC8_A | Encoder Lateral 3 pino A |
| 37 ⚠️ | ENC8_B | Encoder Lateral 3 pino B |
| 38 | ENC4_A | Encoder TC pino A |
| 39 | ENC4_B | Encoder TC pino B |
| 40 | ENC5_A | Encoder ABS pino A |
| 41 | ENC5_B | Encoder ABS pino B |
| 42 | ENC6_A | Encoder Lateral 1 pino A |
| 43 | UART TX | WT32 GPIO 11 (RX) |
| 46 ⚠️ | ENC9_B | Encoder 9 pino B |
| 47 | ENC6_B | Encoder Lateral 1 pino B |
| 48 | ENC7_A | Encoder Lateral 2 pino A |

**Total:** 23 GPIOs em uso + USB nativo

### GPIOs NÃO Utilizados (disponíveis)

4, 5, 6, 7, 10, 11, 12, 13, 19, 20, 44, 45

---

*Documento criado para o projeto ESP-ButtonBox-WHEEL. Revise antes de soldar!*
