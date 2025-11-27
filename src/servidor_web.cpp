// Conteúdo de servidor_web.cpp
#include "globals.h"
#include "web_modoap.h"
#include "web_dashboard.h"
#include "web_config_gerais.h"
#include "web_reset.h"
#include "web_pins.h"
#include "web_mqtt.h"
#include "web_intermod.h"
#include "web_files.h"
#include "web_actions.h"
#include "web_firmware.h"
#include "web_preferencias.h"
#include "web_littlefs.h"
#include "web_serial.h"
#include <LittleFS.h>
#include <Preferences.h>
// Inclusões para navegação completa no NVS
#include <nvs.h>
#include <nvs_flash.h>
#include <Update.h>

// Forward declarations para handlers de arquivos
void fV_handleFilesPage(AsyncWebServerRequest *request);
void fV_handleFirmwarePage(AsyncWebServerRequest *request);
void fV_handleNVSList(AsyncWebServerRequest *request);
void fV_handleNVSExport(AsyncWebServerRequest *request);
void fV_handleNVSImport(AsyncWebServerRequest *request);
void fV_handleFilesList(AsyncWebServerRequest *request);
void fV_handleFileDownload(AsyncWebServerRequest *request);
void fV_handleFileDelete(AsyncWebServerRequest *request);
void fV_handleFormatFlash(AsyncWebServerRequest *request);
void fV_handleSerialPage(AsyncWebServerRequest *request);
void fV_handleSerialLogs(AsyncWebServerRequest *request);

// Helper: serve página de LittleFS com fallback para PROGMEM
void servePageWithFallback(AsyncWebServerRequest *request, const char* fsPath, const char* progmemContent) {
    if (LittleFS.exists(fsPath)) {
        fV_printSerialDebug(LOG_WEB, "Servindo %s de LittleFS", fsPath);
        request->send(LittleFS, fsPath, "text/html");
    } else {
        fV_printSerialDebug(LOG_WEB, "Arquivo %s nao encontrado, usando PROGMEM fallback", fsPath);
        request->send(200, "text/html", progmemContent);
    }
}

// Buffer circular para logs do Web Serial
#define SERIAL_LOG_BUFFER_SIZE 100
static String g_serialLogBuffer[SERIAL_LOG_BUFFER_SIZE];
static uint16_t g_serialLogIndex = 0;
static uint32_t g_serialLogCounter = 0;
static bool g_webServerReady = false;

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
             
             // Envia dashboard com LittleFS-first + fallback
             servePageWithFallback(request, "/web_dashboard.html", web_dashboard_html);
             
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
    
    // Rota para página de configuração de pinos (/pins e /pinos)
    auto pinsPageHandler = [](AsyncWebServerRequest *request) {
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
        
        // Envia página de pinos com LittleFS-first + fallback
        servePageWithFallback(request, "/web_pins.html", web_pins_html);
        
        // Log de finalização
        unsigned long endTime = millis();
        String endTimeStr = fS_getFormattedTime();
        fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /pins - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
    };
    
    SERVIDOR_WEB_ASYNC->on("/pins", HTTP_GET, pinsPageHandler);
    SERVIDOR_WEB_ASYNC->on("/pinos", HTTP_GET, pinsPageHandler);
    
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
    
    // Rota para página de Inter-Módulos
    SERVIDOR_WEB_ASYNC->on("/intermod", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleInterModPage(request);
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
    
    // Rotas para arquivos - rotas específicas PRIMEIRO (antes da genérica /arquivos)
    // Rota para página de firmware OTA
    SERVIDOR_WEB_ASYNC->on("/arquivos/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleFirmwarePage(request);
    });
    
    // Rota para página de preferências NVS
    SERVIDOR_WEB_ASYNC->on("/arquivos/preferencias", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handlePreferenciasPage(request);
    });
    
    // Rota para página de LittleFS
    SERVIDOR_WEB_ASYNC->on("/arquivos/littlefs", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleLittleFSPage(request);
    });
    
    // Rota para página de arquivos (mantida para compatibilidade - redireciona para firmware)
    SERVIDOR_WEB_ASYNC->on("/arquivos", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/arquivos/firmware");
    });
    
    // Rota para página de ações
    SERVIDOR_WEB_ASYNC->on("/acoes", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleActionsPage(request);
    });

    // Rota para página de atualização de firmware
    SERVIDOR_WEB_ASYNC->on("/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleFirmwarePage(request);
    });

    // API: Upload de firmware (OTA)
    SERVIDOR_WEB_ASYNC->on("/api/firmware/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Handler final: envia status baseado em Update.hasError()
            if (vSt_mainConfig.vB_authEnabled) {
                if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                    return request->requestAuthentication();
                }
            }

            bool ok = !Update.hasError();
            int code = ok ? 200 : 500;
            String payload = ok ? "{\"success\": true, \"message\": \"Firmware atualizado. Reiniciando...\"}"
                                : "{\"success\": false, \"message\": \"Falha no update\"}";
            AsyncWebServerResponse *response = request->beginResponse(code, "application/json", payload);
            response->addHeader("Connection", "close");
            request->send(response);

            if (ok) {
                fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[OTA] Update concluido com sucesso. Reiniciando...");
                // Pequeno atraso para permitir envio completo da resposta HTTP
                // e dar tempo ao browser para processar o sucesso.
                delay(800);
                ESP.restart();
            } else {
                fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[OTA] Falha no update");
            }
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // Upload em partes usando Update
            if (index == 0) {
                fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[OTA] Inicio upload firmware: %s (%u bytes esperados)", filename.c_str(), (unsigned)request->contentLength());
                // Tenta iniciar o update para tamanho desconhecido
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }

            if (!Update.hasError()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }

            if (final) {
                if (Update.end(true)) {
                    fV_printSerialDebug(LOG_WEB | LOG_FLASH, "[OTA] Upload finalizado (%u bytes)", (unsigned)(index + len));
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
    
    // Rota para página de Web Serial
    SERVIDOR_WEB_ASYNC->on("/serial", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleSerialPage(request);
    });
    
    // API: Obter logs do Web Serial (polling)
    SERVIDOR_WEB_ASYNC->on("/api/serial/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleSerialLogs(request);
    });
    
    // ==== API de Ações ====
    
    // API: Listar todas as ações
    SERVIDOR_WEB_ASYNC->on("/api/actions", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleActionsListApi(request);
    });
    
    // API: Criar nova ação
    SERVIDOR_WEB_ASYNC->on("/api/actions", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleActionCreateApi(request);
    });
    
    // API: Salvar ações na flash
    SERVIDOR_WEB_ASYNC->on("/api/actions/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleActionsSaveApi(request);
    });
    
    // Rotas de reset (POST)
    SERVIDOR_WEB_ASYNC->on("/reset/soft", HTTP_POST, fV_handleSoftReset);
    SERVIDOR_WEB_ASYNC->on("/reset/factory", HTTP_POST, fV_handleFactoryReset);
    SERVIDOR_WEB_ASYNC->on("/reset/network", HTTP_POST, fV_handleNetworkReset);
    SERVIDOR_WEB_ASYNC->on("/reset/config", HTTP_POST, fV_handleConfigReset);
    SERVIDOR_WEB_ASYNC->on("/reset/pins", HTTP_POST, fV_handlePinsReset);
    SERVIDOR_WEB_ASYNC->on("/restart", HTTP_POST, fV_handleRestart);
    
    // API: Salvar configurações MQTT
    SERVIDOR_WEB_ASYNC->on("/api/mqtt/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        bool configChanged = false;
        
        if (request->hasArg("mqtt_enabled")) {
            vSt_mainConfig.vB_mqttEnabled = request->arg("mqtt_enabled") == "true";
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_server")) {
            vSt_mainConfig.vS_mqttServer = request->arg("mqtt_server");
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_port")) {
            vSt_mainConfig.vU16_mqttPort = request->arg("mqtt_port").toInt();
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_user")) {
            vSt_mainConfig.vS_mqttUser = request->arg("mqtt_user");
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_password")) {
            vSt_mainConfig.vS_mqttPassword = request->arg("mqtt_password");
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_topic_base")) {
            vSt_mainConfig.vS_mqttTopicBase = request->arg("mqtt_topic_base");
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_publish_interval")) {
            vSt_mainConfig.vU16_mqttPublishInterval = request->arg("mqtt_publish_interval").toInt();
            configChanged = true;
        }
        
        if (request->hasArg("mqtt_ha_discovery")) {
            vSt_mainConfig.vB_mqttHomeAssistantDiscovery = request->arg("mqtt_ha_discovery") == "true";
            configChanged = true;
        }
        if (request->hasArg("mqtt_ha_batch")) {
            int v = request->arg("mqtt_ha_batch").toInt();
            if (v < 1) v = 1; if (v > 16) v = 16;
            vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize = (uint8_t)v;
            configChanged = true;
        }
        if (request->hasArg("mqtt_ha_interval_ms")) {
            int v = request->arg("mqtt_ha_interval_ms").toInt();
            if (v < 10) v = 10; if (v > 5000) v = 5000;
            vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs = (uint16_t)v;
            configChanged = true;
        }
        
        if (configChanged) {
            fV_salvarMainConfig();
            fV_printSerialDebug(LOG_WEB, "[MQTT] Configurações salvas com sucesso");
            request->send(200, "application/json", "{\"success\": true, \"message\": \"Configurações MQTT salvas. Reinicie o ESP para aplicar.\"}");
        } else {
            request->send(400, "application/json", "{\"success\": false, \"message\": \"Nenhuma configuração foi alterada\"}");
        }
    });
    
    // API: Obter configurações MQTT atuais (somente dados de configuração - leve e rápido)
    SERVIDOR_WEB_ASYNC->on("/api/mqtt/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        String json = "{";
        json += "\"mqtt_enabled\":" + String(vSt_mainConfig.vB_mqttEnabled ? "true" : "false") + ",";
        json += "\"mqtt_server\":\"" + vSt_mainConfig.vS_mqttServer + "\",";
        json += "\"mqtt_port\":" + String(vSt_mainConfig.vU16_mqttPort) + ",";
        json += "\"mqtt_user\":\"" + vSt_mainConfig.vS_mqttUser + "\",";
        json += "\"mqtt_password\":\"" + vSt_mainConfig.vS_mqttPassword + "\",";
        json += "\"mqtt_topic_base\":\"" + vSt_mainConfig.vS_mqttTopicBase + "\",";
        json += "\"mqtt_publish_interval\":" + String(vSt_mainConfig.vU16_mqttPublishInterval) + ",";
        json += "\"mqtt_ha_discovery\":" + String(vSt_mainConfig.vB_mqttHomeAssistantDiscovery ? "true" : "false") + ",";
        json += "\"mqtt_ha_batch\":" + String(vSt_mainConfig.vU8_mqttHaDiscoveryBatchSize) + ",";
        json += "\"mqtt_ha_interval_ms\":" + String(vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs);
        json += "}";
        
        request->send(200, "application/json", json);
    });

    // API: Status MQTT (separado para não travar carregamento da página)
    SERVIDOR_WEB_ASYNC->on("/api/mqtt/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }

        String json = "{";
        json += "\"mqtt_unique_id\":\"" + fS_getMqttUniqueId() + "\",";
        json += "\"mqtt_status\":\"" + fS_getMqttStatus() + "\"";
        json += "}";

        request->send(200, "application/json", json);
    });
    
    // API: Listar preferências NVS
    SERVIDOR_WEB_ASYNC->on("/api/nvs/list", HTTP_GET, fV_handleNVSList);
    // API: Exportar preferências NVS (download JSON)
    SERVIDOR_WEB_ASYNC->on("/api/nvs/export", HTTP_GET, fV_handleNVSExport);
    // API: Importar preferências NVS (upload JSON)
    SERVIDOR_WEB_ASYNC->on("/api/nvs/import", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Handler final: valida presença do corpo
            if (request->_tempObject == nullptr) {
                request->send(400, "application/json", "{\"success\": false, \"message\": \"Arquivo JSON não recebido\"}");
                return;
            }
            fV_handleNVSImport(request);
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Acumula body (JSON) em memória
            if (index == 0) {
                request->_tempObject = new String();
            }
            String *body = (String*)request->_tempObject;
            for (size_t i = 0; i < len; i++) {
                *body += (char)data[i];
            }
        }
    );
    
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
        doc["show_analog_history"] = vSt_mainConfig.vB_showAnalogHistory;
        doc["show_digital_history"] = vSt_mainConfig.vB_showDigitalHistory;
        
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
    g_webServerReady = true;
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
    
    auto sanitizeHostname = [](const String &in) -> String {
        String out;
        out.reserve(in.length());
        for (size_t i = 0; i < in.length(); ++i) {
            char c = in.charAt(i);
            // Converte letras minúsculas para maiúsculas
            if (c >= 'a' && c <= 'z') c = c - ('a' - 'A');
            // Mantém apenas A-Z e 0-9 (remove espaços, acentos UTF-8 e especiais)
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                out += c;
            }
            // Demais bytes (ex.: parte de UTF-8) são descartados
        }
        return out;
    };

    if (isInitialConfig) {
        // Configuração inicial (modo AP)
        if (request->hasArg("hostname")) vSt_mainConfig.vS_hostname = sanitizeHostname(request->arg("hostname"));
        if (request->hasArg("ssid")) vSt_mainConfig.vS_wifiSsid = request->arg("ssid");
        if (request->hasArg("password")) vSt_mainConfig.vS_wifiPass = request->arg("password");
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes iniciais recebidas via POST.");
        
    } else if (isAdvancedConfig) {
        // Configurações avançadas
        fV_printSerialDebug(LOG_WEB, "[CONFIG] Processando configurações avançadas...");
        
        if (request->hasArg("hostname")) {
            vSt_mainConfig.vS_hostname = sanitizeHostname(request->arg("hostname"));
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
        
        // Histórico
        vSt_mainConfig.vB_showAnalogHistory = request->hasArg("show_analog_history") && request->arg("show_analog_history") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] show_analog_history = %d", vSt_mainConfig.vB_showAnalogHistory);
        
        vSt_mainConfig.vB_showDigitalHistory = request->hasArg("show_digital_history") && request->arg("show_digital_history") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] show_digital_history = %d", vSt_mainConfig.vB_showDigitalHistory);
        
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
    // PRIMEIRO: Verifica se é DELETE para /api/actions/{pin}/{num}
    String url = request->url();
    if (request->method() == HTTP_DELETE && url.startsWith("/api/actions/")) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        // Tenta parsear a URL
        int lastSlash = url.lastIndexOf('/');
        int secondLastSlash = url.lastIndexOf('/', lastSlash - 1);
        
        if (lastSlash > secondLastSlash && secondLastSlash > 12) { // 12 = tamanho de "/api/actions/"
            String pinStr = url.substring(secondLastSlash + 1, lastSlash);
            String numStr = url.substring(lastSlash + 1);
            
            // Valida se são números
            bool validPin = true;
            bool validNum = true;
            
            for (unsigned int i = 0; i < pinStr.length(); i++) {
                if (!isDigit(pinStr.charAt(i))) validPin = false;
            }
            for (unsigned int i = 0; i < numStr.length(); i++) {
                if (!isDigit(numStr.charAt(i))) validNum = false;
            }
            
            if (validPin && validNum) {
                int pin = pinStr.toInt();
                int num = numStr.toInt();
                
                if (num >= 1 && num <= 4 && pin <= 254) {
                    fV_handleActionDeleteApi(request);
                    return;
                }
            }
        }
    }
    
    // SEGUNDO: 404 padrão
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
    // Versão do firmware
    system_obj["fw_version"] = FIRMWARE_VERSION;
    
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
                
                // Adiciona histórico para pinos analógicos (se habilitado)
                if (vA_pinConfigs[i].tipo == 192 && vSt_mainConfig.vB_showAnalogHistory) { // PIN_TYPE_ANALOG
                    JsonArray history_array = pin_obj["historico"].to<JsonArray>();
                    
                    // Monta o histórico na ordem correta (mais antigo para mais novo)
                    uint8_t count = vA_pinConfigs[i].historico_count;
                    if (count > 0) {
                        uint8_t startIndex = (vA_pinConfigs[i].historico_index + 8 - count) % 8;
                        for (uint8_t h = 0; h < count; h++) {
                            uint8_t idx = (startIndex + h) % 8;
                            history_array.add(vA_pinConfigs[i].historico_analogico[idx]);
                        }
                    }
                }
                
                // Adiciona histórico para pinos digitais (se habilitado) - ENTRADA ou SAÍDA
                if (vA_pinConfigs[i].tipo == 1 && vSt_mainConfig.vB_showDigitalHistory) { // PIN_TYPE_DIGITAL
                    JsonArray history_array = pin_obj["historico"].to<JsonArray>();
                    
                    // Monta o histórico na ordem correta (mais antigo para mais novo)
                    uint8_t count = vA_pinConfigs[i].historico_count;
                    if (count > 0) {
                        uint8_t startIndex = (vA_pinConfigs[i].historico_index + 8 - count) % 8;
                        for (uint8_t h = 0; h < count; h++) {
                            uint8_t idx = (startIndex + h) % 8;
                            history_array.add(vA_pinConfigs[i].historico_digital[idx]);
                        }
                    }
                }
            }
        }
    }
    
    // Adiciona cores do sistema para o dashboard
    system_obj["cor_com_alerta"] = vSt_mainConfig.vS_corStatusComAlerta;
    system_obj["cor_sem_alerta"] = vSt_mainConfig.vS_corStatusSemAlerta;

    // 3. Sincronizacao (Agrupados em "sync")
    JsonObject sync_obj = vL_jsonDoc["sync"].to<JsonObject>();
    sync_obj["ntp_status"] = vL_ntp_ok ? "Sincronizado" : "Desabilitado";
    sync_obj["mqtt_status"] = fS_getMqttStatus(); 
    
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
    
    servePageWithFallback(request, "/web_config_gerais.html", web_config_gerais_html);
    
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
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /mqtt - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    servePageWithFallback(request, "/web_mqtt.html", web_mqtt_html);
    
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /mqtt - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}
// Handler para página de Inter-Módulos
void fV_handleInterModPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /intermod - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    servePageWithFallback(request, "/web_intermod.html", WEB_HTML_INTERMOD);
    
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /intermod - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de reset
void fV_handleResetPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /reset - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    servePageWithFallback(request, "/web_reset.html", web_reset_html);
    
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /reset - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de arquivos
void fV_handleFilesPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /arquivos - %s (millis: %lu)", timeStr.c_str(), startTime);
    
    servePageWithFallback(request, "/web_files.html", web_files_html);
    
    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /arquivos - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de firmware (OTA)
void fV_handleFirmwarePage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /firmware - %s (millis: %lu)", timeStr.c_str(), startTime);

    servePageWithFallback(request, "/web_firmware.html", web_firmware_html);

    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /firmware - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de preferências NVS
void fV_handlePreferenciasPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /preferencias - %s (millis: %lu)", timeStr.c_str(), startTime);

    servePageWithFallback(request, "/web_preferencias.html", web_preferencias_html);

    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /preferencias - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler para página de LittleFS
void fV_handleLittleFSPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /littlefs - %s (millis: %lu)", timeStr.c_str(), startTime);

    servePageWithFallback(request, "/web_littlefs.html", web_littlefs_html);

    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /littlefs - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler: API - Obter logs do Web Serial
void fV_handleSerialLogs(AsyncWebServerRequest *request) {
    uint32_t since = 0;
    if (request->hasParam("since")) {
        since = request->getParam("since")->value().toInt();
    }
    
    JsonDocument doc;
    JsonArray logs = doc["logs"].to<JsonArray>();
    doc["counter"] = g_serialLogCounter;
    
    // Se since == 0, retorna todas as linhas do buffer
    // Se since > 0, retorna apenas linhas novas
    if (since == 0) {
        // Retorna todo o buffer (até 100 linhas)
        for (uint16_t i = 0; i < SERIAL_LOG_BUFFER_SIZE; i++) {
            uint16_t idx = (g_serialLogIndex + i) % SERIAL_LOG_BUFFER_SIZE;
            if (!g_serialLogBuffer[idx].isEmpty()) {
                logs.add(g_serialLogBuffer[idx]);
            }
        }
    } else {
        // Retorna apenas novas (desde o contador informado)
        uint32_t diffCount = g_serialLogCounter - since;
        if (diffCount > SERIAL_LOG_BUFFER_SIZE) diffCount = SERIAL_LOG_BUFFER_SIZE;
        
        for (uint32_t i = 0; i < diffCount; i++) {
            uint16_t idx = (g_serialLogIndex - diffCount + i + SERIAL_LOG_BUFFER_SIZE) % SERIAL_LOG_BUFFER_SIZE;
            if (!g_serialLogBuffer[idx].isEmpty()) {
                logs.add(g_serialLogBuffer[idx]);
            }
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// Handler para página Web Serial
void fV_handleSerialPage(AsyncWebServerRequest *request) {
    unsigned long startTime = millis();
    String timeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] INICIO carregamento /serial - %s (millis: %lu)", timeStr.c_str(), startTime);

    servePageWithFallback(request, "/web_serial.html", web_serial_html);

    unsigned long endTime = millis();
    String endTimeStr = fS_getFormattedTime();
    fV_printSerialDebug(LOG_WEB, "[PERFORMANCE] FIM carregamento /serial - %s (millis: %lu, duracao: %lu ms)", endTimeStr.c_str(), endTime, endTime - startTime);
}

// Handler: API - Listar preferências NVS
void fV_handleNVSList(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Listando preferências NVS (app namespaces)");

    JsonDocument doc;
    JsonArray preferences = doc["preferences"].to<JsonArray>();

    // Namespaces do aplicativo; padrão: apenas estes
    const char* appNamespaces[] = {"smcrconf", "smcrgenc"};
    const size_t appNsCount = sizeof(appNamespaces) / sizeof(appNamespaces[0]);
    bool includeSystem = request->hasParam("includeSystem") && request->getParam("includeSystem")->value() == "1";

    // Itera sobre TODAS as entradas do NVS (todas as namespaces) na partição padrão "nvs"
    nvs_iterator_t it = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY);
    size_t count = 0;
    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info); // preenche info com namespace, key e type

        // Filtra namespaces: inclui apenas os do app, a menos que includeSystem=1
        bool isAppNs = false;
        for (size_t i = 0; i < appNsCount; i++) {
            if (String(info.namespace_name) == appNamespaces[i]) { isAppNs = true; break; }
        }
        if (!isAppNs && !includeSystem) { it = nvs_entry_next(it); continue; }

        // Abre a namespace em modo leitura
        nvs_handle_t handle;
        esp_err_t err = nvs_open(info.namespace_name, NVS_READONLY, &handle);
        if (err == ESP_OK) {
            String typeStr = "";
            String valueStr = "";

            // Defende dados sensíveis
            auto isSensitive = [](const char* key) -> bool {
                String k = String(key);
                k.toLowerCase();
                return (k.indexOf("pass") >= 0) || (k.indexOf("senha") >= 0) || (k.indexOf("token") >= 0) || (k.indexOf("secret") >= 0);
            };

            switch (info.type) {
                case NVS_TYPE_U8: {
                    typeStr = "U8";
                    uint8_t v; if (nvs_get_u8(handle, info.key, &v) == ESP_OK) valueStr = String(v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_I8: {
                    typeStr = "I8";
                    int8_t v; if (nvs_get_i8(handle, info.key, &v) == ESP_OK) valueStr = String(v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_U16: {
                    typeStr = "U16";
                    uint16_t v; if (nvs_get_u16(handle, info.key, &v) == ESP_OK) valueStr = String(v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_I16: {
                    typeStr = "I16";
                    int16_t v; if (nvs_get_i16(handle, info.key, &v) == ESP_OK) valueStr = String(v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_U32: {
                    typeStr = "U32";
                    uint32_t v; if (nvs_get_u32(handle, info.key, &v) == ESP_OK) valueStr = String(v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_I32: {
                    typeStr = "I32";
                    int32_t v; if (nvs_get_i32(handle, info.key, &v) == ESP_OK) valueStr = String(v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_U64: {
                    typeStr = "U64";
                    uint64_t v; if (nvs_get_u64(handle, info.key, &v) == ESP_OK) valueStr = String((unsigned long long)v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_I64: {
                    typeStr = "I64";
                    int64_t v; if (nvs_get_i64(handle, info.key, &v) == ESP_OK) valueStr = String((long long)v); else valueStr = "<erro>";
                    break;
                }
                case NVS_TYPE_STR: {
                    typeStr = "String";
                    size_t len = 0;
                    if (nvs_get_str(handle, info.key, NULL, &len) == ESP_OK && len > 0) {
                        std::unique_ptr<char[]> buf(new char[len]);
                        if (nvs_get_str(handle, info.key, buf.get(), &len) == ESP_OK) {
                            if (isSensitive(info.key)) valueStr = "[OCULTO]"; else valueStr = String(buf.get());
                        } else valueStr = "<erro>";
                    } else {
                        valueStr = "";
                    }
                    break;
                }
                case NVS_TYPE_BLOB: {
                    typeStr = "BLOB";
                    size_t len = 0;
                    if (nvs_get_blob(handle, info.key, NULL, &len) == ESP_OK) {
                        valueStr = String("[BLOB ") + String(len) + String(" bytes]");
                    } else valueStr = "<erro>";
                    break;
                }
                default: {
                    typeStr = "UNKNOWN";
                    valueStr = "";
                    break;
                }
            }

            nvs_close(handle);

            JsonObject obj = preferences.add<JsonObject>();
            // Colocamos namespace no próprio campo 'key' para facilitar exibição na UI atual
            String fullKey = String(info.namespace_name) + ":" + String(info.key);
            obj["key"] = fullKey;
            obj["type"] = typeStr;
            obj["value"] = valueStr;
            obj["namespace"] = String(info.namespace_name);

            count++;
        }

        it = nvs_entry_next(it);
    }
    if (it) nvs_release_iterator(it);

    doc["count"] = (uint32_t)count;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);

    fV_printSerialDebug(LOG_WEB, "[API] Lista NVS enviada (%u entradas)", (unsigned)count);
}

// Handler: API - Exportar preferências NVS como arquivo JSON (attachment)
void fV_handleNVSExport(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Exportando preferências NVS (JSON)");

    // Parâmetros opcionais: format=(json|text) e include_secrets=1
    String formatParam = request->hasParam("format") ? request->getParam("format")->value() : "json";
    bool includeSecrets = request->hasParam("include_secrets") && request->getParam("include_secrets")->value() != "0";
    bool includeSystem = request->hasParam("includeSystem") && request->getParam("includeSystem")->value() == "1";

    const char* appNamespaces[] = {"smcrconf", "smcrgenc"};
    const size_t appNsCount = sizeof(appNamespaces) / sizeof(appNamespaces[0]);

    // Função de máscara de segredos
    auto isSensitive = [](const char* key) -> bool {
        String k = String(key);
        k.toLowerCase();
        return (k.indexOf("pass") >= 0) || (k.indexOf("senha") >= 0) || (k.indexOf("token") >= 0) || (k.indexOf("secret") >= 0);
    };

    size_t count = 0;

    if (formatParam == "text") {
        // Formato texto: uma linha por entrada: namespace:key [type]=value
        String textOut;
        textOut.reserve(2048);

        nvs_iterator_t it = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY);
        while (it != NULL) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);

            bool isAppNs = false;
            for (size_t i = 0; i < appNsCount; i++) {
                if (String(info.namespace_name) == appNamespaces[i]) { isAppNs = true; break; }
            }
            if (!isAppNs && !includeSystem) { it = nvs_entry_next(it); continue; }

            nvs_handle_t handle;
            if (nvs_open(info.namespace_name, NVS_READONLY, &handle) == ESP_OK) {
                String typeStr = "";
                String valueStr = "";

                switch (info.type) {
                    case NVS_TYPE_U8: { typeStr = "U8"; uint8_t v; valueStr = (nvs_get_u8(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                    case NVS_TYPE_I8: { typeStr = "I8"; int8_t v; valueStr = (nvs_get_i8(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                    case NVS_TYPE_U16:{ typeStr = "U16"; uint16_t v; valueStr = (nvs_get_u16(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                    case NVS_TYPE_I16:{ typeStr = "I16"; int16_t v; valueStr = (nvs_get_i16(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                    case NVS_TYPE_U32:{ typeStr = "U32"; uint32_t v; valueStr = (nvs_get_u32(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                    case NVS_TYPE_I32:{ typeStr = "I32"; int32_t v; valueStr = (nvs_get_i32(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                    case NVS_TYPE_U64:{ typeStr = "U64"; uint64_t v; valueStr = (nvs_get_u64(handle, info.key, &v) == ESP_OK) ? String((unsigned long long)v) : "<erro>"; break; }
                    case NVS_TYPE_I64:{ typeStr = "I64"; int64_t v; valueStr = (nvs_get_i64(handle, info.key, &v) == ESP_OK) ? String((long long)v) : "<erro>"; break; }
                    case NVS_TYPE_STR: {
                        typeStr = "String";
                        size_t len = 0;
                        if (nvs_get_str(handle, info.key, NULL, &len) == ESP_OK && len > 0) {
                            std::unique_ptr<char[]> buf(new char[len]);
                            if (nvs_get_str(handle, info.key, buf.get(), &len) == ESP_OK) {
                                valueStr = (isSensitive(info.key) && !includeSecrets) ? "[OCULTO]" : String(buf.get());
                            } else valueStr = "<erro>";
                        } else {
                            valueStr = "";
                        }
                        break;
                    }
                    case NVS_TYPE_BLOB: {
                        typeStr = "BLOB";
                        size_t len = 0;
                        if (nvs_get_blob(handle, info.key, NULL, &len) == ESP_OK) {
                            valueStr = String("[BLOB ") + String(len) + String(" bytes]");
                        } else valueStr = "<erro>";
                        break;
                    }
                    default: { typeStr = "UNKNOWN"; valueStr = ""; break; }
                }
                nvs_close(handle);

                textOut += String(info.namespace_name) + ":" + String(info.key) + " [" + typeStr + "]=" + valueStr + "\n";
                count++;
            }

            it = nvs_entry_next(it);
        }
        if (it) nvs_release_iterator(it);

        String fname = "nvs_export_" + vSt_mainConfig.vS_hostname + "_" + String(millis()) + ".txt";
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", textOut);
        response->addHeader("Content-Disposition", String("attachment; filename=\"") + fname + String("\""));
        request->send(response);
        fV_printSerialDebug(LOG_WEB, "[API] Export NVS (texto) enviado (%u entradas)", (unsigned)count);
        return;
    }

    // Formato JSON (padrão)
    JsonDocument doc;
    JsonArray preferences = doc["preferences"].to<JsonArray>();

    nvs_iterator_t it = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY);
    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        bool isAppNs = false;
        for (size_t i = 0; i < appNsCount; i++) {
            if (String(info.namespace_name) == appNamespaces[i]) { isAppNs = true; break; }
        }
        if (!isAppNs && !includeSystem) { it = nvs_entry_next(it); continue; }

        nvs_handle_t handle;
        esp_err_t err = nvs_open(info.namespace_name, NVS_READONLY, &handle);
        if (err == ESP_OK) {
            String typeStr = "";
            String valueStr = "";

            switch (info.type) {
                case NVS_TYPE_U8: { typeStr = "U8"; uint8_t v; valueStr = (nvs_get_u8(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                case NVS_TYPE_I8: { typeStr = "I8"; int8_t v; valueStr = (nvs_get_i8(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                case NVS_TYPE_U16:{ typeStr = "U16"; uint16_t v; valueStr = (nvs_get_u16(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                case NVS_TYPE_I16:{ typeStr = "I16"; int16_t v; valueStr = (nvs_get_i16(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                case NVS_TYPE_U32:{ typeStr = "U32"; uint32_t v; valueStr = (nvs_get_u32(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                case NVS_TYPE_I32:{ typeStr = "I32"; int32_t v; valueStr = (nvs_get_i32(handle, info.key, &v) == ESP_OK) ? String(v) : "<erro>"; break; }
                case NVS_TYPE_U64:{ typeStr = "U64"; uint64_t v; valueStr = (nvs_get_u64(handle, info.key, &v) == ESP_OK) ? String((unsigned long long)v) : "<erro>"; break; }
                case NVS_TYPE_I64:{ typeStr = "I64"; int64_t v; valueStr = (nvs_get_i64(handle, info.key, &v) == ESP_OK) ? String((long long)v) : "<erro>"; break; }
                case NVS_TYPE_STR: {
                    typeStr = "String";
                    size_t len = 0;
                    if (nvs_get_str(handle, info.key, NULL, &len) == ESP_OK && len > 0) {
                        std::unique_ptr<char[]> buf(new char[len]);
                        if (nvs_get_str(handle, info.key, buf.get(), &len) == ESP_OK) {
                            valueStr = (isSensitive(info.key) && !includeSecrets) ? "[OCULTO]" : String(buf.get());
                        } else valueStr = "<erro>";
                    } else {
                        valueStr = "";
                    }
                    break;
                }
                case NVS_TYPE_BLOB: {
                    typeStr = "BLOB";
                    size_t len = 0;
                    if (nvs_get_blob(handle, info.key, NULL, &len) == ESP_OK) {
                        valueStr = String("[BLOB ") + String(len) + String(" bytes]");
                    } else valueStr = "<erro>";
                    break;
                }
                default: { typeStr = "UNKNOWN"; valueStr = ""; break; }
            }

            nvs_close(handle);

            JsonObject obj = preferences.add<JsonObject>();
            obj["namespace"] = String(info.namespace_name);
            obj["key"] = String(info.key);
            obj["type"] = typeStr;
            obj["value"] = valueStr;

            count++;
        }

        it = nvs_entry_next(it);
    }
    if (it) nvs_release_iterator(it);

    doc["count"] = (uint32_t)count;

    String payload;
    serializeJson(doc, payload);

    String fname = "nvs_export_" + vSt_mainConfig.vS_hostname + "_" + String(millis()) + ".json";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", payload);
    response->addHeader("Content-Disposition", String("attachment; filename=\"") + fname + String("\""));
    request->send(response);

    fV_printSerialDebug(LOG_WEB, "[API] Export NVS (JSON) enviado (%u entradas)", (unsigned)count);
}

// Handler: API - Importar preferências NVS a partir de JSON
void fV_handleNVSImport(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Importando preferências NVS (JSON)");

    // Recupera o corpo acumulado
    String body = *((String*)request->_tempObject);
    // Libera o ponteiro temporário
    delete (String*)request->_tempObject;
    request->_tempObject = nullptr;

    if (body.length() == 0) {
        request->send(400, "application/json", "{\"success\": false, \"message\": \"JSON vazio\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        fV_printSerialDebug(LOG_WEB, "[API] Import NVS - erro JSON: %s", err.c_str());
        request->send(400, "application/json", "{\"success\": false, \"message\": \"JSON inválido\"}");
        return;
    }

    if (!doc["preferences"].is<JsonArray>()) {
        request->send(400, "application/json", "{\"success\": false, \"message\": \"Estrutura inválida: falta 'preferences'\"}");
        return;
    }

    JsonArray arr = doc["preferences"].as<JsonArray>();
    size_t okCount = 0, skipCount = 0, errCount = 0, clearedNsCount = 0;

    // Verifica se deve apagar namespaces antes de importar
    bool clearBefore = request->hasParam("clear") && request->getParam("clear")->value() != "0";
    bool includeSystem = request->hasParam("includeSystem") && request->getParam("includeSystem")->value() == "1";
    const char* appNamespaces[] = {"smcrconf", "smcrgenc"};
    const size_t appNsCount = sizeof(appNamespaces) / sizeof(appNamespaces[0]);
    if (clearBefore) {
        // Monta lista única de namespaces presentes no JSON e apaga todos os pares de cada um
        String seen; // formato |ns1|ns2| para evitar duplicados com busca por substring
        for (JsonObject obj : arr) {
            String ns = obj["namespace"] | "";
            if (ns.isEmpty()) continue;
            String token = String("|") + ns + String("|");
            if (seen.indexOf(token) >= 0) continue; // já processado

            // Filtra namespaces: apenas app por padrão
            bool isAppNs = false;
            for (size_t i = 0; i < appNsCount; i++) {
                if (ns == appNamespaces[i]) { isAppNs = true; break; }
            }
            if (!isAppNs && !includeSystem) { seen += token; continue; }

            nvs_handle_t h;
            if (nvs_open(ns.c_str(), NVS_READWRITE, &h) == ESP_OK) {
                if (nvs_erase_all(h) == ESP_OK && nvs_commit(h) == ESP_OK) {
                    clearedNsCount++;
                } else {
                    errCount++;
                }
                nvs_close(h);
            } else {
                errCount++;
            }
            seen += token;
        }
        fV_printSerialDebug(LOG_WEB, "[API] Import NVS - namespaces apagados: %u", (unsigned)clearedNsCount);
    }

    // Auxiliar para detectar segredos mascarados
    auto isMasked = [](const String &v) -> bool {
        return v == "[OCULTO]";
    };

    for (JsonObject obj : arr) {
        String ns = obj["namespace"] | "";
        String key = obj["key"] | "";
        String type = obj["type"] | "";
        // value pode vir como string ou número; convertemos para String primeiro
        String value;
        if (obj["value"].is<const char*>()) value = String(obj["value"].as<const char*>());
        else if (obj["value"].is<long long>()) value = String((long long)obj["value"].as<long long>());
        else if (obj["value"].is<unsigned long long>()) value = String((unsigned long long)obj["value"].as<unsigned long long>());
        else if (obj["value"].is<int>()) value = String(obj["value"].as<int>());
        else if (obj["value"].is<unsigned int>()) value = String(obj["value"].as<unsigned int>());
        else value = String("");

        if (ns.isEmpty() || key.isEmpty() || type.isEmpty()) { skipCount++; continue; }
        if (isMasked(value)) { skipCount++; continue; }
        if (type == "BLOB" || type == "UNKNOWN") { skipCount++; continue; }

        // Filtra namespaces na importação: apenas app por padrão
        bool isAppNs = false;
        for (size_t i = 0; i < appNsCount; i++) {
            if (ns == appNamespaces[i]) { isAppNs = true; break; }
        }
        if (!isAppNs && !includeSystem) { skipCount++; continue; }

        nvs_handle_t handle;
        esp_err_t e = nvs_open(ns.c_str(), NVS_READWRITE, &handle);
        if (e != ESP_OK) { errCount++; continue; }

        bool wrote = false;
        // Converte inteiros 64 bits de forma segura
        auto toU64 = [](const String &s) -> uint64_t { return strtoull(s.c_str(), NULL, 10); };
        auto toI64 = [](const String &s) -> int64_t { return strtoll(s.c_str(), NULL, 10); };

        if (type == "U8") { uint8_t v = (uint8_t) value.toInt(); wrote = (nvs_set_u8(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "I8") { int8_t v = (int8_t) value.toInt(); wrote = (nvs_set_i8(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "U16") { uint16_t v = (uint16_t) value.toInt(); wrote = (nvs_set_u16(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "I16") { int16_t v = (int16_t) value.toInt(); wrote = (nvs_set_i16(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "U32") { uint32_t v = (uint32_t) value.toInt(); wrote = (nvs_set_u32(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "I32") { int32_t v = (int32_t) value.toInt(); wrote = (nvs_set_i32(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "U64") { uint64_t v = toU64(value); wrote = (nvs_set_u64(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "I64") { int64_t v = toI64(value); wrote = (nvs_set_i64(handle, key.c_str(), v) == ESP_OK); }
        else if (type == "String") { wrote = (nvs_set_str(handle, key.c_str(), value.c_str()) == ESP_OK); }
        else { /* tipo não suportado */ }

        if (wrote) {
            if (nvs_commit(handle) == ESP_OK) okCount++; else errCount++;
        } else {
            errCount++;
        }
        nvs_close(handle);
    }

    char res[128];
    snprintf(res, sizeof(res), "{\"success\": true, \"message\": \"Import concluido\", \"ok\": %u, \"skipped\": %u, \"errors\": %u, \"cleared\": %u}", (unsigned)okCount, (unsigned)skipCount, (unsigned)errCount, (unsigned)clearedNsCount);
    request->send(200, "application/json", String(res));

    fV_printSerialDebug(LOG_WEB, "[API] Import NVS: ok=%u, skip=%u, err=%u, cleared=%u", (unsigned)okCount, (unsigned)skipCount, (unsigned)errCount, (unsigned)clearedNsCount);
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
    
    // *** VALIDAÇÃO: Verifica se pino está em uso por ações ***
    if (fB_isPinUsedByActions(pinNumber)) {
        fV_printSerialDebug(LOG_WEB, "[API] ERRO: Pino %d não pode ser excluído - está em uso por ações", pinNumber);
        request->send(409, "application/json", 
            "{\"error\": \"Pino está em uso por ações\", "
            "\"message\": \"Exclua todas as ações relacionadas a este pino antes de removê-lo\"}");
        return;
    }
    
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

//========================================
// Handlers da API de Ações
//========================================

// API: Listar todas as ações configuradas
void fV_handleActionsListApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Listando ações configuradas");
    
    JsonDocument doc;
    JsonArray actionsArray = doc["actions"].to<JsonArray>();
    
    for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
        if (vA_actionConfigs[i].acao != 0) {
            JsonObject actionObj = actionsArray.add<JsonObject>();
            actionObj["pino_origem"] = vA_actionConfigs[i].pino_origem;
            actionObj["numero_acao"] = vA_actionConfigs[i].numero_acao;
            actionObj["pino_destino"] = vA_actionConfigs[i].pino_destino;
            actionObj["acao"] = vA_actionConfigs[i].acao;
            actionObj["tempo_on"] = vA_actionConfigs[i].tempo_on;
            actionObj["tempo_off"] = vA_actionConfigs[i].tempo_off;
            actionObj["pino_remoto"] = vA_actionConfigs[i].pino_remoto;
            actionObj["envia_modulo"] = vA_actionConfigs[i].envia_modulo;
            actionObj["telegram"] = vA_actionConfigs[i].telegram;
            actionObj["assistente"] = vA_actionConfigs[i].assistente;
            actionObj["mqtt"] = vA_actionConfigs[i].mqtt;
            actionObj["classe_mqtt"] = vA_actionConfigs[i].classe_mqtt;
            actionObj["icone_mqtt"] = vA_actionConfigs[i].icone_mqtt;
        }
    }
    
    doc["total"] = vU8_activeActionsCount;
    
    String response;
    serializeJson(doc, response);
    
    request->send(200, "application/json", response);
}

// API: Criar nova ação
void fV_handleActionCreateApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Criando nova ação");
    
    // Validação de parâmetros obrigatórios
    if (!request->hasArg("pino_origem") || !request->hasArg("numero_acao") || 
        !request->hasArg("pino_destino") || !request->hasArg("acao")) {
        request->send(400, "application/json", "{\"error\": \"Parâmetros obrigatórios ausentes\"}");
        return;
    }
    
    ActionConfig_t newAction;
    newAction.pino_origem = request->arg("pino_origem").toInt();
    newAction.numero_acao = request->arg("numero_acao").toInt();
    newAction.pino_destino = request->arg("pino_destino").toInt();
    newAction.acao = request->arg("acao").toInt();
    newAction.tempo_on = request->hasArg("tempo_on") ? request->arg("tempo_on").toInt() : 0;
    newAction.tempo_off = request->hasArg("tempo_off") ? request->arg("tempo_off").toInt() : 0;
    newAction.pino_remoto = request->hasArg("pino_remoto") ? request->arg("pino_remoto").toInt() : 0;
    newAction.envia_modulo = request->hasArg("envia_modulo") ? request->arg("envia_modulo").toInt() : 0;
    newAction.telegram = request->hasArg("telegram") && request->arg("telegram") == "true";
    newAction.assistente = request->hasArg("assistente") && request->arg("assistente") == "true";
    newAction.mqtt = request->hasArg("mqtt") && request->arg("mqtt") == "true";
    newAction.classe_mqtt = request->hasArg("classe_mqtt") ? request->arg("classe_mqtt") : "";
    newAction.icone_mqtt = request->hasArg("icone_mqtt") ? request->arg("icone_mqtt") : "";
    
    // Detecta modo de edição (campos hidden)
    bool isEdit = request->hasArg("edit_pino_origem") && request->hasArg("edit_numero_acao") &&
                  request->arg("edit_pino_origem") != "" && request->arg("edit_numero_acao") != "";
    
    if (isEdit) {
        // Modo edição: usa os valores originais dos campos hidden
        uint8_t originalPinOrigem = request->arg("edit_pino_origem").toInt();
        uint8_t originalNumeroAcao = request->arg("edit_numero_acao").toInt();
        
        // Remove a ação antiga
        fB_removeActionConfig(originalPinOrigem, originalNumeroAcao);
        fV_printSerialDebug(LOG_WEB, "[API] Editando ação: Origem=%d, Ação#%d", originalPinOrigem, originalNumeroAcao);
    }
    
    int result = fI_addActionConfig(newAction);
    
    if (result >= 0) {
        fB_saveActionConfigs(); // Salva automaticamente
        fV_printSerialDebug(LOG_WEB, "[API] Ação %s com sucesso (índice %d)", isEdit ? "editada" : "adicionada", result);
        request->send(200, "application/json", "{\"success\": true, \"message\": \"Ação salva\"}");
    } else {
        fV_printSerialDebug(LOG_WEB, "[API] ERRO: Falha ao adicionar ação");
        request->send(400, "application/json", "{\"error\": \"Limite de ações atingido ou ação já existe\"}");
    }
}

// API: Deletar ação
void fV_handleActionDeleteApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Deletando ação");
    
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    int secondLastSlash = url.lastIndexOf('/', lastSlash - 1);
    
    if (lastSlash == -1 || secondLastSlash == -1) {
        request->send(400, "application/json", "{\"error\": \"URL inválida\"}");
        return;
    }
    
    uint8_t pinOrigem = url.substring(secondLastSlash + 1, lastSlash).toInt();
    uint8_t numeroAcao = url.substring(lastSlash + 1).toInt();
    
    bool success = fB_removeActionConfig(pinOrigem, numeroAcao);
    if (!success) {
        request->send(404, "application/json", "{\"error\": \"Ação não encontrada\"}");
        return;
    }
    
    fB_saveActionConfigs(); // Salva automaticamente
    request->send(200, "application/json", "{\"success\": true, \"message\": \"Ação removida\"}");
}

// API: Salvar configurações de ações na flash
void fV_handleActionsSaveApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Salvando configurações de ações na flash");
    
    bool success = fB_saveActionConfigs();
    
    if (success) {
        request->send(200, "application/json", "{\"success\": true, \"message\": \"Configurações de ações salvas na flash\"}");
    } else {
        fV_printSerialDebug(LOG_WEB, "[API] ERRO: Falha ao salvar configurações de ações");
        request->send(500, "application/json", "{\"success\": false, \"error\": \"Falha ao salvar configurações na flash\"}");
    }
}

// Adiciona linha ao buffer circular de logs do Web Serial
void fV_addSerialLogLine(const String &line) {
    if (!g_webServerReady) return;
    
    g_serialLogBuffer[g_serialLogIndex] = line;
    g_serialLogIndex = (g_serialLogIndex + 1) % SERIAL_LOG_BUFFER_SIZE;
    g_serialLogCounter++;
}

// Página de cadastro de ações
void fV_handleActionsPage(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[WEB] Servindo página de ações");
    servePageWithFallback(request, "/web_actions.html", WEBPAGE_ACTIONS);
}


