# 📡 WiFiManager - Configuração Fácil de WiFi

## ✨ O que mudou?

Agora você **NÃO precisa mais recompilar** o firmware para trocar de rede WiFi!

## 🚀 Como Usar

### Primeira Vez (ou Nova Rede WiFi)

1. **Ligue o ESP32** - LED 5 fica **azul piscando**
2. **No celular/PC**, procure rede WiFi: **`ESP-SimHub-Config`**
3. **Conecte** nessa rede (sem senha)
4. **Navegador abre automaticamente** em `192.168.4.1`
   - Se não abrir, digite manualmente: http://192.168.4.1
5. **Clique "Configure WiFi"**
6. **Selecione sua rede** da lista
7. **Digite a senha**
8. **Clique "Save"**
9. **ESP32 reinicia** e conecta automaticamente!

LED 5 fica **verde** = WiFi conectado ✅

### Próximas Vezes

O ESP32 **lembra** da rede configurada. Ao ligar, conecta automaticamente (LED verde em ~5 segundos).

## 🔄 Trocar de Rede WiFi

**Método 1 - Via Botões (Recomendado):**

1. **Segure botões 1 + 5** por **3 segundos**
2. LED 5 pisca rapidamente
3. ESP32 reinicia
4. **Portal WiFi abre** novamente (`ESP-SimHub-Config`)
5. Configure nova rede

**Método 2 - Via Código (se não funcionar):**

No arquivo `main_buttons.cpp`, altere linha 18:
```cpp
const char* FALLBACK_SSID = "Orlando_tplink";      // Sua rede
const char* FALLBACK_PASSWORD = "83185003";       // Sua senha
```

Recompile e faça upload. ESP32 tenta essas credenciais se WiFiManager falhar.

## 📊 Indicadores LED 5

| Cor/Padrão | Status |
|------------|--------|
| 🔵 **Piscando lento** | Conectando WiFi (aguarde ~15s) |
| 🟢 **Fixo** | WiFi OK, aguardando SimHub |
| 🔵🟢 **Ciano** | WiFi + SimHub conectados |
| 🔴 **Piscando** | Erro WiFi (resete com botões 1+5) |

## 🛠️ Troubleshooting

### Portal WiFi não abre

- **Certifique que conectou** na rede `ESP-SimHub-Config`
- Tente abrir manualmente: http://192.168.4.1
- Se não funcionar, faça reset: **segure botões 1+5 por 3s**

### ESP não conecta após configurar

- Verifique se senha WiFi está correta (case-sensitive!)
- Certifique que roteador é **2.4GHz** (ESP32 não suporta 5GHz)
- LED vermelho piscando = senha errada ou rede fora de alcance
- Reset: **botões 1+5 por 3s** e configure novamente

### IP Fixo vs DHCP

Por padrão, ESP32 usa **IP fixo** `192.168.0.223`.

Se sua rede for diferente (ex: 192.168.1.x), edite `main_buttons.cpp` linhas 21-27:

```cpp
const bool USE_STATIC_IP = true;
IPAddress STATIC_IP(192, 168, 0, 223);  // Ajuste primeiro octeto
IPAddress GATEWAY(192, 168, 0, 1);      // Ajuste primeiro octeto
```

Ou desabilite IP fixo (usa DHCP automático):
```cpp
const bool USE_STATIC_IP = false;  // DHCP
```

## 🎯 Resumo Rápido

- **Configurar WiFi**: Conecte em `ESP-SimHub-Config` → 192.168.4.1
- **Trocar rede**: Botões 1+5 por 3s
- **IP do ESP**: 192.168.0.223 (se usar IP fixo)
- **SimHub**: Configure TruePort/bridge para esse IP, porta 20777

---

**Vantagens:**
- ✅ Zero recompilações para trocar WiFi
- ✅ Interface web bonita (WiFiManager)
- ✅ Credenciais salvas em flash (sobrevive a reinicializações)
- ✅ Fallback para credenciais hardcoded se WiFiManager falhar
- ✅ Reset via botões (sem precisar de USB)

**Firmware atualizado:** Versão com WiFiManager ativo!
