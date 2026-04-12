// Conteúdo de  globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

/*=======================================
Inclusão de bibliotecas
*/
#include "include.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Versão do firmware atual
#define FIRMWARE_VERSION "2.2.2"


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
    uint32_t vU32_mqttHaDiscoveryRepeatSec;  // Intervalo para re-executar discovery completo (segundos, padrão 900 = 15min)

    // 12. Configurações de Inter-Módulos
    bool vB_interModEnabled;        // Habilita/desabilita comunicação inter-módulos
    uint16_t vU16_interModHealthCheckInterval; // Intervalo de healthcheck em segundos (padrão 30)
    uint8_t vU8_interModMaxFailures; // Quantidade de falhas antes de marcar offline (padrão 3)
    bool vB_interModAutoDiscovery;  // Habilita descoberta automática via mDNS (padrão true)

    // 13. Configurações de Telegram
    bool vB_telegramEnabled;        // Habilita/desabilita notificações por Telegram
    String vS_telegramToken;        // Token do bot Telegram
    String vS_telegramChatId;       // Chat ID do destinatário
    uint16_t vU16_telegramCheckInterval; // Intervalo de verificação de mensagens em segundos (padrão 30)

    // 14. Configurações SMCR Cloud
    String vS_cloudUrl;                  // URL da cloud SMCR (padrão: smcr.pensenet.com.br)
    uint16_t vU16_cloudPort;             // Porta HTTP da cloud (padrão: 8765)
    bool vB_cloudSyncEnabled;            // Habilita busca periódica de configurações na cloud
    uint16_t vU16_cloudSyncIntervalMin;  // Intervalo de sync em minutos (padrão 5)
    String vS_cloudApiToken;             // Token API para autenticação na cloud
    bool vB_cloudHeartbeatEnabled;       // Habilita envio periódico de heartbeat para a cloud
    uint16_t vU16_cloudHeartbeatIntervalMin; // Intervalo do heartbeat em minutos (padrão 2)

};

// --- Estrutura para Configuração de Pinos ---
struct PinConfig_t {
    String nome;                 // Nome descritivo do pino
    uint16_t pino;              // Número físico do GPIO (0-65535)
    uint16_t tipo;              // Tipo: 0=Sem Uso, 1=Digital, 192=Analógico(ADC), 193=PWM(Saída), 65533=Remoto Analógico, 65534=Remoto Digital
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
    // MQTT Discovery
    String classe_mqtt;          // Classe do dispositivo no MQTT/Home Assistant (ex: switch, light, sensor)
    String icone_mqtt;           // Ícone MDI para o dispositivo (ex: mdi:light-switch, mdi:led-on)
};

// --- Estrutura para Configuração de Ações ---
struct ActionConfig_t {
    uint16_t pino_origem;        // GPIO que dispara a ação (pino de entrada/sensor)
    uint8_t numero_acao;         // Número da ação (1, 2, 3 ou 4)
    uint16_t pino_destino;       // GPIO que será acionado (pino de saída/controle)
    uint16_t acao;               // Tipo de ação: 0=NENHUMA, 1=LIGA, 2=LIGA_DELAY, 3=PISCA, 4=PULSO, 5=PULSO_DELAY_ON, 65534=STATUS, 65535=SINCRONISMO
    uint16_t tempo_on;           // Tempo ON em ciclos (0-65535)
    uint16_t tempo_off;          // Tempo OFF em ciclos (0-65535)
    uint16_t pino_remoto;        // Número do pino no módulo remoto (0-65535)
    String envia_modulo;         // ID do módulo destino (string vazia=nenhum, ou hostname/ID do módulo)
    bool telegram;               // Envia notificação para Telegram
    bool assistente;             // Envia notificação para Assistente
    // Controle interno da ação
    uint16_t contador_on;        // Contador atual de ciclos ON
    uint16_t contador_off;       // Contador atual de ciclos OFF
    bool estado_acao;            // Estado atual da ação (true=executando, false=parada)
    bool ultimo_estado_origem;   // Último estado do pino origem (para detectar mudanças)
};

// --- Estrutura para Módulos Inter-Comunicação ---
struct InterModConfig_t {
    String id;                   // ID único do módulo (MAC address ou UUID)
    String hostname;             // Hostname do módulo
    String ip;                   // Endereço IP do módulo
    uint16_t porta;              // Porta do servidor web do módulo
    bool online;                 // Status online/offline
    uint8_t falhas_consecutivas; // Contador de falhas de healthcheck
    unsigned long ultimo_healthcheck; // Timestamp do último healthcheck bem-sucedido
    bool auto_descoberto;        // Se foi descoberto via mDNS ou cadastrado manualmente
    // Alerta de offline
    String pins_offline;              // GPIOs que piscam quando módulo fica offline (ex: "3,4")
    bool offline_alert_enabled;       // Habilitar piscar ao ficar offline
    uint16_t offline_flash_ms;        // Intervalo de piscada offline em ms (padrão 500)
    // Alerta de healthcheck
    String pins_healthcheck;          // GPIOs que piscam durante healthcheck (ex: "5")
    bool healthcheck_alert_enabled;   // Habilitar piscar durante healthcheck
    uint16_t healthcheck_flash_ms;    // Intervalo de piscada healthcheck em ms (padrão 200)
    // Controle de flash em runtime (não salvo)
    unsigned long ultimo_offline_flash;   // Timestamp do último toggle offline
    bool offline_flash_state;             // Estado atual do flash offline
    unsigned long ultimo_hc_flash;        // Timestamp do último toggle healthcheck
    bool hc_flash_state;                  // Estado atual do flash healthcheck
};

// --- Fila de reenvio para alertas inter-módulos ---
#define REMOTE_QUEUE_SIZE 10

struct RemoteQueueItem_t {
    String   moduleId;   // ID do módulo destino
    uint16_t remotePin;  // Pino no módulo remoto
    uint16_t value;      // Valor a enviar (0/1 digital ou 0-4095 analógico)
    bool     active;     // Slot em uso
};

extern RemoteQueueItem_t vA_remoteQueue[REMOTE_QUEUE_SIZE];

// --- Estrutura para Log de Comunicações Inter-Módulos ---
struct InterModCommLog_t {
    String time;                 // Horário da comunicação (HH:MM:SS)
    String module;               // Nome do módulo (from/to)
    uint16_t pin;                // Número do pino
    uint16_t value;              // Valor (0-65535, suporta analógico 0-4095)
    bool is_sent;                // true=enviado, false=recebido
};

// Instância global da sua configuração em memória (sua running-config)
extern MainConfig_t vSt_mainConfig; // vSt_ para variável do tipo Struct

// --- Constante e latch de interrupção para detecção de pulsos rápidos ---
#define MAX_GPIO_NUM 40  // ESP32 clássico: GPIO 0-39
extern volatile bool vB_gpioActivationLatch[MAX_GPIO_NUM];

// --- Variáveis globais para gerenciamento de pinos ---
extern PinConfig_t* vA_pinConfigs;   // Array dinâmico de configurações de pinos
extern uint8_t vU8_activePinsCount;  // Contador de pinos ativos configurados

// --- Variáveis globais para gerenciamento de ações ---
extern ActionConfig_t* vA_actionConfigs;  // Array dinâmico de configurações de ações
extern uint8_t vU8_activeActionsCount;    // Contador de ações ativas configuradas

// --- Variáveis globais para gerenciamento de inter-módulos ---
extern InterModConfig_t* vA_interModConfigs;  // Array dinâmico de módulos cadastrados
extern uint8_t vU8_activeInterModCount;       // Contador de módulos ativos
extern bool vB_firstDiscoveryDone;            // Flag para controlar primeira execução do discovery

// --- Variáveis globais para log de comunicações inter-módulos ---
#define MAX_INTERMOD_COMM_LOG 5  // Máximo de 5 comunicações de cada tipo
extern InterModCommLog_t vSt_InterModCommReceived[MAX_INTERMOD_COMM_LOG];
extern InterModCommLog_t vSt_InterModCommSent[MAX_INTERMOD_COMM_LOG];
extern uint8_t vU8_InterModCommReceivedIndex; // Índice do próximo slot (circular buffer)
extern uint8_t vU8_InterModCommSentIndex;     // Índice do próximo slot (circular buffer)

// Funções de gerenciamento do log
void fV_logInterModReceived(const String& module, uint16_t pin, uint16_t value);
void fV_logInterModSent(const String& module, uint16_t pin, uint16_t value);

// Funções da fila de reenvio
void fV_enqueueRemoteAction(const String& moduleId, uint16_t remotePin, uint16_t value);
void fV_processRemoteQueue(void);

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
uint8_t fU8_findPinIndex(uint16_t pinNumber); // Encontra índice do pino no array
int fI_addPinConfig(const PinConfig_t& config); // Adiciona nova configuração de pino
bool fB_removePinConfig(uint16_t pinNumber);    // Remove configuração de pino
bool fB_updatePinConfig(uint16_t pinNumber, const PinConfig_t& config); // Atualiza configuração


//Gera um ID para o módulo com base nas informações do chip ESP32
String fS_idModulo();
// Função para imprimir na serial com controle de flags
void fV_printSerialDebug(uint32_t vU32_messageFlag, const char *vC_format, ...);


// --- Mutex para proteção de vA_pinConfigs e vA_actionConfigs entre cores ---
extern SemaphoreHandle_t vO_pinActionMutex;

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
uint8_t fU8_findPinIndex(uint16_t pinNumber);
int fI_addPinConfig(const PinConfig_t& config);
bool fB_removePinConfig(uint16_t pinNumber);
bool fB_updatePinConfig(uint16_t pinNumber, const PinConfig_t& config);
bool fB_hasPinConfigChanges(void);
void fV_updatePinStatus(void);
void fV_readPinsTask(void); // Task periódica para leitura de pinos (ignora remotos)
bool fB_isPinActivated(uint8_t pinIndex);  // Verifica se pino está acionado baseado no nível configurado

/* Funções do Gerenciador de Ações (action_manager.cpp) */
void fV_initActionSystem(void);
void fV_loadActionConfigs(void);
bool fB_saveActionConfigs(void);
void fV_clearActionConfigs(void);
uint8_t fU8_findActionIndex(uint16_t pinOrigem, uint8_t numeroAcao);
int fI_addActionConfig(const ActionConfig_t& config);
bool fB_removeActionConfig(uint16_t pinOrigem, uint8_t numeroAcao);
bool fB_updateActionConfig(uint16_t pinOrigem, uint8_t numeroAcao, const ActionConfig_t& config);
bool fB_isPinUsedByActions(uint16_t pinNumber); // Verifica se pino está em uso por ações
void fV_executeActionsTask(void); // Task periódica para execução de ações
void fV_executeAction(uint8_t actionIndex); // Executa uma ação específica
void fV_syncRemotePinsOnBoot(void); // Sincroniza todos os pinos remotos após inicialização
void fV_processPendingRemoteSends(void); // Processa envios HTTP pendentes (chamado fora do mutex)

/* Funções do SMCR Cloud (cloud_sync.cpp) */
void fV_cloudSyncTask(void);               // Executa sincronização com a cloud SMCR
String fS_getCloudSyncStatus(void);        // Retorna último status de sync
String fS_getCloudSyncLastTime(void);      // Retorna horário do último sync
extern bool vB_pendingCloudSync;           // Flag para forçar sync imediato
void fV_fetchCloudFilesTask(void);         // Baixa arquivos HTML da cloud para LittleFS
String fS_getFetchCloudFilesStatus(void);  // Retorna status do download
extern bool vB_pendingFetchCloudFiles;     // Flag para iniciar download
extern String vS_fetchCloudFilesUrl;       // URL do servidor para download
void fV_cloudHeartbeatTask(void);          // Envia heartbeat de status para a cloud SMCR
String fS_getCloudHeartbeatStatus(void);   // Retorna último status do heartbeat
String fS_getCloudHeartbeatLastTime(void); // Retorna horário do último heartbeat

/* Funções do Gerenciador de Telegram (telegram_manager.cpp) */
void fV_initTelegram(void);
void fV_telegramLoop(void);
bool fB_sendTelegramMessage(const String& message);
void fV_sendTelegramActionNotification(const ActionConfig_t* action, const String& pinOrigemNome, const String& pinDestinoNome);
bool fB_sendRemoteAction(const String& moduleId, uint16_t remotePin, bool state); // Envia ação digital para módulo remoto
bool fB_sendRemoteAction(const String& moduleId, uint16_t remotePin, uint16_t value); // Envia valor analógico para módulo remoto
bool fB_requestPinSyncFromModule(const String& moduleId); // Solicita sincronização de pinos de um módulo específico
extern bool vB_pendingModuleSyncRequest; // Flag para sincronização pendente (executada na loop)
extern String vS_pendingModuleSyncId;   // ID do módulo aguardando sincronização

/* Funções do Gerenciador de MQTT (mqtt_manager.cpp) */
void fV_initMqtt(void);               // Inicializa sistema MQTT
void fV_setupMqtt(void);              // Configura conexão MQTT (não bloqueante)
void fV_mqttLoop(void);               // Loop MQTT (reconexão automática não bloqueante)
void fV_publishMqttDiscovery(void);   // Publica configurações de auto-discovery do Home Assistant
void fV_publishPinStatus(uint8_t pinIndex); // Publica status de um pino específico
void fV_publishAllPinsStatus(void);   // Publica status de todos os pinos
String fS_getMqttStatus(void);        // Retorna status da conexão MQTT
String fS_getMqttUniqueId(void);      // Retorna ID único do módulo para MQTT

/* Funções do Gerenciador de Inter-Módulos (intermod_manager.cpp) */
void fV_initInterModSystem(void);              // Inicializa sistema de inter-módulos
void fV_loadInterModConfigs(void);             // Carrega módulos cadastrados do LittleFS
bool fB_saveInterModConfigs(void);             // Salva módulos cadastrados no LittleFS
void fV_clearInterModConfigs(void);            // Limpa todas as configurações de módulos
int fI_addInterModConfig(const InterModConfig_t& config); // Adiciona novo módulo
bool fB_removeInterModConfig(const String& id); // Remove módulo por ID
bool fB_updateInterModConfig(const String& id, const InterModConfig_t& config); // Atualiza módulo
int fI_findInterModIndex(const String& id);    // Encontra índice do módulo por ID
void fV_interModHealthCheckTask(void);         // Task de healthcheck periódico
void fV_interModDiscoveryTask(void);           // Task de descoberta automática via mDNS
void fV_interModAlertFlashTask(void);          // Task de piscar LEDs de alerta quando módulo offline
bool fB_checkModuleHealth(uint8_t moduleIndex); // Verifica saúde de um módulo específico
String fS_getInterModStatus(void);             // Retorna resumo de status dos módulos

#endif // GLOBALS_H