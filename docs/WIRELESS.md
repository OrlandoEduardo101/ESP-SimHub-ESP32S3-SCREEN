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

## Transporte cru (porta 10002) — o modo rápido

### Por que ele existe

Sobre a porta 10001 a tela fala o protocolo ARQ do ESP-SimHub: pacotes de até
32 bytes, CRC e **um ack por pacote, um de cada vez**. Isso existe para tornar
confiável uma serial que não é. Sobre TCP, que já entrega ordenado e íntegro,
não sobra nada além do custo: um frame de telemetria de ~270 bytes vira ~17
idas-e-voltas em sequência.

Medido nesta ligação: a placa confirma em ~5ms, mas cada ida-e-volta custa
~92ms porque o lado Windows (SimHub + porta COM virtual) demora a mandar o
pacote seguinte. 17 × 92ms ≈ 1,6s por frame — os "3 a 4 segundos para a
velocidade mudar".

Na porta 10002 não há ack nenhum. O emissor empurra o frame inteiro e segue.

### Números medidos (mesma placa, mesma rede)

| | ARQ (10001) | Cru (10002) |
|---|---|---|
| frames por segundo | 0,6 | **30** |
| pior iteração do `loop()` | 2.689.780 µs | **13.000 µs** |
| iterações por segundo | 179 | 2.530 |

A queda de 206× no pior `loop()` é o que destravou também o menu MFC: durante
aqueles 2,7s presos em `ARQSerial::read()`, o `handleButtonBoxUart()` não
rodava. Era uma causa raiz só, com dois sintomas.

### O que NÃO era o gargalo

Vale registrar porque custou tempo: o desenho da tela inteiro custa **6,3 ms
por frame** (`page` 4858µs + `alert` 61µs + `indicator` 437µs + `leds` 909µs),
ou seja **0,4%** de um frame de 1559ms. Duas auditorias externas apontaram
`canvas->flush()` e o `drawAlert()` como gargalo principal; a instrumentação
mostrou que não. Mais: `BOARD_HAS_PSRAM` não está definido neste ambiente, então
`canvas` é `nullptr` e **`canvas->flush()` nunca executa** — o gargalo apontado
não roda uma vez sequer. Sempre medir antes de otimizar.

### Como configurar o SimHub

1. **Custom serial devices** → o device apontado para a COM virtual da ponte
   serial-TCP (recomendado — ver seção "Ponte serial-TCP" abaixo) ou, se ainda
   não configurada, para a COM virtual da HW VSP3 apontada para
   `192.168.0.5:10002`. A HW VSP3 entrega os frames certos mas em rajada
   (ver medições na seção da ponte) — some com fluidez visível mesmo com o
   `fps` correto.
2. A fórmula vai na caixa **Update messages** (a que tem o seletor de Hz), não
   em "Message before device disconnect".
3. Conteúdo: o template de `customProtocol-dashBoard.txt` **sem prefixo algum**,
   e com o terminador no fim:

   ```
   ... + isnull([DataCorePlugin.GameData.NewData.TrackId], 'Unknown') + ';' + '~'
   ```

4. Taxa: **20 a 40 Hz**. Acima disso só acumula fila — a 200 Hz o fps *cai*
   para 21, porque a placa satura em ~33 fps e o excesso vira backlog.

Duas armadilhas do NCalc do SimHub, ambas já pagas aqui: ele usa **aspas
simples** (`'P'`, não `"P"`), e **não tem a função `chr()`**. É por isso que o
terminador é um caractere comum (`~`) e que o firmware não exige mais o header
`0x03 'P'` no modo cru — exigir um byte de controle obrigaria a reescrever os
72 campos em JavaScript.

### Enquadramento por terminador, e por que ele é indispensável

O `SHCustomProtocol::read()` lê **72 campos** de forma fixa. O número de campos
que o SimHub emite, porém, é editável a qualquer momento. Sem uma fronteira
explícita, um frame com contagem diferente faz o parser atravessar para dentro
do frame seguinte, e **a partir daí todo valor cai na célula errada, para
sempre** — foi assim que um relógio de sessão apareceu como "PENALTY: 05:19:47".

O terminador resolve isso: `ARQSerial::read()` para ao encontrá-lo e permanece
fechado até o fim daquele parse (`rawFrameEnded`), então um frame malformado
custa apenas ele mesmo. Verificado com frames de 68, 72 e 76 campos: os de 68
preenchem tudo que existe e deixam o resto em branco, e o frame seguinte já
volta correto.

Contar `;` em vez de usar terminador **não** funciona, e a tentativa está
registrada aqui para ninguém repetir: quando o emissor supera a tela, o
`LoopbackStream` descarta bytes em silêncio, mas a contagem já os creditou. O
contador passa a mentir e não há como realinhar.

### Diagnóstico ao vivo (porta 10004)

Conectar via TCP em `192.168.0.5:10004` devolve um texto e zera a janela:

```
window_ms=... loops=... loop_max_us=... uart_us_total=...
frames=... fps=... frame_max_us=...
avg per frame (us): page=... alert=... indicator=... leds=...
canvas=... raw_mode=... overlay_paints=... short=... full=...
speed=... gear=... sessTime=... flag=... pen=... alert=... track=... overlay=[...]
```

A última linha é a ferramenta de alinhamento: compare-a com o que o SimHub
mostra. Um tempo de sessão aparecendo em `pen=` significa deslocamento de
campo, não erro de desenho.

`short=` conta frames que acabaram antes de o parser ter todos os campos (o
emissor manda menos que 72); `full=` é o caso normal. **`short` diferente de 0
de forma contínua significa que o template do SimHub e o `read()` discordam na
contagem** — é o primeiro lugar a olhar se valores aparecerem em branco.

## Ponte serial-TCP (substitui a HW VSP3)

### O problema que ela resolve

A HW VSP3 (SimHub → COM15 virtual → HW VSP3 → TCP 10002) entrega os frames
certos, mas não na hora: ela acumula na porta virtual e manda em rajada. Com
`gap_max_ms`, `gaps_over_100ms` e `multi_frame_chunks` instrumentados em
`lib/TcpSerialBridge2/TcpSerialBridge2.h`, o caminho real mediu, janela após
janela:

```
chunks=63   max_chunk=1200  gaps_over_100ms=21  multi_frame_chunks=21
```

ou seja, a cada ~5 chunks a HW VSP3 juntava 4-5 frames num só pacote, depois
de segurar por até ~250ms. A placa desenha a rajada inteira em poucos
milissegundos — você só vê o último frame dela — e por isso ~21 frames/s
chegavam à tela como ~4 atualizações visíveis por segundo, apesar de o
contador de `fps` no SimHub e no `perfDiagHandle()` concordarem que os frames
estavam todos ali.

Antes de trocar a HW VSP3 de vez, testado que não era Nagle nem a placa: o
mesmo fluxo de frames mandado direto por TCP (sem passar pela porta virtual),
com `TCP_NODELAY` ligado e desligado, chegou um frame por pacote nos dois
casos — `multi_frame_chunks=0`, `gap_max_ms` na casa dos 60-90ms. O
acúmulo é específico da camada de porta serial virtual da HW VSP3, não do
TCP, do WiFi ou do firmware.

### Como ela funciona

`scripts/serial_tcp_bridge.py` ocupa o lugar da HW VSP3: lê o lado COM21 de um
par com0com (o SimHub escreve no COM20, o outro lado do mesmo par), corta em
frames pelo terminador `~` assim que ele chega, e manda cada um imediatamente
por TCP para `192.168.0.5:10002` — sem juntar, sem esperar. Reconecta sozinho
se a placa cair ou o cabo/WiFi oscilar.

Uma tarefa agendada (`scripts/register_serial_bridge_task.ps1`) mantém o
script rodando sozinho: inicia no logon e verifica a cada 1 minuto se ainda
está de pé, relançando se tiver caído — sem precisar abrir terminal.

### Configuração do zero (ex.: depois de formatar o PC)

1. **Instale o com0com** — https://sourceforge.net/projects/com0com/ (a versão
   assinada, "com0com-3.0.0.0-i386-and-amd64-signed"). O instalador já cria um
   par (`CNCA0`↔`CNCB0`); mantenha-o ou remova-o, não importa.

2. **Crie o par COM20/COM21**, dedicado à ponte. Abra o *Setup Command Prompt*
   do com0com (instalado junto, no menu Iniciar) e rode:

   ```
   install PortName=COM20 PortName=COM21
   ```

   Confirme que apareceram os dois:

   ```powershell
   Get-ItemProperty 'HKLM:\HARDWARE\DEVICEMAP\SERIALCOMM' | Format-List
   ```

3. **Instale o Python** (3.10+) marcando "Add python.exe to PATH" no
   instalador, depois:

   ```
   pip install pyserial
   ```

4. **Pare a HW VSP3**, se estiver instalada e rodando — só um cliente pode
   segurar a porta 10002 da placa por vez:

   ```powershell
   Stop-Service HW_VSP3s_Service -Force -EA SilentlyContinue
   Set-Service HW_VSP3s_Service -StartupType Manual -EA SilentlyContinue
   Stop-Process -Name HW_VSP3s_client -Force -EA SilentlyContinue
   ```

5. **Registre a tarefa agendada** (PowerShell **como administrador**, uma vez
   só — ela já deixa a ponte rodando na hora, não precisa relogar):

   ```powershell
   powershell -File "D:\developer\projects\arduino\ESP-SimHub-ESP32S3-SCREEN\scripts\register_serial_bridge_task.ps1"
   ```

6. **No SimHub**, no Custom Serial Device: porta **COM20** (não COM15),
   baudrate 115200 (a ponte ignora o valor — é porta virtual — mas o SimHub
   exige preencher algo), a mesma fórmula de sempre terminada em `+ '~'`,
   30-40 Hz.

Verificação rápida (log da ponte, deve mostrar frames/s e nenhum "descartado"
persistente):

```powershell
Get-Content C:\ProgramData\simhub_serial_bridge.log -Tail 10
```

### Se algo não bater

- **Log mostra "falha ao conectar" em loop**: a placa não está em
  `192.168.0.5` ou não está em modo WiFi (veja "Como ativar" acima).
- **SimHub não lista COM20**: o par com0com não foi criado, ou foi criado com
  outro nome — confira o passo 2.
- **Nada chega na placa mesmo com a ponte "conectado"**: confira se a HW VSP3
  realmente não subiu sozinha e roubou a porta 10002 (`Get-Service
  HW_VSP3s_Service`) — só um cliente por vez.
- Havia um rascunho antigo e abandonado em `docs/tcp2com_bridge.py` (IP e
  porta diferentes dos atuais, sem enquadramento por terminador) — removido
  do repositório junto com esta seção, pra ninguém confundir com a ponte
  atual.

## Upload OTA e o firewall do Windows

`pio run -e wt32-sc01-plus-ota -t upload` só funciona com a tela já em modo
WiFi (o listener do ArduinoOTA só sobe dentro do `if (wifiTransportActive)`).

O espota trabalha em duas etapas: o PC manda um convite UDP pra
`192.168.0.5:3232`, e **a placa então abre uma conexão TCP de volta pro PC**.
Essa segunda etapa é inbound, e o Windows bloqueia inbound por padrão. O
sintoma é característico — autentica e depois expira:

```
Authenticating...OK
[INFO]: Waiting for device...
[ERROR]: No response from device
```

Se parar em `Authenticating`, o problema é senha ou a placa não estar em modo
WiFi. Se autenticar e só então expirar, é o firewall.

Por isso o env fixa `--host_port=38300` (por padrão o espota sorteia uma porta
a cada upload, e não dá pra liberar antecipadamente uma porta aleatória). Com
a porta fixa, uma regra estreita resolve — PowerShell **como administrador**,
uma vez só:

```powershell
New-NetFirewallRule -DisplayName "PlatformIO espota (ESP-SimHub)" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 38300 -RemoteAddress 192.168.0.5
```

Ela libera uma única porta TCP e só pra o IP da tela.

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
