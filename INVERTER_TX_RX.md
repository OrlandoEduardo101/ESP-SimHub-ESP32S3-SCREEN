# Como Inverter TX/RX no ZXACC-ESPDB V2

## ⚠️ Problema Atual

Você está recebendo:
```
Chip is ESP32-S3 (QFN56) (revision v0.2)  ✅ Detecta o chip
A fatal error occurred: Failed to write to target RAM  ❌ Falha ao escrever
```

**Isso indica que TX/RX estão invertidos!**

## 🔄 Solução: Inverter TX e RX

### Passo a Passo

1. **Desconecte apenas TX e RX** (mantenha EN, BOOT e GND conectados)

2. **Troque as conexões:**
   ```
   ANTES (não funciona):
   TX do ZXACC → ESP_RXD (pino 4)
   RX do ZXACC → ESP_TXD (pino 3)

   DEPOIS (deve funcionar):
   TX do ZXACC → ESP_TXD (pino 3)  ← TROCOU
   RX do ZXACC → ESP_RXD (pino 4)  ← TROCOU
   ```

3. **Mantenha o resto igual:**
   - EN → EN (pino 5) ✅
   - BOOT → BOOT (pino 6) ✅
   - GND → GND (pino 7) ✅

4. **Entre em modo bootloader novamente:**
   - Conecte BOOT ao GND
   - Pressione e solte EN
   - Desconecte BOOT do GND

5. **Tente upload novamente:**
   ```bash
   ./upload-debug.sh
   ```

## 📋 Checklist Completo de Conexões

Após inverter TX/RX, verifique:

```
ZXACC-ESPDB V2          →    WT32-SC01 Plus
─────────────────────────────────────────────
TX                       →    ESP_TXD (pino 3) ✅
RX                       →    ESP_RXD (pino 4) ✅
EN / RST                 →    EN (pino 5) ✅
BOOT / GPIO0             →    BOOT (pino 6) ✅
GND                      →    GND (pino 7) ✅

⚠️ NÃO CONECTE +5V!
```

## 💡 Por Que Isso Acontece?

A nomenclatura TX/RX pode variar dependendo do fabricante:
- **TX** = Transmit (envia dados)
- **RX** = Receive (recebe dados)

O problema é que:
- TX do ZXACC deve enviar dados → ESP deve RECEBER (RXD)
- RX do ZXACC deve receber dados → ESP deve ENVIAR (TXD)

Mas alguns adaptadores têm a nomenclatura invertida ou diferente.

## ✅ Teste Rápido

Se ainda não funcionar após inverter:

1. **Verifique GND** - Sem GND comum, nada funciona
2. **Teste velocidade mais baixa** - Já está em 115200, que é bom
3. **Use método direto** - Conecte WT32 ao Mac via USB para confirmar que firmware está OK

## 🔍 Diagnóstico

**Se após inverter TX/RX:**
- ✅ Upload funciona → Problema resolvido!
- ❌ Ainda falha → Pode ser problema de EN/BOOT ou GND

**Se não detectar chip:**
- ❌ TX/RX ainda invertidos (volte ao original)
- ❌ GND não conectado
- ❌ Porta errada

