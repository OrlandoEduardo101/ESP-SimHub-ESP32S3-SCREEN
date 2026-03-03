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
- **Botões Virtuais (60–69):** usados pelo MFC em modo ajuste (TC2/TC3/TYRE/VOL_A/VOL_B).

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
- **SHIFT + MFC press curto** em **TC3** → TC3 no valor máximo
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
9. **VOL_A** — botão virtual 66/67 para software mixer
10. **VOL_B** — botão virtual 68/69 para software mixer
11. **TC2** — botão virtual 60/61 (mapeia no jogo)
12. **TC3** — botão virtual 62/63 (mapeia no jogo)
13. **TYRE** — botão virtual 64/65 (mapeia no jogo)
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
- **Botão RADIO** (slot 10) → MUTE (quando VOL_SYS ativo)
- **Botão FLASH** (slot 11) → PLAY/PAUSE (quando VOL_SYS ativo)
- Pressione MFC → sai

#### VOL_A, VOL_B (Volume Apps via Botões)
- Pressione MFC em **VOL_A** ou **VOL_B**
- Gire MFC CW → botão UP (66 ou 68), CCW → botão DN (67 ou 69)
- Mapeia em software de mixer (EarTrumpet, VoiceMeeter, etc)
- Pressione MFC → sai

#### TC2, TC3, TYRE (Botões Virtuais do Jogo)
- Pressione MFC em **TC2**, **TC3** ou **TYRE**
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

## 10) Botões Virtuais HID (60-69)

Enviados pelo encoder MFC em modo ajuste:

| Botão | Função | Usado por | Modo |
|-------|--------|-----------|------|
| 60 | TC2 UP | Menu MFC | Ajuste |
| 61 | TC2 DN | Menu MFC | Ajuste |
| 62 | TC3 UP | Menu MFC | Ajuste |
| 63 | TC3 DN | Menu MFC | Ajuste |
| 64 | TYRE UP | Menu MFC | Ajuste |
| 65 | TYRE DN | Menu MFC | Ajuste |
| 66 | VOL_A UP | Menu MFC | Ajuste |
| 67 | VOL_A DN | Menu MFC | Ajuste |
| 68 | VOL_B UP | Menu MFC | Ajuste |
| 69 | VOL_B DN | Menu MFC | Ajuste |

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
- **TC2, TC3, TYRE** devem ser mapeados dentro do jogo para as funções desejadas.

## 13) Tabela de Resumo de IDs

| Intervalo | Uso | Quantidade | Observações |
|-----------|-----|------------|-------------|
| 1–22 | Botões matriz (HID) | 22 | Físicos, reportados ao HID |
| 23–26 | HAT/POV (5-way dir.) | 4 | Convertidos em HAT switch, **não** botões |
| 27 | 5-way center click | 1 | HID button 27 (OK/confirm) |
| 28 | SHIFT | 1 | Interno, não reportado ao HID |
| 40–55 | Encoders BTN mode | 16 | Virtuais, 2 botões cada (8 encoders) |
| 60–69 | MFC menu (ajuste) | 10 | Virtuais, 2 botões cada (5 itens) |
| **Total** | **Livres** | **até 64** | Espaço para expansão futura |

**Sem conflitos:** matriz física (1–22, 27) vs. HAT (23–26) vs. encoders virtuais (40–55) vs. MFC virtuais (60–69).
