#!/bin/bash

echo "========================================"
echo "     SMCR ESP32 Firmware Flasher"
echo "========================================"
echo

# Verifica se os arquivos existem
if [ ! -f "SMCR_v2.0.0_firmware.bin" ]; then
    echo "❌ Erro: Arquivo SMCR_v2.0.0_firmware.bin não encontrado!"
    echo "   Baixe os arquivos de firmware do GitHub primeiro."
    exit 1
fi

echo "📋 Arquivos encontrados:"
ls -la SMCR_v*.bin 2>/dev/null
echo

echo "🔍 Detectando portas USB..."
echo
echo "Portas disponíveis:"
if [ "$(uname)" = "Darwin" ]; then
    # macOS
    ls /dev/cu.usbserial* /dev/cu.usbmodem* 2>/dev/null | head -5
else
    # Linux
    ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -5
fi

echo
read -p "Digite a porta do ESP32 (ex: /dev/ttyUSB0): " PORTA

if [ -z "$PORTA" ]; then
    echo "❌ Porta não informada!"
    exit 1
fi

echo
echo "⚠️  PREPARAÇÃO DO ESP32:"
echo "   1. Mantenha BOOT pressionado"
echo "   2. Pressione RESET"
echo "   3. Solte RESET"
echo "   4. Solte BOOT"
echo "   5. ESP32 estará em modo download"
echo
read -p "Pressione ENTER quando ESP32 estiver pronto..."

echo
echo "🚀 Iniciando gravação do firmware..."
echo "   Porta: $PORTA"
echo "   Arquivo: SMCR_v2.0.0_firmware.bin"
echo

# Verifica se esptool está instalado
if ! command -v esptool.py &> /dev/null; then
    echo "❌ esptool.py não encontrado!"
    echo "   Instale com: pip install esptool"
    exit 1
fi

# Grava firmware
esptool.py --chip esp32 --port "$PORTA" --baud 921600 write_flash 0x10000 SMCR_v2.0.0_firmware.bin

if [ $? -eq 0 ]; then
    echo
    echo "✅ Firmware gravado com sucesso!"
    echo
    echo "🔌 PRIMEIRO ACESSO:"
    echo "   1. Pressione RESET no ESP32"
    echo "   2. Conecte-se na rede WiFi: SMCR_AP_SETUP"
    echo "   3. Senha: senha12345678"
    echo "   4. Acesse: http://192.168.4.1:8080"
    echo "   5. Configure sua rede WiFi"
    echo
else
    echo
    echo "❌ Erro na gravação!"
    echo
    echo "🔧 Soluções:"
    echo "   - Verifique se ESP32 está em modo download"
    echo "   - Confirme a porta correta"
    echo "   - Teste com velocidade menor: 115200"
    echo "   - Use cabo USB de qualidade"
    echo
    echo "💾 Comando manual:"
    echo "esptool.py --chip esp32 --port $PORTA --baud 115200 write_flash 0x10000 SMCR_v2.0.0_firmware.bin"
    echo
fi