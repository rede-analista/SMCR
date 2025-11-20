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
// Chaves para as configurações da MainConfig_t
const char* NVS_NAMESPACE_MAIN_CONFIG = "smcrconf"; // Novo namespace para a struct principal
const char* KEY_SERIAL_DEBUG_ENABLED = "s_dbg_en"; // Chave curta para dentro do NVS
const char* KEY_LOG_FLAGS = "log_flag";
const char* KEY_HOSTNAME = "hostname";
const char* KEY_AP_FALLBACK = "ap_fallb";


void fV_carregarMainConfig(void) {
    preferences.begin(NVS_NAMESPACE_MAIN_CONFIG, true); // Abre para leitura

    // Carrega cada membro da struct com um valor padrão para o primeiro boot
    vSt_mainConfig.vB_serialDebugEnabled = preferences.getBool(KEY_SERIAL_DEBUG_ENABLED, true);
    vSt_mainConfig.vU32_activeLogFlags = preferences.getUInt(KEY_LOG_FLAGS, LOG_INIT | LOG_NETWORK | LOG_PINS); // Usar getUInt para uint32_t
    vSt_mainConfig.vS_hostname = preferences.getString(KEY_HOSTNAME, "");
    vSt_mainConfig.vB_apFallbackEnabled = preferences.getBool(KEY_AP_FALLBACK, true);

    preferences.end();

    if (vSt_mainConfig.vS_hostname.length() == 0) {
        vSt_mainConfig.vS_hostname = fS_idModulo();
        fV_printSerialDebug(LOG_FLASH, "Primeira execucao detectada. Gerando um hostname.");

        // Se o SSID principal estiver vazio,
        // garantimos que o AP Fallback esteja forçadamente ativado
        // para evitar o loop de reinicialização.
        if (vSt_mainConfig.vS_wifiSsid.length() == 0) {
            vSt_mainConfig.vB_apFallbackEnabled = true;
            fV_printSerialDebug(LOG_FLASH, "Primeira execucao detectada. Fallback AP forcado como ATIVADO.");
        }        
    }

    // Neste ponto, vSt_mainConfig está preenchida com os valores da flash ou padrões.
}

void fV_salvarMainConfig(void) {
    preferences.begin(NVS_NAMESPACE_MAIN_CONFIG, false); // Abre para escrita

    // Salva cada membro da struct
    preferences.putBool(KEY_SERIAL_DEBUG_ENABLED, vSt_mainConfig.vB_serialDebugEnabled);
    preferences.putUInt(KEY_LOG_FLAGS, vSt_mainConfig.vU32_activeLogFlags); // Usar putUInt para uint32_t
    preferences.putString(KEY_HOSTNAME, vSt_mainConfig.vS_hostname);
    preferences.putBool(KEY_AP_FALLBACK, vSt_mainConfig.vB_apFallbackEnabled);

    preferences.end();
    // Opcional: Adicionar um log para confirmar que as configurações foram salvas
    // fV_printSerialDebug(LOG_FLASH, "MainConfig_t salva na flash."); // Não pode usar aqui diretamente, pois LOG_FLASH pode estar desativado
}

// A implementação de fV_printSerialDebug foi movida para utils.cpp