# Como Compilar e Fazer Upload no Mac

## Passo 1: Conectar o WT32-SC01 Plus

1. Conecte o WT32-SC01 Plus ao Mac via cabo USB
2. Aguarde alguns segundos para o sistema reconhecer o dispositivo

## Passo 2: Identificar a Porta Serial

No Mac, execute um dos comandos abaixo para encontrar a porta:

```bash
# Opção 1: Listar portas USB
ls /dev/cu.*

# Opção 2: Usar PlatformIO para listar dispositivos
pio device list

# Opção 3: Verificar portas USB específicas
ls /dev/cu.usbmodem* /dev/tty.usbmodem* 2>/dev/null
```

Você verá algo como:
- `/dev/cu.usbmodem14103` ou
- `/dev/tty.usbmodem14103`

**Nota**: Use `/dev/cu.*` (não `/dev/tty.*`) para upload, pois é mais confiável.

## Passo 3: Compilar e Fazer Upload

### Método 1: Compilar e Upload Automático (Recomendado)

```bash
cd /Users/orlandoeduardo101/Projects/arduino/ESP-SimHub-ESP32S3-SCREEN

# Compilar e fazer upload automaticamente
pio run -e wt32-sc01-plus -t upload
```

O PlatformIO detectará automaticamente a porta USB.

### Método 2: Especificar Porta Manualmente

Se o upload automático não funcionar, especifique a porta:

```bash
# Substitua /dev/cu.usbmodem14103 pela porta do seu dispositivo
pio run -e wt32-sc01-plus -t upload --upload-port /dev/cu.usbmodem14103
```

### Método 3: Compilar Separadamente

```bash
# Apenas compilar
pio run -e wt32-sc01-plus

# Depois fazer upload
pio run -e wt32-sc01-plus -t upload
```

## Passo 4: Monitorar Serial (Opcional)

Para ver os logs do dispositivo:

```bash
pio device monitor -e wt32-sc01-plus
```

Ou especificando a porta:
```bash
pio device monitor --port /dev/cu.usbmodem14103 --baud 115200
```

Para sair do monitor: `Ctrl+C` ou `Ctrl+]`

## Troubleshooting

### Problema: "Port not found" ou "No device found"

**Solução 1**: Verifique se o cabo USB suporta dados (não apenas carregamento)
**Solução 2**: Tente outro cabo USB
**Solução 3**: Verifique se o driver USB está instalado:
   - Para chips CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - Para chips CH340: geralmente funciona nativamente no Mac

### Problema: "Failed to connect" ou timeout

**Solução**: Coloque o ESP32-S3 em modo de bootloader:
1. Mantenha pressionado o botão **BOOT** (se houver)
2. Pressione e solte o botão **RESET**
3. Solte o botão **BOOT**
4. Tente o upload novamente

### Problema: Permissão negada

```bash
# Adicionar seu usuário ao grupo dialout (se necessário)
sudo dseditgroup -o edit -a $(whoami) -t user _developer
```

Ou use `sudo` (não recomendado, mas funciona):
```bash
sudo pio run -e wt32-sc01-plus -t upload
```

## Comandos Úteis

```bash
# Ver dispositivos conectados
pio device list

# Limpar build anterior
pio run -e wt32-sc01-plus -t clean

# Compilar, upload e monitorar tudo de uma vez
pio run -e wt32-sc01-plus -t upload && pio device monitor -e wt32-sc01-plus

# Ver informações do ambiente
pio run -e wt32-sc01-plus -t envdump
```

## Após o Upload

1. O firmware será carregado automaticamente
2. O dispositivo reiniciará
3. Você pode monitorar a saída serial para verificar se está funcionando
4. Depois, desconecte do Mac e conecte no Windows para testar com o SimHub

## Notas Importantes

- **Primeira vez**: Pode ser necessário entrar em modo bootloader manualmente
- **Velocidade**: O upload está configurado para 921600 baud (rápido)
- **USB CDC**: O firmware está configurado para comunicação USB nativa
- **No Windows**: O dispositivo aparecerá como uma porta COM para o SimHub

