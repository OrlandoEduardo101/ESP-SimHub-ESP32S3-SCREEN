#!/bin/bash

# Script para testar se o ESP32 está em modo bootloader
# Uso: ./testar-bootloader.sh [porta]

# Tentar encontrar o comando pio
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
    echo "❌ Erro: PlatformIO não encontrado!"
    exit 1
fi

# Se porta não foi fornecida, listar dispositivos
if [ -z "$1" ]; then
    echo "📋 Dispositivos disponíveis:"
    echo ""
    ${PIO_CMD} device list
    echo ""
    echo "💡 Uso: ./testar-bootloader.sh /dev/cu.usbmodem58370635041"
    exit 0
fi

PORT="$1"

echo "🔍 Testando se ESP32 está em modo bootloader..."
echo "   Porta: $PORT"
echo ""

# Tentar usar esptool.py diretamente (mais confiável)
if command -v esptool.py &> /dev/null; then
    echo "📡 Tentando conectar ao ESP32 usando esptool.py..."
    echo ""

    # Tentar ler informações do chip
    OUTPUT=$(esptool.py --port "$PORT" --baud 115200 chip_id 2>&1)

    if echo "$OUTPUT" | grep -q "Chip is ESP32"; then
        echo "✅ SUCESSO! ESP32 detectado e respondendo!"
        echo ""
        echo "$OUTPUT" | grep -E "(Chip is|Features|Crystal|MAC)"
        echo ""
        echo "✅ Isso indica que está em modo bootloader ou pronto para upload!"
        echo "   Você pode tentar fazer upload agora."
        exit 0
    else
        echo "❌ Não foi possível detectar o chip."
        echo ""
        echo "Saída do esptool:"
        echo "$OUTPUT"
        echo ""
        echo "💡 Possíveis causas:"
        echo "   1. ESP32 não está em modo bootloader"
        echo "   2. Conexões TX/RX incorretas (tente inverter)"
        echo "   3. Porta serial incorreta"
        echo "   4. GND não conectado"
        echo "   5. EN ou BOOT não conectados corretamente"
        echo ""
        echo "🔄 Tente entrar em modo bootloader novamente:"
        echo "   1. Conecte BOOT (pino 6) ao GND (pino 7)"
        echo "   2. Pressione e solte EN (pino 5)"
        echo "   3. Desconecte BOOT do GND"
        echo "   4. Execute este script novamente IMEDIATAMENTE"
        exit 1
    fi
else
    echo "⚠️  esptool.py não encontrado no PATH."
    echo "   Tentando via PlatformIO..."
    echo ""

    # Tentar via PlatformIO (menos confiável, mas funciona)
    if ${PIO_CMD} run -e wt32-sc01-plus-debug -t upload --upload-port "$PORT" --dry-run 2>&1 | grep -q "Chip is ESP32"; then
        echo "✅ Chip detectado via PlatformIO!"
        echo "   Isso indica que está em modo bootloader ou pronto."
    else
        echo "❌ Não foi possível detectar o chip."
        echo ""
        echo "💡 Instale esptool.py para teste mais confiável:"
        echo "   pip install esptool"
        echo ""
        echo "Ou tente fazer upload diretamente - se começar, está OK."
    fi
fi

echo ""
echo "📊 Informações:"
echo "   - Modo bootloader não tem feedback visual (LED)"
echo "   - O único jeito de confirmar é tentar comunicação"
echo "   - Se o esptool conseguir ler informações do chip, está OK"
echo ""

