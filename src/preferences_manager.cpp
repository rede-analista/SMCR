// Conteúdo de preferences_manager.cpp

#include "globals.h" // Inclui globals.h que já inclui include.h

// --- Implementação das funções genéricas para salvar/ler (mantidas) ---

void fV_salvarString(const char* vC_key, const String& vS_value) {
  preferences.begin("smcr_generic_configs", false); // Namespace genérico para chaves avulsas
  preferences.putString(vC_key, vS_value);
  preferences.end();
  Serial.printf("DEBUG: Salvou String (generico): %s = %s\n", vC_key, vS_value.c_str());
}

String fS_lerString(const char* vC_key, const String& vS_defaultValue) {
  preferences.begin("smcr_generic_configs", true);
  String vS_value = preferences.getString(vC_key, vS_defaultValue);
  preferences.end();
  Serial.printf("DEBUG: Leu String (generico): %s = %s (Default: %s)\n", vC_key, vS_value.c_str(), vS_defaultValue.c_str());
  return vS_value;
}

void fV_salvarInt(const char* vC_key, int vI_value) {
  preferences.begin("smcr_generic_configs", false);
  preferences.putInt(vC_key, vI_value);
  preferences.end();
  Serial.printf("DEBUG: Salvou Int (generico): %s = %d\n", vC_key, vI_value);
}

int fI_lerInt(const char* vC_key, int vI_defaultValue) {
  preferences.begin("smcr_generic_configs", true);
  int vI_value = preferences.getInt(vC_key, vI_defaultValue);
  preferences.end();
  Serial.printf("DEBUG: Leu Int (generico): %s = %d (Default: %d)\n", vC_key, vI_value, vI_defaultValue);
  return vI_value;
}

void fV_salvarBool(const char* vC_key, bool vB_value) {
  preferences.begin("smcr_generic_configs", false);
  preferences.putBool(vC_key, vB_value);
  preferences.end();
  Serial.printf("DEBUG: Salvou Bool (generico): %s = %s\n", vC_key, vB_value ? "true" : "false");
}

bool fB_lerBool(const char* vC_key, bool vB_defaultValue) {
  preferences.begin("smcr_generic_configs", true);
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
    vSt_mainConfig.vS_ntpServer1 = preferences.getString(KEY_NTP_SERVER1, "pool.ntp.br"); // Servidor NTP padrão do Brasil
    vSt_mainConfig.vI_gmtOffsetSec = preferences.getInt(KEY_GMT_OFFSET_SEC, -10800); // -3 horas em segundos
    vSt_mainConfig.vI_daylightOffsetSec = preferences.getInt(KEY_DAYLIGHT_OFFSET_SEC, 0); // 0 segundos por padrão

    preferences.end();

    if (vSt_mainConfig.vS_hostname.length() == 0) {
        // se for a primeira execução, atribui um hostname padrão baseado no ID do módulo
        vSt_mainConfig.vS_hostname = preferences.getString(KEY_HOSTNAME, fS_idModulo());
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
    preferences.putString(KEY_NTP_SERVER1, vSt_mainConfig.vS_ntpServer1);
    preferences.putInt(KEY_GMT_OFFSET_SEC, vSt_mainConfig.vI_gmtOffsetSec);
    preferences.putInt(KEY_DAYLIGHT_OFFSET_SEC, vSt_mainConfig.vI_daylightOffsetSec);

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

    fV_printSerialDebug(LOG_FLASH, "Limpeza de fabrica concluida. Configs de rede e sistema apagadas.");
}