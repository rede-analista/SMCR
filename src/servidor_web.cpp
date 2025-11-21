// Conteúdo de servidor_web.cpp
#include "globals.h"
#include "web_modoap.h"
#include "web_dashboard.h"
#include "web_config_gerais.h"
#include "web_reset.h"

// Função de inicialização do servidor web
void fV_setupWebServer() {
    fV_printSerialDebug(LOG_WEB, "Configurando o servidor web...");

    // Criação do servidor web na porta 80 (padrão HTTP)
    if (SERVIDOR_WEB_ASYNC != nullptr) {
        delete SERVIDOR_WEB_ASYNC;
    }
    SERVIDOR_WEB_ASYNC = new AsyncWebServer(8080);

    // Rota da página principal (Dashboard ou Configuração Inicial)
    SERVIDOR_WEB_ASYNC->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
         // Se nao estiver conectado, envia a pagina de configuracao inicial (AP)
         if (WiFi.status() != WL_CONNECTED) {
             fV_printSerialDebug(LOG_WEB, "Servindo pagina de CONFIGURACAO INICIAL...");
             request->send(200, "text/html", web_config_html);
         } else {
             // NOVO: Se estiver conectado, serve o Dashboard de Status (STA)
             fV_printSerialDebug(LOG_WEB, "Servindo DASHBOARD de Status...");
             request->send(200, "text/html", web_dashboard_html);
         }
     });

    // CORREÇÃO: Rota para salvar a configuracao inicial usa HTTP_POST e chama o handler
    SERVIDOR_WEB_ASYNC->on("/save_config", HTTP_POST, fV_handleSaveConfig);

    // Rota para APLICAR configurações (running-config) - sem salvar na flash
    SERVIDOR_WEB_ASYNC->on("/apply_config", HTTP_POST, fV_handleApplyConfig);

    // Rota para SALVAR configurações (startup-config) - salva na flash sem reiniciar
    SERVIDOR_WEB_ASYNC->on("/save_to_flash", HTTP_POST, fV_handleSaveToFlash);

    // Rota para página de configurações avançadas
    SERVIDOR_WEB_ASYNC->on("/configuracao", HTTP_GET, fV_handleConfigPage);
    
    // Rota para página de reset
    SERVIDOR_WEB_ASYNC->on("/reset", HTTP_GET, fV_handleResetPage);
    
    // Rotas de reset (POST)
    SERVIDOR_WEB_ASYNC->on("/reset/soft", HTTP_POST, fV_handleSoftReset);
    SERVIDOR_WEB_ASYNC->on("/reset/factory", HTTP_POST, fV_handleFactoryReset);
    SERVIDOR_WEB_ASYNC->on("/reset/network", HTTP_POST, fV_handleNetworkReset);
    
    // Rota para obter configurações atuais em JSON
    SERVIDOR_WEB_ASYNC->on("/config/json", HTTP_GET, [](AsyncWebServerRequest *request) {
        fV_printSerialDebug(LOG_WEB, "[WEB] Solicitação para /config/json");
        
        JsonDocument doc;
        
        // Configurações de rede
        doc["hostname"] = vSt_mainConfig.vS_hostname;
        doc["wifi_ssid"] = vSt_mainConfig.vS_wifiSsid;
        doc["wifi_pass"] = vSt_mainConfig.vS_wifiPass;
        doc["wifi_attempts"] = vSt_mainConfig.vU16_wifiConnectAttempts;
        
        // Configurações de NTP
        doc["ntp_server1"] = vSt_mainConfig.vS_ntpServer1;
        doc["gmt_offset"] = vSt_mainConfig.vI_gmtOffsetSec;
        doc["daylight_offset"] = vSt_mainConfig.vI_daylightOffsetSec;
        
        // Configurações da interface
        doc["status_pinos"] = vSt_mainConfig.vB_statusPinosEnabled;
        doc["inter_modulos"] = vSt_mainConfig.vB_interModulosEnabled;
        doc["cor_com_alerta"] = vSt_mainConfig.vS_corStatusComAlerta;
        doc["cor_sem_alerta"] = vSt_mainConfig.vS_corStatusSemAlerta;
        doc["tempo_refresh"] = vSt_mainConfig.vU16_tempoRefresh;
        
        // Configurações do watchdog
        doc["watchdog_enabled"] = vSt_mainConfig.vB_watchdogEnabled;
        doc["clock_esp32"] = vSt_mainConfig.vU16_clockEsp32Mhz;
        doc["tempo_watchdog"] = vSt_mainConfig.vU32_tempoWatchdogUs;
        
        // Configurações dos pinos
        doc["qtd_pinos"] = vSt_mainConfig.vU8_quantidadePinos;
        
        String response;
        serializeJson(doc, response);
        
        fV_printSerialDebug(LOG_WEB, "[WEB] Enviando JSON: %s", response.c_str());
        request->send(200, "application/json", response);
    });

    //Rota para retornar dados de status em JSON (para o Dashboard)
    SERVIDOR_WEB_ASYNC->on("/status/json", HTTP_GET, fV_handleStatusJson);

    // Rota de nao encontrado
    SERVIDOR_WEB_ASYNC->onNotFound(fV_handleNotFound);
    
    // Inicia o servidor
    SERVIDOR_WEB_ASYNC->begin();
    fV_printSerialDebug(LOG_WEB, "Servidor web assincrono iniciado na porta 8080");
}

// Rota Handler: Funcao para salvar configurações (inicial e avançadas)
void fV_handleSaveConfig(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "Recebendo configurações via POST...");
    
    bool isInitialConfig = request->hasArg("ssid") && request->hasArg("password");
    bool isAdvancedConfig = request->hasArg("ntp_server1") || request->hasArg("qtd_pinos");
    
    if (isInitialConfig) {
        // Configuração inicial (modo AP)
        if (request->hasArg("hostname")) vSt_mainConfig.vS_hostname = request->arg("hostname");
        if (request->hasArg("ssid")) vSt_mainConfig.vS_wifiSsid = request->arg("ssid");
        if (request->hasArg("password")) vSt_mainConfig.vS_wifiPass = request->arg("password");
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes iniciais recebidas via POST.");
        
    } else if (isAdvancedConfig) {
        // Configurações avançadas
        if (request->hasArg("hostname")) vSt_mainConfig.vS_hostname = request->arg("hostname");
        if (request->hasArg("wifi_ssid")) vSt_mainConfig.vS_wifiSsid = request->arg("wifi_ssid");
        if (request->hasArg("wifi_pass") && request->arg("wifi_pass").length() > 0) {
            vSt_mainConfig.vS_wifiPass = request->arg("wifi_pass");
        }
        if (request->hasArg("wifi_attempts")) {
            vSt_mainConfig.vU16_wifiConnectAttempts = request->arg("wifi_attempts").toInt();
        }
        
        // NTP
        if (request->hasArg("ntp_server1")) vSt_mainConfig.vS_ntpServer1 = request->arg("ntp_server1");
        if (request->hasArg("gmt_offset")) vSt_mainConfig.vI_gmtOffsetSec = request->arg("gmt_offset").toInt();
        if (request->hasArg("daylight_offset")) vSt_mainConfig.vI_daylightOffsetSec = request->arg("daylight_offset").toInt();
        
        // Interface
        vSt_mainConfig.vB_statusPinosEnabled = request->hasArg("status_pinos");
        vSt_mainConfig.vB_interModulosEnabled = request->hasArg("inter_modulos");
        if (request->hasArg("cor_com_alerta")) vSt_mainConfig.vS_corStatusComAlerta = request->arg("cor_com_alerta");
        if (request->hasArg("cor_sem_alerta")) vSt_mainConfig.vS_corStatusSemAlerta = request->arg("cor_sem_alerta");
        if (request->hasArg("tempo_refresh")) vSt_mainConfig.vU16_tempoRefresh = request->arg("tempo_refresh").toInt();
        
        // Watchdog
        vSt_mainConfig.vB_watchdogEnabled = request->hasArg("watchdog_enabled");
        if (request->hasArg("clock_esp32")) vSt_mainConfig.vU16_clockEsp32Mhz = request->arg("clock_esp32").toInt();
        if (request->hasArg("tempo_watchdog")) vSt_mainConfig.vU32_tempoWatchdogUs = request->arg("tempo_watchdog").toInt();
        
        // Pinos
        if (request->hasArg("qtd_pinos")) vSt_mainConfig.vU8_quantidadePinos = request->arg("qtd_pinos").toInt();
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes avancadas recebidas via POST.");
        
    } else {
        fV_printSerialDebug(LOG_WEB, "Erro: Parametros do formulario ausentes.");
        request->send(400, "text/plain", "Erro: Parametros ausentes");
        return;
    }
        
    // Salva na memoria flash
    fV_salvarMainConfig();
    
    fV_printSerialDebug(LOG_WEB, "Configuracoes salvas na flash. Acionando reboot...");

    // Envia resposta 200 (OK) para o JavaScript
    request->send(200, "text/plain", "OK");

    // Força o esvaziamento do buffer da Serial e reinicia
    Serial.flush(); 
    delay(300);
    ESP.restart();
}

// Rota Handler: 404 Not Found
void fV_handleNotFound(AsyncWebServerRequest *request) {
    // Captura o IP do cliente que fez a requisição
    String vS_clientIP = (request->client() != nullptr) ? request->client()->remoteIP().toString() : "N/A";

    fV_printSerialDebug(LOG_WEB, "404 - URL nao encontrada. URI: %s (Source IP: %s)", 
        request->url().c_str(), vS_clientIP.c_str());
        
    // Envia uma resposta simples 404
    request->send(404, "text/plain", "404 Not Found. A URL solicitada nao existe neste dispositivo.");
}

// Rota Handler: Funcao para salvar a configuracao inicial
void fV_handleStatusJson(AsyncWebServerRequest *request) {
    // Usamos DynamicJsonDocument com um tamanho estimado (aprox. 512 bytes deve ser suficiente para os dados de status).
    DynamicJsonDocument vL_jsonDoc(512); 

    // Variáveis auxiliares para o status da sincronização
    struct tm vSt_timeinfo;
    bool vL_ntp_ok = getLocalTime(&vSt_timeinfo, 0); // Checa se o NTP sincronizou sem bloquear

    // 1. Dados de Rede (Agrupados em "wifi")
    JsonObject wifi_obj = vL_jsonDoc["wifi"].to<JsonObject>(); 
    wifi_obj["status"] = vB_wifiIsConnected ? "Conectado" : "Desconectado";
    wifi_obj["ssid"] = WiFi.SSID();
    wifi_obj["local_ip"] = WiFi.localIP().toString();
    
    // 2. Dados de Sistema (Agrupados em "system")
    JsonObject system_obj = vL_jsonDoc["system"].to<JsonObject>();
    
    // CORREÇÃO FINAL: Captura o IP do cliente e coloca na estrutura 'system'
    AsyncClient *client = request->client();
    system_obj["client_ip"] = client ? client->remoteIP().toString() : "N/A";
    
    // Uso da variável global correta
    system_obj["hostname"] = vSt_mainConfig.vS_hostname; 
    
    // Utiliza a funcao fS_formatUptime implementada em rede.cpp
    system_obj["uptime"] = fS_formatUptime(millis()); 
    system_obj["current_time"] = fS_getFormattedTime();
    system_obj["timestamp"] = millis(); // Timestamp da leitura    
    
    // Retorna o heap livre em bytes. O JS fara a conversao para KB/MB.
    system_obj["free_heap"] = ESP.getFreeHeap(); 

    // 3. Sincronizacao (Agrupados em "sync")
    JsonObject sync_obj = vL_jsonDoc["sync"].to<JsonObject>();
    sync_obj["ntp_status"] = vL_ntp_ok ? "Sincronizado" : "Desabilitado";
    sync_obj["mqtt_status"] = "Desabilitado"; 
    
    // Usa o tempo formatado se o NTP estiver OK, caso contrario, Uptime formatado
    sync_obj["last_update"] = vL_ntp_ok ? fS_getFormattedTime() : fS_formatUptime(millis()); 


    // 4. Serializa o JSON para uma String
    String vS_response;
    vS_response.reserve(512); 
    serializeJson(vL_jsonDoc, vS_response);

    // 5. Envia a resposta HTTP
    request->send(200, "application/json", vS_response);

    fV_printSerialDebug(LOG_WEB, "JSON de status servido.");
}

// Rota Handler: Página de configurações gerais
void fV_handleConfigPage(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "Servindo página de configurações gerais...");
    request->send(200, "text/html", web_config_gerais_html);
}

// Handler para APLICAR configurações (running-config apenas) - Estilo Cisco
void fV_handleApplyConfig(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RUNNING-CONFIG] Aplicando configurações em memória...");
    
    // Atualiza apenas a running-config (vSt_mainConfig) sem salvar na flash
    if (request->hasArg("hostname")) {
        vSt_mainConfig.vS_hostname = request->arg("hostname");
    }
    if (request->hasArg("wifi_ssid")) {
        vSt_mainConfig.vS_wifiSsid = request->arg("wifi_ssid");
    }
    if (request->hasArg("wifi_pass")) {
        vSt_mainConfig.vS_wifiPass = request->arg("wifi_pass");
    }
    if (request->hasArg("wifi_attempts")) {
        vSt_mainConfig.vU16_wifiConnectAttempts = request->arg("wifi_attempts").toInt();
    }
    if (request->hasArg("ntp_server1")) {
        vSt_mainConfig.vS_ntpServer1 = request->arg("ntp_server1");
    }
    if (request->hasArg("gmt_offset")) {
        vSt_mainConfig.vI_gmtOffsetSec = request->arg("gmt_offset").toInt();
    }
    if (request->hasArg("daylight_offset")) {
        vSt_mainConfig.vI_daylightOffsetSec = request->arg("daylight_offset").toInt();
    }
    
    // Configurações da interface
    vSt_mainConfig.vB_statusPinosEnabled = request->hasArg("status_pinos");
    vSt_mainConfig.vB_interModulosEnabled = request->hasArg("inter_modulos");
    
    if (request->hasArg("cor_com_alerta")) {
        vSt_mainConfig.vS_corStatusComAlerta = request->arg("cor_com_alerta");
    }
    if (request->hasArg("cor_sem_alerta")) {
        vSt_mainConfig.vS_corStatusSemAlerta = request->arg("cor_sem_alerta");
    }
    if (request->hasArg("tempo_refresh")) {
        vSt_mainConfig.vU16_tempoRefresh = request->arg("tempo_refresh").toInt();
    }
    
    // Configurações do watchdog
    vSt_mainConfig.vB_watchdogEnabled = request->hasArg("watchdog_enabled");
    
    if (request->hasArg("clock_esp32")) {
        vSt_mainConfig.vU16_clockEsp32Mhz = request->arg("clock_esp32").toInt();
    }
    if (request->hasArg("tempo_watchdog")) {
        vSt_mainConfig.vU32_tempoWatchdogUs = request->arg("tempo_watchdog").toInt();
    }
    if (request->hasArg("qtd_pinos")) {
        vSt_mainConfig.vU8_quantidadePinos = request->arg("qtd_pinos").toInt();
    }

    fV_printSerialDebug(LOG_WEB, "[RUNNING-CONFIG] Configurações aplicadas em memória. Teste e depois salve se estiver OK.");
    request->send(200, "text/plain", "RUNNING-CONFIG aplicada com sucesso! Teste as configurações e depois salve.");
}

// Handler para SALVAR running-config na startup-config (flash) - Estilo Cisco 
void fV_handleSaveToFlash(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[STARTUP-CONFIG] Salvando running-config na flash...");
    
    // Salva a running-config atual (vSt_mainConfig) na startup-config (flash/NVS)
    fV_salvarMainConfig();
    
    fV_printSerialDebug(LOG_WEB, "[STARTUP-CONFIG] Configurações salvas na flash com sucesso!");
    request->send(200, "text/plain", "STARTUP-CONFIG salva! As configurações serão mantidas após reinício.");
}

// Handler para página de reset
void fV_handleResetPage(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Servindo página de reset...");
    request->send(200, "text/html", web_reset_html);
}

// Handler para reinicialização simples (soft reset)
void fV_handleSoftReset(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Executando reinicialização simples...");
    
    request->send(200, "text/plain", "Sistema reinicializando...");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Reinicia o ESP32
    ESP.restart();
}

// Handler para reset de fábrica (factory reset)
void fV_handleFactoryReset(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Executando reset de fábrica...");
    
    request->send(200, "text/plain", "Reset de fábrica executado. Reconectando ao AP...");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Limpa todas as configurações da flash
    fV_clearPreferences();
    
    // Delay adicional para garantir que a limpeza seja concluída
    delay(500);
    
    // Reinicia o ESP32
    ESP.restart();
}

// Handler para reset de rede apenas
void fV_handleNetworkReset(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Executando reset de rede...");
    
    request->send(200, "text/plain", "Reset de rede executado. Reconectando ao AP...");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Limpa apenas as configurações de WiFi
    preferences.begin("smcrconf", false);
    preferences.remove("hostname");
    preferences.remove("wifi_ssid");
    preferences.remove("wifi_pass");
    preferences.remove("wifi_attempts");
    preferences.end();
    
    // Delay adicional
    delay(500);
    
    // Reinicia o ESP32
    ESP.restart();
}



