# Verificação de Conexões ZXACC-ESPDB V2

## ⚠️ Problema: "Failed to write to target RAM"

Este erro geralmente indica que as conexões do ZXACC-ESPDB V2 não estão corretas ou o modo bootloader não está sendo ativado.

## Checklist de Conexões

### Conexões Obrigatórias

```
ZXACC-ESPDB V2          →    WT32-SC01 Plus (Interface Debug)
─────────────────────────────────────────────────────────────
TX                      →    ESP_RXD (pino 4) ✅
RX                      →    ESP_TXD (pino 3) ✅
EN / RST                →    EN (pino 5) ✅
BOOT / GPIO0            →    BOOT (pino 6) ✅
GND                     →    GND (pino 7) ✅

⚠️ NÃO CONECTE +5V!
```

### Verificações Importantes

1. **GND sempre conectado** - Sem GND comum, nada funciona!
2. **TX/RX não invertidos** - TX do ZXACC vai para RXD do WT32
3. **EN conectado** - Necessário para reset
4. **BOOT conectado** - Necessário para modo bootloader

## Teste de Conexões

### Método 1: Verificar Comunicação Serial

1. Conecte apenas TX, RX e GND
2. Abra um monitor serial na porta do ZXACC:
   ```bash
   pio device monitor --port /dev/cu.usbmodem58370635041 --baud 115200
   ```
3. Se você conseguir ver dados (mesmo que lixo), TX/RX estão corretos

### Método 2: Testar Modo Bootloader Manual

1. **Desconecte o ZXACC do Mac temporariamente**
2. **Conecte o WT32 diretamente ao Mac via USB**
3. **Tente fazer upload normalmente** usando `./upload-mac.sh`
4. Se funcionar, o problema está nas conexões do ZXACC

## Problemas Comuns

### 1. TX/RX Invertidos ⚠️ MAIS COMUM

**Sintoma**:
- ✅ Consegue detectar o chip ("Chip is ESP32-S3")
- ❌ Falha ao escrever ("Failed to write to target RAM")

**Isso é EXATAMENTE o seu problema!**

**Solução**: **INVERTA TX e RX:**

**Configuração ATUAL (que está falhando):**
```
ZXACC-ESPDB V2          →    WT32-SC01 Plus
─────────────────────────────────────────────
TX                       →    ESP_RXD (pino 4) ❌
RX                       →    ESP_TXD (pino 3) ❌
```

**Tente INVERTER:**
```
ZXACC-ESPDB V2          →    WT32-SC01 Plus
─────────────────────────────────────────────
TX                       →    ESP_TXD (pino 3) ✅ NOVO
RX                       →    ESP_RXD (pino 4) ✅ NOVO
```

**Passo a passo:**
1. Desconecte TX e RX do ZXACC
2. Reconecte TROCANDO:
   - O que estava em ESP_RXD (pino 4) → coloque em ESP_TXD (pino 3)
   - O que estava em ESP_TXD (pino 3) → coloque em ESP_RXD (pino 4)
3. Mantenha EN, BOOT e GND como estão
4. Tente upload novamente

### 2. EN Não Conectado ou Mal Conectado

**Sintoma**: Não consegue entrar em modo bootloader

**Solução**:
- Verifique se EN está bem conectado
- Tente resetar manualmente: desconecte e reconecte EN

### 3. BOOT Não Funciona

**Sintoma**: Não entra em modo bootloader

**Solução**:
- Verifique se BOOT está conectado ao pino 6
- Tente conectar BOOT diretamente ao GND manualmente (com jumper)
- Mantenha em GND, faça reset (EN), solte BOOT

### 4. GND Não Conectado

**Sintoma**: Nada funciona, erros aleatórios

**Solução**: GND é OBRIGATÓRIO! Sempre conecte GND.

## Sequência Correta para Modo Bootloader

1. **Certifique-se de que todas as conexões estão corretas**
2. **Conecte BOOT (pino 6) ao GND (pino 7)** - use um jumper ou fio
3. **Pressione e solte EN (pino 5)** - isso faz reset
4. **Desconecte BOOT do GND** - agora está em modo bootloader
5. **Execute upload IMEDIATAMENTE** (tempo limitado!)

## Alternativa: Usar Botões do ZXACC (se disponível)

Se o ZXACC-ESPDB V2 tiver botões físicos:
1. Segure o botão BOOT do ZXACC
2. Pressione o botão RESET do ZXACC
3. Solte o botão BOOT
4. Execute upload

## Se Nada Funcionar

Use o método direto (mais confiável):
1. Desconecte WT32 do PC
2. Conecte WT32 ao Mac via USB
3. Use `./upload-mac.sh`
4. Reconecte no PC

Isso confirma que o firmware está correto e o problema é apenas com a interface de debug.

