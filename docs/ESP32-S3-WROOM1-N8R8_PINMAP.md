# ESP32-S3-WROOM1-N8R8

The ESP32-S3-WROOM1-N8R8 is a general-purpose Wi-Fi + Bluetooth 5 (LE) module based on the ESP32-S3 SoC, featuring **8 MB Flash** and **8 MB Octal PSRAM**. It exposes up to 45 programmable GPIOs and supports a wide range of peripherals: SPI, I2C, I2S, UART, PWM, ADC, DAC, USB OTG, and capacitive touch. Ideal for demanding IoT, HID, and display projects.

- **Datasheet:** [ESP32-S3-WROOM-1 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- **Technical Reference:** [ESP32-S3 TRM](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

---

## Module Variants

| Suffix | Flash | PSRAM | Antenna |
|--------|-------|-------|---------|
| N8     | 8 MB  | —     | PCB     |
| N8R2   | 8 MB  | 2 MB  | PCB     |
| **N8R8** | **8 MB** | **8 MB Octal** | **PCB** |
| N16R8  | 16 MB | 8 MB Octal | PCB |

> **⚠️ N8R8 — Octal PSRAM:** GPIO 33–37 are internally connected to the Octal PSRAM and are **not available** for external use.

---

## Pinout Overview

```
                    ┌─────────────────────┐
             3.3V ──┤ 1               22 ├── GND
             3.3V ──┤ 2               21 ├── TX  (GPIO 43 / UART0_TX)
              RST ──┤ 3               20 ├── RX  (GPIO 44 / UART0_RX)
           GPIO 4 ──┤ 4               19 ├── GPIO 1
           GPIO 5 ──┤ 5               18 ├── GPIO 2
           GPIO 6 ──┤ 6               17 ├── GPIO 42
           GPIO 7 ──┤ 7               16 ├── GPIO 41
          GPIO 15 ──┤ 8               15 ├── GPIO 40
          GPIO 16 ──┤ 9               14 ├── GPIO 39
          GPIO 17 ──┤ 10              13 ├── GPIO 38
          GPIO 18 ──┤ 11              12 ├── GPIO 37 ⚠️
           GPIO 8 ──┤ 12              11 ├── GPIO 36 ⚠️
           GPIO 3 ──┤ 13              10 ├── GPIO 35 ⚠️
          GPIO 46 ──┤ 14               9 ├── GPIO 0  (BOOT)
           GPIO 9 ──┤ 15               8 ├── GPIO 45 (strapping)
          GPIO 10 ──┤ 16               7 ├── GPIO 48
          GPIO 11 ──┤ 17               6 ├── GPIO 47
          GPIO 12 ──┤ 18               5 ├── GPIO 21
          GPIO 13 ──┤ 19               4 ├── GPIO 20
          GPIO 14 ──┤ 20               3 ├── GPIO 19
              5V  ──┤ 21               2 ├── GND
              GND ──┤ 22               1 ├── GND
                    └─────────────────────┘
                         ESP32-S3-WROOM1
```

> ⚠️ GPIO 35–37 = Octal PSRAM (N8R8). **Não usar externamente.**
> ⚠️ GPIO 0, 3, 45, 46 = Strapping pins. Cuidado com pull-up/pull-down no boot.

---

## Interface Description

### 1. Power Interface

| Pin | Description | Voltage Range | Remark |
|-----|-------------|---------------|--------|
| 3.3V (×2) | Power supply output | 3.3V | Para alimentar periféricos lógicos |
| 5V | Power supply input | 5V ±5% | Entrada via USB ou fonte externa |
| GND (×3) | Ground | 0V | Terra comum |
| EN (RST) | Chip enable / Reset | 0–3.3V | Pull-up interno; LOW = reset |

---

### 2. Left Side Pins

| Pin # | GPIO | Voltage | ADC | Touch | Remark |
|-------|------|---------|-----|-------|--------|
| 4  | GPIO 4  | 0–3.3V | ADC1_CH3 | TOUCH4 | I2C SDA (padrão: GPIO 8) |
| 5  | GPIO 5  | 0–3.3V | ADC1_CH4 | TOUCH5 | I2C SCL (padrão: GPIO 9) |
| 6  | GPIO 6  | 0–3.3V | ADC1_CH5 | TOUCH6 | |
| 7  | GPIO 7  | 0–3.3V | ADC1_CH6 | TOUCH7 | |
| 15 | GPIO 15 | 0–3.3V | ADC2_CH4 | TOUCH15 | |
| 16 | GPIO 16 | 0–3.3V | ADC2_CH5 | — | |
| 17 | GPIO 17 | 0–3.3V | ADC2_CH6 | — | |
| 18 | GPIO 18 | 0–3.3V | ADC2_CH7 | — | |
| 8  | GPIO 8  | 0–3.3V | ADC1_CH7 | TOUCH8 | I2C SDA (uso neste projeto) |
| 3  | GPIO 3  | 0–3.3V | ADC1_CH2 | TOUCH3 | ⚠️ Strapping pin (JTAG) — pull-down no boot |
| 46 | GPIO 46 | 0–3.3V | — | — | ⚠️ Strapping pin — input only no boot |
| 9  | GPIO 9  | 0–3.3V | ADC1_CH8 | TOUCH9 | I2C SCL (uso neste projeto) |
| 10 | GPIO 10 | 0–3.3V | ADC1_CH9 | TOUCH10 | NeoPixel / EXT_IO1 |
| 11 | GPIO 11 | 0–3.3V | ADC2_CH0 | TOUCH11 | UART1 RX (Wheel←WT32 PONG) |
| 12 | GPIO 12 | 0–3.3V | ADC2_CH1 | TOUCH12 | |
| 13 | GPIO 13 | 0–3.3V | ADC2_CH2 | TOUCH13 | |
| 14 | GPIO 14 | 0–3.3V | ADC2_CH3 | TOUCH14 | ENC1-A (MFC CLK) |

---

### 3. Right Side Pins

| Pin # | GPIO | Voltage | ADC | Touch | Remark |
|-------|------|---------|-----|-------|--------|
| 19 | GPIO 1  | 0–3.3V | ADC1_CH0 | TOUCH1 | Hall A → Clutch L (eixo Z) |
| 18 | GPIO 2  | 0–3.3V | ADC1_CH1 | TOUCH2 | Hall B → Clutch R (eixo Rz) |
| 17 | GPIO 42 | 0–3.3V | — | — | ENC6-A (Lateral 1 CLK) |
| 16 | GPIO 41 | 0–3.3V | — | — | SD_CS / ENC5-B |
| 15 | GPIO 40 | 0–3.3V | — | — | SD_MOSI / ENC5-A |
| 14 | GPIO 39 | 0–3.3V | — | — | SD_CLK / ENC4-B |
| 13 | GPIO 38 | 0–3.3V | — | — | SD_MISO / ENC4-A |
| 12 | GPIO 37 | 0–3.3V | — | — | ⚠️ PSRAM (N8R8) — não usar |
| 11 | GPIO 36 | 0–3.3V | — | — | ⚠️ PSRAM (N8R8) — não usar |
| 10 | GPIO 35 | 0–3.3V | — | — | ⚠️ PSRAM (N8R8) — não usar |
| 9  | GPIO 0  | 0–3.3V | — | — | ⚠️ Strapping / BOOT button — pull-up no boot |
| 8  | GPIO 45 | 0–3.3V | — | — | ⚠️ Strapping (VDD_SPI) — pull-down no boot |
| 7  | GPIO 48 | 0–3.3V | — | — | LCD TE / ENC7-B |
| 6  | GPIO 47 | 0–3.3V | — | — | LCD WR / ENC6-B |
| 5  | GPIO 21 | 0–3.3V | — | — | ENC3-B (MAP DT) |
| 4  | GPIO 20 | 0–3.3V | — | — | USB D+ (native USB OTG) |
| 3  | GPIO 19 | 0–3.3V | — | — | USB D− (native USB OTG) |

---

### 4. UART Interface

| Signal | GPIO | Voltage | Remark |
|--------|------|---------|--------|
| UART0_TX | GPIO 43 | 3.3V TTL | Serial/USB via CP2102 ou USB nativo |
| UART0_RX | GPIO 44 | 3.3V TTL | Serial/USB via CP2102 ou USB nativo |
| UART1_TX | GPIO 43 | 3.3V TTL | Redireciona para WT32 (dados dashboard) |
| UART1_RX | GPIO 11 | 3.3V TTL | Recebe PONG do WT32-SC01 Plus |

> Neste projeto o UART0 (GPIO 43) é reutilizado como **UART1_TX** para envio de dados ao WT32. O retorno vem pelo GPIO 11.

---

### 5. I2C Interface (MCP23017 + PCA9685)

| Signal | GPIO | Endereço | Periférico |
|--------|------|----------|------------|
| SDA | GPIO 8 | 0x20 | MCP23017 (matriz de botões) |
| SDA | GPIO 8 | 0x40 | PCA9685 (driver LED PWM) |
| SCL | GPIO 9 | 0x20 | MCP23017 |
| SCL | GPIO 9 | 0x40 | PCA9685 |

```
GPIO 8 (SDA) ──┬── MCP23017 SDA (0x20)
                └── PCA9685  SDA (0x40)

GPIO 9 (SCL) ──┬── MCP23017 SCL (0x20)
                └── PCA9685  SCL (0x40)
```

---

### 6. USB OTG (Native USB)

| Signal | GPIO | Remark |
|--------|------|--------|
| D− | GPIO 19 | USB Full Speed / HID |
| D+ | GPIO 20 | USB Full Speed / HID |

> O ESP32-S3 tem USB nativo no chip. Neste projeto o dispositivo aparece como **Gamepad + Consumer Control** (HID).
> Não é necessário chip CP2102/CH340 para HID — apenas para upload via Serial.

---

### 7. Encoder Inputs (Direto no ESP32)

| Encoder | Função | Pino A (CLK) | Pino B (DT) | SW → Matriz MCP23017 |
|---------|--------|-----------|----------|---------------------|
| ENC1 | MFC (Menu) | GPIO 14 | GPIO 15 | Slot 1 |
| ENC2 | BB (Brake Bias) | GPIO 16 | GPIO 17 | Slot 2 |
| ENC3 | MAP (Engine Map) | GPIO 18 | GPIO 21 | Slot 3 |
| ENC4 | TC (Traction Ctrl) | GPIO 38 | GPIO 39 | Slot 4 |
| ENC5 | ABS | GPIO 40 | GPIO 41 | Slot 5 |
| ENC6 | Lateral 1 | GPIO 42 | GPIO 47 | — |
| ENC7 | Lateral 2 | GPIO 48 | GPIO 35 ⚠️ | — |
| ENC8 | Lateral 3 | GPIO 36 ⚠️ | GPIO 37 ⚠️ | — |
| ENC9 | Lateral 4 | GPIO 3 ⚠️ | GPIO 46 ⚠️ | — |

> ⚠️ GPIO 35–37 = PSRAM no N8R8. ENC7/8 requerem versão sem PSRAM (N8) ou remapeamento.
> ⚠️ GPIO 3 e 46 = strapping pins — testar antes de soldar.

---

### 8. Analog Inputs (Hall Sensors / Embreagens)

| Sensor | GPIO | Canal ADC | Eixo HID | Remark |
|--------|------|-----------|----------|--------|
| Hall A (Clutch L) | GPIO 1 | ADC1_CH0 | Z  | Alimentação 3.3V obrigatória |
| Hall B (Clutch R) | GPIO 2 | ADC1_CH1 | Rz | Alimentação 3.3V obrigatória |

---

### 9. Strapping Pins — Resumo

| GPIO | Strapping | Estado no Boot | Uso Seguro |
|------|-----------|----------------|------------|
| GPIO 0  | BOOT mode | Pull-up = Normal; GND = Download | Sim (após boot) |
| GPIO 3  | JTAG      | Pull-down = disable JTAG | Com cuidado |
| GPIO 45 | VDD_SPI   | Pull-down = 3.3V SPI; Pull-up = 1.8V | Não conectar pull-up |
| GPIO 46 | ROM messages | Pull-down = silencia boot log | Input-only em alguns chips |

---

### 10. GPIOs Indisponíveis no N8R8

| GPIO Range | Motivo |
|-----------|--------|
| GPIO 22–25 | Reservados (usados internamente / não exposto no módulo) |
| GPIO 26–32 | Conectados ao Flash SPI interno — **não acessíveis** |
| GPIO 33–37 | **Octal PSRAM** (N8R8) — **não usar externamente** |

---

## Mapeamento Completo — Neste Projeto (Button Box Wheel)

| GPIO | Função no Projeto | Periférico | Remark |
|------|-------------------|------------|--------|
| GPIO 1  | Hall A (Clutch L) | ADC1_CH0 | Eixo Z |
| GPIO 2  | Hall B (Clutch R) | ADC1_CH1 | Eixo Rz |
| GPIO 3  | ENC9-A ⚠️ | Encoder direto | Strapping pin |
| GPIO 4  | — | Livre | ADC / Touch disponível |
| GPIO 5  | — | Livre | ADC / Touch disponível |
| GPIO 6  | — | Livre | ADC / Touch disponível |
| GPIO 7  | — | Livre | ADC / Touch disponível |
| GPIO 8  | I2C SDA | MCP23017 + PCA9685 | Bus I2C compartilhado |
| GPIO 9  | I2C SCL | MCP23017 + PCA9685 | Bus I2C compartilhado |
| GPIO 10 | NeoPixel / EXT_IO1 | LED endereçável | 5V tolerante com resistor 330Ω |
| GPIO 11 | UART1 RX | WT32 PONG | Recebe dados do WT32-SC01 Plus |
| GPIO 12 | — | Livre | ADC2 / Touch disponível |
| GPIO 13 | — | Livre | ADC2 / Touch disponível |
| GPIO 14 | ENC1-A (MFC CLK) | Encoder direto | |
| GPIO 15 | ENC1-B (MFC DT)  | Encoder direto | |
| GPIO 16 | ENC2-A (BB CLK)  | Encoder direto | |
| GPIO 17 | ENC2-B (BB DT)   | Encoder direto | |
| GPIO 18 | ENC3-A (MAP CLK) | Encoder direto | |
| GPIO 19 | USB D−           | Native USB HID | |
| GPIO 20 | USB D+           | Native USB HID | |
| GPIO 21 | ENC3-B (MAP DT)  | Encoder direto | |
| GPIO 35 | ENC7-B ⚠️        | Encoder direto | PSRAM no N8R8 |
| GPIO 36 | ENC8-A ⚠️        | Encoder direto | PSRAM no N8R8 |
| GPIO 37 | ENC8-B ⚠️        | Encoder direto | PSRAM no N8R8 |
| GPIO 38 | ENC4-A (TC CLK)  | Encoder direto | |
| GPIO 39 | ENC4-B (TC DT)   | Encoder direto | |
| GPIO 40 | ENC5-A (ABS CLK) | Encoder direto | |
| GPIO 41 | ENC5-B (ABS DT)  | Encoder direto | |
| GPIO 42 | ENC6-A (Lat1 CLK)| Encoder direto | |
| GPIO 43 | UART0 TX / UART1 TX | WT32 dashboard | TX do ESP → RX do WT32 |
| GPIO 44 | UART0 RX         | Serial/USB upload | |
| GPIO 45 | — ⚠️             | Strapping | Não conectar pull-up |
| GPIO 46 | ENC9-B ⚠️        | Encoder direto | Strapping / input-only |
| GPIO 47 | ENC6-B (Lat1 DT) | Encoder direto | |
| GPIO 48 | ENC7-A (Lat2 CLK)| Encoder direto | |

---

## Notas de Hardware

1. **Alimentação:** Sempre use **3.3V** para periféricos lógicos. Os sensores Hall SS49E/SS495A também exigem 3.3V (não 5V).
2. **NeoPixel (GPIO 10):** Coloque um resistor 330Ω em série no sinal entre o ESP32 e o DIN da fita.
3. **I2C pull-ups:** Adicione resistores 4.7kΩ entre SDA/SCL e 3.3V se os módulos não os incluírem internamente.
4. **PSRAM (N8R8):** Se precisar dos GPIOs 35–37, use a variante **N8** (sem PSRAM).
5. **USB HID:** Para que o dispositivo apareça como Gamepad no Windows, use `USB_CLASS_HID` no `platformio.ini` com `board_build.arduino.upload.use_1200bps_touch = yes`.

---

## Recursos Adicionais

- **Espressif GitHub:** [ESP32-S3-DevKitC-1](https://github.com/espressif/esp-idf/tree/master/examples)
- **PlatformIO Board:** `esp32-s3-devkitc-1`
- **Arduino Core:** [arduino-esp32](https://github.com/espressif/arduino-esp32)
