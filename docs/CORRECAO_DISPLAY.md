# Correção da Configuração do Display

## Problema Identificado

O erro `gpio_set_level(227): GPIO output gpio_num error` ocorria porque estávamos tentando usar **SPI** quando o WT32-SC01 Plus usa **interface 8-bit MCU (8080) paralela**.

## Solução

Baseado no repositório oficial [WT32-SC01-Plus-ESP32](https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32), corrigimos a configuração do display:

### Pinout Correto (8-bit MCU 8080)

| Pino | GPIO | Descrição |
|------|------|-----------|
| BL_PWM | 45 | Backlight control (active high) |
| LCD_RESET | 4 | LCD reset |
| LCD_RS | 0 | Command/Data selection |
| LCD_WR | 47 | Write clock |
| LCD_DB0 | 9 | Data bit 0 |
| LCD_DB1 | 46 | Data bit 1 |
| LCD_DB2 | 3 | Data bit 2 |
| LCD_DB3 | 8 | Data bit 3 |
| LCD_DB4 | 18 | Data bit 4 |
| LCD_DB5 | 17 | Data bit 5 |
| LCD_DB6 | 16 | Data bit 6 |
| LCD_DB7 | 15 | Data bit 7 |

### Mudanças no Código

1. **Interface alterada de SPI para 8-bit paralela:**
   ```cpp
   // ANTES (ERRADO - SPI):
   Arduino_DataBus *bus = new Arduino_ESP32SPI(...);

   // DEPOIS (CORRETO - 8-bit paralela):
   Arduino_DataBus *bus = new Arduino_ESP32LCD8(
       TFT_RS,  // GPIO 0
       -1,      // CS (não usado)
       TFT_WR,  // GPIO 47
       -1,      // RD (não usado)
       TFT_D0, TFT_D1, TFT_D2, TFT_D3, TFT_D4, TFT_D5, TFT_D6, TFT_D7
   );
   ```

2. **Backlight corrigido para GPIO 45:**
   ```cpp
   #define TFT_BL 45  // Correto segundo documentação oficial
   ```

3. **Reset pin corrigido para GPIO 4:**
   ```cpp
   #define TFT_RST 4  // LCD_RESET segundo documentação
   ```

## Referências

- [Repositório WT32-SC01-Plus-ESP32](https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32)
- [Arduino_GFX_Library - Arduino_ESP32LCD8](https://github.com/moononournation/Arduino_GFX_Library)

## Próximos Passos

1. **Compile e faça upload:**
   ```bash
   ./upload-mac.sh
   ```

2. **Verifique o monitor serial** - deve mostrar:
   - "Backlight pin configured: GPIO 45"
   - "Initializing display (8-bit parallel interface)..."
   - "Backlight enabled"
   - "Display initialized successfully!"

3. **A tela deve acender** mostrando "ESP-SimHubDisplay" no centro

4. **Configure o SimHub** conforme `COMO_CONFIGURAR_SIMHUB.md`

