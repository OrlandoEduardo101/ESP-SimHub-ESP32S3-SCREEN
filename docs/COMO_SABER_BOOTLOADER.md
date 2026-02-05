# Como Saber se o ESP32 Está em Modo Bootloader

## ⚠️ Problema: Sem Feedback Visual

O ESP32-S3 **não tem LED indicador** de modo bootloader. Não há feedback visual quando entra em modo bootloader.

## ✅ Métodos para Verificar Modo Bootloader

### Método 1: Testar com esptool (Recomendado)

Use o script de teste que criei:

```bash
./testar-bootloader.sh /dev/cu.usbmodem58370635041
```

Este script tenta comunicar com o ESP32 e verifica se está respondendo.

**Se funcionar:**
- ✅ Chip detectado = Está em modo bootloader ou pronto
- ✅ Pode fazer upload

**Se não funcionar:**
- ❌ Não está em modo bootloader
- ❌ Conexões podem estar incorretas
- ❌ Tente entrar em modo bootloader novamente

### Método 2: Tentar Upload Direto

A forma mais prática é **tentar fazer upload**:

```bash
./upload-debug.sh
```

**Se o upload começar e mostrar:**
```
Connecting....
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded PSRAM 2MB (AP_3v3)
```

✅ **Está em modo bootloader!** O upload deve continuar.

**Se mostrar erro imediatamente:**
```
A fatal error occurred: Failed to write to target RAM
```

❌ **Não está em modo bootloader** ou há problema nas conexões.

### Método 3: Monitor Serial (Limitado)

Você pode tentar abrir o monitor serial:

```bash
pio device monitor --port /dev/cu.usbmodem58370635041 --baud 115200
```

**Em modo bootloader:**
- Geralmente não há saída (silêncio)
- Ou pode mostrar caracteres aleatórios

**Em modo normal (firmware rodando):**
- Mostra logs do firmware
- Mensagens de inicialização

⚠️ **Nota:** Este método não é muito confiável, pois o bootloader pode não enviar nada.

## 🔄 Sequência Correta para Modo Bootloader

1. **Certifique-se de que todas as conexões estão corretas:**
   - TX, RX, EN, BOOT, GND conectados

2. **Entre em modo bootloader:**
   ```
   a) Conecte BOOT (pino 6) ao GND (pino 7) - use jumper ou fio
   b) Pressione e solte EN (pino 5) - isso faz reset
   c) Desconecte BOOT do GND IMEDIATAMENTE
   ```

3. **Execute upload IMEDIATAMENTE:**
   - O modo bootloader dura apenas alguns segundos
   - Execute `./upload-debug.sh` logo após soltar BOOT

## 💡 Dicas Importantes

### Timing é Crítico

- ⏱️ Modo bootloader dura **5-10 segundos** no máximo
- 🚀 Execute o upload **imediatamente** após entrar em modo bootloader
- ⚡ Não espere muito tempo

### Se o Upload Falhar

1. **Tente novamente** - pode ter saído do modo bootloader
2. **Verifique conexões** - especialmente GND e TX/RX
3. **Tente inverter TX/RX** - problema comum
4. **Use método direto** - conecte WT32 ao Mac via USB

### Alternativa: Usar Botões do ZXACC (se disponível)

Se o ZXACC-ESPDB V2 tiver botões físicos:
1. Segure o botão **BOOT** do ZXACC
2. Pressione o botão **RESET** do ZXACC
3. Solte o botão **BOOT**
4. Execute upload imediatamente

## 🔍 Verificação Rápida

**Checklist antes de tentar upload:**

- [ ] TX, RX, EN, BOOT, GND conectados
- [ ] WT32 alimentado (via USB do PC)
- [ ] ZXACC conectado ao Mac
- [ ] Porta correta selecionada
- [ ] Entrou em modo bootloader (BOOT→GND, reset EN, soltar BOOT)
- [ ] Upload executado IMEDIATAMENTE após modo bootloader

## 📝 Resumo

**Não há feedback visual de modo bootloader no ESP32-S3.**

A única forma de confirmar é:
1. ✅ Tentar fazer upload - se começar, está OK
2. ✅ Usar `./testar-bootloader.sh` - verifica comunicação
3. ✅ Ver logs do esptool - se detectar chip, está OK

**Se o upload falhar com "Failed to write to target RAM":**
- Pode não estar em modo bootloader
- Ou há problema nas conexões (TX/RX, GND, etc.)

