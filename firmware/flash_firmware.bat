@echo off
echo ========================================
echo     SMCR ESP32 Firmware Flasher
echo ========================================
echo.

REM Verifica se os arquivos existem
if not exist "SMCR_v2.0.0_firmware.bin" (
    echo ❌ Erro: Arquivo SMCR_v2.0.0_firmware.bin nao encontrado!
    echo    Baixe os arquivos de firmware do GitHub primeiro.
    pause
    exit /b 1
)

echo 📋 Arquivos encontrados:
dir /b SMCR_v*.bin 2>nul
echo.

echo 🔍 Detectando porta ESP32...
echo.
echo Portas COM disponíveis:
for /f "skip=1" %%p in ('wmic path win32_serialport get deviceid^,description') do echo   %%p

echo.
set /p PORTA="Digite a porta COM do ESP32 (ex: COM3): "

if "%PORTA%"=="" (
    echo ❌ Porta nao informada!
    pause
    exit /b 1
)

echo.
echo ⚠️  PREPARACAO DO ESP32:
echo    1. Mantenha BOOT pressionado
echo    2. Pressione RESET
echo    3. Solte RESET
echo    4. Solte BOOT
echo    5. ESP32 estara em modo download
echo.
pause

echo.
echo 🚀 Iniciando gravacao do firmware...
echo    Porta: %PORTA%
echo    Arquivo: SMCR_v2.0.0_firmware.bin
echo.

REM Tenta usar esptool.py
esptool.py --chip esp32 --port %PORTA% --baud 921600 write_flash 0x10000 SMCR_v2.0.0_firmware.bin

if %errorlevel% equ 0 (
    echo.
    echo ✅ Firmware gravado com sucesso!
    echo.
    echo 🔌 PRIMEIRO ACESSO:
    echo    1. Pressione RESET no ESP32
    echo    2. Conecte-se na rede WiFi: SMCR_AP_SETUP
    echo    3. Senha: senha12345678
    echo    4. Acesse: http://192.168.4.1:8080
    echo    5. Configure sua rede WiFi
    echo.
) else (
    echo.
    echo ❌ Erro na gravacao!
    echo.
    echo 🔧 Soluções:
    echo    - Verifique se ESP32 esta em modo download
    echo    - Confirme a porta COM correta
    echo    - Teste com velocidade menor: 115200
    echo    - Use cabo USB de qualidade
    echo.
    echo 💾 Comando manual:
    echo esptool.py --chip esp32 --port %PORTA% --baud 115200 write_flash 0x10000 SMCR_v2.0.0_firmware.bin
    echo.
)

pause