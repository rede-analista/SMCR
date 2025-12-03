// Conteúdo de preferences_manager.cpp

#include "globals.h" // Inclui globals.h que já inclui include.h

// --- Implementação das funções genéricas para salvar/ler (mantidas) ---

void fV_salvarString(const char* vC_key, const String& vS_value) {
  preferences.begin("smcrgenc", false); // Namespace genérico (até 8 letras)
  preferences.putString(vC_key, vS_value);
  preferences.end();
  Serial.printf("DEBUG: Salvou String (generico): %s = %s\n", vC_key, vS_value.c_str());
}

String fS_lerString(const char* vC_key, const String& vS_defaultValue) {
  preferences.begin("smcrgenc", true);
  String vS_value = preferences.getString(vC_key, vS_defaultValue);
  preferences.end();
  Serial.printf("DEBUG: Leu String (generico): %s = %s (Default: %s)\n", vC_key, vS_value.c_str(), vS_defaultValue.c_str());
  return vS_value;
}

void fV_salvarInt(const char* vC_key, int vI_value) {
  preferences.begin("smcrgenc", false);
  preferences.putInt(vC_key, vI_value);
  preferences.end();
  Serial.printf("DEBUG: Salvou Int (generico): %s = %d\n", vC_key, vI_value);
}

int fI_lerInt(const char* vC_key, int vI_defaultValue) {
  preferences.begin("smcrgenc", true);
  int vI_value = preferences.getInt(vC_key, vI_defaultValue);
  preferences.end();
  Serial.printf("DEBUG: Leu Int (generico): %s = %d (Default: %d)\n", vC_key, vI_value, vI_defaultValue);
  return vI_value;
}

void fV_salvarBool(const char* vC_key, bool vB_value) {
  preferences.begin("smcrgenc", false);
  preferences.putBool(vC_key, vB_value);
  preferences.end();
  Serial.printf("DEBUG: Salvou Bool (generico): %s = %s\n", vC_key, vB_value ? "true" : "false");
}

bool fB_lerBool(const char* vC_key, bool vB_defaultValue) {
  preferences.begin("smcrgenc", true);
  bool vB_value = preferences.getBool(vC_key, vB_defaultValue);
  preferences.end();
  Serial.printf("DEBUG: Leu Bool (generico): %s = %s (Default: %s)\n", vC_key, vB_value ? "true" : "false", vB_defaultValue ? "true" : "false");
  return vB_value;
}


// --- Implementação das funções para Carregar/Salvar a MainConfig_t ---
// Namespace e chaves para a MainConfig_t
const char* NVS_NAMESPACE_MAIN_CONFIG = "smcrconf";
const char* NVS_NAMESPACE_GENERIC = "smcrgenc";
const char* KEY_SERIAL_DEBUG_ENABLED = "sdbgen";
const char* KEY_LOG_FLAGS = "logflags";

// Chaves da rede
const char* KEY_HOSTNAME = "hostname";
const char* KEY_WIFI_SSID = "w_ssid";
const char* KEY_WIFI_PASS = "w_pass";
const char* KEY_WIFI_ATTEMPTS = "w_attmp";
const char* KEY_AP_SSID = "a_ssid";
const char* KEY_AP_PASS = "a_pass";
const char* KEY_AP_FALLBACK = "a_fallb";
const char* KEY_WIFI_CHECK_INTERVAL = "w_chkint";
const char* KEY_NTP_SERVER1 = "ntp1";
const char* KEY_GMT_OFFSET_SEC = "gmtofs";
const char* KEY_DAYLIGHT_OFFSET_SEC = "dltofs";

// Chaves da interface web
const char* KEY_STATUS_PINOS = "st_pinos";
const char* KEY_INTER_MODULOS = "inter_mod";
const char* KEY_COR_COM_ALERTA = "cor_alert";
const char* KEY_COR_SEM_ALERTA = "cor_ok";
const char* KEY_TEMPO_REFRESH = "refresh";

// Chaves do watchdog
const char* KEY_WATCHDOG_ENABLED = "wdt_en";
const char* KEY_CLOCK_ESP32 = "clk_mhz";
const char* KEY_TEMPO_WATCHDOG = "wdt_time";

// Chaves dos pinos
const char* KEY_QTD_PINOS = "qtd_pinos";

// Chaves do servidor web e autenticação
const char* KEY_WEB_PORT = "web_port";
const char* KEY_AUTH_ENABLED = "auth_en";
const char* KEY_WEB_USERNAME = "web_user";
const char* KEY_WEB_PASSWORD = "web_pass";
const char* KEY_DASHBOARD_AUTH = "dash_auth";

// Chaves de histórico
const char* KEY_SHOW_ANALOG_HISTORY = "show_ahist";
const char* KEY_SHOW_DIGITAL_HISTORY = "show_dhist";

// Chaves de MQTT
const char* KEY_MQTT_ENABLED = "mqtt_en";
const char* KEY_MQTT_SERVER = "mqtt_srv";
const char* KEY_MQTT_PORT = "mqtt_port";
const char* KEY_MQTT_USER = "mqtt_user";
const char* KEY_MQTT_PASSWORD = "mqtt_pass";
const char* KEY_MQTT_TOPIC_BASE = "mqtt_topic";
const char* KEY_MQTT_PUBLISH_INTERVAL = "mqtt_pint";
const char* KEY_MQTT_HA_DISCOVERY = "mqtt_had";
const char* KEY_MQTT_HA_BATCH = "mqtt_hab";
const char* KEY_MQTT_HA_INTMS = "mqtt_haim";
const char* KEY_MQTT_HA_REPEAT = "mqtt_harp";

// Chaves de Inter-Módulos
const char* KEY_INTERMOD_ENABLED = "imod_en";
const char* KEY_INTERMOD_HEALTHCHECK = "imod_hchk";
const char* KEY_INTERMOD_MAX_FAILURES = "imod_mfail";
const char* KEY_INTERMOD_AUTO_DISCOVERY = "imod_adisc";

// Chaves de Telegram
const char* KEY_TELEGRAM_ENABLED = "tg_en";
const char* KEY_TELEGRAM_TOKEN = "tg_token";
const char* KEY_TELEGRAM_CHATID = "tg_chatid";
const char* KEY_TELEGRAM_INTERVAL = "tg_intval";

void fV_carregarMainConfig(void) {

    preferences.begin(NVS_NAMESPACE_MAIN_CONFIG, true); // Abre para leitura

    // 1. Configurações de Debug/Log
    vSt_mainConfig.vB_serialDebugEnabled = preferences.getBool(KEY_SERIAL_DEBUG_ENABLED, true);
    vSt_mainConfig.vU32_activeLogFlags = preferences.getUInt(KEY_LOG_FLAGS, LOG_FULL); 

    // 2. Configurações de Rede (WIFI STA)
    vSt_mainConfig.vS_hostname = preferences.getString(KEY_HOSTNAME, "");
    vSt_mainConfig.vS_wifiSsid = preferences.getString(KEY_WIFI_SSID, ""); 
    vSt_mainConfig.vS_wifiPass = preferences.getString(KEY_WIFI_PASS, ""); 
    vSt_mainConfig.vU16_wifiConnectAttempts = preferences.getUInt(KEY_WIFI_ATTEMPTS, 15);

    // 3. Configurações de Acesso (AP Fallback)
    vSt_mainConfig.vS_apSsid = preferences.getString(KEY_AP_SSID, "SMCR_AP_SETUP"); 
    vSt_mainConfig.vS_apPass = preferences.getString(KEY_AP_PASS, "senha1234");
    vSt_mainConfig.vB_apFallbackEnabled = preferences.getBool(KEY_AP_FALLBACK, true); // Padrão é TRUE
    
    // 4. Configurações de Checagem de Rede
    vSt_mainConfig.vU32_wifiCheckInterval = preferences.getULong(KEY_WIFI_CHECK_INTERVAL, 32000); // 15 segundos
    
    // 5. Configurações NTP
    vSt_mainConfig.vS_ntpServer1 = preferences.getString(KEY_NTP_SERVER1, "pool.ntp.br"); // Servidor NTP padrão do Brasil
    vSt_mainConfig.vI_gmtOffsetSec = preferences.getInt(KEY_GMT_OFFSET_SEC, -10800); // -3 horas em segundos
    vSt_mainConfig.vI_daylightOffsetSec = preferences.getInt(KEY_DAYLIGHT_OFFSET_SEC, 0); // 0 segundos por padrão

    // 6. Configurações da Interface Web
    vSt_mainConfig.vB_statusPinosEnabled = preferences.getBool(KEY_STATUS_PINOS, true);
    vSt_mainConfig.vB_interModulosEnabled = preferences.getBool(KEY_INTER_MODULOS, false);
    vSt_mainConfig.vS_corStatusComAlerta = preferences.getString(KEY_COR_COM_ALERTA, "#ff0000");
    vSt_mainConfig.vS_corStatusSemAlerta = preferences.getString(KEY_COR_SEM_ALERTA, "#00ff00");
    vSt_mainConfig.vU16_tempoRefresh = preferences.getUInt(KEY_TEMPO_REFRESH, 15);

    // 7. Configurações do Watchdog
    vSt_mainConfig.vB_watchdogEnabled = preferences.getBool(KEY_WATCHDOG_ENABLED, false);
    vSt_mainConfig.vU16_clockEsp32Mhz = preferences.getUInt(KEY_CLOCK_ESP32, 240);
    vSt_mainConfig.vU32_tempoWatchdogUs = preferences.getULong(KEY_TEMPO_WATCHDOG, 8000000);

    // 8. Configurações dos Pinos
    vSt_mainConfig.vU8_quantidadePinos = preferences.getUChar(KEY_QTD_PINOS, 16);

    // 9. Configurações do Servidor Web e Autenticação
    vSt_mainConfig.vU16_webServerPort = preferences.getUInt(KEY_WEB_PORT, 8080);
    vSt_mainConfig.vB_authEnabled = preferences.getBool(KEY_AUTH_ENABLED, false); // Padrão desabilitado
    vSt_mainConfig.vS_webUsername = preferences.getString(KEY_WEB_USERNAME, "admin");
    vSt_mainConfig.vS_webPassword = preferences.getString(KEY_WEB_PASSWORD, "admin1234");
    vSt_mainConfig.vB_dashboardAuthRequired = preferences.getBool(KEY_DASHBOARD_AUTH, false); // Dashboard sem auth por padrão

    // 10. Configurações de Histórico no Dashboard
    vSt_mainConfig.vB_showAnalogHistory = preferences.getBool(KEY_SHOW_ANALOG_HISTORY, true); // Padrão habilitado
    vSt_mainConfig.vB_showDigitalHistory = preferences.getBool(KEY_SHOW_DIGITAL_HISTORY, true); // Padrão desabilitado

    // 11. Configurações de MQTT
    vSt_mainConfig.vB_mqttEnabled = preferences.getBool(KEY_MQTT_ENABLED, false); // Padrão desabilitado
    vSt_mainConfig.vS_mqttServer = preferences.getString(KEY_MQTT_SERVER, "");
    vSt_mainConfig.vU16_mqttPort = preferences.getUInt(KEY_MQTT_PORT, 1883);
    vSt_mainConfig.vS_mqttUser = preferences.getString(KEY_MQTT_USER, "");
    vSt_mainConfig.vS_mqttPassword = preferences.getString(KEY_MQTT_PASSWORD, "");
    vSt_mainConfig.vS_mqttTopicBase = preferences.getString(KEY_MQTT_TOPIC_BASE, "smcr");
    vSt_mainConfig.vU16_mqttPublishInterval = preferences.getUInt(KEY_MQTT_PUBLISH_INTERVAL, 60); // 60 segundos
    vSt_mainConfig.vB_mqttHomeAssistantDiscovery = preferences.getBool(KEY_MQTT_HA_DISCOVERY, true); // Habilitado por padrão
    vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize = preferences.getUChar(KEY_MQTT_HA_BATCH, 4);
    vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs = preferences.getUInt(KEY_MQTT_HA_INTMS, 100);
    vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec = preferences.getUInt(KEY_MQTT_HA_REPEAT, 900); // 15 minutos

    // 12. Configurações de Inter-Módulos
    vSt_mainConfig.vB_interModEnabled = preferences.getBool(KEY_INTERMOD_ENABLED, false); // Padrão desabilitado
    vSt_mainConfig.vU16_interModHealthCheckInterval = preferences.getUInt(KEY_INTERMOD_HEALTHCHECK, 60); // 60 segundos
    vSt_mainConfig.vU8_interModMaxFailures = preferences.getUChar(KEY_INTERMOD_MAX_FAILURES, 3); // 3 falhas
    vSt_mainConfig.vB_interModAutoDiscovery = preferences.getBool(KEY_INTERMOD_AUTO_DISCOVERY, false); // Desabilitado por padrão (evita lentidão)

    // 13. Configurações de Telegram
    vSt_mainConfig.vB_telegramEnabled = preferences.getBool(KEY_TELEGRAM_ENABLED, false); // Padrão desabilitado
    vSt_mainConfig.vS_telegramToken = preferences.getString(KEY_TELEGRAM_TOKEN, "");
    vSt_mainConfig.vS_telegramChatId = preferences.getString(KEY_TELEGRAM_CHATID, "");
    vSt_mainConfig.vU16_telegramCheckInterval = preferences.getUInt(KEY_TELEGRAM_INTERVAL, 30); // 30 segundos

    preferences.end();

    if (vSt_mainConfig.vS_hostname.length() == 0) {
        // se for a primeira execução, atribui um hostname padrão baseado no ID do módulo
        vSt_mainConfig.vS_hostname = preferences.getString(KEY_HOSTNAME, "esp32modularx");
        if (vSt_mainConfig.vS_wifiSsid.length() == 0) {
          vSt_mainConfig.vB_apFallbackEnabled = true;
        }
    }

}

void fV_salvarMainConfig(void) {

    preferences.begin(NVS_NAMESPACE_MAIN_CONFIG, false); // Abre para escrita

    // 1. Configurações de Debug/Log
    preferences.putBool(KEY_SERIAL_DEBUG_ENABLED, vSt_mainConfig.vB_serialDebugEnabled);
    preferences.putUInt(KEY_LOG_FLAGS, vSt_mainConfig.vU32_activeLogFlags);

    // 2. Configurações de Rede (WIFI STA)
    preferences.putString(KEY_HOSTNAME, vSt_mainConfig.vS_hostname);
    preferences.putString(KEY_WIFI_SSID, vSt_mainConfig.vS_wifiSsid);
    preferences.putString(KEY_WIFI_PASS, vSt_mainConfig.vS_wifiPass);
    preferences.putUInt(KEY_WIFI_ATTEMPTS, vSt_mainConfig.vU16_wifiConnectAttempts);

    // 3. Configurações de Acesso (AP Fallback)
    preferences.putString(KEY_AP_SSID, vSt_mainConfig.vS_apSsid);
    preferences.putString(KEY_AP_PASS, vSt_mainConfig.vS_apPass);
    preferences.putBool(KEY_AP_FALLBACK, vSt_mainConfig.vB_apFallbackEnabled);

    // 4. Configurações de Checagem de Rede
    preferences.putULong(KEY_WIFI_CHECK_INTERVAL, vSt_mainConfig.vU32_wifiCheckInterval);
    
    // 5. Configurações NTP
    preferences.putString(KEY_NTP_SERVER1, vSt_mainConfig.vS_ntpServer1);
    preferences.putInt(KEY_GMT_OFFSET_SEC, vSt_mainConfig.vI_gmtOffsetSec);
    preferences.putInt(KEY_DAYLIGHT_OFFSET_SEC, vSt_mainConfig.vI_daylightOffsetSec);

    // 6. Configurações da Interface Web
    preferences.putBool(KEY_STATUS_PINOS, vSt_mainConfig.vB_statusPinosEnabled);
    preferences.putBool(KEY_INTER_MODULOS, vSt_mainConfig.vB_interModulosEnabled);
    preferences.putString(KEY_COR_COM_ALERTA, vSt_mainConfig.vS_corStatusComAlerta);
    preferences.putString(KEY_COR_SEM_ALERTA, vSt_mainConfig.vS_corStatusSemAlerta);
    preferences.putUInt(KEY_TEMPO_REFRESH, vSt_mainConfig.vU16_tempoRefresh);

    // 7. Configurações do Watchdog
    preferences.putBool(KEY_WATCHDOG_ENABLED, vSt_mainConfig.vB_watchdogEnabled);
    preferences.putUInt(KEY_CLOCK_ESP32, vSt_mainConfig.vU16_clockEsp32Mhz);
    preferences.putULong(KEY_TEMPO_WATCHDOG, vSt_mainConfig.vU32_tempoWatchdogUs);

    // 8. Configurações dos Pinos
    preferences.putUChar(KEY_QTD_PINOS, vSt_mainConfig.vU8_quantidadePinos);

    // 9. Configurações do Servidor Web e Autenticação
    preferences.putUInt(KEY_WEB_PORT, vSt_mainConfig.vU16_webServerPort);
    preferences.putBool(KEY_AUTH_ENABLED, vSt_mainConfig.vB_authEnabled);
    preferences.putString(KEY_WEB_USERNAME, vSt_mainConfig.vS_webUsername);
    preferences.putString(KEY_WEB_PASSWORD, vSt_mainConfig.vS_webPassword);
    preferences.putBool(KEY_DASHBOARD_AUTH, vSt_mainConfig.vB_dashboardAuthRequired);

    // 10. Configurações de Histórico no Dashboard
    preferences.putBool(KEY_SHOW_ANALOG_HISTORY, vSt_mainConfig.vB_showAnalogHistory);
    preferences.putBool(KEY_SHOW_DIGITAL_HISTORY, vSt_mainConfig.vB_showDigitalHistory);

    // 11. Configurações de MQTT
    preferences.putBool(KEY_MQTT_ENABLED, vSt_mainConfig.vB_mqttEnabled);
    preferences.putString(KEY_MQTT_SERVER, vSt_mainConfig.vS_mqttServer);
    preferences.putUInt(KEY_MQTT_PORT, vSt_mainConfig.vU16_mqttPort);
    preferences.putString(KEY_MQTT_USER, vSt_mainConfig.vS_mqttUser);
    preferences.putString(KEY_MQTT_PASSWORD, vSt_mainConfig.vS_mqttPassword);
    preferences.putString(KEY_MQTT_TOPIC_BASE, vSt_mainConfig.vS_mqttTopicBase);
    preferences.putUInt(KEY_MQTT_PUBLISH_INTERVAL, vSt_mainConfig.vU16_mqttPublishInterval);
    preferences.putBool(KEY_MQTT_HA_DISCOVERY, vSt_mainConfig.vB_mqttHomeAssistantDiscovery);
    preferences.putUChar(KEY_MQTT_HA_BATCH, vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize);
    preferences.putUInt(KEY_MQTT_HA_INTMS, vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs);
    preferences.putUInt(KEY_MQTT_HA_REPEAT, vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec);

    // 12. Configurações de Inter-Módulos
    preferences.putBool(KEY_INTERMOD_ENABLED, vSt_mainConfig.vB_interModEnabled);
    preferences.putUInt(KEY_INTERMOD_HEALTHCHECK, vSt_mainConfig.vU16_interModHealthCheckInterval);
    preferences.putUChar(KEY_INTERMOD_MAX_FAILURES, vSt_mainConfig.vU8_interModMaxFailures);
    preferences.putBool(KEY_INTERMOD_AUTO_DISCOVERY, vSt_mainConfig.vB_interModAutoDiscovery);

    // 13. Configurações de Telegram
    preferences.putBool(KEY_TELEGRAM_ENABLED, vSt_mainConfig.vB_telegramEnabled);
    preferences.putString(KEY_TELEGRAM_TOKEN, vSt_mainConfig.vS_telegramToken);
    preferences.putString(KEY_TELEGRAM_CHATID, vSt_mainConfig.vS_telegramChatId);
    preferences.putUInt(KEY_TELEGRAM_INTERVAL, vSt_mainConfig.vU16_telegramCheckInterval);

    preferences.end();

    fV_printSerialDebug(LOG_FLASH, "MainConfig_t salva na flash.");
}

//=======================================
// Limpa todos os namespaces (Reset de Fábrica)
//=======================================
void fV_clearPreferences(void) {

    fV_printSerialDebug(LOG_FLASH, "Abrindo Preferences para LIMPEZA GERAL (Reset de Fabrica)...");

  // 1. Limpa o namespace principal (MainConfig_t)
    preferences.begin(NVS_NAMESPACE_MAIN_CONFIG, false);
    preferences.clear();
    preferences.end();
    
  // 2. Limpa o namespace genérico (chaves avulsas)
    preferences.begin(NVS_NAMESPACE_GENERIC, false);
    preferences.clear();
    preferences.end();

  // 3. Limpa namespaces legados não mais utilizados pelo app
  //    smcr-config (antigo principal) e smcr_generic_configs (antigo genérico)
  preferences.begin("smcr-config", false);
  preferences.clear();
  preferences.end();

  preferences.begin("smcr_generic_configs", false);
  preferences.clear();
  preferences.end();

    fV_printSerialDebug(LOG_FLASH, "Limpeza de fabrica concluida. Configs de rede e sistema apagadas.");
}

// Limpa todas as configurações EXCETO as de rede (hostname, SSID, senha)
void fV_clearConfigExceptNetwork(void) {
    fV_printSerialDebug(LOG_FLASH, "Abrindo Preferences para LIMPEZA SELETIVA (preservando rede)...");

    // Salva temporariamente as configurações de rede
    String tempHostname = vSt_mainConfig.vS_hostname;
    String tempWifiSsid = vSt_mainConfig.vS_wifiSsid;
    String tempWifiPass = vSt_mainConfig.vS_wifiPass;
    
    // 1. Limpa o namespace principal (MainConfig_t)
    preferences.begin(NVS_NAMESPACE_MAIN_CONFIG, false);
    preferences.clear();
    
    // 2. Restaura apenas as configurações de rede usando as chaves corretas
    preferences.putString(KEY_HOSTNAME, tempHostname);
    preferences.putString(KEY_WIFI_SSID, tempWifiSsid);
    preferences.putString(KEY_WIFI_PASS, tempWifiPass);
    preferences.end();
    
    // 3. Limpa o namespace genérico (chaves avulsas) - mantém tudo limpo
    preferences.begin(NVS_NAMESPACE_GENERIC, false);
    preferences.clear();
    preferences.end();
    
    // 4. Limpa configurações de pinos (arquivo LittleFS)
    fV_clearPinConfigs();

    fV_printSerialDebug(LOG_FLASH, "Limpeza seletiva concluida. Rede preservada: %s", tempHostname.c_str());
}