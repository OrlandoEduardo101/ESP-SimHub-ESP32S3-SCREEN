# Como Identificar a Porta do ZXACC-ESPDB V2 no Mac

## Método 1: Usando PlatformIO (Recomendado)

Execute no terminal:

```bash
pio device list
```

Você verá algo como:

```
/dev/cu.usbserial-14103
--------------------
Hardware ID: USB VID:PID=10C4:EA60 SER=0001 LOCATION=20-1
Description: CP210x USB to UART Bridge Controller

/dev/cu.usbmodem1101
--------------------
Hardware ID: USB VID:PID=303A:1001 SER=D0:CF:13:40:9F:A4 LOCATION=1-1
Description: USB JTAG/serial debug unit
```

**Como identificar:**
- **ZXACC-ESPDB V2** geralmente aparece como:
  - `/dev/cu.usbserial-XXXXX` ou `/dev/tty.usbserial-XXXXX`
  - Descrição: "CP210x USB to UART Bridge Controller" ou similar
  - Hardware ID: geralmente contém `10C4:EA60` (CP210x) ou `0403:6001` (FTDI)

- **WT32-SC01 Plus** (se conectado) aparece como:
  - `/dev/cu.usbmodemXXXXX` ou `/dev/tty.usbmodemXXXXX`
  - Descrição: "USB JTAG/serial debug unit"
  - Hardware ID: geralmente contém `303A:1001` (ESP32-S3)

## Método 2: Listar Portas USB Manualmente

```bash
# Listar todas as portas seriais
ls /dev/cu.* /dev/tty.* 2>/dev/null | grep -E "(usb|serial)"

# Ou apenas portas USB
ls /dev/cu.usb* /dev/tty.usb* 2>/dev/null
```

## Método 3: Antes e Depois (Mais Confiável)

1. **Antes de conectar o ZXACC:**
   ```bash
   pio device list > portas_antes.txt
   ```

2. **Conecte o ZXACC-ESPDB V2 ao Mac**

3. **Depois de conectar:**
   ```bash
   pio device list > portas_depois.txt
   ```

4. **Compare as diferenças:**
   ```bash
   diff portas_antes.txt portas_depois.txt
   ```

A nova porta que aparecer será a do ZXACC-ESPDB V2!

## Método 4: Usar system_profiler (macOS)

```bash
system_profiler SPUSBDataType | grep -A 10 -i "serial\|uart\|cp210\|ftdi"
```

Isso mostrará informações detalhadas sobre dispositivos USB seriais conectados.

## Método 5: Verificar em Tempo Real

```bash
# Monitorar mudanças nas portas (pressione Ctrl+C para sair)
watch -n 1 'ls -la /dev/cu.usb* /dev/tty.usb* 2>/dev/null'
```

Conecte o ZXACC e veja qual porta aparece.

## Identificadores Comuns do ZXACC-ESPDB V2

O ZXACC-ESPDB V2 pode usar diferentes chips USB-to-Serial:

| Chip | VID:PID | Porta Típica |
|------|---------|--------------|
| CP210x | 10C4:EA60 | `/dev/cu.usbserial-XXXXX` |
| FTDI | 0403:6001 | `/dev/cu.usbserial-XXXXX` |
| CH340 | 1A86:7523 | `/dev/cu.wch*` ou `/dev/cu.usbserial-XXXXX` |

## Dica: Usar no Script

O script `upload-debug.sh` já lista as portas disponíveis antes de pedir para você escolher. Basta olhar a lista e identificar qual é a do ZXACC.

## Exemplo Prático

```bash
$ pio device list

/dev/cu.usbserial-14103          ← Este é o ZXACC-ESPDB V2
--------------------
Hardware ID: USB VID:PID=10C4:EA60 SER=0001
Description: CP210x USB to UART Bridge Controller

/dev/cu.usbmodem1101             ← Este é o WT32 (se conectado)
--------------------
Hardware ID: USB VID:PID=303A:1001 SER=D0:CF:13:40:9F:A4
Description: USB JTAG/serial debug unit
```

Neste caso, a porta do ZXACC seria: `/dev/cu.usbserial-14103`

## Troubleshooting

### Nenhuma porta aparece
- Verifique se o ZXACC está conectado ao Mac
- Tente outra porta USB
- Verifique se o cabo USB suporta dados (não apenas carregamento)

### Múltiplas portas aparecem
- Use o método "antes e depois" para identificar qual é nova
- Verifique o Hardware ID na descrição
- O ZXACC geralmente tem descrição relacionada a "UART", "Serial", "CP210x" ou "FTDI"

### Porta muda a cada conexão
- Isso é normal no macOS
- Use `pio device list` antes de cada upload para ver a porta atual
- Ou use o script `upload-debug.sh` que lista as portas automaticamente

