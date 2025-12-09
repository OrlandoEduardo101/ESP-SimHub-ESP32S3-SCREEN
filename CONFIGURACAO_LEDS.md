# Configuração de LEDs WS2812B - WT32-SC01 PLUS

## 📋 Visão Geral

Este documento descreve a configuração de **21 LEDs WS2812B** conectados ao WT32-SC01 PLUS via porta de expansão GPIO.

## 🔌 Conexão Física

- **Pino de Dados**: GPIO 10 (EXT_IO1)
- **Total de LEDs**: 21 LEDs WS2812B
- **Alimentação**: Fonte externa 5V recomendada (LEDs consomem até ~1.2A @ 100% brilho)

### ⚠️ Avisos Importantes

1. **Limite de Brilho**: Configurado em 150/255 (60%) para proteger a fonte de alimentação
2. **Consumo de Corrente**: Cada LED consome ~60mA @ brilho máximo
3. **Fonte Externa**: Recomendado usar fonte 5V/2A dedicada para os LEDs
4. **GPIO Limitado**: ESP32-S3 não suporta RMT, usa BitBang (compatível com GPIO < 32)

## 🎨 Layout dos LEDs (Esquerda → Direita)

```
┌─────────────────────────────────────────────────────────────┐
│  [1][2][3]  [4-5-6-7-8-9-10-11-12-13-14-15-16-17-18]  [19][20][21]  │
│   FLAGS      ←──────────── RPM METER ────────────→   SPOTTER │
└─────────────────────────────────────────────────────────────┘
```

### LEDs 1-3 (Índices 0-2): BANDEIRAS & ALERTAS ESQUERDA
**Prioridade de Exibição**:
1. **Alertas Críticos** (Máxima Prioridade)
   - Pisca rápido VERMELHO (250ms)
   - Ativa quando `alertMessage` não é vazio/NORMAL
   - Exemplos: "ENGINE OFF", "PIT LIMITER", "LOW FUEL"

2. **Spotter Esquerdo**
   - MAGENTA sólido quando carro detectado à esquerda
   - Ativa quando `spotterLeft = "1"`

3. **Bandeiras** (Menor Prioridade)
   - 🟢 **Verde**: Largada/Relargada
   - 🟡 **Amarela**: Perigo à frente (pisca)
   - 🔴 **Vermelha**: Sessão parada
   - 🔵 **Azul**: Sendo ultrapassado
   - ⚪ **Branca**: Carro lento à frente
   - 🏁 **Quadriculada**: Fim da corrida/sessão (pisca)
   - ⚫ **Preta**: Penalidade/Desqualificação

### LEDs 4-18 (Índices 3-17): RPM METER - 15 LEDs
**Barra Progressiva de RPM**:
- Acende proporcionalmente ao `rpmPercent` (0-100%)
- Cores progressivas baseadas em segmentos:

| RPM %     | Cor          | Descrição              |
|-----------|--------------|------------------------|
| 0-60%     | 🟢 Verde     | RPM baixo/econômico    |
| 60-80%    | 🟡 Amarelo   | RPM médio/otimizado    |
| 80-90%    | 🟠 Laranja   | RPM alto/performance   |
| 90-100%   | 🔴 Vermelho  | Redline/Shift point    |

**Modo DRS** (Substitui cores de RPM):
- Quando `drsAvailable = "1"`: 🟢 **Verde** (DRS disponível)
- Quando `drsActive = "1"`: 🔵 **Ciano** (DRS ativo)

**Shift Light**:
- Quando `shiftLightTrigger = "1"`: Pisca VERMELHO rápido (100ms)
- Redline ativa: LEDs vermelhos piscam

### LEDs 19-21 (Índices 18-20): SPOTTER & AVISOS DIREITA
**Prioridade de Exibição**:
1. **Spotter Direito** (Máxima Prioridade)
   - MAGENTA sólido quando carro detectado à direita
   - Ativa quando `spotterRight = "1"`

2. **Shift Light** (Quando sem spotter)
   - VERMELHO piscante (100ms) quando `shiftLightTrigger = "1"`
   - Indica momento ideal de troca de marcha

## 📊 Dados de Telemetria Utilizados

Os LEDs recebem os seguintes dados do SimHub via serial:

```cpp
// RPM e Performance
int rpmPercent              // Porcentagem do RPM (0-100)
int rpmRedLineSetting       // Limite de redline (0-100)
String shiftLightTrigger    // Gatilho de troca de marcha ("0"/"1")

// Bandeiras
String currentFlag          // Bandeira atual ("Green", "Yellow", "Red", etc.)

// Spotter
String spotterLeft          // Carro à esquerda ("0"/"1")
String spotterRight         // Carro à direita ("0"/"1")

// DRS (Drag Reduction System)
String drsAvailable         // DRS disponível ("0"/"1")
String drsActive            // DRS ativo ("0"/"1")

// Alertas
String alertMessage         // Mensagem de alerta crítico
```

## ⚙️ Configuração no Código

### Arquivo: `src/NeoPixelBusLEDs.h`

#### Definições Principais
```cpp
#define LED_COUNT 21                // Total de LEDs
#define DATA_PIN 10                 // GPIO 10 (EXT_IO1)
#define LUMINANCE_LIMIT 150         // Brilho máximo (0-255)
#define TEST_MODE 1                 // 1 = Teste vermelho no boot
#define RIGHTTOLEFT 0               // 0 = Esquerda→Direita
```

#### Método ESP32-S3
```cpp
#define method NeoEsp32BitBangWs2812xMethod  // BitBang para ESP32-S3
#define colorSpec NeoGrbFeature              // GRB para WS2812B
```

### Integração com SHCustomProtocol

A função `updateCustomLEDs()` é chamada automaticamente no `loop()`:

```cpp
void loop() {
    // ... código de atualização do display ...
    
    #ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
    updateCustomLEDs(
        rpmPercent,
        rpmRedLineSetting,
        currentFlag,
        spotterLeft,
        spotterRight,
        drsAvailable,
        drsActive,
        alertMessage,
        shiftLightTrigger == "1"
    );
    #endif
}
```

## 🔧 Personalização

### Alterar Cores
Edite as definições de cores em `NeoPixelBusLEDs.h`:

```cpp
#define COLOR_FLAG_YELLOW RgbColor(255, 255, 0)
#define COLOR_SPOTTER RgbColor(255, 0, 255)
#define COLOR_DRS_ACTIVE RgbColor(0, 200, 255)
// ... etc
```

### Alterar Brilho
```cpp
#define LUMINANCE_LIMIT 150  // 0-255 (recomendado: 100-150)
```

### Alterar Pino de Dados
```cpp
#define DATA_PIN 10  // Troque para GPIO 11 se necessário
```

### Inverter Ordem dos LEDs
```cpp
#define RIGHTTOLEFT 1  // 1 = Direita→Esquerda, 0 = Esquerda→Direita
```

## 🧪 Modo de Teste

Com `TEST_MODE 1`, todos os LEDs acendem VERMELHOS no boot:
- Confirma que a fita está funcionando
- Valida conexões
- Testa alimentação

Para desabilitar:
```cpp
#define TEST_MODE 0
```

## 📐 Segmentos de RPM Detalhados

A barra de RPM divide-se em 15 segmentos:

| LEDs      | RPM Range | Cor          | Aplicação                    |
|-----------|-----------|--------------|------------------------------|
| 4-6       | 0-20%     | 🟢 Verde     | Idle/Pit lane                |
| 7-9       | 20-40%    | 🟢 Verde     | Cruzeiro/Economia            |
| 10-11     | 40-60%    | 🟢 Verde     | Range otimizado              |
| 12-13     | 60-80%    | 🟡 Amarelo   | Alta performance             |
| 14-15     | 80-90%    | 🟠 Laranja   | Próximo do limite            |
| 16-18     | 90-100%   | 🔴 Vermelho  | Redline/Shift point          |

## 🎯 Exemplos de Comportamento

### Cenário 1: Corrida Normal
```
Estado: RPM 85%, Bandeira Verde, Sem spotter
LEDs 1-3:    🟢🟢🟢 (Verde - bandeira)
LEDs 4-18:   🟢🟢🟢🟡🟡🟠🟠🟠⚫⚫⚫⚫⚫⚫⚫ (RPM 85%)
LEDs 19-21:  ⚫⚫⚫ (Apagados)
```

### Cenário 2: Ultrapassagem com Spotter
```
Estado: RPM 70%, Carro à esquerda e direita
LEDs 1-3:    💜💜💜 (Magenta - spotter left)
LEDs 4-18:   🟢🟢🟢🟢🟡🟡🟡⚫⚫⚫⚫⚫⚫⚫⚫ (RPM 70%)
LEDs 19-21:  💜💜💜 (Magenta - spotter right)
```

### Cenário 3: DRS Ativo + Redline
```
Estado: RPM 95%, DRS ativo, Shift light
LEDs 1-3:    🟢🟢🟢 (Verde - corrida normal)
LEDs 4-18:   🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵 (Ciano - DRS ativo)
LEDs 19-21:  🔴⚫🔴 (Vermelho piscante - shift)
```

### Cenário 4: Bandeira Amarela + Alerta
```
Estado: Bandeira amarela, Alerta crítico "LOW FUEL"
LEDs 1-3:    🔴⚫🔴 (Vermelho piscante - alerta override)
LEDs 4-18:   [RPM normal]
LEDs 19-21:  ⚫⚫⚫ (Apagados)
```

## 🐛 Troubleshooting

### LEDs Não Acendem
1. Verificar conexão no GPIO 10
2. Confirmar alimentação 5V externa
3. Verificar `TEST_MODE 1` - deve acender vermelho no boot
4. Medir voltagem: Data deve estar ~3.3V, VCC em 5V

### Cores Erradas
1. Verificar `#define colorSpec` - deve ser `NeoGrbFeature` para WS2812B
2. Alguns clones usam `NeoRgbFeature` - testar alternativa

### Piscando Aleatoriamente
1. Adicionar resistor 470Ω no pino de dados
2. Adicionar capacitor 1000µF na alimentação
3. Usar cabo curto (< 30cm) entre ESP32 e primeiro LED

### Baixo Brilho
```cpp
#define LUMINANCE_LIMIT 255  // Máximo (cuidado com corrente!)
```

### Atraso na Atualização
- BitBang tem overhead de CPU
- Evitar `delay()` no código
- LEDs atualizam a cada frame (~60 FPS)

## 📚 Referências

- [NeoPixelBus Library](https://github.com/Makuna/NeoPixelBus)
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [WT32-SC01 Plus Pinout](https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32)
- [Adafruit NeoPixel Power Guide](https://learn.adafruit.com/adafruit-neopixel-uberguide/powering-neopixels)

## 🔄 Changelog

### v1.0 (2025-12-09)
- ✅ Configuração inicial de 21 LEDs WS2812B
- ✅ GPIO 10 (BitBang para ESP32-S3)
- ✅ LEDs 1-3: Bandeiras + Alertas + Spotter Left
- ✅ LEDs 4-18: RPM Meter (15 LEDs) + DRS
- ✅ LEDs 19-21: Spotter Right + Shift Light
- ✅ Integração completa com SHCustomProtocol
- ✅ Suporte a todas as bandeiras FIA
- ✅ Priorização de alertas críticos
- ✅ Efeitos de piscagem (bandeiras, shift light, alertas)
