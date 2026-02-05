# Configuração WiFi + SimHub

## ⚙️ Configuração Inicial

### 1. Alterar Credenciais WiFi

Abra `src/main_buttons.cpp` e altere estas linhas (próximo ao topo):

```cpp
const char* WIFI_SSID = "SUA_REDE_WIFI";      // <-- ALTERE AQUI
const char* WIFI_PASSWORD = "SUA_SENHA_WIFI"; // <-- ALTERE AQUI
```

Substitua pelos dados da sua rede WiFi.

### 2. Upload do Firmware

```powershell
pio run -e dingyimei-s3-zero -t upload --upload-port COM15
```

### 3. Monitorar Conexão WiFi

Abra o monitor serial para ver o IP atribuído:

```powershell
pio device monitor -p COM15 -b 115200
```

Você verá algo como:

```
[WiFi] Connected! IP: 192.168.1.100
[WiFi] SimHub TCP server started on port 20777
[SimHub] Waiting for TCP connection on port 20777...
[SimHub] Configure no SimHub: IP 192.168.1.100 porta 20777
```

**Anote o endereço IP!** Você precisará dele no SimHub.

---

## 🎮 Configuração no SimHub

### Passo 1: Abrir Configurações de Arduino

1. Abra o **SimHub**
2. Vá em **Settings** (Configurações)
3. Clique na aba **Arduino**

### Passo 2: Adicionar Dispositivo via Rede

1. No campo **Connection Type**, selecione **Network (TCP/IP)**
2. No campo **IP Address**, digite o IP que apareceu no monitor serial (ex: `192.168.1.100`)
3. No campo **Port**, digite `20777`
4. Clique em **Connect** ou **Add Device**

### Passo 3: Verificar Detecção

- O SimHub deve detectar o dispositivo como **"ESP-SimHub-ButtonBox"**
- Na lista de devices, você verá:
  - **Buttons**: 20
  - **RGB LEDs**: 5

### Passo 4: Configurar LEDs

1. Na aba **LEDs**, você verá os 5 LEDs WS2812B disponíveis
2. Configure cores/efeitos de acordo com telemetria do jogo
3. LEDs 1-4 podem ser mapeados para dados de corrida
4. LED 5 mostrará status WiFi quando não controlado pelo SimHub

---

## 🔍 Diagnóstico de Problemas

### ❌ WiFi não conecta

**Sintomas:**
- LED 5 piscando azul continuamente
- Monitor serial mostra `[WiFi] Connection FAILED!`

**Soluções:**
1. Verificar se SSID e senha estão corretos
2. Confirmar que ESP32 está dentro do alcance do roteador
3. Verificar se a rede WiFi está disponível (2.4GHz, não 5GHz)

### ❌ SimHub não detecta dispositivo

**Sintomas:**
- WiFi conectado (LED 5 verde)
- SimHub não lista o dispositivo

**Soluções:**
1. Verificar se IP e porta estão corretos no SimHub
2. Testar conexão com `telnet <IP> 20777` (Windows PowerShell)
3. Desabilitar firewall temporariamente para testar
4. Confirmar que PC e ESP32 estão na mesma rede WiFi

### ❌ LEDs não respondem ao SimHub

**Sintomas:**
- Dispositivo conectado
- LEDs ficam mostrando encoders/WiFi status

**Soluções:**
1. Verificar se SimHub está enviando comandos LED
2. No SimHub, testar "Test LED Pattern"
3. Monitor serial deve mostrar `[LEDs] SimHub control activated`

---

## 📊 Feedback LED (LED 5)

| Cor/Padrão | Significado |
|------------|-------------|
| **Apagado** | WiFi desconectado |
| **Azul Piscante** | Conectando ao WiFi... |
| **Verde Fixo** | WiFi OK, aguardando SimHub |
| **Ciano Fixo** | WiFi OK + SimHub conectado |
| **Vermelho Piscante** | Erro de conexão WiFi |

---

## ⚡ Vantagens desta Solução

✅ **Um único cabo USB** (só para alimentação + HID gamepad)  
✅ **HID Gamepad funciona independente** do WiFi  
✅ **SimHub controla LEDs via rede** TCP  
✅ **Dual-core**: Core 0 (WiFi) + Core 1 (USB/botões) = sem lag  
✅ **Auto-reconexão**: se WiFi cair, reconecta automaticamente  
✅ **Timeout SimHub**: LEDs voltam ao modo local se SimHub desconectar  

---

## 🔧 Comandos Úteis

### Verificar IP do ESP32

```powershell
# No monitor serial, procure por:
[WiFi] Connected! IP: 192.168.x.x
```

### Testar Conexão TCP

```powershell
# Substitua <IP> pelo IP do ESP32
Test-NetConnection -ComputerName <IP> -Port 20777
```

### Resetar WiFi

Se precisar trocar de rede, basta recompilar com novas credenciais e fazer upload novamente.

---

## 📝 Versão do Firmware

**Versão:** k (WiFi + USB HID)  
**Data:** 2025-12-27  
**Recursos:**
- USB HID Gamepad (20 botões físicos + 8 virtuais)
- 4x Encoders rotativos com dual-mode (axes ↔ buttons)
- WiFi TCP servidor (porta 20777)
- Protocolo SimHub para controle de 5x WS2812B LEDs
- Feedback visual de status WiFi/SimHub

---

## 🆘 Suporte

Se tiver problemas, verifique:
1. Monitor serial (`pio device monitor`) para mensagens de debug
2. LED 5 para status WiFi
3. Windows Gerenciador de Dispositivos para verificar se HID gamepad aparece
4. SimHub logs na aba Arduino

**Gamepad sempre funciona**, mesmo sem WiFi! 🎮
