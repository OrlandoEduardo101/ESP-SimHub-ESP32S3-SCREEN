# 🧪 Guia de Teste - LEDs WS2812B (21 LEDs)

## 📋 Pré-requisitos

Antes de testar, certifique-se de ter:
- ✅ 21 LEDs WS2812B conectados ao GPIO 10
- ✅ Fonte de alimentação 5V/2A para os LEDs
- ✅ Firmware compilado e carregado no WT32-SC01 PLUS
- ✅ SimHub instalado e configurado

## 🔌 Conexões Físicas

### Diagrama de Conexão
```
WT32-SC01 PLUS (GPIO 10)  ───────────────►  LED Strip DIN
                                             (Data Input)

Fonte 5V (+)  ─────────────────────────────►  LED Strip VCC
Fonte 5V (-)  ─────────────────────────────►  LED Strip GND
ESP32 GND     ─────────────────────────────►  LED Strip GND
                                             (Comum)
```

### ⚠️ Importante
1. **GND Comum**: ESP32 e fonte de alimentação devem compartilhar o mesmo GND
2. **Resistor Série**: Adicionar resistor 470Ω entre GPIO 10 e DIN (opcional, mas recomendado)
3. **Capacitor**: Adicionar capacitor 1000µF entre VCC e GND da fita (opcional, reduz ruído)

## 🧪 Teste 1: Boot Test (Teste de Inicialização)

### Objetivo
Verificar se todos os 21 LEDs acendem corretamente ao ligar o ESP32.

### Procedimento
1. Desconecte o SimHub (feche o programa)
2. Desconecte o cabo USB do ESP32
3. Aguarde 5 segundos
4. Reconecte o cabo USB
5. Observe o boot

### ✅ Resultado Esperado
```
Boot Sequence:
┌─────────────────────────────────────────────────┐
│ T+0s:   Display mostra tela de loading          │
│ T+1s:   Todos os 21 LEDs acendem VERMELHO       │
│         [🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴🔴] │
│ T+2s:   LEDs permanecem vermelhos               │
│ T+3s:   Display mostra "Aguardando SimHub..."   │
│ T+4s:   LEDs ainda vermelhos (aguardando dados) │
└─────────────────────────────────────────────────┘
```

### 🐛 Se falhar:
- **Nenhum LED acende**: Verificar alimentação 5V e conexão no GPIO 10
- **Alguns LEDs não acendem**: LED defeituoso ou conexão solta na fita
- **LEDs em cores erradas**: Verificar `colorSpec` em `NeoPixelBusLEDs.h` (deve ser `NeoGrbFeature`)

## 🧪 Teste 2: Conexão com SimHub

### Objetivo
Verificar se os LEDs respondem aos dados do SimHub.

### Procedimento
1. Abra o SimHub
2. Vá em **Settings → Serial devices**
3. Conecte ao ESP32 (porta COMxx)
4. Vá em **Dash Studio**
5. Selecione qualquer jogo (AC, ACC, iRacing, F1, etc.)
6. Entre em uma sessão de teste/prática

### ✅ Resultado Esperado
```
Ao entrar no jogo:
┌─────────────────────────────────────────────────┐
│ LEDs 1-3:    Devem acender VERDE (bandeira      │
│              verde de largada)                   │
│ LEDs 4-18:   Devem começar a acender conforme   │
│              você acelera o carro               │
│ LEDs 19-21:  Devem permanecer apagados          │
└─────────────────────────────────────────────────┘
```

### 🐛 Se falhar:
- **LEDs permanecem vermelhos**: SimHub não está enviando dados → Verificar configuração Custom Serial
- **LEDs apagam**: Problema de comunicação → Verificar cabo USB e porta COM
- **LEDs piscam aleatoriamente**: Ruído elétrico → Adicionar resistor/capacitor

## 🧪 Teste 3: RPM Meter (Aceleração)

### Objetivo
Testar a barra de RPM progressiva (LEDs 4-18).

### Procedimento
1. Inicie uma sessão no jogo
2. Pare o carro (neutro, freio de mão)
3. Acelere progressivamente (sem trocar marcha)
4. Observe os LEDs do centro

### ✅ Resultado Esperado
```
RPM 0%:     ⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫ (Todos apagados)
RPM 30%:    🟢🟢🟢🟢🟢⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫ (5 LEDs verdes)
RPM 60%:    🟢🟢🟢🟢🟢🟢🟢🟢🟢⚫⚫⚫⚫⚫⚫ (9 LEDs verdes)
RPM 75%:    🟢🟢🟢🟢🟢🟢🟢🟢🟢🟡🟡🟡⚫⚫⚫ (9V + 3A)
RPM 85%:    🟢🟢🟢🟢🟢🟢🟢🟢🟢🟡🟡🟡🟠🟠⚫ (9V + 3A + 2L)
RPM 95%:    🟢🟢🟢🟢🟢🟢🟢🟢🟢🟡🟡🟡🟠🟠🔴 (9V + 3A + 2L + 1R)
RPM 100%:   🟢🟢🟢🟢🟢🟢🟢🟢🟢🟡🟡🟡🟠🟠🔴 (Todos acesos)
            + LEDs 19-21 PISCAM VERMELHO (shift light)
```

### 🐛 Se falhar:
- **LEDs não sobem**: Verificar `rpmPercent` no serial monitor
- **Cores erradas**: Ajustar limiares em `updateCustomLEDs()`
- **Shift light não pisca**: Verificar `shiftLightTrigger` nos dados

## 🧪 Teste 4: Bandeiras (Flags)

### Objetivo
Testar se os LEDs 1-3 respondem às bandeiras do jogo.

### Procedimento
1. Entre em uma corrida online ou campeonato
2. Aguarde diferentes situações de bandeira
3. Observe os LEDs da esquerda (1-3)

### ✅ Resultado Esperado
```
Bandeira Verde:        🟢🟢🟢 (Verde sólido)
Bandeira Amarela:      🟡⚫🟡 (Amarelo piscante - 500ms)
Bandeira Vermelha:     🔴🔴🔴 (Vermelho sólido)
Bandeira Azul:         🔵🔵🔵 (Azul sólido)
Bandeira Branca:       ⚪⚪⚪ (Branco sólido)
Bandeira Quadriculada: ⚪⚫⚪ (Branco/cinza piscante)
Bandeira Preta:        ⚫⚫⚫ (Preto/cinza escuro)
```

### 🐛 Se falhar:
- **LEDs não mudam**: Verificar `currentFlag` no serial monitor
- **Piscam errado**: Ajustar `millis() / 500` para velocidade diferente
- **Cores incorretas**: Verificar definições `COLOR_FLAG_*`

## 🧪 Teste 5: Spotter (Carros Próximos)

### Objetivo
Testar se os LEDs indicam carros ao lado durante ultrapassagens.

### Procedimento
1. Entre em uma corrida online
2. Posicione-se lado a lado com outro carro
3. Observe os LEDs laterais

### ✅ Resultado Esperado
```
Carro à ESQUERDA:      💜💜💜 ⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫ ⚫⚫⚫
                       └─────┘                  └─────┘
                       Spotter                  Normal
                       
Carro à DIREITA:       ⚫⚫⚫ ⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫ 💜💜💜
                       └─────┘                  └─────┘
                       Normal                   Spotter
                       
Carros DOS DOIS LADOS: 💜💜💜 ⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫⚫ 💜💜💜
                       └─────┘                  └─────┘
                       Spotter                  Spotter
```

### 🐛 Se falhar:
- **LEDs não acendem**: Verificar se o jogo suporta spotter
- **Sempre magenta**: Verificar `spotterLeft`/`spotterRight` no serial
- **Atraso**: Normal, spotter tem delay de ~100-200ms

## 🧪 Teste 6: DRS (Drag Reduction System)

### Objetivo
Testar indicação de DRS disponível/ativo (F1, alguns mods de AC/ACC).

### Procedimento
1. Use um carro com DRS (F1 2024, F1 2023, etc.)
2. Complete 2 voltas para ativar DRS
3. Entre na zona de DRS
4. Observe os LEDs centrais (4-18)

### ✅ Resultado Esperado
```
SEM DRS:         🟢🟢🟢🟡🟡🟠🟠🔴⚫⚫⚫⚫⚫⚫⚫
                 (Cores normais baseadas em RPM)

DRS DISPONÍVEL:  🟢🟢🟢🟢🟢🟢🟢🟢🟢🟢🟢🟢🟢🟢🟢
                 (Todos VERDE - DRS pronto para ativar)

DRS ATIVO:       🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵
                 (Todos CIANO - DRS aberto)
```

### 🐛 Se falhar:
- **LEDs não mudam**: Verificar se o jogo suporta DRS
- **Sempre verde**: `drsActive` não está sendo enviado
- **DRS não funciona**: Jogo pode não ter suporte (AC, ACC básico, etc.)

## 🧪 Teste 7: Alertas Críticos

### Objetivo
Testar se alertas críticos sobrepõem bandeiras nos LEDs 1-3.

### Procedimento
1. Entre no jogo
2. Ative o Pit Limiter
3. Observe os LEDs da esquerda

### ✅ Resultado Esperado
```
PIT LIMITER ATIVO:
LEDs 1-3: 🔴⚫🔴 (Vermelho piscante rápido - 250ms)

Display também mostra:
┌────────────────────────────┐
│                            │
│     🔴 PIT LIMITER 🔴      │
│                            │
└────────────────────────────┘
```

### Outros Alertas para Testar
```
LOW FUEL:      🔴⚫🔴 (Vermelho piscante)
ENGINE OFF:    🔴⚫🔴 (Vermelho piscante)
PENALTY:       🔴⚫🔴 (Vermelho piscante)
```

### 🐛 Se falhar:
- **LEDs não piscam**: Verificar `alertMessage` no serial
- **Pisca muito lento**: Ajustar `millis() / 250` para velocidade maior
- **Não sobrepõe bandeira**: Verificar ordem de prioridade em `updateCustomLEDs()`

## 🧪 Teste 8: Desempenho e Latência

### Objetivo
Verificar se os LEDs respondem em tempo real sem atrasos.

### Procedimento
1. Entre no jogo
2. Acelere e desacelere rapidamente
3. Observe se os LEDs acompanham o RPM

### ✅ Resultado Esperado
- Latência: < 50ms
- LEDs devem subir/descer instantaneamente com RPM
- Sem "lagging" ou "stuttering"
- 60 FPS de atualização (sincronizado com display)

### 🐛 Se falhar:
- **LEDs com atraso**: Possível sobrecarga do CPU → Reduzir `LUMINANCE_LIMIT`
- **Piscam/Flickering**: Ruído elétrico → Adicionar capacitor 1000µF
- **Lentidão**: Verificar taxa de atualização do SimHub (deve ser 60Hz)

## 📊 Monitor Serial - Verificação de Dados

Para depurar problemas, abra o **Serial Monitor** (115200 baud):

```
========== TOUCH INITIALIZATION START ==========
Display init OK
Aguardando SimHub...

[RAW] packet: 0;N;50;90;...;Green;0;0;NORMAL;...
[SHCustomProtocol] Flag value received [39]: 'Green'
[SHCustomProtocol] Alert [42]: 'NORMAL'
```

### Dados Importantes para LEDs
```
rpmPercent:           [2]  - Deve variar de 0-100
currentFlag:          [39] - "Green", "Yellow", "Red", etc.
spotterLeft:          [45] - "0" ou "1"
spotterRight:         [46] - "0" ou "1"
drsAvailable:         [58] - "0" ou "1"
drsActive:            [59] - "0" ou "1"
shiftLightTrigger:    [57] - "0" ou "1"
alertMessage:         [42] - "" ou "PIT LIMITER", etc.
```

## 🔧 Troubleshooting Avançado

### Problema: LEDs com cores aleatórias
**Causa**: Ruído elétrico ou timing incorreto
**Solução**:
1. Adicionar resistor 470Ω no pino de dados
2. Adicionar capacitor 1000µF na alimentação
3. Usar cabo blindado para dados
4. Reduzir comprimento do cabo (< 30cm entre ESP32 e LED 1)

### Problema: Primeiro LED sempre aceso
**Causa**: Cabo muito longo ou sem resistor
**Solução**:
- Adicionar resistor 470Ω entre GPIO 10 e DIN
- Reduzir comprimento do cabo

### Problema: LEDs param de responder após alguns minutos
**Causa**: Sobrecarga térmica ou fonte insuficiente
**Solução**:
1. Reduzir brilho: `#define LUMINANCE_LIMIT 100`
2. Usar fonte 5V/3A (ao invés de 2A)
3. Adicionar dissipador de calor nos LEDs

### Problema: LEDs funcionam, mas display trava
**Causa**: BitBang consome muito CPU
**Solução**:
- Isso é esperado com ESP32-S3 (não tem RMT)
- LEDs atualizam a ~60 FPS, o que é aceitável
- Se necessário, reduzir taxa de atualização

## ✅ Checklist Final

Antes de considerar o teste completo, verifique:

- [ ] Todos os 21 LEDs acendem no boot (vermelho)
- [ ] LEDs respondem ao RPM do carro
- [ ] Cores progressivas (verde → amarelo → laranja → vermelho)
- [ ] Shift light pisca no redline (LEDs 19-21)
- [ ] Bandeiras aparecem corretamente (LEDs 1-3)
- [ ] Spotter funciona dos dois lados (magenta)
- [ ] DRS muda cores quando disponível/ativo (F1)
- [ ] Alertas críticos piscam em vermelho (LEDs 1-3)
- [ ] Sem atraso perceptível (< 50ms)
- [ ] Sem flickering ou cores erradas
- [ ] Display continua funcionando normalmente
- [ ] SimHub se conecta sem problemas

## 📸 Documentação Visual

### Fotos Recomendadas
Tire fotos dos LEDs nos seguintes estados para documentar:

1. Boot test (todos vermelhos)
2. RPM baixo (verde)
3. RPM médio (amarelo)
4. RPM alto + shift light (vermelho piscante)
5. Bandeira amarela (amarelo piscante)
6. Spotter ativo (magenta)
7. DRS ativo (ciano)
8. Alerta crítico (vermelho piscante)

### Vídeos Recomendados
Grave vídeos curtos (10-15s) mostrando:

1. Boot sequence completa
2. Aceleração progressiva (RPM 0→100%)
3. Ultrapassagem com spotter
4. Ativação de DRS (se aplicável)
5. Diferentes bandeiras

## 🎓 Próximos Passos

Após validar todos os testes:

1. **Personalização**: Ajuste cores, brilho e efeitos conforme preferência
2. **Montagem**: Fixe a fita de LEDs no case/cockpit
3. **Fiação**: Organize cabos e conexões
4. **Otimização**: Ajuste limiar de RPM para cada carro
5. **Backup**: Documente configuração funcionando

## 📞 Suporte

Se encontrar problemas não cobertos neste guia:

1. Verifique o Serial Monitor (115200 baud)
2. Revise o arquivo `CONFIGURACAO_LEDS.md`
3. Teste com `TEST_MODE 1` para isolar hardware
4. Consulte logs de debug no UART0 (COM12 via ZXACC)

---

**Data do Documento**: 2025-12-09  
**Versão**: 1.0  
**Autor**: ESP-SimHub-ESP32S3-SCREEN Project
