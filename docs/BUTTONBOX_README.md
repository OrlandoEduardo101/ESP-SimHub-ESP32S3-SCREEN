# ESP32-S3-Zero SimHub ButtonBox

Firmware completo para ButtonBox de corrida compatível com SimHub usando ESP32-S3-Zero.

## 📋 Componentes

### Hardware Utilizado
- **ESP32-S3-Zero** (4MB Flash, USB-C)
- **Matriz de Botões 5x5** (25 botões)
- **4x Encoders Rotativos** (EC11, canais A/B — sem push usado, modo eixo ou botões)
- **Barra de LEDs WS2812B** (5 LEDs)
- **LED Onboard RGB** (GP21 - opcional)

## 🔌 Pinagem ESP32-S3-Zero

### Matriz de Botões (5x5)
| Componente | Função | GPIO | Direção |
|------------|--------|------|---------|
| Coluna 1 | Saída Matriz | GP1 | OUTPUT |
| Coluna 2 | Saída Matriz | GP2 | OUTPUT |
| Coluna 3 | Saída Matriz | GP3 | OUTPUT |
| Coluna 4 | Saída Matriz | GP4 | OUTPUT |
| Coluna 5 | Saída Matriz | GP5 | OUTPUT |
| Linha 1 | Entrada Botões | GP6 | INPUT_PULLUP |
| Linha 2 | Entrada Botões | GP7 | INPUT_PULLUP |
| Linha 3 | Entrada Botões | GP8 | INPUT_PULLUP |
| Linha 4 | Entrada Botões | GP9 | INPUT_PULLUP |
| Linha 5 | Entrada Botões | GP10 | INPUT_PULLUP |

**Numeração dos Botões (1-25):**
```
Linha 1: Botões 1-5
Linha 2: Botões 6-10
Linha 3: Botões 11-15
Linha 4: Botões 16-20
Linha 5: Botões 21-25
```

### Encoders Rotativos (4x)
| Encoder | Pino A | Pino B | Função |
|---------|--------|--------|--------|
| Encoder 1 | GP11 | GP12 | TC/ABS/Map/etc |
| Encoder 2 | GP13 | GP14 | Brake Bias |
| Encoder 3 | GP15 | GP16 | ARB/Diff |
| Encoder 4 | GP17 | GP18 | Volume/Etc |

**Todas as entradas configuradas com `INPUT_PULLUP`**

**Modo eixo x modo botões:** segure juntos os botões `11` e `20` por ~1,5s para alternar. 
- Modo eixos (padrão): encoders mapeados para **eixos de rotação** apenas (Z, Rx, Ry, Rz) — joystick X/Y ficam zerados.
  - Encoder 1 → Eixo Z
  - Encoder 2 → Rotação X
  - Encoder 3 → Rotação Y  
  - Encoder 4 → Rotação Z
- Modo botões: cada giro envia cliques nos botões virtuais 26-33 (CW/CCW). 
- LED 5 indica o modo: ciano = eixos, âmbar = botões.

### LEDs
| Componente | GPIO | Tipo | Função |
|------------|------|------|--------|
| Barra LEDs | GP45 | WS2812B | 5 LEDs indicadores |
| LED Onboard | GP21 | RGB | Status (opcional) |

## ⚡ Funcionalidades

### ✅ Implementado
- [x] **Matriz de Botões 5x5** com debounce de 50ms
- [x] **4 Encoders Rotativos** com detecção Gray Code
- [x] **5 LEDs WS2812B** com indicação visual
- [x] **Integração SimHub** (protocolo nativo)
- [x] **Detecção automática** de 25 botões
- [x] **Feedback Serial** completo para debug

### 🔧 Características Técnicas
- **Debounce**: 50ms para todos os botões
- **Scan Rate**: ~1ms por ciclo completo
- **Encoders**: Detecção por Gray Code (sem perda de pulsos)
- **LEDs**: Atualização em tempo real baseada em encoders
- **Serial**: 115200 baud (USB CDC)

## 📦 Compilação e Upload

### Compilar Firmware
```bash
pio run -e buttonbox-s3-zero
```

### Upload para ESP32-S3-Zero
```bash
pio run -e buttonbox-s3-zero -t upload
```

### Monitor Serial
```bash
pio device monitor -e buttonbox-s3-zero
```

## 🎮 Configuração no SimHub

### 1. Conectar Device
1. Abra **SimHub**
2. Vá em **Settings → Arduino**
3. Clique em **Scan for Arduinos**
4. Selecione `ESP-SimHub-ButtonBox` na porta detectada

### 2. Mapear Botões
1. Vá em **Controls & Events → Custom Serial Devices**
2. Selecione `ESP-SimHub-ButtonBox`
3. Configure os **25 botões** para suas funções:
   - DRS
   - Pit Limiter
   - Look Left/Right
   - TC/ABS Toggle
   - Ignition
   - Etc.

### 3. Mapear Encoders
Os 4 encoders são detectados como **botões incrementais**:
- Encoder 1 CW/CCW
- Encoder 2 CW/CCW
- Encoder 3 CW/CCW
- Encoder 4 CW/CCW

Configure para:
- TC Level +/-
- ABS Level +/-
- Brake Bias +/-
- Engine Map +/-

## 🔍 Debug e Testes

### Monitor Serial
O firmware envia logs completos via Serial USB:

```
========================================
ESP32-S3 SimHub ButtonBox Firmware
Version: j (ButtonBox Edition)
========================================
Device: ESP-SimHub-ButtonBox
Free Heap: 340000
========================================

[ButtonMatrix] Initialized 5x5 matrix (25 buttons)
[Encoders] Initialized 4 rotary encoders
[LEDs] Initialized 5 WS2812B LEDs on GP45
[Startup] LED Test Sequence

[System] ButtonBox Ready!
Waiting for SimHub connection...

[Button] Pressed: 1
[Button] Released: 1
[Encoder 1] CW: 1
[Encoder 2] CCW: -1
[SimHub] Hello command received
```

### Teste de LEDs
Na inicialização, os 5 LEDs acendem em sequência (verde) para verificar funcionamento.

### Teste de Botões
Pressione qualquer botão e veja no monitor serial:
```
[Button] Pressed: 12
[Button] Released: 12
```

### Teste de Encoders
Gire qualquer encoder e veja:
```
[Encoder 1] CW: 1   (sentido horário)
[Encoder 1] CCW: -1 (sentido anti-horário)
```

## 💡 Exemplo de LEDs

Os LEDs atualmente mostram o estado dos encoders:
- **LED 1-4**: Correspondem aos Encoders 1-4
  - 🟢 **Verde**: Valor positivo (girado CW)
  - 🔴 **Vermelho**: Valor negativo (girado CCW)
  - 🔵 **Azul**: Valor zero (posição inicial)
- **LED 5**: Status do sistema (branco = ativo)

**Você pode customizar** a função dos LEDs editando `updateLEDs()` em `main_buttons.cpp`.

## 🛠️ Esquema de Ligação

### Matriz de Botões
```
         COL1  COL2  COL3  COL4  COL5
         GP1   GP2   GP3   GP4   GP5
          |     |     |     |     |
ROW1 GP6--●-----●-----●-----●-----●--
ROW2 GP7--●-----●-----●-----●-----●--
ROW3 GP8--●-----●-----●-----●-----●--
ROW4 GP9--●-----●-----●-----●-----●--
ROW5 GP10-●-----●-----●-----●-----●--

● = Botão (NO - Normally Open)
```

### Encoder (EC11)
```
   Encoder EC11 (A/B)
   ┌─────────────┐
   │   A    B    │
   │  GP11 GP12  │  (Encoder 1)
   │             │
   │   GND  VCC  │
   └─────────────┘
```

### LEDs WS2812B
```
ESP32-S3-Zero          Barra de LEDs
    GP45 ──────────────> DIN (LED1)
    GND  ──────────────> GND
    5V   ──────────────> VCC
```

## ⚙️ Customização

### Mudar Número de Botões/Encoders
Edite `main_buttons.cpp`:
```cpp
#define MATRIX_COLS 5  // Alterar conforme sua matriz
#define MATRIX_ROWS 5
#define NUM_ENCODERS 4 // Alterar número de encoders
#define LED_COUNT 5    // Alterar número de LEDs
```

### Debounce dos Botões
```cpp
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms (ajustar se necessário)
```

### Comportamento dos LEDs
Customize a função `updateLEDs()` para:
- Indicar TC/ABS ativo
- Mostrar marcha atual
- Indicar bandeiras
- Nível de combustível
- Qualquer outro dado do SimHub

## 📊 Especificações Técnicas

| Item | Valor |
|------|-------|
| Microcontrolador | ESP32-S3 (Dual Core 240MHz) |
| Flash | 4MB |
| RAM | 512KB SRAM |
| Botões Suportados | 25 (matriz 5x5) |
| Encoders | 4 rotativos |
| LEDs | 5 WS2812B + 1 onboard |
| Comunicação | USB CDC (Serial) |
| Velocidade Serial | 115200 baud |
| Scan Rate | ~1000 Hz |
| Latência | <2ms |

## 🔗 Links Úteis

- [SimHub Download](https://www.simhubdash.com/)
- [ESP32-S3-Zero Specs](https://wiki.waveshare.com/ESP32-S3-Zero)
- [NeoPixelBus Library](https://github.com/Makuna/NeoPixelBus)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

## 📝 Notas

- **Encoders**: Use encoders com detents para melhor feedback tátil
- **Botões**: Switches Cherry MX ou similar recomendados
- **Alimentação**: ESP32-S3-Zero alimentado via USB-C (5V)
- **LEDs**: Se usar mais de 10 LEDs, considere fonte externa 5V

## 🚀 Próximas Melhorias

- [ ] Suporte para encoder com botão (push)
- [ ] Profiles de LED customizáveis via SimHub
- [ ] Calibração automática de encoders
- [ ] Modo standalone (sem SimHub)
- [ ] Suporte para displays OLED (I2C)

---

**Desenvolvido para SimHub Racing Community** 🏎️
