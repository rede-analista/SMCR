# 🚀 Guia para Release de Firmware no GitHub

## 📦 **Arquivos Preparados**

Sua pasta `/firmware/` contém:
- ✅ **SMCR_v2.0.0_firmware.bin** - Firmware principal (878 KB)
- ✅ **SMCR_v2.0.0_bootloader.bin** - Bootloader (17 KB)
- ✅ **SMCR_v2.0.0_partitions.bin** - Partições (3 KB)
- ✅ **README.md** - Documentação completa
- ✅ **flash_firmware.sh** - Script automático Linux/Mac
- ✅ **flash_firmware.bat** - Script automático Windows

## 🔄 **Passos para Criar Release**

### **1. Commit dos Arquivos**
```bash
git add firmware/
git commit -m "feat: adiciona firmware v2.0.0 pré-compilado com autenticação"
git push origin main
```

### **2. Criar Tag de Versão**
```bash
git tag v2.0.0 -m "SMCR v2.0.0 - Sistema com autenticação e interface moderna"
git push origin v2.0.0
```

### **3. Criar Release no GitHub**

1. Acesse: https://github.com/rede-analista/SMCR/releases
2. Clique em **"Create a new release"**
3. **Tag**: `v2.0.0`
4. **Title**: `SMCR v2.0.0 - Firmware com Sistema de Autenticação`

#### **Descrição da Release**:
```markdown
# 🚀 SMCR v2.0.0 - Sistema Modular de Controle e Recursos

## ✨ Principais Novidades

### 🔐 **Sistema de Autenticação**
- HTTP Basic Authentication configurável
- Dashboard público opcional (painel de apresentação)
- Proteção granular de páginas administrativas
- Configuração padrão: autenticação desabilitada

### 🎨 **Interface Moderna**  
- Design responsivo com Tailwind CSS
- Campos pré-preenchidos (incluindo senhas)
- Sistema de configuração dual (Ativa/Persistente)
- Interface 100% em português brasileiro

### ⚙️ **Configurações Avançadas**
- Porta do servidor web configurável
- Controle de autenticação por página
- Sistema NTP com fuso horário brasileiro
- Watchdog timer configurável

## 📦 **Como Instalar**

### **Opção 1: Gravação Automática**
1. Baixe os arquivos desta release
2. **Windows**: Execute `flash_firmware.bat`
3. **Linux/Mac**: Execute `./flash_firmware.sh`
4. Siga as instruções na tela

### **Opção 2: Gravação Manual**
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 SMCR_v2.0.0_firmware.bin
```

### **Primeira Configuração**
1. Conecte-se em: **SMCR_AP_SETUP** (senha: `senha12345678`)
2. Acesse: http://192.168.4.1:8080
3. Configure sua rede WiFi
4. Acesse configurações avançadas: http://[IP_DO_MODULO]:8080/configuracao

## 🔧 **Configurações Padrão**

- **Porta Web**: 8080
- **Autenticação**: Desabilitada
- **Usuário**: admin  
- **Senha**: admin123
- **Debug Serial**: Habilitado
- **Watchdog**: Desabilitado

## 📋 **Especificações Técnicas**

- **Plataforma**: ESP32 (Arduino Framework)
- **Flash**: ~890 KB utilizados
- **RAM**: ~46 KB utilizados
- **Pinos**: Até 254 configuráveis
- **Conectividade**: WiFi STA/AP com mDNS

## 🆘 **Suporte**

- 📖 **Documentação**: Veja pasta `/manual/` do repositório
- 🐛 **Problemas**: Abra uma issue no GitHub
- 💬 **Discussões**: Use GitHub Discussions

---

**Full Changelog**: https://github.com/rede-analista/SMCR/compare/v1.0.0...v2.0.0
```

---

## 📌 Modelo de Release v2.1.0 (Atual)

Use este modelo para a versão 2.1.0 contendo MQTT Discovery em lotes, NVS Export/Import, correções no menu Arquivos e base para renomeação inteligente de pinos (feature refinada pós-2.1.0).

### 1. Commit dos Arquivos
```bash
git add firmware/
git commit -m "release: v2.1.0 (MQTT discovery em lotes, NVS export/import, correções UI)"
git push origin main
```

### 2. Criar Tag de Versão
```bash
git tag v2.1.0 -m "SMCR v2.1.0 - MQTT HA discovery em lotes + NVS export/import + fixes"
git push origin v2.1.0
```

### 3. Criar Release no GitHub

1. Acesse: https://github.com/rede-analista/SMCR/releases
2. Clique em "Create a new release"
3. Tag: `v2.1.0`
4. Title: `SMCR v2.1.0 - MQTT HA em Lotes + NVS Export/Import`

#### Descrição da Release (cole no GitHub):
```markdown
# 🚀 SMCR v2.1.0 - MQTT HA em Lotes + NVS Export/Import

## ✨ Destaques

### 🧩 MQTT + Home Assistant (não bloqueante)
- Discovery do HA em lotes, com tamanho e intervalo configuráveis na UI.
- Tópicos numéricos (0/1, 0–4095) e reconexão não bloqueante.
- `LOG_MQTT` separado para depuração (checkbox na página).

### 🗃️ NVS: Exportar/Importar + Lista Completa
- Página Arquivos agora lista TODAS as namespaces e chaves (segredos mascarados).
- Exportar NVS (JSON/Text) com opção de incluir segredos.
- Importar NVS com `clear=1` para apagar namespaces antes de restaurar.
- Correção de crash ao abrir Arquivos (iteração NVS ajustada para partição "nvs").

### 🖥️ UI/UX e Robustez
- Dashboard exibe `SMCR - [HOSTNAME]` e no-cache aplicado para evitar layout antigo.
- Sanitização de hostname: somente A-Z/0-9, uppercase, sem espaços/acentos.
- Página MQTT mais leve: endpoints separados (`/api/mqtt/config` e `/api/mqtt/status`) e timeouts.

### 🔁 Renomeação Inteligente de Pinos (pós-2.1.0)
- A atualização do número de um pino na interface não exige remoção/recriação.
- Ações que usavam o pino como origem/destino são ajustadas automaticamente.
- Persistência automática: pinos sempre salvos; ações somente se houve renomeação.
- Evita perda acidental de lógica encadeada ao reorganizar GPIOs.

## 📦 Arquivos
- `SMCR_v2.1.0_firmware.bin`
- `SMCR_v2.1.0_bootloader.bin`
- `SMCR_v2.1.0_partitions.bin`
- Scripts de gravação (`flash_firmware.sh` / `.bat`)

## 🔧 Gravação Rápida
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x10000 SMCR_v2.1.0_firmware.bin
```

## 📚 Documentação
- Implementação: `manual/implementacoes/NVS_EXPORT_IMPORT.md`
- MQTT HA Batching: `manual/implementacoes/MQTT_DISCOVERY_BATCHING.md`

---
```

### 4. Anexar Arquivos (v2.1.0)
- `SMCR_v2.1.0_firmware.bin`
- `SMCR_v2.1.0_bootloader.bin`
- `SMCR_v2.1.0_partitions.bin`
- `flash_firmware.sh`
- `flash_firmware.bat`

### 5. Publicar Release
- Marque "Set as the latest release"
- Clique em "Publish release"

### 🔁 Script de Build
```bash
./build_release.sh 2.1.0
```

### **4. Anexar Arquivos**
Na seção **"Attach binaries"**, adicione:
- `SMCR_v2.0.0_firmware.bin`
- `SMCR_v2.0.0_bootloader.bin`  
- `SMCR_v2.0.0_partitions.bin`
- `flash_firmware.sh`
- `flash_firmware.bat`

### **5. Publicar Release**
- Marque **"Set as the latest release"**
- Clique em **"Publish release"**

## 🎊 **Resultado Final**

Os usuários poderão:
- ⬇️ **Baixar** firmware pré-compilado
- 🔧 **Gravar** com scripts automáticos
- 📱 **Usar** imediatamente sem compilar
- 📖 **Seguir** documentação completa

## 🔮 **Automação Futura**

Para automatizar releases:
```bash
# Use o script de build
./build_release.sh 2.1.0

# Cria automaticamente:
# - firmware/v2.1.0/
# - Todos arquivos versionados  
# - Checksums SHA256
# - Build info
```