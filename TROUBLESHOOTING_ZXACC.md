# Troubleshooting - Upload via ZXACC-ESPDB V2

## Erro: "Failed to write to target RAM"

Este erro geralmente indica que o ESP32-S3 não está em modo bootloader ou há problema nas conexões.

## Soluções

### 1. Verificar se a Porta Está Correta

A porta que você selecionou pode ser do WT32, não do ZXACC-ESPDB V2.

**Como identificar:**
- **ZXACC-ESPDB V2**: Geralmente `/dev/cu.usbserial-*` ou `/dev/tty.usbserial-*`
- **WT32**: Geralmente `/dev/cu.usbmodem*` ou `/dev/tty.usbmodem*`

**Teste:**
1. Desconecte o WT32 do PC
2. Conecte apenas o ZXACC-ESPDB V2 ao Mac
3. Execute `pio device list`
4. A porta que aparecer será a do ZXACC

### 2. Verificar Conexões do ZXACC-ESPDB V2

Certifique-se de que todas as conexões estão corretas:

```
ZXACC-ESPDB V2          →    WT32-SC01 Plus (Interface Debug)
─────────────────────────────────────────────────────────────
TX                      →    ESP_RXD (pino 4)
RX                      →    ESP_TXD (pino 3)
EN / RST                →    EN (pino 5)
BOOT / GPIO0            →    BOOT (pino 6)
GND                     →    GND (pino 7)

⚠️ NÃO CONECTE +5V!
```

### 3. Entrar em Modo Bootloader Manualmente

Antes de fazer o upload:

1. **Mantenha o pino BOOT (GPIO0) em GND** (ou conecte ao pino BOOT do ZXACC)
2. **Pressione e solte o pino EN** (reset) - isso reinicia o ESP32
3. **Solte o pino BOOT** - agora o ESP32 está em modo bootloader
4. **Imediatamente execute o upload** (tempo limitado)

**Dica**: Se o ZXACC-ESPDB V2 tiver botões, use-os:
- Segure BOOT
- Pressione RESET
- Solte BOOT
- Faça upload

### 4. Reduzir Velocidade de Upload

Se ainda não funcionar, tente reduzir a velocidade:

No `platformio.ini`, altere:
```ini
upload_speed = 115200  ; Velocidade mais baixa, mais estável
```

### 5. Verificar se WT32 Está Sendo Usado

- **Feche o SimHub no PC** durante o upload
- **Desconecte temporariamente o WT32 do PC** se possível
- Isso evita conflitos de porta serial

### 6. Verificar Alimentação

- O WT32 deve estar alimentado via USB do PC
- O ZXACC-ESPDB V2 deve estar conectado ao Mac
- **NÃO conecte +5V do ZXACC ao WT32** - pode danificar!

### 7. Testar Conexões

Verifique se as conexões estão firmes:
- Todos os fios bem conectados
- Sem fios soltos ou mal contato
- GND sempre conectado (muito importante!)

## Sequência Recomendada

1. **Conecte o WT32 ao PC via USB** (alimentação)
2. **Conecte o ZXACC-ESPDB V2 ao Mac via USB**
3. **Faça as conexões de debug** (TX, RX, EN, BOOT, GND)
4. **Feche o SimHub no PC**
5. **Entre em modo bootloader** (BOOT em GND, reset EN, solte BOOT)
6. **Execute o upload imediatamente**

## Se Nada Funcionar

Tente fazer upload diretamente via USB do WT32:
- Desconecte do PC
- Conecte ao Mac
- Use `./upload-mac.sh`
- Reconecte no PC

Isso confirma que o firmware está correto e o problema é apenas com a interface de debug.

