# DEBUG MONITOR via ZXACC

## O Problema
Quando você tem o **SimHub conectado na COM11 (USB CDC)**, não consegue usar a mesma porta para monitorar logs de debug. A solução é usar a **UART1 hardware do ESP32-S3** via os pinos de debug do WT32-SC01 Plus.

## A Solução
O firmware agora foi configurado para enviar logs de debug via **UART1** nos pinos:
- **GPIO 17 (TXD)** → WT32-SC01 Plus Debug Interface Pin 3
- **GPIO 18 (RXD)** → WT32-SC01 Plus Debug Interface Pin 4

Isso permite monitorar logs em **tempo real** enquanto o SimHub está conectado!

## Como Conectar o ZXACC

Usando o pinout do Debug Interface da WT32-SC01 Plus:

```
Debug Interface Pinout:
Pin 1: +5V     → ZXACC Pin 1 (+5V)
Pin 2: +3.3V   → (referência, não usar)
Pin 3: ESP_TXD → ZXACC Pin 3 (RXD - recebe dados)
Pin 4: ESP_RXD → ZXACC Pin 4 (TXD - envia dados)
Pin 5: EN      → (não necessário)
Pin 6: BOOT    → (não necessário)
Pin 7: GND     → ZXACC Pin 7 (GND)
```

**Diagrama de Conexão:**
```
WT32-SC01 Plus          ZXACC Programmer
─────────────────────────────────────────
Pin 1 (+5V)    ────────→ Pin 1 (+5V)
Pin 3 (TXD)    ────────→ Pin 3 (RXD)
Pin 4 (RXD)    ────────→ Pin 4 (TXD)
Pin 7 (GND)    ────────→ Pin 7 (GND)
```

⚠️ **IMPORTANTE:** 
- TXD (transmissão) do ESP vai para RXD (recebimento) do ZXACC
- RXD (recebimento) do ESP vai para TXD (transmissão) do ZXACC

## Como Usar

### 1. Preparar o Hardware
1. Desconecte a WT32-SC01 Plus do SimHub (ou feche o SimHub)
2. Conecte o ZXACC aos pinos de Debug conforme o diagrama acima
3. Conecte o ZXACC ao seu PC via USB
4. Conecte a WT32-SC01 Plus novamente ao SimHub (COM11)

### 2. Identificar a Porta COM do ZXACC
1. Abra **Gerenciador de Dispositivos** (Device Manager)
2. Procure por "Portas (COM e LPT)"
3. Você deve ver:
   - **COM11** → WT32-SC01 Plus (SimHub USB CDC)
   - **COMx** → ZXACC (Debug Monitor)

Anote a porta COM do ZXACC (ex: COM5, COM6, etc.)

### 3. Executar o Monitor de Debug

#### Opção A: Listar portas disponíveis
```powershell
python debug_monitor_zxacc.py --list-ports
```

#### Opção B: Conectar e monitorar
```powershell
python debug_monitor_zxacc.py --port COM5
```

Substitua `COM5` pela porta que identificou do ZXACC.

#### Opção C: Especificar baud rate customizado
```powershell
python debug_monitor_zxacc.py --port COM5 --baud 115200
```

### 4. Resultado
Você verá logs em tempo real como:

```
[HH:MM:SS.mmm] [DEBUG UART1 Initialized]
[HH:MM:SS.mmm] [Monitoring via ZXACC on TXD/RXD pins]
[HH:MM:SS.mmm] >>> About to call shCustomProtocol.setup()
[HH:MM:SS.mmm] >>> shCustomProtocol.setup() completed
[HH:MM:SS.mmm] [main.loop] Received byte: 0x1E
[HH:MM:SS.mmm] [main.loop] MESSAGE_HEADER detected!
[HH:MM:SS.mmm] [main.loop] Command byte: P
[HH:MM:SS.mmm] [SHCustomProtocol.read()] First data packet received from SimHub!
[HH:MM:SS.mmm] [SHCustomProtocol] Alert: YELLOW FLAG
```

## Informações Importantes

### O que você vai ver nos logs?
- ✅ Inicialização do sistema
- ✅ Detecção de comandos SimHub
- ✅ Recebimento de dados de telemetria
- ✅ **Alertas e bandeiras** (YELLOW FLAG, BLUE FLAG, etc.)
- ✅ Mensagens de debug de cada módulo

### Troubleshooting

**Problema: "Nenhuma porta COM aparece para o ZXACC"**
- Verifique se o ZXACC está conectado ao PC
- Instale drivers do ZXACC se necessário
- Reinicie o PC
- Use Device Manager para verificar portas COM

**Problema: "Recebo lixo/caracteres inválidos"**
- Verifique se o baud rate está correto (deve ser 115200)
- Verifique se os fios TXD/RXD estão invertidos
- Tente usar `--baud 9600` para testar

**Problema: "Nenhum dado chega"**
- Verifique se SimHub está conectado (debug continua mesmo desconectado)
- Verifique se o firmware foi uploadado corretamente
- Confirme que GPIO17 e GPIO18 não estão sendo usados por outro módulo

### Voltando ao Serial USB

Se precisar voltar a monitorar via USB CDC (COM11), você pode:
1. Desconectar o ZXACC
2. Usar PlatformIO Device Monitor:
```powershell
pio device monitor --port COM11 --baud 115200
```

Mas lembre-se: quando SimHub estiver conectado, não conseguirá usar COM11!

## Próximos Passos

Agora você pode:
1. ✅ Executar o script `debug_monitor_zxacc.py` com o ZXACC conectado
2. ✅ Abrir SimHub conectando na COM11
3. ✅ Ver os logs de debug em tempo real na outra janela
4. ✅ Testar com replay do SimHub
5. ✅ Verificar se os alertas (flags) estão sendo detectados corretamente
