# Implementação do Botão de Reboot via MQTT

**Data**: 08/12/2025  
**Tipo**: Nova Funcionalidade  
**Status**: ✅ Implementado  

## 📋 Resumo

Implementação de um botão de reboot do ESP32 via MQTT com suporte completo ao Home Assistant Discovery. Permite reiniciar o dispositivo remotamente através de comandos MQTT ou pela interface do Home Assistant.

## 🎯 Motivação

- Facilitar reinicialização remota do ESP32 sem necessidade de acesso físico
- Integração nativa com Home Assistant através do componente `button`
- Útil para manutenção, aplicação de configurações e troubleshooting remoto

## 🔧 Implementação Técnica

### 1. Tópico MQTT

**Formato do comando**: `<topicBase>/<uniqueId>/system/reboot`

Exemplo com configurações padrão:
```
smcr/smcr_aabbccddeeff/system/reboot
```

**Payloads aceitos**:
- `PRESS` (padrão Home Assistant)
- `ON`
- `1`

### 2. Processamento no Callback

Adicionado no `fV_mqttCallback()`:

```cpp
// Processar comando de reboot do sistema
String rebootTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/system/reboot";
if (topicStr == rebootTopic) {
    if (message == "PRESS" || message == "ON" || message == "1") {
        // Publicar confirmação
        String statusTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/system/status";
        vO_mqttClient.publish(statusTopic.c_str(), "Reiniciando...", false);
        
        // Aguardar publicação
        delay(100);
        vO_mqttClient.loop();
        
        // Executar reboot
        ESP.restart();
    }
}
```

### 3. Subscrição ao Tópico

Adicionado na conexão MQTT (`fV_mqttConnect()`):

```cpp
// Subscrever ao tópico de reboot do sistema
String rebootTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + clientId + "/system/reboot";
vO_mqttClient.subscribe(rebootTopic.c_str());
```

### 4. Home Assistant Discovery

Publicação de configuração no formato Home Assistant:

```json
{
  "name": "esp32modularx Reboot",
  "unique_id": "smcr_aabbccddeeff_reboot",
  "command_topic": "smcr/smcr_aabbccddeeff/system/reboot",
  "payload_press": "PRESS",
  "device_class": "restart",
  "availability_topic": "smcr/smcr_aabbccddeeff/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": ["smcr_aabbccddeeff"],
    "name": "esp32modularx",
    "model": "SMCR ESP32",
    "manufacturer": "SMCR",
    "sw_version": "2.1.3"
  }
}
```

**Tópico de discovery**: `homeassistant/button/<uniqueId>_reboot/config`

### 5. Integração no Discovery em Lotes

O botão de reboot é publicado **primeiro** no processo de discovery (índice 0), antes dos pinos:

```cpp
void fV_publishMqttDiscoveryStep(void) {
    // Se o index está em 0, publicar primeiro o botão de reboot
    if (vU8_discoveryIndex == 0) {
        // ... publicar discovery do botão reboot
        vU8_discoveryIndex = 1; // Avançar para os pinos
        return;
    }
    
    // Continua com discovery dos pinos (índice 1+)
}
```

## 📁 Arquivos Modificados

### `src/mqtt_manager.cpp`

1. **`fV_mqttCallback()`** (linhas ~72-140)
   - Adicionado processamento do tópico `/system/reboot`
   - Verifica payload e executa `ESP.restart()`
   - Publica status "Reiniciando..." antes do reboot

2. **`fV_mqttConnect()`** (linhas ~220-230)
   - Adicionada subscrição ao tópico de reboot
   - Log de confirmação da subscrição

3. **`fV_publishMqttDiscoveryStep()`** (linhas ~325-380)
   - Publicação do botão reboot no índice 0
   - Ajuste do índice dos pinos (agora começam em 1)
   - Correção da lógica de array (`pinArrayIndex = vU8_discoveryIndex - 1`)

## ✅ Validação e Testes

### Teste Manual via Mosquitto

```bash
# Publicar comando de reboot
mosquitto_pub -h <broker_ip> -t "smcr/smcr_aabbccddeeff/system/reboot" -m "PRESS"

# Verificar status (antes do reboot)
mosquitto_sub -h <broker_ip> -t "smcr/smcr_aabbccddeeff/system/status"
# Deve exibir: Reiniciando...
```

### Integração Home Assistant

1. **Descoberta automática**: O botão aparece automaticamente no HA quando discovery está habilitado
2. **Localização**: Dispositivo > "esp32modularx" > Controles
3. **Ícone**: Ícone de restart (device_class: restart)
4. **Ação**: Pressionar o botão → ESP32 reinicia em ~100ms

### Teste de Discovery em Lotes

```
[MQTT] Discovery agendado (lotes de 5)
[MQTT] Discovery do botao reboot publicado
[MQTT] Discovery publicado para pino 2 (tipo=1, modo=3, component=switch)
[MQTT] Discovery publicado para pino 4 (tipo=192, modo=1, component=sensor)
...
```

## 🎯 Benefícios

### 1. **Manutenção Remota**
- Reiniciar ESP32 sem acesso físico ao dispositivo
- Útil para instalações em locais de difícil acesso

### 2. **Troubleshooting**
- Reset rápido após mudanças de configuração
- Limpar estados inconsistentes sem desenergizar

### 3. **Integração Home Assistant**
- Controle nativo pela interface do HA
- Automações podem incluir reboot (ex: reboot diário às 3h)
- Notificações quando dispositivo fica offline

### 4. **Zero Configuração**
- Botão aparece automaticamente no HA via discovery
- Funciona imediatamente após habilitar MQTT

## 🔍 Comportamento Detalhado

### Fluxo de Execução

```
1. Usuário pressiona botão no HA ou publica comando MQTT
   ↓
2. ESP32 recebe comando no tópico /system/reboot
   ↓
3. Callback valida payload (PRESS, ON ou 1)
   ↓
4. Publica "Reiniciando..." no tópico /system/status
   ↓
5. Aguarda 100ms para garantir publicação
   ↓
6. Executa vO_mqttClient.loop() para enviar
   ↓
7. Chama ESP.restart()
   ↓
8. ESP32 reinicia (boot completo ~2-5 segundos)
   ↓
9. Ao reconectar, publica "online" no status
   ↓
10. HA detecta dispositivo online novamente
```

### Estados no Home Assistant

- **Disponível**: `online` (ESP32 conectado ao broker)
- **Indisponível**: `offline` (ESP32 desconectado)
- **Reiniciando**: Mensagem transitória no tópico `/system/status`

## 📊 Impacto na Memória

```
RAM:   [==        ]  16.1% (usado 52792 bytes de 327680 bytes)
Flash: [========= ]  88.0% (usado 1152873 bytes de 1310720 bytes)
```

**Incremento**: +~2.2KB Flash (código + strings do discovery)  
**Overhead runtime**: Desprezível (apenas callback e discovery)

## 🚀 Uso em Automações

### Exemplo 1: Reboot Diário
```yaml
automation:
  - alias: "Reboot ESP32 Diário"
    trigger:
      - platform: time
        at: "03:00:00"
    action:
      - service: button.press
        target:
          entity_id: button.esp32modularx_reboot
```

### Exemplo 2: Reboot se Offline por 5min
```yaml
automation:
  - alias: "Reboot ESP32 se Offline"
    trigger:
      - platform: state
        entity_id: binary_sensor.esp32modularx_status
        to: "unavailable"
        for:
          minutes: 5
    action:
      # Tentar reboot via MQTT (pode não funcionar se offline)
      - service: mqtt.publish
        data:
          topic: "smcr/smcr_aabbccddeeff/system/reboot"
          payload: "PRESS"
```

### Exemplo 3: Reboot Após Mudança de Config
```yaml
automation:
  - alias: "Reboot Após Config"
    trigger:
      - platform: state
        entity_id: input_boolean.config_changed
        to: "on"
    action:
      - service: button.press
        target:
          entity_id: button.esp32modularx_reboot
      - service: input_boolean.turn_off
        target:
          entity_id: input_boolean.config_changed
```

## 🔐 Considerações de Segurança

1. **Autenticação MQTT**: Sempre use usuário/senha no broker MQTT
2. **ACL no Broker**: Restringir acesso ao tópico `/system/reboot`
3. **Disponibilidade**: Reboot excessivo pode causar indisponibilidade
4. **Logs**: Ações de reboot são logadas no serial debug (flag LOG_MQTT)

## 📝 Notas Técnicas

- **Delay de 100ms**: Garante que mensagem "Reiniciando..." seja publicada antes do restart
- **Índice 0**: Botão reboot sempre publicado primeiro no discovery (antes dos pinos)
- **Retained**: Tópico de config é retained (true) para persistir no broker
- **Device Class**: Usa `restart` para ícone apropriado no HA
- **QoS**: QoS 0 (fire-and-forget) é suficiente para comando de reboot

## 🔄 Compatibilidade

- **Home Assistant**: 2023.x ou superior (suporte a button component)
- **Broker MQTT**: Qualquer broker compatível (Mosquitto, HiveMQ, etc.)
- **ESP32**: Todos os modelos (ESP32, ESP32-S2, ESP32-C3, etc.)

## 📚 Referências

- [Home Assistant Button Component](https://www.home-assistant.io/integrations/button.mqtt/)
- [MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
- [ESP32 Restart Methods](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html)
