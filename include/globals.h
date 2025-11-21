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
    LOG_NETWORK     = (1 << 1),  // 2 - Log de rede (Wi-Fi, MQTT, etc.)
    LOG_PINS        = (1 << 2),  // 4 - Log de pinos e GPIO
    LOG_FLASH       = (1 << 3),  // 8 - Log de operações de flash (Preferences, LittleFS)
    LOG_WEB         = (1 << 4),  // 16 - Log da interface web (requisições, respostas)
    LOG_SENSOR      = (1 << 5),  // 32 - Log de leituras de sensores
    LOG_ACTIONS     = (1 << 6),  // 64 - Log de execução de ações
    LOG_INTERMOD    = (1 << 7),  // 128 - Log de comunicação entre módulos
    LOG_WATCHDOG    = (1 << 8),  // 256 - Log do Watchdog Timer
    
    // Log Total: uma soma de todas as flags relevantes
    LOG_FULL        = LOG_INIT | LOG_NETWORK | LOG_PINS | LOG_FLASH | LOG_WEB | 
                      LOG_SENSOR | LOG_ACTIONS | LOG_INTERMOD | LOG_WATCHDOG
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

};

// Instância global da sua configuração em memória (sua running-config)
extern MainConfig_t vSt_mainConfig; // vSt_ para variável do tipo Struct

// --- Novas funções para carregar e salvar a estrutura MainConfig_t ---
// Estas serão as funções de interface para a sua "startup-config"
void fV_carregarMainConfig(void); // Carrega da Flash para vSt_mainConfig
void fV_salvarMainConfig(void);   // Salva de vSt_mainConfig para a Flash
void fV_clearPreferences(void) ; // Limpa todas as preferências (Reset de Fábrica)


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
void fV_handleSoftReset(AsyncWebServerRequest *request); // Handler para reinicialização simples
void fV_handleFactoryReset(AsyncWebServerRequest *request); // Handler para reset de fábrica
void fV_handleNetworkReset(AsyncWebServerRequest *request); // Handler para reset de rede
void fV_handleNotFound(AsyncWebServerRequest *request); // Handler 404

/* Funções de NTP (ntp_func.cpp) */
void fV_setupNtp();
String fS_getFormattedTime();
String fS_getNtpStatus();

#endif // GLOBALS_H