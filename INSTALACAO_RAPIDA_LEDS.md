# 🚀 Guia Rápido - Instalação de LEDs WS2812B

## ⚡ Início Rápido (5 minutos)

### 1. Material Necessário
- ✅ Fita WS2812B com **21 LEDs** (ou corte de uma fita maior)
- ✅ Fonte de alimentação **5V/2A** (mínimo) ou **5V/3A** (recomendado)
- ✅ **3 fios** (vermelho, preto, amarelo ou cores similares)
- ✅ **Resistor 470Ω** (opcional, mas recomendado)
- ✅ **Capacitor 1000µF** (opcional, reduz ruído)
- ✅ Ferro de solda e solda
- ✅ WT32-SC01 PLUS já funcionando com SimHub

### 2. Conexões (2 minutos)

```
┌─────────────────────────────────────────────────┐
│                                                 │
│  WT32-SC01 PLUS                  Fita LED       │
│  ┌──────────┐                    ┌──────────┐  │
│  │          │                    │          │  │
│  │  GPIO 10 ├────────[470Ω]─────►│ DIN      │  │
│  │          │                    │          │  │
│  │  GND     ├────────────────────┤ GND      │  │
│  │          │                    │          │  │
│  └──────────┘                    │ VCC      │  │
│                                  └────┬─────┘  │
│                                       │         │
│  Fonte 5V/2A                          │         │
│  ┌──────────┐                         │         │
│  │  (+5V)   ├─────────────────────────┘         │
│  │          │                                   │
│  │  (GND)   ├───────────────────────────────────┤
│  └──────────┘                        ▲          │
│                                      │          │
│                    ATENÇÃO: GND comum entre     │
│                    ESP32 e Fonte!               │
└─────────────────────────────────────────────────┘
```

**Passos**:
1. Solde **fio amarelo/verde** no GPIO 10 do WT32-SC01 PLUS
2. Solde **resistor 470Ω** no fio (entre GPIO 10 e DIN da fita)
3. Solde o fio no **DIN** da fita LED (primeiro LED)
4. Conecte **GND** do ESP32 ao **GND** da fita LED
5. Conecte **+5V** da fonte no **VCC** da fita LED
6. Conecte **GND** da fonte ao **GND** da fita LED
7. **IMPORTANTE**: GND do ESP32 e GND da fonte devem estar conectados

### 3. Compilar e Carregar (2 minutos)

```powershell
# No terminal do VS Code (PowerShell):
cd D:\developer\projects\arduino\ESP-SimHub-ESP32S3-SCREEN
pio run -e wt32-sc01-plus -t upload
```

Aguarde a mensagem:
```
=============================== [SUCCESS] Took XX.XX seconds ===============================
```

### 4. Teste Rápido (1 minuto)

1. Desconecte o cabo USB do ESP32
2. Aguarde 5 segundos
3. Reconecte o cabo USB
4. **TODOS os 21 LEDs devem acender VERMELHO** por alguns segundos
5. Se sim, **PARABÉNS! Hardware está OK!** ✅

Se não acenderam:
- ❌ Verificar alimentação 5V (usar multímetro)
- ❌ Verificar conexão no GPIO 10
- ❌ Verificar polaridade da fita (DIN, não DOUT)
- ❌ Verificar GND comum entre ESP32 e fonte

## 📍 Pinout do WT32-SC01 PLUS

```
Vista Traseira (componentes)
┌────────────────────────────────────┐
│                                    │
│  Porta Expansão (EXT Headers)      │
│  ┌─────────────────────────────┐   │
│  │ GPIO 10 (EXT_IO1) ◄─ USE    │   │
│  │ GPIO 11 (EXT_IO2)           │   │
│  │ GPIO 12 (ocupado - display) │   │
│  │ GPIO 13 (ocupado - display) │   │
│  │ GPIO 14 (ocupado - display) │   │
│  │ GPIO 21 (ocupado - display) │   │
│  │ GND                          │   │
│  │ 3.3V                         │   │
│  └─────────────────────────────┘   │
│                                    │
│  USB-C (Porta de Dados)            │
│  ┌──────┐                          │
│  │      │                          │
│  └──────┘                          │
└────────────────────────────────────┘
```

**GPIO 10 (EXT_IO1)** é o pino de dados para os LEDs.

## 🎨 Layout dos LEDs

```
┌────────────────────────────────────────────────────────┐
│                                                        │
│  [1] [2] [3]    [4─5─6─7─8─9─10─11─12─13─14─15─16─17─18]    [19] [20] [21]  │
│    FLAGS             RPM METER (15 LEDs)                SPOTTER │
│  Bandeiras         Verde→Amarelo→Laranja→Vermelho      Direita │
│   Alertas                   + DRS                              │
│  Esquerda                                                      │
│                                                        │
└────────────────────────────────────────────────────────┘
```

### LEDs 1-3 (Esquerda)
- 🏁 **Bandeiras**: Verde, Amarela, Vermelha, Azul, Branca, Quadriculada
- 🚨 **Alertas**: ENGINE OFF, PIT LIMITER, LOW FUEL (pisca vermelho rápido)
- 💜 **Spotter**: Carro detectado à esquerda (magenta)

### LEDs 4-18 (Centro)
- 📊 **Barra de RPM**: 15 LEDs progressivos
  - 0-60%: 🟢 Verde
  - 60-80%: 🟡 Amarelo
  - 80-90%: 🟠 Laranja
  - 90-100%: 🔴 Vermelho (pisca ao atingir shift point)
- 🚀 **DRS**: Sobrepõe cores quando disponível/ativo (F1)
  - Disponível: 🟢 Verde sólido
  - Ativo: 🔵 Ciano sólido

### LEDs 19-21 (Direita)
- 💜 **Spotter**: Carro detectado à direita (magenta)
- 🔴 **Shift Light**: Pisca vermelho quando momento de trocar marcha

## 🧪 Teste com SimHub

### 1. Abrir SimHub
```
1. Inicie SimHub
2. Vá em Settings → Serial devices
3. Verifique se ESP32 está conectado (COMxx)
4. Status deve mostrar "Connected"
```

### 2. Entrar no Jogo
```
1. Abra qualquer jogo (AC, ACC, iRacing, F1, etc.)
2. Entre em uma sessão de teste/prática
3. Observe os LEDs:
   - LEDs 1-3: VERDE (bandeira verde)
   - LEDs 4-18: Devem acender conforme você acelera
   - LEDs 19-21: Apagados (sem spotter)
```

### 3. Teste de RPM
```
1. Pare o carro (neutro)
2. Acelere progressivamente
3. Observe LEDs do centro acenderem:
   - Baixo RPM: Verde
   - Médio RPM: Amarelo
   - Alto RPM: Laranja
   - Redline: Vermelho (+ LEDs direita piscam)
```

### 4. Teste de Bandeiras
```
1. Entre em corrida online
2. Aguarde situações diferentes:
   - Largada: Verde
   - Acidente: Amarela (pisca)
   - Safety car: Bandeira específica do jogo
```

## ⚙️ Personalização Rápida

### Ajustar Brilho
Edite `src/NeoPixelBusLEDs.h`, linha ~30:
```cpp
#define LUMINANCE_LIMIT 150  // 0-255
```
- **50-100**: Muito fraco, bom para ambientes escuros
- **150**: Padrão, balanceado (recomendado)
- **200**: Muito forte, exige fonte mais potente
- **255**: Máximo, pode sobrecarregar fonte 2A

### Desabilitar Teste no Boot
Edite `src/NeoPixelBusLEDs.h`, linha ~20:
```cpp
#define TEST_MODE 0  // 0 = desabilitado, 1 = habilitado
```

### Inverter Direção dos LEDs
Edite `src/NeoPixelBusLEDs.h`, linha ~17:
```cpp
#define RIGHTTOLEFT 1  // 1 = direita→esquerda, 0 = esquerda→direita
```

## 🐛 Problemas Comuns

### ❌ LEDs não acendem no boot
**Causa**: Problema de hardware
**Solução**:
1. Verificar alimentação 5V (medir com multímetro)
2. Verificar conexão GPIO 10 → DIN
3. Verificar polaridade da fita (DIN, não DOUT)
4. Verificar GND comum entre ESP32 e fonte

### ❌ Alguns LEDs não acendem
**Causa**: LED defeituoso ou conexão solta
**Solução**:
1. Verificar soldas na fita
2. Testar com outra fita
3. Substituir LED defeituoso

### ❌ Cores erradas/aleatórias
**Causa**: Ruído elétrico ou ordem de cores errada
**Solução**:
1. Adicionar resistor 470Ω no pino de dados
2. Adicionar capacitor 1000µF na alimentação
3. Usar cabo mais curto (< 30cm)
4. Verificar se é WS2812B (GRB), não WS2811 (RGB)

### ❌ LEDs piscam aleatoriamente
**Causa**: Ruído elétrico forte
**Solução**:
1. Adicionar capacitor 1000µF (470µF mínimo)
2. Usar fonte de melhor qualidade
3. Separar GND de potência do GND de sinal

### ❌ LEDs não respondem ao SimHub
**Causa**: Firmware não carregado ou SimHub não conectado
**Solução**:
1. Recarregar firmware: `pio run -e wt32-sc01-plus -t upload`
2. Verificar conexão no SimHub (Settings → Serial devices)
3. Verificar se custom protocol está configurado
4. Abrir Serial Monitor (115200 baud) e verificar dados

## 📊 Consumo de Corrente

| LEDs Acesos | Brilho 60% (150/255) | Brilho 100% (255/255) |
|-------------|----------------------|-----------------------|
| 0 (todos off) | 20 mA (ESP32 only) | 20 mA (ESP32 only) |
| 5 LEDs | ~150 mA | ~300 mA |
| 10 LEDs | ~300 mA | ~600 mA |
| 15 LEDs | ~450 mA | ~900 mA |
| 21 LEDs (todos) | ~630 mA | ~1260 mA |

**Recomendação de Fonte**:
- Brilho 60% (padrão): **5V/1A** suficiente
- Brilho 100%: **5V/2A** mínimo, **5V/3A** recomendado

## 📚 Documentação Completa

Para informações detalhadas, consulte:

1. **CONFIGURACAO_LEDS.md** - Documentação técnica completa
2. **TESTE_LEDS.md** - Guia de testes passo-a-passo (8 testes)
3. **RESUMO_IMPLEMENTACAO.md** - Detalhes da implementação

## ✅ Checklist de Instalação

Antes de considerar a instalação completa, verifique:

- [ ] Fita WS2812B com 21 LEDs preparada
- [ ] Fonte 5V/2A ou superior conectada
- [ ] GPIO 10 conectado ao DIN da fita (com resistor 470Ω)
- [ ] GND comum entre ESP32 e fonte
- [ ] Firmware compilado e carregado sem erros
- [ ] Boot test: Todos os 21 LEDs acendem vermelho
- [ ] SimHub conectado ao ESP32
- [ ] LEDs respondem ao RPM do carro
- [ ] Bandeiras funcionam (verde ao iniciar sessão)
- [ ] Sem flickering ou cores erradas
- [ ] Brilho ajustado conforme preferência

## 🎯 Resultado Esperado

Com tudo funcionando corretamente, você terá:

✅ **21 LEDs respondendo em tempo real**:
- Bandeiras do jogo nas laterais
- Barra de RPM progressiva no centro
- Alertas críticos piscando
- Spotter indicando carros ao lado
- DRS visível quando disponível (F1)
- Shift light no momento certo

✅ **Performance**:
- Latência < 50ms (imperceptível)
- 60 FPS de atualização
- Sincronizado com display
- Sem afetar responsividade

✅ **Visual Profissional**:
- Cores suaves e progressivas
- Efeitos de piscagem para alertas
- Integração perfeita com dashboard

## 📞 Suporte

Se tiver problemas:

1. ✅ Siga este guia passo-a-passo
2. ✅ Execute os testes do `TESTE_LEDS.md`
3. ✅ Verifique Serial Monitor (115200 baud)
4. ✅ Consulte seção de Troubleshooting

---

**Boa sorte com sua instalação! 🏁🚗💨**

---

**Versão**: 1.0  
**Data**: 2025-12-09  
**Compatibilidade**: WT32-SC01 PLUS + ESP32-S3
