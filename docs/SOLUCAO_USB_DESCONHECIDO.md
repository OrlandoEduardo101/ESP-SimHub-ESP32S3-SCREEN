# Solução: "Dispositivo USB Desconhecido" no Windows

## Problema

No Windows aparecem dois dispositivos:
1. ✅ "USB JTAG/serial debug unit" - Correto (USB-Serial/JTAG integrado)
2. ❌ "Dispositivo USB Desconhecido (Falha na Solicitação de Descritor de Dispositivo)" - Problema

## Causa

O ESP32-S3 tem **dois controladores USB**:
- **USB-Serial/JTAG** (sempre ativo) - usado para upload/debug
- **USB CDC** (configurado com `board_build.usb_mode = cdc`) - usado para comunicação Serial

O Windows pode não reconhecer o USB CDC se:
- O driver não estiver instalado
- Há conflito entre os dois modos USB
- O USB CDC não está sendo inicializado corretamente

## Soluções

### Solução 1: Instalar Driver USB CDC (Recomendado)

1. **Baixar o driver ESP32 USB CDC:**
   - Visite: https://github.com/espressif/usb-pid
   - Ou use o driver do Arduino IDE

2. **Instalar o driver:**
   - No Gerenciador de Dispositivos, clique com botão direito no "Dispositivo USB Desconhecido"
   - Selecione "Atualizar driver"
   - Escolha "Procurar driver no computador"
   - Aponte para a pasta do driver

3. **Alternativa - Usar Zadig (Windows):**
   - Baixe Zadig: https://zadig.akeo.ie/
   - Conecte o ESP32-S3
   - Selecione o dispositivo USB desconhecido
   - Instale o driver WinUSB ou libusb

### Solução 2: Usar Apenas USB-Serial/JTAG

Se o USB CDC continuar dando problema, podemos usar apenas o USB-Serial/JTAG:

1. **Modificar `platformio.ini`:**
   ```ini
   ; Remover ou comentar esta linha:
   ; board_build.usb_mode = cdc

   ; Remover do build_flags:
   ; -DUSE_USB_CDC=true
   ```

2. **Modificar `main.cpp`:**
   - Remover `#ifdef USE_USB_CDC`
   - Usar Serial normalmente (USB-Serial/JTAG)

### Solução 3: Verificar Inicialização do USB CDC

O USB CDC precisa ser inicializado corretamente no código. Verifique se:

1. `Serial.begin()` está sendo chamado
2. Há delay suficiente para o USB CDC inicializar
3. O código não está tentando usar ambos os modos USB simultaneamente

## Verificação

Após aplicar a solução:

1. **Desconecte e reconecte o ESP32-S3**
2. **Verifique no Gerenciador de Dispositivos:**
   - Deve aparecer apenas "USB JTAG/serial debug unit" (para upload)
   - E uma porta COM (para comunicação Serial)
   - **NÃO deve aparecer "Dispositivo USB Desconhecido"**

3. **No SimHub:**
   - A porta COM deve aparecer e ser escaneada corretamente
   - Não deve aparecer "port not scanned"

## Nota Importante

O **USB-Serial/JTAG** (que aparece como "USB JTAG/serial debug unit") é usado para:
- Upload de firmware
- Debug/JTAG
- Monitor serial durante desenvolvimento

O **USB CDC** (que deve aparecer como porta COM) é usado para:
- Comunicação Serial com SimHub
- Comunicação de dados em tempo de execução

Ambos podem funcionar simultaneamente, mas o Windows precisa dos drivers corretos.

