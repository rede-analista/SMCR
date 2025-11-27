// Conteúdo de  globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

/*=======================================
Inclusão de bibliotecas
*/
#include "include.h"


// Objeto Preferences global, para ser acessado em qualquer lugar
extern Preferences preferences;


// Protótipos das funções genéricas para salvar dados (fV_ para função que retorna void)
// Estas podem ser mantidas para uso genérico, mas as configurações principais irão usar as funções da struct.
void fV_salvarString(const char* vC_key, const String& vS_value);
void fV_salvarInt(const char* vC_key, int vI_value);
void fV_salvarBool(const char* vC_key, bool vB_value);

// Protótipos das funções genéricas para ler dados (fS_ para função que retorna String, fI_ para int, fB_ para bool)
String fS_lerString(const char* vC_key, const String& vS_defaultValue);
int fI_lerInt(const char* vC_key, int vI_defaultValue);
bool fB_lerBool(const char* vC_key, bool vB_defaultValue);

// --- Definições para controle Serial com Bitmasking ---
// Define as flags de log usando potências de 2
enum LogFlags {
    LOG_NONE        = 0,         // Desliga todos os logs
    LOG_INIT        = (1 << 0),  // 1 - Log de inicialização
    LOG_NETWORK     = (1 << 1),  // 2 - Log de rede (Wi-Fi)
    LOG_PINS        = (1 << 2),  // 4 - Log de pinos e GPIO
    LOG_FLASH       = (1 << 3),  // 8 - Log de operações de flash (Preferences, LittleFS)
    LOG_WEB         = (1 << 4),  // 16 - Log da interface web (requisições, respostas)
    LOG_SENSOR      = (1 << 5),  // 32 - Log de leituras de sensores
    LOG_ACTIONS     = (1 << 6),  // 64 - Log de execução de ações
    LOG_INTERMOD    = (1 << 7),  // 128 - Log de comunicação entre módulos
    LOG_WATCHDOG    = (1 << 8),  // 256 - Log do Watchdog Timer
    LOG_MQTT        = (1 << 9),  // 512 - Log de MQTT (conexão, publicação, subscrição)
    
    // Log Total: uma soma de todas as flags relevantes
    LOG_FULL        = LOG_INIT | LOG_NETWORK | LOG_PINS | LOG_FLASH | LOG_WEB | 
                      LOG_SENSOR | LOG_ACTIONS | LOG_INTERMOD | LOG_WATCHDOG | LOG_MQTT
};

// --- Estrutura da "running-config" (Configuração em Memória) ---
// Adicione aqui todas as configurações que deseja gerenciar dinamicamente
struct MainConfig_t { // Usando _t como sufixo para indicar um tipo (Type)
    // 1. Configurações de Debug/Log
    bool vB_serialDebugEnabled; // Indica se a Serial deve ser iniciada e logs exibidos
    uint32_t vU32_activeLogFlags; // As flags de categorias de log ativas

    // 2. Configurações de Rede (WIFI STA)
    String vS_hostname;             // Hostname para a rede (mDNS)
    String vS_wifiSsid;             // SSID da rede Wi-Fi principal
    String vS_wifiPass;             // Senha da rede Wi-Fi principal
    uint16_t vU16_wifiConnectAttempts; // Número de tentativas antes do fallback (padrão 15)

    // 3. Configurações de Acesso (AP Fallback)
    String vS_apSsid;               // SSID para o modo Access Point
    String vS_apPass;               // Senha para o modo Access Point
    bool vB_apFallbackEnabled;      // Habilitar AP se a conexão falhar
    
    // 4. Configurações de Checagem de Rede
    uint32_t vU32_wifiCheckInterval; // Intervalo de checagem em ms (padrão 15000ms)

    // 5. Configurações de NTP
    String vS_ntpServer1;           // Servidor NTP primário
    long vI_gmtOffsetSec;           // Offset GMT em segundos
    int vI_daylightOffsetSec;       // Offset de horário de verão em segundos

    // 6. Configurações da Interface Web
    bool vB_statusPinosEnabled;     // Habilita/desabilita status dos pinos na página inicial
    bool vB_interModulosEnabled;    // Habilita/desabilita info de inter-módulos na página inicial
    String vS_corStatusComAlerta;   // Cor do status quando há alerta (padrão HTML)
    String vS_corStatusSemAlerta;   // Cor do status quando não há alerta (padrão HTML)
    uint16_t vU16_tempoRefresh;     // Tempo de refresh da página inicial em segundos

    // 7. Configurações do Watchdog
    bool vB_watchdogEnabled;        // Habilita/desabilita execução do watchdog
    uint16_t vU16_clockEsp32Mhz;    // Velocidade do clock do ESP32 em MHz
    uint32_t vU32_tempoWatchdogUs;  // Tempo para reset do watchdog em microsegundos

    // 8. Configurações dos Pinos
    uint8_t vU8_quantidadePinos;    // Quantidade total de pinos configuráveis (max 254)

    // 9. Configurações do Servidor Web
    uint16_t vU16_webServerPort;    // Porta do servidor web (padrão 8080)
    bool vB_authEnabled;            // Habilita/desabilita autenticação web
    String vS_webUsername;          // Nome de usuário para autenticação web
    String vS_webPassword;          // Senha para autenticação web
    bool vB_dashboardAuthRequired;  // Se o dashboard requer autenticação (quando auth habilitada)

    // 10. Configurações de Histórico no Dashboard
    bool vB_showAnalogHistory;      // Exibe histórico de 8 leituras para pinos analógicos
    bool vB_showDigitalHistory;     // Exibe histórico de 8 estados para pinos digitais

    // 11. Configurações de MQTT
    bool vB_mqttEnabled;            // Habilita/desabilita integração MQTT
    String vS_mqttServer;           // Endereço do broker MQTT (IP ou hostname)
    uint16_t vU16_mqttPort;         // Porta do broker MQTT (padrão 1883)
    String vS_mqttUser;             // Usuário para autenticação MQTT (opcional)
    String vS_mqttPassword;         // Senha para autenticação MQTT (opcional)
    String vS_mqttTopicBase;        // Tópico base para publicações (ex: "smcr")
    uint16_t vU16_mqttPublishInterval; // Intervalo de publicação de status em segundos (padrão 60)
    bool vB_mqttHomeAssistantDiscovery; // Habilita auto-discovery do Home Assistant
    // Controle de auto-discovery (lotes)
    uint8_t vU8_mqttHaDiscoveryBatchSize;   // Tamanho do lote de discovery
    uint16_t vU16_mqttHaDiscoveryIntervalMs; // Intervalo entre lotes (ms)

};

// --- Estrutura para Configuração de Pinos ---
struct PinConfig_t {
    String nome;                 // Nome descritivo do pino
    uint8_t pino;               // Número físico do GPIO (0-254)
    uint16_t tipo;              // Tipo: 0=Sem Uso, 1=Digital, 192=Analógico, 65534=Remoto
    uint8_t modo;               // pinMode(): 0=SEM_USO, 1=INPUT, 3=OUTPUT, etc.
    uint8_t xor_logic;          // Lógica invertida (0=normal, 1=invertido)
    uint32_t tempo_retencao;    // Tempo em millisegundos para ações temporizadas
    uint16_t status_atual;      // Status: 0-1 (digital) ou 0-4095 (analógico)
    uint16_t ignore_contador;   // Contador para ignore temporário
    unsigned long ultimo_acionamento_ms; // Timestamp da última mudança (para tempo de retenção)
    // Nível de acionamento
    uint16_t nivel_acionamento_min;  // Digital: 0 ou 1 | Analógico: valor mínimo do range
    uint16_t nivel_acionamento_max;  // Digital: igual ao min | Analógico: valor máximo do range
    // Histórico (últimos 8 valores/estados)
    uint16_t historico_analogico[8]; // Array circular para armazenar últimas 8 leituras analógicas
    uint8_t historico_digital[8];    // Array circular para armazenar últimos 8 estados digitais (0 ou 1)
    uint8_t historico_index;         // Índice atual no array circular (0-7)
    uint8_t historico_count;         // Quantidade de valores válidos no histórico (0-8)
};

// --- Estrutura para Configuração de Ações ---
struct ActionConfig_t {
    uint8_t pino_origem;         // GPIO que dispara a ação (pino de entrada/sensor)
    uint8_t numero_acao;         // Número da ação (1, 2, 3 ou 4)
    uint8_t pino_destino;        // GPIO que será acionado (pino de saída/controle)
    uint16_t acao;               // Tipo de ação: 0=NENHUMA, 1=LIGA, 2=LIGA_DELAY, 3=PISCA, 4=PULSO, 5=PULSO_DELAY_ON, 65534=STATUS, 65535=SINCRONISMO
    uint16_t tempo_on;           // Tempo ON em ciclos (0-65535)
    uint16_t tempo_off;          // Tempo OFF em ciclos (0-65535)
    uint8_t pino_remoto;         // Máscara do pino no módulo remoto (0-254)
    uint16_t envia_modulo;       // ID do módulo destino (0=nenhum, 1-65533=ID do módulo)
    bool telegram;               // Envia notificação para Telegram
    bool assistente;             // Envia notificação para Assistente
    bool mqtt;                   // Publica no broker MQTT
    String classe_mqtt;          // Classe do device MQTT (ex: "binary_sensor", "switch")
    String icone_mqtt;           // Ícone MDI para MQTT (ex: "mdi:door")
    // Controle interno da ação
    uint16_t contador_on;        // Contador atual de ciclos ON
    uint16_t contador_off;       // Contador atual de ciclos OFF
    bool estado_acao;            // Estado atual da ação (true=executando, false=parada)
    bool ultimo_estado_origem;   // Último estado do pino origem (para detectar mudanças)
};

// Instância global da sua configuração em memória (sua running-config)
extern MainConfig_t vSt_mainConfig; // vSt_ para variável do tipo Struct

// --- Variáveis globais para gerenciamento de pinos ---
extern PinConfig_t* vA_pinConfigs;   // Array dinâmico de configurações de pinos
extern uint8_t vU8_activePinsCount;  // Contador de pinos ativos configurados

// --- Variáveis globais para gerenciamento de ações ---
extern ActionConfig_t* vA_actionConfigs;  // Array dinâmico de configurações de ações
extern uint8_t vU8_activeActionsCount;    // Contador de ações ativas configuradas

// --- Novas funções para carregar e salvar a estrutura MainConfig_t ---
// Estas serão as funções de interface para a sua "startup-config"
void fV_carregarMainConfig(void); // Carrega da Flash para vSt_mainConfig
void fV_salvarMainConfig(void);   // Salva de vSt_mainConfig para a Flash
void fV_clearPreferences(void) ; // Limpa todas as preferências (Reset de Fábrica)
void fV_clearConfigExceptNetwork(void); // Limpa todas as configurações exceto rede

// --- Funções para gerenciamento de pinos ---
void fV_initPinSystem(void);       // Inicializa sistema de pinos
void fV_loadPinConfigs(void);      // Carrega configurações de pinos do LittleFS
bool fB_savePinConfigs(void);      // Salva configurações de pinos no LittleFS (retorna true se sucesso)
void fV_clearPinConfigs(void);     // Limpa todas as configurações de pinos
void fV_setupConfiguredPins(void); // Aplica pinMode() para pinos configurados
uint8_t fU8_findPinIndex(uint8_t pinNumber); // Encontra índice do pino no array
int fI_addPinConfig(const PinConfig_t& config); // Adiciona nova configuração de pino
bool fB_removePinConfig(uint8_t pinNumber);    // Remove configuração de pino
bool fB_updatePinConfig(uint8_t pinNumber, const PinConfig_t& config); // Atualiza configuração


//Gera um ID para o módulo com base nas informações do chip ESP32
String fS_idModulo();
// Função para imprimir na serial com controle de flags
void fV_printSerialDebug(uint32_t vU32_messageFlag, const char *vC_format, ...);


// Sinaliza se o Wi-Fi está atualmente conectado (true/false)
extern bool vB_wifiIsConnected;
// Ponteiro global para o Servidor Web Assíncrono
extern AsyncWebServer* SERVIDOR_WEB_ASYNC;

/* Funções de Wi-Fi e Rede (rede.cpp) */
void fV_setupWifi();
void fV_setupMdns();          // Inicializa o mDNS
void fV_checkWifiConnection(void); 
void fV_connectWifiSta(void); // Auxiliar: Tenta conectar como station
void fV_startWifiAp(void);    // Auxiliar: Inicia o Access Point
String fS_formatUptime(unsigned long vL_ms);

/* Funções do Servidor Web (servidor_web.cpp) */
void fV_setupWebServer(); 
void fV_handleSaveConfig(AsyncWebServerRequest *request); // Handler para salvar a config inicial (com restart)
void fV_handleApplyConfig(AsyncWebServerRequest *request); // Handler para aplicar config (running-config apenas)
void fV_handleSaveToFlash(AsyncWebServerRequest *request); // Handler para salvar running-config na flash (sem restart)
void fV_handleStatusJson(AsyncWebServerRequest *request); // Handler para API JSON de status
void fV_handleConfigPage(AsyncWebServerRequest *request); // Handler para página de configurações
void fV_handleResetPage(AsyncWebServerRequest *request); // Handler para página de reset
void fV_handleMqttPage(AsyncWebServerRequest *request); // Handler para página de MQTT/Serviços
void fV_handleInterModPage(AsyncWebServerRequest *request); // Handler para página de Inter-Módulos
void fV_handleSoftReset(AsyncWebServerRequest *request); // Handler para reinicialização simples
void fV_handleFactoryReset(AsyncWebServerRequest *request); // Handler para reset de fábrica
void fV_handleNetworkReset(AsyncWebServerRequest *request); // Handler para reset de rede
void fV_handleConfigReset(AsyncWebServerRequest *request); // Handler para reset de config (mantém rede)
void fV_handlePinsReset(AsyncWebServerRequest *request);   // Handler para reset apenas de pinos
void fV_handlePinAdd(AsyncWebServerRequest *request);     // Handler para adicionar novo pino
void fV_handleRestart(AsyncWebServerRequest *request);     // Handler para restart simples
void fV_handleNotFound(AsyncWebServerRequest *request); // Handler 404
void fV_handlePinsListApi(AsyncWebServerRequest *request); // Handler para API de listagem de pinos
void fV_handlePinCreateApi(AsyncWebServerRequest *request); // Handler para API de criação de pinos
void fV_handlePinUpdateApi(AsyncWebServerRequest *request); // Handler para API de atualização de pinos
void fV_handlePinDeleteApi(AsyncWebServerRequest *request); // Handler para API de deleção de pinos
void fV_handlePinsSaveApi(AsyncWebServerRequest *request); // Handler para API de salvamento de pinos
void fV_handlePinsReloadApi(AsyncWebServerRequest *request); // Handler para API de reload de pinos
void fV_handleFilesPage(AsyncWebServerRequest *request);    // Handler para página de arquivos (deprecated - usa firmware)
void fV_handleFirmwarePage(AsyncWebServerRequest *request);  // Handler para página de firmware OTA
void fV_handlePreferenciasPage(AsyncWebServerRequest *request); // Handler para página de preferências NVS
void fV_handleLittleFSPage(AsyncWebServerRequest *request);  // Handler para página LittleFS
void fV_handleNVSList(AsyncWebServerRequest *request);      // Handler para API de listagem NVS
void fV_handleFilesList(AsyncWebServerRequest *request);    // Handler para API de listagem de arquivos
void fV_handleFileDownload(AsyncWebServerRequest *request); // Handler para API de download de arquivos
void fV_handleFileDelete(AsyncWebServerRequest *request);   // Handler para API de deleção de arquivos
void fV_handleFormatFlash(AsyncWebServerRequest *request);  // Handler para API de formatação da flash
void fV_handlePinsClearApi(AsyncWebServerRequest *request); // Handler para API de limpeza de pinos
void fV_handleActionsListApi(AsyncWebServerRequest *request); // Handler para API de listagem de ações
void fV_handleActionCreateApi(AsyncWebServerRequest *request); // Handler para API de criação de ações
void fV_handleActionUpdateApi(AsyncWebServerRequest *request); // Handler para API de atualização de ações
void fV_handleActionDeleteApi(AsyncWebServerRequest *request); // Handler para API de deleção de ações
void fV_handleActionsSaveApi(AsyncWebServerRequest *request); // Handler para API de salvamento de ações
void fV_handleActionsPage(AsyncWebServerRequest *request); // Handler para página de ações

/* Funções de NTP (ntp_func.cpp) */
void fV_setupNtp();
String fS_getFormattedTime();
String fS_getNtpStatus();

/* Funções do Gerenciador de Pinos (pin_manager.cpp) */
void fV_initPinSystem(void);
void fV_loadPinConfigs(void);
bool fB_savePinConfigs(void);
void fV_clearPinConfigs(void);
void fV_setupConfiguredPins(void);
uint8_t fU8_findPinIndex(uint8_t pinNumber);
int fI_addPinConfig(const PinConfig_t& config);
bool fB_removePinConfig(uint8_t pinNumber);
bool fB_updatePinConfig(uint8_t pinNumber, const PinConfig_t& config);
bool fB_hasPinConfigChanges(void);
void fV_updatePinStatus(void);
void fV_readPinsTask(void); // Task periódica para leitura de pinos (ignora remotos)
bool fB_isPinActivated(uint8_t pinIndex);  // Verifica se pino está acionado baseado no nível configurado

/* Funções do Gerenciador de Ações (action_manager.cpp) */
void fV_initActionSystem(void);
void fV_loadActionConfigs(void);
bool fB_saveActionConfigs(void);
void fV_clearActionConfigs(void);
uint8_t fU8_findActionIndex(uint8_t pinOrigem, uint8_t numeroAcao);
int fI_addActionConfig(const ActionConfig_t& config);
bool fB_removeActionConfig(uint8_t pinOrigem, uint8_t numeroAcao);
bool fB_updateActionConfig(uint8_t pinOrigem, uint8_t numeroAcao, const ActionConfig_t& config);
bool fB_isPinUsedByActions(uint8_t pinNumber); // Verifica se pino está em uso por ações
void fV_executeActionsTask(void); // Task periódica para execução de ações
void fV_executeAction(uint8_t actionIndex); // Executa uma ação específica

/* Funções do Gerenciador de MQTT (mqtt_manager.cpp) */
void fV_initMqtt(void);               // Inicializa sistema MQTT
void fV_setupMqtt(void);              // Configura conexão MQTT (não bloqueante)
void fV_mqttLoop(void);               // Loop MQTT (reconexão automática não bloqueante)
void fV_publishMqttDiscovery(void);   // Publica configurações de auto-discovery do Home Assistant
void fV_publishPinStatus(uint8_t pinIndex); // Publica status de um pino específico
void fV_publishAllPinsStatus(void);   // Publica status de todos os pinos
String fS_getMqttStatus(void);        // Retorna status da conexão MQTT
String fS_getMqttUniqueId(void);      // Retorna ID único do módulo para MQTT

#endif // GLOBALS_H