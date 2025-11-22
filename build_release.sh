#!/bin/bash

# SMCR Build Release Script
# Compila e prepara arquivos para release no GitHub

echo "🚀 SMCR - Build Release Script"
echo "================================"

# Verifica se estamos no diretório correto
if [ ! -f "platformio.ini" ]; then
    echo "❌ Erro: Execute este script na raiz do projeto SMCR"
    exit 1
fi

# Lê a versão do arquivo platformio.ini ou define padrão
VERSION=${1:-"2.0.0"}
echo "📦 Versão: v$VERSION"

# Limpa build anterior
echo "🧹 Limpando build anterior..."
platformio run --target clean

# Compila o projeto
echo "🔨 Compilando projeto..."
platformio run

# Verifica se compilação foi bem sucedida
if [ $? -ne 0 ]; then
    echo "❌ Erro na compilação!"
    exit 1
fi

# Cria diretório de release
RELEASE_DIR="firmware/v$VERSION"
mkdir -p "$RELEASE_DIR"

# Copia arquivos compilados
echo "📁 Copiando arquivos para $RELEASE_DIR..."
cp .pio/build/smcr/firmware.bin "$RELEASE_DIR/SMCR_v${VERSION}_firmware.bin"
cp .pio/build/smcr/bootloader.bin "$RELEASE_DIR/SMCR_v${VERSION}_bootloader.bin"  
cp .pio/build/smcr/partitions.bin "$RELEASE_DIR/SMCR_v${VERSION}_partitions.bin"

# Cria arquivo de informações da build
cat > "$RELEASE_DIR/build_info.txt" << EOF
SMCR Firmware v$VERSION
Build Date: $(date)
Git Commit: $(git rev-parse HEAD 2>/dev/null || echo "N/A")
Git Branch: $(git branch --show-current 2>/dev/null || echo "N/A")

Files:
- SMCR_v${VERSION}_firmware.bin ($(stat -c%s "$RELEASE_DIR/SMCR_v${VERSION}_firmware.bin" | numfmt --to=iec)B)
- SMCR_v${VERSION}_bootloader.bin ($(stat -c%s "$RELEASE_DIR/SMCR_v${VERSION}_bootloader.bin" | numfmt --to=iec)B)  
- SMCR_v${VERSION}_partitions.bin ($(stat -c%s "$RELEASE_DIR/SMCR_v${VERSION}_partitions.bin" | numfmt --to=iec)B)

Flash Command (esptool):
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash \\
  0x1000 SMCR_v${VERSION}_bootloader.bin \\
  0x8000 SMCR_v${VERSION}_partitions.bin \\
  0x10000 SMCR_v${VERSION}_firmware.bin

Simple Flash (firmware only):
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x10000 SMCR_v${VERSION}_firmware.bin
EOF

# Calcula checksums
echo "🔐 Calculando checksums..."
cd "$RELEASE_DIR"
sha256sum *.bin > checksums.sha256
cd - > /dev/null

# Exibe resumo
echo "✅ Build concluída com sucesso!"
echo ""
echo "📋 Resumo:"
echo "  Versão: v$VERSION"
echo "  Diretório: $RELEASE_DIR"
echo "  Arquivos:"
ls -lah "$RELEASE_DIR"
echo ""
echo "🚀 Para criar release no GitHub:"
echo "  1. Commit e push dos arquivos"
echo "  2. Crie uma tag: git tag v$VERSION"
echo "  3. Push da tag: git push origin v$VERSION"
echo "  4. Crie release no GitHub anexando os arquivos .bin"
echo ""
echo "💾 Comando de gravação rápida:"
echo "esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x10000 $RELEASE_DIR/SMCR_v${VERSION}_firmware.bin"