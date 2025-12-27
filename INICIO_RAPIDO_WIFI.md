# 🚀 GUIA RÁPIDO - WiFi + USB HID

## ⚡ Primeiros Passos (5 minutos)

### 1️⃣ Configure seu WiFi

Edite **APENAS ESTAS DUAS LINHAS** em `src/main_buttons.cpp` (linhas 8-9):

```cpp
const char* WIFI_SSID = "SUA_REDE_WIFI";      // <-- Coloque o nome da sua rede
const char* WIFI_PASSWORD = "SUA_SENHA_WIFI"; // <-- Coloque a senha
```

### 2️⃣ Faça Upload

```powershell
pio run -e dingyimei-s3-zero -t upload --upload-port COM15
```

### 3️⃣ Veja o IP no Monitor Serial

```powershell
pio device monitor -p COM15 -b 115200
```

Procure por esta linha:

```
[WiFi] Connected! IP: 192.168.1.XXX  <-- ANOTE ESTE IP!
```

### 4️⃣ Configure no SimHub

1. Abra **SimHub** → **Settings** → **Arduino**
2. **Connection Type**: `Network (TCP/IP)`
3. **IP Address**: `192.168.1.XXX` (o IP que anotou)
4. **Port**: `20777`
5. Clique **Connect**

### 5️⃣ Pronto! ✅

- **Gamepad USB**: Funciona imediatamente em qualquer jogo
- **LEDs SimHub**: Configure na aba LEDs do SimHub
- **LED 5**: 
  - 🔵 Piscando = Conectando WiFi
  - 🟢 Fixo = WiFi OK, aguardando SimHub
  - 🔵🟢 Ciano = WiFi + SimHub conectados

---

## 🎮 Testando o Gamepad

1. Windows: `Win + R` → digite `joy.cpl` → Enter
2. Você verá o gamepad listado
3. Gire os encoders → veja os eixos Z/Rx/Ry/Rz se moverem
4. Pressione botões → veja acender na tela

---

## 💡 Dicas

- **Gamepad funciona SEM WiFi** (apenas LEDs precisam de WiFi)
- Se WiFi falhar, LED 5 fica vermelho piscando
- ESP32 reconecta automaticamente se WiFi cair
- Serial monitor mostra todos os eventos (útil para debug)

---

## 📖 Documentação Completa

Para configuração avançada, troubleshooting e detalhes técnicos:  
👉 Veja **WIFI_SIMHUB_CONFIG.md**

---

**Versão Firmware:** k (WiFi + USB HID)  
**Última Atualização:** 2025-12-27
