# Volante Redondo — Guia Completo de Construção

> **Volante simplificado estilo Opala/Rally/GT** — usa a mesma ESP32-S3-WROOM1 N8R8, o mesmo firmware base (`main_wheel.cpp`) e é **intercambiável** com o volante de Fórmula 1 sem remapear controles no jogo.

---

## Índice

1. [Visão Geral da Arquitetura](#1-visão-geral-da-arquitetura)
2. [Diferenças em relação ao Volante F1](#2-diferenças-em-relação-ao-volante-f1)
3. [Lista de Compras](#3-lista-de-compras)
4. [Pinagem do ESP32-S3 — Volante Redondo](#4-pinagem-do-esp32-s3--volante-redondo)
5. [Diagrama de Blocos](#5-diagrama-de-blocos)
6. [Mapa de Slots da Matriz (Retrocompatível)](#6-mapa-de-slots-da-matriz-retrocompatível)
7. [Tabela de Soldagem — Botão por Botão](#7-tabela-de-soldagem--botão-por-botão)
8. [MAX7219 — Display 7 Segmentos](#8-max7219--display-7-segmentos)
9. [WS2812 — LEDs Endereçáveis (RPM/Alertas)](#9-ws2812--leds-endereçáveis-rpmalertas)
10. [Recepção SimHub via USB CDC](#10-recepção-simhub-via-usb-cdc)
11. [Adaptações no Firmware](#11-adaptações-no-firmware)
12. [Novo Env no platformio.ini](#12-novo-env-no-platformioini)
13. [Plano de Implementação por Fases](#13-plano-de-implementação-por-fases)
14. [Preservação do Volante F1 + WT32](#14-preservação-do-volante-f1--wt32)
15. [Intercâmbio Rápido de Volantes](#15-intercâmbio-rápido-de-volantes)
16. [Checklist de Montagem](#16-checklist-de-montagem)
17. [Troubleshooting](#17-troubleshooting)

---

## 1) Visão Geral da Arquitetura

### Volante F1 (existente — NÃO muda)

```
┌─────────────────┐     USB HID      ┌────────┐
│  ESP32-S3 WHEEL │◄────────────────►│   PC   │
│  (main_wheel)   │   Gamepad+CC     │ SimHub │
│                 │                   │  Jogo  │
│  GPIO 43 (TX)───┼──UART 115200──►  └────┬───┘
│  GPIO 11 (RX)◄──┤                       │ USB CDC
│                 │                   ┌────┴───┐
│  MCP23017 (I2C) │                   │  WT32  │
│  PCA9685  (I2C) │                   │SC01+   │
│  9 Encoders     │                   │  TFT   │
│  2 Halls        │                   │ WS2812 │
└─────────────────┘                   └────────┘
```

### Volante Redondo (novo)

```
┌────────────────────────────────────────────┐
│            ESP32-S3 WHEEL ROUND            │
│          (main_wheel_round.cpp)            │
│                                            │
│  USB Nativa (GPIO 19/20):                  │
│    ├── HID Gamepad + Consumer Control      │
│    └── CDC Serial ◄── SimHub telemetria    │
│                                            │
│  USB Debug (CH340/CP2102, GPIO 43/44):     │
│    └── Debug serial / Monitor              │
│                                            │
│  I2C (GPIO 8/9):                           │
│    └── MCP23017 (0x20) — Matriz 8x8       │
│                                            │
│  SPI Software (GPIO 4/12/13):              │
│    └── MAX7219 — Display 7seg (8 dígitos)  │
│                                            │
│  GPIO 10:                                  │
│    └── WS2812 — LEDs RPM/Alertas           │
│                                            │
│  Encoders diretos (mesmos GPIOs do F1):    │
│    ENC1-ENC5 (MFC, BB, MAP, TC, ABS)      │
│    ENC6-ENC9 opcionais (laterais)          │
│                                            │
│  Halls (GPIO 1/2):                         │
│    Hall A/B — Embreagens (opcional)        │
└────────────────────────────────────────────┘
```

**Princípio fundamental:** Uma placa, um cabo USB, três funções (HID + SimHub + debug separado).

---

## 2) Diferenças em relação ao Volante F1

| Aspecto | Volante F1 | Volante Redondo |
|---------|-----------|-----------------|
| Tela | WT32-SC01 Plus (TFT 480x272) | MAX7219 8 dígitos 7seg |
| LEDs RPM/Alertas | WS2812 no WT32 | WS2812 direto no ESP32 |
| LEDs dos botões | PCA9685 (12 canais PWM) | **Não usa** |
| Botões com LED | 12 (4 terminais cada) | Botões simples (2 terminais) |
| Comunicação SimHub | Via WT32 (USB CDC separado) | Via ESP32 mesmo (USB CDC composta) |
| UART entre placas | GPIO 43→WT32, GPIO 11←WT32 | **Não usa** (sem WT32) |
| Display monocromático | — | MAX7219 SPI (GPIO 4/12/13) |
| Encoders | 9 (MFC + 4 frontais + 4 laterais) | Mínimo: 1 (MFC). Recomendado: 3-5 |
| Halls / Embreagens | 2 (GPIO 1/2) | Opcional (GPIO 1/2 se quiser) |
| 5-way joystick | Sim (slots 25-29) | Opcional |
| Cables USB | 2 (wheel + WT32) | 1 (só a ESP32) |
| Placas | 2 (ESP32 + WT32) | 1 (só ESP32) |

---

## 3) Lista de Compras

### Obrigatórios

| Qty | Componente | Preço Estimado | Observação |
|:---:|-----------|:--------------:|------------|
| 1 | ESP32-S3-WROOM1 N8R8 (DevKitC-1) | R$ 45-80 | **Mesma placa do F1.** 2 conectores USB-C |
| 1 | MCP23017 (DIP-28 ou módulo) | R$ 8-15 | Expansor I2C — matriz de botões (0x20) |
| 1 | Módulo MAX7219 8 dígitos 7 segmentos | R$ 12-25 | Display monocromático (Gear, Speed, popups) |
| 1 | Fita WS2812B (16-26 LEDs) | R$ 15-35 | RPM bar + shift light + alertas |
| 1 | Encoder EC11 (com push button) | R$ 3-8 | MFC (Menu — obrigatório) |
| — | Botões push momentâneos | R$ 1-3 cada | Quantidade depende do seu layout |
| — | Diodos 1N4148 | R$ 0,10 cada | 1 por botão na matriz (anti-ghosting) |
| — | Resistor 330Ω (¼W) | R$ 0,10 | 1 unidade, em série no DIN do WS2812 |
| — | Fios 24-28 AWG, solda, conectores | R$ 10-20 | Ribbon cable recomendado |
| 1 | Cabo USB-C (dados, não só carga) | R$ 10-20 | Para a USB **nativa** (GPIO 19/20) |

### Opcionais (Recomendados)

| Qty | Componente | Preço Estimado | Observação |
|:---:|-----------|:--------------:|------------|
| 2-4 | Encoder EC11 extra | R$ 3-8 cada | BB, MAP, TC, ABS (nos mesmos GPIOs do F1) |
| 2 | Sensor Hall SS49E/SS495A | R$ 5-10 cada | Embreagens (Clutch L/R, GPIO 1/2) |
| 1 | 5-way joystick switch | R$ 8-15 | D-Pad (slots 25-29) |
| 2 | Micro switch (borboletas) | R$ 2-5 cada | Marchas UP/DOWN (slots 33/34) |
| 1 | Botão SHIFT | R$ 1-3 | Slot 30 — modificador interno |
| 2 | Resistor 100kΩ (¼W) | R$ 0,10 cada | Divisor de tensão para GPIO 1/2 (ver nota abaixo) |
| — | Capacitores 100nF cerâmicos | R$ 0,10 cada | Debounce para encoders com ruído |

### NÃO comprar (diferente do F1)

| Componente | Motivo |
|-----------|--------|
| WT32-SC01 Plus | Sem tela TFT neste volante |
| PCA9685 | Sem LEDs PWM nos botões |
| Botões de 4 terminais (com LED) | Usar botões simples (2 terminais) |
| Resistores 68Ω | Eram para LEDs do PCA9685 |

### Preço total estimado (configuração mínima)

| Configuração | Componentes | Estimativa |
|-------------|-------------|:----------:|
| **Mínima** (MFC + 6 botões + display + LEDs) | ESP32 + MCP + MAX7219 + WS2812 + 1 enc + 6 botões | **R$ 100-150** |
| **Recomendada** (3 enc + 10 botões + halls + 5-way) | + 2 enc + halls + 5-way + mais botões | **R$ 150-220** |
| **Completa** (mesmo controles do F1) | Todos os 9 enc + halls + 5-way + borboletas | **R$ 200-280** |

---

## 4) Pinagem do ESP32-S3 — Volante Redondo

### GPIOs utilizados (retrocompatível com F1)

> **REGRA DE OURO:** Nenhum GPIO muda de função em relação ao volante F1.
> Controles que você não instalar simplesmente não são conectados — o firmware trata normalmente.

| GPIO | Função | Periférico | Obrigatório? |
|:----:|--------|------------|:------------:|
| **1** | Hall A (Clutch L) | ADC1_CH0 → Eixo Z | Opcional |
| **2** | Hall B (Clutch R) | ADC1_CH1 → Eixo Rz | Opcional |
| **3** ⚠️ | ENC9-A (Lateral 4) | Encoder direto | Opcional |

> **⚠️ Importante — GPIO 1/2 e sensores Hall:**
>
> O firmware detecta automaticamente a presença dos sensores por análise de variância
> no boot (`detectHallPresence()`). **O divisor de tensão é opcional** — serve apenas
> para dar uma leitura estável em VCC/2 (posição central do eixo HID) quando o GPIO
> está flutuante, evitando qualquer ruído residual.
>
> | Cenário | Solução | Resistores |
> |---------|---------|:----------:|
> | **Sem halls** (sem embreagem) | Divisor compartilhado ✅ | **2** |
> | **Com 1 hall** (só 1 embreagem) | Divisor independente **só no GPIO sem sensor** | **2** |
> | **Com 2 halls** (ambas embreagens) | **Nenhum divisor necessário** | **0** |
>
> **⚠️ NUNCA compartilhe o divisor se algum hall estiver conectado!**
> Os dois GPIOs ficariam no mesmo nó elétrico — a saída de um sensor vaza para o
> outro GPIO (cross-talk), e com dois sensores as saídas ficam curto-circuitadas.
>
> **Circuito A — Sem halls (divisor compartilhado, 2 resistores):**
> ```
> 3.3V ── [100kΩ] ──┬── GPIO 1
>                    ├── GPIO 2
>  GND ── [100kΩ] ──┘
> ```
>
> **Circuito B — Com 1 hall (divisor independente no GPIO livre, 2 resistores):**
> ```
>                    ┌── GPIO 1 ←── Hall A OUT
>                    │                (sensor conectado, não precisa de divisor)
>                    │
> 3.3V ── [100kΩ] ──┤
>                    ├── GPIO 2      (sem sensor, divisor estabiliza)
>  GND ── [100kΩ] ──┘
> ```
> Neste caso os 2 resistores formam um divisor **isolado** só para o GPIO 2.
> O GPIO 1 recebe sinal direto do sensor — sem ligação com o divisor.
>
> **No volante F1 com os 2 halls soldados, nenhum divisor é necessário.**
| **4** | **MAX7219 CS** | SPI Software (novo) | **Sim** |
| **8** | I2C SDA | MCP23017 (0x20) | **Sim** |
| **9** | I2C SCL | MCP23017 (0x20) | **Sim** |
| **10** | WS2812 DIN | LEDs endereçáveis | **Sim** |
| **11** | ~~UART RX (WT32)~~ | **Livre** (sem WT32) | — |
| **12** | **MAX7219 DIN** | SPI Software (novo) | **Sim** |
| **13** | **MAX7219 CLK** | SPI Software (novo) | **Sim** |
| **14** | ENC1-A (MFC CLK) | Encoder direto | **Sim** |
| **15** | ENC1-B (MFC DT) | Encoder direto | **Sim** |
| **16** | ENC2-A (BB CLK) | Encoder direto | Opcional |
| **17** | ENC2-B (BB DT) | Encoder direto | Opcional |
| **18** | ENC3-A (MAP CLK) | Encoder direto | Opcional |
| **19** | USB D− | USB Nativa (HID + CDC) | **Sim** (automático) |
| **20** | USB D+ | USB Nativa (HID + CDC) | **Sim** (automático) |
| **21** | ENC3-B (MAP DT) | Encoder direto | Opcional |
| **38** | ENC4-A (TC CLK) | Encoder direto | Opcional |
| **39** | ENC4-B (TC DT) | Encoder direto | Opcional |
| **40** | ENC5-A (ABS CLK) | Encoder direto | Opcional |
| **41** | ENC5-B (ABS DT) | Encoder direto | Opcional |
| **42** | ENC6-A (Lat.1 CLK) | Encoder direto | Opcional |
| **43** | UART TX / Debug | CH340 (monitor serial) | Debug |
| **44** | UART RX / Debug | CH340 (upload/serial) | Debug |
| **46** ⚠️ | ENC9-B (Lateral 4) | Encoder direto | Opcional |
| **47** | ENC6-B (Lat.1 DT) | Encoder direto | Opcional |
| **48** | ENC7-A (Lat.2 CLK) | Encoder direto | Opcional |

### GPIOs reservados / não usar

| GPIO | Motivo |
|------|--------|
| 0 | Strapping / BOOT — **nunca usar** |
| 19, 20 | USB nativa — gerenciado automaticamente |
| 22-32 | Reservados internamente / Flash SPI |
| 33-37 | PSRAM Octal (N8R8) — **não acessíveis** |
| 45 | Strapping (VDD_SPI) — não conectar pull-up |

### GPIOs completamente livres (sem uso neste volante)

| GPIO | Nota |
|------|------|
| 5 | ADC1_CH4 / Touch — disponível para expansão |
| 6 | ADC1_CH5 / Touch — disponível para expansão |
| 7 | ADC1_CH6 / Touch — disponível para expansão |
| 11 | Livre (era UART RX do WT32 no F1) |

---

## 5) Diagrama de Blocos

```
                         ┌──────────────────────────────────────┐
                         │       ESP32-S3-WROOM1 N8R8            │
                         │       main_wheel_round.cpp            │
                         │                                      │
  USB-C Nativa ◄────────►│  USB HID (Gamepad + Consumer)        │
  (GPIO 19/20)           │  USB CDC (SimHub telemetria)         │
                         │                                      │
  USB-C Debug  ◄────────►│  UART0 (GPIO 43/44 via CH340)       │
  (CH340/CP2102)         │  → Monitor serial / Debug            │
                         │                                      │
                         │  GPIO 8 (SDA) ──► MCP23017 (0x20)   │
                         │  GPIO 9 (SCL) ──┘   ↕               │
                         │                  Matriz 8x8          │
                         │                  (até 64 slots)      │
                         │                                      │
                         │  GPIO 12 (DIN) ──► MAX7219           │
                         │  GPIO 13 (CLK) ──┘  8 dígitos        │
                         │  GPIO  4 (CS)  ──┘  7 segmentos      │
                         │                                      │
                         │  GPIO 10 (DIN) ──► WS2812B           │
                         │                    16-26 LEDs        │
                         │                    (RPM bar + flags) │
                         │                                      │
                         │  GPIO 14,15 ──► ENC1 (MFC)           │
                         │  GPIO 16,17 ──► ENC2 (BB)  opcional  │
                         │  GPIO 18,21 ──► ENC3 (MAP) opcional  │
                         │  GPIO 38,39 ──► ENC4 (TC)  opcional  │
                         │  GPIO 40,41 ──► ENC5 (ABS) opcional  │
                         │                                      │
                         │  GPIO 1 ──► Hall A (Clutch L) opc.   │
                         │  GPIO 2 ──► Hall B (Clutch R) opc.   │
                         └──────────────────────────────────────┘
```

---

## 6) Mapa de Slots da Matriz (Retrocompatível)

**A matriz é idêntica ao volante F1.** Mesmos slots = mesmos botões HID = mesmo mapeamento no jogo.

Fórmula: `Slot = (ROW × 8) + COL + 1`

Use **somente os slots que você vai instalar fisicamente**. Os slots não conectados simplesmente não disparam — não atrapalham nada.

### Layout sugerido para volante redondo

| Slot | Função | ROW (GPB) | COL (GPA) | Obrigatório? | Nota |
|:----:|--------|:---------:|:---------:|:------------:|------|
| **1** | MFC SW (push encoder) | GPB0 (1) | GPA0 (21) | **Sim** | Menu principal |
| 2 | ENC2 SW (BB push) | GPB0 (1) | GPA1 (22) | Opcional | Se instalar ENC2 |
| 3 | ENC3 SW (MAP push) | GPB0 (1) | GPA2 (23) | Opcional | Se instalar ENC3 |
| 4 | ENC4 SW (TC push) | GPB0 (1) | GPA3 (24) | Opcional | Se instalar ENC4 |
| 5 | ENC5 SW (ABS push) | GPB0 (1) | GPA4 (25) | Opcional | Se instalar ENC5 |
| **13** | RADIO / Mute | GPB1 (2) | GPA4 (25) | Recomendado | Consumer Control |
| **14** | FLASH / Play-Pause | GPB1 (2) | GPA5 (26) | Recomendado | Consumer Control |
| 25 | 5-way UP | GPB3 (4) | GPA0 (21) | Opcional | HAT N |
| 26 | 5-way DOWN | GPB3 (4) | GPA1 (22) | Opcional | HAT S |
| 27 | 5-way LEFT | GPB3 (4) | GPA2 (23) | Opcional | HAT W |
| 28 | 5-way RIGHT | GPB3 (4) | GPA3 (24) | Opcional | HAT E |
| 29 | 5-way CENTER | GPB3 (4) | GPA4 (25) | Opcional | Button 29 |
| **30** | SHIFT | GPB3 (4) | GPA5 (26) | **Recomendado** | Modificador |
| **33** | Borboleta UP (+) | GPB4 (5) | GPA0 (21) | Recomendado | Marcha + |
| **34** | Borboleta DOWN (−) | GPB4 (5) | GPA1 (22) | Recomendado | Marcha − |
| 9-12 | Botões extras (col. esq) | GPB1 (2) | GPA0-3 | Opcional | Livres |
| 17-22 | Botões extras (col. dir) | GPB2 (3) | GPA0-5 | Opcional | Livres |
| 35-37 | Botões traseiros/extra | GPB4 (5) | GPA2-4 | Opcional | Livres |

### Configuração Mínima Recomendada

Para um volante redondo funcional com estilo rally/touring:

```
Controles mínimos:
  ├── 1× MFC encoder (slot 1) — OBRIGATÓRIO
  ├── 2× Borboletas UP/DOWN (slots 33/34)
  ├── 1× SHIFT (slot 30)
  ├── 1× RADIO (slot 13)
  ├── 1× FLASH (slot 14)
  └── 2-4× Botões extras nos slots que quiser

Total: 7-11 botões na matriz + 1 encoder
```

---

## 7) Tabela de Soldagem — Botão por Botão

> Mesma regra do F1: **cada botão precisa de 1 diodo 1N4148**.
> Sentido: `ROW (GPBx) ──►|── [BOTÃO] ── COL (GPAx)` (banda no lado ROW)

### Configuração Mínima

| # | Componente | ROW + Diodo | COL | Slot |
|:-:|-----------|:-----------:|:---:|:----:|
| 1 | MFC SW | 🔵 GPB0 (pin 1) | ⚪ GPA0 (pin 21) | 1 |
| 2 | RADIO (Mute) | 🟠 GPB1 (pin 2) | ⚫ GPA4 (pin 25) | 13 |
| 3 | FLASH (Play/Pause) | 🟠 GPB1 (pin 2) | 🟣 GPA5 (pin 26) | 14 |
| 4 | SHIFT | 🟢 GPB3 (pin 4) | 🟣 GPA5 (pin 26) | 30 |
| 5 | Borboleta UP | 🟤 GPB4 (pin 5) | ⚪ GPA0 (pin 21) | 33 |
| 6 | Borboleta DOWN | 🟤 GPB4 (pin 5) | 🟤 GPA1 (pin 22) | 34 |

### Botões Extras (adicione conforme quiser)

| # | Componente | ROW + Diodo | COL | Slot |
|:-:|-----------|:-----------:|:---:|:----:|
| 7 | ENC2 SW (BB) | 🔵 GPB0 (pin 1) | 🟤 GPA1 (pin 22) | 2 |
| 8 | ENC3 SW (MAP) | 🔵 GPB0 (pin 1) | 🔴 GPA2 (pin 23) | 3 |
| 9 | Extra 1 | 🟠 GPB1 (pin 2) | ⚪ GPA0 (pin 21) | 9 |
| 10 | Extra 2 | 🟠 GPB1 (pin 2) | 🟤 GPA1 (pin 22) | 10 |
| 11 | Extra 3 | 🟠 GPB1 (pin 2) | 🔴 GPA2 (pin 23) | 11 |
| 12 | Extra 4 | 🟠 GPB1 (pin 2) | 🟡 GPA3 (pin 24) | 12 |
| 13 | 5-way UP | 🟢 GPB3 (pin 4) | ⚪ GPA0 (pin 21) | 25 |
| 14 | 5-way DOWN | 🟢 GPB3 (pin 4) | 🟤 GPA1 (pin 22) | 26 |
| 15 | 5-way LEFT | 🟢 GPB3 (pin 4) | 🔴 GPA2 (pin 23) | 27 |
| 16 | 5-way RIGHT | 🟢 GPB3 (pin 4) | 🟡 GPA3 (pin 24) | 28 |
| 17 | 5-way CENTER | 🟢 GPB3 (pin 4) | ⚫ GPA4 (pin 25) | 29 |
| 18 | Traseiro 1 | 🟤 GPB4 (pin 5) | 🔴 GPA2 (pin 23) | 35 |
| 19 | Traseiro 2 | 🟤 GPB4 (pin 5) | 🟡 GPA3 (pin 24) | 36 |

> ⚠️ Os botões **NÃO têm LED** neste volante (sem PCA9685). Use botões simples de 2 terminais.

---

## 8) MAX7219 — Display 7 Segmentos

### O que é

Módulo com 8 dígitos de 7 segmentos, controlado por SPI com apenas 3 fios. Mostra: marcha, velocidade, informações do MFC, alertas do SimHub.

### Conexão

| MAX7219 Pino | ESP32 GPIO | Função |
|:---:|:---:|:---:|
| VCC | 3.3V **ou** 5V | Alimentação (5V = mais brilho) |
| GND | GND | Terra comum |
| DIN | **GPIO 12** | Dados (SPI MOSI) |
| CS / LOAD | **GPIO 4** | Chip Select |
| CLK | **GPIO 13** | Clock (SPI SCK) |

```
ESP32-S3                     MAX7219 (8 dígitos)
┌──────────┐                 ┌──────────────┐
│ GPIO 12  │────────────────►│ DIN          │
│ GPIO 13  │────────────────►│ CLK          │
│ GPIO  4  │────────────────►│ CS / LOAD    │
│ 3.3V/5V  │────────────────►│ VCC          │
│   GND    │────────────────►│ GND          │
└──────────┘                 └──────────────┘
```

> **Nota:** O MAX7219 aceita 3.3V ou 5V. Com 5V os dígitos ficam mais brilhantes e legíveis ao sol. Se usar 5V, a linha de dados (DIN) do ESP32 a 3.3V ainda é reconhecida como HIGH pelo MAX7219 (threshold ~2.4V).

### O que será exibido

| Modo | Dígitos 1-2 | Dígitos 3-8 | Exemplo |
|------|:-----------:|:-----------:|---------|
| **Normal** (corrida) | Marcha | Velocidade | `3  215` |
| **MFC navegação** | — | Item do menu | `  CLUTCH` ou `   BITE` |
| **MFC ajuste** | — | Valor | `  BT  60` ou `BR  220` |
| **Alerta SimHub** | — | Mensagem curta | `PIT  ON` ou `FUEL LO` |
| **Shift light** | — | Pisca tudo | `--------` (blink) |

### Mapeamento de caracteres especiais 7seg

```
Letras possíveis: A b C c d E F G H h I J L n O o P r S t U u Y
Letras NÃO possíveis: K M V W X Z (limitação do 7 segmentos)
```

### Biblioteca recomendada

```
LedControl (Arduino)
```
ou controle direto via SPI — apenas 3 funções: `init()`, `setDigit()`, `setChar()`.

---

## 9) WS2812 — LEDs Endereçáveis (RPM/Alertas)

### O que é

Fita de LEDs endereçáveis RGB, mesmo tipo já usado no WT32 do volante F1 (`NeoPixelBusLEDs.h`). No volante redondo, fica conectada **diretamente no ESP32** em vez de no WT32.

### Conexão

| WS2812 Pino | ESP32 GPIO | Função |
|:---:|:---:|:---:|
| VCC / 5V | **5V** do ESP32 | Alimentação (fita pode consumir >500mA) |
| GND | GND | Terra comum |
| DIN | **GPIO 10** (via 330Ω) | Dados — mesmo pino do projeto atual |

```
ESP32-S3                     WS2812B (16-26 LEDs)
┌──────────┐                 ┌──────────────────┐
│ GPIO 10  │──[330Ω]────────►│ DIN              │
│   5V     │────────────────►│ VCC              │
│   GND    │────────────────►│ GND              │
└──────────┘                 └──────────────────┘
```

### Layout sugerido (arco no topo do aro)

```
         ┌──────────────────────────────────────┐
         │  L2  L1  L0 │ R0  R1  R2  R3  R4 ...│  ← LEDs no arco
         │  FLAGS/SPOT  │     RPM BAR            │
         └──────────────────────────────────────┘
```

### Funções dos LEDs (reaproveitadas do NeoPixelBusLEDs.h)

| Zona | LEDs | Função | Prioridade |
|------|:----:|--------|:----------:|
| Esquerda (0-2) | 3 | Bandeiras / Spotter L / TC ativo / Alertas | Alta → Baixa |
| Centro (3-17) | ~15 | Barra RPM progressiva (Verde→Amarelo→Laranja→Vermelho) | — |
| Direita (18-20+) | 3+ | Spotter R / ABS ativo | — |

**Cores da barra RPM:**
- < 60% = 🟢 Verde
- 60-80% = 🟡 Amarelo
- 80-redline = 🟠 Laranja
- ≥ redline = 🔴 Vermelho (piscando = shift light)
- DRS disponível = 🔵 Azul
- DRS ativo = 🟣 Roxo

---

## 10) Recepção SimHub via USB CDC

### Como funciona hoje (WT32)

```
PC → USB CDC → Serial → FlowSerialRead.h → ARQSerial → SHCommands.h → SHCustomProtocol.h
```

O SimHub envia comandos binários (header `0x03`) seguidos de um byte de comando (`'1'`=Hello, `'P'`=CustomProtocol, `'6'`=RGBLEDs, etc.). O WT32 responde com versão, features e device name. Após handshake, o SimHub envia dados custom (72 campos separados por `;`).

### Como vai funcionar no Volante Redondo

O ESP32-S3 com `USB_MODE=0` (TinyUSB) já expõe dois endpoints simultâneos:
- **HID** → Gamepad + Consumer Control (já funciona no `main_wheel.cpp`)
- **CDC** → Porta serial virtual (é o `Serial` do Arduino, disponível via USB nativa)

No `main_wheel.cpp` atual, `Serial` (CDC) é inicializada mas **não é usada para nada** — o debug vai por `ButtonBoxSerial` (UART GPIO43/CH340). Isso significa que a CDC está **livre** para receber o SimHub.

### O que precisa ser portado

Do firmware do WT32, o volante redondo precisa de uma versão **simplificada** da stack SimHub:

| Componente WT32 | Ação no Volante Redondo |
|-----------------|------------------------|
| `FlowSerialRead.h` / `ArqSerial.h` | **Portar** — parser do protocolo SimHub (essencial) |
| `SHCommands.h` | **Portar simplificado** — Hello, Features, DeviceName, UniqueId, CustomProtocol, RGBLEDs |
| `SHCustomProtocol.h` (parsing) | **Portar parcial** — só os campos úteis para MAX7219 e WS2812 |
| `SHCustomProtocol.h` (drawing TFT) | **Não portar** — substituir por saída MAX7219 + WS2812 |
| `NeoPixelBusLEDs.h` | **Reutilizar** — lógica de cores RPM/flags/spotter |
| `GFXHelpers.h` | **Não portar** — específico do TFT |
| `TrackMaps.h` | **Não portar** — sem tela para minimapa |

### Dados úteis para o volante redondo

Do protocolo de 72 campos, estes são os que importam para MAX7219 + WS2812:

| Índice | Campo | Destino |
|:------:|-------|---------|
| 0 | Speed (km/h) | MAX7219 |
| 1 | Gear | MAX7219 |
| 2 | RPM % | WS2812 (barra RPM) |
| 3 | RPM Redline | WS2812 (threshold) |
| 25 | TC Level | MAX7219 (popup) |
| 26 | TC Active | WS2812 (indicador) |
| 27 | ABS Level | MAX7219 (popup) |
| 28 | ABS Active | WS2812 (indicador) |
| 30 | Brake Bias | MAX7219 (popup) |
| 31 | Brake % | — |
| 34 | Fuel Remaining Laps | MAX7219 (alerta LOW FUEL) |
| 40 | Flag | WS2812 (bandeiras) |
| 42 | Alert Message | MAX7219 + WS2812 |
| 45 | Spotter Left | WS2812 |
| 46 | Spotter Right | WS2812 |
| 57 | Shift Light Trigger | WS2812 (blink) |
| 58 | DRS Available | WS2812 |
| 59 | DRS Active | WS2812 |

### Protocolo CustomProtocol simplificado para o Redondo

Em vez de usar os 72 campos do dashboard TFT, recomendo criar um **segundo protocolo custom** no SimHub (`customProtocol-round.txt`) com apenas os campos necessários. Isso reduz latência e simplifica o parsing.

Sugestão (16 campos):

```
format([DataCorePlugin.GameData.NewData.SpeedKmh],'0') + ';' +      /* [0]  Speed */
isnull([DataCorePlugin.GameData.NewData.Gear],'N') + ';' +          /* [1]  Gear */
format([CarSettings_CurrentDisplayedRPMPercent],'0') + ';' +        /* [2]  RPM % */
format([CarSettings_RPMRedLineSetting],'0') + ';' +                 /* [3]  Redline */
isnull([TCLevel], 0) + ';' +                                        /* [4]  TC Level */
isnull([TCActive], 0) + ';' +                                       /* [5]  TC Active */
isnull([ABSLevel], 0) + ';' +                                       /* [6]  ABS Level */
isnull([ABSActive], 0) + ';' +                                      /* [7]  ABS Active */
abs(format([BrakeBias], '00.0')) + ';' +                            /* [8]  Brake Bias */
format([DataCorePlugin.GameData.NewData.FuelEstimatedLaps],'0.0') + ';' + /* [9] Fuel Laps */
isnull([Flag_CurrentFlag], 'None') + ';' +                          /* [10] Flag */
isnull([DataCorePlugin.GameData.NewData.AlertMessage],'') + ';' +   /* [11] Alert */
format([CarSettings_CurrentDisplayedRPMPercent],'0') + ';' +        /* [12] RPM % (LEDs) */
isnull([SpotterCarLeft], 0) + ';' +                                 /* [13] Spotter L */
isnull([SpotterCarRight], 0) + ';' +                                /* [14] Spotter R */
[ShiftLightTrigger] + ';'                                           /* [15] Shift Light */
```

---

## 11) Adaptações no Firmware

### Arquivo novo: `src/main_wheel_round.cpp`

Este arquivo **não substitui** o `main_wheel.cpp`. É um arquivo separado, selecionado via `build_src_filter` no `platformio.ini`.

### Código reutilizado do main_wheel.cpp (copiar/adaptar)

| Seção | Linhas aprox. | Status |
|-------|:------------:|--------|
| HID Descriptor + CustomGamepad | 1-100 | Copiar integralmente |
| Variáveis globais (axes, buttons, hat) | 101-130 | Copiar integralmente |
| Pinout (encoders, halls, I2C) | 131-175 | Copiar integralmente |
| Matrix state + debounce | 177-220 | Copiar integralmente |
| Encoder virtual buttons / shift pulses | 460-520 | Copiar integralmente |
| Clutch config + modes (DUAL/MIRROR/BITE/etc.) | 522-555 | Copiar integralmente |
| MFC Menu enum + names | 556-593 | Copiar integralmente |
| GamepadReport struct + sendGamepad() | 598-635 | Copiar integralmente |
| Halls + clutch logic (updateClutches) | 1180-1320 | Copiar integralmente |
| Matrix scan (setupButtonMatrix + scanButtonMatrix) | 822-915 | Copiar integralmente |
| Encoders (setup + scan + handleMfc) | 917-1140 | Copiar integralmente |
| MFC press handling | 1345-1540 | Copiar integralmente |
| Consumer Control (MUTE/PLAY) | 1660-1707 | Copiar integralmente |

### Código removido (não copiar)

| Seção | Motivo |
|-------|--------|
| PCA9685 setup/update/sweep/breath | Sem LEDs PWM |
| uartSend() / uartSendInt() | Sem WT32 — substituir por saída local |
| handleWt32UartRx() / uartRoundtripTask() | Sem WT32 |
| UART PING/PONG | Sem WT32 |

### Código novo a implementar

| Funcionalidade | Descrição | Complexidade |
|---------------|-----------|:------------:|
| **MAX7219 driver** | Init, setDigit, setChar, clear, setBrightness | Baixa |
| **MAX7219 display manager** | Decide o que mostrar: gear+speed vs popup MFC vs alerta SimHub | Média |
| **SimHub CDC receiver** | FlowSerial simplificado + parser dos 16 campos | Média |
| **SimHub handshake** | Hello/Features/DeviceName/Acq/CustomProtocol | Média |
| **WS2812 local driver** | NeoPixelBus no GPIO 10, mesma lógica do WT32 | Baixa (reuso) |
| **WS2812 telemetry update** | RPM bar, flags, spotter, DRS, shift light, alerts | Média (reuso) |
| **Substituição do uartSend** | `uartSend("MFC","NAV","CLUTCH")` → `max7219Show("CLUTCH")` | Baixa |

### Fluxo do loop() no volante redondo

```cpp
void loop() {
    // 1. Encoders (GPIO-only, mais alta prioridade de polling)
    scanEncoders();

    // 2. Matriz de botões (I2C, throttled a cada 3ms)
    scanButtonMatrix();

    // 3. SimHub CDC (parsing quando disponível)
    processSimHubCDC();

    // 4. Atualizar MAX7219 (throttled a cada ~50ms)
    updateMax7219Display();

    // 5. Atualizar WS2812 (throttled a cada ~16ms)
    updateLocalLEDs();

    // 6. MFC press handling
    handleMfcPress();

    // 7. Multimedia (MUTE/PLAY quando VOL_SYS ativo)
    handleMultimediaButtons();

    // 8. Virtual button release
    releaseVirtualButtonPulses();

    // 9. Clutch/Hall update
    updateClutches();

    // 10. SHIFT+clutch combos
    handleShiftClutchCombo();
}
```

---

## 12) Novo Env no platformio.ini

Adicionar esta seção ao `platformio.ini` **sem alterar nenhum env existente**:

```ini
[env:wroom1-n8r8-wheel-round]
platform = espressif32@^6.9
board = esp32-s3-devkitc-1
framework = arduino
board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L
board_build.flash_mode = qio
board_upload.flash_size = 16MB
board_build.variants_dir = variants
board_build.variant = wroom1_wheel
; USB_MODE=0: TinyUSB OTG (HID Gamepad + CDC SimHub no mesmo cabo)
build_flags =
    -w
    -DESP32=true
    -DARDUINO_USB_MODE=0
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DCFG_TUD_HID=1
    -DCFG_TUD_CDC=1
    -DUSB_PRODUCT=\"ESP-ButtonBox-WHEEL\"
    -DUSB_MANUFACTURER=\"SimRacing_DIY\"
    -DUSB_SERIAL=\"\"
    -DWHEEL_ROUND=1

lib_deps =
    adafruit/Adafruit MCP23017 Arduino Library @ ^2.3.2
    makuna/NeoPixelBus @ ^2.8
    Wire

monitor_speed = 115200
upload_speed = 921600
build_src_filter = +<main_wheel_round.cpp>
extra_scripts = pre:scripts/patch_hid_name.py
```

> **IMPORTANTE:** Mesmo VID/PID e USB_PRODUCT que o F1 → intercâmbio transparente.
> A flag `-DWHEEL_ROUND=1` permite `#ifdef` para código condicional se quiser.

---

## 13) Plano de Implementação por Fases

### Fase 1 — Hardware Básico (sem SimHub, sem MAX7219)

**Objetivo:** Validar que a ESP32 funciona como HID com a matriz e pelo menos o MFC.

1. Soldar MCP23017 (I2C em GPIO 8/9)
2. Soldar ENC1 MFC (GPIO 14/15, SW no slot 1)
3. Soldar 2-6 botões extras com diodos
4. Fazer upload do `main_wheel.cpp` **original** (env `wroom1-n8r8-wheel`)
5. Testar no Windows → Game Controllers → botões e encoder devem funcionar
6. Menu MFC deve navegar (mesmo sem tela, o debug UART mostra os logs)

> ✅ Nesta fase, o volante redondo já funciona como gamepad HID idêntico ao F1.
> O PCA9685 não está conectado — o firmware loga `[WARN] PCA9685 not found` e prossegue normalmente.
> A UART envia mensagens para o vazio — sem crash.

### Fase 2 — WS2812 (LEDs locais standalone)

**Objetivo:** LEDs RPM funcionando com efeito de boot (sem SimHub ainda).

1. Soldar fita WS2812 no GPIO 10 (com 330Ω)
2. No `main_wheel_round.cpp`, inicializar NeoPixelBus
3. Implementar efeito de boot: sweep de cores
4. Implementar breathing idle (semelhante ao PCA9685 do F1)

### Fase 3 — MAX7219 (display standalone)

**Objetivo:** Display mostrando informações do MFC sem SimHub.

1. Conectar MAX7219 (GPIO 4/12/13)
2. Implementar driver MAX7219 (init, digits, chars)
3. Substituir chamadas `uartSend()` por exibição local:
   - MFC navegação → mostrar nome do item
   - MFC ajuste → mostrar valor
   - Clutch mode → mostrar modo
   - Calibração → mostrar status

### Fase 4 — SimHub via USB CDC

**Objetivo:** Receber telemetria do SimHub pelo mesmo cabo USB.

1. Portar `FlowSerialRead.h` + `ArqSerial.h` para ler da `Serial` (CDC nativa)
2. Implementar handshake SimHub (Hello/Features/DeviceName/Acq)
3. Implementar parser CustomProtocol (16 campos simplificados)
4. Criar `customProtocol-round.txt` no SimHub
5. Configurar SimHub para enviar dados ao dispositivo "ESP-ButtonBox-WHEEL"

### Fase 5 — Integração SimHub + Display + LEDs

**Objetivo:** Tudo funcionando junto.

1. Conectar dados do parser → MAX7219 (gear, speed, alertas)
2. Conectar dados do parser → WS2812 (RPM, flags, spotter, DRS)
3. Priorização: alerta SimHub > popup MFC > gear+speed no MAX7219
4. Testar com Assetto Corsa / iRacing / AMS2

### Fase 6 — Polimento

1. Adicionar encoders extras (BB, MAP, TC, ABS) conforme desejado
2. Adicionar halls / embreagens se quiser
3. Adicionar 5-way se quiser
4. Ajustar brilho MAX7219 via menu MFC (MFC_BRIGHT)
5. Ajustar WS2812 luminance via menu MFC

---

## 14) Preservação do Volante F1 + WT32

### O que NÃO muda

| Arquivo | Status |
|---------|--------|
| `src/main_wheel.cpp` | ❌ **NÃO ALTERAR** |
| `src/main.cpp` | ❌ **NÃO ALTERAR** |
| `src/SHCustomProtocol.h` | ❌ **NÃO ALTERAR** |
| `src/NeoPixelBusLEDs.h` | ❌ **NÃO ALTERAR** |
| `platformio.ini` (envs existentes) | ❌ **NÃO ALTERAR** |

### O que é ADICIONADO

| Arquivo | Propósito |
|---------|-----------|
| `src/main_wheel_round.cpp` | Novo entrypoint do volante redondo |
| `customProtocol-round.txt` | Protocolo SimHub simplificado (16 campos) |
| `platformio.ini` → `[env:wroom1-n8r8-wheel-round]` | Novo env (adicionado no final) |
| `docs/VOLANTE_REDONDO.md` | Esta documentação |

### Compilar cada volante

```bash
# Volante F1 (wheel ESP32):
pio run -e wroom1-n8r8-wheel

# Volante F1 (tela WT32):
pio run -e wt32-sc01-plus

# Volante Redondo:
pio run -e wroom1-n8r8-wheel-round
```

---

## 15) Intercâmbio Rápido de Volantes

### Pré-requisitos

1. Ambos os volantes com firmware que usa o mesmo **VID/PID/USB_PRODUCT**
2. Botões importantes nos **mesmos slots da matriz**
3. **Apenas um volante conectado por vez**

### Procedimento de troca

```
1. Acabou a corrida no Assetto Corsa
2. Desconecte o cabo USB do volante F1
3. Troque o aro (mecanismo de quick release no chassi do volante)
4. Conecte o cabo USB no volante redondo (mesmo cabo, mesma porta)
5. Windows reconhece como "ESP-ButtonBox-WHEEL" (mesmo nome)
6. Abra o novo jogo / entre na pista
7. Todos os botões/eixos mantêm o mesmo mapeamento
```

### Tabela de mapeamento universal (referência)

| Função | Slot Matriz | Botão HID | Presente no F1? | Presente no Redondo? |
|--------|:-----------:|:---------:|:----------------:|:--------------------:|
| MFC push | 1 | Button 1 | ✅ | ✅ |
| BB push | 2 | Button 2 | ✅ | Opcional |
| MAP push | 3 | Button 3 | ✅ | Opcional |
| TC push | 4 | Button 4 | ✅ | Opcional |
| ABS push | 5 | Button 5 | ✅ | Opcional |
| RADIO (Mute) | 13 | Button 13 | ✅ | ✅ |
| FLASH (Play) | 14 | Button 14 | ✅ | ✅ |
| 5-way UP | 25 | HAT N | ✅ | Opcional |
| 5-way DOWN | 26 | HAT S | ✅ | Opcional |
| 5-way LEFT | 27 | HAT W | ✅ | Opcional |
| 5-way RIGHT | 28 | HAT E | ✅ | Opcional |
| 5-way CENTER | 29 | Button 29 | ✅ | Opcional |
| SHIFT | 30 | Interno | ✅ | ✅ |
| Borboleta UP | 33 | Button 33 | ✅ | ✅ |
| Borboleta DOWN | 34 | Button 34 | ✅ | ✅ |
| Clutch L | — | Eixo Z | ✅ | Opcional |
| Clutch R | — | Eixo Rz | ✅ | Opcional |
| MFC encoder | ENC1 | Controle MFC | ✅ | ✅ |
| BB encoder | ENC2 | Eixo X | ✅ | Opcional |
| MAP encoder | ENC3 | Eixo Y | ✅ | Opcional |

### E se os dois estiverem conectados acidentalmente?

- Windows verá dois "ESP-ButtonBox-WHEEL"
- **Não queima, não corrompe, não remapeia**
- O jogo pode capturar input do volante errado
- **Solução:** desconectar um deles antes de entrar na pista

---

## 16) Checklist de Montagem

### Fase 1: Alimentação e I2C (MCP23017)
- [ ] GND comum entre todos os módulos
- [ ] ESP32 3.3V → MCP23017 VDD (pin 9)
- [ ] MCP23017 VSS (pin 10) → GND
- [ ] MCP23017 RESET (pin 18) → 3.3V
- [ ] MCP23017 A0/A1/A2 (pin 15/16/17) → GND (endereço 0x20)
- [ ] ESP32 GPIO 8 → MCP23017 SDA (pin 13)
- [ ] ESP32 GPIO 9 → MCP23017 SCL (pin 12)
- [ ] Upload firmware → verificar `[I2C] MCP23017 found at 0x20` no Serial Monitor
- [ ] Aceitar `[WARN] PCA9685 not found` — **NORMAL no volante redondo**

### Fase 2: Encoder MFC
- [ ] ENC1: A→GPIO14, B→GPIO15, C→GND
- [ ] ENC1 SW: diodo + GPB0(pin 1) / GPA0(pin 21) → Slot 1
- [ ] Girar encoder → debug mostra `[MFC ENC1] ROT dir=CW`

### Fase 3: Botões da Matriz
- [ ] Soldar diodo 1N4148 em cada botão (banda no lado ROW)
- [ ] Testar cada botão → verificar no Game Controllers (joy.cpl)
- [ ] SHIFT (slot 30) → verificar `[SHIFT] DOWN` no debug

### Fase 4: MAX7219
- [ ] VCC→3.3V ou 5V, GND→GND
- [ ] DIN→GPIO12, CLK→GPIO13, CS→GPIO4
- [ ] Firmware mostra marcha e velocidade

### Fase 5: WS2812
- [ ] DIN→GPIO10 (via 330Ω), VCC→5V, GND→GND
- [ ] Boot sweep executa ao ligar
- [ ] Barra RPM funciona com SimHub conectado

### Fase 6: SimHub CDC
- [ ] Configurar SimHub para o dispositivo "ESP-ButtonBox-WHEEL"
- [ ] Colar `customProtocol-round.txt` no SimHub Custom Protocol
- [ ] MAX7219 mostra gear + speed da corrida
- [ ] WS2812 mostra RPM + bandeiras

### Fase 7: Extras (opcional)
- [ ] Encoders BB/MAP/TC/ABS adicionados
- [ ] Halls / embreagens funcionando
- [ ] 5-way joystick conectado
- [ ] Borboletas UP/DOWN
- [ ] Troca de volante F1↔Redondo sem remapear

---

## 17) Troubleshooting

| Problema | Causa | Solução |
|----------|-------|---------|
| `[WARN] PCA9685 not found` no boot | Normal — volante redondo não usa PCA9685 | Ignorar ✅ |
| `[UART] PING timeout` repetido | Normal — sem WT32 para responder | Ignorar (será removido no round) |
| MAX7219 não acende | VCC/GND faltando, DIN/CLK/CS invertidos | Checar fiação, testar com sketch exemplo |
| MAX7219 dígitos errados | CS errado ou biblioteca diferente | Verificar GPIO 4 = CS |
| WS2812 não acende | Resistor 330Ω faltando, DIN invertido | Checar fiação, alimentação 5V |
| WS2812 cores erradas | Color order diferente (RGB vs GRB) | Mudar `NeoGrbFeature` para `NeoRgbFeature` |
| SimHub não encontra o dispositivo | CDC não está habilitada ou conflito com HID | Verificar `USB_MODE=0` e `CDC_ON_BOOT=1` |
| SimHub mostra como Serial desconhecida | VID/PID não reconhecido | Usar VID=0x303A PID=0x8172 (já configurado) |
| Botão não aparece no Game Controllers | Diodo invertido ou slot errado na matriz | Verificar banda do diodo + ROW/COL |
| Dois botões ativam juntos | Diodo faltando (ghost key) | Adicionar diodo 1N4148 |
| Encoder gira para um lado só | A/B invertido | Trocar fios A↔B |
| Troca de volante não mantém mapeamento | VID/PID ou nome diferente | Garantir mesmo build_flags e patch_hid_name |
| Windows confunde dois volantes | Ambos conectados simultaneamente | Desconectar um antes de jogar |
| Eixo Z/Rz tremendo sem hall | GPIO 1/2 flutuante lê ruído ADC | Adicionar divisor 2×100kΩ **independente por GPIO** (ver seção 4, nota GPIO 1/2). Sem halls: pode compartilhar |
| Ambos eixos leem o mesmo valor | Divisor compartilhado com hall conectado — cross-talk | **Separar os divisores!** Cada GPIO precisa de seu próprio par de resistores, ou remover o divisor do GPIO com sensor |
| Eixo Z/Rz preso no meio com hall | Divisor de tensão no GPIO com sensor | Remover o divisor desse GPIO — sensor já fornece tensão estável |

---

## Resumo Final — Pedido de Compra Rápido

### Para começar (Fase 1-3): ~R$ 100

```
1× ESP32-S3-WROOM1-N8R8 DevKitC-1
1× MCP23017 DIP-28
1× Módulo MAX7219 8 dígitos 7seg
1× Fita WS2812B 16-26 LEDs
1× Encoder EC11
6-10× Botões push momentâneo
10× Diodo 1N4148
1× Resistor 330Ω
Fios + solda
1× Cabo USB-C (dados)
```

### Para completar (Fase 4+): +R$ 50-100

```
2-4× Encoder EC11 extra
2× Sensor Hall SS49E
2× Micro switch (borboletas)
1× 5-way joystick
1× Botão SHIFT
Mais diodos/botões conforme layout
```

---

*Documento criado para o projeto ESP-ButtonBox-WHEEL — Variante Volante Redondo.*
*Compatível com `main_wheel.cpp` rev. atual. Não altera nenhum arquivo existente.*
