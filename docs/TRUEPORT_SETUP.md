# 🚀 Configuração WiFi SimHub com TruePort (Método eCrowne)

## O que é TruePort?

**Perle TruePort** é um driver profissional que cria portas COM virtuais e redireciona para TCP/IP. É mais estável que com0com + bridge Python.

## 📥 Download e Instalação

### 1. Baixar TruePort
https://www.perle.com/downloads/trueport.shtml

- Escolha versão para Windows
- Instale normalmente (Next → Next → Finish)

### 2. Configurar Porta COM Virtual

Após instalar:

1. Abra **TruePort Manager** (procure no menu Iniciar)
2. Clique **"Add Port"** ou **"Configurar nova porta"**
3. Preencha:
   - **Device Name/IP**: `192.168.0.223` (IP do seu ESP32)
   - **TCP Port**: `20777`
   - **COM Port Number**: `COM20` (ou qualquer livre)
   - **Mode**: **LITE** ⚠️ **IMPORTANTE!** Selecione LITE mode
4. Clique **Apply** ou **OK**

### 3. Verificar Porta Criada

```powershell
Get-CimInstance -ClassName Win32_SerialPort | Select-Object DeviceID, Description
```

Você deve ver `COM20 - TruePort` listado.

## 🎮 Configurar no SimHub

1. Abra **SimHub** → **Settings** → **Arduino**
2. Clique **"Add Arduino"** ou **"Scan"**
3. Selecione porta **COM20** (TruePort)
4. SimHub deve detectar **"ESP-SimHub-ButtonBox"**
   - 20 botões
   - 5 LEDs RGB

## ✅ Verificar Funcionamento

### LED 5 do ESP32:
- 🔵 Piscando = Conectando WiFi
- 🟢 Fixo = WiFi OK, aguardando SimHub
- 🔵🟢 Ciano = SimHub conectado via TruePort!

### No SimHub:
- Botões devem responder quando você apertar
- LEDs devem aceitar comandos da aba LEDs

## ⚙️ Configurações Importantes

### ⚠️ TruePort DEVE estar em LITE mode!

Se não funcionar, verifique:
1. Abra **TruePort Manager**
2. Edite a porta COM20
3. Em **Mode**, confirme que está **LITE** (não RAW, não outros)
4. Apply e reinicie SimHub

### Se SimHub não detectar:

1. Feche SimHub completamente
2. Abra TruePort Manager e verifique se porta está "Connected" (verde)
3. Teste conexão:
   ```powershell
   Test-NetConnection -ComputerName 192.168.0.223 -Port 20777
   ```
4. Reabra SimHub e clique "Scan" novamente

## 🔧 Troubleshooting

### "Port already in use"
- Feche qualquer programa que esteja usando COM20
- Se estava rodando meu bridge Python, feche ele (`Ctrl+C`)

### TruePort não conecta ao ESP32
- Verifique se ESP32 está ligado (LED 5 verde = WiFi OK)
- Confirme IP no código (`main_buttons.cpp` linha 16):
  ```cpp
  IPAddress STATIC_IP(192, 168, 0, 223);
  ```
- Firewall Windows pode estar bloqueando

### SimHub conecta mas não recebe dados
- **Essencial:** TruePort em modo **LITE**
- Tente recriar a porta COM no TruePort Manager

## 📊 Comparação: TruePort vs Bridge Python

| Recurso | TruePort (eCrowne) | Bridge Python (meu) |
|---------|-------------------|---------------------|
| Instalação | Download + GUI | Grátis, código aberto |
| Estabilidade | ⭐⭐⭐⭐⭐ Comercial | ⭐⭐⭐⭐ DIY |
| Auto-start | ✅ Serviço Windows | ⚠️ Manual ou script |
| Performance | Melhor (driver nativo) | Bom (Python overhead) |
| Custo | Grátis (trial ilimitado) | 100% grátis |
| Suporte | Documentação oficial | Código-fonte editável |

## 🎯 Recomendação

Use **TruePort** se:
- ✅ Quer solução plug-and-play
- ✅ Prefere GUI em vez de linha de comando
- ✅ Quer que funcione automaticamente ao ligar Windows

Use **Bridge Python** se:
- ✅ Não conseguir instalar TruePort
- ✅ Quiser entender/modificar o código
- ✅ Precisar de logs detalhados de debug

---

## 📝 Resumo Rápido

1. **Instale TruePort**: https://www.perle.com/downloads/trueport.shtml
2. **Configure porta**: IP `192.168.0.223`, porta `20777`, modo **LITE**
3. **SimHub**: Settings → Arduino → selecione COM criada
4. **Pronto!** LED ciano = funcionando ✨

**Seu ESP32 já está pronto** - o firmware WiFi atual funciona perfeitamente com TruePort!
