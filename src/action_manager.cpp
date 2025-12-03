// Conteúdo de action_manager.cpp
#include "globals.h"
#include "FS.h"
#include "LittleFS.h"
#include <HTTPClient.h>
#include <WiFi.h>

// Variáveis globais para gerenciamento de ações
ActionConfig_t* vA_actionConfigs = nullptr;
uint8_t vU8_activeActionsCount = 0;

// Constantes para tipos de ação
const uint16_t ACTION_TYPE_NONE = 0;
const uint16_t ACTION_TYPE_LIGA = 1;
const uint16_t ACTION_TYPE_LIGA_DELAY = 2;
const uint16_t ACTION_TYPE_PISCA = 3;
const uint16_t ACTION_TYPE_PULSO = 4;
const uint16_t ACTION_TYPE_PULSO_DELAY_ON = 5;
const uint16_t ACTION_TYPE_STATUS = 65534;
const uint16_t ACTION_TYPE_SINCRONISMO = 65535;

// Máximo de ações: 4 ações por pino de origem
const uint8_t MAX_ACTIONS_PER_PIN = 4;

//========================================
// Inicializa o sistema de ações
//========================================
void fV_initActionSystem(void) {
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Inicializando sistema de gerenciamento de ações...");
    
    // Verifica se LittleFS está montado
    if (!LittleFS.begin()) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Erro ao montar LittleFS");
        return;
    }
    
    // Calcula capacidade máxima: 4 ações para cada pino configurado
    uint8_t maxPins = vSt_mainConfig.vU8_quantidadePinos;
    uint8_t maxActions = maxPins * MAX_ACTIONS_PER_PIN;
    
    if (vA_actionConfigs != nullptr) {
        delete[] vA_actionConfigs;
    }
    
    vA_actionConfigs = new ActionConfig_t[maxActions];
    if (vA_actionConfigs == nullptr) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: Falha ao alocar memória para %d ações!", maxActions);
        return;
    }
    
    // Inicializa array com valores padrão
    for (uint8_t i = 0; i < maxActions; i++) {
        vA_actionConfigs[i].pino_origem = 0;
        vA_actionConfigs[i].numero_acao = 0;
        vA_actionConfigs[i].pino_destino = 0;
        vA_actionConfigs[i].acao = ACTION_TYPE_NONE;
        vA_actionConfigs[i].tempo_on = 0;
        vA_actionConfigs[i].tempo_off = 0;
        vA_actionConfigs[i].pino_remoto = 0;
        vA_actionConfigs[i].envia_modulo = "";
        vA_actionConfigs[i].telegram = false;
        vA_actionConfigs[i].assistente = false;
        vA_actionConfigs[i].contador_on = 0;
        vA_actionConfigs[i].contador_off = 0;
        vA_actionConfigs[i].estado_acao = false;
        vA_actionConfigs[i].ultimo_estado_origem = false;
    }
    
    vU8_activeActionsCount = 0;
    
    // Carrega configurações salvas
    fV_loadActionConfigs();
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Sistema de ações inicializado. Máximo: %d, Ativos: %d", 
                       maxActions, vU8_activeActionsCount);
    
    // *** SINCRONIZAÇÃO INICIAL: Verifica estado atual dos pinos de origem ***
    // Após o boot, força a verificação do estado atual de todos os pinos de origem
    // para garantir que ações sejam aplicadas corretamente mesmo se o pino já estava acionado
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Sincronizando estados iniciais dos pinos de origem...");
    for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
        if (vA_actionConfigs[i].acao == ACTION_TYPE_NONE) {
            continue;
        }
        
        uint8_t pinOrigemIndex = fU8_findPinIndex(vA_actionConfigs[i].pino_origem);
        if (pinOrigemIndex == 255) {
            continue;
        }
        
        // Inicializa ultimo_estado_origem com o estado OPOSTO ao atual
        // Isso garante que na primeira execução de fV_executeActionsTask() seja detectada uma "mudança"
        bool estadoAtual = fB_isPinActivated(pinOrigemIndex);
        vA_actionConfigs[i].ultimo_estado_origem = !estadoAtual; // Inverte para forçar detecção de mudança
        
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Pino origem %d: estado atual=%s, forçando detecção na primeira iteração", 
            vA_actionConfigs[i].pino_origem, estadoAtual ? "ACIONADO" : "NÃO ACIONADO");
    }
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Sincronização inicial concluída.");
}

//========================================
// Carrega configurações de ações do LittleFS
//========================================
void fV_loadActionConfigs(void) {
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Carregando configurações de ações do LittleFS...");
    
    if (!LittleFS.begin()) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: LittleFS não disponível para carregamento!");
        return;
    }
    
    File file = LittleFS.open("/actions_config.json", "r");
    if (!file) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Arquivo de configuração não encontrado, usando padrões.");
        return;
    }
    
    size_t fileSize = file.size();
    if (fileSize == 0) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Arquivo de configuração está vazio.");
        file.close();
        return;
    }
    
    if (fileSize > 16384) { // Limite de segurança 16KB
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: Arquivo muito grande (%d bytes), possível corrupção!", fileSize);
        file.close();
        return;
    }
    
    String jsonContent = file.readString();
    file.close();
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Arquivo lido (%d bytes)", fileSize);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    
    if (error) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Erro ao fazer parse JSON: %s", error.c_str());
        return;
    }
    
    if (!doc["actions"].is<JsonArray>()) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: JSON não contém array 'actions' válido!");
        return;
    }
    
    JsonArray actions = doc["actions"];
    vU8_activeActionsCount = 0;
    
    uint8_t maxActions = vSt_mainConfig.vU8_quantidadePinos * MAX_ACTIONS_PER_PIN;
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Processando %d ações do arquivo...", actions.size());
    
    for (JsonObject action : actions) {
        if (vU8_activeActionsCount >= maxActions) {
            fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Limite de ações atingido (%d), ignorando restantes.", maxActions);
            break;
        }
        
        uint8_t pinOrigem = action["pino_origem"] | 0;
        uint8_t numeroAcao = action["numero_acao"] | 0;
        
        if (pinOrigem == 0 || numeroAcao == 0 || numeroAcao > 4) {
            fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Pulando ação inválida (origem=%d, num=%d)", pinOrigem, numeroAcao);
            continue;
        }
        
        // Verifica se ação já existe
        uint8_t index = fU8_findActionIndex(pinOrigem, numeroAcao);
        if (index == 255) {
            index = vU8_activeActionsCount;
            vU8_activeActionsCount++;
        }
        
        vA_actionConfigs[index].pino_origem = pinOrigem;
        vA_actionConfigs[index].numero_acao = numeroAcao;
        vA_actionConfigs[index].pino_destino = action["pino_destino"] | 0;
        vA_actionConfigs[index].acao = action["acao"] | 0;
        vA_actionConfigs[index].tempo_on = action["tempo_on"] | 0;
        vA_actionConfigs[index].tempo_off = action["tempo_off"] | 0;
        vA_actionConfigs[index].pino_remoto = action["pino_remoto"] | 0;
        vA_actionConfigs[index].envia_modulo = action["envia_modulo"].as<String>();
        vA_actionConfigs[index].telegram = action["telegram"] | false;
        vA_actionConfigs[index].assistente = action["assistente"] | false;
        vA_actionConfigs[index].contador_on = 0;
        vA_actionConfigs[index].contador_off = 0;
        vA_actionConfigs[index].estado_acao = false;
        vA_actionConfigs[index].ultimo_estado_origem = false;
        
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Carregada: Origem=%d, Ação#%d, Destino=%d, Tipo=%d", 
            pinOrigem, numeroAcao, vA_actionConfigs[index].pino_destino, vA_actionConfigs[index].acao);
    }
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Carregadas %d configurações de ações.", vU8_activeActionsCount);
}

//========================================
// Salva configurações de ações no LittleFS
//========================================
bool fB_saveActionConfigs(void) {
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Salvando configurações de ações no LittleFS...");
    
    if (!LittleFS.begin()) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: LittleFS não disponível!");
        return false;
    }
    
    JsonDocument doc;
    JsonArray actions = doc["actions"].to<JsonArray>();
    
    for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
        if (vA_actionConfigs[i].acao != ACTION_TYPE_NONE && vA_actionConfigs[i].pino_origem > 0) {
            JsonObject action = actions.add<JsonObject>();
            action["pino_origem"] = vA_actionConfigs[i].pino_origem;
            action["numero_acao"] = vA_actionConfigs[i].numero_acao;
            action["pino_destino"] = vA_actionConfigs[i].pino_destino;
            action["acao"] = vA_actionConfigs[i].acao;
            action["tempo_on"] = vA_actionConfigs[i].tempo_on;
            action["tempo_off"] = vA_actionConfigs[i].tempo_off;
            action["pino_remoto"] = vA_actionConfigs[i].pino_remoto;
            action["envia_modulo"] = vA_actionConfigs[i].envia_modulo;
            action["telegram"] = vA_actionConfigs[i].telegram;
            action["assistente"] = vA_actionConfigs[i].assistente;
        }
    }
    
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] JSON gerado (%d chars)", jsonOutput.length());
    
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    
    if (freeBytes < jsonOutput.length() + 100) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: Espaço insuficiente no LittleFS!");
        return false;
    }
    
    File file = LittleFS.open("/actions_config.json", "w");
    if (!file) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: Falha ao abrir arquivo para escrita!");
        return false;
    }
    
    size_t bytesWritten = file.print(jsonOutput);
    file.close();
    
    if (bytesWritten != jsonOutput.length()) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: Escrita incompleta!");
        return false;
    }
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Configurações de ações salvas com sucesso! (%d bytes)", bytesWritten);
    return true;
}

//========================================
// Limpa todas as configurações de ações
//========================================
void fV_clearActionConfigs(void) {
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Limpando todas as configurações de ações...");
    
    if (LittleFS.exists("/actions_config.json")) {
        LittleFS.remove("/actions_config.json");
    }
    
    if (vA_actionConfigs != nullptr) {
        uint8_t maxActions = vSt_mainConfig.vU8_quantidadePinos * MAX_ACTIONS_PER_PIN;
        for (uint8_t i = 0; i < maxActions; i++) {
            vA_actionConfigs[i].pino_origem = 0;
            vA_actionConfigs[i].numero_acao = 0;
            vA_actionConfigs[i].acao = ACTION_TYPE_NONE;
        }
    }
    
    vU8_activeActionsCount = 0;
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Configurações de ações limpas!");
}

//========================================
// Encontra índice da ação no array
//========================================
uint8_t fU8_findActionIndex(uint8_t pinOrigem, uint8_t numeroAcao) {
    for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
        if (vA_actionConfigs[i].pino_origem == pinOrigem && 
            vA_actionConfigs[i].numero_acao == numeroAcao) {
            return i;
        }
    }
    return 255; // Não encontrado
}

//========================================
// Adiciona nova configuração de ação
//========================================
int fI_addActionConfig(const ActionConfig_t& config) {
    // Verifica se já existe
    uint8_t existing = fU8_findActionIndex(config.pino_origem, config.numero_acao);
    if (existing != 255) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Ação já existe (origem=%d, num=%d) no índice %d", 
            config.pino_origem, config.numero_acao, existing);
        return existing;
    }
    
    // Verifica limite
    uint8_t maxActions = vSt_mainConfig.vU8_quantidadePinos * MAX_ACTIONS_PER_PIN;
    if (vU8_activeActionsCount >= maxActions) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] ERRO: Limite de ações atingido (%d)", maxActions);
        return -1;
    }
    
    // Adiciona nova ação
    uint8_t index = vU8_activeActionsCount;
    vA_actionConfigs[index] = config;
    vA_actionConfigs[index].contador_on = 0;
    vA_actionConfigs[index].contador_off = 0;
    vA_actionConfigs[index].estado_acao = false;
    vA_actionConfigs[index].ultimo_estado_origem = false;
    vU8_activeActionsCount++;
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Ação adicionada: Origem=%d, Ação#%d, Destino=%d", 
        config.pino_origem, config.numero_acao, config.pino_destino);
    
    return index;
}

//========================================
// Remove configuração de ação
//========================================
bool fB_removeActionConfig(uint8_t pinOrigem, uint8_t numeroAcao) {
    uint8_t index = fU8_findActionIndex(pinOrigem, numeroAcao);
    if (index == 255) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Ação não encontrada para remoção (origem=%d, num=%d)", 
            pinOrigem, numeroAcao);
        return false;
    }
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Removendo ação origem=%d, num=%d do índice %d", 
        pinOrigem, numeroAcao, index);
    
    // Move elementos restantes para frente
    for (uint8_t i = index; i < vU8_activeActionsCount - 1; i++) {
        vA_actionConfigs[i] = vA_actionConfigs[i + 1];
    }
    
    // Limpa último elemento
    vU8_activeActionsCount--;
    vA_actionConfigs[vU8_activeActionsCount].pino_origem = 0;
    vA_actionConfigs[vU8_activeActionsCount].numero_acao = 0;
    vA_actionConfigs[vU8_activeActionsCount].acao = ACTION_TYPE_NONE;
    
    return true;
}

//========================================
// Atualiza configuração de ação
//========================================
bool fB_updateActionConfig(uint8_t pinOrigem, uint8_t numeroAcao, const ActionConfig_t& config) {
    uint8_t index = fU8_findActionIndex(pinOrigem, numeroAcao);
    if (index == 255) {
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Ação não encontrada para atualização");
        return false;
    }
    
    // Preserva contadores e estados
    uint16_t contOn = vA_actionConfigs[index].contador_on;
    uint16_t contOff = vA_actionConfigs[index].contador_off;
    bool estado = vA_actionConfigs[index].estado_acao;
    bool ultimoEstado = vA_actionConfigs[index].ultimo_estado_origem;
    
    vA_actionConfigs[index] = config;
    vA_actionConfigs[index].contador_on = contOn;
    vA_actionConfigs[index].contador_off = contOff;
    vA_actionConfigs[index].estado_acao = estado;
    vA_actionConfigs[index].ultimo_estado_origem = ultimoEstado;
    
    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Ação atualizada: Origem=%d, Ação#%d", pinOrigem, numeroAcao);
    return true;
}

//========================================
// Verifica se um pino está em uso por ações (origem ou destino)
//========================================
bool fB_isPinUsedByActions(uint8_t pinNumber) {
    for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
        // Pula ações desabilitadas
        if (vA_actionConfigs[i].acao == ACTION_TYPE_NONE) {
            continue;
        }
        
        // Verifica se o pino é origem ou destino desta ação
        if (vA_actionConfigs[i].pino_origem == pinNumber || 
            vA_actionConfigs[i].pino_destino == pinNumber) {
            fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Pino %d está em uso - Ação: origem=%d, destino=%d, tipo=%d", 
                pinNumber, 
                vA_actionConfigs[i].pino_origem, 
                vA_actionConfigs[i].pino_destino,
                vA_actionConfigs[i].acao);
            return true;
        }
    }
    return false;
}

//========================================
// Helper: Escreve no pino respeitando nível de acionamento
//========================================
void fV_writeActionPin(uint8_t pinIndex, uint8_t pinGpio, bool ligar) {
    if (pinIndex == 255) return;
    
    // Verifica se pino tem nível de acionamento invertido (nivel_acionamento_min == 0)
    bool nivelInvertido = (vA_pinConfigs[pinIndex].nivel_acionamento_min == 0);
    
    // Aplica lógica: se invertido, inverte o comando
    bool valorFisico = nivelInvertido ? !ligar : ligar;
    
    // Atualizar status e adicionar ao histórico digital (array circular)
    int novoEstado = ligar ? 1 : 0;
    if (vA_pinConfigs[pinIndex].status_atual != novoEstado) {
        vA_pinConfigs[pinIndex].status_atual = novoEstado;
        
        // *** ADICIONAR AO HISTÓRICO DIGITAL ***
        vA_pinConfigs[pinIndex].historico_digital[vA_pinConfigs[pinIndex].historico_index] = novoEstado;
        vA_pinConfigs[pinIndex].historico_index = (vA_pinConfigs[pinIndex].historico_index + 1) % 8;
        if (vA_pinConfigs[pinIndex].historico_count < 8) {
            vA_pinConfigs[pinIndex].historico_count++;
        }
        
        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] GPIO %d mudou para %s [Histórico: %d/8]", 
            pinGpio, novoEstado ? "HIGH" : "LOW", vA_pinConfigs[pinIndex].historico_count);
        
        // Publica no MQTT se habilitado
        if (vSt_mainConfig.vB_mqttEnabled) {
            fV_publishPinStatus(pinIndex);
        }
    }
    
    digitalWrite(pinGpio, valorFisico ? HIGH : LOW);
}

//========================================
// Task para execução de ações
// Chamada periodicamente (ex: a cada 100ms)
//========================================
void fV_executeActionsTask(void) {
    // Early return se não há ações configuradas
    if (vU8_activeActionsCount == 0) {
        return;
    }
    
    for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
        // Pula ações desabilitadas
        if (vA_actionConfigs[i].acao == ACTION_TYPE_NONE) {
            continue;
        }
        
        // Busca índice do pino de origem
        uint8_t pinOrigemIndex = fU8_findPinIndex(vA_actionConfigs[i].pino_origem);
        if (pinOrigemIndex == 255) {
            continue; // Pino de origem não encontrado
        }
        
        // Verifica se pino de origem está acionado
        bool pinoAcionado = fB_isPinActivated(pinOrigemIndex);
        
        // Detecta mudança de estado do pino origem
        if (pinoAcionado != vA_actionConfigs[i].ultimo_estado_origem) {
            vA_actionConfigs[i].ultimo_estado_origem = pinoAcionado;
            
            // Se pino foi acionado, inicia/reseta a ação
            if (pinoAcionado) {
                vA_actionConfigs[i].contador_on = 0;
                vA_actionConfigs[i].contador_off = 0;
                vA_actionConfigs[i].estado_acao = true;
                
                fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Ação iniciada: Origem=%d, Ação#%d", 
                    vA_actionConfigs[i].pino_origem, vA_actionConfigs[i].numero_acao);
                
                // Envia para módulo remoto se configurado (envia estado ON)
                if (vA_actionConfigs[i].envia_modulo != "" && vA_actionConfigs[i].pino_remoto > 0) {
                    fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Enviando ON para módulo '%s', pino remoto %d", 
                        vA_actionConfigs[i].envia_modulo.c_str(), vA_actionConfigs[i].pino_remoto);
                    fB_sendRemoteAction(vA_actionConfigs[i].envia_modulo, vA_actionConfigs[i].pino_remoto, true);
                }
                
                // Envia notificação Telegram se habilitado na ação
                if (vA_actionConfigs[i].telegram) {
                    String pinOrigemNome = vA_pinConfigs[pinOrigemIndex].nome;
                    uint8_t pinDestinoIndex = fU8_findPinIndex(vA_actionConfigs[i].pino_destino);
                    String pinDestinoNome = "";
                    if (pinDestinoIndex != 255) {
                        pinDestinoNome = vA_pinConfigs[pinDestinoIndex].nome;
                    }
                    fV_sendTelegramActionNotification(
                        &vA_actionConfigs[i],
                        pinOrigemNome,
                        pinDestinoNome
                    );
                }
            } else {
                // Pino origem desativou - desliga destino e reseta contadores
                uint8_t pinDestinoIndex = fU8_findPinIndex(vA_actionConfigs[i].pino_destino);
                if (pinDestinoIndex != 255) {
                    // Desliga o pino destino para todas as ações (exceto STATUS/SINCRONISMO)
                    if (vA_actionConfigs[i].acao != ACTION_TYPE_STATUS && 
                        vA_actionConfigs[i].acao != ACTION_TYPE_SINCRONISMO) {
                        fV_writeActionPin(pinDestinoIndex, vA_actionConfigs[i].pino_destino, false);
                        fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Desligando GPIO %d (origem desativada) - Ação tipo %d", 
                            vA_actionConfigs[i].pino_destino, vA_actionConfigs[i].acao);
                        
                        // Se ação deve ser enviada para módulo remoto, envia estado OFF também
                        if (vA_actionConfigs[i].envia_modulo != "" && vA_actionConfigs[i].pino_remoto > 0) {
                            fV_printSerialDebug(LOG_ACTIONS, "[ACTION] Enviando OFF para módulo '%s', pino remoto %d", 
                                vA_actionConfigs[i].envia_modulo.c_str(), vA_actionConfigs[i].pino_remoto);
                            fB_sendRemoteAction(vA_actionConfigs[i].envia_modulo, vA_actionConfigs[i].pino_remoto, false);
                        }
                    }
                }
                
                // Envia notificação Telegram de normalização se habilitado na ação
                if (vA_actionConfigs[i].telegram) {
                    fV_printSerialDebug(LOG_ACTIONS, "[TELEGRAM] Enviando notificação de normalização - Pino %d, Ação #%d", 
                        vA_actionConfigs[i].pino_origem, vA_actionConfigs[i].numero_acao);
                    
                    String pinOrigemNome = vA_pinConfigs[pinOrigemIndex].nome;
                    uint8_t pinDestinoIndex = fU8_findPinIndex(vA_actionConfigs[i].pino_destino);
                    String pinDestinoNome = "";
                    if (pinDestinoIndex != 255) {
                        pinDestinoNome = vA_pinConfigs[pinDestinoIndex].nome;
                    }
                    
                    // Determinar texto do tipo de ação
                    String tipoAcao;
                    switch(vA_actionConfigs[i].acao) {
                        case 1: tipoAcao = "LIGA"; break;
                        case 2: tipoAcao = "LIGA COM DELAY"; break;
                        case 3: tipoAcao = "PISCA"; break;
                        case 4: tipoAcao = "PULSO"; break;
                        case 5: tipoAcao = "PULSO COM DELAY"; break;
                        case 65534: tipoAcao = "STATUS"; break;
                        case 65535: tipoAcao = "SINCRONISMO"; break;
                        default: tipoAcao = "DESCONHECIDO"; break;
                    }
                    
                    String message = "✅ <b>Normalização SMCR</b>\n\n";
                    message += "📌 <b>Módulo:</b> " + vSt_mainConfig.vS_hostname + "\n";
                    
                    // Pino de origem
                    message += "📍 <b>Pino Origem:</b> " + String(vA_actionConfigs[i].pino_origem);
                    if (!pinOrigemNome.isEmpty()) {
                        message += " (" + pinOrigemNome + ")";
                    }
                    message += "\n";
                    
                    // Pino de destino
                    message += "🎯 <b>Pino Destino:</b> " + String(vA_actionConfigs[i].pino_destino);
                    if (!pinDestinoNome.isEmpty()) {
                        message += " (" + pinDestinoNome + ")";
                    }
                    message += "\n";
                    
                    message += "⚡ <b>Ação:</b> #" + String(vA_actionConfigs[i].numero_acao) + " - " + tipoAcao + " normalizada\n";
                    message += "🕐 <b>Horário:</b> " + fS_getFormattedTime();
                    
                    fB_sendTelegramMessage(message);
                }
                
                // Reseta contadores e estado da ação
                vA_actionConfigs[i].contador_on = 0;
                vA_actionConfigs[i].contador_off = 0;
                vA_actionConfigs[i].estado_acao = false;
            }
        }
        
        // Se pino está acionado, executa a ação
        if (pinoAcionado && vA_actionConfigs[i].estado_acao) {
            fV_executeAction(i);
        }
    }
}

//========================================
// Executa uma ação específica
//========================================
void fV_executeAction(uint8_t actionIndex) {
    ActionConfig_t* action = &vA_actionConfigs[actionIndex];
    uint8_t pinDestinoIndex = fU8_findPinIndex(action->pino_destino);
    
    if (pinDestinoIndex == 255) {
        return; // Pino destino não encontrado
    }
    
    switch (action->acao) {
        case ACTION_TYPE_LIGA: // 1 - LIGA
            // Liga o pino destino imediatamente
            fV_writeActionPin(pinDestinoIndex, action->pino_destino, true);
            action->estado_acao = false; // Executa uma vez
            fV_printSerialDebug(LOG_ACTIONS, "[ACTION] LIGA: GPIO %d", action->pino_destino);
            break;
            
        case ACTION_TYPE_LIGA_DELAY: { // 2 - LIGA DELAY
            // Aguarda tempo_on antes de ligar (tempo em ms, task roda a cada 100ms)
            action->contador_on++;
            uint16_t ciclos_delay = action->tempo_on / 100; // Converte ms para ciclos
            if (action->contador_on >= ciclos_delay) {
                fV_writeActionPin(pinDestinoIndex, action->pino_destino, true);
                action->estado_acao = false;
                fV_printSerialDebug(LOG_ACTIONS, "[ACTION] LIGA_DELAY: GPIO %d após %d ms", 
                    action->pino_destino, action->tempo_on);
            }
            break;
        }
            
        case ACTION_TYPE_PISCA: { // 3 - PISCA
            // Alterna entre ON e OFF (tempos em ms, task roda a cada 100ms)
            uint16_t ciclos_on_pisca = action->tempo_on / 100;
            uint16_t ciclos_off_pisca = action->tempo_off / 100;
            
            if (vA_pinConfigs[pinDestinoIndex].status_atual == 0) {
                action->contador_off++;
                if (action->contador_off >= ciclos_off_pisca) {
                    fV_writeActionPin(pinDestinoIndex, action->pino_destino, true);
                    action->contador_off = 0;
                }
            } else {
                action->contador_on++;
                if (action->contador_on >= ciclos_on_pisca) {
                    fV_writeActionPin(pinDestinoIndex, action->pino_destino, false);
                    action->contador_on = 0;
                }
            }
            break;
        }
            
        case ACTION_TYPE_PULSO: { // 4 - PULSO
            // Liga por tempo_on e depois desliga (tempo em ms, task roda a cada 100ms)
            if (action->contador_on == 0) {
                fV_writeActionPin(pinDestinoIndex, action->pino_destino, true);
                fV_printSerialDebug(LOG_ACTIONS, "[ACTION] PULSO iniciado: GPIO %d por %d ms", 
                    action->pino_destino, action->tempo_on);
            }
            action->contador_on++;
            uint16_t ciclos_pulso = action->tempo_on / 100; // Converte ms para ciclos
            if (ciclos_pulso < 1) ciclos_pulso = 1; // Mínimo 1 ciclo
            if (action->contador_on >= ciclos_pulso) {
                fV_writeActionPin(pinDestinoIndex, action->pino_destino, false);
                action->estado_acao = false;
                fV_printSerialDebug(LOG_ACTIONS, "[ACTION] PULSO finalizado: GPIO %d após %d ms", 
                    action->pino_destino, action->tempo_on);
            }
            break;
        }
            
        case ACTION_TYPE_PULSO_DELAY_ON: { // 5 - PULSO DELAY ON
            // Aguarda tempo_on, liga, aguarda tempo_off, desliga (tempos em ms, task roda a cada 100ms)
            uint16_t ciclos_delay_on = action->tempo_on / 100;
            uint16_t ciclos_duracao = action->tempo_off / 100;
            
            if (action->contador_on < ciclos_delay_on) {
                action->contador_on++;
            } else if (action->contador_off == 0) {
                fV_writeActionPin(pinDestinoIndex, action->pino_destino, true);
                action->contador_off = 1;
                fV_printSerialDebug(LOG_ACTIONS, "[ACTION] PULSO_DELAY_ON ligado: GPIO %d após %d ms delay", 
                    action->pino_destino, action->tempo_on);
            } else if (action->contador_off < ciclos_duracao) {
                action->contador_off++;
            } else {
                fV_writeActionPin(pinDestinoIndex, action->pino_destino, false);
                action->estado_acao = false;
                fV_printSerialDebug(LOG_ACTIONS, "[ACTION] PULSO_DELAY_ON finalizado: GPIO %d (delay %d ms + duração %d ms)", 
                    action->pino_destino, action->tempo_on, action->tempo_off);
            }
            break;
        }
            
        case ACTION_TYPE_STATUS: // 65534 - STATUS (pino virtual/remoto)
            // Atualiza status de pino virtual
            vA_pinConfigs[pinDestinoIndex].status_atual = vA_pinConfigs[fU8_findPinIndex(action->pino_origem)].status_atual;
            action->estado_acao = false;
            fV_printSerialDebug(LOG_ACTIONS, "[ACTION] STATUS: GPIO %d <- GPIO %d", 
                action->pino_destino, action->pino_origem);
            break;
            
        default:
            break;
    }
}

// NOTA: O envio para módulo remoto agora é feito apenas na mudança de estado do pino origem,
// não a cada execução da ação, para evitar envios repetidos em ações tipo PISCA

//========================================
// Envia ação para módulo remoto via HTTP
//========================================
bool fB_sendRemoteAction(const String& moduleId, uint8_t remotePin, bool state) {
    // Busca o módulo na lista de módulos cadastrados
    uint8_t moduleIndex = 255;
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        if (vA_interModConfigs[i].id == moduleId) {
            moduleIndex = i;
            break;
        }
    }
    
    if (moduleIndex == 255) {
        fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Módulo '%s' não encontrado na lista", moduleId.c_str());
        return false;
    }
    
    // Verifica se módulo está online
    if (!vA_interModConfigs[moduleIndex].online) {
        fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Módulo '%s' está offline, ignorando envio", moduleId.c_str());
        return false;
    }
    
    // Monta a URL para o endpoint do módulo remoto
    String url = "http://";
    url += vA_interModConfigs[moduleIndex].ip;
    url += ":";
    url += String(vA_interModConfigs[moduleIndex].porta);
    url += "/api/pin/set";
    
    fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Enviando comando para %s", url.c_str());
    
    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000); // 3 segundos de timeout
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Monta o body da requisição
    String postData = "pin=" + String(remotePin) + "&value=" + String(state ? 1 : 0);
    
    int httpCode = http.POST(postData);
    
    bool success = false;
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED) {
            String response = http.getString();
            fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Comando enviado com sucesso: pino=%d, valor=%d", remotePin, state ? 1 : 0);
            success = true;
        } else {
            fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Erro HTTP %d ao enviar comando", httpCode);
        }
    } else {
        fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Falha na conexão: %s", http.errorToString(httpCode).c_str());
    }
    
    http.end();
    return success;
}
