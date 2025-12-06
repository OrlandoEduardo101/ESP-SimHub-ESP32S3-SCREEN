# Como Configurar o SimHub para o Dashboard

Este guia explica como configurar o SimHub para enviar dados para o dashboard no WT32-SC01 Plus.

## 1. Configurar o Protocolo Customizado no SimHub

1. **Abra o SimHub**
2. **Vá em "Arduino" → "Single Arduino"**
3. **Selecione sua porta COM** (ex: COM11)
4. **Clique em "Scan"** para detectar o dispositivo
5. **Vá em "Custom Protocol"** (ou "Protocolo Customizado")
6. **Cole o conteúdo do arquivo `customProtocol-dashBoard.txt`** no campo de protocolo customizado

## 2. Estrutura do Protocolo

O protocolo envia os seguintes dados separados por `;`:

1. **SpeedKmh** - Velocidade em km/h
2. **Gear** - Marcha atual (N, R, 1-8)
3. **CurrentDisplayedRPMPercent** - RPM em percentual (0-100)
4. **RPMRedLineSetting** - Configuração da linha vermelha do RPM
5. **CurrentLapTime** - Tempo da volta atual (formato MM:SS.mmm)
6. **LastLapTime** - Tempo da última volta
7. **BestLapTime** - Melhor tempo de volta
8. **SessionBestLiveDeltaSeconds** - Delta em relação ao melhor tempo da sessão
9. **SessionBestLiveDeltaProgressSeconds** - Delta de progresso
10. **TyrePressureFrontLeft** - Pressão do pneu dianteiro esquerdo
11. **TyrePressureFrontRight** - Pressão do pneu dianteiro direito
12. **TyrePressureRearLeft** - Pressão do pneu traseiro esquerdo
13. **TyrePressureRearRight** - Pressão do pneu traseiro direito
14. **TCLevel** - Nível de controle de tração
15. **TCActive** - Controle de tração ativo
16. **ABSLevel** - Nível de ABS
17. **ABSActive** - ABS ativo
18. **isnull([GameRawData.Graphics.TCCut])** - Se TCCut é nulo
19. **TCLevel + TCCut** - Nível de TC e TCCut combinados
20. **BrakeBias** - Viés de freio
21. **Brake** - Nível de freio
22. **LapInvalidated** - Se a volta foi invalidada

## 3. Layout do Dashboard

O dashboard exibe:

### Linha Superior (RPM Meter)
- Barra de RPM com cores:
  - **Verde**: RPM normal
  - **Laranja**: Próximo da linha vermelha (5% antes)
  - **Vermelho**: Na linha vermelha ou acima

### Coluna Esquerda (Tempos de Volta)
- **Best Lap**: Melhor tempo de volta
- **Last Lap**: Última volta
- **Current Lap**: Volta atual (vermelho se invalidada)

### Coluna Central
- **Gear**: Marcha atual (grande, amarelo)
- **Speed**: Velocidade

### Coluna Direita (Delta)
- **Delta**: Delta em relação ao melhor tempo (verde se negativo, vermelho se positivo)
- **Delta P**: Delta de progresso

### Linha Inferior
- **TC**: Nível de controle de tração (amarelo)
- **ABS**: Nível de ABS (azul)
- **BB**: Brake Bias (magenta)
- **FL/FR/RL/RR**: Pressão dos pneus (ciano)

## 4. Resolução da Tela

O dashboard está configurado para:
- **320x480 pixels** (WT32-SC01 Plus)
- **5 linhas x 5 colunas** de células
- Cada célula tem aproximadamente **64x96 pixels**

## 5. Personalização

Você pode personalizar o dashboard editando `src/SHCustomProtocol.h`:

- **Cores**: Altere as constantes de cor (RED, GREEN, YELLOW, etc.)
- **Layout**: Ajuste as posições em `COL[]` e `ROW[]`
- **Tamanho da fonte**: Altere o parâmetro `fontSize` nas funções `drawCell()`

## 6. Troubleshooting

### O display não acende
- Verifique se `displayEnabled = true` em `SHCustomProtocol.h`
- Verifique as conexões do display
- Verifique se o pino de backlight está correto (GPIO 23 ou 45)

### O SimHub não envia dados
- Verifique se o protocolo customizado está configurado corretamente
- Verifique se o dispositivo foi escaneado com sucesso
- Verifique se a velocidade serial está em 19200 baud

### Os dados não aparecem corretamente
- Verifique se o protocolo customizado está exatamente como no arquivo `customProtocol-dashBoard.txt`
- Verifique os logs do SimHub para erros
- Verifique se o jogo está rodando e enviando dados

## 7. Próximos Passos

1. **Compile e faça upload** do firmware
2. **Configure o protocolo customizado** no SimHub
3. **Inicie um jogo** e verifique se os dados aparecem
4. **Ajuste o layout** conforme necessário

