# Como Compilar no Mac e Fazer Upload no Windows

## Opção 1: Compilar no Mac e Gerar Binário

### 1. Compilar o firmware no Mac:

```bash
cd /Users/orlandoeduardo101/Projects/arduino/ESP-SimHub-ESP32S3-SCREEN
pio run -e wt32-sc01-plus
```

Ou se estiver usando PlatformIO Core diretamente:
```bash
platformio run -e wt32-sc01-plus
```

### 2. Localizar o arquivo binário gerado:

Após a compilação, o arquivo `.bin` estará em:
```
.pio/build/wt32-sc01-plus/firmware.bin
```

### 3. Transferir para o Windows:

- Copie o arquivo `firmware.bin` para o Windows (via USB, nuvem, etc.)
- O caminho completo no Mac será:
  ```
  /Users/orlandoeduardo101/Projects/arduino/ESP-SimHub-ESP32S3-SCREEN/.pio/build/wt32-sc01-plus/firmware.bin
  ```

## Opção 2: Fazer Upload no Windows

### Usando PlatformIO no Windows:

1. **Instale o PlatformIO** no Windows (VS Code + PlatformIO extension)

2. **Copie o projeto inteiro** para o Windows OU apenas o arquivo `.bin`

3. **Se copiou o projeto inteiro:**
   ```bash
   cd caminho/para/projeto
   pio run -e wt32-sc01-plus -t upload
   ```

4. **Se copiou apenas o .bin:**
   - Use o **ESP32 Flash Download Tool** ou
   - Use o **esptool.py** diretamente:
   ```bash
   esptool.py --chip esp32s3 --port COM3 write_flash 0x0 firmware.bin
   ```

### Usando ESP32 Flash Download Tool (Windows):

1. Baixe o [ESP32 Flash Download Tool](https://www.espressif.com/en/support/download/other-tools)
2. Configure:
   - **Chip Type**: ESP32-S3
   - **WorkMode**: develop
   - **LoadMode**: SPI SPEED: 80MHz, SPI MODE: QIO
   - **COM Port**: Selecione a porta do seu dispositivo
   - **Bin File**: Selecione o `firmware.bin`
   - **Offset**: 0x0
3. Clique em **START**

### Usando esptool.py (Windows/Mac/Linux):

```bash
# Instalar esptool (se não tiver)
pip install esptool

# Fazer upload
esptool.py --chip esp32s3 --port COM3 --baud 921600 write_flash 0x0 firmware.bin
```

**Nota**: Substitua `COM3` pela porta serial do seu dispositivo no Windows.

## Opção 3: Compilar e Fazer Upload Direto no Mac (se tiver o dispositivo)

Se você tiver o WT32-SC01 Plus conectado no Mac:

```bash
# Compilar e fazer upload
pio run -e wt32-sc01-plus -t upload

# Monitorar serial (opcional)
pio device monitor -e wt32-sc01-plus
```

**Nota**: No Mac, a porta serial geralmente será `/dev/cu.usbmodem*` ou `/dev/tty.usbmodem*`

## Verificar Porta Serial no Windows:

1. Conecte o WT32-SC01 Plus via USB
2. Abra o **Gerenciador de Dispositivos**
3. Procure em **Portas (COM e LPT)**
4. Você verá algo como: **USB Serial Port (COM3)** ou **Silicon Labs CP210x USB to UART Bridge (COM3)**

## Dicas:

- **Primeira vez**: Pode ser necessário segurar o botão **BOOT** enquanto faz o upload
- **Velocidade**: O `upload_speed = 921600` está configurado no `platformio.ini`
- **Tamanho do Flash**: O firmware está configurado para 16MB de flash

## Estrutura de Arquivos Após Compilação:

```
ESP-SimHub-ESP32S3-SCREEN/
├── .pio/
│   └── build/
│       └── wt32-sc01-plus/
│           ├── firmware.bin      ← Este é o arquivo que você precisa!
│           ├── firmware.elf
│           └── ...
└── ...
```

