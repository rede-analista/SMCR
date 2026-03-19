/*
 * mqtt_manager.cpp
 * 
 * Gerenciamento de conexão e publicações MQTT
 * Integração com Home Assistant via auto-discovery
 * Publicação de status dos pinos (digitais e analógicos)
 */

#include "globals.h"
#include <PubSubClient.h>
#include <WiFi.h>

// Cliente WiFi e MQTT
WiFiClient vO_wifiClient;
PubSubClient vO_mqttClient(vO_wifiClient);

// Variáveis de controle
bool vB_mqttInitialized = false;
unsigned long vUL_lastMqttReconnectAttempt = 0;
unsigned long vUL_lastMqttPublish = 0;
const uint32_t MQTT_RECONNECT_INTERVAL = 15000; // 15 segundos entre tentativas de reconexão
bool vB_discoveryPublished = false; // Flag para publicar discovery apenas uma vez por conexão
unsigned long vUL_lastDiscoveryComplete = 0; // Timestamp do último discovery completo
// Publicação de discovery em lotes para reduzir burst
uint8_t vU8_discoveryIndex = 0;           // índice do próximo pino para publicar
unsigned long vUL_lastDiscoveryPublish = 0; // controle de ritmo entre publicações
// Os parâmetros de batch e intervalo são lidos da configuração em tempo de execução

// ID único do módulo (baseado no MAC)
String vS_mqttUniqueId = "";

// Forward declaration
void fV_publishMqttDiscoveryStep(void);

/**
 * Gera um ID único baseado no MAC do ESP32 para uso no MQTT
 * Formato: smcr_XXXXXXXXXXXX (onde X são os últimos 6 bytes do MAC em hex)
 */
String fS_getMqttUniqueId(void) {
    if (vS_mqttUniqueId.isEmpty()) {
        uint64_t chipid = ESP.getEfuseMac();
        char uniqueId[32];
        snprintf(uniqueId, sizeof(uniqueId), "smcr_%012llx", chipid);
        vS_mqttUniqueId = String(uniqueId);
    }
    return vS_mqttUniqueId;
}

/**
 * Retorna o status atual da conexão MQTT
 */
String fS_getMqttStatus(void) {
    if (!vSt_mainConfig.vB_mqttEnabled) {
        return "Desabilitado";
    }
    
    if (!vB_wifiIsConnected) {
        return "Aguardando WiFi";
    }
    
    if (vO_mqttClient.connected()) {
        return "Conectado";
    }
    
    return "Desconectado";
}

/**
 * Callback para mensagens recebidas do MQTT
 * Permite controle remoto de pinos via MQTT
 */
void fV_mqttCallback(char* topic, byte* payload, unsigned int length) {
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Mensagem recebida no topico: %s", topic);
    
    // Converter payload para string
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Payload: %s", message.c_str());
    
    String topicStr = String(topic);
    String uniqueId = fS_getMqttUniqueId();
    
    // Processar comando de reboot do sistema
    // Formato: <topicBase>/<uniqueId>/system/reboot
    String rebootTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/system/reboot";
    if (topicStr == rebootTopic) {
        if (message == "PRESS" || message == "ON" || message == "1") {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Comando de reboot recebido via MQTT");
            
            // Publicar confirmação
            String statusTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/system/status";
            vO_mqttClient.publish(statusTopic.c_str(), "Reiniciando...", false);
            
            // Aguardar publicação
            delay(100);
            vO_mqttClient.loop();
            
            // Executar reboot
            ESP.restart();
        }
        return;
    }
    
    // Processar comandos de controle de pinos
    // Formato do tópico: <topicBase>/<uniqueId>/pin/<pinNumber>/set
    String expectedPrefix = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/";
    
    if (topicStr.startsWith(expectedPrefix) && topicStr.endsWith("/set")) {
        // Extrair número do pino
        int startIdx = expectedPrefix.length();
        int endIdx = topicStr.lastIndexOf("/set");
        String pinNumberStr = topicStr.substring(startIdx, endIdx);
        uint16_t pinNumber = pinNumberStr.toInt();
        
        // Encontrar índice do pino
        uint8_t pinIndex = fU8_findPinIndex(pinNumber);
        if (pinIndex < vU8_activePinsCount) {
            PinConfig_t* pin = &vA_pinConfigs[pinIndex];
            
            // Tipo 193 = PWM (saída analógica)
            if (pin->tipo == 193) {
                // Processar valor PWM (0-255)
                int pwmValue = message.toInt();
                if (pwmValue < 0) pwmValue = 0;
                if (pwmValue > 255) pwmValue = 255;
                
                // Escrever PWM (assumindo que o pino já foi configurado como OUTPUT)
                analogWrite(pin->pino, pwmValue);
                pin->status_atual = pwmValue;
                
                fV_printSerialDebug(LOG_PINS, "[MQTT] Pino PWM %d ajustado para %d via MQTT", pinNumber, pwmValue);
                
                // Publicar estado atualizado
                fV_publishPinStatus(pinIndex);
                
            } else if (pin->modo == OUTPUT && pin->tipo == 1) { // Digital output
                // Processar comando (aceita 0/1 ou ON/OFF para compatibilidade)
                int value = message.toInt();
                if (message == "ON") value = 1;
                if (message == "OFF") value = 0;
                
                if (value == 1) {
                    digitalWrite(pin->pino, pin->xor_logic ? LOW : HIGH);
                    pin->status_atual = 1;
                    fV_printSerialDebug(LOG_PINS, "[MQTT] Pino %d acionado via MQTT", pinNumber);
                } else if (value == 0) {
                    digitalWrite(pin->pino, pin->xor_logic ? HIGH : LOW);
                    pin->status_atual = 0;
                    fV_printSerialDebug(LOG_PINS, "[MQTT] Pino %d desligado via MQTT", pinNumber);
                }
                
                // Publicar estado atualizado
                fV_publishPinStatus(pinIndex);
            }
        }
    }
}

/**
 * Inicializa o sistema MQTT (chamado no setup)
 */
void fV_initMqtt(void) {
    if (!vSt_mainConfig.vB_mqttEnabled) {
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Sistema MQTT desabilitado na configuracao");
        return;
    }
    
    fV_printSerialDebug(LOG_INIT, "[MQTT] Inicializando sistema MQTT");
    
    // Configurar servidor MQTT
    vO_mqttClient.setServer(vSt_mainConfig.vS_mqttServer.c_str(), vSt_mainConfig.vU16_mqttPort);
    vO_mqttClient.setCallback(fV_mqttCallback);
    vO_mqttClient.setBufferSize(512); // Aumentar buffer para payloads maiores
    vO_mqttClient.setKeepAlive(60); // 60 segundos
    vO_mqttClient.setSocketTimeout(5); // 5 segundos de timeout
    
    vB_mqttInitialized = true;
    fV_printSerialDebug(LOG_INIT, "[MQTT] Sistema inicializado - Server: %s:%d", 
                       vSt_mainConfig.vS_mqttServer.c_str(), 
                       vSt_mainConfig.vU16_mqttPort);
}

/**
 * Configura conexão MQTT (não bloqueante)
 * Tenta conectar ao broker MQTT
 */
void fV_setupMqtt(void) {
    if (!vSt_mainConfig.vB_mqttEnabled || !vB_mqttInitialized) {
        return;
    }
    
    if (!vB_wifiIsConnected) {
        return;
    }
    
    if (vO_mqttClient.connected()) {
        return;
    }
    
    unsigned long now = millis();
    if (now - vUL_lastMqttReconnectAttempt < MQTT_RECONNECT_INTERVAL) {
        return;
    }
    
    vUL_lastMqttReconnectAttempt = now;
    
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Tentando conectar ao broker...");
    
    String clientId = fS_getMqttUniqueId();
    String willTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + clientId + "/status";
    
    bool connected = false;
    if (!vSt_mainConfig.vS_mqttUser.isEmpty()) {
        // Conectar com autenticação
        connected = vO_mqttClient.connect(
            clientId.c_str(),
            vSt_mainConfig.vS_mqttUser.c_str(),
            vSt_mainConfig.vS_mqttPassword.c_str(),
            willTopic.c_str(),
            0,
            true,
            "offline"
        );
    } else {
        // Conectar sem autenticação
        connected = vO_mqttClient.connect(
            clientId.c_str(),
            willTopic.c_str(),
            0,
            true,
            "offline"
        );
    }
    
    if (connected) {
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Conectado com sucesso!");
        
        // Publicar status online
        vO_mqttClient.publish(willTopic.c_str(), "online", true);
        
        // Subscrever aos tópicos de controle de pinos
        String commandTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + clientId + "/pin/+/set";
        vO_mqttClient.subscribe(commandTopic.c_str());
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Subscrito ao topico: %s", commandTopic.c_str());
        
        // Subscrever ao tópico de reboot do sistema
        String rebootTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + clientId + "/system/reboot";
        vO_mqttClient.subscribe(rebootTopic.c_str());
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Subscrito ao topico de reboot: %s", rebootTopic.c_str());
        
        // Agendar discovery (em lotes) se habilitado
        if (vSt_mainConfig.vB_mqttHomeAssistantDiscovery) {
            fV_publishMqttDiscovery();
        } else {
            vB_discoveryPublished = true;
        }
        
        // Publicar status inicial de todos os pinos
        fV_publishAllPinsStatus();
        
    } else {
        int state = vO_mqttClient.state();
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Falha na conexao. Estado: %d", state);
    }
}

/**
 * Loop MQTT - deve ser chamado no loop principal
 * Mantém conexão ativa e processa mensagens (não bloqueante)
 */
void fV_mqttLoop(void) {
    if (!vSt_mainConfig.vB_mqttEnabled || !vB_mqttInitialized) {
        return;
    }
    
    // Se não estiver conectado, tentar reconectar
    if (!vO_mqttClient.connected()) {
        fV_setupMqtt();
        return;
    }
    
    // Processar mensagens MQTT (não bloqueante)
    vO_mqttClient.loop();

    // Publicar discovery em lotes sem bloquear
    if (!vB_discoveryPublished) {
        fV_publishMqttDiscoveryStep();
    }
    
    // Re-executar discovery periodicamente se habilitado e já concluído
    if (vSt_mainConfig.vB_mqttHomeAssistantDiscovery && vB_discoveryPublished) {
        unsigned long now = millis();
        uint32_t repeatInterval = vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec * 1000; // Converte segundos para ms
        
        if (repeatInterval > 0 && (now - vUL_lastDiscoveryComplete) >= repeatInterval) {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Re-executando discovery periódico após %d segundos", vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec);
            fV_publishMqttDiscovery(); // Reseta vB_discoveryPublished para false
        }
    }
    
    // Publicar status periodicamente
    unsigned long now = millis();
    uint32_t publishInterval = vSt_mainConfig.vU16_mqttPublishInterval * 1000;
    
    if (now - vUL_lastMqttPublish >= publishInterval) {
        vUL_lastMqttPublish = now;
        fV_publishAllPinsStatus();
    }
}

/**
 * Publica configurações de auto-discovery do Home Assistant
 * Formato: homeassistant/<component>/<uniqueId>_<pinNumber>/config
 */
// Agenda a publicação de discovery em lotes (não bloqueante)
void fV_publishMqttDiscovery(void) {
    vB_discoveryPublished = false;
    vU8_discoveryIndex = 0;
    vUL_lastDiscoveryPublish = 0;
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Discovery agendado (lotes de %d)", vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize);
}

// Publica um ou mais itens de discovery respeitando limites de taxa
void fV_publishMqttDiscoveryStep(void) {
    if (!vSt_mainConfig.vB_mqttHomeAssistantDiscovery) return;
    
    // Se o index está em 0, publicar primeiro o botão de reboot do sistema
    if (vU8_discoveryIndex == 0) {
        String uniqueId = fS_getMqttUniqueId();
        String configTopic = "homeassistant/button/" + uniqueId + "_reboot/config";
        String commandTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/system/reboot";
        String statusTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/system/status";
        String availabilityTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/status";
        
        StaticJsonDocument<512> doc;
        doc["name"] = vSt_mainConfig.vS_hostname + " Reboot";
        doc["unique_id"] = uniqueId + "_reboot";
        doc["command_topic"] = commandTopic;
        doc["payload_press"] = "PRESS";
        doc["device_class"] = "restart";
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "online";
        doc["payload_not_available"] = "offline";
        
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"][0] = uniqueId;
        device["name"] = vSt_mainConfig.vS_hostname;
        device["model"] = "SMCR ESP32";
        device["manufacturer"] = "SMCR";
        device["sw_version"] = FIRMWARE_VERSION;
        
        String payload;
        serializeJson(doc, payload);
        
        bool published = vO_mqttClient.publish(configTopic.c_str(), payload.c_str(), true);
        if (published) {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Discovery do botao reboot publicado");
        } else {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Falha ao publicar discovery do botao reboot");
        }
        
        // Publicar status inicial
        vO_mqttClient.publish(statusTopic.c_str(), "Pronto", false);
        
        vU8_discoveryIndex = 1; // Avançar para os pinos
        vUL_lastDiscoveryPublish = millis();
        return;
    }
    if (!vO_mqttClient.connected() || vB_discoveryPublished) return;

    unsigned long now = millis();
    if (now - vUL_lastDiscoveryPublish < vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs) return;
    vUL_lastDiscoveryPublish = now;

    String uniqueId = fS_getMqttUniqueId();
    String deviceName = vSt_mainConfig.vS_hostname;

    uint8_t publishedThisCycle = 0;
    uint8_t batchSize = vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize;
    if (batchSize == 0) batchSize = 1;
    
    // Ajustar índice: vU8_discoveryIndex=1 corresponde ao primeiro pino (index 0 no array)
    uint8_t pinArrayIndex = vU8_discoveryIndex - 1;
    
    while (pinArrayIndex < vU8_activePinsCount && publishedThisCycle < batchSize) {
        PinConfig_t* pin = &vA_pinConfigs[pinArrayIndex];
        pinArrayIndex++;
        vU8_discoveryIndex++;

        // Ignorar apenas pinos sem uso (tipo 0 = não configurado)
        // Pinos remotos (tipo 65534) devem ser publicados
        if (pin->tipo == 0) {
            continue;
        }

        String pinName = pin->nome.isEmpty() ? String("Pino ") + String(pin->pino) : pin->nome;
        String objectId = uniqueId + "_pin" + String(pin->pino);

        // Determina component type baseado no TIPO e MODO do pino
        String component;
        
        // Corrigir erros comuns de digitação no campo classe_mqtt
        String classeMqtt = pin->classe_mqtt;
        classeMqtt.trim();
        classeMqtt.toLowerCase();
        if (classeMqtt == "ligth") classeMqtt = "light"; // Corrige erro de digitação comum
        
        // Determina se é entrada ou saída baseado no MODO
        bool isOutput = (pin->modo == 3 || pin->modo == 12); // OUTPUT ou OUTPUT_OPEN_DRAIN
        bool isInput = (pin->modo == 1 || pin->modo == 5 || pin->modo == 9); // INPUT, INPUT_PULLUP, INPUT_PULLDOWN
        bool isRemoteDigital = (pin->tipo == 65534); // Pino remoto digital (de outro módulo)
        bool isRemoteAnalog = (pin->tipo == 65533); // Pino remoto analógico (de outro módulo)
        bool isAnalogInput = (pin->tipo == 192); // ADC (leitura analógica)
        bool isPWM = (pin->tipo == 193); // PWM (saída analógica)
        
        // Prioridade: TIPO tem precedência sobre MODO para casos especiais
        if (isRemoteDigital) {
            // Para pinos remotos digitais, SEMPRE usar binary_sensor (read-only)
            component = "binary_sensor";
        } else if (isRemoteAnalog) {
            // Para pinos remotos analógicos, usar sensor (read-only, valores 0-4095)
            component = classeMqtt.isEmpty() ? "sensor" : classeMqtt;
        } else if (isAnalogInput) {
            // Entrada analógica (ADC): SEMPRE sensor, ignora modo
            component = classeMqtt.isEmpty() ? "sensor" : classeMqtt;
        } else if (isPWM) {
            // Saída PWM: usa number (controle 0-255) ou light se configurado
            if (classeMqtt == "light" || classeMqtt == "fan") {
                component = classeMqtt; // Respeita light ou fan
            } else {
                component = "number"; // Default para PWM é number
            }
        } else if (isOutput) {
            // Saídas digitais: usa classe configurada ou default "switch"
            component = classeMqtt.isEmpty() ? "switch" : classeMqtt;
        } else if (isInput) {
            // Entradas digitais: binary_sensor
            component = "binary_sensor";
        } else {
            // Fallback para modos desconhecidos
            component = "binary_sensor";
        }
        
        // Define ícone
        String icon;
        if (!pin->icone_mqtt.isEmpty()) {
            icon = pin->icone_mqtt;
        } else {
            // Fallback baseado no tipo
            if (pin->tipo == 1) icon = "mdi:electric-switch";     // Digital
            else if (pin->tipo == 192) icon = "mdi:gauge";         // Analógico ADC
            else if (pin->tipo == 193) icon = "mdi:tune";         // PWM
            else if (pin->tipo == 65533) icon = "mdi:lan-check";   // Remoto Analógico
            else if (pin->tipo == 65534) icon = "mdi:lan-connect"; // Remoto Digital
            else icon = "mdi:numeric";
        }

        String stateTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/" + String(pin->pino) + "/state";
        String configTopic = "homeassistant/" + component + "/" + objectId + "/config";
        
        // Adicionar command topic para pinos de saída (digital ou PWM)
        String commandTopic = "";
        // isPWM, isOutput já foram declarados anteriormente
        
        if (isOutput || isPWM) {
            commandTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/" + String(pin->pino) + "/set";
        }

        // Montar payload completo de discovery conforme especificação Home Assistant
        String payload = "{";
        payload += "\"name\":\"" + pinName + "\",";
        payload += "\"unique_id\":\"" + objectId + "\",";
        payload += "\"state_topic\":\"" + stateTopic + "\",";
        
        // Configurações específicas por tipo de componente
        if (isPWM && component == "number") {
            // PWM como Number: controle 0-255
            payload += "\"command_topic\":\"" + commandTopic + "\",";
            payload += "\"min\":0,";
            payload += "\"max\":255,";
            payload += "\"step\":1,";
            payload += "\"mode\":\"slider\",";
        } else if (isPWM && component == "light") {
            // PWM como Light: controle de brilho
            payload += "\"command_topic\":\"" + commandTopic + "\",";
            payload += "\"brightness_state_topic\":\"" + stateTopic + "\",";
            payload += "\"brightness_command_topic\":\"" + commandTopic + "\",";
            payload += "\"brightness_scale\":255,";
            payload += "\"on_command_type\":\"brightness\",";
        } else if (commandTopic.length() > 0) {
            // Saídas digitais (switch, light digital)
            payload += "\"command_topic\":\"" + commandTopic + "\",";
            payload += "\"payload_on\":\"1\",";
            payload += "\"payload_off\":\"0\",";
            payload += "\"state_on\":\"1\",";
            payload += "\"state_off\":\"0\",";
        } else {
            // Sensores (binary_sensor, sensor)
            if (component == "binary_sensor") {
                payload += "\"payload_on\":\"1\",";
                payload += "\"payload_off\":\"0\",";
            }
        }
        
        payload += "\"icon\":\"" + icon + "\",";
        
        // Para sensores analógicos (ADC local ou remoto), adicionar unidade e atributos
        if (pin->tipo == 192 || pin->tipo == 65533) {
            payload += "\"unit_of_measurement\":\"ADC\",";
            if (classeMqtt.isEmpty()) {
                payload += "\"device_class\":\"voltage\",";
            }
            // Adicionar informações de nível de acionamento como atributos JSON
            String attrTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/" + String(pin->pino) + "/attributes";
            payload += "\"json_attributes_topic\":\"" + attrTopic + "\",";
            
            // Publicar atributos (níveis de acionamento)
            String attrPayload = "{";
            attrPayload += "\"nivel_acionamento_min\":" + String(pin->nivel_acionamento_min) + ",";
            attrPayload += "\"nivel_acionamento_max\":" + String(pin->nivel_acionamento_max) + ",";
            attrPayload += "\"acionado\":" + String((pin->status_atual >= pin->nivel_acionamento_min && pin->status_atual <= pin->nivel_acionamento_max) ? "true" : "false");
            attrPayload += "}";
            vO_mqttClient.publish(attrTopic.c_str(), attrPayload.c_str(), true);
        }
        
        // Informações do dispositivo
        payload += "\"device\":{";
        payload += "\"identifiers\":[\"" + uniqueId + "\"],";
        payload += "\"name\":\"" + deviceName + "\",";
        payload += "\"model\":\"SMCR ESP32\",";
        payload += "\"manufacturer\":\"SMCR\",";
        payload += "\"sw_version\":\"" + String(FIRMWARE_VERSION) + "\"";
        payload += "}";
        
        payload += "}";

        bool success = vO_mqttClient.publish(configTopic.c_str(), payload.c_str(), true);
        if (success) {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Discovery publicado para pino %d (tipo=%d, modo=%d, component=%s)", 
                pin->pino, pin->tipo, pin->modo, component.c_str());
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Topico: %s", configTopic.c_str());
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Payload: %s", payload.c_str());
        } else {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] ERRO ao publicar discovery para pino %d", pin->pino);
        }

        publishedThisCycle++;
    }

    // Discovery completo quando processar todos os pinos (índice = activePinsCount + 1, pois começou em 1)
    if (pinArrayIndex >= vU8_activePinsCount) {
        vB_discoveryPublished = true;
        vUL_lastDiscoveryComplete = millis(); // Registra timestamp da conclusão
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Auto-discovery concluido - %d entidades publicadas", publishedThisCycle);
    }
}

/**
 * Publica status de um pino específico
 */
void fV_publishPinStatus(uint8_t pinIndex) {
    if (!vO_mqttClient.connected() || pinIndex >= vU8_activePinsCount) {
        return;
    }
    
    PinConfig_t* pin = &vA_pinConfigs[pinIndex];
    
    // Ignorar apenas pinos sem uso (tipo 0 = não configurado)
    // Pinos remotos (tipo 65534) devem publicar status
    if (pin->tipo == 0) {
        return;
    }
    
    String uniqueId = fS_getMqttUniqueId();
    String stateTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/" + String(pin->pino) + "/state";
    
    String payload;
    
    // ANALÓGICO (ADC local ou remoto): Envia valor bruto (0-4095)
    if (pin->tipo == 192 || pin->tipo == 65533) {
        payload = String(pin->status_atual);
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Status publicado - Pino %d: %s (analógico)", 
                           pin->pino, payload.c_str());
    } else {
        // DIGITAL: Considera nível de acionamento para entradas
        bool isInput = (pin->modo == 1 || pin->modo == 5 || pin->modo == 9); // INPUT, INPUT_PULLUP, INPUT_PULLDOWN
        bool isOutput = (pin->modo == 3 || pin->modo == 12); // OUTPUT, OUTPUT_OPEN_DRAIN
        bool isRemote = (pin->tipo == 65534); // Pino remoto
        
        int estadoLogico = pin->status_atual;
        
        if (isRemote) {
            // Para pinos remotos, usar status atual direto (já vem processado do módulo de origem)
            estadoLogico = pin->status_atual;
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Status publicado - Pino %d: %d (remoto)", 
                               pin->pino, estadoLogico);
        } else if (isInput) {
            // Para entradas, verificar se o estado atual corresponde ao nível de acionamento
            // Se nivel_acionamento_min == 0: acionado quando status_atual == 0 (inverte para enviar 1)
            // Se nivel_acionamento_min == 1: acionado quando status_atual == 1 (mantém)
            if (pin->status_atual == pin->nivel_acionamento_min) {
                estadoLogico = 1; // Acionado
            } else {
                estadoLogico = 0; // Não acionado
            }
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Status publicado - Pino %d: %d (físico=%d, nível_acion=%d)", 
                               pin->pino, estadoLogico, pin->status_atual, pin->nivel_acionamento_min);
        } else if (isOutput) {
            // Para saídas, envia estado físico direto (comando MQTT já considera lógica invertida)
            estadoLogico = pin->status_atual;
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Status publicado - Pino %d: %d (saída)", 
                               pin->pino, estadoLogico);
        }
        
        payload = String(estadoLogico);
    }
    
    vO_mqttClient.publish(stateTopic.c_str(), payload.c_str());
}

/**
 * Publica status de todos os pinos
 */
void fV_publishAllPinsStatus(void) {
    if (!vO_mqttClient.connected()) {
        return;
    }
    
    // Early return se não há pinos configurados
    if (vU8_activePinsCount == 0) {
        return;
    }
    
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Publicando status de todos os pinos");
    
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        fV_publishPinStatus(i);
        delay(10); // Pequeno delay entre publicações
    }
}
