# Manual Button Box ESP32-S3-WROOM1 N8R8

## 1) Visão Geral
Este firmware expõe um USB HID Gamepad com 10 eixos e até 64 botões. As funções reais (BB, MAP, TC, ABS) são sempre definidas no jogo. O hardware apenas envia eixos/botões.

## 2) Pinagem (ESP32-S3-WROOM1 N8R8 + MCP23017) — **revise antes de soldar**

### Regras básicas
- **GND comum** para todos os módulos (encoders, halls, matrix, WT32, MCP23017).
- **3.3V apenas** (não usar 5V nos halls/entradas).
- **Cada botão da matrix deve ter diodo 1N4148 em série** (sentido linha → diodo → botão → coluna).
- **MCP23017 alimentado com 3.3V** (VDD = 3.3V, VSS = GND).
- Pinos marcados com **⚠️** precisam de teste em hardware real (PSRAM/strapping).

### Sensores Hall (Analógico)
| GPIO | Função | O que soldar | Observações |
|------|--------|-------------|-------------|
| 1 | ADC1_CH0 | Hall A (Clutch L) | Saída analógica 0–3.3V |
| 2 | ADC1_CH1 | Hall B (Clutch R) | Saída analógica 0–3.3V |

**Dica:** VCC do Hall = 3.3V, GND comum. **Nunca 5V**.

### I2C (MCP23017)
| GPIO | Função | Conexão | Observações |
|------|--------|---------|-------------|
| 8 | I2C SDA | MCP23017 SDA | Pullup interno ativado |
| 9 | I2C SCL | MCP23017 SCL | Pullup interno ativado |

**MCP23017 Pinout:**
- VDD → 3.3V
- VSS → GND
- SDA → GPIO 8
- SCL → GPIO 9
- A0, A1, A2 → GND (endereço 0x20)
- RESET → 3.3V (ou 10kΩ pullup)

### Matrix 8x8 (64 Botões via MCP23017)
| MCP GPIO | Função | Tipo | Observações |
|----------|--------|------|-------------|
| GPA0 | COL0 | OUTPUT | Coluna 0 |
| GPA1 | COL1 | OUTPUT | Coluna 1 |
| GPA2 | COL2 | OUTPUT | Coluna 2 |
| GPA3 | COL3 | OUTPUT | Coluna 3 |
| GPA4 | COL4 | OUTPUT | Coluna 4 |
| GPA5 | COL5 | OUTPUT | Coluna 5 |
| GPA6 | COL6 | OUTPUT | Coluna 6 |
| GPA7 | COL7 | OUTPUT | Coluna 7 |
| GPB0 | ROW0 | INPUT_PULLUP | Linha 0 |
| GPB1 | ROW1 | INPUT_PULLUP | Linha 1 |
| GPB2 | ROW2 | INPUT_PULLUP | Linha 2 |
| GPB3 | ROW3 | INPUT_PULLUP | Linha 3 |
| GPB4 | ROW4 | INPUT_PULLUP | Linha 4 |
| GPB5 | ROW5 | INPUT_PULLUP | Linha 5 |
| GPB6 | ROW6 | INPUT_PULLUP | Linha 6 |
| GPB7 | ROW7 | INPUT_PULLUP | Linha 7 |

**Sentido do diodo:** linha → diodo → botão → coluna.
**Vantagens:** libera GPIO 4-13 do ESP32, resolve conflitos de PSRAM/strapping.

### Encoders (A/B)
| GPIO | Função | Encoder | Observações |
|------|--------|---------|-------------|
| 14 | ENC1_A | MFC A | INPUT_PULLUP |
| 15 | ENC1_B | MFC B | INPUT_PULLUP |
| 16 | ENC2_A | BB A | INPUT_PULLUP |
| 17 | ENC2_B | BB B | INPUT_PULLUP |
| 18 | ENC3_A | MAP A | INPUT_PULLUP |
| 21 | ENC3_B | MAP B | INPUT_PULLUP |
| 38 | ENC4_A | TC A | INPUT_PULLUP |
| 39 | ENC4_B | TC B | INPUT_PULLUP |
| 40 | ENC5_A | ABS A | INPUT_PULLUP |
| 41 | ENC5_B | ABS B | INPUT_PULLUP |
| 42 | ENC6_A | Lateral 1 A | INPUT_PULLUP |
| 47 | ENC6_B | Lateral 1 B | INPUT_PULLUP |
| 48 | ENC7_A | Lateral 2 A | INPUT_PULLUP |
| 35 | ENC7_B | Lateral 2 B | ⚠️ Testar (PSRAM) |
| 36 | ENC8_A | Lateral 3 A | ⚠️ Testar (PSRAM) |
| 37 | ENC8_B | Lateral 3 B | ⚠️ Testar (PSRAM) |
| 3 | ENC9_A | Lateral 4 A | ⚠️ Strapping |
| 46 | ENC9_B | Lateral 4 B | ⚠️ Strapping |

**Importante:** os pinos A/B vão direto ao encoder. O **SW** de cada encoder vai para a **matrix** (com diodo).

### UART (WT32)
| GPIO | Função | Conexão | Observações |
|------|--------|---------|-------------|
| 43 | UART TX | → WT32 RX (GPIO11) | 115200 baud |

**Atenção:** RX/TX **cruzado** (TX do ESP32 no RX do WT32) e GND comum.

## 3) Matrix 8x8 (64 slots via MCP23017)
- Colunas: MCP GPA0-GPA7 (8 colunas)
- Linhas: MCP GPB0-GPB7 (8 linhas)
- I2C: ESP32 GPIO 8 (SDA) e GPIO 9 (SCL)
- Endereço: 0x20 (A0=A1=A2=GND)
- Cada botão deve usar diodo 1N4148 em série (linha → diodo → botão → coluna)

**Slots sugeridos:**
- Slot 1: MFC SW
- Slots 2–5: SW dos encoders 2–5 (BB, MAP, TC, ABS)
- Slots 6–9: **botões push extras** (funções adicionais)
- Slots 10–22: **botões principais** (funções de jogo)
- Slots 23–27: **livres**
- Slot 28: SHIFT (uso interno, não reportado ao HID)
- Slots 29–64: **livres** (expansão futura)

**Configuração atual da matriz (28 entradas):**
- 12 botões frontais
- 5 SW de encoder
- 2 borboletas de marcha
- 2 botões extras frontal
- 2 botões extras traseiros
- 1 joystick 5-way (5 sinais)

**Capacidade:** 64 slots totais, 28 usados inicialmente, **36 livres** para expansão.

## 4) Encoders EC11 (A/B + SW)
Cada encoder:
- A → GPIO do ENC*_A
- B → GPIO do ENC*_B
- GND comum
- SW entra na matrix (via diodo)

## 5) Hall Sensors (Clutches)
- VCC → 3.3V
- GND → GND
- OUT → GPIO1 e GPIO2

## 6) UART para WT32
- ESP32 GPIO43 (TX) → WT32 GPIO11 (RX)
- GND comum
- 115200 baud

Protocolo: `$CAT:FUNC:VAL\n`

## 7) Eixos HID
| Eixo | Função | Fonte |
|------|--------|-------|
| X | Encoder 2 | Eixo |
| Y | Encoder 3 | Eixo |
| Z | Clutch A | Hall GPIO1 |
| Rz | Clutch B | Hall GPIO2 |
| Rx | Encoder 4 | Eixo |
| Ry | Encoder 5 | Eixo |
| Slider | Encoder 6 | Eixo |
| Dial | Encoder 7 | Eixo |
| Vx | Encoder 8 | Eixo |
| Vy | Encoder 9 | Eixo |

## 8) Botões HID e HAT Switch
- Botões da matrix: 1–22, 27 (23 botões reportados ao HID)
- Slots 23–26 → **HAT/POV switch** (D-Pad, não são botões individuais)
- Slot 27 → **HID button 27** (center click = OK/confirm)
- Slot 28 (SHIFT) é **interno** e não vai ao HID

### Botões de Encoders (Switch)
- O **SW** de cada encoder 2–9 entra na matrix e aparece como botão HID (slot correspondente).
- O **SW do MFC** é o botão de confirmação no menu (não é usado como encoder normal).
- **Encoders laterais (ENC6–ENC9):** o SW **não será usado** (não dá para pressionar lateralmente nesse volante).

**Sugestão prática:** use os slots 6–9 para **botões extras**, já que os SW laterais não serão conectados.

 **Slots sugeridos (matriz 8x8 = 64 slots):**
 - Slot 1: MFC SW
 - Slots 2–5: SW dos encoders 2–5
 - Slots 6–22: botões principais (inclui borboletas e extras)
 - Slots 23–26: **5-way joystick (HAT/POV)** — UP/DOWN/LEFT/RIGHT
 - Slot 27: **5-way center click** (HID button 27 = OK/confirm)
 - Slot 28: SHIFT (uso interno, não reportado ao HID)
 - Slots 29–64: livres para expansão

### 5-Way Joystick → HAT/POV Switch

Os 4 sinais direcionais do 5-way joystick (slots 23–26) são convertidos em um **HAT/POV switch** HID, que o Windows reconhece como D-Pad nativo. O center click (slot 27) continua como botão HID normal.

| Slot | Direção | HAT Value |
|------|---------|----------|
| 23 | UP | 1 (N) |
| 24 | DOWN | 5 (S) |
| 25 | LEFT | 7 (W) |
| 26 | RIGHT | 3 (E) |
| — | Nenhum | 0 (Null) |

**Diagonais (8 direções):** o firmware detecta combinações simultâneas:

| Combinação | HAT Value | Direção |
|------------|-----------|--------|
| UP + RIGHT | 2 | NE |
| DOWN + RIGHT | 4 | SE |
| DOWN + LEFT | 6 | SW |
| UP + LEFT | 8 | NW |

**No Windows Game Controllers:** o HAT aparece como o POV clássico (seta que gira 360°). Jogos de corrida reconhecem automaticamente.

**Encoders em modo BTN (botões virtuais 40–55):**
- ENC2 (BB): Buttons 40/41
- ENC3 (MAP): Buttons 42/43
- ENC4 (TC): Buttons 44/45
- ENC5 (ABS): Buttons 46/47
- ENC6 (Lateral 1): Buttons 48/49
- ENC7 (Lateral 2): Buttons 50/51
- ENC8 (Lateral 3): Buttons 52/53
- ENC9 (Lateral 4): Buttons 54/55

**Observação:** com 64 botões HID, não é necessário desativar botões reais da matriz.

### Mapeamento no Jogo (Botões)
- **Matrix 1–27:** mapeie diretamente as funções do jogo (BB, MAP, TC, ABS, etc).
- **Encoders em BTN (40–55):** mapeie como "incremento/decremento".
- **Botões Virtuais (6-8, 38-39, 60-64):** usados pelo MFC em modo ajuste (TC2/FFB/TYRE/VOL_A/VOL_B).

## 9) Comandos (Manual)

### Combos
| Combo | Tempo | Ação |
|------|------|------|
| SHIFT + MFC rotação | — | Navega rápido (pula 2 itens) |
| SHIFT + MFC press | <0.5s | Preset rápido (varia conforme item) |
| SHIFT + MFC press | 1.5s | Toggle ENC_MODE (AXIS ↔ BTN) |
| SHIFT + Clutch A+B | Imediato | Swap embreagens (inverte Z/Rz) |
| SHIFT + Clutch A+B | 2s | Ciclar modo embreagem (DUAL/MIRROR/BITE/PROGRESSIVE/SINGLE_L/SINGLE_R) |

### Atalhos e Controles Principais
- **MFC (Encoder 1):** navega e ajusta o menu (não é usado como eixo/botão comum).
- **SHIFT (Slot 28):** modificador interno com 4 combos avançados (ver tabela acima).
- **Encoders 2–9:** podem operar como **eixos** ou **botões**, conforme ENC_MODE.

### MFC (Encoder 1) - Menu Ajustável
O encoder MFC agora funciona em **dois modos**:

#### Modo Navegação (padrão)
- **Girar MFC** → navega pelos itens do menu
- **Pressionar MFC** → entra no modo de ajuste (para certos itens)

#### Navegação Rápida (com SHIFT)
- **SHIFT + Girar MFC** → pula 2 itens em vez de 1
- Útil para navegar rapidamente entre 15 itens do menu

#### Modo Ajuste (quando em um item ajustável)
- **Girar MFC** → altera o valor/envia comando
- **Pressionar MFC** → sai e volta ao modo navegação

#### Modo Ajuste + SHIFT (Presets Rápidos)
- **SHIFT + MFC press curto** em **PAGE** → reseta para página 0
- **SHIFT + MFC press curto** em **BRIGHT** → reseta para brilho 220 (padrão)
- **SHIFT + MFC press curto** em **TC2** → TC2 no valor máximo
- **SHIFT + MFC press curto** em **FFB** → FFB no valor máximo
- **SHIFT + MFC press curto** em **TYRE** → TYRE no valor máximo

### Itens do Menu MFC (15 itens)

#### Itens de Um Clique (sem modo ajuste)
1. **CLUTCH** — cicla DUAL → MIRROR → BITE → PROGRESSIVE → SINGLE_L → SINGLE_R
	- **DUAL**: dois eixos totalmente independentes (pode mapear para acelerador/freio)
	- **MIRROR**: média dos dois paddles
	- **BITE**: largada F1 com remapeamento para bite point
	- **PROGRESSIVE**: limitador inverso (rally/drift)
	- **SINGLE_L/SINGLE_R**: apenas um paddle ativo
2. **CALIB** — inicia/finaliza calibração Hall
3. **ENC MODE** — alterna AXIS ↔ BTN
4. **RESET** — reseta tudo pro padrão

#### Itens com Modo Ajuste (gira MFC para ajustar)
5. **BITE** — ajusta bite point (0-100, gira MFC)
6. **BRIGHT** — ajusta brilho tela+LEDs (15-255, gira MFC, envia UART)
7. **PAGE** — muda páginas dashboard (gira MFC → NEXT/PREV, envia UART)
8. **VOL_SYS** — volume Windows (HID Consumer Control)
9. **VOL_A** — botão virtual 7/8 para software mixer
10. **VOL_B** — botão virtual 38/39 para software mixer
11. **TC2** — botão virtual 60/61 (mapeia no jogo)
12. **FFB** — botão virtual 62/63 (mapeia no jogo)
13. **TYRE** — botão virtual 64/6 (mapeia no jogo)
14. **ERS** — cicla entre ERS MODES (BALANCED → HARVEST → DEPLOY → HOTLAP)
15. **FUEL** — mistura combustível (LEAN ↔ RICH, gira MFC)

### Modo Ajuste Detalhado

#### BITE (Bite Point)
- Pressione MFC no item **BITE**
- Gire MFC CW/CCW → ajusta 0-100
- Pressione MFC → sai e salva em NVS
- Ao sair, o WT32 mostra confirmação do ajuste

#### BRIGHT (Brilho)
- Pressione MFC no item **BRIGHT**
- Gire MFC CW (+15) / CCW (-15) → ajusta 15-255
- Envia UART `$BRIGHT:VAL:220` ao WT32 em tempo real
- Pressione MFC → sai (volta ao menu)
- **Proteção:** valor mínimo é 15 para não apagar a tela
- Ao sair, o WT32 mostra confirmação do ajuste

#### PAGE (Páginas Dashboard)
- Pressione MFC no item **PAGE**
- Gire MFC CW → próxima página, CCW → página anterior
- Envia UART `$PAGE:NEXT:` ou `$PAGE:PREV:`
- Pressione MFC → sai
- **Observação:** não exibe popup (evita poluição visual)

#### VOL_SYS (Volume Windows - HID Consumer Control)
- Pressione MFC no item **VOL_SYS**
- **Gire MFC** → Volume+/- (HID Consumer Control, reconhecido pelo Windows)
- **Botão RADIO** (slot 13) → MUTE (quando VOL_SYS ativo)
- **Botão FLASH** (slot 14) → PLAY/PAUSE (quando VOL_SYS ativo)
- Pressione MFC → sai

#### VOL_A, VOL_B (Volume Apps via Botões)
- Pressione MFC em **VOL_A** ou **VOL_B**
- Gire MFC CW → botão UP (7 ou 38), CCW → botão DN (8 ou 39)
- Mapeia em software de mixer (EarTrumpet, VoiceMeeter, etc)
- Pressione MFC → sai

#### TC2, FFB, TYRE (Botões Virtuais do Jogo)
- Pressione MFC em **TC2**, **FFB** ou **TYRE**
- Gire MFC CW → botão UP, CCW → botão DN
- No jogo, mapeia os botões para funções (ex: TC Map Up/Down)
- Pressione MFC → sai

### Calibração Hall
- Selecione **CALIB** no menu MFC
- Pressione MFC (inicia calibração)
- Mova as duas embreagens do mínimo ao máximo
- Pressione MFC novamente (finaliza e salva em NVS)
- Se a calibração for inválida, o WT32 mostra **CALIB ERR: HALL** e os valores voltam ao padrão

### Swap de Embreagens (Inversão Rápida)
- **SHIFT + Borboleta A + Borboleta B** (simultaneamente, sem tempo de espera)
  - Inverte os eixos Z/Rz instantaneamente
  - Útil se a pedaleira está conectada invertida ou há Hall invertido
  - Envia UART: `$CLUTCH:SWAP:OK`
  - Segure > 2s DEPOIS do swap para ciclar modo de embreagem (mantém a inversão)

### Mapeamento no Jogo (Eixos)
- **ENC_MODE = AXIS**: cada encoder 2–9 vira um eixo analógico (X, Y, Rx, Ry, Slider, Dial, Vx, Vy).
- **Clutches (Z/Rz):** podem ser mapeadas como embreagem, freio ou acelerador (modo DUAL permite uso independente).
- **ENC_MODE = BTN**: cada encoder vira dois botões (CW/CCW) para funções de incremento/decremento.

## 10) Botões Virtuais HID (MFC menu ajuste)

Enviados pelo encoder MFC em modo ajuste (IDs dentro do descriptor de 64 botões):

| Botão | Função | Usado por | Modo |
|-------|--------|-----------|------|
| 6 | TYRE DN | Menu MFC | Ajuste |
| 7 | VOL_A UP | Menu MFC | Ajuste |
| 8 | VOL_A DN | Menu MFC | Ajuste |
| 38 | VOL_B UP | Menu MFC | Ajuste |
| 39 | VOL_B DN | Menu MFC | Ajuste |
| 60 | TC2 UP | Menu MFC | Ajuste |
| 61 | TC2 DN | Menu MFC | Ajuste |
| 62 | FFB UP | Menu MFC | Ajuste |
| 63 | FFB DN | Menu MFC | Ajuste |
| 64 | TYRE UP | Menu MFC | Ajuste |

**Nota:** IDs 6/7/8 são slots fisicamente vazios na matriz (nunca soldados), portanto não há conflito com botões físicos. IDs 65-69 ultrapassariam o limite de 64 botões do descriptor HID e não foram usados.

## 11) HID Consumer Control (Multimídia)

Quando **VOL_SYS** está em modo de ajuste:

| Comando | Código | Origem |
|---------|--------|--------|
| Volume + | 0xE9 | Giro MFC CW |
| Volume - | 0xEA | Giro MFC CCW |
| Mute | 0xE2 | Botão RADIO |
| Play/Pause | 0xCD | Botão FLASH |

## 12) Observações Importantes
- A função real dos encoders deve ser mapeada no jogo.
- Se o jogo aceita eixo, use ENC_MODE = AXIS.
- Se o jogo não aceita eixo, use ENC_MODE = BTN.
- GPIOs 35–37 e 3/46 devem ser testados no hardware real.
- **VOL_SYS multimídia** requer que o Windows reconheça o USB HID Consumer Control automaticamente.
- **VOL_A e VOL_B** dependem de software de mixer instalado no PC (EarTrumpet, VoiceMeeter, etc).
- **TC2, FFB, TYRE** devem ser mapeados dentro do jogo para as funções desejadas.

## 13) Tabela de Resumo de IDs

| Intervalo | Uso | Quantidade | Observações |
|-----------|-----|------------|-------------|
| 1–5 | Botões matriz (HID) | 5 | MFC SW + ENC2-5 SW |
| 6–8 | MFC virtual TYRE DN / VOL_A | 3 | Slots vazios na matriz, usados como virtuais |
| 9–37 | Botões matriz (HID) | 29 | Físicos (exceto 23-26 que viram HAT) |
| 23–26 | HAT/POV (5-way dir.) | 4 | Convertidos em HAT switch, **não** botões |
| 27 | 5-way center click | 1 | HID button 27 (OK/confirm) |
| 28 | SHIFT | 1 | Interno, não reportado ao HID |
| 38–39 | MFC virtual VOL_B | 2 | Acima de MATRIX_HID_MAX=37, sem conflito |
| 40–55 | Encoders BTN mode | 16 | Virtuais, 2 botões cada (8 encoders) |
| 56–59 | SHIFT+ENC2-5 (BTN) | 4 | Virtuais, shift combos |
| 60–64 | MFC menu (ajuste) | 5 | TC2 UP/DN, FFB UP/DN, TYRE UP |
| **Total** | **Livres dentro de 64** | vários | Slots 15-16, 23-24, 31-32 livres p/ expansão |

**Sem conflitos:** IDs 6-8 são slots fisicamente vazios na matriz (nunca soldados). Botões físicos terminam no slot 37 (MATRIX_HID_MAX). IDs 38-64 são exclusivamente virtuais.

## 14) Camada SHIFT (Consumer Control — Dispositivo 2 no Content Manager)

O firmware registra dois dispositivos HID no USB: **Gamepad** (64 botões + eixos + HAT) e **Consumer Control** (volume/mídia). O Content Manager do Assetto Corsa mostra ambos como dispositivos separados com o nome "ESP-ButtonBox-WHEEL".

A **camada SHIFT** aproveita o Consumer Control como segundo dispositivo de botões: quando SHIFT está pressionado, certos botões e encoders enviam usage IDs do Consumer Control em vez do botão normal do Gamepad. Isso **dobra** as funções disponíveis sem alterar o hardware.

### Regras do SHIFT Layer
- **SHIFT suprime o botão normal**: SHIFT+Frontal1 NÃO dispara o botão 17 do Gamepad — apenas o CC
- **Borboletas são IMUNES**: SHIFT+Borboleta sempre troca marcha (segurança em corrida)
- **Combos existentes preservados**: SHIFT+MFC, SHIFT+ENC2-5 rotação, SHIFT+Clutch continuam iguais

### Mídia Direta (sem precisar do MFC)

| Combo | Ação | Consumer Control |
|---|---|---|
| SHIFT + RADIO (slot 13) | Mute toggle | 0xE2 |
| SHIFT + FLASH (slot 14) | Play/Pause | 0xCD |

### Troca de Página WT32 (UART)

| Combo | Ação | UART |
|---|---|---|
| SHIFT + Traseiro 1 (slot 35) | Próxima página | `$PAGE:NEXT:` |
| SHIFT + Traseiro 2 (slot 36) | Página anterior | `$PAGE:PREV:` |

### Botões Extras de Jogo (Consumer Control → Device 2 no CM)

Cada combo SHIFT+botão envia um CC usage ID, que o Content Manager vê como botão no segundo dispositivo. Mapear no jogo como qualquer outro botão.

| Combo | CC Usage | Sugestão de Uso |
|---|---|---|
| SHIFT + Frontal 1 (slot 17) | 0x01 | Ignição / Starter |
| SHIFT + Frontal 2 (slot 18) | 0x02 | Pit Limiter |
| SHIFT + Frontal 3 (slot 19) | 0x03 | Wiper |
| SHIFT + Frontal 4 (slot 20) | 0x04 | Headlights |
| SHIFT + Frontal 5 (slot 21) | 0x05 | Rain Light |
| SHIFT + Frontal 6 (slot 22) | 0x06 | Flash Light |
| SHIFT + Frontal 7 (slot 37) | 0x07 | HUD toggle |
| SHIFT + Extra 1 (slot 9) | 0x08 | Camera cycle |
| SHIFT + Extra 2 (slot 10) | 0x09 | Look back |
| SHIFT + Extra 3 (slot 11) | 0x0A | Pit request |
| SHIFT + Extra 4 (slot 12) | 0x0B | Chat / Spotter |
| SHIFT + 5-way CENTER (slot 29) | 0x0E | MFD cycle |
| SHIFT + ENC2 SW (slot 2) | 0x0F | BB reset |
| SHIFT + ENC3 SW (slot 3) | 0x10 | MAP reset |
| SHIFT + ENC4 SW (slot 4) | 0x11 | TC reset |
| SHIFT + ENC5 SW (slot 5) | 0x12 | ABS reset |

### 5-Way Joystick com SHIFT (Consumer Control)

Quando SHIFT está ativo, o 5-way envia CC em vez de HAT/D-Pad:

| Combo | CC Usage | Sugestão |
|---|---|---|
| SHIFT + 5-way UP (slot 25) | 0x1B | MFD Up |
| SHIFT + 5-way DOWN (slot 26) | 0x1C | MFD Down |
| SHIFT + 5-way LEFT (slot 27) | 0x1D | MFD Left |
| SHIFT + 5-way RIGHT (slot 28) | 0x1E | MFD Right |

**Nota:** O HAT/D-Pad fica inativo enquanto SHIFT é segurado. Soltar SHIFT restaura o HAT normalmente.

### Encoders Laterais com SHIFT (Consumer Control)

Os encoders ENC6–ENC9 (laterais) ganham função extra via SHIFT:

| Combo | CC CW / CCW | Sugestão |
|---|---|---|
| SHIFT + ENC6 (Lateral 1) | 0x13 / 0x14 | Turbo adjust |
| SHIFT + ENC7 (Lateral 2) | 0x15 / 0x16 | Engine brake |
| SHIFT + ENC8 (Lateral 3) | 0x17 / 0x18 | Diff adjust |
| SHIFT + ENC9 (Lateral 4) | 0x19 / 0x1A | MGU-K deploy |

### Como Mapear no Content Manager / Assetto Corsa

1. No Content Manager, abra **Settings → Controls**
2. O segundo dispositivo "ESP-ButtonBox-WHEEL" aparece (é o Consumer Control)
3. Clique no campo do binding desejado
4. Segure SHIFT no volante + pressione o botão — o CM detecta o "botão" no Device 2
5. Confirme o binding

**Total:** 30 novas funções SHIFT (2 mídia + 2 UART page + 18 botões CC + 8 encoders CC).
