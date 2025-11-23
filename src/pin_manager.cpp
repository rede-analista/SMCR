// Conteúdo de pin_manager.cpp
#include "globals.h"
#include "FS.h"
#include "LittleFS.h"

// Variáveis globais para gerenciamento de pinos
PinConfig_t* vA_pinConfigs = nullptr;
uint8_t vU8_activePinsCount = 0;

// Constantes para tipos de pino
const uint16_t PIN_TYPE_UNUSED = 0;
const uint16_t PIN_TYPE_DIGITAL = 1; 
const uint16_t PIN_TYPE_ANALOG = 192;
const uint16_t PIN_TYPE_REMOTE = 65534;

// Constantes para modos de pino
const uint8_t PIN_MODE_UNUSED = 0;
const uint8_t PIN_MODE_INPUT = 1;
const uint8_t PIN_MODE_OUTPUT = 3;
const uint8_t PIN_MODE_PULLUP = 4;
const uint8_t PIN_MODE_INPUT_PULLUP = 5;
const uint8_t PIN_MODE_PULLDOWN = 8;
const uint8_t PIN_MODE_INPUT_PULLDOWN = 9;
const uint8_t PIN_MODE_OPEN_DRAIN = 10;
const uint8_t PIN_MODE_OUTPUT_OPEN_DRAIN = 12;
const uint8_t PIN_MODE_ANALOG = 192;

//========================================
// Inicializa o sistema de pinos
//========================================
void fV_initPinSystem(void) {
    fV_printSerialDebug(LOG_PINS, "[PIN] Inicializando sistema de gerenciamento de pinos...");
    
    // Verifica se LittleFS está montado
    if (!LittleFS.begin()) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Erro ao montar LittleFS, formatando...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            fV_printSerialDebug(LOG_PINS, "[PIN] ERRO CRITICO: Falha ao inicializar LittleFS!");
            return;
        }
    }
    
    // Aloca memória para array de pinos baseado na quantidade configurada
    uint8_t maxPins = vSt_mainConfig.vU8_quantidadePinos;
    if (vA_pinConfigs != nullptr) {
        delete[] vA_pinConfigs;
    }
    
    vA_pinConfigs = new PinConfig_t[maxPins];
    if (vA_pinConfigs == nullptr) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: Falha ao alocar memória para %d pinos!", maxPins);
        return;
    }
    
    // Inicializa array com valores padrão
    for (uint8_t i = 0; i < maxPins; i++) {
        vA_pinConfigs[i].nome = "";
        vA_pinConfigs[i].pino = 0;
        vA_pinConfigs[i].tipo = PIN_TYPE_UNUSED;
        vA_pinConfigs[i].modo = PIN_MODE_UNUSED;
        vA_pinConfigs[i].xor_logic = 0;
        vA_pinConfigs[i].tempo_retencao = 0;
        vA_pinConfigs[i].nivel_acionamento_min = 0;
        vA_pinConfigs[i].nivel_acionamento_max = 0;
        vA_pinConfigs[i].status_atual = 0;
        vA_pinConfigs[i].ignore_contador = 0;
    }
    
    vU8_activePinsCount = 0;
    
    // Carrega configurações salvas
    fV_loadPinConfigs();
    
    // Aplica configurações físicas aos pinos
    fV_setupConfiguredPins();
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Sistema de pinos inicializado. Máximo: %d, Ativos: %d", 
                       maxPins, vU8_activePinsCount);
}

//========================================
// Carrega configurações de pinos do LittleFS
//========================================
void fV_loadPinConfigs(void) {
    fV_printSerialDebug(LOG_PINS, "[PIN] Carregando configurações de pinos do LittleFS...");
    
    // Verifica se LittleFS está disponível
    if (!LittleFS.begin()) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: LittleFS não disponível para carregamento!");
        return;
    }
    
    File file = LittleFS.open("/pins_config.json", "r");
    if (!file) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Arquivo de configuração não encontrado, usando padrões.");
        return;
    }
    
    // Verifica tamanho do arquivo
    size_t fileSize = file.size();
    if (fileSize == 0) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Arquivo de configuração está vazio.");
        file.close();
        return;
    }
    
    if (fileSize > 8192) { // Limite de segurança
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: Arquivo muito grande (%d bytes), possível corrupção!", fileSize);
        file.close();
        return;
    }
    
    // Lê arquivo JSON
    String jsonContent = file.readString();
    file.close();
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Arquivo lido (%d bytes): %s", fileSize, jsonContent.c_str());
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    
    if (error) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Erro ao fazer parse JSON: %s", error.c_str());
        return;
    }
    
    // Verifica se estrutura JSON é válida
    if (!doc["pins"].is<JsonArray>()) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: JSON não contém array 'pins' válido!");
        return;
    }
    
    // Carrega configurações do JSON
    JsonArray pins = doc["pins"];
    vU8_activePinsCount = 0;
    
    uint8_t maxPins = vSt_mainConfig.vU8_quantidadePinos;
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Processando %d pinos do arquivo...", pins.size());
    
    for (JsonObject pin : pins) {
        if (vU8_activePinsCount >= maxPins) {
            fV_printSerialDebug(LOG_PINS, "[PIN] Limite de pinos atingido (%d), ignorando restantes.", maxPins);
            break;
        }
        
        uint8_t pinNumber = pin["pino"] | 0;
        
        if (pinNumber == 0) {
            fV_printSerialDebug(LOG_PINS, "[PIN] Pulando pino inválido (número 0)");
            continue;
        }
        
        // Verifica se pino já existe
        uint8_t index = fU8_findPinIndex(pinNumber);
        if (index == 255) {
            // Novo pino, adiciona no próximo slot livre
            index = vU8_activePinsCount;
            vU8_activePinsCount++;
        }
        
        vA_pinConfigs[index].nome = pin["nome"] | "";
        vA_pinConfigs[index].pino = pinNumber;
        vA_pinConfigs[index].tipo = pin["tipo"] | PIN_TYPE_UNUSED;
        vA_pinConfigs[index].modo = pin["modo"] | PIN_MODE_UNUSED;
        vA_pinConfigs[index].xor_logic = pin["xor_logic"] | 0;
        vA_pinConfigs[index].tempo_retencao = pin["tempo_retencao"] | 0;
        vA_pinConfigs[index].nivel_acionamento_min = pin["nivel_acionamento_min"] | 0;
        vA_pinConfigs[index].nivel_acionamento_max = pin["nivel_acionamento_max"] | 0;
        vA_pinConfigs[index].status_atual = 0;
        vA_pinConfigs[index].ignore_contador = 0;
        vA_pinConfigs[index].ultimo_acionamento_ms = 0;  // Inicializa timestamp de retenção
        
        // Inicializa históricos
        vA_pinConfigs[index].historico_index = 0;
        vA_pinConfigs[index].historico_count = 0;
        for (uint8_t h = 0; h < 8; h++) {
            vA_pinConfigs[index].historico_analogico[h] = 0;
            vA_pinConfigs[index].historico_digital[h] = 0;
        }
        
        fV_printSerialDebug(LOG_PINS, "[PIN] Carregado: %s (GPIO %d, tipo %d)", vA_pinConfigs[index].nome.c_str(), pinNumber, vA_pinConfigs[index].tipo);
    }
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Carregadas %d configurações de pinos.", vU8_activePinsCount);
}

//========================================
// Salva configurações de pinos no LittleFS
//========================================
bool fB_savePinConfigs(void) {
    fV_printSerialDebug(LOG_PINS, "[PIN] Salvando configurações de pinos no LittleFS...");
    
    // Verifica se LittleFS está disponível
    if (!LittleFS.begin()) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: LittleFS não disponível!");
        return false;
    }
    
    // Cria documento JSON
    JsonDocument doc;
    JsonArray pins = doc["pins"].to<JsonArray>();
    
    // Adiciona configurações ativas ao JSON
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        if (vA_pinConfigs[i].tipo != PIN_TYPE_UNUSED && vA_pinConfigs[i].pino > 0) {
            JsonObject pin = pins.add<JsonObject>();
            pin["nome"] = vA_pinConfigs[i].nome;
            pin["pino"] = vA_pinConfigs[i].pino;
            pin["tipo"] = vA_pinConfigs[i].tipo;
            pin["modo"] = vA_pinConfigs[i].modo;
            pin["xor_logic"] = vA_pinConfigs[i].xor_logic;
            pin["tempo_retencao"] = vA_pinConfigs[i].tempo_retencao;
            pin["nivel_acionamento_min"] = vA_pinConfigs[i].nivel_acionamento_min;
            pin["nivel_acionamento_max"] = vA_pinConfigs[i].nivel_acionamento_max;
        }
    }
    
    // Converte para string JSON
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    
    fV_printSerialDebug(LOG_PINS, "[PIN] JSON gerado (%d chars): %s", jsonOutput.length(), jsonOutput.c_str());
    
    // Verifica espaço disponível
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Espaço LittleFS - Total: %d, Usado: %d, Livre: %d", totalBytes, usedBytes, freeBytes);
    
    if (freeBytes < jsonOutput.length() + 100) { // Margem de segurança
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: Espaço insuficiente no LittleFS!");
        return false;
    }
    
    // Salva no arquivo
    File file = LittleFS.open("/pins_config.json", "w");
    if (!file) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: Falha ao abrir arquivo para escrita!");
        return false;
    }
    
    size_t bytesWritten = file.print(jsonOutput);
    file.close();
    
    if (bytesWritten != jsonOutput.length()) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: Escrita incompleta! Esperado: %d, Escrito: %d", jsonOutput.length(), bytesWritten);
        return false;
    }
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Configurações de pinos salvas com sucesso! (%d bytes)", bytesWritten);
    return true;
}

//========================================
// Limpa todas as configurações de pinos
//========================================
void fV_clearPinConfigs(void) {
    fV_printSerialDebug(LOG_PINS, "[PIN] Limpando todas as configurações de pinos...");
    
    // Remove arquivo de configuração
    if (LittleFS.exists("/pins_config.json")) {
        LittleFS.remove("/pins_config.json");
    }
    
    // Limpa array em memória
    if (vA_pinConfigs != nullptr) {
        uint8_t maxPins = vSt_mainConfig.vU8_quantidadePinos;
        for (uint8_t i = 0; i < maxPins; i++) {
            vA_pinConfigs[i].nome = "";
            vA_pinConfigs[i].pino = 0;
            vA_pinConfigs[i].tipo = PIN_TYPE_UNUSED;
            vA_pinConfigs[i].modo = PIN_MODE_UNUSED;
            vA_pinConfigs[i].xor_logic = 0;
            vA_pinConfigs[i].tempo_retencao = 0;
            vA_pinConfigs[i].nivel_acionamento_min = 0;
            vA_pinConfigs[i].nivel_acionamento_max = 0;
            vA_pinConfigs[i].status_atual = 0;
            vA_pinConfigs[i].ignore_contador = 0;
        }
    }
    
    vU8_activePinsCount = 0;
    fV_printSerialDebug(LOG_PINS, "[PIN] Configurações de pinos limpas!");
}

//========================================
// Aplica pinMode() para pinos configurados
//========================================
void fV_setupConfiguredPins(void) {
    fV_printSerialDebug(LOG_PINS, "[PIN] Aplicando configurações físicas aos pinos...");
    
    uint8_t configurados = 0;
    
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        if (vA_pinConfigs[i].pino > 0 && vA_pinConfigs[i].tipo != PIN_TYPE_UNUSED) {
            uint8_t pinNumber = vA_pinConfigs[i].pino;
            uint8_t modo = vA_pinConfigs[i].modo;
            uint16_t tipo = vA_pinConfigs[i].tipo;
            
            // Aplica pinMode apenas para pinos digitais e analógicos
            if (tipo == PIN_TYPE_DIGITAL || tipo == PIN_TYPE_ANALOG) {
                
                switch (modo) {
                    case PIN_MODE_INPUT:
                        pinMode(pinNumber, INPUT);
                        fV_printSerialDebug(LOG_PINS, "[PIN] GPIO %d configurado como INPUT", pinNumber);
                        break;
                    case PIN_MODE_OUTPUT:
                        pinMode(pinNumber, OUTPUT);
                        fV_printSerialDebug(LOG_PINS, "[PIN] GPIO %d configurado como OUTPUT", pinNumber);
                        break;
                    case PIN_MODE_INPUT_PULLUP:
                        pinMode(pinNumber, INPUT_PULLUP);
                        fV_printSerialDebug(LOG_PINS, "[PIN] GPIO %d configurado como INPUT_PULLUP", pinNumber);
                        break;
                    case PIN_MODE_INPUT_PULLDOWN:
                        pinMode(pinNumber, INPUT_PULLDOWN);
                        fV_printSerialDebug(LOG_PINS, "[PIN] GPIO %d configurado como INPUT_PULLDOWN", pinNumber);
                        break;
                    case PIN_MODE_OUTPUT_OPEN_DRAIN:
                        pinMode(pinNumber, OUTPUT_OPEN_DRAIN);
                        fV_printSerialDebug(LOG_PINS, "[PIN] GPIO %d configurado como OUTPUT_OPEN_DRAIN", pinNumber);
                        break;
                    default:
                        fV_printSerialDebug(LOG_PINS, "[PIN] Modo %d não reconhecido para GPIO %d", modo, pinNumber);
                        break;
                }
                configurados++;
            } else if (tipo == PIN_TYPE_REMOTE) {
                fV_printSerialDebug(LOG_PINS, "[PIN] Pino remoto %d registrado (%s)", pinNumber, vA_pinConfigs[i].nome.c_str());
                configurados++;
            }
        }
    }
    
    fV_printSerialDebug(LOG_PINS, "[PIN] %d pinos configurados fisicamente.", configurados);
}

//========================================
// Encontra índice do pino no array
//========================================
uint8_t fU8_findPinIndex(uint8_t pinNumber) {
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        if (vA_pinConfigs[i].pino == pinNumber) {
            return i;
        }
    }
    return 255; // Não encontrado
}

//========================================
// Adiciona nova configuração de pino
//========================================
int fI_addPinConfig(const PinConfig_t& config) {
    // Verifica se já existe
    uint8_t existing = fU8_findPinIndex(config.pino);
    if (existing != 255) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Pino %d já existe no índice %d", config.pino, existing);
        return existing;
    }
    
    // Verifica limite
    if (vU8_activePinsCount >= vSt_mainConfig.vU8_quantidadePinos) {
        fV_printSerialDebug(LOG_PINS, "[PIN] ERRO: Limite de pinos atingido (%d)", vSt_mainConfig.vU8_quantidadePinos);
        return -1;
    }
    
    // Adiciona novo pino
    uint8_t index = vU8_activePinsCount;
    vA_pinConfigs[index] = config;
    vA_pinConfigs[index].status_atual = 0;
    vA_pinConfigs[index].ignore_contador = 0;
    vA_pinConfigs[index].ultimo_acionamento_ms = 0;  // Inicializa timestamp de retenção
    vU8_activePinsCount++;
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Pino %d adicionado no índice %d (%s)", 
                       config.pino, index, config.nome.c_str());
    
    return index;
}

//========================================
// Remove configuração de pino
//========================================
bool fB_removePinConfig(uint8_t pinNumber) {
    uint8_t index = fU8_findPinIndex(pinNumber);
    if (index == 255) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Pino %d não encontrado para remoção", pinNumber);
        return false;
    }
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Removendo pino %d (%s) do índice %d", 
                       pinNumber, vA_pinConfigs[index].nome.c_str(), index);
    
    // Move elementos restantes para frente (compacta array)
    for (uint8_t i = index; i < vU8_activePinsCount - 1; i++) {
        vA_pinConfigs[i] = vA_pinConfigs[i + 1];
    }
    
    // Limpa último elemento
    vU8_activePinsCount--;
    vA_pinConfigs[vU8_activePinsCount].nome = "";
    vA_pinConfigs[vU8_activePinsCount].pino = 0;
    vA_pinConfigs[vU8_activePinsCount].tipo = PIN_TYPE_UNUSED;
    vA_pinConfigs[vU8_activePinsCount].modo = PIN_MODE_UNUSED;
    vA_pinConfigs[vU8_activePinsCount].xor_logic = 0;
    vA_pinConfigs[vU8_activePinsCount].tempo_retencao = 0;
    vA_pinConfigs[vU8_activePinsCount].nivel_acionamento_min = 0;
    vA_pinConfigs[vU8_activePinsCount].nivel_acionamento_max = 0;
    vA_pinConfigs[vU8_activePinsCount].status_atual = 0;
    vA_pinConfigs[vU8_activePinsCount].ignore_contador = 0;
    vA_pinConfigs[vU8_activePinsCount].ultimo_acionamento_ms = 0;  // Limpa timestamp
    
    return true;
}

//========================================
// Atualiza configuração de pino
//========================================
bool fB_updatePinConfig(uint8_t pinNumber, const PinConfig_t& config) {
    uint8_t index = fU8_findPinIndex(pinNumber);
    if (index == 255) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Pino %d não encontrado para atualização", pinNumber);
        return false;
    }
    
    fV_printSerialDebug(LOG_PINS, "[PIN] Atualizando pino %d no índice %d", pinNumber, index);
    
    // Preserva status atual, ignore_contador e ultimo_acionamento_ms
    uint16_t statusAtual = vA_pinConfigs[index].status_atual;
    uint16_t ignoreContador = vA_pinConfigs[index].ignore_contador;
    unsigned long ultimoAcionamento = vA_pinConfigs[index].ultimo_acionamento_ms;
    
    vA_pinConfigs[index] = config;
    vA_pinConfigs[index].status_atual = statusAtual;
    vA_pinConfigs[index].ignore_contador = ignoreContador;
    vA_pinConfigs[index].ultimo_acionamento_ms = ultimoAcionamento;  // Preserva timestamp
    
    return true;
}

//========================================
// Verifica se há diferenças entre running e startup config
//========================================
bool fB_hasPinConfigChanges(void) {
    // Carrega configurações da flash para comparação
    File file = LittleFS.open("/pins_config.json", "r");
    if (!file) {
        // Se não há arquivo, considera que há mudanças se existem pinos ativos
        return (vU8_activePinsCount > 0);
    }
    
    String jsonContent = file.readString();
    file.close();
    
    JsonDocument savedDoc;
    DeserializationError error = deserializeJson(savedDoc, jsonContent);
    
    if (error) {
        fV_printSerialDebug(LOG_PINS, "[PIN] Erro ao comparar configs: %s", error.c_str());
        return true; // Assume diferenças se não conseguir ler
    }
    
    JsonArray savedPins = savedDoc["pins"];
    
    // Verifica se a quantidade é diferente
    if (savedPins.size() != vU8_activePinsCount) {
        return true;
    }
    
    // Verifica se os pinos são idênticos
    uint8_t matchCount = 0;
    for (JsonObject savedPin : savedPins) {
        uint8_t pinNumber = savedPin["pino"] | 0;
        uint8_t index = fU8_findPinIndex(pinNumber);
        
        if (index != 255) {
            // Compara todos os campos
            if (vA_pinConfigs[index].nome == (savedPin["nome"] | "") &&
                vA_pinConfigs[index].tipo == (savedPin["tipo"] | 0) &&
                vA_pinConfigs[index].modo == (savedPin["modo"] | 0) &&
                vA_pinConfigs[index].xor_logic == (savedPin["xor_logic"] | 0) &&
                vA_pinConfigs[index].tempo_retencao == (savedPin["tempo_retencao"] | 0) &&
                vA_pinConfigs[index].nivel_acionamento_min == (savedPin["nivel_acionamento_min"] | 0) &&
                vA_pinConfigs[index].nivel_acionamento_max == (savedPin["nivel_acionamento_max"] | 0)) {
                matchCount++;
            }
        }
    }
    
    // Se nem todos os pinos batem, há diferenças
    return (matchCount != vU8_activePinsCount);
}

//========================================
// Atualiza o status atual de todos os pinos
//========================================
void fV_updatePinStatus(void) {
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        if (vA_pinConfigs[i].pino > 0 && vA_pinConfigs[i].tipo != PIN_TYPE_UNUSED) {
            uint8_t pinNumber = vA_pinConfigs[i].pino;
            uint16_t tipo = vA_pinConfigs[i].tipo;
            
            // Lê apenas pinos digitais e analógicos (não remotos)
            if (tipo == PIN_TYPE_DIGITAL) {
                // Leitura digital
                int value = digitalRead(pinNumber);
                vA_pinConfigs[i].status_atual = value;
                
            } else if (tipo == PIN_TYPE_ANALOG) {
                // Leitura analógica
                int value = analogRead(pinNumber);
                vA_pinConfigs[i].status_atual = value;
            }
            // Pinos remotos (tipo 65534) mantêm o status atual definido por outros meios
        }
    }
}

//========================================
// Verifica se um pino está acionado baseado no nível configurado
//========================================
bool fB_isPinActivated(uint8_t pinIndex) {
    if (pinIndex >= vU8_activePinsCount) return false;
    
    uint16_t tipo = vA_pinConfigs[pinIndex].tipo;
    uint16_t statusAtual = vA_pinConfigs[pinIndex].status_atual;
    uint16_t nivelMin = vA_pinConfigs[pinIndex].nivel_acionamento_min;
    uint16_t nivelMax = vA_pinConfigs[pinIndex].nivel_acionamento_max;
    
    if (tipo == PIN_TYPE_DIGITAL) {
        // Para pinos digitais, compara com o valor configurado (0 ou 1)
        return (statusAtual == nivelMin);
    } else if (tipo == PIN_TYPE_ANALOG) {
        // Para pinos analógicos, verifica se está dentro do range configurado
        return (statusAtual >= nivelMin && statusAtual <= nivelMax);
    }
    
    // Pinos remotos ou não utilizados retornam false
    return false;
}

//========================================
// Task para leitura periódica dos pinos
// Ignora pinos remotos (tipo 65534) que são atualizados remotamente
// Implementa tempo de retenção e lógica XOR
//========================================
void fV_readPinsTask(void) {
    unsigned long currentTime = millis();
    
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        // Valida se o pino está configurado
        if (vA_pinConfigs[i].pino == 0 || vA_pinConfigs[i].tipo == PIN_TYPE_UNUSED) {
            continue;
        }
        
        uint8_t pinNumber = vA_pinConfigs[i].pino;
        uint16_t tipo = vA_pinConfigs[i].tipo;
        uint8_t modo = vA_pinConfigs[i].modo;
        uint32_t tempoRetencao = vA_pinConfigs[i].tempo_retencao;
        uint8_t xorLogic = vA_pinConfigs[i].xor_logic;
        
        // *** IGNORA PINOS REMOTOS - serão atualizados via comunicação inter-módulos ***
        if (tipo == PIN_TYPE_REMOTE) {
            continue;
        }
        
        // *** VERIFICA TEMPO DE RETENÇÃO ***
        // Se está em período de retenção, mantém o valor atual e ignora leitura
        if (tempoRetencao > 0 && vA_pinConfigs[i].ultimo_acionamento_ms > 0) {
            unsigned long tempoDecorrido = currentTime - vA_pinConfigs[i].ultimo_acionamento_ms;
            if (tempoDecorrido < tempoRetencao) {
                // Ainda em período de retenção, ignora leitura
                fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d em retenção (%lu/%lu ms)", 
                    pinNumber, tempoDecorrido, tempoRetencao);
                continue;
            }
        }
        
        // Leitura de pinos físicos (digitais e analógicos)
        if (tipo == PIN_TYPE_DIGITAL || (tipo >= 1 && tipo <= 10)) {
            // Pinos digitais (tipos 1-10: Output, Input, Pull-Up, Pull-Down, etc)
            if (modo == PIN_MODE_INPUT || modo == PIN_MODE_INPUT_PULLUP || modo == PIN_MODE_INPUT_PULLDOWN) {
                int value = digitalRead(pinNumber);
                
                // *** APLICAR LÓGICA XOR ***
                // Se xor_logic = 1, inverte o valor lido
                if (xorLogic == 1) {
                    value = !value;
                    fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d XOR aplicado: %d -> %d", 
                        pinNumber, !value, value);
                }
                
                // Atualizar apenas se o valor mudou
                if (vA_pinConfigs[i].status_atual != value) {
                    vA_pinConfigs[i].status_atual = value;
                    
                    // *** ADICIONAR AO HISTÓRICO DIGITAL (array circular) ***
                    vA_pinConfigs[i].historico_digital[vA_pinConfigs[i].historico_index] = value ? 1 : 0;
                    vA_pinConfigs[i].historico_index = (vA_pinConfigs[i].historico_index + 1) % 8; // Avança índice circular
                    if (vA_pinConfigs[i].historico_count < 8) {
                        vA_pinConfigs[i].historico_count++; // Incrementa até atingir 8
                    }
                    
                    // *** INICIAR TEMPO DE RETENÇÃO ***
                    // Marca o timestamp da mudança para iniciar período de retenção
                    if (tempoRetencao > 0) {
                        vA_pinConfigs[i].ultimo_acionamento_ms = currentTime;
                        fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d (%s) mudou para %s - Retenção iniciada por %lu ms [Histórico: %d/%d]", 
                            pinNumber, 
                            vA_pinConfigs[i].nome.c_str(),
                            value ? "HIGH" : "LOW",
                            tempoRetencao,
                            vA_pinConfigs[i].historico_count,
                            8);
                    } else {
                        fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d (%s) mudou para %s [Histórico: %d/%d]", 
                            pinNumber, 
                            vA_pinConfigs[i].nome.c_str(),
                            value ? "HIGH" : "LOW",
                            vA_pinConfigs[i].historico_count,
                            8);
                    }
                }
            } else if (modo == PIN_MODE_OUTPUT) {
                // Para outputs, lê o estado atual e adiciona ao histórico digital
                int value = digitalRead(pinNumber);
                
                // Aplicar XOR também em outputs (se configurado)
                if (xorLogic == 1) {
                    value = !value;
                }
                
                // Atualizar apenas se o valor mudou
                if (vA_pinConfigs[i].status_atual != value) {
                    vA_pinConfigs[i].status_atual = value;
                    
                    // *** ADICIONAR AO HISTÓRICO DIGITAL (array circular) ***
                    vA_pinConfigs[i].historico_digital[vA_pinConfigs[i].historico_index] = value ? 1 : 0;
                    vA_pinConfigs[i].historico_index = (vA_pinConfigs[i].historico_index + 1) % 8;
                    if (vA_pinConfigs[i].historico_count < 8) {
                        vA_pinConfigs[i].historico_count++;
                    }
                    
                    fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d (%s) OUTPUT mudou para %s [Histórico: %d/%d]", 
                        pinNumber, 
                        vA_pinConfigs[i].nome.c_str(),
                        value ? "HIGH" : "LOW",
                        vA_pinConfigs[i].historico_count,
                        8);
                } else {
                    vA_pinConfigs[i].status_atual = value;
                }
            }
            
        } else if (tipo == PIN_TYPE_ANALOG) {
            // Pinos analógicos (ADC)
            int value = analogRead(pinNumber);
            
            // Aplicar filtro de histerese para reduzir ruído (tolerância de ±5)
            int diff = abs(value - vA_pinConfigs[i].status_atual);
            if (diff > 5) {
                vA_pinConfigs[i].status_atual = value;
                
                // *** ADICIONAR AO HISTÓRICO (array circular) ***
                vA_pinConfigs[i].historico_analogico[vA_pinConfigs[i].historico_index] = value;
                vA_pinConfigs[i].historico_index = (vA_pinConfigs[i].historico_index + 1) % 8; // Avança índice circular
                if (vA_pinConfigs[i].historico_count < 8) {
                    vA_pinConfigs[i].historico_count++; // Incrementa até atingir 8
                }
                
                // Iniciar tempo de retenção para analógicos também
                if (tempoRetencao > 0) {
                    vA_pinConfigs[i].ultimo_acionamento_ms = currentTime;
                    fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d (%s) ADC: %d - Retenção iniciada por %lu ms", 
                        pinNumber, 
                        vA_pinConfigs[i].nome.c_str(),
                        value,
                        tempoRetencao);
                } else {
                    fV_printSerialDebug(LOG_PINS, "[PIN TASK] GPIO %d (%s) ADC: %d [Histórico: %d/%d]", 
                        pinNumber, 
                        vA_pinConfigs[i].nome.c_str(),
                        value,
                        vA_pinConfigs[i].historico_count,
                        8);
                }
            }
        }
    }
}
