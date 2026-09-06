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

## Problema não resolvido #2: dashboard da tela via WiFi é lento

**Sintoma**: valores (velocidade etc.) demoram ~3s pra atualizar na tela,
mesmo depois de corrigir dois bugs reais no firmware.

**Bugs reais já corrigidos** (e que valeram a pena, mas não foram a causa
completa):
1. Buffer de rede de 64 bytes (`FullLoopbackStream` default) estourando
   com frames de ~270 bytes → corrigido pra 1024 bytes.
2. Corrida de threads: `AsyncTCP` escreve no buffer numa FreeRTOS task
   separada do `loop()` principal, sem lock nenhum no `LoopbackStream`
   original → adicionado `portMUX`/critical section em
   `lib/FullLoopbackStream/`.
3. `WiFi.setSleep(false)` — o power-save do WiFi deixava o RTT em
   40-120ms; sem ele caiu pra ~5-18ms. Ajudou, mas sozinho não resolveu.

**Causa raiz ainda não resolvida**, isolada via instrumentação:

- O protocolo ARQ (`lib/EspSimHub/ArqSerial.h`) fatia cada mensagem em
  pacotes de até 32 bytes (na prática, o SimHub manda em pedaços de
  até 16 bytes) e espera confirmação de cada um antes do próximo — um
  frame de 270 bytes vira **~17 idas-e-voltas sequenciais**.
- Medi com uma sonda de timing embutida (`ARQ_TIMING_TRACE`, ainda ligada
  em `platformio.ini` como `-DARQ_TIMING_TRACE=1` — **desligar depois que
  resolver, adiciona overhead de print por pacote**): a placa confirma
  cada pacote em **~2ms** (mediana), mas fica **~96ms** (mediana) parada
  esperando o próximo pedaço do PC. Em 45s de captura, 11.7s foram só
  espera. **O firmware está inocentado** — o atraso está do lado Windows.
- Testado e descartado: opção "Strict Baudrate Emulation" do HW VSP3
  (desmarcada, sem efeito).
- **TruePort nunca funcionou**: o assistente fecha sozinho ao clicar
  "Avançar", sem erro visível. Suspeita não confirmada: ele tenta um
  protocolo de descoberta próprio da Perle que nosso `AsyncServer` cru
  não fala. Não foi mais investigado a fundo (usuário: "nunca consegui
  usar o Perle").
- **com0com**: bate em Código 52 (driver não assinado). Usuário recusou
  usar `bcdedit /set testsigning on` (quer solução definitiva, não
  temporária). Não seguido adiante.

**Como a conexão está funcionando agora** (funcional, só lento): SimHub →
HW VSP3 (cria COM15 ⇄ TCP 192.168.0.5:10001) → firmware.

## Próximos passos em aberto (não decidido ainda)

Duas linhas de solução propostas, nenhuma implementada:

1. **Plugin C# pro SimHub** (`IDataPlugin`): manda o frame telemetria
   inteiro via TCP direto, sem o protocolo ARQ (que só faz sentido pra
   serial não-confiável; sobre TCP é overhead puro). Eliminaria as ~17
   idas-e-voltas por completo. Exige: DLL .NET Framework referenciando
   `SimHub.Plugins.dll`/`GameReaderCommon.dll` da instalação do SimHub do
   usuário; um modo novo no firmware pra ler frame cru terminado em
   newline (mantendo o parser de campos `;` que já existe). **Eu não
   consigo compilar nem testar isso** (não tenho Windows/SimHub) — o
   usuário precisaria compilar aí.
2. **UDP Relay do SimHub** (aba na barra lateral): possivelmente manda a
   telemetria bruta do jogo em formato binário nativo (varia por jogo) em
   vez do protocolo customizado com `;`. Ainda não confirmado — usuário
   ia mandar print da aba e a conversa foi interrompida antes de eu ver.
   Se o formato não for customizável, a placa teria que parsear binário
   específico de cada jogo (pior pra manutenção) em troca de zero
   idas-e-voltas.

**Decisão pendente com o usuário**: qual das duas linhas seguir (ou tentar
o TruePort de novo primeiro, já que é o que a documentação do protocolo
—eCrowne— realmente prescreve, mas o usuário já disse preferir uma ponte
nova a insistir no Perle).

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
