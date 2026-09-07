# Contexto da sessão — WiFi/BLE opcional (pra continuar no Claude do Windows)

Este documento resume tudo que foi feito e descoberto numa sessão longa de
implementação + debug do modo sem fio. Cole isso na conversa nova do Claude
(ou só peça pra ele ler este arquivo) pra continuar sem perder contexto.

## O que foi pedido originalmente

Tornar configurável o uso de WiFi (tela WT32-SC01 Plus) e BLE (volante
ESP32-S3-WROOM1), como alternativa **opcional** ao USB/UART com fio — nunca
substituindo, sempre desligado por padrão, sem adicionar item novo no
adesivo físico do menu MFC do volante.

## Estado atual: commitado

Commit `3bb9227` (branch `main`) tem toda a implementação. Ver a mensagem
do commit pra lista completa do que mudou — resumo rápido abaixo.

## O que está funcionando

- **BLE do volante**: implementado (NimBLE, mesmo descritor HID do USB),
  ativa/desativa com SHIFT + segurar MFC no item RESET. **Bloqueado por
  hardware** — ver seção "Problema não resolvido: brownout do volante".
- **WiFi da tela**: implementado, ativa/desativa com SHIFT + segurar MFC no
  item CALIB (solta 1.5-4s = toggle; segura 4s+ = força portal de config).
  Conecta, aparece no SimHub como `ESP-SimHubDisplay` via bridge TCP na
  porta 10001. **Funciona, mas lento** — ver seção abaixo.
- **OTA da tela**: funcionando de verdade (testado, upload real via rede
  sem cabo). Ambiente `wt32-sc01-plus-ota` no `platformio.ini`. **Senha
  hardcoded `changeme-simhub-wt32`** em `src/main.cpp` (`OTA_PASSWORD`) e
  em `platformio.ini` (`--auth=`) — trocar os dois juntos quando puder
  (exige um upload por USB pra aplicar a senha nova).
- **OTA do volante**: não implementado de propósito — ele não tem WiFi,
  e não deveria ganhar (ver brownout abaixo).

## Problema não resolvido #1: brownout do volante ao ligar BLE

O volante **reseta por brownout** (`BROWNOUT_RST`, confirmado pelo próprio
ROM do chip) assim que `NimBLEDevice::init()` liga o rádio — mesmo com a
placa completamente descarregada (testado ligando o BLE **antes** de USB,
I2C, LEDs, tudo). Não é bug de software, não é RAM (310KB livres na hora do
crash), não é os LEDs (usuário já roda em 20% de brilho). **Capacitores de
desacoplamento já estão instalados** — não sugerir isso de novo.

- Uma trava anti-boot-loop já existe (`bleBoot` na NVS) — se o BLE travar
  nunca mais deixa a placa presa reiniciando pra sempre, ela volta com BLE
  desligado sozinha.
- Causa provável: fonte de 3,3V sem margem pro pico de corrente da
  energização do rádio. Próximo passo pra retomar isso: medir a tensão
  3,3V no pino do módulo em repouso com multímetro; se já estiver abaixo de
  3,3V parado, é regulador/fiação, não rádio.
- Upload no volante SEMPRE falha na primeira tentativa (erro pySerial
  "Device not configured") e funciona na segunda — normal nessa placa
  (USB nativo TinyUSB, não CH340), não é sinal de problema.
- Se o firmware do volante travar de verdade (boot-loop), recuperação é
  manual: GPIO0 (pino 9 do módulo WROOM-1) no GND, plugar USB, soltar.
  Confirma que entrou em bootloader vendo PID `1001` no `pio device list`
  (em vez do normal `8172`).

## RESOLVIDO: dashboard lento por WiFi (e o MFC junto)

**Era uma causa raiz só, com dois sintomas.** O protocolo ARQ é stop-and-wait:
um ack por pacote, um de cada vez. Um frame de ~270 bytes vira ~17 idas-e-voltas
sequenciais, e cada uma custa ~92ms porque o lado Windows demora a mandar o
pacote seguinte. 17 × 92ms ≈ 1,6s por frame. E enquanto a placa esperava dentro
de `ARQSerial::read()` (medido: 2,69s numa única iteração do `loop()`), o
`handleButtonBoxUart()` não rodava — daí o menu MFC parecer travado.

**Correção**: transporte cru na porta 10002, sem ARQ. Ver `docs/WIRELESS.md`,
seção "Transporte cru", para a arquitetura, a configuração do SimHub e os
números. Resumo: **0,6 → 30 fps**, pior `loop()` de 2.689.780µs → 13.000µs.

Correções secundárias, todas medidas:

- `idle()` em `main.cpp` agora atende o UART do volante. Esse hook já era
  chamado durante toda a espera bloqueante do ARQ e não fazia nada útil.
- Render gate em `SHCustomProtocol::loop()`: só desenha se chegou telemetria,
  a página mudou, ou passaram 250ms. Antes redesenhava a cada iteração, o que
  prendia o ack do ARQ em 105ms; com o gate caiu para 5ms.
- `drawAlert()` só repinta o overlay quando ele muda, e o `loop()` não desenha
  a página por baixo de um overlay que a cobre. Antes eram 101ms por frame com
  overlay ativo (3,8 fps).
- `Serial.setTxTimeoutMs(0)` quando em modo WiFi: sem isso, com o cabo USB só
  alimentando e ninguém lendo o CDC, cada print travava até 100ms.

### Lições que custaram tempo — não repetir

1. **Medir antes de otimizar.** Duas auditorias externas (GPT e Gemini) e eu
   mesmo apontamos o desenho da tela como gargalo. A instrumentação mostrou que
   o desenho inteiro custa **6,3ms de um frame de 1559ms — 0,4%**. Otimizar
   `drawCell`/`drawAlert` ou mover a UI para o Core 0 teria dado ~0,4%.

2. **`canvas` é `nullptr` neste ambiente.** `BOARD_HAS_PSRAM` não está definido
   em `wt32-sc01-plus`, então `gfx = tft` e o desenho é direto no painel.
   `canvas->flush()`, apontado por ambas as auditorias como custo principal,
   **nunca executa**.

3. **Dual-core para a UI é perigoso aqui.** A task de desenho leria dezenas de
   `String` enquanto o parser as reatribui — `String` realoca ao ser atribuída,
   e ler durante isso é use-after-free. Este projeto já se queimou com corrida
   sem lock (ver o comentário em `lib/FullLoopbackStream/`). Se algum dia for
   necessário, fazer com snapshot sob mutex.

4. **Contar separadores não substitui um terminador.** Quando o emissor supera
   a tela, o `LoopbackStream` descarta bytes em silêncio mas a contagem já os
   creditou — o contador mente e não há como realinhar. Só um marcador de fim
   de frame resolve.

5. **A sonda pode ser o problema.** `ARQ_TIMING_TRACE` gravava o timestamp
   *antes* dos próprios `Serial.print`, que bloqueiam até 100ms no CDC. Corrigido,
   mas a lição vale: instrumento que mede a si mesmo engana.

## Em aberto

1. **Validação com tráfego real ainda pendente.** O alinhamento de campos foi
   verificado com um emissor sintético (`scripts/raw_align_test.py`, que planta
   valores conhecidos e lê de volta o que a placa parseou, com frames de 68, 72
   e 76 campos), mas o HW VSP3 caiu num reboot de OTA e não
   reconectou sozinho, então a última medição com SimHub de verdade é anterior
   à correção do enquadramento. **Ao voltar: clicar "Create COM" no HW VSP3 e
   ler a porta 10004 (`scripts/perf_read.py`).** Se `short=` ficar diferente de 0 continuamente, o
   template do SimHub emite um número de campos diferente de 72.

2. **O plugin C# (`IDataPlugin`) não foi feito, e talvez não precise.** A rota
   Custom Serial Devices + HW VSP3 já entrega os 30 fps porque o gargalo eram os
   round trips do ARQ, não a porta virtual. O plugin só valeria para eliminar a
   dependência do HW VSP3; ganho de taxa seria pequeno.

3. **`PERF_DIAG` (porta 10004) continua instalado.** É o único jeito de
   diagnosticar com o WiFi ligado, já que o CDC não é legível nesse modo. Se for
   removido, tirar: o bloco `perfDiagHandle()` em `main.cpp`, os contadores
   `pd*`/`perfFieldDump()` em `SHCustomProtocol.h`, e as marcações de fase no
   `loop()`.

4. **`drawCell()` ainda usa `getTextBounds` + `clearTextArea`.** Sugerido por
   revisão externa trocar por padding de espaços. Não fiz: `page` custa ~5ms e
   não é gargalo hoje. Vale só se o desenho voltar a importar.

5. **Nada commitado.** `src/main_wheel.cpp` tem trabalho independente do usuário
   (pipeline de amostragem dos halls) no mesmo working tree — commitar tudo junto
   misturaria as duas coisas. Separar antes.

## Arquivos-chave

- `src/main.cpp` — firmware da tela (WT32-SC01 Plus, env `wt32-sc01-plus`)
- `src/main_wheel.cpp` — firmware do volante (env `wroom1-n8r8-wheel`)
- `lib/EspSimHub/ArqSerial.h` — protocolo ARQ (framing de 32 bytes,
  `ARQ_TIMING_TRACE`)
- `lib/FullLoopbackStream/` — buffer da ponte WiFi (tamanho + thread-safety)
- `lib/TcpSerialBridge2/` — bridge TCP + WiFiManager (código do eCrowne,
  reaproveitado)
- `docs/WIRELESS.md` — documentação do recurso (gestos, limitações)
- `scripts/auto_upload_port.py` — trava de segurança por MAC ao gravar
  (nunca desativar/ignorar; se abortar, é sinal real de placa errada/
  desconectada, não bug)

## Comandos úteis

```
# Compilar
pio run -e wt32-sc01-plus          # tela, USB
pio run -e wroom1-n8r8-wheel       # volante, USB

# Gravar
pio run -e wt32-sc01-plus -t upload        # tela via USB
pio run -e wt32-sc01-plus-ota -t upload    # tela via WiFi (sem cabo)
pio run -e wroom1-n8r8-wheel -t upload     # volante via USB (retry se falhar 1x)
```

IP fixo da tela: `192.168.0.5`. MACs de referência em `platformio.ini`
(`custom_expected_mac`) — tela `d0:cf:13:40:9f:a4`, volante
`dc:b4:d9:0b:51:6c`.
