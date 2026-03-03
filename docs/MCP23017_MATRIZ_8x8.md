# Matriz 8x8 com MCP23017 - Guia Completo

## 📋 Resumo

Expandimos a matriz de botões de **5x5 (25 slots)** para **8x8 (64 slots)** usando o chip **MCP23017** via I2C.

---

## 🎯 Por Que MCP23017?

### Vantagens
✅ **Libera 9 GPIO do ESP32** (GPIO 4-13 agora disponíveis)
✅ **Resolve conflitos de PSRAM** (GPIO 35-37)
✅ **Resolve conflitos de strapping pins** (GPIO 3, 46)
✅ **64 slots totais** (36 livres para expansão futura)
✅ **Comunicação I2C** (apenas 2 pinos: SDA + SCL)
✅ **Latência aceitável** (~2-3ms scan completo)

### Desvantagens
⚠️ **Latência um pouco maior** que GPIO direto (mas imperceptível para botões)
⚠️ **Requer chip adicional** (custo ~R$10)

---

## 🔌 Pinout MCP23017

### Conexões ESP32 → MCP23017

| ESP32 | MCP23017 | Função |
|-------|----------|--------|
| GPIO 8 | SDA (pin 13) | I2C Data |
| GPIO 9 | SCL (pin 12) | I2C Clock |
| 3.3V | VDD (pin 9) | Alimentação |
| GND | VSS (pin 10) | Ground |
| 3.3V | RESET (pin 18) | Reset (ou 10kΩ pullup) |
| GND | A0, A1, A2 (pins 15-17) | Endereço I2C = 0x20 |

### Configuração de Endereço I2C

| A2 | A1 | A0 | Endereço |
|----|----|----|----------|
| GND | GND | GND | 0x20 ⭐ (padrão) |
| GND | GND | 3.3V | 0x21 |
| GND | 3.3V | GND | 0x22 |
| ... | ... | ... | ... |
| 3.3V | 3.3V | 3.3V | 0x27 |

**Usamos 0x20** (todos em GND) neste projeto.

---

## 🎛️ Distribuição de GPIO no MCP23017

### PORTA A (GPA0-GPA7): Colunas (OUTPUT)

| MCP GPIO | Função | Conecta em |
|----------|--------|------------|
| GPA0 (pin 21) | COL0 | Coluna 0 da matriz |
| GPA1 (pin 22) | COL1 | Coluna 1 da matriz |
| GPA2 (pin 23) | COL2 | Coluna 2 da matriz |
| GPA3 (pin 24) | COL3 | Coluna 3 da matriz |
| GPA4 (pin 25) | COL4 | Coluna 4 da matriz |
| GPA5 (pin 26) | COL5 | Coluna 5 da matriz |
| GPA6 (pin 27) | COL6 | Coluna 6 da matriz |
| GPA7 (pin 28) | COL7 | Coluna 7 da matriz |

### PORTB (GPB0-GPB7): Linhas (INPUT_PULLUP)

| MCP GPIO | Função | Conecta em |
|----------|--------|------------|
| GPB0 (pin 1) | ROW0 | Linha 0 da matriz |
| GPB1 (pin 2) | ROW1 | Linha 1 da matriz |
| GPB2 (pin 3) | ROW2 | Linha 2 da matriz |
| GPB3 (pin 4) | ROW3 | Linha 3 da matriz |
| GPB4 (pin 5) | ROW4 | Linha 4 da matriz |
| GPB5 (pin 6) | ROW5 | Linha 5 da matriz |
| GPB6 (pin 7) | ROW6 | Linha 6 da matriz |
| GPB7 (pin 8) | ROW7 | Linha 7 da matriz |

---

## 🔧 Como Funciona a Matriz

### Princípio de Scanning

1. **Ativa uma coluna** (LOW) via GPA0-GPA7
2. **Lê todas as linhas** (GPB0-GPB7)
3. Se linha estiver LOW → botão pressionado
4. **Desativa coluna** (HIGH)
5. Repete para próxima coluna

### Fluxo de Scan Completo

```
Loop principal (1000 Hz):
├── Scan coluna 0 → lê 8 linhas
├── Scan coluna 1 → lê 8 linhas
├── Scan coluna 2 → lê 8 linhas
├── Scan coluna 3 → lê 8 linhas
├── Scan coluna 4 → lê 8 linhas
├── Scan coluna 5 → lê 8 linhas
├── Scan coluna 6 → lê 8 linhas
└── Scan coluna 7 → lê 8 linhas
    ↓
Total: 64 botões verificados em ~2-3ms
```

---

## 🛠️ Esquema de Ligação (Matriz)

### Sentido do Diodo

**LINHA → DIODO (1N4148) → BOTÃO → COLUNA**

```
           ROW0 (GPB0)
              ↓
           |>|---- [SW] ---- COL0 (GPA0)
         DIODO
```

### Exemplo Prático (Slot 1 = MFC SW)

```
GPB0 (ROW0) → |>| → [SW MFC] → GPA0 (COL0)
```

Quando **COL0 = LOW** e **SW pressionado**:
- Corrente flui: GPA0 → SW → Diodo → GPB0
- GPB0 lê **LOW** → botão detectado

---

## 📊 Mapeamento de Slots (8x8 = 64 botões)

| Linha | Col0 | Col1 | Col2 | Col3 | Col4 | Col5 | Col6 | Col7 |
|-------|------|------|------|------|------|------|------|------|
| ROW0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| ROW1 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| ROW2 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 |
| ROW3 | 25 | 26 | 27 | 28 | 29 | 30 | 31 | 32 |
| ROW4 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 |
| ROW5 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 |
| ROW6 | 49 | 50 | 51 | 52 | 53 | 54 | 55 | 56 |
| ROW7 | 57 | 58 | 59 | 60 | 61 | 62 | 63 | **64** |

### Slots Reservados

- **Slot 1**: MFC SW (push do encoder MFC)
- **Slots 2-5**: SW dos encoders 2-5 (BB, MAP, TC, ABS)
- **Slots 6-27**: Botões principais (inclui frontais, borboletas, extras e 5-way)
- **Slot 28**: **SHIFT** (uso interno, não reportado ao HID)
- **Slots 29-64**: **Livres** (36 slots para expansão)

---

## 💻 Código Implementado

### Bibliotecas Necessárias

```cpp
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
```

### Inicialização (setup)

```cpp
Wire.begin(I2C_SDA, I2C_SCL);
if (!mcp.begin_I2C(0x20)) {
    Serial.println("MCP23017 not found!");
    while (1);
}

// PORTA (GPA0-GPA7): colunas (OUTPUT, inicialmente HIGH)
for (uint8_t col = 0; col < 8; col++) {
    mcp.pinMode(col, OUTPUT);
    mcp.digitalWrite(col, HIGH);
}

// PORTB (GPB0-GPB7): linhas (INPUT_PULLUP)
for (uint8_t row = 0; row < 8; row++) {
    mcp.pinMode(8 + row, INPUT_PULLUP);
}
```

### Scanning (loop)

```cpp
for (uint8_t col = 0; col < 8; col++) {
    // Ativa coluna (LOW)
    mcp.digitalWrite(col, LOW);
    delayMicroseconds(10);

    // Lê todas as linhas (GPB0-GPB7)
    for (uint8_t row = 0; row < 8; row++) {
        bool pressed = !mcp.digitalRead(8 + row); // Inverte (pullup)

        // Debounce + atualiza HID
        // ...
    }

    // Desativa coluna (HIGH)
    mcp.digitalWrite(col, HIGH);
}
```

---

## 🧪 Testes e Validação

### Teste 1: Verificar Comunicação I2C

```cpp
Wire.begin(I2C_SDA, I2C_SCL);
if (!mcp.begin_I2C(0x20)) {
    Serial.println("MCP23017 not found!");
} else {
    Serial.println("MCP23017 OK!");
}
```

### Teste 2: Verificar Scanning

Pressione cada botão e veja no monitor serial:
```
Button 1 pressed
Button 1 released
Button 10 pressed
Button 10 released
```

### Teste 3: Verificar HID

- Abra "Propriedades do Controle de Jogo" no Windows
- Pressione cada botão de matriz mapeado ao HID (1-27)
- Slot 28 (SHIFT) **não deve aparecer** (uso interno)

---

## 🚀 Expansão Futura (2 MCP23017)

Se precisar de **mais de 64 slots**, pode adicionar outro MCP23017:

### Endereços I2C

- **MCP1** (matriz 8x8): 0x20 (A0=A1=A2=GND)
- **MCP2** (expansão): 0x21 (A0=3.3V, A1=A2=GND)

### Capacidade Total

- MCP1: 64 slots (matriz principal)
- MCP2: 64 slots (expansão)
- **Total: 128 slots** 🤯

---

## ✅ Checklist de Montagem

- [ ] MCP23017 alimentado com 3.3V (VDD/VSS)
- [ ] A0, A1, A2 → GND (endereço 0x20)
- [ ] RESET → 3.3V (ou 10kΩ pullup)
- [ ] SDA → GPIO 8
- [ ] SCL → GPIO 9
- [ ] GND comum (ESP32 + MCP23017)
- [ ] Todos os botões com diodo 1N4148 (sentido linha → diodo → botão → coluna)
- [ ] Biblioteca `Adafruit_MCP23X17` instalada no platformio.ini

---

## 📚 Referências

- [Datasheet MCP23017](https://ww1.microchip.com/downloads/en/devicedoc/20001952c.pdf)
- [Adafruit MCP23017 Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library)
- [I2C Pull-up Calculator](https://www.ti.com/lit/an/slva689/slva689.pdf)

---

## 🎯 Resumo Técnico

| Parâmetro | Valor |
|-----------|-------|
| **Chip** | MCP23017 |
| **Interface** | I2C (400kHz) |
| **Endereço** | 0x20 |
| **GPIO usados** | 2 (SDA + SCL) |
| **GPIO liberados** | 9 (GPIO 4-13) |
| **Slots totais** | 64 |
| **Slots usados** | 28 |
| **Slots livres** | 36 |
| **Latência scan** | ~2-3ms |
| **Custo** | ~R$10 |

---

**Status:** ✅ Implementado e documentado
