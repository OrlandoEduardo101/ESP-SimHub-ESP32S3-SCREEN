# Troubleshooting: Erros de Upload via ZXACC-ESPDB V2

## 🔍 Diagnóstico por Erro

### Erro 1: "Failed to write to target RAM"
**Significado**: Consegue detectar o chip, mas não consegue escrever dados.

**Causa**: TX/RX provavelmente invertidos

**Solução**:
- Inverter TX e RX
- TX do ZXACC → ESP_TXD (pino 3)
- RX do ZXACC → ESP_RXD (pino 4)

---

### Erro 2: "No serial data received" ⚠️ SEU ERRO ATUAL
**Significado**: Não recebe NENHUM dado do ESP32.

**Possíveis causas**:
1. ❌ TX/RX completamente desconectados ou errados
2. ❌ GND não conectado (MUITO COMUM!)
3. ❌ ESP32 não está em modo bootloader
4. ❌ EN/BOOT não conectados corretamente
5. ❌ Porta serial errada

**Solução passo a passo**:

#### 1. Verificar GND (MAIS IMPORTANTE!)
```
✅ GND do ZXACC → GND (pino 7) do WT32
```
**Sem GND comum, NADA funciona!**

#### 2. Verificar TX/RX
Tente AMBAS as configurações:

**Configuração A:**
```
TX do ZXACC → ESP_RXD (pino 4)
RX do ZXACC → ESP_TXD (pino 3)
```

**Configuração B (invertido):**
```
TX do ZXACC → ESP_TXD (pino 3)
RX do ZXACC → ESP_RXD (pino 4)
```

#### 3. Verificar EN e BOOT
```
EN do ZXACC → EN (pino 5) do WT32
BOOT do ZXACC → BOOT (pino 6) do WT32
```

#### 4. Entrar em modo bootloader CORRETAMENTE
```
1. Conecte BOOT (pino 6) ao GND (pino 7)
2. Pressione e solte EN (pino 5) - faz reset
3. Desconecte BOOT do GND IMEDIATAMENTE
4. Execute upload IMEDIATAMENTE (tempo limitado!)
```

#### 5. Verificar se porta está correta
```bash
pio device list
```
Certifique-se de que está usando a porta do ZXACC, não do WT32.

---

## 📋 Checklist Completo

Antes de tentar upload, verifique:

- [ ] **GND conectado** (pino 7) - OBRIGATÓRIO!
- [ ] **TX conectado** (teste pino 3 e 4)
- [ ] **RX conectado** (teste pino 3 e 4)
- [ ] **EN conectado** (pino 5)
- [ ] **BOOT conectado** (pino 6)
- [ ] **+5V NÃO conectado** (já alimentado via USB)
- [ ] **Porta correta** selecionada
- [ ] **Modo bootloader** ativado
- [ ] **Upload executado** imediatamente após modo bootloader

## 🔄 Sequência Correta

1. **Conecte todas as conexões** (GND, TX, RX, EN, BOOT)
2. **Verifique GND** - sem ele, nada funciona!
3. **Entre em modo bootloader**:
   - BOOT → GND
   - Pressione EN
   - Solte BOOT
4. **Execute upload IMEDIATAMENTE** (dentro de 5-10 segundos)

## 💡 Dica: Teste GND Primeiro

Se você está recebendo "No serial data received", o problema mais comum é **GND não conectado**.

**Teste rápido**:
1. Desconecte tudo
2. Conecte APENAS GND (ZXACC → WT32 pino 7)
3. Reconecte TX, RX, EN, BOOT
4. Tente novamente

## 🆘 Se Nada Funcionar

Use o método direto para confirmar que o firmware está OK:

1. Desconecte WT32 do PC
2. Conecte WT32 ao Mac via USB
3. Execute `./upload-mac.sh`
4. Se funcionar, o problema está nas conexões do ZXACC

