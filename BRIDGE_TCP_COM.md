# 🌉 Bridge TCP → COM para SimHub

## O que é isso?

Como o SimHub não tem suporte nativo para conexão TCP/IP com Arduino, este bridge cria uma **porta COM virtual** no Windows que redireciona dados para o ESP32-S3 via WiFi.

```
SimHub → COM Virtual (ex: COM20) → Bridge Python → WiFi TCP → ESP32-S3
```

## 📋 Pré-requisitos

### 1. Instalar com0com (Porta COM Virtual)

1. Baixe: https://sourceforge.net/projects/com0com/
2. Execute o instalador `com0com-setup.exe`
3. Durante instalação, **desmarque** "Enable Signed Driver" (para Windows 10/11)
4. Após instalar, abra **Setup Command Prompt** (com0com)
5. Crie um par de portas virtuais:
   ```
   install PortName=COM20 PortName=COM21
   ```
6. Confirme que funcionou:
   ```powershell
   Get-CimInstance -ClassName Win32_SerialPort | Select-Object DeviceID, Description
   ```
   Você deve ver `COM20` e `COM21` listadas.

### 2. Instalar Python + pyserial

```powershell
pip install pyserial
```

## 🚀 Como Usar

### Passo 1: Verifique o IP do ESP32

No código `main_buttons.cpp`, confirme o IP fixo:
```cpp
IPAddress STATIC_IP(192, 168, 0, 223);  // Deve bater com sua rede
```

### Passo 2: Execute o Bridge

```powershell
python tcp2com_bridge.py
```

Você verá:
```
============================================================
TCP to COM Bridge for SimHub + ESP32-S3 WiFi
============================================================
ESP32 Address: 192.168.0.223:20777
Virtual COM Port: COM20
============================================================

[TCP] Connecting to 192.168.0.223:20777...
[TCP] Connected!
[COM] Opening COM20...
[COM] COM20 opened!

============================================================
BRIDGE ACTIVE - SimHub can now connect to COM20
Press Ctrl+C to stop
============================================================
```

### Passo 3: Configure SimHub

1. Abra **SimHub** → **Settings** → **Arduino**
2. Clique **Add Arduino** ou **Scan for Arduinos**
3. Selecione a porta **COM20** (a porta virtual)
4. SimHub deve detectar `ESP-SimHub-ButtonBox` com 20 botões + 5 LEDs

### Passo 4: Teste

- LED 5 do ESP32 deve mudar de **verde** para **ciano** quando SimHub conectar
- No bridge, você verá logs de dados:
  ```
  [TCP→COM] 12 bytes
  [COM→TCP] 5 bytes
  ```

## ⚙️ Ajustes

Se precisar usar portas diferentes, edite o topo do `tcp2com_bridge.py`:

```python
ESP32_IP = "192.168.0.223"      # IP do seu ESP32
ESP32_PORT = 20777              # Porta TCP
VIRTUAL_COM = "COM20"           # Porta virtual (par com0com)
```

## 🔧 Troubleshooting

### Bridge não conecta ao ESP32

```powershell
# Teste conexão TCP
Test-NetConnection -ComputerName 192.168.0.223 -Port 20777
```

- Se falhar: verifique se ESP32 está ligado, WiFi conectado (LED 5 verde), firewall Windows

### SimHub não vê COM20

```powershell
# Liste portas COM
Get-CimInstance -ClassName Win32_SerialPort | Select-Object DeviceID
```

- Se COM20 não aparece: reinstale com0com ou use outro número de porta

### Bridge abre mas SimHub não detecta

- Feche o bridge (`Ctrl+C`)
- Abra SimHub, clique **Scan** na aba Arduino
- **Depois** execute o bridge novamente
- SimHub às vezes precisa "ver" a porta antes de conectar

## 🎯 Automatizar (Opcional)

Para iniciar o bridge automaticamente com Windows:

1. Crie atalho de `tcp2com_bridge.py`
2. Mova para: `C:\Users\SeuUser\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup`
3. Edite propriedades do atalho: Target = `pythonw.exe "caminho\para\tcp2com_bridge.py"` (pythonw = sem janela)

## 📊 Indicadores LED (ESP32)

| LED 5 | Status |
|-------|--------|
| 🔵 Piscando | Conectando WiFi |
| 🟢 Fixo | WiFi OK, aguardando SimHub |
| 🔵🟢 Ciano | WiFi + SimHub conectados via bridge |
| 🔴 Piscando | Erro WiFi |

---

**Vantagens desta solução:**
- ✅ Um cabo USB (só alimentação + HID gamepad)
- ✅ SimHub funciona normalmente (pensa que é COM port)
- ✅ WiFi para controle de LEDs
- ✅ Auto-reconexão se WiFi cair

**Desvantagem:**
- ⚠️ Precisa deixar bridge Python rodando quando usar SimHub
