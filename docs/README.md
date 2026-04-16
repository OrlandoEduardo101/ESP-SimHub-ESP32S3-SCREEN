# Documentation Index

This index helps you quickly find the right document by topic.

## 1) Getting Started

- [INSTALAR_PLATFORMIO.md](INSTALAR_PLATFORMIO.md)
- [COMPILAR_NO_MAC.md](COMPILAR_NO_MAC.md)
- [UPLOAD_NO_MAC.md](UPLOAD_NO_MAC.md)
- [COMO_CONFIGURAR_SIMHUB.md](COMO_CONFIGURAR_SIMHUB.md)

## 2) Hardware and Wiring

- [ESQUEMA_ELETRICO_VOLANTE_COMPLETO.md](ESQUEMA_ELETRICO_VOLANTE_COMPLETO.md)
- [PINMAP_SOLDERING_GUIDE.md](PINMAP_SOLDERING_GUIDE.md)
- [ESP32-S3-WROOM1-N8R8_PINMAP.md](ESP32-S3-WROOM1-N8R8_PINMAP.md)
- [MCP23017_MATRIZ_8x8.md](MCP23017_MATRIZ_8x8.md)
- [WT32-SC01 Plus .md](WT32-SC01%20Plus%20.md)

## 3) Wheel/ButtonBox Firmware

- [BUTTONBOX_README.md](BUTTONBOX_README.md)
- [MANUAL_BUTTONBOX.md](MANUAL_BUTTONBOX.md)
- [MFC_MENU_IMPLEMENTATION.md](MFC_MENU_IMPLEMENTATION.md)
- [VOLANTE_REDONDO.md](VOLANTE_REDONDO.md)
- [RESUMO_IMPLEMENTACAO.md](RESUMO_IMPLEMENTACAO.md)
- [BUTTONBOX_README_S3ZERO_LEGACY.md](BUTTONBOX_README_S3ZERO_LEGACY.md) (legacy)

## 4) Display and LED Features

- [MULTIPLAS_PAGINAS.md](MULTIPLAS_PAGINAS.md)
- [CONFIGURACAO_LEDS.md](CONFIGURACAO_LEDS.md)
- [INSTALACAO_RAPIDA_LEDS.md](INSTALACAO_RAPIDA_LEDS.md)
- [TESTE_LEDS.md](TESTE_LEDS.md)
- [CORRECAO_DISPLAY.md](CORRECAO_DISPLAY.md)

## 5) WiFi / Bridge / Network

- [WIFIMANAGER_GUIA.md](WIFIMANAGER_GUIA.md)
- [WIFI_SIMHUB_CONFIG.md](WIFI_SIMHUB_CONFIG.md)
- [INICIO_RAPIDO_WIFI.md](INICIO_RAPIDO_WIFI.md)
- [TRUEPORT_SETUP.md](TRUEPORT_SETUP.md)

## 6) Troubleshooting

- [TROUBLESHOOTING_DISPLAY.md](TROUBLESHOOTING_DISPLAY.md)
- [TROUBLESHOOTING_ERROS_UPLOAD.md](TROUBLESHOOTING_ERROS_UPLOAD.md)
- [TROUBLESHOOTING_ZXACC.md](TROUBLESHOOTING_ZXACC.md)
- [SOLUCAO_USB_DESCONHECIDO.md](SOLUCAO_USB_DESCONHECIDO.md)
- [ERRO_I80_BUS_SLOT.md](ERRO_I80_BUS_SLOT.md)
- [INVERTER_TX_RX.md](INVERTER_TX_RX.md)
- [IDENTIFICAR_PORTA_ZXACC.md](IDENTIFICAR_PORTA_ZXACC.md)
- [VERIFICAR_CONEXOES_ZXACC.md](VERIFICAR_CONEXOES_ZXACC.md)
- [PROGRAMAR_COM_ZXACC.md](PROGRAMAR_COM_ZXACC.md)
- [COMO_SABER_BOOTLOADER.md](COMO_SABER_BOOTLOADER.md)

## 7) Test and Utility Scripts (inside docs)

These files are utility/testing scripts and not core firmware documentation:

- [tcp2com_bridge.py](tcp2com_bridge.py)
- [test_serial_connection.py](test_serial_connection.py)
- [test_simple.py](test_simple.py)
- [sniff_protocol.py](sniff_protocol.py)
- [TESTAR_COMUNICACAO_SIMHUB.md](TESTAR_COMUNICACAO_SIMHUB.md)
- [TESTE_README.md](TESTE_README.md)

## 8) Assets

- [img/](img/)
- [svg/](svg/)
- [crashapp.txt](crashapp.txt) (external crash log reference)

## 9) Cleanup Notes

Already removed from repository workspace:

- `docs/.DS_Store`
- `scripts/__pycache__/`

Potential cleanup candidates (manual decision recommended):

- `docs/crashapp.txt` (unrelated external iOS crash log)
- `docs/test_simple.py` (one-off serial sniff test)
- `docs/test_serial_connection.py` (one-off ARQ handshake tester)
- `docs/sniff_protocol.py` (one-off protocol sniffer)

Keep by default because they are still useful in specific workflows:

- `docs/tcp2com_bridge.py` (referenced by bridge documentation)
- `scripts/patch_hid_name.py` (used automatically by PlatformIO)
- `scripts/upload-debug.sh` and `scripts/testar-bootloader.sh` (debug/programming workflow)
