# Troubleshooting Display - GPIO 227 Error

## Problema

O display não acende e aparecem erros no monitor serial:
```
[E][esp32-hal-gpio.c:102] __pinMode(): Invalid pin selected
E (1547) gpio: gpio_set_level(227): GPIO output gpio_num error
```

## Causa

O erro do GPIO 227 indica que a biblioteca GFX está tentando usar um pino que não existe no ESP32-S3. O ESP32-S3 tem GPIOs de 0 a 48, então o pino 227 está completamente fora do range válido.

Este erro geralmente vem de dentro da biblioteca `Arduino_GFX_Library` durante a inicialização do display.

## Soluções Possíveis

### 1. Verificar Versão da Biblioteca

A versão atual da biblioteca GFX pode ter um bug. Verifique se há uma versão mais recente:

```ini
lib_deps =
    moononournation/GFX Library for Arduino@1.4.0
```

Tente atualizar para a versão mais recente ou usar uma versão específica conhecida por funcionar com WT32-SC01 Plus.

### 2. Verificar Pinout do WT32-SC01 Plus

Confirme que os pinos estão corretos:

```cpp
#define TFT_BL 23  // Backlight (pode ser 45 em algumas versões)
#define TFT_CS 15
#define TFT_DC 21
#define TFT_RST 22
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
```

### 3. Tentar Pino de Backlight Alternativo

Algumas versões do WT32-SC01 Plus usam GPIO 45 para o backlight:

```cpp
#define TFT_BL 45  // Tente este se GPIO 23 não funcionar
```

### 4. Verificar se o Display Está Conectado

- Verifique se o cabo USB está bem conectado
- Verifique se o display está fisicamente conectado à placa
- Tente desconectar e reconectar o USB

### 5. Usar Biblioteca Alternativa

Se o problema persistir, considere usar a biblioteca **LovyanGFX** que é conhecida por funcionar bem com WT32-SC01 Plus:

```ini
lib_deps =
    lovyan03/LovyanGFX@^1.1.0
```

### 6. Verificar Configuração do SPI

Certifique-se de que o SPI está configurado corretamente para ESP32-S3:

```cpp
// ESP32-S3 usa HSPI (SPI2) por padrão
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, false);
```

### 7. Desabilitar Display Temporariamente

Para testar se o problema é apenas com o display, você pode desabilitá-lo temporariamente:

```cpp
bool displayEnabled = false;  // Em SHCustomProtocol.h
```

Isso permitirá que você teste a comunicação com o SimHub sem o display.

## Próximos Passos

1. **Compile e teste** com as correções aplicadas
2. **Verifique os logs** do monitor serial para mais informações
3. **Tente atualizar** a biblioteca GFX para a versão mais recente
4. **Considere usar LovyanGFX** se o problema persistir

## Referências

- [Arduino_GFX_Library GitHub](https://github.com/moononournation/Arduino_GFX_Library)
- [LovyanGFX GitHub](https://github.com/lovyan03/LovyanGFX)
- [WT32-SC01 Plus Documentation](https://www.wireless-tag.com/portfolio/wt32-sc01/)

