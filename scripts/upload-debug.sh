#!/bin/bash

# Script para compilar e fazer upload do firmware via ZXACC-ESPDB V2
# Permite programar o WT32 enquanto ele está conectado ao PC via USB
# Uso: ./upload-debug.sh

set -e

ENV="wt32-sc01-plus-debug"

echo "🔨 Compilando firmware para WT32-SC01 Plus (via ZXACC-ESPDB V2)..."
echo ""

# Tentar encontrar o comando pio em locais comuns
PIO_CMD=""
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
elif command -v platformio &> /dev/null; then
    PIO_CMD="platformio"
elif [ -f "$HOME/.platformio/penv/bin/pio" ]; then
    PIO_CMD="$HOME/.platformio/penv/bin/pio"
elif [ -f "$HOME/.local/bin/pio" ]; then
    PIO_CMD="$HOME/.local/bin/pio"
fi

if [ -z "$PIO_CMD" ]; then
    echo "❌ Erro: PlatformIO não encontrado no PATH ou em locais comuns!"
    echo "   Por favor, instale o PlatformIO CLI ou adicione-o ao seu PATH."
    exit 1
fi

echo "📋 Verificando dispositivos conectados..."
echo ""
${PIO_CMD} device list
echo ""

echo "💡 Dica para identificar o ZXACC-ESPDB V2:"
echo ""
echo "   ✅ ZXACC-ESPDB V2 geralmente aparece como:"
echo "      - /dev/cu.usbserial-* ou /dev/tty.usbserial-*"
echo "      - Descrição: 'CP210x', 'FTDI', 'UART Bridge', 'USB Single Serial' ou similar"
echo "      - Hardware ID: contém '10C4:EA60' (CP210x), '0403:6001' (FTDI) ou '1A86:55D3' (CH340)"
echo ""
echo "   ⚠️  WT32 (se conectado) aparece como:"
echo "      - /dev/cu.usbmodem* ou /dev/tty.usbmodem*"
echo "      - Descrição: 'USB JTAG/serial debug unit'"
echo "      - Hardware ID: contém '303A:1001' (ESP32-S3)"
echo ""
echo "   📌 Se você vê apenas uma porta, desconecte o WT32 do PC temporariamente"
echo "      para evitar confusão, ou conecte o ZXACC primeiro e identifique qual é."
echo ""

echo "🔨 Compilando..."
${PIO_CMD} run -e ${ENV}

echo ""
echo "✅ Compilação concluída!"
echo ""

# Perguntar pela porta do ZXACC-ESPDB V2
echo "📌 Selecione a porta do ZXACC-ESPDB V2:"
echo "   (Digite o caminho completo, ex: /dev/cu.usbserial-14103)"
echo ""
read -p "Porta: " DEBUG_PORT

# Remover espaços em branco e caracteres especiais, e garantir que não está duplicado
DEBUG_PORT=$(echo "$DEBUG_PORT" | tr -d '[:space:]' | sed 's|/dev/\([^/]*\)/dev/\([^/]*\)|/dev/\1|')

if [ -z "$DEBUG_PORT" ]; then
    echo "❌ Porta não especificada!"
    exit 1
fi

# Validar formato da porta (deve começar com /dev/)
if [[ ! "$DEBUG_PORT" =~ ^/dev/ ]]; then
    echo "⚠️  Aviso: A porta '$DEBUG_PORT' não parece estar no formato correto."
    echo "   Portas no Mac geralmente são: /dev/cu.* ou /dev/tty.*"
    read -p "   Continuar mesmo assim? (s/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Ss]$ ]]; then
        exit 1
    fi
fi

# Verificar se a porta existe
if [ ! -e "$DEBUG_PORT" ]; then
    echo "⚠️  Aviso: A porta '$DEBUG_PORT' não foi encontrada!"
    echo "   Verifique se o ZXACC-ESPDB V2 está conectado."
    read -p "   Continuar mesmo assim? (s/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Ss]$ ]]; then
        exit 1
    fi
fi

# Nota: ZXACC pode aparecer como usbmodem ou usbserial dependendo do chip USB-to-Serial
# O importante é que seja a porta que apareceu quando você conectou o ZXACC

echo ""
echo "📤 Fazendo upload via ZXACC-ESPDB V2..."
echo "   Porta: $DEBUG_PORT"
echo ""
echo "⚠️  INSTRUÇÕES IMPORTANTES:"
echo ""
echo "   1. ✅ Verifique as conexões do ZXACC-ESPDB V2:"
echo "      - ⚠️  GND do ZXACC → GND (pino 7) ⚠️  OBRIGATÓRIO! Sem GND, nada funciona!"
echo "      - TX do ZXACC → ESP_RXD (pino 4) ou ESP_TXD (pino 3) - teste ambos"
echo "      - RX do ZXACC → ESP_TXD (pino 3) ou ESP_RXD (pino 4) - teste ambos"
echo "      - EN do ZXACC → EN (pino 5)"
echo "      - BOOT/GPIO0 do ZXACC → BOOT (pino 6)"
echo "      - ⚠️  NÃO conecte +5V do ZXACC ao WT32!"
echo ""
echo "      💡 Se receber 'No serial data received':"
echo "         → PRIMEIRO verifique GND (pino 7) - está conectado?"
echo "         → Depois teste inverter TX/RX"
echo ""
echo "      💡 Se o upload falhar, tente INVERTER TX e RX:"
echo "         - TX do ZXACC → ESP_TXD (pino 3)"
echo "         - RX do ZXACC → ESP_RXD (pino 4)"
echo ""
echo "   2. 🔌 Feche o SimHub no PC (evita conflitos de porta)"
echo ""
echo "   3. 🔄 Entre em modo bootloader AGORA:"
echo "      a) Mantenha o pino BOOT (pino 6) do WT32 em GND"
echo "      b) Pressione e solte o pino EN (pino 5) do WT32"
echo "      c) Solte o pino BOOT"
echo "      d) O ESP32 agora está em modo bootloader (tempo limitado!)"
echo ""
read -p "   Você já entrou em modo bootloader? (s/n) " -n 1 -r
BOOTLOADER_REPLY="$REPLY"
echo ""

# Oferecer teste de bootloader
read -p "   Deseja testar se está em modo bootloader antes do upload? (s/n) " -n 1 -r
TEST_REPLY="$REPLY"
echo ""

if [[ $TEST_REPLY =~ ^[Ss]$ ]]; then
    echo ""
    echo "   🔍 Testando modo bootloader..."
    if ./testar-bootloader.sh "$DEBUG_PORT" 2>/dev/null; then
        echo ""
        echo "   ✅ Modo bootloader confirmado! Continuando com upload..."
        sleep 1
    else
        echo ""
        echo "   ⚠️  Não foi possível confirmar modo bootloader."
        echo "   Se você entrou em modo bootloader, pode continuar mesmo assim."
        read -p "   Continuar com upload? (s/n) " -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Ss]$ ]]; then
            echo "   Upload cancelado."
            exit 0
        fi
    fi
fi

if [[ ! $BOOTLOADER_REPLY =~ ^[Ss]$ ]]; then
    echo ""
    echo "   ⏸️  Pausando... Siga estes passos:"
    echo ""
    echo "   📌 Passo a passo para modo bootloader:"
    echo "      1. Localize o pino BOOT (pino 6) na interface de debug do WT32"
    echo "      2. Localize o pino EN (pino 5) na interface de debug do WT32"
    echo "      3. Conecte o pino BOOT ao GND (pino 7) - use um jumper ou fio"
    echo "      4. Pressione e solte o pino EN (reset)"
    echo "      5. Desconecte o BOOT do GND"
    echo ""
    echo "   💡 Dica: Se o ZXACC tem botões, pode usar:"
    echo "      - Segure o botão BOOT do ZXACC"
    echo "      - Pressione o botão RESET do ZXACC"
    echo "      - Solte o botão BOOT"
    echo ""
    read -p "   Pressione ENTER quando estiver pronto para continuar..."
    echo ""
fi
echo ""
echo "   🚀 Iniciando upload em 2 segundos..."
sleep 2
echo ""
echo ""
echo "   ⚠️  TROUBLESHOOTING - Se o upload falhar:"
echo ""
echo "   Erro 'Failed to write to target RAM':"
echo "   → TX/RX provavelmente invertidos"
echo "   → Tente inverter TX e RX"
echo ""
echo "   Erro 'No serial data received' (SEU ERRO ATUAL):"
echo "   → ⚠️  GND pode não estar conectado (MUITO COMUM!)"
echo "   → Verifique TODAS as conexões (GND, TX, RX, EN, BOOT)"
echo "   → ESP32 pode não estar em modo bootloader"
echo "   → Tente entrar em modo bootloader novamente"
echo ""
echo "   📖 Consulte TROUBLESHOOTING_ERROS_UPLOAD.md para mais detalhes"
echo ""

${PIO_CMD} run -e ${ENV} -t upload --upload-port ${DEBUG_PORT}

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Upload concluído com sucesso!"
    echo ""
    echo "💡 O WT32 pode permanecer conectado ao PC via USB."
    echo "   O SimHub deve continuar funcionando normalmente."
else
    echo ""
    echo "❌ Erro no upload!"
    echo ""
    echo "💡 Dicas para resolver:"
    echo "   1. Verifique todas as conexões do ZXACC-ESPDB V2:"
    echo "      - TX → ESP_RXD (pino 4)"
    echo "      - RX → ESP_TXD (pino 3)"
    echo "      - EN → EN (pino 5)"
    echo "      - BOOT/GPIO0 → BOOT (pino 6)"
    echo "      - GND → GND (pino 7)"
    echo ""
    echo "   2. NÃO conecte +5V do ZXACC ao WT32!"
    echo "      O WT32 já está alimentado via USB do PC."
    echo ""
    echo "   3. Tente entrar em modo bootloader manualmente:"
    echo "      - Mantenha BOOT (GPIO0) em GND"
    echo "      - Pressione e solte EN (reset)"
    echo "      - Solte BOOT"
    echo "      - Tente o upload novamente"
    echo ""
    echo "   4. Feche o SimHub durante o upload (evita conflitos)"
    exit 1
fi

