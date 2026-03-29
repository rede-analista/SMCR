// Conteúdo de intermod_manager.cpp
// Gerenciamento de comunicação entre módulos ESP32

#include "globals.h"
#include <HTTPClient.h>
#include <ESPmDNS.h>

// Variáveis globais
InterModConfig_t* vA_interModConfigs = nullptr;
uint8_t vU8_activeInterModCount = 0;

// Sincronização pendente (agendada pelo handler web, executada na loop())
bool vB_pendingModuleSyncRequest = false;
String vS_pendingModuleSyncId = "";

// Estado do flash de alerta de módulos offline
static unsigned long vU32_healthCheckFlashUntil = 0; // Pisca durante healthcheck até este timestamp

// Protótipo interno (definido mais abaixo em fV_interModAlertFlashTask)
static void fV_applyPinsFlash(const String& pinsStr, bool state);

// Variáveis globais para log de comunicações
InterModCommLog_t vSt_InterModCommReceived[MAX_INTERMOD_COMM_LOG];
InterModCommLog_t vSt_InterModCommSent[MAX_INTERMOD_COMM_LOG];
uint8_t vU8_InterModCommReceivedIndex = 0;
uint8_t vU8_InterModCommSentIndex = 0;

// Controle de timing para tasks periódicas
unsigned long vU32_lastHealthCheck = 0;
unsigned long vU32_lastDiscovery = 0;
const unsigned long vU32_discoveryInterval = 300000; // Descoberta a cada 5 minutos (300 segundos)
bool vB_firstDiscoveryDone = false; // Flag para executar discovery logo após o boot

// Arquivo JSON no LittleFS para armazenar módulos
const char* INTERMOD_FILE = "/intermod_config.json";

/**
 * @brief Inicializa o sistema de inter-módulos
 */
void fV_initInterModSystem(void) {
    fV_printSerialDebug(LOG_INTERMOD, "Inicializando sistema de inter-módulos");
    
    // Aloca memória inicial para array de módulos (começa com 0, expandirá conforme necessário)
    vU8_activeInterModCount = 0;
    vA_interModConfigs = nullptr;
    
    // Carrega configurações do LittleFS
    fV_loadInterModConfigs();
    
    fV_printSerialDebug(LOG_INTERMOD, "Sistema de inter-módulos inicializado com %d módulos", 
                       vU8_activeInterModCount);
}

/**
 * @brief Carrega configurações de módulos do LittleFS (JSON)
 */
void fV_loadInterModConfigs(void) {
    if (!LittleFS.exists(INTERMOD_FILE)) {
        fV_printSerialDebug(LOG_INTERMOD, "Arquivo de módulos não existe, iniciando vazio");
        return;
    }
    
    File file = LittleFS.open(INTERMOD_FILE, "r");
    if (!file) {
        fV_printSerialDebug(LOG_INTERMOD, "ERRO: Não foi possível abrir arquivo de módulos");
        return;
    }
    
    // Ler conteúdo JSON
    String jsonContent = file.readString();
    file.close();
    
    if (jsonContent.isEmpty()) {
        fV_printSerialDebug(LOG_INTERMOD, "Arquivo de módulos vazio");
        return;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    
    if (error) {
        fV_printSerialDebug(LOG_INTERMOD, "ERRO: Falha ao fazer parse do JSON: %s", error.c_str());
        return;
    }
    
    JsonArray modules = doc["modules"].as<JsonArray>();
    if (modules.isNull()) {
        fV_printSerialDebug(LOG_INTERMOD, "Nenhum módulo encontrado no JSON");
        return;
    }
    
    // Limpa configurações existentes
    if (vA_interModConfigs != nullptr) {
        delete[] vA_interModConfigs;
    }
    
    // Conta módulos
    vU8_activeInterModCount = modules.size();
    if (vU8_activeInterModCount == 0) {
        vA_interModConfigs = nullptr;
        return;
    }
    
    // Aloca memória
    vA_interModConfigs = new InterModConfig_t[vU8_activeInterModCount];
    
    // Carrega cada módulo
    uint8_t index = 0;
    for (JsonObject module : modules) {
        vA_interModConfigs[index].id = module["id"].as<String>();
        vA_interModConfigs[index].hostname = module["hostname"].as<String>();
        vA_interModConfigs[index].ip = module["ip"].as<String>();
        vA_interModConfigs[index].porta = module["porta"] | 8080;
        vA_interModConfigs[index].online = false; // Inicializa como offline
        vA_interModConfigs[index].falhas_consecutivas = 0;
        vA_interModConfigs[index].ultimo_healthcheck = 0;
        vA_interModConfigs[index].auto_descoberto = module["auto_descoberto"] | false;
        vA_interModConfigs[index].pins_offline            = module["pins_offline"]            | "";
        vA_interModConfigs[index].offline_alert_enabled   = module["offline_alert_enabled"]   | false;
        vA_interModConfigs[index].offline_flash_ms        = module["offline_flash_ms"]        | 200;
        vA_interModConfigs[index].pins_healthcheck        = module["pins_healthcheck"]        | "";
        vA_interModConfigs[index].healthcheck_alert_enabled = module["healthcheck_alert_enabled"] | false;
        vA_interModConfigs[index].healthcheck_flash_ms    = module["healthcheck_flash_ms"]    | 500;
        // Controle de flash (inicializa em runtime)
        vA_interModConfigs[index].ultimo_offline_flash = 0;
        vA_interModConfigs[index].offline_flash_state  = false;
        vA_interModConfigs[index].ultimo_hc_flash      = 0;
        vA_interModConfigs[index].hc_flash_state       = false;
        
        fV_printSerialDebug(LOG_INTERMOD, "Módulo carregado: %s (%s:%d)", 
                           vA_interModConfigs[index].hostname.c_str(),
                           vA_interModConfigs[index].ip.c_str(),
                           vA_interModConfigs[index].porta);
        index++;
    }
    
    fV_printSerialDebug(LOG_INTERMOD, "Total de %d módulos carregados", vU8_activeInterModCount);
}

/**
 * @brief Salva configurações de módulos no LittleFS (JSON)
 */
bool fB_saveInterModConfigs(void) {
    fV_printSerialDebug(LOG_INTERMOD, "Salvando %d módulos no LittleFS", vU8_activeInterModCount);
    
    // Cria documento JSON
    JsonDocument doc;
    JsonArray modules = doc["modules"].to<JsonArray>();

    // Adiciona cada módulo
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        JsonObject module = modules.add<JsonObject>();
        module["id"] = vA_interModConfigs[i].id;
        module["hostname"] = vA_interModConfigs[i].hostname;
        module["ip"] = vA_interModConfigs[i].ip;
        module["porta"] = vA_interModConfigs[i].porta;
        module["auto_descoberto"]           = vA_interModConfigs[i].auto_descoberto;
        module["pins_offline"]              = vA_interModConfigs[i].pins_offline;
        module["offline_alert_enabled"]     = vA_interModConfigs[i].offline_alert_enabled;
        module["offline_flash_ms"]          = vA_interModConfigs[i].offline_flash_ms;
        module["pins_healthcheck"]          = vA_interModConfigs[i].pins_healthcheck;
        module["healthcheck_alert_enabled"] = vA_interModConfigs[i].healthcheck_alert_enabled;
        module["healthcheck_flash_ms"]      = vA_interModConfigs[i].healthcheck_flash_ms;
    }
    
    // Abre arquivo para escrita
    File file = LittleFS.open(INTERMOD_FILE, "w");
    if (!file) {
        fV_printSerialDebug(LOG_INTERMOD, "ERRO: Não foi possível abrir arquivo para escrita");
        return false;
    }
    
    // Serializa JSON para arquivo
    if (serializeJson(doc, file) == 0) {
        fV_printSerialDebug(LOG_INTERMOD, "ERRO: Falha ao serializar JSON");
        file.close();
        return false;
    }
    
    file.close();
    fV_printSerialDebug(LOG_INTERMOD, "Módulos salvos com sucesso");
    return true;
}

/**
 * @brief Limpa todas as configurações de módulos
 */
void fV_clearInterModConfigs(void) {
    fV_printSerialDebug(LOG_INTERMOD, "Limpando configurações de inter-módulos");
    
    if (vA_interModConfigs != nullptr) {
        delete[] vA_interModConfigs;
        vA_interModConfigs = nullptr;
    }
    
    vU8_activeInterModCount = 0;
    
    // Remove arquivo
    if (LittleFS.exists(INTERMOD_FILE)) {
        LittleFS.remove(INTERMOD_FILE);
    }
    
    fV_printSerialDebug(LOG_INTERMOD, "Configurações limpas");
}

/**
 * @brief Encontra índice do módulo por ID
 * @return Índice do módulo ou -1 se não encontrado
 */
int fI_findInterModIndex(const String& id) {
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        if (vA_interModConfigs[i].id == id ||
            vA_interModConfigs[i].hostname == id ||
            vA_interModConfigs[i].ip == id) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Adiciona novo módulo
 * @return Índice do novo módulo ou -1 em caso de erro
 */
int fI_addInterModConfig(const InterModConfig_t& config) {
    // Verifica se já existe
    if (fI_findInterModIndex(config.id) >= 0) {
        fV_printSerialDebug(LOG_INTERMOD, "Módulo %s já existe", config.id.c_str());
        return -1;
    }
    
    // Cria novo array com espaço adicional
    InterModConfig_t* newArray = new InterModConfig_t[vU8_activeInterModCount + 1];
    
    // Copia módulos existentes
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        newArray[i] = vA_interModConfigs[i];
    }
    
    // Adiciona novo módulo
    newArray[vU8_activeInterModCount] = config;
    newArray[vU8_activeInterModCount].online = false;
    newArray[vU8_activeInterModCount].falhas_consecutivas = 0;
    newArray[vU8_activeInterModCount].ultimo_healthcheck = 0;
    newArray[vU8_activeInterModCount].ultimo_offline_flash = 0;
    newArray[vU8_activeInterModCount].offline_flash_state  = false;
    newArray[vU8_activeInterModCount].ultimo_hc_flash      = 0;
    newArray[vU8_activeInterModCount].hc_flash_state       = false;
    
    // Substitui array antigo
    if (vA_interModConfigs != nullptr) {
        delete[] vA_interModConfigs;
    }
    vA_interModConfigs = newArray;
    
    uint8_t newIndex = vU8_activeInterModCount;
    vU8_activeInterModCount++;
    
    fV_printSerialDebug(LOG_INTERMOD, "Módulo %s adicionado no índice %d", 
                       config.id.c_str(), newIndex);
    
    return newIndex;
}

/**
 * @brief Remove módulo por ID
 */
bool fB_removeInterModConfig(const String& id) {
    int index = fI_findInterModIndex(id);
    if (index < 0) {
        fV_printSerialDebug(LOG_INTERMOD, "Módulo %s não encontrado", id.c_str());
        return false;
    }
    
    if (vU8_activeInterModCount == 1) {
        // Último módulo, apenas limpa
        delete[] vA_interModConfigs;
        vA_interModConfigs = nullptr;
        vU8_activeInterModCount = 0;
    } else {
        // Cria novo array menor
        InterModConfig_t* newArray = new InterModConfig_t[vU8_activeInterModCount - 1];
        
        // Copia módulos exceto o removido
        uint8_t newIndex = 0;
        for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
            if (i != index) {
                newArray[newIndex++] = vA_interModConfigs[i];
            }
        }
        
        delete[] vA_interModConfigs;
        vA_interModConfigs = newArray;
        vU8_activeInterModCount--;
    }
    
    fV_printSerialDebug(LOG_INTERMOD, "Módulo %s removido", id.c_str());
    return true;
}

/**
 * @brief Atualiza configuração de módulo
 */
bool fB_updateInterModConfig(const String& id, const InterModConfig_t& config) {
    int index = fI_findInterModIndex(id);
    if (index < 0) {
        fV_printSerialDebug(LOG_INTERMOD, "Módulo %s não encontrado para atualização", id.c_str());
        return false;
    }
    
    // Mantém informações de status
    bool oldOnline = vA_interModConfigs[index].online;
    uint8_t oldFalhas = vA_interModConfigs[index].falhas_consecutivas;
    unsigned long oldHealthcheck = vA_interModConfigs[index].ultimo_healthcheck;
    
    // Atualiza configuração
    vA_interModConfigs[index] = config;
    
    // Restaura informações de status
    vA_interModConfigs[index].online = oldOnline;
    vA_interModConfigs[index].falhas_consecutivas = oldFalhas;
    vA_interModConfigs[index].ultimo_healthcheck = oldHealthcheck;
    
    fV_printSerialDebug(LOG_INTERMOD, "Módulo %s atualizado", id.c_str());
    return true;
}

/**
 * @brief Verifica saúde de um módulo específico via HTTP POST
 * @return true se módulo respondeu, false se offline
 */
bool fB_checkModuleHealth(uint8_t moduleIndex) {
    if (moduleIndex >= vU8_activeInterModCount) {
        return false;
    }
    
    InterModConfig_t* module = &vA_interModConfigs[moduleIndex];
    
    // Monta URL de healthcheck
    String url = "http://" + module->ip + ":" + String(module->porta) + "/api/healthcheck";
    
    HTTPClient http;
    http.setTimeout(3000); // Timeout de 3 segundos
    
    if (!http.begin(url)) {
        fV_printSerialDebug(LOG_INTERMOD, "ERRO: Falha ao iniciar HTTP para %s", module->hostname.c_str());
        return false;
    }
    
    // Envia POST vazio
    int httpCode = http.POST("");
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
        // Verifica se o módulo estava offline e agora voltou
        bool wasOffline = !module->online;

        // Módulo respondeu
        module->online = true;
        module->falhas_consecutivas = 0;
        module->ultimo_healthcheck = millis();

        fV_printSerialDebug(LOG_INTERMOD, "Healthcheck OK: %s (%s)",
                           module->hostname.c_str(), module->ip.c_str());

        // Se o módulo estava offline e voltou, sincroniza em ambas as direções
        if (wasOffline) {
            fV_printSerialDebug(LOG_INTERMOD, "[AUTO-SYNC] Módulo %s voltou ONLINE, sincronizando...",
                               module->hostname.c_str());

            // Desliga pinos de alerta offline do módulo que voltou online
            if (module->pins_offline.length() > 0) {
                fV_applyPinsFlash(module->pins_offline, false);
            }
            module->offline_flash_state = false;

            http.end(); // Fecha conexão do healthcheck primeiro
            delay(500); // Pequeno delay para módulo estabilizar

            // 1. Pede ao módulo que envie seus estados para cá
            fB_requestPinSyncFromModule(module->id);

            // 2. Envia nossos estados de pino para o módulo que voltou online
            fV_syncRemotePinsOnBoot();
        } else {
            http.end();
        }

        return true;
    } else {
        // Módulo não respondeu ou erro HTTP
        module->falhas_consecutivas++;
        
        if (module->falhas_consecutivas >= vSt_mainConfig.vU8_interModMaxFailures) {
            if (module->online) {
                module->online = false;
                fV_printSerialDebug(LOG_INTERMOD, "Módulo OFFLINE após %d falhas: %s", 
                                   module->falhas_consecutivas, module->hostname.c_str());
            }
        }
        
        fV_printSerialDebug(LOG_INTERMOD, "Healthcheck FALHOU: %s (código: %d, falhas: %d)", 
                           module->hostname.c_str(), httpCode, module->falhas_consecutivas);
        
        http.end();
        return false;
    }
}

/**
 * @brief Task periódica de healthcheck para todos os módulos
 * Deve ser chamada no loop() principal
 */
void fV_interModHealthCheckTask(void) {
    // Verifica se inter-módulos está habilitado
    if (!vSt_mainConfig.vB_interModEnabled) {
        return;
    }
    
    // Early return se não há módulos cadastrados
    if (vU8_activeInterModCount == 0) {
        return;
    }
    
    // Verifica se é hora de fazer healthcheck
    unsigned long now = millis();
    unsigned long interval = vSt_mainConfig.vU16_interModHealthCheckInterval * 1000UL;
    
    if (now - vU32_lastHealthCheck < interval) {
        return;
    }
    
    vU32_lastHealthCheck = now;

    // Calcula janela de flash: máximo healthcheck_flash_ms entre módulos habilitados × 3
    // Garante pelo menos 1 ciclo completo visível (mínimo 1000ms)
    unsigned long vU32_maxFlashMs = 1000;
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        if (vA_interModConfigs[i].healthcheck_alert_enabled &&
            vA_interModConfigs[i].pins_healthcheck.length() > 0 &&
            vA_interModConfigs[i].healthcheck_flash_ms > vU32_maxFlashMs) {
            vU32_maxFlashMs = vA_interModConfigs[i].healthcheck_flash_ms;
        }
    }
    vU32_healthCheckFlashUntil = now + (vU32_maxFlashMs * 3);

    // Executa healthcheck para cada módulo
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        fB_checkModuleHealth(i);
        delay(300); // Delay entre requests para não sobrecarregar rede
    }
}

/**
 * @brief Task de descoberta automática via mDNS
 * Busca outros módulos SMCR na rede e adiciona automaticamente
 */
void fV_interModDiscoveryTask(void) {
    // Verifica se inter-módulos e auto-discovery estão habilitados
    if (!vSt_mainConfig.vB_interModEnabled || !vSt_mainConfig.vB_interModAutoDiscovery) {
        return;
    }
    
    // Verifica se WiFi está conectado
    if (!vB_wifiIsConnected) {
        return;
    }
    
    unsigned long now = millis();
    
    // Executa discovery: 1) Imediatamente após boot (quando vB_firstDiscoveryDone=false)
    //                     2) A cada 5 minutos depois disso
    bool shouldRun = false;
    
    if (!vB_firstDiscoveryDone && now > 30000) { // 30s após boot para WiFi estabilizar
        shouldRun = true;
        vB_firstDiscoveryDone = true;
        fV_printSerialDebug(LOG_INTERMOD, "Executando primeira descoberta automática após boot");
    } else if (vB_firstDiscoveryDone && (now - vU32_lastDiscovery >= vU32_discoveryInterval)) {
        shouldRun = true;
        fV_printSerialDebug(LOG_INTERMOD, "Executando descoberta periódica (intervalo de 5min)");
    }
    
    if (!shouldRun) {
        return;
    }
    
    vU32_lastDiscovery = now;
    
    fV_printSerialDebug(LOG_INTERMOD, "Iniciando descoberta automática de módulos via mDNS");
    
    // Busca por serviços _http._tcp (todos os módulos SMCR anunciam esse serviço)
    int numServices = MDNS.queryService("http", "tcp");
    
    if (numServices == 0) {
        fV_printSerialDebug(LOG_INTERMOD, "Nenhum serviço mDNS encontrado");
        return;
    }
    
    fV_printSerialDebug(LOG_INTERMOD, "Encontrados %d serviços mDNS", numServices);
    
    // Processa cada serviço encontrado
    for (int i = 0; i < numServices; i++) {
        String hostname = MDNS.hostname(i);
        String ip = MDNS.IP(i).toString();
        uint16_t port = MDNS.port(i);
        
        int numTxtRecords = MDNS.numTxt(i);
        fV_printSerialDebug(LOG_INTERMOD, "Serviço #%d: hostname=%s, ip=%s, port=%d, txtRecords=%d",
                           i, hostname.c_str(), ip.c_str(), port, numTxtRecords);

        // Verifica se é um módulo SMCR (possui TXT record com device_type=smcr OU apenas "smcr")
        bool isSmcr = false;

        // Método 1: Leitura de TXT records (padrão)
        for (int j = 0; j < numTxtRecords; j++) {
            String txtRecord = String(MDNS.txt(i, j));
            fV_printSerialDebug(LOG_INTERMOD, "  TXT[%d]: %s", j, txtRecord.c_str());

            // Aceita formato completo "device_type=smcr" ou apenas o valor "smcr"
            if (txtRecord.indexOf("device_type=smcr") >= 0 || txtRecord.equalsIgnoreCase("smcr")) {
                isSmcr = true;
            }
        }

        // Método 2: Se não encontrou TXT records, tenta buscar por chave específica
        if (!isSmcr && numTxtRecords == 0) {
            fV_printSerialDebug(LOG_INTERMOD, "  -> Nenhum TXT record via numTxt(), tentando método alternativo...");

            // Tenta acessar TXT record diretamente por chave (algumas bibliotecas usam essa abordagem)
            String deviceType = MDNS.txt(i, "device_type");
            fV_printSerialDebug(LOG_INTERMOD, "  -> device_type direto: %s", deviceType.c_str());

            if (deviceType == "smcr" || deviceType == "SMCR") {
                isSmcr = true;
                fV_printSerialDebug(LOG_INTERMOD, "  -> Módulo SMCR identificado via método alternativo!");
            }
        }

        // Método 3: Identificação por padrão de hostname (fallback)
        if (!isSmcr) {
            // Se o hostname contém "esp32modular" ou "smcr", assume que é um módulo SMCR
            String hostnameLower = hostname;
            hostnameLower.toLowerCase();
            if (hostnameLower.indexOf("esp32modular") >= 0 || hostnameLower.indexOf("smcr") >= 0) {
                fV_printSerialDebug(LOG_INTERMOD, "  -> Possível módulo SMCR por hostname (sem TXT records confirmados)");
                // Não marca como isSmcr automaticamente para evitar falsos positivos
            }
        }
        
        // Método 4: Se ainda não identificou, tenta healthcheck HTTP (fallback final)
        if (!isSmcr && port > 0) {
            fV_printSerialDebug(LOG_INTERMOD, "  -> Tentando identificação via healthcheck HTTP...");

            HTTPClient http;
            http.setTimeout(2000); // Timeout curto de 2 segundos

            String healthUrl = "http://" + ip + ":" + String(port) + "/api/healthcheck";

            if (http.begin(healthUrl)) {
                int httpCode = http.POST("");

                if (httpCode == HTTP_CODE_OK) {
                    // Tenta parsear resposta JSON
                    String payload = http.getString();
                    fV_printSerialDebug(LOG_INTERMOD, "  -> Resposta healthcheck: %s", payload.c_str());

                    // Se contém "hostname" e "firmware", provavelmente é SMCR
                    if (payload.indexOf("hostname") >= 0 && payload.indexOf("firmware") >= 0) {
                        isSmcr = true;
                        fV_printSerialDebug(LOG_INTERMOD, "  -> Módulo SMCR identificado via healthcheck HTTP!");
                    }
                }

                http.end();
            }
        }

        if (!isSmcr) {
            fV_printSerialDebug(LOG_INTERMOD, "  -> Não é módulo SMCR, ignorando");
            continue; // Não é um módulo SMCR
        }
        
        fV_printSerialDebug(LOG_INTERMOD, "  -> Módulo SMCR detectado!");
        
        // Gera ID baseado no hostname
        String moduleId = hostname;
        
        // Verifica se é o próprio módulo
        if (hostname == vSt_mainConfig.vS_hostname) {
            fV_printSerialDebug(LOG_INTERMOD, "Ignorando o próprio módulo: %s", hostname.c_str());
            continue;
        }
        
        // Verifica se já está cadastrado
        if (fI_findInterModIndex(moduleId) >= 0) {
            fV_printSerialDebug(LOG_INTERMOD, "Módulo já cadastrado: %s", hostname.c_str());
            continue;
        }
        
        // Adiciona novo módulo descoberto
        InterModConfig_t newModule;
        newModule.id = moduleId;
        newModule.hostname = hostname;
        newModule.ip = ip;
        newModule.porta = port;
        newModule.auto_descoberto = true;
        
        int index = fI_addInterModConfig(newModule);
        if (index >= 0) {
            fV_printSerialDebug(LOG_INTERMOD, "Módulo auto-descoberto adicionado: %s (%s:%d)", 
                               hostname.c_str(), ip.c_str(), port);
            
            // Salva configurações
            fB_saveInterModConfigs();
        }
    }
}

/**
 * @brief Retorna resumo de status dos módulos
 */
String fS_getInterModStatus(void) {
    if (!vSt_mainConfig.vB_interModEnabled) {
        return "Desabilitado";
    }
    
    if (vU8_activeInterModCount == 0) {
        return "Sem módulos cadastrados";
    }
    
    uint8_t online = 0;
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        if (vA_interModConfigs[i].online) {
            online++;
        }
    }
    
    return String(online) + "/" + String(vU8_activeInterModCount) + " online";
}

/**
 * @brief Aplica estado de flash (on/off) a uma lista de GPIOs separados por vírgula
 */
static void fV_applyPinsFlash(const String& pinsStr, bool state) {
    if (pinsStr.length() == 0) return;
    String str = pinsStr;
    int startPos = 0;
    while (startPos <= (int)str.length()) {
        int commaPos = str.indexOf(',', startPos);
        String pinStr = (commaPos == -1) ? str.substring(startPos) : str.substring(startPos, commaPos);
        pinStr.trim();
        if (pinStr.length() > 0) {
            uint16_t gpio = (uint16_t)pinStr.toInt();
            uint8_t idx = fU8_findPinIndex(gpio);
            if (idx != 255) {
                bool nivelInvertido = (vA_pinConfigs[idx].nivel_acionamento_min == 0);
                bool valorFisico = nivelInvertido ? !state : state;
                vA_pinConfigs[idx].status_atual = state ? 1 : 0;
                digitalWrite(gpio, valorFisico ? HIGH : LOW);
            }
        }
        if (commaPos == -1) break;
        startPos = commaPos + 1;
    }
}

/**
 * @brief Task de flash dos pinos de alerta — offline e healthcheck separados por módulo
 * Deve ser chamada na loop() a cada ~50ms para suportar intervalos curtos
 */
void fV_interModAlertFlashTask(void) {
    if (!vSt_mainConfig.vB_interModEnabled || vU8_activeInterModCount == 0) return;

    unsigned long now = millis();
    bool inHealthCheckFlash = (now < vU32_healthCheckFlashUntil);

    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        InterModConfig_t* m = &vA_interModConfigs[i];

        // ── Alerta de Offline ────────────────────────────────────────────
        if (m->offline_alert_enabled && !m->online && m->pins_offline.length() > 0) {
            if (now - m->ultimo_offline_flash >= m->offline_flash_ms) {
                m->ultimo_offline_flash = now;
                m->offline_flash_state = !m->offline_flash_state;
                fV_applyPinsFlash(m->pins_offline, m->offline_flash_state);
            }
        } else if (m->offline_flash_state) {
            // Módulo voltou online ou alerta desabilitado: desliga pinos
            m->offline_flash_state = false;
            fV_applyPinsFlash(m->pins_offline, false);
        }

        // ── Alerta de HealthCheck ────────────────────────────────────────
        if (m->healthcheck_alert_enabled && inHealthCheckFlash && m->pins_healthcheck.length() > 0) {
            if (now - m->ultimo_hc_flash >= m->healthcheck_flash_ms) {
                m->ultimo_hc_flash = now;
                m->hc_flash_state = !m->hc_flash_state;
                fV_applyPinsFlash(m->pins_healthcheck, m->hc_flash_state);
            }
        } else if (m->hc_flash_state) {
            // Healthcheck terminou ou desabilitado: desliga pinos
            m->hc_flash_state = false;
            fV_applyPinsFlash(m->pins_healthcheck, false);
        }
    }
}
