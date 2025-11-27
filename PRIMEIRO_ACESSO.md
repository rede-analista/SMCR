# 🚀 SMCR - Guia de Primeiro Acesso e Configuração

## 📋 Pré-requisitos

- Firmware SMCR compilado (arquivo `.bin`)
- Ferramenta de gravação ESP32 (esptool, PlatformIO, etc.)
- Arquivos HTML da pasta `data/` (11 arquivos)
- Computador com WiFi

---

## 🔧 Etapa 1: Gravação do Firmware

### Opção A: Usando PlatformIO (Recomendado)

```bash
# No diretório do projeto
platformio run --target upload --upload-port /dev/ttyUSB0
```

> **Nota:** Ajuste `/dev/ttyUSB0` conforme a porta USB do seu ESP32
> - Linux: `/dev/ttyUSB0`, `/dev/ttyUSB1`, etc.
> - Windows: `COM3`, `COM4`, etc.
> - Mac: `/dev/cu.usbserial-*`

### Opção B: Usando esptool.py

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 firmware.bin
```

### Opção C: Usando script automatizado

**Linux/Mac:**
```bash
cd firmware/
chmod +x flash_firmware.sh
./flash_firmware.sh
```

**Windows:**
```cmd
cd firmware
flash_firmware.bat
```

---

## 📡 Etapa 2: Primeira Conexão (Modo Access Point)

Após gravar o firmware, o ESP32 iniciará em **Modo Access Point**.

### 2.1 Conectar no WiFi do ESP32

1. **Procure a rede WiFi:**
   - Nome: `esp32modularx Ponto de Acesso`
   - Senha: `senha12345678`

2. **Conecte seu computador/celular** nesta rede

3. **Aguarde** a conexão ser estabelecida (LED do ESP32 pode piscar)

### 2.2 Acessar Interface de Configuração

1. Abra o navegador e acesse:
   ```
   http://192.168.4.1:8080
   ```

2. Você verá a **página de configuração inicial** (modo AP)

### 2.3 Configurar Rede WiFi

Na página de configuração, preencha:

| Campo | Descrição | Exemplo |
|-------|-----------|---------|
| **Hostname** | Nome do dispositivo na rede | `esp32modular` |
| **SSID** | Nome da sua rede WiFi | `MinhaRede` |
| **Senha WiFi** | Senha da sua rede | `MinhaSenh@123` |

3. Clique em **"Salvar e Reiniciar"**

4. **Aguarde ~30 segundos** para o ESP32 reiniciar e conectar na sua rede

---

## 🌐 Etapa 3: Acesso em Modo Estação (STA)

Após a configuração, o ESP32 conectará na sua rede WiFi.

### 3.1 Descobrir o IP do ESP32

**Opção A: Usando mDNS (recomendado)**
```
http://esp32modularx.local:8080
```
> Substitua `esp32modularx` pelo hostname configurado

**Opção B: Verificar no roteador**
- Acesse o painel do seu roteador
- Procure por dispositivos conectados
- Localize o ESP32 pelo hostname ou MAC address

**Opção C: Usar scanner de rede**
```bash
# Linux
nmap -p 8080 192.168.1.0/24

# Ou use app móvel como "Fing" ou "Network Scanner"
```

### 3.2 Acessar Dashboard Inicial

1. Acesse o endereço descoberto:
   ```
   http://esp32modularx.local:8080
   ```

2. Você verá uma **página de aviso** indicando que os arquivos HTML precisam ser carregados:

   ```
   ⚠️ Configuração Inicial Necessária
   As páginas HTML ainda não foram carregadas no sistema de arquivos.
   ```

3. Clique no botão: **"📂 Acessar Gerenciador de Arquivos"**

---

## 📤 Etapa 4: Upload dos Arquivos HTML

### 4.1 Acessar Interface LittleFS

Você será redirecionado para:
```
http://esp32modularx.local:8080/arquivos/littlefs
```

### 4.2 Preparar Arquivos para Upload

Os arquivos estão na pasta `data/` do projeto:

```
data/
├── web_dashboard.html      (24 KB)
├── web_pins.html            (37 KB)
├── web_actions.html         (32 KB)
├── web_config_gerais.html   (31 KB)
├── web_files.html           (26 KB)
├── web_mqtt.html            (20 KB)
├── web_reset.html           (19 KB)
├── web_littlefs.html        (18 KB)
├── web_preferencias.html    (18 KB)
├── web_firmware.html        (12 KB)
└── web_intermod.html        (10 KB)
```

**Total:** 11 arquivos (~247 KB)

### 4.3 Fazer Upload dos Arquivos

1. Na interface LittleFS, localize a seção **"📤 Upload de Arquivos"**

2. Para cada arquivo:
   - Clique em **"Escolher arquivo"**
   - Selecione um arquivo HTML da pasta `data/`
   - Clique em **"📤 Fazer Upload"**
   - Aguarde a confirmação "Upload realizado com sucesso!"

3. **Repita o processo** para todos os 11 arquivos

### 4.4 Verificar Upload

Após fazer upload de todos os arquivos:

1. Clique em **"🔄 Recarregar Lista"**
2. Verifique se os 11 arquivos aparecem na lista:
   - `web_dashboard.html`
   - `web_pins.html`
   - `web_actions.html`
   - `web_config_gerais.html`
   - `web_files.html`
   - `web_mqtt.html`
   - `web_reset.html`
   - `web_littlefs.html`
   - `web_preferencias.html`
   - `web_firmware.html`
   - `web_intermod.html`

---

## ✅ Etapa 5: Sistema Pronto!

### 5.1 Acessar Dashboard Completo

1. Volte para a página inicial:
   ```
   http://esp32modularx.local:8080/
   ```

2. Agora você verá o **Dashboard completo** com:
   - Status do sistema
   - Informações de WiFi
   - Uptime
   - Memória livre
   - Status NTP

### 5.2 Navegar pelas Páginas

O sistema possui 12 páginas funcionais:

#### Menu Principal
- **Status** (`/`) - Dashboard com informações do sistema
- **Configurações** (`/configuracao`) - Configurações gerais
- **Pinos** (`/pinos`) - Configuração de GPIOs
- **Ações** (`/acoes`) - Automações e regras

#### Menu Serviços ▾
- **MQTT** (`/mqtt`) - Configuração do broker MQTT
- **Inter-Módulos** (`/intermod`) - Comunicação entre módulos

#### Menu Arquivos ▾
- **Firmware** (`/arquivos/firmware`) - Atualização OTA
- **Preferências** (`/arquivos/preferencias`) - Backup/restore NVS
- **LittleFS** (`/arquivos/littlefs`) - Gerenciamento de arquivos

#### Menu Util ▾
- **Web Serial** (`/serial`) - Monitor serial via web
- **Reset** (`/reset`) - Opções de reset do sistema

---

## 🔒 Etapa 6: Configuração de Segurança (Opcional)

### 6.1 Ativar Autenticação HTTP

1. Acesse **Configurações** (`/configuracao`)
2. Role até **"Autenticação Web"**
3. Marque **"Habilitar Autenticação"**
4. Defina:
   - **Usuário:** (ex: `admin`)
   - **Senha:** (mínimo 8 caracteres)
5. Clique em **"Salvar Configurações"**
6. O ESP32 reiniciará
7. Nas próximas conexões, será solicitado usuário/senha

### 6.2 Alterar Porta do Servidor Web

1. Em **Configurações** → **"Configurações de Rede"**
2. Altere **"Porta do Servidor Web"** (padrão: 8080)
3. Clique em **"Salvar Configurações"**
4. Acesse o novo endereço: `http://esp32modularx.local:NOVA_PORTA`

---

## 🛠️ Configurações Avançadas

### Configurar NTP (Sincronização de Hora)

1. Acesse **Configurações** (`/configuracao`)
2. Em **"Servidor NTP"**, configure:
   - **Servidor Primário:** `pool.ntp.br` (Brasil)
   - **Offset GMT:** `-10800` (Brasília = GMT-3)
   - **Offset Horário de Verão:** `0` ou `3600`
3. Salve e reinicie

### Configurar MQTT

1. Acesse **MQTT** (`/mqtt`)
2. Configure:
   - **Habilitar MQTT:** Sim
   - **Servidor:** IP ou hostname do broker
   - **Porta:** 1883 (padrão)
   - **Usuário/Senha:** (se necessário)
   - **Tópico Base:** `smcr`
3. Salve e reinicie

### Configurar Pinos/GPIOs

1. Acesse **Pinos** (`/pinos`)
2. Clique em **"➕ Adicionar Pino"**
3. Configure:
   - **Número do Pino:** GPIO (0-39)
   - **Nome:** Identificação
   - **Modo:** Entrada/Saída
   - **Nível de Acionamento:** Alto/Baixo
4. Salve a configuração

---

## 🚨 Solução de Problemas

### Problema: Não consigo conectar no AP do ESP32

**Soluções:**
1. Verifique se o LED está piscando (indica modo AP)
2. Pressione o botão RESET do ESP32
3. Verifique a senha: `senha12345678`
4. Tente desligar e ligar o WiFi do computador

### Problema: ESP32 não conecta na minha rede WiFi

**Soluções:**
1. Verifique se o SSID e senha estão corretos
2. Certifique-se que é uma rede **2.4 GHz** (ESP32 não suporta 5 GHz)
3. Verifique se o roteador permite novos dispositivos
4. Refaça a configuração no modo AP

### Problema: Não encontro o IP do ESP32

**Soluções:**
1. Use mDNS: `http://hostname.local:8080`
2. Verifique dispositivos conectados no roteador
3. Use um scanner de rede (Fing, nmap)
4. Conecte um cabo serial e veja o IP no monitor serial

### Problema: Páginas aparecem em branco

**Causa:** Arquivos HTML não foram carregados no LittleFS

**Solução:**
1. Acesse diretamente: `http://IP:8080/arquivos/littlefs`
2. Faça upload dos 11 arquivos HTML
3. Recarregue as páginas

### Problema: Upload de arquivo falha

**Soluções:**
1. Verifique se o arquivo é `.html`
2. Tente arquivos menores primeiro
3. Aguarde o upload completar antes do próximo
4. Verifique espaço disponível no LittleFS
5. Se necessário, formate o LittleFS e tente novamente

### Problema: Sistema lento ou travando

**Soluções:**
1. Verifique uso de memória no dashboard
2. Reduza número de pinos configurados
3. Desabilite MQTT se não estiver usando
4. Faça um reset de configurações preservando rede

---

## 📊 Verificação de Status

Após configuração completa, verifique no Dashboard:

- ✅ **WiFi Status:** Conectado
- ✅ **IP Local:** Atribuído pelo roteador
- ✅ **Tempo de Atividade:** Contando
- ✅ **Memória Livre:** > 100 KB
- ✅ **Status NTP:** Sincronizado
- ✅ **Versão Firmware:** 2.1.0

---

## 📝 Backup e Restauração

### Fazer Backup das Configurações

1. Acesse **Preferências** (`/arquivos/preferencias`)
2. Marque **"Incluir segredos (senha/token)"** se desejar
3. Clique em **"⬇️ Exportar NVS (JSON)"**
4. Salve o arquivo `nvs_export.json`

### Restaurar Configurações

1. Acesse **Preferências** (`/arquivos/preferencias`)
2. Em **"Importar NVS de arquivo JSON"**
3. Selecione o arquivo de backup
4. Marque **"Apagar namespaces antes de importar"** se quiser reset completo
5. Clique em **"⤴️ Importar NVS (JSON)"**
6. Aguarde confirmação e reinicie

---

## 🔄 Atualização de Firmware (OTA)

1. Compile nova versão do firmware
2. Acesse **Firmware** (`/arquivos/firmware`)
3. Clique em **"Escolher arquivo"**
4. Selecione o arquivo `.bin`
5. Clique em **"Enviar e Atualizar"**
6. **NÃO DESLIGUE** durante a atualização
7. Aguarde reinicialização automática (~30 segundos)

---

## 📞 Suporte

- **Repositório:** [github.com/rede-analista/SMCR](https://github.com/rede-analista/SMCR)
- **Documentação Completa:** Veja pasta `manual/`
- **Issues:** Abra uma issue no GitHub para reportar problemas

---

## 📄 Resumo Rápido

```
1. Gravar firmware no ESP32
2. Conectar no WiFi "esp32modularx Ponto de Acesso" (senha: senha12345678)
3. Acessar http://192.168.4.1:8080
4. Configurar hostname e WiFi da sua rede
5. Acessar http://hostname.local:8080
6. Clicar no botão para acessar LittleFS
7. Fazer upload dos 11 arquivos HTML da pasta data/
8. Sistema pronto! 🎉
```

---

**Última atualização:** 27 de novembro de 2025  
**Versão do Firmware:** 2.1.0  
**Flash Utilizado:** 74.2% (~973 KB)  
**Espaço Livre:** 330 KB
