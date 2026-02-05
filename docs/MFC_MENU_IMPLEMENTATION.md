# Menu MFC Ajustável - Implementação Completa

## 📋 Resumo da Implementação

Sistema profissional de menu no encoder MFC (Multifunctional) com navegação dupla e modo de ajuste em tempo real.

---

## 🎮 Funcionamento do Menu MFC

### Modo Navegação (padrão)
```
Girar MFC CW/CCW
    ↓
Navega pelos 15 itens do menu
    ↓
Display mostra: "MFC: <ITEM>"
```

### Modo Ajuste (quando pressiona MFC em item ajustável)
```
Pressionar MFC
    ↓
Entra no modo de ajuste
    ↓
Girar MFC CW/CCW
    ↓
Modifica valor/envia comando
    ↓
Pressionar MFC novamente
    ↓
Sai e volta ao modo navegação
```

---

## 🔧 Os 15 Itens do Menu

### 1️⃣ CLUTCH (Um clique)
- Cicla entre: DUAL → MIRROR → BITE → PROGRESSIVE → SINGLE_L → SINGLE_R
- Salva em NVS
- **DUAL**: Totalmente independente, sem bite point (pode mapear para acelerador/freio)
- **MIRROR**: Sincronizado (média), sem bite point
- **BITE**: F1 Style com detecção automática de largada
- **PROGRESSIVE**: Limitador inverso (rally/drift)
- **SINGLE_L / SINGLE_R**: Apenas um paddle ativo

### 2️⃣ BITE (Modo Ajuste)
- Faixa: 0-100%
- Girar MFC ±1 por pulso
- Salva em NVS ao sair
- **Ativo apenas no modo CLUTCH = BITE**
- Controla o ponto de bite para largadas F1

### 3️⃣ CALIB (Um clique)
- Inicia/Finaliza calibração Hall
- Coleta min/max dos sensores analógicos

### 4️⃣ ENC MODE (Um clique)
- Alterna: AXIS ↔ BTN
- Aplica ao todos encoders 2-9
- MFC (encoder 1) sempre dedicado ao menu

### 5️⃣ BRIGHT (Modo Ajuste)
- Faixa: 15-255 (PWM backlight WT32)
- Passo: ±15 por giro
- UART: `$BRIGHT:VAL:220` em tempo real
- Controla tela e LEDs simultaneamente

### 6️⃣ PAGE (Modo Ajuste)
- Cicla: 0→1→2→3→4→5→6→0
- UART: `$PAGE:NEXT:` ou `$PAGE:PREV:`
- Muda páginas do dashboard WT32

### 7️⃣ VOL_SYS (Modo Ajuste + Multimídia)
- **Giro MFC**: HID Consumer Control Volume+/Volume-
- **Botão RADIO**: HID Consumer Control Mute (0xE2)
- **Botão FLASH**: HID Consumer Control Play/Pause (0xCD)
- Windows reconhece automaticamente

### 8️⃣ VOL_A (Modo Ajuste)
- **Giro MFC CW**: Botão HID virtual 56 (UP)
- **Giro MFC CCW**: Botão HID virtual 57 (DN)
- Mapeia em software de mixer (EarTrumpet, VoiceMeeter)

### 9️⃣ VOL_B (Modo Ajuste)
- **Giro MFC CW**: Botão HID virtual 58 (UP)
- **Giro MFC CCW**: Botão HID virtual 59 (DN)
- Independente de VOL_A, usa software mixer

### 🔟 TC2 (Modo Ajuste)
- **Giro MFC CW**: Botão HID virtual 50 (UP)
- **Giro MFC CCW**: Botão HID virtual 51 (DN)
- Mapeia no jogo para: TC Map Up/Down

### 1️⃣1️⃣ TC3 (Modo Ajuste)
- **Giro MFC CW**: Botão HID virtual 52 (UP)
- **Giro MFC CCW**: Botão HID virtual 53 (DN)
- Mapeia no jogo para: TC 3-way Up/Down

### 1️⃣2️⃣ TYRE (Modo Ajuste)
- **Giro MFC CW**: Botão HID virtual 54 (UP)
- **Giro MFC CCW**: Botão HID virtual 55 (DN)
- Mapeia no jogo para: Compound Up/Down, Tire Pressure, etc

### 1️⃣3️⃣ ERS (Um clique - F1/Hybrid)
- Cicla entre: BALANCED → HARVEST → DEPLOY → HOTLAP
- Salva em NVS
- **BALANCED**: Modo padrão de corrida
- **HARVEST**: Acumula energia (recuperação máxima)
- **DEPLOY**: Gasta toda energia acumulada
- **HOTLAP**: Máxima potência (quali/overtake)

### 1️⃣4️⃣ FUEL (Modo Ajuste - F1/Hybrid)
- Faixa: 0-100% (LEAN ↔ RICH)
- Girar MFC ±1 por pulso
- Salva em NVS ao sair
- Controla estratégia de consumo de combustível
- UART: `$FUEL:VAL:75`

### 1️⃣5️⃣ RESET (Um clique)
- Reseta tudo para padrão
- Limpa NVS
- UART: `$SYS:RESET:OK`

---

## 📡 Protocolo UART (Button Box → WT32)

### Navegação
```
$MFC:NAV:BRIGHT    (quando gira MFC em navegação)
```

### Entrando em Modo Ajuste
```
$MFC:ADJUST:BRIGHT    (quando pressiona MFC)
$BRIGHT:VAL:220       (valores continuam sendo enviados)
```

### Comandos Específicos
```
$BRIGHT:VAL:220       (valor 15-255)
$PAGE:NEXT:           (próxima página)
$PAGE:PREV:           (página anterior)
$BITE:VAL:75          (bite 0-100)
$FUEL:VAL:75          (fuel mix 0-100, LEAN to RICH)
$CLUTCH:MODE:DUAL    (DUAL/MIRROR/BITE/PROGRESSIVE/SINGLE_L/SINGLE_R)
$ERS:MODE:BALANCED   (BALANCED/HARVEST/DEPLOY/HOTLAP)
$MEDIA:MUTE:1         (quando pressiona RADIO em VOL_SYS)
$MEDIA:PLAY_PAUSE:1   (quando pressiona FLASH em VOL_SYS)
$MFC:CONFIRM:BITE     (confirma ajuste de bite)
$MFC:CONFIRM:BRIGHT   (confirma ajuste de brilho)
$MFC:CONFIRM:FUEL     (confirma ajuste de fuel)
$CALIB:INVALID:HALL   (calibração inválida, valores resetados)
```

---

## 🎛️ Botões Virtuais HID (50-59)

| Botão | Item | CW | CCW | Para Mapear |
|-------|------|----|----|------------|
| 50-51 | TC2 | UP | DN | TC Map 1/2/3 |
| 52-53 | TC3 | UP | DN | TC Range/Mode |
| 54-55 | TYRE | UP | DN | Compound/Pressure |
| 56-57 | VOL_A | UP | DN | Software Mixer |
| 58-59 | VOL_B | UP | DN | Software Mixer |

---

## 🔊 HID Consumer Control

Quando **VOL_SYS** está em modo de ajuste:

| Ação | Código HID | Windows Reconhece |
|------|-----------|------------------|
| Giro CW | 0xE9 (Volume Up) | ✅ Sim |
| Giro CCW | 0xEA (Volume Down) | ✅ Sim |
| RADIO | 0xE2 (Mute) | ✅ Sim |
| FLASH | 0xCD (Play/Pause) | ✅ Sim |

Relatório HID Separado (Report ID 2) para compatibilidade máxima.

---

## 💾 Persistência (NVS/Preferences)

Salvos automaticamente:
- Modo embreagem (DUAL/MIRROR/BITE)
- Calibração Hall (min/max analógicos)
- Bite point
- Modo encoder (AXIS/BTN)

---

## 🎯 Casos de Uso

### Caso 1: Ajustar Brilho da Tela
```
1. Gira MFC → "BRIGHT" aparece
2. Pressiona MFC → Entra modo ajuste
3. Gira MFC CW (5 vezes) → Brilho de 220 para 295 (capped 255)
4. Pressiona MFC → Sai, volta ao menu
```

### Caso 2: Controlar Volume Windows
```
1. Gira MFC → "VOL_SYS" aparece
2. Pressiona MFC → Entra modo multimídia
3. Gira MFC CW/CCW → Volume+/Volume- (reconhecido pelo Windows)
4. Pressiona RADIO → Mute
5. Pressiona FLASH → Play/Pause
6. Pressiona MFC → Sai
```

### Caso 3: Mapear TC Map no Jogo
```
1. Gira MFC → "TC2" aparece
2. Pressiona MFC → Entra modo ajuste
3. Gira MFC CW → Envia Botão HID 50
4. No jogo, mapeia Botão 50 → "TC Map Up"
5. Gira MFC CCW → Envia Botão HID 51
6. No jogo, mapeia Botão 51 → "TC Map Down"
7. Pressiona MFC → Sai
```

---

## 🔌 Integração Hardware

### Pinos Usados
- **GPIO 14-15**: Encoder MFC A/B
- **GPIO 43**: UART TX → WT32 GPIO11
- **GPIO 4-8**: Matrix colunas (RADIO, FLASH buttons aqui)
- **GPIO 9-13**: Matrix linhas

### Botões Matrix Especiais
- **Slot 1**: MFC press (Button 1 HID)
- **Slot 10**: RADIO/MUTE (Button 10 HID, ou multimídia em VOL_SYS)
- **Slot 11**: FLASH/PLAY (Button 11 HID, ou multimídia em VOL_SYS)
- **Slot 25**: SHIFT (interno, não aparece no HID)

---

## 🛠️ Estruturas de Dados

### Enum MfcMenuItem
```cpp
enum MfcMenuItem : uint8_t {
    MFC_CLUTCH = 0,
    MFC_BITE,
    MFC_CALIB,
    MFC_ENC_MODE,
    MFC_BRIGHT,
    MFC_PAGE,
    MFC_VOL_SYS,
    MFC_VOL_A,
    MFC_VOL_B,
    MFC_TC2,
    MFC_TC3,
    MFC_TYRE,
    MFC_ERS,
    MFC_FUEL,
    MFC_RESET,
    MFC_COUNT = 15
};
```

### Estados Globais
```cpp
int8_t mfcIndex = 0;              // Item atual
bool mfcAdjustMode = false;       // Nav vs Adjust
int16_t brightnessValue = 220;    // Valor atual
int8_t pageValue = 0;             // Página atual
```

---

## ✅ Checklist de Validação

- [x] Enum com 15 itens
- [x] Navegação ciclante (wrap around)
- [x] Modo ajuste com giro MFC
- [x] UART para WT32 com valores
- [x] HID Consumer Control (Report ID 2)
- [x] Botões virtuais 50-59
- [x] Multimídia em VOL_SYS (RADIO/FLASH)
- [x] F1/Hybrid (ERS + FUEL)
- [x] NVS persistência
- [x] Compilação sem erros
- [x] Documentação atualizada

---

## 📚 Arquivos Modificados

1. **src/main_wheel.cpp**
   - Enum MfcMenuItem
   - handleMfcRotate() completo
   - handleMfcPress() modo duplo
   - handleMultimediaButtons()
   - sendConsumerControl()
   - HID descriptor duplo (Report ID 1+2)

2. **src/main.cpp**
   - processButtonBoxLine() com novos comandos
   - Suporte a BRIGHT:VAL, PAGE:NEXT/PREV, MEDIA

3. **docs/MANUAL_BUTTONBOX.md**
   - Seções 9-12 reescritas
   - Tabelas de botões virtuais
   - Consumer Control explicado
   - Casos de uso

---

## 🏎️ Modos de Embreagem - Detalhamento Completo

### **MODO 1: DUAL (Totalmente Independente)**

**Descrição:**
Cada paddle controla sua própria embreagem de forma completamente independente. Não há sincronização nem limite de bite point.

**Ajuste de Bite:**
- ❌ **Não possui** ajuste de bite point
- Cada paddle envia seu valor diretamente (0-100%)

**Fórmula:**
```cpp
outA = paddleA  // Esquerda independente
outB = paddleB  // Direita independente
```

**Casos de Uso:**
- 🏁 Dragster (controle individual para wheelie control)
- 🌲 Rally (modulação independente em terrenos irregulares)
- 🔧 Manobras de pit (aciona apenas uma)

**Exemplos:**

| Paddle A | Paddle B | Saída A | Saída B |
|----------|----------|---------|---------|
| 100% | 100% | 100% | 100% |
| 50% | 80% | 50% | 80% |
| 0% | 100% | 0% | 100% |
| 30% | 0% | 30% | 0% |

---

### **MODO 2: MIRROR (Sincronizado)**

**Descrição:**
Ambas as embreagens sempre recebem a **média aritmética** dos dois paddles. Sincronização total e simétrica.

**Ajuste de Bite:**
- ❌ **Não possui** ajuste de bite point
- Sempre usa a média entre as duas (0-100%)

**Fórmula:**
```cpp
avg = (paddleA + paddleB) / 2
outA = avg
outB = avg
```

**Casos de Uso:**
- 🏎️ Fórmula 2, Fórmula 3 (sem bite point sofisticado)
- 🚗 Street cars, GT4, carros simples
- 🎮 Iniciantes (comportamento previsível)

**Exemplos:**

| Paddle A | Paddle B | Média | Saída |
|----------|----------|-------|-------|
| 100% | 100% | 100% | 100% |
| 80% | 20% | 50% | 50% |
| 100% | 0% | 50% | 50% |
| 30% | 70% | 50% | 50% |

---

### **MODO 3: BITE (F1 Style - Assimétrico Inteligente)** ⭐

**Descrição:**
Modo profissional com **detecção automática de largada**. Quando ambas as paddles partem de 100%, ativa **modo largada** que remapeia o range para controle fino do bite point. Nos boxes, comporta-se como independente.

**Ajuste de Bite:**
- ✅ **Possui ajuste de 0-100%** (padrão: 60%)
- Ativo APENAS em modo largada
- Controlável via menu MFC (item BITE)

**Fórmula:**
```cpp
// Detecção de modo largada
if (paddleA > 95% && paddleB > 95%) launchMode = TRUE
if (paddleA < 5% && paddleB < 5%) launchMode = FALSE

combined = MAX(paddleA, paddleB)

// MODO LARGADA: Remapeia quando solta UMA paddle
if (launchMode && (paddleA == 0 || paddleB == 0)) {
    combined = (combined * bitePoint) / 100
}

outA = combined
outB = combined
```

**Estados:**

**🏁 Modo Largada (ambas partiram de 100%):**
- Ativa quando **ambas > 95%**
- Remapeia paddle ativa de 0-100% para 0-bitePoint%
- Permite controle fino do bite usando range completo
- Desativa quando **ambas < 5%**

**🔧 Modo Boxes (ambas partiram de 0%):**
- Comportamento independente (igual DUAL)
- Sem remapeamento de bite
- Permite 100% com uma paddle só

**Casos de Uso:**
- 🏎️ Fórmula 1, Fórmula E
- 🏁 GT3, LMP1, IndyCar
- 🎯 Qualquer single-seater profissional

**Exemplos (Bite Point = 60%):**

**Largada:**

| Paddle A | Paddle B | LaunchMode | Cálculo | Saída |
|----------|----------|------------|---------|-------|
| 100% | 100% | ✅ | MAX=100% | 100% |
| 0% | 100% | ✅ | 100% × 60% | 60% |
| 0% | 80% | ✅ | 80% × 60% | 48% |
| 0% | 50% | ✅ | 50% × 60% | 30% |
| 0% | 20% | ✅ | 20% × 60% | 12% |
| 0% | 0% | ❌ | MAX=0% | 0% |

**Boxes:**

| Paddle A | Paddle B | LaunchMode | Saída |
|----------|----------|------------|-------|
| 0% | 0% | ❌ | 0% |
| 0% | 50% | ❌ | 50% |
| 0% | 100% | ❌ | 100% |
| 80% | 0% | ❌ | 80% |

---

### **MODO 4: PROGRESSIVE (Rally/Drift)**

**Descrição:**
Modo com efeito de tração controlada. A saída da direita é limitada pelo inverso da esquerda.

**Ajuste de Bite:**
- ❌ **Não possui** ajuste de bite point

**Fórmula:**
```cpp
combined = MAX(paddleA, paddleB)
outA = combined
outB = MIN(paddleB, 100% - paddleA)
```

**Casos de Uso:**
- 🌲 Rally, Drift, Rallycross
- Situações onde tração progressiva ajuda a controlar a saída

**Exemplos:**

| Paddle A | Paddle B | Saída A | Saída B |
|----------|----------|---------|---------|
| 80% | 50% | 80% | 20% |
| 20% | 80% | 80% | 80% |
| 60% | 60% | 60% | 40% |

---

### **MODO 5: SINGLE_L / SINGLE_R (Um Paddle Ativo)**

**Descrição:**
Somente um paddle fica ativo. O outro é ignorado.

**Ajuste de Bite:**
- ❌ **Não possui** ajuste de bite point

**Fórmula:**
```cpp
// SINGLE_L
outA = paddleA
outB = 0

// SINGLE_R
outA = 0
outB = paddleB
```

**Casos de Uso:**
- 🔧 Paddle com defeito
- 🎮 Preferência por um lado só
- 🧪 Testes de mapeamento

**Exemplos:**

| Modo | Paddle A | Paddle B | Saída A | Saída B |
|------|----------|----------|---------|---------|
| SINGLE_L | 70% | 40% | 70% | 0% |
| SINGLE_R | 70% | 40% | 0% | 40% |

---

## 🎛️ Resumo Comparativo

| Modo | Bite Point | Sincronização | Uso Principal |
|------|-----------|---------------|---------------|
| **DUAL** | Não | Nenhuma | Rally, Dragster |
| **MIRROR** | Não | Média | F2, Street Cars |
| **BITE** | **Sim (0-100%)** | MAX + Remapeamento | F1, GT3, Pro Racing |
| **PROGRESSIVE** | Não | Limite inverso | Rally, Drift |
| **SINGLE_L/R** | Não | Apenas um eixo | Contingência/Testes |

---

## 🔧 Como Ajustar o Bite Point

### Passo a Passo:
```
1. Gira MFC → "CLUTCH" aparece
2. Pressiona MFC → Cicla para modo "BITE"
3. Gira MFC → "BITE" aparece (item de ajuste)
4. Pressiona MFC → Entra modo ajuste
5. Gira MFC CW/CCW → Ajusta 0-100% (±1% por pulso)
6. Display WT32 mostra: "BITE: 75%"
7. Pressiona MFC → Salva em NVS
```

### Valores Recomendados:
- **40-50%**: Largadas agressivas (risco de wheelspin)
- **55-65%**: Balanceado (padrão: 60%)
- **70-85%**: Conservador (pneus frios, chuva)

---

## 📊 Quando Usar Cada Modo

### Escolha **DUAL** se:
- Precisa de controle individual total
- Rally/Dirt com aderência irregular
- Manobras de pit com uma mão

### Escolha **MIRROR** se:
- Carros sem bite point (F2, F3, GT4)
- Prefere comportamento simétrico
- Está começando em sim racing

### Escolha **BITE** se:
- Corre F1, GT3, LMP, IndyCar
- Precisa de largadas consistentes
- Quer comportamento profissional (Fanatec/Simucube)

### Escolha **PROGRESSIVE** se:
- Corre rally/drift e quer mais controle de tração
- Prefere um limitador dinâmico na saída

### Escolha **SINGLE_L/R** se:
- Quer usar apenas um paddle
- Precisa contornar falha de hardware

---

## 🚀 Próximos Passos (Opcionais)

- [ ] Compact mode para games com <10 axes
- [ ] Display OLED para mostrar menu em tempo real
- [ ] Persistência de mapa de botões customizado
- [ ] Wireless via Bluetooth (ESP32 suporta)

