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
    
    // Processar comandos de controle de pinos
    // Formato do tópico: <topicBase>/<uniqueId>/pin/<pinNumber>/set
    String topicStr = String(topic);
    String expectedPrefix = vSt_mainConfig.vS_mqttTopicBase + "/" + fS_getMqttUniqueId() + "/pin/";
    
    if (topicStr.startsWith(expectedPrefix) && topicStr.endsWith("/set")) {
        // Extrair número do pino
        int startIdx = expectedPrefix.length();
        int endIdx = topicStr.lastIndexOf("/set");
        String pinNumberStr = topicStr.substring(startIdx, endIdx);
        uint8_t pinNumber = pinNumberStr.toInt();
        
        // Encontrar índice do pino
        uint8_t pinIndex = fU8_findPinIndex(pinNumber);
        if (pinIndex < vU8_activePinsCount) {
            PinConfig_t* pin = &vA_pinConfigs[pinIndex];
            
            // Verificar se é pino de saída
            if (pin->modo == OUTPUT && pin->tipo == 1) { // Digital output
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
    if (!vO_mqttClient.connected() || vB_discoveryPublished) return;

    unsigned long now = millis();
    if (now - vUL_lastDiscoveryPublish < vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs) return;
    vUL_lastDiscoveryPublish = now;

    String uniqueId = fS_getMqttUniqueId();
    String deviceName = vSt_mainConfig.vS_hostname;

    uint8_t publishedThisCycle = 0;
    uint8_t batchSize = vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize;
    if (batchSize == 0) batchSize = 1;
    while (vU8_discoveryIndex < vU8_activePinsCount && publishedThisCycle < batchSize) {
        PinConfig_t* pin = &vA_pinConfigs[vU8_discoveryIndex++];

        // Ignorar pinos sem uso ou remotos
        if (pin->tipo == 0 || pin->tipo == 65534) {
            continue;
        }

        String pinName = pin->nome.isEmpty() ? String("Pino ") + String(pin->pino) : pin->nome;
        String objectId = uniqueId + "_pin" + String(pin->pino);

        String component = "sensor";
        String icon = "mdi:numeric";
        if (pin->tipo == 1) icon = "mdi:electric-switch";     // Digital
        else if (pin->tipo == 192) icon = "mdi:gauge";         // Analógico

        for (uint8_t j = 0; j < vU8_activeActionsCount; j++) {
            ActionConfig_t* action = &vA_actionConfigs[j];
            if (action->pino_origem == pin->pino && action->mqtt) {
                if (!action->icone_mqtt.isEmpty()) {
                    icon = action->icone_mqtt;
                }
                break;
            }
        }

        String stateTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/" + String(pin->pino) + "/state";
        String configTopic = "homeassistant/" + component + "/" + objectId + "/config";

        String payload = "{";
        payload += "\"name\":\"" + pinName + "\",";
        payload += "\"uniq_id\":\"" + objectId + "\",";
        payload += "\"stat_t\":\"" + stateTopic + "\",";
        payload += "\"icon\":\"" + icon + "\",";
        payload += "\"dev\":{\"ids\":[\"" + uniqueId + "\"],\"name\":\"" + deviceName + "\"}";
        payload += "}";

        bool success = vO_mqttClient.publish(configTopic.c_str(), payload.c_str(), true);
        if (success) {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] Discovery publicado para pino %d", pin->pino);
        } else {
            fV_printSerialDebug(LOG_MQTT, "[MQTT] ERRO ao publicar discovery para pino %d", pin->pino);
        }

        publishedThisCycle++;
    }

    if (vU8_discoveryIndex >= vU8_activePinsCount) {
        vB_discoveryPublished = true;
        fV_printSerialDebug(LOG_MQTT, "[MQTT] Auto-discovery concluido (em lotes)");
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
    
    // Ignorar pinos sem uso ou remotos
    if (pin->tipo == 0 || pin->tipo == 65534) {
        return;
    }
    
    String uniqueId = fS_getMqttUniqueId();
    String stateTopic = vSt_mainConfig.vS_mqttTopicBase + "/" + uniqueId + "/pin/" + String(pin->pino) + "/state";
    
    // Sempre publicar valor numérico (0/1 para digital, 0-4095 para analógico)
    String payload = String(pin->status_atual);
    
    vO_mqttClient.publish(stateTopic.c_str(), payload.c_str());
    
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Status publicado - Pino %d: %s", 
                       pin->pino, payload.c_str());
}

/**
 * Publica status de todos os pinos
 */
void fV_publishAllPinsStatus(void) {
    if (!vO_mqttClient.connected()) {
        return;
    }
    
    fV_printSerialDebug(LOG_MQTT, "[MQTT] Publicando status de todos os pinos");
    
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        fV_publishPinStatus(i);
        delay(10); // Pequeno delay entre publicações
    }
}
