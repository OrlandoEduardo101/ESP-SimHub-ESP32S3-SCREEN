# Manual Button Box ESP32-S3-WROOM1 N8R8

## 1) Visão Geral
Este firmware expõe um USB HID Gamepad com 10 eixos e até 64 botões. As funções reais (BB, MAP, TC, ABS) são sempre definidas no jogo. O hardware apenas envia eixos/botões.

## 2) Pinagem (ESP32-S3-WROOM1 N8R8) — **revise antes de soldar**

### Regras básicas
- **GND comum** para todos os módulos (encoders, halls, matrix, WT32).
- **3.3V apenas** (não usar 5V nos halls/entradas).
- **Cada botão da matrix deve ter diodo 1N4148 em série** (sentido linha → diodo → botão → coluna).
- Pinos marcados com **⚠️** precisam de teste em hardware real (PSRAM/strapping).

### Sensores Hall (Analógico)
| GPIO | Função | O que soldar | Observações |
|------|--------|-------------|-------------|
| 1 | ADC1_CH0 | Hall A (Clutch L) | Saída analógica 0–3.3V |
| 2 | ADC1_CH1 | Hall B (Clutch R) | Saída analógica 0–3.3V |

**Dica:** VCC do Hall = 3.3V, GND comum. **Nunca 5V**.

### Matrix 5x5 (Botões)
| GPIO | Função | Tipo | Observações |
|------|--------|------|-------------|
| 4 | COL0 | OUTPUT | Coluna 0 |
| 5 | COL1 | OUTPUT | Coluna 1 |
| 6 | COL2 | OUTPUT | Coluna 2 |
| 7 | COL3 | OUTPUT | Coluna 3 |
| 8 | COL4 | OUTPUT | Coluna 4 |
| 9 | ROW0 | INPUT_PULLUP | Linha 0 |
| 10 | ROW1 | INPUT_PULLUP | Linha 1 |
| 11 | ROW2 | INPUT_PULLUP | Linha 2 |
| 12 | ROW3 | INPUT_PULLUP | Linha 3 |
| 13 | ROW4 | INPUT_PULLUP | Linha 4 |

**Sentido do diodo:** linha → diodo → botão → coluna.

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

## 3) Matrix 5x5 (25 slots)
- Colunas: GPIO 4,5,6,7,8
- Linhas: GPIO 9,10,11,12,13
- Cada botão deve usar diodo 1N4148 em série (linha → diodo → botão → coluna)

**Slots sugeridos (com laterais sem SW):**
- Slot 1: MFC SW
- Slots 2–5: SW dos encoders 2–5
- Slots 6–9: **botões extras** (aproveite para push buttons ou funções adicionais)
- Slot 25: SHIFT (uso interno)

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

## 8) Botões HID
- Botões da matrix: 1–22 (22 botões)
- Slot 25 (SHIFT) é **interno** e não vai ao HID

### Botões de Encoders (Switch)
- O **SW** de cada encoder 2–9 entra na matrix e aparece como botão HID (slot correspondente).
- O **SW do MFC** é o botão de confirmação no menu (não é usado como encoder normal).
- **Encoders laterais (ENC6–ENC9):** o SW **não será usado** (não dá para pressionar lateralmente nesse volante).

**Sugestão prática:** use os slots 6–9 para **botões extras**, já que os SW laterais não serão conectados.

 **Slots sugeridos:**
 - Slot 1: MFC SW
 - Slots 2–5: SW dos encoders 2–5
 - Slots 6–9: reservados (SW dos encoders laterais não usados)
 - Slot 25: SHIFT (uso interno)
- ENC3: Buttons 25/26
- ENC4: Buttons 27/28
- ENC5: Buttons 29/30
- ENC6: Buttons 31/32
- ENC7: Buttons 33/34
- ENC8: Buttons 35/36
- ENC9: Buttons 37/38

**Observação:** com 64 botões HID, não é necessário desativar botões reais da matriz.

### Mapeamento no Jogo (Botões)
- **Matrix 1–22:** mapeie diretamente as funções do jogo (BB, MAP, TC, ABS, etc).
- **Encoders em BTN (23–38):** mapeie como “incremento/decremento”.
- **Botões Virtuais (50–59):** usados pelo MFC em modo ajuste (TC2/TC3/TYRE/VOL_A/VOL_B).

## 9) Comandos (Manual)

### Combos
| Combo | Tempo | Ação |
|------|------|------|
| SHIFT + MFC press | 1.5s | Toggle ENC_MODE (AXIS ↔ BTN) |
| SHIFT + Clutch A+B | 2s | Ciclar modo embreagem (DUAL/MIRROR/BITE/PROGRESSIVE/SINGLE_L/SINGLE_R) |

### Atalhos e Controles Principais
- **MFC (Encoder 1):** navega e ajusta o menu (não é usado como eixo/botão comum).
- **SHIFT (Slot 25):** modificador interno (não aparece no HID).
- **Encoders 2–9:** podem operar como **eixos** ou **botões**, conforme ENC_MODE.

### MFC (Encoder 1) - Menu Ajustável
O encoder MFC agora funciona em **dois modos**:

#### Modo Navegação (padrão)
- **Girar MFC** → navega pelos itens do menu
- **Pressionar MFC** → entra no modo de ajuste (para certos itens)

#### Modo Ajuste (quando em um item ajustável)
- **Girar MFC** → altera o valor/envia comando
- **Pressionar MFC** → sai e volta ao modo navegação

### Itens do Menu MFC (13 itens)

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
9. **VOL_A** — botão virtual 56/57 para software mixer
10. **VOL_B** — botão virtual 58/59 para software mixer
11. **TC2** — botão virtual 50/51 (mapeia no jogo)
12. **TC3** — botão virtual 52/53 (mapeia no jogo)
13. **TYRE** — botão virtual 54/55 (mapeia no jogo)
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
- Gire MFC CW → botão UP (56 ou 58), CCW → botão DN (57 ou 59)
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

### Mapeamento no Jogo (Eixos)
- **ENC_MODE = AXIS**: cada encoder 2–9 vira um eixo analógico (X, Y, Rx, Ry, Slider, Dial, Vx, Vy).
- **Clutches (Z/Rz):** podem ser mapeadas como embreagem, freio ou acelerador (modo DUAL permite uso independente).
- **ENC_MODE = BTN**: cada encoder vira dois botões (CW/CCW) para funções de incremento/decremento.

## 10) Botões Virtuais HID (50-59)

Enviados pelo encoder MFC em modo ajuste:

| Botão | Função | Usado por | Modo |
|-------|--------|-----------|------|
| 50 | TC2 UP | Menu MFC | Ajuste |
| 51 | TC2 DN | Menu MFC | Ajuste |
| 52 | TC3 UP | Menu MFC | Ajuste |
| 53 | TC3 DN | Menu MFC | Ajuste |
| 54 | TYRE UP | Menu MFC | Ajuste |
| 55 | TYRE DN | Menu MFC | Ajuste |
| 56 | VOL_A UP | Menu MFC | Ajuste |
| 57 | VOL_A DN | Menu MFC | Ajuste |
| 58 | VOL_B UP | Menu MFC | Ajuste |
| 59 | VOL_B DN | Menu MFC | Ajuste |

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
