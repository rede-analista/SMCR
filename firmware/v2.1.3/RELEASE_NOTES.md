# 🚀 SMCR v2.1.3 - Correções Críticas para Pinos Remotos Analógicos

## 📅 Data de Lançamento
4 de dezembro de 2025

## 🐛 Correções de Bugs Críticos

### **Bug #1: Overflow na Transmissão de Valores Analógicos**
- **Problema**: Parâmetro `uint8_t` truncava valores de 0-4095 para 0-255
- **Exemplo**: Valor 2535 era recebido como 159 (2535 & 0xFF)
- **Solução**: Alterado para `uint16_t` no endpoint `/api/pin/set`
- **Arquivo**: `src/servidor_web.cpp` (linha 975)

### **Bug #2: Overflow nos Logs de Comunicação**
- **Problema**: `InterModCommLog_t.value` também usava `uint8_t`
- **Impacto**: Dashboard mostrava valores truncados no histórico
- **Solução**: Alterado struct e funções para `uint16_t`
- **Arquivos**: `include/globals.h`, `src/utils.cpp`

### **Bug #3: Lixo de Memória no Histórico**
- **Problema**: Arrays `historico_analogico[]` não inicializados
- **Sintoma**: Valores impossíveis como 17399, 55264 no histórico
- **Solução**: Inicialização completa com zeros em `fV_initPinSystem()`
- **Arquivo**: `src/pin_manager.cpp` (linhas 58-77)

### **Bug #4: Leitura Incorreta de Pinos OUTPUT**
- **Problema**: `digitalRead()` em pinos OUTPUT retornava buffer (1023)
- **Impacto**: Dashboard mostrava valores errados para saídas digitais
- **Solução**: `digitalRead()` seletivo apenas para modo INPUT
- **Arquivo**: `src/pin_manager.cpp` (linhas 513-522)

### **Bug #5: Ações Não Disparavam com Pinos Remotos Analógicos**
- **Problema**: `fB_isPinActivated()` não reconhecia tipo 65533
- **Impacto**: Ações configuradas com pinos remotos nunca executavam
- **Solução**: Adicionado case para tipo 65533 com verificação de faixa
- **Arquivo**: `src/pin_manager.cpp` (linhas 553-562)

### **Bug #6: Factory Reset Incompleto**
- **Problema**: Reset apagava apenas NVS, arquivos LittleFS persistiam
- **Impacto**: Configurações de pinos/ações permaneciam após reset
- **Solução**: Implementado `LittleFS.format()` para limpeza completa
- **Arquivo**: `src/servidor_web.cpp` (linhas 2398-2420)

## ✨ Novas Funcionalidades

### **Endpoint GET para Factory Reset**
- **URL**: `/api/reset/factory?confirm=yes`
- **Método**: GET (permite automação com curl/scripts)
- **Segurança**: Requer parâmetro `confirm=yes` explícito
- **Uso**: Ideal para testes automatizados e reset rápido

**Exemplo de uso:**
```bash
curl "http://192.168.1.100:8080/api/reset/factory?confirm=yes"
```

### **Histórico para Pinos Remotos**
- Pinos remotos analógicos (tipo 65533) agora mantêm histórico
- Pinos remotos digitais (tipo 65534) também registram mudanças
- Dashboard exibe histórico completo com timestamp

## 📊 Tipos de Pinos Suportados

| Tipo   | Descrição              | Faixa de Valores | Histórico |
|--------|------------------------|------------------|-----------|
| 0      | Não usado              | -                | ❌        |
| 1      | Digital GPIO           | 0-1              | ✅        |
| 192    | Analógico ADC          | 0-4095           | ✅        |
| 193    | PWM Output             | 0-255            | ❌        |
| 65533  | Remoto Analógico       | 0-4095           | ✅        |
| 65534  | Remoto Digital         | 0-1              | ✅        |

## 🔧 Especificações Técnicas

### Uso de Memória
- **Flash**: 87.7% (1.149.465 bytes / 1.310.720 bytes)
- **RAM**: 16.1% (52.768 bytes / 327.680 bytes)
- **Incremento**: +532 bytes vs v2.1.2

### Alterações de Tipos
- ⚠️ **Breaking Change**: `uint8_t` → `uint16_t` em API e logs
- ✅ **Compatibilidade**: Módulos devem usar v2.1.3+ para valores completos
- ⚠️ **Migração**: Reconfigurar pinos remotos após atualização

## 📦 Arquivos da Release

- **SMCR_v2.1.3_firmware.bin** (1.2 MB) - Firmware principal
- **SMCR_v2.1.3_bootloader.bin** (18 KB) - Bootloader
- **SMCR_v2.1.3_partitions.bin** (3 KB) - Tabela de partições
- **checksums.sha256** - Verificação de integridade

## 🚀 Como Instalar

### Gravação Rápida (Apenas Firmware)
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 SMCR_v2.1.3_firmware.bin
```

### Gravação Completa (Com Bootloader)
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash \
  0x1000 SMCR_v2.1.3_bootloader.bin \
  0x8000 SMCR_v2.1.3_partitions.bin \
  0x10000 SMCR_v2.1.3_firmware.bin
```

## ⚠️ Avisos Importantes

### Factory Reset
- ⚠️ **ATENÇÃO**: Factory reset via GET apaga **TUDO**
- Inclui: NVS, arquivos de configuração, páginas HTML
- Após reset: dispositivo reinicia em modo AP para reconfiguração
- Requer reupload de páginas HTML se customizadas

### Compatibilidade Inter-Módulos
- ✅ Módulos v2.1.3+ suportam valores analógicos completos (0-4095)
- ⚠️ Módulos v2.1.2 ou anteriores ainda usam `uint8_t` (requer atualização)
- 💡 Recomendação: Atualizar todos os módulos para v2.1.3

### Home Assistant
- ✅ MQTT Discovery atualizado automaticamente
- ✅ Sensores analógicos remotos: `sensor` com unidade `ADC`
- ✅ Sensores digitais remotos: `binary_sensor`

## 📚 Documentação

Para detalhes técnicos completos, consulte:
- **manual/implementacoes/REMOTE_ANALOG_PIN_FIX.md** - Análise detalhada de todos os bugs
- **PRIMEIRO_ACESSO.md** - Guia de primeira configuração
- **README.md** - Documentação geral do projeto

## 🔍 Cenários de Teste Recomendados

### 1. Transmissão de Valores Analógicos
```bash
# Enviar valor alto (deve receber 2535, não 159)
curl -X POST http://MOD2/api/pin/set -d "pin=101&value=2535"
```

### 2. Histórico de Pinos Remotos
- Configurar pino remoto analógico (tipo 65533)
- Enviar múltiplos valores via `/api/pin/set`
- Verificar histórico no dashboard (deve mostrar últimos 8 valores)

### 3. Ações com Pinos Remotos
- Criar ação: Pino remoto 101 (tipo 65533) ativa pino local 5
- Configurar faixa: min=900, max=2000
- Enviar valor 1500 → pino 5 deve acionar

### 4. Factory Reset Completo
```bash
# Via GET
curl "http://IP:8080/api/reset/factory?confirm=yes"

# Via POST
curl -X POST http://IP:8080/api/reset/factory -d "confirm=yes"
```

## 🎯 Benefícios desta Release

- ✅ Valores analógicos remotos transmitidos corretamente (0-4095)
- ✅ Logs de comunicação com valores precisos
- ✅ Histórico sem valores impossíveis (lixo de memória)
- ✅ Dashboard mostra estados corretos de saídas digitais
- ✅ Ações com pinos remotos analógicos funcionam perfeitamente
- ✅ Factory reset completo e confiável
- ✅ Automação de reset facilitada (endpoint GET)

## 📈 Próximas Versões

Melhorias planejadas:
- 🔄 Calibração de sensores analógicos remotos
- 📊 Filtros/média para sinais ruidosos
- 🌡️ Suporte a sensores I2C remotos (tipo 65535)
- 💾 Compressão de histórico para economia de RAM

---

**Versão**: 2.1.3  
**Data**: 4 de dezembro de 2025  
**Compilador**: PlatformIO + ESP-IDF  
**Plataforma**: ESP32 (Arduino Framework)
