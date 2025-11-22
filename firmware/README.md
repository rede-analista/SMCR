# 📦 SMCR - Firmware Pré-Compilado

Este diretório contém os arquivos de firmware pré-compilados do sistema SMCR para ESP32, prontos para gravação direta.

## 🔧 **Arquivos Necessários**

### **Opção 1: Gravação Simples (Recomendada)**
- **`SMCR_v2.0.0_firmware.bin`** - Arquivo principal do firmware (878 KB)

### **Opção 2: Gravação Completa (Para ESP32 novo/limpo)**  
- **`SMCR_v2.0.0_bootloader.bin`** - Bootloader do ESP32 (17 KB)
- **`SMCR_v2.0.0_partitions.bin`** - Tabela de partições (3 KB)  
- **`SMCR_v2.0.0_firmware.bin`** - Firmware principal (878 KB)

## 🚀 **Métodos de Instalação**

### **Método 1: ESP32 Flash Download Tool (Windows)**

1. **Baixe a ferramenta oficial**: [ESP32 Flash Download Tool](https://www.espressif.com/en/support/download/other-tools)

2. **Configure os endereços**:

#### **Gravação Simples** (ESP32 já tem bootloader):
```
Arquivo: SMCR_v2.0.0_firmware.bin
Endereço: 0x10000
```

#### **Gravação Completa** (ESP32 novo/limpo):
```
SMCR_v2.0.0_bootloader.bin  → 0x1000
SMCR_v2.0.0_partitions.bin  → 0x8000  
SMCR_v2.0.0_firmware.bin    → 0x10000
```

3. **Configurações**:
   - **SPI SPEED**: 40MHz
   - **SPI MODE**: DIO
   - **FLASH SIZE**: 4MB

### **Método 2: esptool (Linux/Mac/Windows)**

#### **Instalação do esptool**:
```bash
pip install esptool
```

#### **Gravação Simples**:
```bash
esptool.py --chip esp32 --port COM3 --baud 921600 write_flash 0x10000 SMCR_v2.0.0_firmware.bin
```

#### **Gravação Completa**:
```bash
esptool.py --chip esp32 --port COM3 --baud 921600 write_flash \
  0x1000 SMCR_v2.0.0_bootloader.bin \
  0x8000 SMCR_v2.0.0_partitions.bin \
  0x10000 SMCR_v2.0.0_firmware.bin
```

**⚠️ Substitua `COM3` pela porta correta do seu ESP32**

### **Método 3: PlatformIO (Para desenvolvedores)**

1. Clone o repositório:
```bash
git clone https://github.com/rede-analista/SMCR.git
cd SMCR
```

2. Compile e grave:
```bash
platformio run --target upload
```

## 🔌 **Preparação do ESP32**

### **Modo Download**:
1. Mantenha **BOOT** pressionado
2. Pressione **RESET** 
3. Solte **RESET**
4. Solte **BOOT**
5. ESP32 estará pronto para receber firmware

### **Identificação da Porta**:
- **Windows**: `COM3`, `COM4`, etc.
- **Linux**: `/dev/ttyUSB0`, `/dev/ttyACM0`
- **Mac**: `/dev/cu.usbserial-*`

## ⚙️ **Configuração Inicial**

### **Primeira Inicialização**:
1. O ESP32 iniciará em **Modo AP** (primeira vez)
2. Conecte-se à rede: `SMCR_AP_SETUP`
3. Senha: `senha12345678`
4. Acesse: `http://192.168.4.1:8080`
5. Configure WiFi e hostname

### **Configuração Avançada**:
- Acesse: `http://[IP_DO_MODULO]:8080/configuracao`
- Configure autenticação, pinos, watchdog, etc.

## 📋 **Especificações da Versão 2.0.0**

### **Funcionalidades**:
- ✅ **Interface Web Responsiva** (Tailwind CSS)
- ✅ **Sistema de Autenticação** HTTP Basic
- ✅ **Configuração Dual** (Ativa/Persistente)
- ✅ **NTP Automático** com fuso horário
- ✅ **Fallback AP** automático
- ✅ **Menu de Reset** abrangente
- ✅ **Watchdog Timer** configurável
- ✅ **Debug Serial** com flags
- ✅ **mDNS** para acesso por nome

### **Limites Técnicos**:
- **Pinos**: Até 254 configuráveis
- **Memória Flash**: ~890 KB utilizados
- **RAM**: ~46 KB utilizados  
- **Porta Web**: Configurável (padrão 8080)

### **Configurações Padrão**:
- **WiFi**: Modo AP inicial
- **Usuário**: admin
- **Senha**: admin123
- **Autenticação**: Desabilitada
- **Debug**: Habilitado
- **Watchdog**: Desabilitado

## 🆘 **Troubleshooting**

### **Erro de Gravação**:
- Verifique se ESP32 está em modo download
- Teste velocidades menores: 115200 baud
- Use cabos USB de boa qualidade

### **Não Conecta WiFi**:
- Acesse modo AP: `SMCR_AP_SETUP`
- Reconfigure credenciais WiFi
- Verifique força do sinal

### **Erro de Autenticação**:
- Use credenciais: `admin` / `admin123`
- Ou desabilite autenticação via configuração

## 📞 **Suporte**

- **Repositório**: [github.com/rede-analista/SMCR](https://github.com/rede-analista/SMCR)
- **Issues**: Reporte problemas via GitHub Issues
- **Documentação**: Veja pasta `/manual/` do repositório

---
**SMCR v2.0.0** - Sistema Modular de Controle e Recursos  
Desenvolvido para ESP32 - Framework Arduino  
© 2025 - Licença MIT