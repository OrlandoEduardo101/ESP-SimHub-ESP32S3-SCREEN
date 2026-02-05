# Erro "no free i80 bus slot"

## Problema

Após corrigir a interface para 8-bit paralela, aparece o erro:
```
E (1522) lcd_panel.io.i80: esp_lcd_new_i80_bus(136): no free i80 bus slot
```

## Causa

O ESP32-S3 tem um número limitado de barramentos I80 (interface 8-bit) disponíveis. O erro indica que todos os slots estão ocupados, geralmente porque:

1. O objeto `Arduino_ESP32LCD8` está tentando criar um barramento I80 que já existe
2. Há múltiplas inicializações do display
3. A biblioteca `Arduino_ESP32LCD8` pode ter problemas com ESP32-S3

## Solução Aplicada

Mudamos de `Arduino_ESP32LCD8` para `Arduino_ESP32PAR8`:

```cpp
// ANTES (causava erro):
Arduino_DataBus *bus = new Arduino_ESP32LCD8(...);

// DEPOIS (corrigido):
Arduino_DataBus *bus = new Arduino_ESP32PAR8(...);
```

### Diferenças

- **Arduino_ESP32LCD8**: Otimizado para ESP32-S3, mas pode ter problemas com barramentos I80
- **Arduino_ESP32PAR8**: Mais genérico, funciona com ESP32, ESP32-S2 e ESP32-S3, mais estável

## Verificação

Após a correção, o monitor serial deve mostrar:
- "Backlight pin configured: GPIO 45"
- "Initializing display (8-bit parallel interface)..."
- "Backlight enabled"
- "Display initialized successfully!"

E **não deve mais aparecer** o erro "no free i80 bus slot".

## Se o Erro Persistir

1. **Verifique se há múltiplas inicializações:**
   - Procure por `gfx->begin()` ou `bus->begin()` sendo chamados múltiplas vezes
   - Certifique-se de que o objeto é criado apenas uma vez (globalmente)

2. **Tente reiniciar o dispositivo:**
   - Desconecte e reconecte o USB
   - Pressione o botão RESET na placa

3. **Verifique a versão da biblioteca:**
   - Tente atualizar para a versão mais recente da GFX Library
   - Ou use uma versão específica conhecida por funcionar

4. **Considere usar LovyanGFX:**
   - A biblioteca LovyanGFX é conhecida por funcionar bem com WT32-SC01 Plus
   - Pode ser uma alternativa se o problema persistir

