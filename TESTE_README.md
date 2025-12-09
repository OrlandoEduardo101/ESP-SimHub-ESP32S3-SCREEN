# 🧪 Testes do Dashboard ESP-SimHub

Estes scripts permitem testar todas as funções do dashboard **sem precisar do SimHub e de um jogo em execução**.

## 📋 Scripts Disponíveis

### 1. `test_dashboard.py` - Teste Completo do Dashboard
Testa o dashboard com diferentes cenários de velocidade e comportamento.

**Cenários testados:**
- ⏸️ Velocidade constante (150 km/h)
- 📈 Aceleração progressiva (0 → 300 km/h)
- 📊 Oscilação de velocidade (120 ↔ 180 km/h)
- 🏁 Volta rápida (simulando uma volta)
- 🛑 Freagem abrupta (300 → 0 km/h)

**Como usar:**
```powershell
python test_dashboard.py
```

**O que esperar:**
- O display mostrará as diferentes páginas com dados variando em tempo real
- Velocidade, RPM, tempos de volta, consumo de combustível, etc. mudarão
- Desgaste dos pneus aumentará a cada volta simulada

---

### 2. `test_alerts.py` - Teste de Alertas e Notificações
Testa todos os tipos de alerta e flags do sistema.

**Alertas testados:**
- 🔴 ENGINE OFF
- ⏸️ PIT LIMITER
- 🟡 YELLOW FLAG
- 🔵 BLUE FLAG
- ⛽ LOW FUEL
- 🚨 CUSTOM ALERT

**Como usar:**
```powershell
python test_alerts.py
```

**O que esperar:**
- Cada alerta aparecerá por 4 segundos no display
- Alerta será exibido com cor e fundo específicos
- Depois será substituído por dados normais

---

### 3. `test_pages.py` - Teste de Navegação entre Páginas
Testa todas as 7 páginas do dashboard.

**Páginas testadas:**
- 📄 PAGE 0: RACE (Velocidade, RPM, Volta)
- ⏱️ PAGE 1: TIMING (Melhores tempos, Delta)
- 📊 PAGE 2: TELEMETRY (TC, ABS, Pressão dos pneus)
- 🌡️ PAGE 3: ADVANCED (Temperaturas, Wear, DRS/KERS)
- 🏎️ PAGE 4: RELATIVE (Head-to-Head)
- 🔄 PAGE 5: LAPS (Setores, Desgaste)
- 🏁 PAGE 6: LEADERBOARD (Classificação)

**Como usar:**
```powershell
python test_pages.py
```

**O que esperar:**
- Cada página será testada por 8 segundos com telemetria variável
- Velocidade aumentará de 100 → 280 km/h e resetará
- Volta aumentará a cada ciclo de velocidade

---

## 🔧 Requisitos

### Python 3.6+

Instale a dependência necessária:

```powershell
pip install pyserial
```

### Conexão COM

- ESP32 deve estar conectado via USB
- Porta padrão: **COM11** (pode ser alterada no script)
- Baud rate: **115200**

---

## ⚙️ Configuração

### Mudar porta COM

Se seu ESP32 está em uma porta diferente, edite o script:

```python
PORT = "COM11"  # ← Mude para sua porta (ex: COM3, COM5, etc)
```

Para encontrar a porta correta em Windows:
```powershell
Get-WmiObject Win32_SerialPort | Select-Object Name, Description
```

### Mudar taxa de dados (baud rate)

```python
BAUD_RATE = 115200  # ← Mude se necessário
```

---

## 🎯 Roteiro de Teste Completo

1. **Conectar o ESP32 via USB**
   ```powershell
   # Verificar porta
   Get-WmiObject Win32_SerialPort | Select-Object Name
   ```

2. **Rodar teste de páginas** (mais completo)
   ```powershell
   python test_pages.py
   ```

3. **Rodar teste de alertas**
   ```powershell
   python test_alerts.py
   ```

4. **Rodar teste do dashboard**
   ```powershell
   python test_dashboard.py
   ```

---

## 🐛 Solução de Problemas

### ❌ "ModuleNotFoundError: No module named 'serial'"
```powershell
pip install pyserial
```

### ❌ "SerialException: [Errno 2] COMxxx: The system cannot find the file specified"
- Verifique se o ESP32 está conectado
- Confirme a porta COM correta
- Tente fechar o Serial Monitor do Arduino IDE

### ❌ "Permission denied"
- Feche o Serial Monitor e IDE do Arduino
- Aguarde 2 segundos
- Tente novamente

### ❌ Dados não aparecem no display
- Verifique se o cabo USB está bem conectado
- Tente desconectar e reconectar o ESP32
- Verifique se o firmware foi uploadado corretamente

---

## 📊 Dados Simulados

Cada script gera dados realistas baseados em condições simuladas:

| Campo | Valores Simulados |
|-------|-------------------|
| Velocidade | 0-300 km/h (variável) |
| RPM | 0-9000 (proporcional à velocidade) |
| Marcha | N, 1-7 (baseada em velocidade) |
| Throttle | 0-100% (proporcional à velocidade) |
| Temperatura | 90-110°C (baseada em carga) |
| Desgaste Pneus | 0-100% (aumenta por volta) |
| Combustível | 5-10 l/volta (consumo simulado) |
| Sector Times | Fixos em intervalos realistas |

---

## 💡 Dicas

- **Para observar comportamento do display**, coloque o monitor perto do ESP32 enquanto executa os testes
- **Teste de stress**: Execute `test_pages.py` por 30 minutos para verificar estabilidade
- **Debug**: Abra o Serial Monitor em paralelo para ver logs

```powershell
# Em outra janela PowerShell
pio device monitor -b 115200 --port COM11
```

---

**Última atualização**: Dezembro 2025
