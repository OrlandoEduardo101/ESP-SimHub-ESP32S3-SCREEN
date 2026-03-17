# ESP-SimHub ESP32-S3 Display Dashboard

[English version](README.md)

Firmware customizado para SimHub em ESP32-S3 com suporte a display TFT, desenvolvido especificamente para a placa **WT32-SC01 Plus**. Este firmware exibe telemetria de corrida em tempo real do SimHub em tela 480x320 (landscape).

## 📋 Visão Geral

Este projeto é um fork do firmware [ESP-SimHub](https://github.com/eCrowneEng/ESP-SimHub), adaptado para placas ESP32-S3 com display TFT. O resultado é uma solução completa de dashboard para sim racing com comunicação serial via USB.

## 📌 Origem do Projeto e Créditos

Para deixar autoria e escopo explícitos:

### Trabalho herdado do repositório original ESP-SimHub (eCrowneEng)

- Modelo base de comunicação com SimHub e fluxo geral de comandos
- Conceitos fundamentais de protocolo serial usados no ecossistema ESP-SimHub
- Conceito original do projeto e abordagem de compatibilidade com SimHub

### Trabalho deste fork (Orlando Eduardo Pereira)

- Implementação focada em ESP32-S3 e adaptação para WT32-SC01 Plus
- Integração de display ST7796 via interface paralela 8-bit e pinagem específica
- Evolução de renderização do dashboard, extensões de protocolo e melhorias visuais
- Integração com volante/button box (UART, interação de menu e telemetria)
- Melhorias de comportamento para NeoPixel e LEDs frontais no uso de sim racing
- Documentação de soldagem, pinagem, troubleshooting e setup do hardware
- Manutenção contínua, correções e melhorias de compatibilidade por plataforma

Se você usar este fork, mantenha os créditos para o projeto original e para este trabalho de implementação.

### Funcionalidades

- ✅ Exibição de telemetria em tempo real (velocidade, RPM, marcha, tempos de volta, pressão de pneus, TC/ABS)
- ✅ Tela de carregamento personalizada com logo
- ✅ Interface de display paralela 8-bit otimizada para WT32-SC01 Plus (ST7796)
- ✅ Suporte opcional a LEDs NeoPixel/WS2812B
- ✅ Comunicação USB Serial/JTAG
- ✅ Layout landscape 480x320 otimizado para dashboard de corrida

## 🎯 Hardware Suportado

### Dispositivo principal

**WT32-SC01 Plus** - placa ESP32-S3 com display TFT integrado

- **Display**: ST7796, 320x480 portrait / 480x320 landscape
- **Interface**: MCU paralela 8-bit (8080)
- **MCU**: ESP32-S3 com PSRAM
- **Comunicação**: USB-Serial/JTAG

### Especificações do display

- **Resolução**: 480x320 pixels (landscape)
- **Controlador**: ST7796
- **Interface**: 8-bit paralela (modo 8080)
- **Backlight**: PWM (GPIO 45)

## 🚀 Início Rápido

### Pré-requisitos

1. **PlatformIO** (extensão VS Code, pip ou Homebrew)
2. **Python 3** com Pillow (para conversão de logo)
3. **SimHub**
4. **Cabo USB**

### Instalação

1. **Clone o repositório**
   ```bash
   git clone https://github.com/your-username/ESP-SimHub-ESP32S3-SCREEN.git
   cd ESP-SimHub-ESP32S3-SCREEN
   ```

2. **Instale o PlatformIO**
   - VS Code: extensão "PlatformIO IDE"
   - pip: `pip install platformio`
   - Homebrew: `brew install platformio`

3. **Converta sua logo** (opcional)
   ```bash
   python3 convert_logo.py
   ```
   Coloque a imagem em `img/logo.png` antes de executar.

4. **Compile e grave**
   ```bash
   ./upload-mac.sh
   # ou
   pio run -e wt32-sc01-plus -t upload
   ```

### Configuração do SimHub

1. Abra **SimHub** -> **Arduino** -> **Single Arduino**
2. Selecione a porta COM
3. Clique em **Scan**
4. Abra a seção **Custom Protocol**
5. Cole o conteúdo de `customProtocol-dashBoard.txt`

## 🛠️ Desenvolvimento

### Estrutura do projeto

```text
ESP-SimHub-ESP32S3-SCREEN/
├── src/
│   ├── main.cpp
│   ├── SHCustomProtocol.h
│   ├── SHCommands.h
│   ├── NeoPixelBusLEDs.h
│   ├── logo_image.h
│   └── GFXHelpers.h
├── img/
│   └── logo.png
├── platformio.ini
├── convert_logo.py
├── upload-mac.sh
└── customProtocol-dashBoard.txt
```

### Build

```bash
# Apenas compilar
pio run -e wt32-sc01-plus

# Compilar e gravar
pio run -e wt32-sc01-plus -t upload

# Monitor serial
pio device monitor
```

## 🔧 Troubleshooting

### Erro "Unrecognized, Invalid version"

Causa comum: envio incorreto no handshake (`Command_Hello`).

Status neste fork: corrigido para enviar apenas o caractere de versão esperado pelo SimHub.

### Problemas comuns

- Display não acende: conferir backlight e inicialização
- Upload falhando: usar script de upload por plataforma
- "no free i80 bus slot": objetos de display inicializados no `setup()`
- Problemas USB: validar modo USB e cabo

## 📚 Recursos adicionais

- ESP-SimHub original: https://github.com/eCrowneEng/ESP-SimHub
- SimHub Discord: https://discord.gg/pnAXf2p3RS
- WT32-SC01 Plus docs: https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32
- Arduino GFX Library: https://github.com/moononournation/Arduino_GFX_Library

## 🤝 Contribuição

Contribuições são bem-vindas.

1. Faça um fork
2. Crie sua branch
3. Faça as alterações
4. Abra um pull request

## 📝 Licença

Baseado no ESP-SimHub. Consulte o repositório original para detalhes de licença.

## 🙏 Créditos

- **ESP-SimHub**: firmware original por eCrowneEng
- **SimHub**: software de dashboard para sim racing
- **Arduino GFX Library**: biblioteca gráfica
- **Comunidade WT32-SC01 Plus**

## 📧 Suporte

Para dúvidas e problemas:

- Abra uma issue no GitHub
- Entre no Discord do SimHub
- Consulte a documentação de troubleshooting

## ☕ Apoie Este Fork

Se este fork te ajudou no seu projeto, você pode apoiar meu trabalho:

- PicPay: **@orlandoeduardo.pereira**
- Link: https://picpay.me/orlandoeduardo.pereira
- Link (PicPay): https://link.picpay.com/p/177377918669b9b8f2b4e25

QR Code PIX:

Imagem do QR neste caminho: `docs/img/pix.png`

![QR Code PIX PicPay](docs/img/pix.png)

---

**Nota**: Este firmware é otimizado para WT32-SC01 Plus. Para outras placas ESP32-S3 com displays diferentes, ajuste pinagem e driver em `src/SHCustomProtocol.h`.
