// Conteúdo de servidor_web.cpp
#include "globals.h"
#include "web_modoap.h"
#include "web_dashboard.h"
#include "web_config_gerais.h"
#include "web_reset.h"
#include "web_pins.h"
#include "web_mqtt.h"
#include "web_files.h"
#include <LittleFS.h>
#include <Preferences.h>

// Forward declarations para handlers de arquivos
void fV_handleFilesPage(AsyncWebServerRequest *request);
void fV_handleNVSList(AsyncWebServerRequest *request);
void fV_handleFilesList(AsyncWebServerRequest *request);
void fV_handleFileDownload(AsyncWebServerRequest *request);
void fV_handleFileDelete(AsyncWebServerRequest *request);
void fV_handleFormatFlash(AsyncWebServerRequest *request);

// Função de inicialização do servidor web
void fV_setupWebServer() {
    fV_printSerialDebug(LOG_WEB, "Configurando o servidor web...");

    // Criação do servidor web na porta configurada
    if (SERVIDOR_WEB_ASYNC != nullptr) {
        delete SERVIDOR_WEB_ASYNC;
    }
    SERVIDOR_WEB_ASYNC = new AsyncWebServer(vSt_mainConfig.vU16_webServerPort);

    // Rota da página principal (Dashboard ou Configuração Inicial)
    SERVIDOR_WEB_ASYNC->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
         // Se nao estiver conectado, envia a pagina de configuracao inicial (AP)
         if (WiFi.status() != WL_CONNECTED) {
             fV_printSerialDebug(LOG_WEB, "Servindo pagina de CONFIGURACAO INICIAL...");
             request->send(200, "text/html", web_config_html);
         } else {
             // Verifica se o dashboard requer autenticação
             if (vSt_mainConfig.vB_authEnabled && vSt_mainConfig.vB_dashboardAuthRequired) {
                 if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                     return request->requestAuthentication();
                 }
             }
             // NOVO: Se estiver conectado, serve o Dashboard de Status (STA)
             // Inicia monitoramento de performance
             unsigned long startTime = millis();
             String timeStr = fS_getFormattedTime();
             fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento / (dashboard) - %s (millis: %lu)", timeStr.c_str(), startTime);
             
             // Envia dashboard com headers de performance
             AsyncWebServerResponse *response = request->beginResponse(200, "text/html", web_dashboard_html);
             response->addHeader("X-Load-Start", String(startTime));
             response->addHeader("X-Load-Time", timeStr);
             request->send(response);
             
             // Log de finalização
             unsigned long endTime = millis();
             String endTimeStr = fS_getFormattedTime();
             fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento / (dashboard) - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
         }
     });

    // CORREÇÃO: Rota para salvar a configuracao inicial usa HTTP_POST e chama o handler
    SERVIDOR_WEB_ASYNC->on("/save_config", HTTP_POST, fV_handleSaveConfig);

    // Rota para APLICAR configurações (running-config) - sem salvar na flash
    SERVIDOR_WEB_ASYNC->on("/apply_config", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleApplyConfig(request);
    });

    // Rota para SALVAR configurações (startup-config) - salva na flash sem reiniciar
    SERVIDOR_WEB_ASYNC->on("/save_to_flash", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleSaveToFlash(request);
    });

    // Rota para página de configurações avançadas
    SERVIDOR_WEB_ASYNC->on("/configuracao", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleConfigPage(request);
    });
    
    // Rota para página de configuração de pinos
    SERVIDOR_WEB_ASYNC->on("/pins", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        // Inicia monitoramento de performance
        unsigned long startTime = millis();
        String timeStr = fS_getFormattedTime();
        fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /pins - %s (millis: %lu)", timeStr.c_str(), startTime);
        
        // Envia página com callback de performance
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", web_pins_html);
        response->addHeader("X-Load-Start", String(startTime));
        response->addHeader("X-Load-Time", timeStr);
        request->send(response);
        
        // Log de finalização
        unsigned long endTime = millis();
        String endTimeStr = fS_getFormattedTime();
        fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /pins - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
    });
    
    // API: Listar todos os pinos configurados
    SERVIDOR_WEB_ASYNC->on("/api/pins", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinsListApi(request);
    });
    
    // API: Criar novo pino
    SERVIDOR_WEB_ASYNC->on("/api/pins", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinCreateApi(request);
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Captura o body da requisição
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        for (size_t i = 0; i < len; i++) {
            *body += (char)data[i];
        }
    });
    
    // API: Atualizar pino existente
    SERVIDOR_WEB_ASYNC->on("/api/pins/*", HTTP_PUT, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinUpdateApi(request);
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Captura o body da requisição
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        for (size_t i = 0; i < len; i++) {
            *body += (char)data[i];
        }
    });
    
    // API: Deletar pino
    SERVIDOR_WEB_ASYNC->on("/api/pins/*", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinDeleteApi(request);
    });
    
    // API: Salvar configurações de pinos na flash (startup config)
    SERVIDOR_WEB_ASYNC->on("/api/pins/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinsSaveApi(request);
    });
    
    // API: Carregar configurações de pinos da flash (reload startup config)
    SERVIDOR_WEB_ASYNC->on("/api/pins/reload", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinsReloadApi(request);
    });
    
    // API: Limpar todas as configurações de pinos (running config apenas)
    SERVIDOR_WEB_ASYNC->on("/api/pins/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinsClearApi(request);
    });

    // Endpoint: Adicionar novo pino (POST /pins/add) 
    SERVIDOR_WEB_ASYNC->on("/pins/add", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePinAdd(request);
    });
    
    // Rota para página de MQTT/Serviços
    SERVIDOR_WEB_ASYNC->on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleMqttPage(request);
    });
    
    // Rota para página de reset
    SERVIDOR_WEB_ASYNC->on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleResetPage(request);
    });
    
    // Rota para página de arquivos
    SERVIDOR_WEB_ASYNC->on("/arquivos", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleFilesPage(request);
    });
    
    // Rotas de reset (POST)
    SERVIDOR_WEB_ASYNC->on("/reset/soft", HTTP_POST, fV_handleSoftReset);
    SERVIDOR_WEB_ASYNC->on("/reset/factory", HTTP_POST, fV_handleFactoryReset);
    SERVIDOR_WEB_ASYNC->on("/reset/network", HTTP_POST, fV_handleNetworkReset);
    SERVIDOR_WEB_ASYNC->on("/reset/config", HTTP_POST, fV_handleConfigReset);
    SERVIDOR_WEB_ASYNC->on("/reset/pins", HTTP_POST, fV_handlePinsReset);
    SERVIDOR_WEB_ASYNC->on("/restart", HTTP_POST, fV_handleRestart);
    
    // API: Listar preferências NVS
    SERVIDOR_WEB_ASYNC->on("/api/nvs/list", HTTP_GET, fV_handleNVSList);
    
    // API: Listar arquivos LittleFS
    SERVIDOR_WEB_ASYNC->on("/api/files/list", HTTP_GET, fV_handleFilesList);
    
    // API: Download de arquivo
    SERVIDOR_WEB_ASYNC->on("/api/files/download", HTTP_GET, fV_handleFileDownload);
    
    // API: Deletar arquivo
    SERVIDOR_WEB_ASYNC->on("/api/files/delete", HTTP_POST, fV_handleFileDelete);
    
    // API: Formatar flash LittleFS
    SERVIDOR_WEB_ASYNC->on("/api/files/format", HTTP_POST, fV_handleFormatFlash);
    
    // API: Upload de arquivo
    SERVIDOR_WEB_ASYNC->on("/api/files/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"success\": true, \"message\": \"Upload concluido\"}");
        },
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
            static File uploadFile;
            
            if (index == 0) {
                String filepath = "/" + filename;
                fV_printSerialDebug(LOG_WEB, "[API] Iniciando upload: %s", filepath.c_str());
                uploadFile = LittleFS.open(filepath, "w");
                if (!uploadFile) {
                    fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[API] Erro ao criar arquivo: %s", filepath.c_str());
                    return;
                }
            }
            
            if (uploadFile) {
                uploadFile.write(data, len);
            }
            
            if (final) {
                if (uploadFile) {
                    uploadFile.close();
                    fV_printSerialDebug(LOG_WEB, "[API] Upload concluído: %s (%d bytes)", filename.c_str(), index + len);
                }
            }
        }
    );
    
    // Rota para obter configurações atuais em JSON
    SERVIDOR_WEB_ASYNC->on("/config/json", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
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
        doc["status_pinos_enabled"] = vSt_mainConfig.vB_statusPinosEnabled;
        doc["inter_modulos_enabled"] = vSt_mainConfig.vB_interModulosEnabled;
        doc["cor_com_alerta"] = vSt_mainConfig.vS_corStatusComAlerta;
        doc["cor_sem_alerta"] = vSt_mainConfig.vS_corStatusSemAlerta;
        doc["tempo_refresh"] = vSt_mainConfig.vU16_tempoRefresh;
        
        // Configurações do watchdog
        doc["watchdog_enabled"] = vSt_mainConfig.vB_watchdogEnabled;
        doc["clock_esp32_mhz"] = vSt_mainConfig.vU16_clockEsp32Mhz;
        doc["tempo_watchdog_us"] = vSt_mainConfig.vU32_tempoWatchdogUs;
        
        // Configurações dos pinos
        doc["qtd_pinos"] = vSt_mainConfig.vU8_quantidadePinos;
        
        // Configurações de debug/log
        doc["serial_debug_enabled"] = vSt_mainConfig.vB_serialDebugEnabled;
        doc["active_log_flags"] = vSt_mainConfig.vU32_activeLogFlags;
        
        // Configurações do servidor web
        doc["web_server_port"] = vSt_mainConfig.vU16_webServerPort;
        doc["auth_enabled"] = vSt_mainConfig.vB_authEnabled;
        doc["web_username"] = vSt_mainConfig.vS_webUsername;
        doc["web_password"] = vSt_mainConfig.vS_webPassword;
        doc["dashboard_auth_required"] = vSt_mainConfig.vB_dashboardAuthRequired;
        
        // Configurações de checagem de rede
        doc["wifi_check_interval"] = vSt_mainConfig.vU32_wifiCheckInterval;
        
        // Configurações de AP (Access Point Fallback)
        doc["ap_ssid"] = vSt_mainConfig.vS_apSsid;
        doc["ap_pass"] = vSt_mainConfig.vS_apPass;
        doc["ap_fallback_enabled"] = vSt_mainConfig.vB_apFallbackEnabled;
        
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
    fV_printSerialDebug(LOG_WEB, "Servidor web assincrono iniciado na porta %d", vSt_mainConfig.vU16_webServerPort);
    if (vSt_mainConfig.vB_authEnabled) {
        fV_printSerialDebug(LOG_WEB, "Autenticação habilitada para usuário: %s", vSt_mainConfig.vS_webUsername.c_str());
        if (vSt_mainConfig.vB_dashboardAuthRequired) {
            fV_printSerialDebug(LOG_WEB, "Dashboard também requer autenticação");
        } else {
            fV_printSerialDebug(LOG_WEB, "Dashboard acessível sem autenticação");
        }
    } else {
        fV_printSerialDebug(LOG_WEB, "Autenticação desabilitada - acesso livre");
    }
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
        fV_printSerialDebug(LOG_WEB, "[CONFIG] Processando configurações avançadas...");
        
        if (request->hasArg("hostname")) {
            vSt_mainConfig.vS_hostname = request->arg("hostname");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] hostname = %s", vSt_mainConfig.vS_hostname.c_str());
        }
        if (request->hasArg("wifi_ssid")) {
            vSt_mainConfig.vS_wifiSsid = request->arg("wifi_ssid");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] wifi_ssid = %s", vSt_mainConfig.vS_wifiSsid.c_str());
        }
        if (request->hasArg("wifi_pass") && request->arg("wifi_pass").length() > 0) {
            vSt_mainConfig.vS_wifiPass = request->arg("wifi_pass");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] wifi_pass atualizada");
        }
        if (request->hasArg("wifi_attempts")) {
            vSt_mainConfig.vU16_wifiConnectAttempts = request->arg("wifi_attempts").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] wifi_attempts = %d", vSt_mainConfig.vU16_wifiConnectAttempts);
        }
        if (request->hasArg("wifi_check_interval")) {
            vSt_mainConfig.vU32_wifiCheckInterval = request->arg("wifi_check_interval").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] wifi_check_interval = %lu", vSt_mainConfig.vU32_wifiCheckInterval);
        }
        
        // NTP
        if (request->hasArg("ntp_server1")) {
            vSt_mainConfig.vS_ntpServer1 = request->arg("ntp_server1");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] ntp_server1 = %s", vSt_mainConfig.vS_ntpServer1.c_str());
        }
        if (request->hasArg("gmt_offset")) {
            vSt_mainConfig.vI_gmtOffsetSec = request->arg("gmt_offset").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] gmt_offset = %ld", vSt_mainConfig.vI_gmtOffsetSec);
        }
        if (request->hasArg("daylight_offset")) {
            vSt_mainConfig.vI_daylightOffsetSec = request->arg("daylight_offset").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] daylight_offset = %d", vSt_mainConfig.vI_daylightOffsetSec);
        }
        
        // Interface
        vSt_mainConfig.vB_statusPinosEnabled = request->hasArg("status_pinos_enabled") && request->arg("status_pinos_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] status_pinos_enabled = %d", vSt_mainConfig.vB_statusPinosEnabled);
        
        vSt_mainConfig.vB_interModulosEnabled = request->hasArg("inter_modulos_enabled") && request->arg("inter_modulos_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] inter_modulos_enabled = %d", vSt_mainConfig.vB_interModulosEnabled);
        
        if (request->hasArg("cor_com_alerta")) {
            vSt_mainConfig.vS_corStatusComAlerta = request->arg("cor_com_alerta");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cor_com_alerta = %s", vSt_mainConfig.vS_corStatusComAlerta.c_str());
        }
        if (request->hasArg("cor_sem_alerta")) {
            vSt_mainConfig.vS_corStatusSemAlerta = request->arg("cor_sem_alerta");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cor_sem_alerta = %s", vSt_mainConfig.vS_corStatusSemAlerta.c_str());
        }
        if (request->hasArg("tempo_refresh")) {
            vSt_mainConfig.vU16_tempoRefresh = request->arg("tempo_refresh").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] tempo_refresh = %d", vSt_mainConfig.vU16_tempoRefresh);
        }
        
        // Sistema
        if (request->hasArg("qtd_pinos")) {
            vSt_mainConfig.vU8_quantidadePinos = request->arg("qtd_pinos").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] qtd_pinos = %d", vSt_mainConfig.vU8_quantidadePinos);
        }
        
        // Watchdog
        vSt_mainConfig.vB_watchdogEnabled = request->hasArg("watchdog_enabled") && request->arg("watchdog_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] watchdog_enabled = %d", vSt_mainConfig.vB_watchdogEnabled);
        
        if (request->hasArg("clock_esp32_mhz")) {
            vSt_mainConfig.vU16_clockEsp32Mhz = request->arg("clock_esp32_mhz").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] clock_esp32_mhz = %d", vSt_mainConfig.vU16_clockEsp32Mhz);
        }
        if (request->hasArg("tempo_watchdog_us")) {
            vSt_mainConfig.vU32_tempoWatchdogUs = request->arg("tempo_watchdog_us").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] tempo_watchdog_us = %lu", vSt_mainConfig.vU32_tempoWatchdogUs);
        }
        
        // Debug/Log
        vSt_mainConfig.vB_serialDebugEnabled = request->hasArg("serial_debug_enabled") && request->arg("serial_debug_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] serial_debug_enabled = %d", vSt_mainConfig.vB_serialDebugEnabled);
        
        if (request->hasArg("active_log_flags")) {
            vSt_mainConfig.vU32_activeLogFlags = request->arg("active_log_flags").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] active_log_flags = %lu", vSt_mainConfig.vU32_activeLogFlags);
        }
        
        // Servidor Web
        if (request->hasArg("web_server_port")) {
            vSt_mainConfig.vU16_webServerPort = request->arg("web_server_port").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] web_server_port = %d", vSt_mainConfig.vU16_webServerPort);
        }
        
        vSt_mainConfig.vB_authEnabled = request->hasArg("auth_enabled") && request->arg("auth_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] auth_enabled = %d", vSt_mainConfig.vB_authEnabled);
        
        if (request->hasArg("web_username")) {
            vSt_mainConfig.vS_webUsername = request->arg("web_username");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] web_username = %s", vSt_mainConfig.vS_webUsername.c_str());
        }
        if (request->hasArg("web_password") && request->arg("web_password").length() > 0) {
            vSt_mainConfig.vS_webPassword = request->arg("web_password");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] web_password atualizada");
        }
        
        vSt_mainConfig.vB_dashboardAuthRequired = request->hasArg("dashboard_auth_required") && request->arg("dashboard_auth_required") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] dashboard_auth_required = %d", vSt_mainConfig.vB_dashboardAuthRequired);
        
        // Configurações de AP (Access Point Fallback)
        if (request->hasArg("ap_ssid")) {
            vSt_mainConfig.vS_apSsid = request->arg("ap_ssid");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] ap_ssid = %s", vSt_mainConfig.vS_apSsid.c_str());
        }
        if (request->hasArg("ap_pass") && request->arg("ap_pass").length() > 0) {
            vSt_mainConfig.vS_apPass = request->arg("ap_pass");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] ap_pass atualizada");
        }
        
        vSt_mainConfig.vB_apFallbackEnabled = request->hasArg("ap_fallback_enabled") && request->arg("ap_fallback_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] ap_fallback_enabled = %d", vSt_mainConfig.vB_apFallbackEnabled);
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes avancadas completas recebidas via POST.");
    }
    
    // === CONCEITO CISCO-LIKE: SALVAR AUTOMATICAMENTE NA FLASH ===
    // Salva na memoria flash (NVS) imediatamente após qualquer alteração
    fV_salvarMainConfig();
    fV_printSerialDebug(LOG_WEB, "[CISCO-LIKE] Configurações salvas automaticamente na flash");
    
    // Envia resposta 200 (OK) para o JavaScript
    request->send(200, "text/plain", "Configurações salvas! O sistema será reiniciado...");
    
    // === REBOOT AUTOMÁTICO APÓS SALVAMENTO ===
    fV_printSerialDebug(LOG_WEB, "[CISCO-LIKE] Reiniciando sistema automaticamente...");
    Serial.flush();
    delay(500);  // Tempo para enviar resposta HTTP
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
    // Usamos JsonDocument para os dados de status
    JsonDocument vL_jsonDoc; 

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
    
    // Adiciona informações sobre pinos
    system_obj["max_pins"] = vSt_mainConfig.vU8_quantidadePinos;
    system_obj["show_pin_status"] = vSt_mainConfig.vB_statusPinosEnabled;
    
    // Conta pinos ativos (excluindo tipo 0 = "Não Utilizado")
    uint8_t activePinsCount = 0;
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        if (vA_pinConfigs[i].tipo != 0) {  // Não conta pinos "Não Utilizados" (tipo 0)
            activePinsCount++;
        }
    }
    system_obj["active_pins"] = activePinsCount;
    
    // 4. Dados de Pinos para Dashboard (apenas se habilitado nas configurações)
    JsonArray pins_array = vL_jsonDoc["pins"].to<JsonArray>();
    if (vSt_mainConfig.vB_statusPinosEnabled) {
        for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
            if (vA_pinConfigs[i].tipo != 0) {  // Apenas pinos utilizados
                JsonObject pin_obj = pins_array.add<JsonObject>();
                pin_obj["gpio"] = vA_pinConfigs[i].pino;
                pin_obj["description"] = vA_pinConfigs[i].nome;
                pin_obj["tipo"] = vA_pinConfigs[i].tipo;
                pin_obj["modo"] = vA_pinConfigs[i].modo;
                pin_obj["status_atual"] = vA_pinConfigs[i].status_atual;
                pin_obj["nivel_acionamento_min"] = vA_pinConfigs[i].nivel_acionamento_min;
                pin_obj["nivel_acionamento_max"] = vA_pinConfigs[i].nivel_acionamento_max;
            }
        }
    }
    
    // Adiciona cores do sistema para o dashboard
    system_obj["cor_com_alerta"] = vSt_mainConfig.vS_corStatusComAlerta;
    system_obj["cor_sem_alerta"] = vSt_mainConfig.vS_corStatusSemAlerta;

    // 3. Sincronizacao (Agrupados em "sync")
    JsonObject sync_obj = vL_jsonDoc["sync"].to<JsonObject>();
    sync_obj["ntp_status"] = vL_ntp_ok ? "Sincronizado" : "Desabilitado";
    sync_obj["mqtt_status"] = "Desabilitado"; 
    
    // Usa o tempo formatado se o NTP estiver OK, caso contrario, Uptime formatado
    sync_obj["last_update"] = vL_ntp_ok ? fS_getFormattedTime() : fS_formatUptime(millis()); 


    // 5. Serializa o JSON para uma String
    String vS_response;
    vS_response.reserve(1024);  // Aumentar o buffer para incluir dados de pinos 
    serializeJson(vL_jsonDoc, vS_response);

    // 5. Envia a resposta HTTP
    request->send(200, "application/json", vS_response);

    fV_printSerialDebug(LOG_WEB, "JSON de status servido.");
}

// Rota Handler: Página de configurações gerais
void fV_handleConfigPage(AsyncWebServerRequest *request) {
    // Inicia monitoramento de performance
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /configuracao - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    // Para páginas grandes, usar chunked encoding para evitar carregamento parcial
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        const char* content = web_config_gerais_html;
        size_t contentLen = strlen(content);
        
        if (index >= contentLen) {
            return 0; // End of content
        }
        
        size_t remaining = contentLen - index;
        size_t copyLen = (remaining < maxLen) ? remaining : maxLen;
        
        memcpy(buffer, content + index, copyLen);
        return copyLen;
    });
    
    // Adiciona headers de performance
    response->addHeader("X-Load-Start", String(startTime));
    response->addHeader("X-Load-Time", timeStr);
    request->send(response);
    
    // Log de finalização
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /configuracao - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
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
    vSt_mainConfig.vB_watchdogEnabled = request->hasArg("watchdog_enabled") && request->arg("watchdog_enabled") != "0";
    
    if (request->hasArg("clock_esp32_mhz")) {
        vSt_mainConfig.vU16_clockEsp32Mhz = request->arg("clock_esp32_mhz").toInt();
    }
    if (request->hasArg("tempo_watchdog_us")) {
        vSt_mainConfig.vU32_tempoWatchdogUs = request->arg("tempo_watchdog_us").toInt();
    }
    if (request->hasArg("qtd_pinos")) {
        vSt_mainConfig.vU8_quantidadePinos = request->arg("qtd_pinos").toInt();
    }
    
    // Configurações do servidor web
    if (request->hasArg("web_server_port")) {
        vSt_mainConfig.vU16_webServerPort = request->arg("web_server_port").toInt();
    }
    vSt_mainConfig.vB_authEnabled = request->hasArg("auth_enabled") && request->arg("auth_enabled") != "0";
    if (request->hasArg("web_username")) {
        vSt_mainConfig.vS_webUsername = request->arg("web_username");
    }
    if (request->hasArg("web_password")) {
        vSt_mainConfig.vS_webPassword = request->arg("web_password");
    }
    vSt_mainConfig.vB_dashboardAuthRequired = request->hasArg("dashboard_auth_required") && request->arg("dashboard_auth_required") != "0";

    fV_printSerialDebug(LOG_WEB, "[CISCO-LIKE] Configurações aplicadas em memória");
    
    // === CONCEITO CISCO-LIKE: SALVAR AUTOMATICAMENTE NA FLASH ===
    // Salva na memoria flash (NVS) imediatamente após alteração
    fV_salvarMainConfig();
    fV_printSerialDebug(LOG_WEB, "[CISCO-LIKE] Configurações salvas automaticamente na flash");
    
    // Envia resposta 200 (OK)
    request->send(200, "text/plain", "Configurações salvas! O sistema será reiniciado...");
    
    // === REBOOT AUTOMÁTICO APÓS SALVAMENTO ===
    fV_printSerialDebug(LOG_WEB, "[CISCO-LIKE] Reiniciando sistema automaticamente...");
    Serial.flush();
    delay(500);  // Tempo para enviar resposta HTTP
    ESP.restart();
}

// Handler para SALVAR configurações na flash (CISCO-LIKE - deprecated, agora é automático)
void fV_handleSaveToFlash(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[CISCO-LIKE] Aviso: Save explícito não necessário - configurações já são salvas automaticamente!");
    request->send(200, "text/plain", "AVISO: Sistema já salva automaticamente. Configurações são persistidas a cada alteração!");
}

// Handler para página de MQTT/Serviços
void fV_handleMqttPage(AsyncWebServerRequest *request) {
    // Inicia monitoramento de performance
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /mqtt - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    // Envia página com headers de performance
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", web_mqtt_html);
    response->addHeader("X-Load-Start", String(startTime));
    response->addHeader("X-Load-Time", timeStr);
    request->send(response);
    
    // Log de finalização
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /mqtt - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de reset
void fV_handleResetPage(AsyncWebServerRequest *request) {
    // Inicia monitoramento de performance
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /reset - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    // Envia página com headers de performance
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", web_reset_html);
    response->addHeader("X-Load-Start", String(startTime));
    response->addHeader("X-Load-Time", timeStr);
    request->send(response);
    
    // Log de finalização
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /reset - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de arquivos
void fV_handleFilesPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /arquivos - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", web_files_html);
    response->addHeader("X-Load-Start", String(startTime));
    response->addHeader("X-Load-Time", timeStr);
    request->send(response);
    
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /arquivos - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler: API - Listar preferências NVS
void fV_handleNVSList(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Listando preferências NVS");
    
    JsonDocument doc;
    JsonArray preferences = doc["preferences"].to<JsonArray>();
    
    Preferences prefs;
    
    // Lê configurações principais do namespace smcrconf (29 parâmetros)
    if (prefs.begin("smcrconf", true)) {
        // 1. Debug/Log
        JsonObject sdbgen = preferences.add<JsonObject>();
        sdbgen["namespace"] = "smcrconf";
        sdbgen["key"] = "sdbgen";
        sdbgen["value"] = prefs.getBool("sdbgen", true) ? "true" : "false";
        sdbgen["type"] = "Bool";
        
        JsonObject logflags = preferences.add<JsonObject>();
        logflags["namespace"] = "smcrconf";
        logflags["key"] = "logflags";
        logflags["value"] = String(prefs.getUInt("logflags", 0xFFFFFFFF));
        logflags["type"] = "UInt";
        
        // 2. Rede WiFi STA
        JsonObject hostname = preferences.add<JsonObject>();
        hostname["namespace"] = "smcrconf";
        hostname["key"] = "hostname";
        hostname["value"] = prefs.getString("hostname", "N/A");
        hostname["type"] = "String";
        
        JsonObject wssid = preferences.add<JsonObject>();
        wssid["namespace"] = "smcrconf";
        wssid["key"] = "w_ssid";
        wssid["value"] = prefs.getString("w_ssid", "N/A");
        wssid["type"] = "String";
        
        JsonObject wpass = preferences.add<JsonObject>();
        wpass["namespace"] = "smcrconf";
        wpass["key"] = "w_pass";
        wpass["value"] = "[OCULTO]";
        wpass["type"] = "String";
        
        JsonObject wattmp = preferences.add<JsonObject>();
        wattmp["namespace"] = "smcrconf";
        wattmp["key"] = "w_attmp";
        wattmp["value"] = String(prefs.getUInt("w_attmp", 15));
        wattmp["type"] = "UInt";
        
        // 3. AP Fallback
        JsonObject assid = preferences.add<JsonObject>();
        assid["namespace"] = "smcrconf";
        assid["key"] = "a_ssid";
        assid["value"] = prefs.getString("a_ssid", "N/A");
        assid["type"] = "String";
        
        JsonObject apass = preferences.add<JsonObject>();
        apass["namespace"] = "smcrconf";
        apass["key"] = "a_pass";
        apass["value"] = "[OCULTO]";
        apass["type"] = "String";
        
        JsonObject afallb = preferences.add<JsonObject>();
        afallb["namespace"] = "smcrconf";
        afallb["key"] = "a_fallb";
        afallb["value"] = prefs.getBool("a_fallb", true) ? "true" : "false";
        afallb["type"] = "Bool";
        
        // 4. Rede - Geral
        JsonObject wchkint = preferences.add<JsonObject>();
        wchkint["namespace"] = "smcrconf";
        wchkint["key"] = "w_chkint";
        wchkint["value"] = String(prefs.getULong("w_chkint", 15000));
        wchkint["type"] = "ULong";
        
        // 5. NTP
        JsonObject ntp1 = preferences.add<JsonObject>();
        ntp1["namespace"] = "smcrconf";
        ntp1["key"] = "ntp1";
        ntp1["value"] = prefs.getString("ntp1", "N/A");
        ntp1["type"] = "String";
        
        JsonObject gmtofs = preferences.add<JsonObject>();
        gmtofs["namespace"] = "smcrconf";
        gmtofs["key"] = "gmtofs";
        gmtofs["value"] = String(prefs.getInt("gmtofs", -10800));
        gmtofs["type"] = "Int";
        
        JsonObject dltofs = preferences.add<JsonObject>();
        dltofs["namespace"] = "smcrconf";
        dltofs["key"] = "dltofs";
        dltofs["value"] = String(prefs.getInt("dltofs", 0));
        dltofs["type"] = "Int";
        
        // 6. Interface Web
        JsonObject stpinos = preferences.add<JsonObject>();
        stpinos["namespace"] = "smcrconf";
        stpinos["key"] = "st_pinos";
        stpinos["value"] = prefs.getBool("st_pinos", true) ? "true" : "false";
        stpinos["type"] = "Bool";
        
        JsonObject intermod = preferences.add<JsonObject>();
        intermod["namespace"] = "smcrconf";
        intermod["key"] = "inter_mod";
        intermod["value"] = prefs.getBool("inter_mod", true) ? "true" : "false";
        intermod["type"] = "Bool";
        
        JsonObject coralert = preferences.add<JsonObject>();
        coralert["namespace"] = "smcrconf";
        coralert["key"] = "cor_alert";
        coralert["value"] = prefs.getString("cor_alert", "#ff0000");
        coralert["type"] = "String";
        
        JsonObject corok = preferences.add<JsonObject>();
        corok["namespace"] = "smcrconf";
        corok["key"] = "cor_ok";
        corok["value"] = prefs.getString("cor_ok", "#00ff00");
        corok["type"] = "String";
        
        JsonObject refresh = preferences.add<JsonObject>();
        refresh["namespace"] = "smcrconf";
        refresh["key"] = "refresh";
        refresh["value"] = String(prefs.getUInt("refresh", 15));
        refresh["type"] = "UInt";
        
        // 7. Watchdog
        JsonObject wdten = preferences.add<JsonObject>();
        wdten["namespace"] = "smcrconf";
        wdten["key"] = "wdt_en";
        wdten["value"] = prefs.getBool("wdt_en", false) ? "true" : "false";
        wdten["type"] = "Bool";
        
        JsonObject clkmhz = preferences.add<JsonObject>();
        clkmhz["namespace"] = "smcrconf";
        clkmhz["key"] = "clk_mhz";
        clkmhz["value"] = String(prefs.getUInt("clk_mhz", 240));
        clkmhz["type"] = "UInt";
        
        JsonObject wdttime = preferences.add<JsonObject>();
        wdttime["namespace"] = "smcrconf";
        wdttime["key"] = "wdt_time";
        wdttime["value"] = String(prefs.getULong("wdt_time", 8000000));
        wdttime["type"] = "ULong";
        
        // 8. Pinos
        JsonObject qtdpinos = preferences.add<JsonObject>();
        qtdpinos["namespace"] = "smcrconf";
        qtdpinos["key"] = "qtd_pinos";
        qtdpinos["value"] = String(prefs.getUChar("qtd_pinos", 16));
        qtdpinos["type"] = "UChar";
        
        // 9. Servidor Web & Auth
        JsonObject webport = preferences.add<JsonObject>();
        webport["namespace"] = "smcrconf";
        webport["key"] = "web_port";
        webport["value"] = String(prefs.getUInt("web_port", 8080));
        webport["type"] = "UInt";
        
        JsonObject authen = preferences.add<JsonObject>();
        authen["namespace"] = "smcrconf";
        authen["key"] = "auth_en";
        authen["value"] = prefs.getBool("auth_en", false) ? "true" : "false";
        authen["type"] = "Bool";
        
        JsonObject webuser = preferences.add<JsonObject>();
        webuser["namespace"] = "smcrconf";
        webuser["key"] = "web_user";
        webuser["value"] = prefs.getString("web_user", "admin");
        webuser["type"] = "String";
        
        JsonObject webpass = preferences.add<JsonObject>();
        webpass["namespace"] = "smcrconf";
        webpass["key"] = "web_pass";
        webpass["value"] = "[OCULTO]";
        webpass["type"] = "String";
        
        JsonObject dashauth = preferences.add<JsonObject>();
        dashauth["namespace"] = "smcrconf";
        dashauth["key"] = "dash_auth";
        dashauth["value"] = prefs.getBool("dash_auth", false) ? "true" : "false";
        dashauth["type"] = "Bool";
        
        prefs.end();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
    
    fV_printSerialDebug(LOG_WEB, "[API] Lista NVS enviada");
}

// Handler: API - Listar arquivos LittleFS
void fV_handleFilesList(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Listando arquivos LittleFS");
    
    JsonDocument doc;
    JsonArray files = doc["files"].to<JsonArray>();
    
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        doc["error"] = "Falha ao abrir diretório raiz";
        String response;
        serializeJson(doc, response);
        request->send(500, "application/json", response);
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            JsonObject fileObj = files.add<JsonObject>();
            String fileName = String(file.name());
            // Remove barra inicial se houver
            if (fileName.startsWith("/")) {
                fileName = fileName.substring(1);
            }
            fileObj["name"] = fileName;
            fileObj["size"] = file.size();
        }
        file = root.openNextFile();
    }
    
    // Informações do sistema de arquivos
    doc["totalBytes"] = LittleFS.totalBytes();
    doc["usedBytes"] = LittleFS.usedBytes();
    doc["freeBytes"] = LittleFS.totalBytes() - LittleFS.usedBytes();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
    
    fV_printSerialDebug(LOG_WEB, "[API] Lista de arquivos enviada");
}

// Handler: API - Download de arquivo
void fV_handleFileDownload(AsyncWebServerRequest *request) {
    if (!request->hasParam("filename")) {
        fV_printSerialDebug(LOG_WEB, "[API] Parâmetro 'filename' ausente na requisição de download");
        request->send(400, "text/plain", "Parâmetro 'filename' ausente");
        return;
    }
    
    String filename = request->getParam("filename")->value();
    fV_printSerialDebug(LOG_WEB, "[API] Filename recebido para download (bruto): '%s'", filename.c_str());
    
    if (!filename.startsWith("/")) {
        filename = "/" + filename;
    }
    
    fV_printSerialDebug(LOG_WEB, "[API] Download solicitado: %s", filename.c_str());
    
    if (!LittleFS.exists(filename)) {
        fV_printSerialDebug(LOG_WEB, "[API] Arquivo não encontrado: %s", filename.c_str());
        request->send(404, "text/plain", "Arquivo não encontrado");
        return;
    }
    
    // Extrai apenas o nome do arquivo (sem barra)
    String displayName = filename;
    if (displayName.startsWith("/")) {
        displayName = displayName.substring(1);
    }
    
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, filename, "application/octet-stream");
    response->addHeader("Content-Disposition", "attachment; filename=\"" + displayName + "\"");
    request->send(response);
    
    fV_printSerialDebug(LOG_WEB, "[API] Arquivo %s enviado como '%s'", filename.c_str(), displayName.c_str());
}

// Handler: API - Deletar arquivo
void fV_handleFileDelete(AsyncWebServerRequest *request) {
    if (!request->hasParam("filename", true)) {
        fV_printSerialDebug(LOG_WEB, "[API] Parâmetro 'filename' ausente na requisição de deleção");
        request->send(400, "application/json", "{\"success\": false, \"message\": \"Parâmetro 'filename' ausente\"}");
        return;
    }
    
    String filename = request->getParam("filename", true)->value();
    fV_printSerialDebug(LOG_WEB, "[API] Filename recebido (bruto): '%s'", filename.c_str());
    
    if (!filename.startsWith("/")) {
        filename = "/" + filename;
    }
    
    fV_printSerialDebug(LOG_WEB, "[API] Deleção solicitada: %s", filename.c_str());
    
    if (!LittleFS.exists(filename)) {
        fV_printSerialDebug(LOG_WEB, "[API] Arquivo não encontrado: %s", filename.c_str());
        request->send(404, "application/json", "{\"success\": false, \"message\": \"Arquivo não encontrado\"}");
        return;
    }
    
    if (LittleFS.remove(filename)) {
        fV_printSerialDebug(LOG_WEB, "[API] Arquivo %s deletado com sucesso", filename.c_str());
        request->send(200, "application/json", "{\"success\": true, \"message\": \"Arquivo deletado\"}");
    } else {
        fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[API] Erro ao deletar %s", filename.c_str());
        request->send(500, "application/json", "{\"success\": false, \"message\": \"Erro ao deletar arquivo\"}");
    }
}

// Handler: API - Formatar flash LittleFS
void fV_handleFormatFlash(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[API] Iniciando formatação da flash LittleFS");
    
    if (LittleFS.format()) {
        fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[API] Flash LittleFS formatada com sucesso");
        request->send(200, "application/json", "{\"success\": true, \"message\": \"Flash formatada com sucesso\"}");
    } else {
        fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[API] Erro ao formatar flash LittleFS");
        request->send(500, "application/json", "{\"success\": false, \"message\": \"Erro ao formatar flash\"}");
    }
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

// Handler para reset de configurações mantendo rede
void fV_handleConfigReset(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Executando reset de configurações (mantendo rede)...");
    
    request->send(200, "text/plain", "Reset de configurações executado. Rede mantida. Reiniciando...");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Limpa configurações exceto rede
    fV_clearConfigExceptNetwork();
    
    // Delay adicional para garantir que a limpeza seja concluída
    delay(500);
    
    // Reinicia o ESP32
    ESP.restart();
}

// Handler para reset apenas de pinos
void fV_handlePinsReset(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Executando reset apenas de pinos...");
    
    request->send(200, "text/plain", "OK");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Limpa apenas configurações de pinos
    fV_clearPinConfigs();
    
    fV_printSerialDebug(LOG_WEB, "[RESET] Reset de pinos concluído.");
}

// Handler para restart simples
void fV_handleRestart(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESTART] Reiniciando sistema...");
    
    request->send(200, "text/plain", "Sistema reiniciando...");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Reinicia o ESP32
    ESP.restart();
}

//========================================
// Handler para Adicionar Pino (/pins/add)
//========================================

void fV_handlePinAdd(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[PINS] Recebendo solicitacao para adicionar novo pino");
    
    // Validação de parâmetros obrigatórios
    if (!request->hasArg("nome") || !request->hasArg("pino") || !request->hasArg("tipo") || !request->hasArg("modo")) {
        fV_printSerialDebug(LOG_WEB, "[PINS] ERRO: Parametros obrigatorios ausentes");
        request->send(400, "text/plain", "ERRO: Parametros obrigatorios ausentes (nome, pino, tipo, modo)");
        return;
    }
    
    // Cria nova configuração de pino
    PinConfig_t newPin;
    newPin.nome = request->arg("nome");
    newPin.pino = request->arg("pino").toInt();
    newPin.tipo = request->arg("tipo").toInt();
    newPin.modo = request->arg("modo").toInt();
    newPin.xor_logic = request->hasArg("xor_logic") ? request->arg("xor_logic").toInt() : 0;
    newPin.tempo_retencao = request->hasArg("tempo_retencao") ? request->arg("tempo_retencao").toInt() : 0;
    newPin.status_atual = 0;
    newPin.ignore_contador = 0;
    newPin.nivel_acionamento_min = request->hasArg("nivel_acionamento_min") ? request->arg("nivel_acionamento_min").toInt() : 0;
    newPin.nivel_acionamento_max = request->hasArg("nivel_acionamento_max") ? request->arg("nivel_acionamento_max").toInt() : 0;
    
    // Tenta adicionar o pino
    int result = fI_addPinConfig(newPin);
    
    if (result >= 0) {
        // === CONCEITO CISCO-LIKE: SALVAR AUTOMATICAMENTE ===
        fB_savePinConfigs();  // Salva configurações na flash automaticamente
        fV_printSerialDebug(LOG_WEB, "[PINS] Pino %d adicionado com sucesso (indice %d) e salvo na flash", newPin.pino, result);
        request->send(200, "text/plain", "OK");
    } else if (result == -1) {
        fV_printSerialDebug(LOG_WEB, "[PINS] ERRO: Limite de pinos atingido");
        request->send(400, "text/plain", "ERRO: Limite de pinos atingido");
    } else {
        fV_printSerialDebug(LOG_WEB, "[PINS] ERRO: Pino ja existe");
        request->send(400, "text/plain", "ERRO: Pino ja existe");
    }
}

//========================================
// Handlers da API de Pinos
//========================================

// API: Listar todos os pinos configurados
void fV_handlePinsListApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Listando pinos configurados");
    
    // Atualiza o status atual dos pinos antes de retornar os dados
    fV_updatePinStatus();
    
    JsonDocument doc;
    JsonArray pinsArray = doc["pins"].to<JsonArray>();
    
    // Adiciona todos os pinos ativos ao array JSON
    for (uint8_t i = 0; i < vU8_activePinsCount; i++) {
        if (vA_pinConfigs[i].tipo != 0) {  // Não adiciona pinos "Não Utilizados" (tipo 0)
            JsonObject pinObj = pinsArray.add<JsonObject>();
            pinObj["nome"] = vA_pinConfigs[i].nome;
            pinObj["pino"] = vA_pinConfigs[i].pino;
            pinObj["tipo"] = vA_pinConfigs[i].tipo;
            pinObj["modo"] = vA_pinConfigs[i].modo;
            pinObj["xor_logic"] = vA_pinConfigs[i].xor_logic;
            pinObj["tempo_retencao"] = vA_pinConfigs[i].tempo_retencao;
            pinObj["nivel_acionamento_min"] = vA_pinConfigs[i].nivel_acionamento_min;
            pinObj["nivel_acionamento_max"] = vA_pinConfigs[i].nivel_acionamento_max;
            pinObj["status_atual"] = vA_pinConfigs[i].status_atual;
        }
    }
    
    doc["total"] = vU8_activePinsCount;
    doc["max_pins"] = vSt_mainConfig.vU8_quantidadePinos;
    doc["has_changes"] = fB_hasPinConfigChanges(); // Indica se há mudanças não salvas
    
    String response;
    serializeJson(doc, response);
    
    request->send(200, "application/json", response);
}

// API: Criar novo pino
void fV_handlePinCreateApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Criando novo pino");
    
    // Verifica se há dados no body
    if (request->_tempObject == nullptr) {
        request->send(400, "application/json", "{\"error\": \"Dados do pino não fornecidos\"}");
        return;
    }
    
    // Parse do JSON recebido
    String body = *((String*)request->_tempObject);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        fV_printSerialDebug(LOG_WEB, "[API] Erro de parse JSON: %s", error.c_str());
        request->send(400, "application/json", "{\"error\": \"JSON inválido\"}");
        return;
    }
    
    // Cria configuração do pino
    PinConfig_t newPin;
    newPin.nome = doc["nome"] | "";
    newPin.pino = doc["pino"] | 0;
    newPin.tipo = doc["tipo"] | 0;
    newPin.modo = doc["modo"] | 0;
    newPin.xor_logic = doc["xor_logic"] | 0;
    newPin.tempo_retencao = doc["tempo_retencao"] | 0;
    newPin.nivel_acionamento_min = doc["nivel_acionamento_min"] | 0;
    newPin.nivel_acionamento_max = doc["nivel_acionamento_max"] | 0;
    newPin.status_atual = 0;
    newPin.ignore_contador = 0;
    
    // Valida dados básicos
    if (newPin.pino == 0 || newPin.nome.isEmpty()) {
        request->send(400, "application/json", "{\"error\": \"Pino e nome são obrigatórios\"}");
        return;
    }
    
    // Adiciona o pino (apenas running config)
    int result = fI_addPinConfig(newPin);
    if (result < 0) {
        request->send(400, "application/json", "{\"error\": \"Erro ao adicionar pino ou limite atingido\"}");
        return;
    }
    
    // Aplica configuração física se necessário
    fV_setupConfiguredPins();
    
    request->send(200, "application/json", "{\"success\": true, \"id\": " + String(result) + ", \"message\": \"Pino adicionado à running config\"}");
}

// API: Atualizar pino existente
void fV_handlePinUpdateApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Atualizando pino");
    
    // Extrai número do pino da URL
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash == -1) {
        request->send(400, "application/json", "{\"error\": \"URL inválida\"}");
        return;
    }
    
    uint8_t pinNumber = url.substring(lastSlash + 1).toInt();
    
    // Verifica se há dados no body
    if (request->_tempObject == nullptr) {
        request->send(400, "application/json", "{\"error\": \"Dados do pino não fornecidos\"}");
        return;
    }
    
    // Parse do JSON recebido
    String body = *((String*)request->_tempObject);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        request->send(400, "application/json", "{\"error\": \"JSON inválido\"}");
        return;
    }
    
    // Cria nova configuração
    PinConfig_t updatedPin;
    updatedPin.nome = doc["nome"] | "";
    updatedPin.pino = doc["pino"] | pinNumber;
    updatedPin.tipo = doc["tipo"] | 0;
    updatedPin.modo = doc["modo"] | 0;
    updatedPin.xor_logic = doc["xor_logic"] | 0;
    updatedPin.tempo_retencao = doc["tempo_retencao"] | 0;
    updatedPin.nivel_acionamento_min = doc["nivel_acionamento_min"] | 0;
    updatedPin.nivel_acionamento_max = doc["nivel_acionamento_max"] | 0;
    
    // Atualiza o pino (apenas running config)
    bool success = fB_updatePinConfig(pinNumber, updatedPin);
    if (!success) {
        request->send(404, "application/json", "{\"error\": \"Pino não encontrado\"}");
        return;
    }
    
    // Aplica configuração física se necessário
    fV_setupConfiguredPins();
    
    request->send(200, "application/json", "{\"success\": true, \"message\": \"Pino atualizado na running config\"}");
}

// API: Deletar pino
void fV_handlePinDeleteApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Deletando pino");
    
    // Extrai número do pino da URL
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash == -1) {
        request->send(400, "application/json", "{\"error\": \"URL inválida\"}");
        return;
    }
    
    uint8_t pinNumber = url.substring(lastSlash + 1).toInt();
    
    // Remove o pino (apenas running config)
    bool success = fB_removePinConfig(pinNumber);
    if (!success) {
        request->send(404, "application/json", "{\"error\": \"Pino não encontrado\"}");
        return;
    }
    
    // Reaplica configurações físicas
    fV_setupConfiguredPins();
    
    request->send(200, "application/json", "{\"success\": true, \"message\": \"Pino removido da running config\"}");
}

// API: Salvar configurações de pinos na flash (startup config)
void fV_handlePinsSaveApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Salvando configurações de pinos na flash (startup config)");
    
    // Salva as configurações atuais na flash
    bool success = fB_savePinConfigs();
    
    if (success) {
        request->send(200, "application/json", "{\"success\": true, \"message\": \"Configurações de pinos salvas na flash\"}");
    } else {
        fV_printSerialDebug(LOG_WEB, "[API] ERRO: Falha ao salvar configurações de pinos na flash");
        request->send(500, "application/json", "{\"success\": false, \"error\": \"Falha ao salvar configurações na flash. Verifique o espaço disponível.\"}");
    }
}

// API: Carregar configurações de pinos da flash (reload startup config)
void fV_handlePinsReloadApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Recarregando configurações de pinos da flash");
    
    // Limpa configurações atuais na memória
    if (vA_pinConfigs != nullptr) {
        uint8_t maxPins = vSt_mainConfig.vU8_quantidadePinos;
        for (uint8_t i = 0; i < maxPins; i++) {
            vA_pinConfigs[i].nome = "";
            vA_pinConfigs[i].pino = 0;
            vA_pinConfigs[i].tipo = 0;
            vA_pinConfigs[i].modo = 0;
            vA_pinConfigs[i].xor_logic = 0;
            vA_pinConfigs[i].tempo_retencao = 0;
            vA_pinConfigs[i].nivel_acionamento_min = 0;
            vA_pinConfigs[i].nivel_acionamento_max = 0;
            vA_pinConfigs[i].status_atual = 0;
            vA_pinConfigs[i].ignore_contador = 0;
        }
    }
    vU8_activePinsCount = 0;
    
    // Recarrega da flash
    fV_loadPinConfigs();
    
    // Reaplica configurações físicas
    fV_setupConfiguredPins();
    
    request->send(200, "application/json", "{\"success\": true, \"message\": \"Configurações recarregadas da flash\"}");
}

// API: Limpar todas as configurações de pinos (running config apenas)
void fV_handlePinsClearApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Limpando configurações de pinos da running config");
    
    // Limpa apenas na memória (running config)
    if (vA_pinConfigs != nullptr) {
        uint8_t maxPins = vSt_mainConfig.vU8_quantidadePinos;
        for (uint8_t i = 0; i < maxPins; i++) {
            vA_pinConfigs[i].nome = "";
            vA_pinConfigs[i].pino = 0;
            vA_pinConfigs[i].tipo = 0;
            vA_pinConfigs[i].modo = 0;
            vA_pinConfigs[i].xor_logic = 0;
            vA_pinConfigs[i].tempo_retencao = 0;
            vA_pinConfigs[i].nivel_acionamento_min = 0;
            vA_pinConfigs[i].nivel_acionamento_max = 0;
            vA_pinConfigs[i].status_atual = 0;
            vA_pinConfigs[i].ignore_contador = 0;
        }
    }
    vU8_activePinsCount = 0;
    
    // Reaplica configurações físicas (limpa GPIOs)
    fV_setupConfiguredPins();
    
    request->send(200, "application/json", "{\"success\": true, \"message\": \"Running config de pinos limpa\"}");
}


