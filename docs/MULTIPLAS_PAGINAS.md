# Sistema de Múltiplas Páginas com Touch

## 📱 Visão Geral

O dashboard agora possui **3 páginas diferentes** que podem ser alternadas tocando na tela do WT32-SC01 Plus.

## 🎯 Páginas Disponíveis

### 1. **RACE** (Página Principal de Corrida)
- Barra de RPM no topo
- Marcha grande no centro
- Tempos de volta (Best, Last, Current)
- Velocidade
- Delta e Delta Progress
- TC, ABS, Brake Bias
- Pressão dos pneus

### 2. **TIMING** (Tempos e Delta Detalhados)
- Barra de RPM no topo
- Marcha
- Tempos de volta em fonte MAIOR
- Delta em destaque
- Delta Progress
- Velocidade em destaque

### 3. **TELEMETRY** (Telemetria do Carro)
- Título "TELEMETRY"
- Marcha no canto superior direito
- TC, ABS, Brake Bias em fonte grande
- Pressão dos 4 pneus em grid
- Velocidade e RPM em destaque

## 🖐️ Como Usar

### Trocar de Página
- **Toque no LADO ESQUERDO da tela**: Vai para a página anterior
- **Toque no LADO DIREITO da tela**: Vai para a próxima página
- **Via MFC (encoder do volante)**: No menu MFC item "PAGE", gire para trocar página remotamente via UART (`$PAGE:NEXT:` / `$PAGE:PREV:`). Ver [MFC_MENU_IMPLEMENTATION.md](MFC_MENU_IMPLEMENTATION.md).

### Indicadores de Página
Na parte inferior da tela há **3 pontos** que indicam:
- **Ponto branco**: Página atual
- **Pontos cinza**: Outras páginas disponíveis

## ⚙️ Configuração Técnica

### Biblioteca Touch
O projeto usa a biblioteca `TouchLib` para suportar o touch capacitivo GT911 do WT32-SC01 Plus.

### Pinout do Touch (GT911)
```cpp
#define TOUCH_SDA 6   // I2C_SDA
#define TOUCH_SCL 5   // I2C_SCL
#define TOUCH_INT 7   // Interrupt
#define TOUCH_RST 4   // Reset (compartilhado com LCD)
```

### Debounce
- Tempo de debounce: **300ms**
- Evita trocas acidentais de página por toques rápidos

## 🔧 Personalização

### Adicionar Mais Páginas

1. **Adicione uma nova entrada no enum:**
```cpp
enum DashPage {
    PAGE_RACE = 0,
    PAGE_TIMING = 1,
    PAGE_TELEMETRY = 2,
    PAGE_NOVA = 3,        // Nova página
    PAGE_COUNT = 4        // Atualizar contador
};
```

2. **Crie uma função de desenho:**
```cpp
void drawPageNova() {
    // Seu código aqui
    drawCell(COL[0], ROW[1], data, "id", "Titulo", "center");
}
```

3. **Adicione no switch do loop():**
```cpp
case PAGE_NOVA:
    drawPageNova();
    break;
```

### Ajustar Layout de uma Página

Cada página tem sua própria função `drawPageXXX()`. Basta editar a função correspondente para mudar o layout.

### Mudar Zona de Toque

Atualmente a tela é dividida em LEFT/RIGHT. Para mudar:

```cpp
void handleTouch() {
    // ... código existente ...

    // Exemplo: dividir em 3 zonas (esquerda, centro, direita)
    if (t.x < SCREEN_WIDTH / 3) {
        // Página anterior
    } else if (t.x > (SCREEN_WIDTH * 2) / 3) {
        // Próxima página
    } else {
        // Zona central - fazer algo diferente
    }
}
```

## 📊 Inspiração

Este sistema foi inspirado nos dashboards profissionais do [SimHub Dash](https://simhubdash.com), com múltiplas páginas para diferentes informações durante a corrida.

## 🐛 Troubleshooting

### Touch não funciona
1. Verifique a saída serial durante o boot
2. Deve aparecer: `Touch screen initialized successfully!`
3. Se aparecer erro, verifique as conexões I2C

### Páginas não trocam
1. Verifique se está tocando na tela (não na borda)
2. Aguarde 300ms entre toques (debounce)
3. Verifique o serial para mensagens "Page changed to: X"

### Display fica lento
- Cada troca de página limpa o cache de renderização
- Isso é normal e garante que a nova página seja desenhada corretamente

## 🎨 Próximas Melhorias

- [ ] Animações de transição entre páginas
- [ ] Swipe gestures (arrastar para trocar)
- [ ] Página de configurações
- [ ] Página com track map
- [ ] Mais indicadores visuais de página ativa
