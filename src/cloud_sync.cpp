// Conteúdo de cloud_sync.cpp
// Gerenciamento de sincronização de configurações com a cloud SMCR

#include "globals.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <LittleFS.h>

bool vB_pendingCloudSync = false;
static String vS_cloudSyncStatus = "Nunca sincronizado";
static uint32_t vU32_lastSyncHash = 0;

// Hash FNV-1a 32-bit do payload — leve e sem dependências
static uint32_t fU32_hashPayload(const String& s) {
    uint32_t hash = 2166136261UL;
    for (unsigned int i = 0; i < s.length(); i++) {
        hash ^= (uint8_t)s[i];
        hash *= 16777619UL;
    }
    return hash;
}

// ── Download de arquivos HTML da cloud ────────────────────────────────────────
bool vB_pendingFetchCloudFiles = false;
String vS_fetchCloudFilesUrl   = "smcr.pensenet.com.br";
static String vS_fetchStatus = "Aguardando";

// ── Heartbeat ─────────────────────────────────────────────────────────────────
static String vS_heartbeatStatus   = "Nunca enviado";
static String vS_heartbeatLastTime = "";
static String vS_cloudSyncLastTime = "";

String fS_getCloudHeartbeatStatus(void) { return vS_heartbeatStatus; }
String fS_getCloudHeartbeatLastTime(void) { return vS_heartbeatLastTime; }
String fS_getCloudSyncLastTime(void) { return vS_cloudSyncLastTime; }

// Monta base URL com porta: http(s)://host:porta
static String fS_cloudBaseUrl(void) {
    bool https = vSt_mainConfig.vB_cloudUseHttps;
    String base = (https ? "https://" : "http://") + vSt_mainConfig.vS_cloudUrl;
    uint16_t defaultPort = https ? 443 : 80;
    if (vSt_mainConfig.vU16_cloudPort > 0 && vSt_mainConfig.vU16_cloudPort != defaultPort) {
        base += ":" + String(vSt_mainConfig.vU16_cloudPort);
    }
    return base;
}
bool vB_fetchDone  = false;
bool vB_fetchError = false;

String fS_getFetchCloudFilesStatus(void) {
    return vS_fetchStatus;
}

String fS_getCloudSyncStatus(void) {
    return vS_cloudSyncStatus;
}

// =============================================================================
// REFERÊNCIA DE CAMPOS - JSON retornado pelo site SMCR Cloud (get_config.php)
// Todos os campos são opcionais. Apenas os presentes no JSON serão aplicados.
// =============================================================================
// REDE:           hostname, wifi_ssid, wifi_pass, wifi_attempts, wifi_check_interval
// AP FALLBACK:    ap_ssid, ap_pass, ap_fallback_enabled
// NTP:            ntp_server1, gmt_offset, daylight_offset
// INTERFACE:      status_pinos_enabled, inter_modulos_enabled, cor_com_alerta,
//                 cor_sem_alerta, tempo_refresh, show_analog_history, show_digital_history
// SISTEMA:        qtd_pinos, serial_debug_enabled, active_log_flags,
//                 watchdog_enabled, tempo_watchdog_us, clock_esp32_mhz
// SERVIDOR WEB:   web_server_port, auth_enabled, web_username, web_password,
//                 dashboard_auth_required
// MQTT:           mqtt_enabled, mqtt_server, mqtt_port, mqtt_user, mqtt_password,
//                 mqtt_topic_base, mqtt_publish_interval, mqtt_ha_discovery,
//                 mqtt_ha_batch, mqtt_ha_interval_ms, mqtt_ha_repeat_sec
// INTER-MÓDULOS:  intermod_enabled, intermod_healthcheck, intermod_max_failures,
//                 intermod_auto_discovery
// MÓDULOS:        intermod_modules[] (module_id, hostname, ip, porta,
//                 pins_offline, offline_alert_enabled, offline_flash_ms,
//                 pins_healthcheck, healthcheck_alert_enabled, healthcheck_flash_ms)
// TELEGRAM:       telegram_enabled, telegram_token, telegram_chatid, telegram_interval
// PINOS:          pins[] (array de objetos com campos do PinConfig_t)
// AÇÕES:          actions[] (array de objetos com campos do ActionConfig_t)
// =============================================================================
// AÇÕES (executadas após sync completo, auto-desativadas pela cloud):
//   reboot_on_sync      — reinicia o ESP32 após aplicar as configurações
//   ota_update_on_sync  — baixa e instala o firmware mais recente do GitHub via OTA
// NÃO SINCRONIZAR (configurações locais do ESP32, não devem vir da cloud):
//   cloud_url, cloud_port, cloud_sync_enabled, cloud_sync_interval_min, cloud_api_token
// =============================================================================

// =============================================================================
// OTA via GitHub — busca o firmware mais recente e instala via streaming
// =============================================================================
void fV_cloudOtaFromGitHub(void) {
    if (vSt_mainConfig.vB_watchdogEnabled) {
        esp_task_wdt_delete(NULL); // loop() bloqueado durante download — desativa WDT para não reiniciar
    }
    fV_printSerialDebug(LOG_NETWORK, "[OTA] Iniciando update via GitHub...");

    String tag = "";
    String binUrl = "";

    // ── 1. Busca a versão mais recente na API do GitHub ───────────────
    // Escopo fechado para garantir que secClient seja destruído antes do download
    {
        WiFiClientSecure secClient;
        secClient.setInsecure();

        HTTPClient http;
        http.setTimeout(10000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.addHeader("User-Agent", "ESP32-SMCR");

        if (!http.begin(secClient, "https://api.github.com/repos/rede-analista/SMCR/releases/latest")) {
            fV_printSerialDebug(LOG_NETWORK, "[OTA] Falha ao conectar na API GitHub");
            return;
        }

        int apiCode = http.GET();
        if (apiCode != HTTP_CODE_OK) {
            http.end();
            fV_printSerialDebug(LOG_NETWORK, "[OTA] Erro HTTP na API GitHub: %d", apiCode);
            return;
        }

        JsonDocument apiDoc;
        JsonDocument filter;
        filter["tag_name"] = true;
        WiFiClient* apiStream = http.getStreamPtr();
        DeserializationError err = deserializeJson(apiDoc, *apiStream, DeserializationOption::Filter(filter));
        http.end();

        if (err || (tag = apiDoc["tag_name"] | "").length() == 0) {
            fV_printSerialDebug(LOG_NETWORK, "[OTA] Erro ao parsear API GitHub");
            return;
        }

        String version = tag.startsWith("v") ? tag.substring(1) : tag;
        binUrl = "https://raw.githubusercontent.com/rede-analista/SMCR/" + tag
               + "/firmware/v" + version + "/SMCR_v" + version + "_firmware.bin";
        fV_printSerialDebug(LOG_NETWORK, "[OTA] Release: %s | URL: %s", tag.c_str(), binUrl.c_str());
    } // secClient + http destruídos aqui — memória SSL liberada antes do download

    delay(300); // Aguarda heap se estabilizar

    // ── 2. Download e gravação em streaming ───────────────────────────
    WiFiClientSecure secClient2;
    secClient2.setInsecure();

    HTTPClient http2;
    http2.setTimeout(60000);
    http2.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    if (!http2.begin(secClient2, binUrl)) {
        fV_printSerialDebug(LOG_NETWORK, "[OTA] Falha ao conectar para download do firmware");
        return;
    }

    int binCode = http2.GET();
    if (binCode != HTTP_CODE_OK) {
        http2.end();
        fV_printSerialDebug(LOG_NETWORK, "[OTA] Erro HTTP no download do firmware: %d", binCode);
        return;
    }

    int contentLen = http2.getSize();
    fV_printSerialDebug(LOG_NETWORK, "[OTA] Tamanho: %d bytes. Gravando...", contentLen);

    if (!Update.begin(contentLen > 0 ? (size_t)contentLen : UPDATE_SIZE_UNKNOWN)) {
        http2.end();
        fV_printSerialDebug(LOG_NETWORK, "[OTA] Falha ao iniciar Update (sem espaço?)");
        return;
    }

    WiFiClient* binStream = http2.getStreamPtr();
    size_t written = Update.writeStream(*binStream);
    http2.end();

    if (written != (size_t)contentLen) {
        fV_printSerialDebug(LOG_NETWORK, "[OTA] Stream incompleto: %u/%u bytes recebidos — CDN/rede falhou", written, (size_t)contentLen);
        Update.abort();
        return;
    }
    if (!Update.end(true)) {
        fV_printSerialDebug(LOG_NETWORK, "[OTA] Falha ao finalizar Update. Erro %d: %s", Update.getError(), Update.errorString());
        return;
    }

    fV_printSerialDebug(LOG_NETWORK, "[OTA] Firmware %s instalado (%d bytes). Reiniciando...", tag.c_str(), written);
    delay(500);
    ESP.restart();
}

void fV_cloudSyncTask(void) {
    if (!vB_wifiIsConnected) {
        vS_cloudSyncStatus = "Erro: Sem conexão WiFi";
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Sync abortado: sem WiFi");
        return;
    }
    if (vSt_mainConfig.vS_cloudUrl.length() == 0) {
        vS_cloudSyncStatus = "Erro: URL não configurada";
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Sync abortado: URL vazia");
        return;
    }

    String url = fS_cloudBaseUrl() + "/api/get_config.php";
    if (vSt_mainConfig.vS_cloudApiToken.length() > 0)
        url += "?token=" + vSt_mainConfig.vS_cloudApiToken;

    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Iniciando sync: %s", url.c_str());

    WiFiClientSecure secClient;
    if (vSt_mainConfig.vB_cloudUseHttps) secClient.setInsecure();

    HTTPClient http;
    http.setTimeout(5000);
    bool httpOk = vSt_mainConfig.vB_cloudUseHttps ? http.begin(secClient, url) : http.begin(url);
    if (!httpOk) {
        vS_cloudSyncStatus = "Erro: Falha ao conectar";
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Falha ao iniciar HTTP");
        return;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        vS_cloudSyncStatus = "Erro HTTP: " + String(httpCode);
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Erro HTTP: %d", httpCode);
        return;
    }

    String payload = http.getString();
    http.end();
    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Resposta: %d bytes", payload.length());

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        vS_cloudSyncStatus = "Erro: JSON inválido";
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Erro ao parsear JSON");
        return;
    }

    // ── Verifica se houve alteração ───────────────────────────────────
    uint32_t newHash = fU32_hashPayload(payload);
    if (newHash == vU32_lastSyncHash) {
        vS_cloudSyncStatus  = "OK: sem alterações";
        vS_cloudSyncLastTime = fS_getFormattedTime();
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Sync: payload idêntico ao anterior, nenhuma alteração aplicada.");
        return;
    }
    vU32_lastSyncHash = newHash;
    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Sync: alteração detectada (hash %u), aplicando configurações.", newHash);

    // ── Rede ──────────────────────────────────────────────────────────
    if (doc["hostname"].is<const char*>())
        vSt_mainConfig.vS_hostname = doc["hostname"].as<String>();
    if (doc["wifi_ssid"].is<const char*>())
        vSt_mainConfig.vS_wifiSsid = doc["wifi_ssid"].as<String>();
    if (doc["wifi_pass"].is<const char*>() && doc["wifi_pass"].as<String>().length() > 0)
        vSt_mainConfig.vS_wifiPass = doc["wifi_pass"].as<String>();
    if (!doc["wifi_attempts"].isNull())
        vSt_mainConfig.vU16_wifiConnectAttempts = doc["wifi_attempts"].as<int>();
    if (!doc["wifi_check_interval"].isNull())
        vSt_mainConfig.vU32_wifiCheckInterval = doc["wifi_check_interval"].as<unsigned long>();
    if (doc["ap_ssid"].is<const char*>())
        vSt_mainConfig.vS_apSsid = doc["ap_ssid"].as<String>();
    if (doc["ap_pass"].is<const char*>() && doc["ap_pass"].as<String>().length() > 0)
        vSt_mainConfig.vS_apPass = doc["ap_pass"].as<String>();
    if (!doc["ap_fallback_enabled"].isNull())
        vSt_mainConfig.vB_apFallbackEnabled = doc["ap_fallback_enabled"].as<bool>();

    // ── NTP ───────────────────────────────────────────────────────────
    if (doc["ntp_server1"].is<const char*>())
        vSt_mainConfig.vS_ntpServer1 = doc["ntp_server1"].as<String>();
    if (!doc["gmt_offset"].isNull())
        vSt_mainConfig.vI_gmtOffsetSec = doc["gmt_offset"].as<int>();
    if (!doc["daylight_offset"].isNull())
        vSt_mainConfig.vI_daylightOffsetSec = doc["daylight_offset"].as<int>();

    // ── Interface Web ─────────────────────────────────────────────────
    if (!doc["status_pinos_enabled"].isNull())
        vSt_mainConfig.vB_statusPinosEnabled = doc["status_pinos_enabled"].as<bool>();
    if (!doc["inter_modulos_enabled"].isNull())
        vSt_mainConfig.vB_interModulosEnabled = doc["inter_modulos_enabled"].as<bool>();
    if (doc["cor_com_alerta"].is<const char*>())
        vSt_mainConfig.vS_corStatusComAlerta = doc["cor_com_alerta"].as<String>();
    if (doc["cor_sem_alerta"].is<const char*>())
        vSt_mainConfig.vS_corStatusSemAlerta = doc["cor_sem_alerta"].as<String>();
    if (!doc["tempo_refresh"].isNull())
        vSt_mainConfig.vU16_tempoRefresh = doc["tempo_refresh"].as<int>();
    if (!doc["show_analog_history"].isNull())
        vSt_mainConfig.vB_showAnalogHistory = doc["show_analog_history"].as<bool>();
    if (!doc["show_digital_history"].isNull())
        vSt_mainConfig.vB_showDigitalHistory = doc["show_digital_history"].as<bool>();

    // ── Sistema ───────────────────────────────────────────────────────
    if (!doc["qtd_pinos"].isNull())
        vSt_mainConfig.vU8_quantidadePinos = doc["qtd_pinos"].as<int>();
    if (!doc["serial_debug_enabled"].isNull())
        vSt_mainConfig.vB_serialDebugEnabled = doc["serial_debug_enabled"].as<bool>();
    if (!doc["active_log_flags"].isNull())
        vSt_mainConfig.vU32_activeLogFlags = doc["active_log_flags"].as<unsigned long>();
    if (!doc["watchdog_enabled"].isNull())
        vSt_mainConfig.vB_watchdogEnabled = doc["watchdog_enabled"].as<bool>();
    if (!doc["tempo_watchdog_us"].isNull())
        vSt_mainConfig.vU32_tempoWatchdogUs = doc["tempo_watchdog_us"].as<unsigned long>();
    // ── Servidor Web ──────────────────────────────────────────────────
    if (!doc["web_server_port"].isNull())
        vSt_mainConfig.vU16_webServerPort = doc["web_server_port"].as<int>();
    if (!doc["auth_enabled"].isNull())
        vSt_mainConfig.vB_authEnabled = doc["auth_enabled"].as<bool>();
    if (doc["web_username"].is<const char*>())
        vSt_mainConfig.vS_webUsername = doc["web_username"].as<String>();
    if (doc["web_password"].is<const char*>() && doc["web_password"].as<String>().length() > 0)
        vSt_mainConfig.vS_webPassword = doc["web_password"].as<String>();
    if (!doc["dashboard_auth_required"].isNull())
        vSt_mainConfig.vB_dashboardAuthRequired = doc["dashboard_auth_required"].as<bool>();

    // ── MQTT ──────────────────────────────────────────────────────────
    if (!doc["mqtt_enabled"].isNull())
        vSt_mainConfig.vB_mqttEnabled = doc["mqtt_enabled"].as<bool>();
    if (doc["mqtt_server"].is<const char*>())
        vSt_mainConfig.vS_mqttServer = doc["mqtt_server"].as<String>();
    if (!doc["mqtt_port"].isNull())
        vSt_mainConfig.vU16_mqttPort = doc["mqtt_port"].as<int>();
    if (doc["mqtt_user"].is<const char*>())
        vSt_mainConfig.vS_mqttUser = doc["mqtt_user"].as<String>();
    if (doc["mqtt_password"].is<const char*>())
        vSt_mainConfig.vS_mqttPassword = doc["mqtt_password"].as<String>();
    if (doc["mqtt_topic_base"].is<const char*>())
        vSt_mainConfig.vS_mqttTopicBase = doc["mqtt_topic_base"].as<String>();
    if (!doc["mqtt_publish_interval"].isNull())
        vSt_mainConfig.vU16_mqttPublishInterval = doc["mqtt_publish_interval"].as<int>();
    if (!doc["mqtt_ha_discovery"].isNull())
        vSt_mainConfig.vB_mqttHomeAssistantDiscovery = doc["mqtt_ha_discovery"].as<bool>();
    if (!doc["mqtt_ha_batch"].isNull())
        vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize = doc["mqtt_ha_batch"].as<int>();
    if (!doc["mqtt_ha_interval_ms"].isNull())
        vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs = doc["mqtt_ha_interval_ms"].as<int>();
    if (!doc["mqtt_ha_repeat_sec"].isNull())
        vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec = doc["mqtt_ha_repeat_sec"].as<unsigned long>();

    // ── Inter-Módulos ─────────────────────────────────────────────────
    // Nomes usados no site SMCR Cloud (snake_case com prefixo intermod_):
    // intermod_enabled, intermod_healthcheck, intermod_max_failures, intermod_auto_discovery
    if (!doc["intermod_enabled"].isNull())
        vSt_mainConfig.vB_interModEnabled = doc["intermod_enabled"].as<bool>();
    if (!doc["intermod_healthcheck"].isNull())
        vSt_mainConfig.vU16_interModHealthCheckInterval = doc["intermod_healthcheck"].as<int>();
    if (!doc["intermod_max_failures"].isNull())
        vSt_mainConfig.vU8_interModMaxFailures = doc["intermod_max_failures"].as<int>();
    if (!doc["intermod_auto_discovery"].isNull())
        vSt_mainConfig.vB_interModAutoDiscovery = doc["intermod_auto_discovery"].as<bool>();

    // ── Telegram ──────────────────────────────────────────────────────
    if (!doc["telegram_enabled"].isNull())
        vSt_mainConfig.vB_telegramEnabled = doc["telegram_enabled"].as<bool>();
    if (doc["telegram_token"].is<const char*>())
        vSt_mainConfig.vS_telegramToken = doc["telegram_token"].as<String>();
    if (doc["telegram_chatid"].is<const char*>())
        vSt_mainConfig.vS_telegramChatId = doc["telegram_chatid"].as<String>();
    if (!doc["telegram_interval"].isNull())
        vSt_mainConfig.vU16_telegramCheckInterval = doc["telegram_interval"].as<int>();

    // ── SMCR Cloud ────────────────────────────────────────────────────
    // NOTA: cloud_sync_enabled, cloud_sync_interval_min e cloud_api_token
    // são configurações locais e NÃO são sobrescritas pelo sync.
    if (!doc["cloud_url"].isNull() && doc["cloud_url"].as<String>().length() > 0)
        vSt_mainConfig.vS_cloudUrl = doc["cloud_url"].as<String>();
    if (!doc["cloud_port"].isNull())
        vSt_mainConfig.vU16_cloudPort = doc["cloud_port"].as<uint16_t>();
    if (!doc["cloud_use_https"].isNull())
        vSt_mainConfig.vB_cloudUseHttps = doc["cloud_use_https"].as<bool>();
    if (!doc["cloud_heartbeat_enabled"].isNull())
        vSt_mainConfig.vB_cloudHeartbeatEnabled = doc["cloud_heartbeat_enabled"].as<bool>();
    if (!doc["cloud_heartbeat_interval_min"].isNull())
        vSt_mainConfig.vU16_cloudHeartbeatIntervalMin = doc["cloud_heartbeat_interval_min"].as<int>();

    // ── Flags de ação ─────────────────────────────────────────────────
    // Lidas antes de salvar/reiniciar para que o reboot_on_sync e ota_update_on_sync
    // atuem após aplicar toda a configuração.
    bool vB_rebootOnSync = !doc["reboot_on_sync"].isNull() && doc["reboot_on_sync"].as<bool>();
    bool vB_otaOnSync    = !doc["ota_update_on_sync"].isNull() && doc["ota_update_on_sync"].as<bool>();

    fV_salvarMainConfig();
    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Config geral aplicada e salva.");

    // ── Pinos ─────────────────────────────────────────────────────────
    JsonArray pins = doc["pins"].as<JsonArray>();
    fV_clearPinConfigs();
    uint8_t vU8_pinsAdded = 0;
    for (JsonObject p : pins) {
        PinConfig_t cfg = {};
        cfg.nome                  = p["nome"] | String("");
        cfg.pino                  = p["pino"] | 0;
        cfg.tipo                  = p["tipo"] | 0;
        cfg.modo                  = p["modo"] | 0;
        cfg.xor_logic             = p["xor_logic"] | 0;
        cfg.tempo_retencao        = p["tempo_retencao"] | 0;
        cfg.nivel_acionamento_min = p["nivel_acionamento_min"] | 0;
        cfg.nivel_acionamento_max = p["nivel_acionamento_max"] | 1;
        cfg.classe_mqtt           = p["classe_mqtt"] | String("");
        cfg.icone_mqtt            = p["icone_mqtt"] | String("");
        cfg.status_atual          = 0;
        cfg.ignore_contador       = 0;
        cfg.ultimo_acionamento_ms = 0;
        cfg.historico_index       = 0;
        cfg.historico_count       = 0;
        if (fI_addPinConfig(cfg) >= 0) vU8_pinsAdded++;
    }
    fB_savePinConfigs();
    fV_setupConfiguredPins();
    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Pinos: %d sincronizados.", vU8_pinsAdded);

    // ── Ações ─────────────────────────────────────────────────────────
    JsonArray actions = doc["actions"].as<JsonArray>();
    fV_clearActionConfigs();
    uint8_t vU8_actionsAdded = 0;
    for (JsonObject a : actions) {
        ActionConfig_t act = {};
        act.pino_origem          = a["pino_origem"] | 0;
        act.numero_acao          = a["numero_acao"] | 1;
        act.pino_destino         = a["pino_destino"] | 0;
        act.acao                 = a["acao"] | 0;
        act.tempo_on             = a["tempo_on"] | 0;
        act.tempo_off            = a["tempo_off"] | 0;
        act.pino_remoto          = a["pino_remoto"] | 0;
        act.envia_modulo         = a["envia_modulo"] | String("");
        act.telegram             = a["telegram"] | false;
        act.assistente           = a["assistente"] | false;
        act.hora_agendada        = a["hora_agendada"] | (uint8_t)255;
        act.minuto_agendado      = a["minuto_agendado"] | (uint8_t)0;
        act.duracao_agendada_s   = a["duracao_agendada_s"] | (uint16_t)0;
        act.contador_on          = 0;
        act.contador_off         = 0;
        act.estado_acao          = false;
        act.ultimo_estado_origem = false;
        if (fI_addActionConfig(act) >= 0) vU8_actionsAdded++;
    }
    fB_saveActionConfigs();
    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Acoes: %d sincronizadas.", vU8_actionsAdded);

    // ── Inter-módulos: atualiza campos de alerta dos módulos cadastrados ─
    // Apenas atualiza campos configuráveis (alerta offline / healthcheck).
    // Preserva estado de runtime (online, falhas, timestamps).
    uint8_t vU8_interModUpdated = 0;
    JsonArray intermodModules = doc["intermod_modules"].as<JsonArray>();
    if (!intermodModules.isNull()) {
        for (JsonObject m : intermodModules) {
            const char* moduleId = m["module_id"] | "";
            if (moduleId[0] == '\0') continue;
            for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
                if (vA_interModConfigs[i].id != moduleId) continue;
                if (m.containsKey("ativo"))                    vA_interModConfigs[i].ativo                    = m["ativo"].as<bool>();
                vA_interModConfigs[i].pins_offline             = m["pins_offline"]              | vA_interModConfigs[i].pins_offline;
                vA_interModConfigs[i].offline_alert_enabled    = m["offline_alert_enabled"]     | vA_interModConfigs[i].offline_alert_enabled;
                vA_interModConfigs[i].offline_flash_ms         = m["offline_flash_ms"]          | vA_interModConfigs[i].offline_flash_ms;
                vA_interModConfigs[i].pins_healthcheck         = m["pins_healthcheck"]          | vA_interModConfigs[i].pins_healthcheck;
                vA_interModConfigs[i].healthcheck_alert_enabled= m["healthcheck_alert_enabled"] | vA_interModConfigs[i].healthcheck_alert_enabled;
                vA_interModConfigs[i].healthcheck_flash_ms     = m["healthcheck_flash_ms"]      | vA_interModConfigs[i].healthcheck_flash_ms;
                vU8_interModUpdated++;
                break;
            }
        }
        if (vU8_interModUpdated > 0) {
            fB_saveInterModConfigs();
            fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Inter-modulos: %d modulos atualizados.", vU8_interModUpdated);
        }
    }

    vS_cloudSyncStatus  = "Sincronizado: " + String(vU8_pinsAdded) + " pinos, " + String(vU8_actionsAdded) + " acoes";
    vS_cloudSyncLastTime = fS_getFormattedTime();
    fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Sync completo.");

    // ── Executa flags de ação (após sync completo) ────────────────────
    if (vB_otaOnSync) {
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Flag ota_update_on_sync ativo — iniciando OTA via GitHub...");
        fV_cloudOtaFromGitHub();
        // Se chegou aqui, OTA falhou (sucesso chama ESP.restart() internamente)
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] OTA falhou, continuando operacao normal.");
        return;
    }
    if (vB_rebootOnSync) {
        fV_printSerialDebug(LOG_NETWORK, "[CLOUD] Flag reboot_on_sync ativo — reiniciando...");
        delay(200);
        ESP.restart();
    }
}

/**
 * @brief Baixa todos os arquivos HTML/assets da SMCR Cloud para o LittleFS
 * Chamada a partir do loop() quando vB_pendingFetchCloudFiles == true
 */
void fV_fetchCloudFilesTask(void) {
    vB_fetchDone  = false;
    vB_fetchError = false;

    if (!vB_wifiIsConnected) {
        vS_fetchStatus = "Erro: sem conexao WiFi";
        vB_fetchDone = vB_fetchError = true;
        return;
    }

    bool fetchHttps = vSt_mainConfig.vB_cloudUseHttps;
    String fetchSchema = fetchHttps ? "https://" : "http://";
    String listUrl = fetchSchema + vS_fetchCloudFilesUrl + ":" + String(vSt_mainConfig.vU16_cloudPort) + "/api/get_web_files.php";
    fV_printSerialDebug(LOG_NETWORK, "[FETCH] Buscando lista: %s", listUrl.c_str());

    WiFiClientSecure fetchSecClient;
    if (fetchHttps) fetchSecClient.setInsecure();

    HTTPClient http;
    http.setTimeout(5000);
    bool fetchOk = fetchHttps ? http.begin(fetchSecClient, listUrl) : http.begin(listUrl);
    if (!fetchOk) {
        vS_fetchStatus = "Erro: falha ao conectar ao servidor";
        vB_fetchDone = vB_fetchError = true;
        fV_printSerialDebug(LOG_NETWORK, "[FETCH] Falha ao iniciar HTTP");
        return;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        vS_fetchStatus = "Erro HTTP na lista: " + String(code);
        vB_fetchDone = vB_fetchError = true;
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        vS_fetchStatus = "Erro: resposta invalida do servidor";
        vB_fetchDone = vB_fetchError = true;
        return;
    }

    JsonArray files = doc["files"].as<JsonArray>();
    int total = files.size();
    if (total == 0) {
        vS_fetchStatus = "Nenhum arquivo encontrado no servidor";
        vB_fetchDone = vB_fetchError = true;
        return;
    }

    fV_printSerialDebug(LOG_NETWORK, "[FETCH] %d arquivos encontrados", total);

    int downloaded = 0;
    int errors = 0;

    for (JsonVariant fv : files) {
        String filename = fv.as<String>();
        String fileUrl  = fetchSchema + vS_fetchCloudFilesUrl + ":" + String(vSt_mainConfig.vU16_cloudPort) + "/api/get_web_files.php?file=" + filename;
        vS_fetchStatus  = "Baixando " + filename + " (" + String(downloaded + errors + 1) + "/" + String(total) + ")";
        fV_printSerialDebug(LOG_NETWORK, "[FETCH] %s", vS_fetchStatus.c_str());

        WiFiClientSecure fileSecClient;
        if (fetchHttps) fileSecClient.setInsecure();
        HTTPClient fhttp;
        fhttp.setTimeout(15000);
        bool fileOk = fetchHttps ? fhttp.begin(fileSecClient, fileUrl) : fhttp.begin(fileUrl);
        if (!fileOk) { errors++; continue; }

        int fc = fhttp.GET();
        if (fc == HTTP_CODE_OK) {
            // Streaming para não saturar a heap com arquivos grandes
            WiFiClient* stream = fhttp.getStreamPtr();
            File f = LittleFS.open("/" + filename, "w");
            if (f) {
                uint8_t buf[512];
                int len;
                while (fhttp.connected() && (len = stream->readBytes(buf, sizeof(buf))) > 0) {
                    f.write(buf, len);
                }
                f.close();
                downloaded++;
                fV_printSerialDebug(LOG_NETWORK, "[FETCH] OK: %s", filename.c_str());
            } else {
                errors++;
                fV_printSerialDebug(LOG_NETWORK, "[FETCH] Erro ao abrir LittleFS para: %s", filename.c_str());
            }
        } else {
            errors++;
            fV_printSerialDebug(LOG_NETWORK, "[FETCH] Erro HTTP %d: %s", fc, filename.c_str());
        }
        fhttp.end();
    }

    if (errors == 0) {
        vS_fetchStatus = "Concluido: " + String(downloaded) + " arquivos baixados com sucesso!";
        vB_fetchError  = false;
    } else {
        vS_fetchStatus = "Concluido: " + String(downloaded) + " OK, " + String(errors) + " erros";
        vB_fetchError  = (downloaded == 0);
    }
    vB_fetchDone = true;
    fV_printSerialDebug(LOG_NETWORK, "[FETCH] %s", vS_fetchStatus.c_str());
}

/**
 * @brief Registra o dispositivo na cloud usando register_token e salva o api_token retornado.
 * Executa apenas quando cloudApiToken está vazio e cloudRegisterToken está configurado.
 */
void fV_cloudAutoRegisterTask(void) {
    if (!vB_wifiIsConnected) return;
    if (vSt_mainConfig.vS_cloudUrl.length() == 0) return;
    if (vSt_mainConfig.vS_cloudRegisterToken.length() == 0) return;
    if (vSt_mainConfig.vS_cloudApiToken.length() > 0) return; // já registrado

    String url = fS_cloudBaseUrl() + "/api/register.php";
    String uniqueId = fS_getMqttUniqueId();

    JsonDocument doc;
    doc["unique_id"]        = uniqueId;
    doc["register_token"]   = vSt_mainConfig.vS_cloudRegisterToken;
    doc["hostname"]         = vSt_mainConfig.vS_hostname;
    doc["ip"]               = WiFi.localIP().toString();
    doc["port"]             = vSt_mainConfig.vU16_webServerPort;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["wifi_ssid"]        = vSt_mainConfig.vS_wifiSsid;
    doc["wifi_attempts"]    = vSt_mainConfig.vU16_wifiConnectAttempts;
    doc["qtd_pinos"]        = vSt_mainConfig.vU8_quantidadePinos;

    String body;
    serializeJson(doc, body);

    WiFiClientSecure regSecClient;
    if (vSt_mainConfig.vB_cloudUseHttps) regSecClient.setInsecure();

    HTTPClient http;
    http.setTimeout(8000);
    bool ok = vSt_mainConfig.vB_cloudUseHttps ? http.begin(regSecClient, url) : http.begin(url);
    if (!ok) {
        fV_printSerialDebug(LOG_NETWORK, "[REG] Falha ao iniciar HTTP: %s", url.c_str());
        return;
    }
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(body);
    String resp = http.getString();
    http.end();

    if (code != 200) {
        fV_printSerialDebug(LOG_NETWORK, "[REG] Erro HTTP %d: %s", code, resp.c_str());
        return;
    }

    JsonDocument respDoc;
    if (deserializeJson(respDoc, resp) != DeserializationError::Ok) {
        fV_printSerialDebug(LOG_NETWORK, "[REG] Resposta inválida");
        return;
    }
    if (!respDoc["ok"].as<bool>()) {
        fV_printSerialDebug(LOG_NETWORK, "[REG] Registro rejeitado: %s", respDoc["error"].as<const char*>());
        return;
    }

    String token = respDoc["api_token"].as<String>();
    if (token.length() == 0) {
        fV_printSerialDebug(LOG_NETWORK, "[REG] api_token vazio na resposta");
        return;
    }

    vSt_mainConfig.vS_cloudApiToken = token;
    fV_salvarMainConfig();
    fV_printSerialDebug(LOG_NETWORK, "[REG] Dispositivo registrado com sucesso. api_token salvo.");

    // Se LittleFS não tem o dashboard, dispara download automático dos HTMLs
    if (!LittleFS.exists("/web_dashboard.html")) {
        vS_fetchCloudFilesUrl = vSt_mainConfig.vS_cloudUrl;
        vB_pendingFetchCloudFiles = true;
        fV_printSerialDebug(LOG_NETWORK, "[REG] LittleFS sem HTMLs — agendando fetch automatico de %s", vS_fetchCloudFilesUrl.c_str());
    }
}

/**
 * @brief Envia heartbeat de status para o SMCR Cloud HA (POST /api/status.php).
 * Mantém o dispositivo marcado como online no painel da cloud.
 * Requer que vS_cloudApiToken esteja configurado (recebido no auto-registro).
 */
void fV_cloudHeartbeatTask(void) {
    if (!vB_wifiIsConnected) {
        vS_heartbeatStatus = "Erro: sem WiFi";
        return;
    }
    if (vSt_mainConfig.vS_cloudApiToken.length() == 0) {
        vS_heartbeatStatus = "Erro: token nao configurado";
        fV_printSerialDebug(LOG_NETWORK, "[HB] Heartbeat abortado: api_token vazio");
        return;
    }

    String url = fS_cloudBaseUrl() + "/api/status.php";

    // Monta payload JSON com informações do dispositivo
    JsonDocument doc;
    doc["ip"]               = WiFi.localIP().toString();
    doc["hostname"]         = vSt_mainConfig.vS_hostname;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["free_heap"]        = ESP.getFreeHeap();
    doc["uptime_ms"]        = millis();
    doc["sketch_size"]      = ESP.getSketchSize();
    doc["sketch_free"]      = ESP.getFreeSketchSpace();
    doc["wifi_rssi"]        = WiFi.RSSI();

    // Inclui histórico de acionamentos para persistência no servidor
    {
        JsonDocument histDoc;
        if (deserializeJson(histDoc, fS_getActionHistoryJson()) == DeserializationError::Ok) {
            doc["action_history"].set(histDoc.as<JsonArray>());
        }
    }

    String body;
    serializeJson(doc, body);

    WiFiClientSecure hbSecClient;
    if (vSt_mainConfig.vB_cloudUseHttps) hbSecClient.setInsecure();

    HTTPClient http;
    http.setTimeout(5000);
    bool hbOk = vSt_mainConfig.vB_cloudUseHttps ? http.begin(hbSecClient, url) : http.begin(url);
    if (!hbOk) {
        vS_heartbeatStatus = "Erro: falha ao conectar";
        fV_printSerialDebug(LOG_NETWORK, "[HB] Falha ao iniciar HTTP: %s", url.c_str());
        return;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + vSt_mainConfig.vS_cloudApiToken);

    int httpCode = http.POST(body);
    String response = http.getString();
    http.end();

    vS_heartbeatLastTime = fS_getFormattedTime();
    if (httpCode == 200) {
        vS_heartbeatStatus = "OK";
        fV_printSerialDebug(LOG_NETWORK, "[HB] Heartbeat enviado com sucesso");
    } else {
        vS_heartbeatStatus = "Erro HTTP: " + String(httpCode);
        fV_printSerialDebug(LOG_NETWORK, "[HB] Erro heartbeat: HTTP %d", httpCode);
    }
}
