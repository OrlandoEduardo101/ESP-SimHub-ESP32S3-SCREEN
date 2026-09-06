# Modo Sem Fio (WiFi da tela / BLE do volante) — opt-in, padrão desligado

## Contexto

Ambas as placas (WT32-SC01 Plus e ESP32-S3-WROOM1-N8R8 do volante) têm WiFi/BLE
de fábrica, mas o projeto sempre usou USB (dados) + UART (entre as placas) por
serem determinísticos e de baixíssima latência — essencial pra input de corrida.

Este modo adiciona duas opções sem fio **totalmente opcionais**, cada uma
**alternativa** ao seu transporte com fio (nunca simultânea) e com **padrão
desligado**:

- **WiFi da tela**: quando ligado, a tela recebe telemetria do SimHub via
  WiFi/TCP em vez de USB CDC. Útil quando a tela está alimentada (5V) mas sem
  cabo de dados disponível.
- **BLE do volante**: quando ligado, o volante vira um gamepad Bluetooth LE em
  vez de gamepad USB. Útil pra uso casual sem fio. **Não recomendado para uso
  competitivo** — BLE tem latência maior que USB HID.

O UART entre as duas placas **continua com fio** (conectores soldados, placas
fisicamente próximas) — isso nunca foi tocado.

## Por que um gesto escondido em vez de um item novo no menu MFC

O adesivo físico do encoder MFC já lista os 15 itens do menu
(`mfcMenuNames[]` em `src/main_wheel.cpp`). Adicionar um 16º item exigiria
reimprimir o adesivo, então os toggles reaproveitam uma combinação já
existente (SHIFT + segurar o botão do MFC) que hoje só faz uma coisa (alternar
`ENC_MODE`), ramificando por qual item do menu está selecionado no momento.

## Como ativar

Todos os gestos usam **SHIFT + segurar o botão do MFC**, com o encoder MFC
parado no item indicado. Duração contada a partir do início do hold.

| Item selecionado | Duração do hold | Ação |
|---|---|---|
| Qualquer item, exceto RESET/CALIB | ≥ 1.5s | Alterna `ENC_MODE` (comportamento original, inalterado) |
| **RESET** | ≥ 1.5s | **Liga/desliga BLE do volante** (aplica na hora, sem reboot) |
| **CALIB** | solta entre 1.5s–4s | **Liga/desliga WiFi da tela** (aplica só no próximo reboot da tela) |
| **CALIB** | ≥ 4s (ainda segurando) | **Força o portal de configuração WiFi** da tela (apaga credenciais salvas; aplica no próximo reboot) |

O toque curto (sem SHIFT) nos itens RESET e CALIB continua fazendo o que
sempre fez (RESET = reseta configs de fábrica; CALIB = inicia/encerra
calibração dos halls do clutch) — só a combinação SHIFT+hold nesses dois
itens específicos foi reaproveitada.

## Por que WiFi só aplica no reboot, mas BLE é na hora

`TcpSerialBridge2::setup()` (usado pelo WiFiManager) é bloqueante — pode
travar a tela por até ~120s na primeira configuração (portal cativo). Por
isso o toggle de WiFi só grava a preferência e pede reboot; nunca é chamado
no meio da sessão. Já o NimBLE (`bleGamepadInit()`/`bleGamepadDeinit()`) é
rápido o bastante pra ligar/desligar ao vivo.

## Primeira configuração do WiFi (sem credenciais salvas)

1. No item CALIB, SHIFT + segurar por 4s+ (ou apenas ligar o WiFi pela
   primeira vez, já que sem credenciais salvas o portal abre sozinho).
2. Reinicie a tela.
3. Conecte um celular/notebook na rede WiFi `ESP_<chipID>-SH` que a tela cria.
4. Um portal cativo abre em `192.168.4.1` — escolha sua rede e senha.
5. A tela reinicia e conecta na sua rede.

## Limitações conhecidas (não resolvidas por este trabalho)

- **USB HID do volante não é literalmente desligado.** TinyUSB fixa os
  descritores no boot — ligar o BLE não remove a interface USB Gamepad do
  barramento se o cabo estiver conectado, só para de mandar reports por ela.
  Pra ter só um gamepad visível no SO, desconecte o USB ao usar BLE.
- **Credenciais WiFi erradas/rede fora do alcance podem causar reboot-loop**
  (comportamento da própria lib `ESPAsync_WiFiManager`/`connectMultiWiFi`).
  Recuperação: repita o gesto CALIB ≥4s pra forçar o portal de novo — mas se a
  placa não chegar a rodar `loop()` por estar presa reconectando, pode ser
  necessário regravar o firmware com o WiFi desligado.
- **Sem hardware físico pra testar.** Ambos os ambientes (`wt32-sc01-plus` e
  `wroom1-n8r8-wheel`) compilam limpos (`pio run`), com folga de RAM/Flash em
  ambos, mas o comportamento em campo (conexão BLE real, portal WiFi real)
  ainda não foi validado em bancada.

## Onde mexer no código

- `src/main.cpp` — transporte runtime USB/WiFi (`wifiTransportActive`,
  wrappers `wsStream*`/`wsFlowSerialBegin`), comandos UART `$WIFI:TOGGLE:` /
  `$WIFI:PORTAL:` / `$BLE:STATE:` em `processButtonBoxLine()`.
- `lib/TcpSerialBridge2/ECrowneWifi.h` — `ECrowneWifi::forgetCredentials()`.
- `src/main_wheel.cpp` — seção "BLE HID GAMEPAD" (perto do topo),
  `toggleBleMode()`, gestos em `handleMfcPress()`.
- `platformio.ini` — dependência `h2zero/NimBLE-Arduino` no ambiente
  `wroom1-n8r8-wheel`.
