# SMCR - Firmware Pré-Compilado

Este diretório contém os arquivos de firmware pré-compilados do sistema SMCR para ESP32, prontos para gravação direta.

## Arquivos de cada versão

Cada pasta de versão contém 3 arquivos necessários para gravação completa em ESP32 novo:

| Arquivo | Endereço | Descrição |
|---------|----------|-----------|
| `SMCR_vX.X.X_bootloader.bin` | `0x1000` | Bootloader do ESP32 |
| `SMCR_vX.X.X_partitions.bin` | `0x8000` | Tabela de partições |
| `SMCR_vX.X.X_firmware.bin` | `0x10000` | Firmware principal |

> Para atualizar um ESP32 que já tem o SMCR gravado, use apenas o `firmware.bin` via OTA pela interface web (`/arquivos/firmware`). Os 3 arquivos são necessários apenas na primeira gravação.

---

## Método recomendado: esptool-js (navegador, sem instalação)

Funciona em Chrome ou Edge. Não precisa instalar nada.

**Acesse:** https://espressif.github.io/esptool-js/

### Passo a passo

**1. Preparar o ESP32 para gravação (modo download)**

1. Mantenha o botão **BOOT** pressionado
2. Pressione e solte o botão **RESET**
3. Solte o botão **BOOT**
4. O ESP32 está pronto para receber firmware

> Alguns modelos entram em modo download automaticamente ao conectar. Se falhar, repita os passos acima.

**2. Conectar ao esptool-js**

1. Abra https://espressif.github.io/esptool-js/ no Chrome ou Edge
2. Clique em **Connect**
3. Selecione a porta USB do ESP32 na lista e clique em **Connect**

**3. Configurar os arquivos**

Na seção **Flash**, configure os 3 arquivos com seus respectivos endereços:

| Campo "Flash Address" | Arquivo |
|-----------------------|---------|
| `0x1000` | `SMCR_vX.X.X_bootloader.bin` |
| `0x8000` | `SMCR_vX.X.X_partitions.bin` |
| `0x10000` | `SMCR_vX.X.X_firmware.bin` |

Use o botão **Add File** para adicionar as 3 linhas. Substitua `X.X.X` pela versão desejada (use sempre a mais recente).

**4. Gravar**

1. Clique em **Program**
2. Aguarde a conclusão (acompanhe o log na tela)
3. Ao concluir, pressione **RESET** no ESP32

---

## Outros métodos

### esptool.py (linha de comando)

```bash
pip install esptool

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash \
  0x1000  SMCR_vX.X.X_bootloader.bin \
  0x8000  SMCR_vX.X.X_partitions.bin \
  0x10000 SMCR_vX.X.X_firmware.bin
```

> No Windows substitua `/dev/ttyUSB0` por `COM3` (ou a porta correspondente).

### PlatformIO (desenvolvedores)

```bash
git clone https://github.com/rede-analista/SMCR.git
cd SMCR
platformio run --target upload
```

---

## Primeiro acesso após gravação

1. O ESP32 inicia em **Modo AP**
2. Conecte-se à rede WiFi: `SMCR_AP_SETUP` — senha: `senha12345678`
3. Acesse no navegador: `http://192.168.4.1:8080`
4. Configure o nome da rede WiFi (SSID) e a senha
5. O ESP32 reinicia e conecta na sua rede
6. Acesse pelo hostname: `http://esp32modularx.local:8080`

Após isso, faça upload dos arquivos HTML pela interface web em **Arquivos > LittleFS**.

---

## Troubleshooting

**Porta não aparece na lista do esptool-js**
- Verifique se o cabo USB suporta dados (não apenas carga)
- Instale o driver CP210x ou CH340 conforme o chip USB do seu ESP32
- Tente outra porta USB do computador

**Erro durante a gravação**
- Repita o procedimento de entrar em modo download (BOOT + RESET)
- Tente velocidade menor: altere o campo **Baud Rate** para `115200`

**ESP32 não conecta no WiFi após gravação**
- Conecte no AP `SMCR_AP_SETUP` e reconfigure as credenciais

---

Repositório: https://github.com/rede-analista/SMCR
