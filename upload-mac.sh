#!/bin/bash

# Script para compilar e fazer upload do firmware no Mac
# Uso: ./build-for-windows.sh

set -e

ENV="wt32-sc01-plus"

echo "🔨 Compilando firmware para WT32-SC01 Plus..."
echo ""

# Procurar PlatformIO em vários locais comuns no Mac
PIO_CMD=""

# 1. Verificar se está no PATH
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
elif command -v platformio &> /dev/null; then
    PIO_CMD="platformio"
# 2. Verificar instalação padrão do VS Code extension
elif [ -f ~/.platformio/penv/bin/pio ]; then
    PIO_CMD="$HOME/.platformio/penv/bin/pio"
# 3. Verificar instalação via pip no usuário
elif [ -f ~/.local/bin/pio ]; then
    PIO_CMD="$HOME/.local/bin/pio"
# 4. Verificar instalação via pip global
elif [ -f /usr/local/bin/pio ]; then
    PIO_CMD="/usr/local/bin/pio"
# 5. Tentar via python3 -m platformio (se instalado via pip)
elif python3 -m platformio --version &> /dev/null; then
    PIO_CMD="python3 -m platformio"
fi

# Se não encontrou, tentar instalar ou dar instruções
if [ -z "$PIO_CMD" ]; then
    echo "❌ Erro: PlatformIO não encontrado!"
    echo ""
    echo "📦 Opções para instalar:"
    echo ""
    echo "   1. Via VS Code (Recomendado):"
    echo "      - Instale a extensão 'PlatformIO IDE' no VS Code"
    echo "      - O PlatformIO será instalado automaticamente"
    echo ""
    echo "   2. Via pip:"
    echo "      pip install platformio"
    echo ""
    echo "   3. Via Homebrew:"
    echo "      brew install platformio"
    echo ""
    echo "   4. Via script oficial:"
    echo "      python3 -c \"\$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py)\""
    echo ""
    exit 1
fi

echo "✅ PlatformIO encontrado: $PIO_CMD"

echo "📋 Verificando dispositivos conectados..."
${PIO_CMD} device list
echo ""

echo "🔨 Compilando..."
${PIO_CMD} run -e ${ENV}

echo ""
echo "✅ Compilação concluída!"
echo ""

# Perguntar se deseja fazer upload
read -p "🚀 Deseja fazer upload para o dispositivo agora? (s/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Ss]$ ]]; then
    echo ""
    echo "📤 Fazendo upload..."
    ${PIO_CMD} run -e ${ENV} -t upload

    if [ $? -eq 0 ]; then
        echo ""
        echo "✅ Upload concluído com sucesso!"
        echo ""
        read -p "📺 Deseja abrir o monitor serial? (s/n) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Ss]$ ]]; then
            echo ""
            echo "📺 Abrindo monitor serial (Ctrl+] para sair)..."
            ${PIO_CMD} device monitor -e ${ENV}
        fi
    else
        echo ""
        echo "❌ Erro no upload!"
        echo "   Dica: Tente entrar em modo bootloader (segure BOOT, pressione RESET, solte BOOT)"
        exit 1
    fi
else
    echo ""
    echo "💡 Para fazer upload depois, execute:"
    echo "   ${PIO_CMD} run -e ${ENV} -t upload"
fi

