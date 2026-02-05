#!/bin/bash

# Script para compilar e fazer upload do firmware no Mac
# Uso: ./build-for-windows.sh

set -e

ENV="wt32-sc01-plus"

echo "🔨 Compilando firmware para WT32-SC01 Plus..."
echo ""

# Verificar se PlatformIO está instalado
if ! command -v pio &> /dev/null && ! command -v platformio &> /dev/null; then
    echo "❌ Erro: PlatformIO não encontrado!"
    echo "   Instale o PlatformIO: https://platformio.org/install/cli"
    exit 1
fi

# Compilar
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
else
    PIO_CMD="platformio"
fi

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

