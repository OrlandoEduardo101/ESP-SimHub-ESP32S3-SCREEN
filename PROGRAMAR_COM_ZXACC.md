# Como Programar WT32-SC01 Plus usando ZXACC-ESPDB V2

## Visão Geral

O ZXACC-ESPDB V2 permite programar o WT32-SC01 Plus via interface de debug, enquanto o dispositivo permanece conectado ao PC via USB para comunicação com o SimHub.

## Conexões

### Interface de Debug do WT32-SC01 Plus

| Pino | Descrição | Conecte ao ZXACC-ESPDB V2 |
|------|-----------|---------------------------|
| 1    | +5V       | **NÃO CONECTE** (já alimentado via USB) |
| 2    | +3.3V     | **NÃO CONECTE** (apenas referência) |
| 3    | ESP_TXD   | **RX** do ZXACC-ESPDB V2 ⚠️ |
| 4    | ESP_RXD   | **TX** do ZXACC-ESPDB V2 ⚠️ |

**⚠️ IMPORTANTE sobre TX/RX:**
- Se o upload falhar com "Failed to write to target RAM", tente **INVERTER** TX e RX:
  - TX do ZXACC → ESP_TXD (pino 3)
  - RX do ZXACC → ESP_RXD (pino 4)
- A nomenclatura pode variar dependendo do fabricante do ZXACC
| 5    | EN        | **EN** ou **RST** do ZXACC-ESPDB V2 |
| 6    | BOOT      | **GPIO0** ou **BOOT** do ZXACC-ESPDB V2 |
| 7    | GND       | **GND** do ZXACC-ESPDB V2 |

### ⚠️ IMPORTANTE - Alimentação

**NÃO conecte o +5V do ZXACC-ESPDB V2 ao WT32!**

O WT32 já está sendo alimentado via USB do PC. Conectar +5V de duas fontes pode danificar o dispositivo.

Conecte apenas:
- **TX/RX** (comunicação serial)
- **EN** (reset/enable)
- **BOOT/GPIO0** (modo bootloader)
- **GND** (terra comum)

## Passo a Passo

### 1. Preparação

1. **WT32 conectado ao PC via USB** (para alimentação e comunicação SimHub)
2. **ZXACC-ESPDB V2 conectado ao Mac via USB**
3. **Conecte os pinos de debug** conforme tabela acima

### 2. Verificar Porta Serial no Mac

```bash
pio device list
```

Você deve ver algo como:
```
/dev/cu.usbserial-XXXXX  (ZXACC-ESPDB V2)
/dev/cu.usbmodemXXXXX    (WT32, se conectado)
```

### 3. Configurar platformio.ini

Adicione uma nova configuração de ambiente para usar o ZXACC-ESPDB V2:

```ini
[env:wt32-sc01-plus-debug]
platform = espressif32@^6.9
board = esp32-s3-devkitc-1
framework = arduino
board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_upload.flash_size = 16MB
board_build.usb_mode = cdc
build_flags = -DBOARD_HAS_PSRAM -w -DESP32=true -DUSE_USB_CDC=true

lib_deps =
    moononournation/GFX Library for Arduino@1.4.0
	locoduino/RingBuffer@^1.0.4
    https://github.com/paulo-raca/ArduinoBufferedStreams.git#5e3a1a3d140955384a07878c64808e77fa2a7521
    https://github.com/khoih-prog/ESPAsync_WiFiManager
    makuna/NeoPixelBus @ ^2.8

board_build.arduino.upstream_packages = no
monitor_speed = 115200
upload_speed = 921600
; Especifique a porta do ZXACC-ESPDB V2 aqui
; upload_port = /dev/cu.usbserial-XXXXX
```

### 4. Fazer Upload

#### Opção A: Especificar porta manualmente

```bash
pio run -e wt32-sc01-plus-debug -t upload --upload-port /dev/cu.usbserial-XXXXX
```

Substitua `XXXXX` pela porta real do seu ZXACC-ESPDB V2.

#### Opção B: Usar script atualizado

Crie um script `upload-debug.sh`:

```bash
#!/bin/bash

set -e

ENV="wt32-sc01-plus-debug"

echo "🔨 Compilando firmware para WT32-SC01 Plus (via ZXACC-ESPDB V2)..."
echo ""

# Tentar encontrar o comando pio
PIO_CMD=""
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
elif command -v platformio &> /dev/null; then
    PIO_CMD="platformio"
elif [ -f "$HOME/.platformio/penv/bin/pio" ]; then
    PIO_CMD="$HOME/.platformio/penv/bin/pio"
fi

if [ -z "$PIO_CMD" ]; then
    echo "❌ Erro: PlatformIO não encontrado!"
    exit 1
fi

echo "📋 Verificando dispositivos conectados..."
${PIO_CMD} device list
echo ""

echo "🔨 Compilando..."
${PIO_CMD} run -e ${ENV}

echo ""
echo "✅ Compilação concluída!"
echo ""

# Perguntar pela porta do ZXACC-ESPDB V2
echo "📌 Selecione a porta do ZXACC-ESPDB V2 (geralmente /dev/cu.usbserial-*):"
read -p "Porta: " DEBUG_PORT

if [ -z "$DEBUG_PORT" ]; then
    echo "❌ Porta não especificada!"
    exit 1
fi

echo ""
echo "📤 Fazendo upload via ZXACC-ESPDB V2..."
${PIO_CMD} run -e ${ENV} -t upload --upload-port ${DEBUG_PORT}

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Upload concluído com sucesso!"
else
    echo ""
    echo "❌ Erro no upload!"
    echo "   Dica: Verifique as conexões e tente entrar em modo bootloader"
    exit 1
fi
```

Torne o script executável:
```bash
chmod +x upload-debug.sh
```

## Modo Bootloader Manual

Se o upload falhar, você pode entrar em modo bootloader manualmente:

1. **Mantenha o pino BOOT (GPIO0) em GND** (ou conecte ao pino BOOT do ZXACC)
2. **Pressione e solte o pino EN** (reset)
3. **Solte o pino BOOT**
4. **Tente o upload novamente**

## Vantagens desta Configuração

✅ **Não precisa desconectar do PC** - WT32 permanece conectado ao SimHub
✅ **Programação rápida** - Via interface de debug dedicada
✅ **Alimentação estável** - WT32 alimentado via USB do PC
✅ **Comunicação simultânea** - Pode manter comunicação SimHub enquanto programa

## Troubleshooting

### Erro: "Failed to connect to ESP32"
- Verifique se todas as conexões estão corretas
- Confirme que o ZXACC-ESPDB V2 está conectado ao Mac
- Tente entrar em modo bootloader manualmente

### Erro: "Port not found"
- Execute `pio device list` para ver portas disponíveis
- Verifique se o ZXACC-ESPDB V2 está conectado
- No Mac, portas geralmente são `/dev/cu.usbserial-*` ou `/dev/tty.usbserial-*`

### Upload trava ou falha
- Verifique se o WT32 não está em uso pelo SimHub (feche o SimHub temporariamente)
- Tente reduzir a velocidade de upload: `upload_speed = 460800`
- Entre em modo bootloader manualmente antes do upload

## Notas Importantes

⚠️ **Nunca conecte +5V do ZXACC ao WT32** - Pode danificar o dispositivo
⚠️ **Sempre conecte GND** - Necessário para referência comum
⚠️ **Feche o SimHub durante o upload** - Evita conflitos de porta serial

