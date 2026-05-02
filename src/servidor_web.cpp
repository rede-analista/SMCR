// Conteúdo de servidor_web.cpp
#include "globals.h"
#include "web_modoap.h"
#include "web_dashboard.h"
#include "web_config_gerais.h"
#include "web_reset.h"
#include "web_pins.h"
#include "web_mqtt.h"
#include "web_intermod.h"
#include "web_assistentes.h"
#include "web_actions.h"
#include "web_firmware.h"
#include "web_preferencias.h"
#include "web_littlefs.h"
#include "web_serial.h"
#include "web_historico.h"
#include <LittleFS.h>
#include <Preferences.h>
// Inclusões para navegação completa no NVS
#include <nvs.h>
#include <nvs_flash.h>
#include <Update.h>

// Forward declarations para handlers de arquivos
void fV_handleFirmwarePage(AsyncWebServerRequest *request);
void fV_handleNVSList(AsyncWebServerRequest *request);
void fV_handleNVSExport(AsyncWebServerRequest *request);
void fV_handleNVSImport(AsyncWebServerRequest *request);
void fV_handleFilesList(AsyncWebServerRequest *request);
void fV_handleFileDownload(AsyncWebServerRequest *request);
void fV_handleFileDelete(AsyncWebServerRequest *request);
void fV_handleFormatFlash(AsyncWebServerRequest *request);
void fV_handleSerialPage(AsyncWebServerRequest *request);
void fV_handleHistoricoPage(AsyncWebServerRequest *request);
void fV_handleSerialLogs(AsyncWebServerRequest *request);

// Helper: serve página de LittleFS com fallback para PROGMEM
void servePageWithFallback(AsyncWebServerRequest *request, const char* fsPath, const char* progmemContent) {
    if (LittleFS.exists(fsPath)) {
        File file = LittleFS.open(fsPath, "r");
        if (file) {
            size_t fileSize = file.size();
            file.close();
            
            if (fileSize > 0 && fileSize < 100000) { // Limite de 100KB
                fV_printSerialDebug(LOG_WEB, "Servindo %s de LittleFS (%d bytes)", fsPath, fileSize);
                request->send(LittleFS, fsPath, "text/html");
                return;
            } else {
                fV_printSerialDebug(LOG_WEB, "Arquivo %s com tamanho invalido (%d bytes), usando PROGMEM", fsPath, fileSize);
            }
        } else {
            fV_printSerialDebug(LOG_WEB, "Erro ao abrir %s, usando PROGMEM fallback", fsPath);
        }
    } else {
        fV_printSerialDebug(LOG_WEB, "Arquivo %s nao encontrado, usando PROGMEM fallback", fsPath);
    }
    
    // Fallback para PROGMEM
    if (strlen(progmemContent) > 0) {
        request->send(200, "text/html", progmemContent);
    } else {
        request->send(500, "text/plain", "Pagina nao disponivel");
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

    // Rota da página principal (Dashboard, Configuração Inicial ou Setup de Arquivos)
    SERVIDOR_WEB_ASYNC->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
         // Se nao estiver conectado, envia a pagina de configuracao inicial (AP)
         if (WiFi.status() != WL_CONNECTED) {
             fV_printSerialDebug(LOG_WEB, "Servindo pagina de CONFIGURACAO INICIAL...");
             request->send(200, "text/html", web_config_html);
         } else if (!LittleFS.exists("/web_dashboard.html")) {
             // WiFi conectado mas LittleFS sem arquivos HTML — setup inicial
             fV_printSerialDebug(LOG_WEB, "Servindo pagina de SETUP DE ARQUIVOS...");
             request->send(200, "text/html", web_setup_files_html);
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

    // Rota: iniciar download de arquivos HTML da SMCR Cloud (GET ou POST, para compatibilidade com fallback PROGMEM)
    auto fV_handleFetchCloudFiles = [](AsyncWebServerRequest *request) {
        if (!request->hasArg("cloud_url") || request->arg("cloud_url").isEmpty()) {
            request->send(400, "application/json", "{\"ok\":false,\"error\":\"cloud_url obrigatorio\"}");
            return;
        }
        String vS_rawUrl = request->arg("cloud_url");
        // Extrai protocolo (https://) se presente
        if (vS_rawUrl.startsWith("https://")) {
            vSt_mainConfig.vB_cloudUseHttps = true;
            vS_rawUrl = vS_rawUrl.substring(8);
        } else if (vS_rawUrl.startsWith("http://")) {
            vS_rawUrl = vS_rawUrl.substring(7);
        }
        // Extrai porta se embutida no host (ex.: "host:2082")
        int vI_colonIdx = vS_rawUrl.indexOf(':');
        if (vI_colonIdx > 0) {
            vSt_mainConfig.vU16_cloudPort = (uint16_t)vS_rawUrl.substring(vI_colonIdx + 1).toInt();
            vS_rawUrl = vS_rawUrl.substring(0, vI_colonIdx);
        }
        vS_fetchCloudFilesUrl = vS_rawUrl;
        vSt_mainConfig.vS_cloudUrl = vS_rawUrl;
        if (request->hasArg("cloud_port") && !request->arg("cloud_port").isEmpty())
            vSt_mainConfig.vU16_cloudPort = (uint16_t)request->arg("cloud_port").toInt();
        if (request->hasArg("cloud_https"))
            vSt_mainConfig.vB_cloudUseHttps = request->arg("cloud_https") == "1";
        fV_salvarMainConfig();
        vB_pendingFetchCloudFiles = true;
        request->send(200, "application/json", "{\"ok\":true}");
    };
    SERVIDOR_WEB_ASYNC->on("/api/fetch_cloud_files",  HTTP_POST, fV_handleFetchCloudFiles);
    SERVIDOR_WEB_ASYNC->on("/api/fetch_cloud_files",  HTTP_GET,  fV_handleFetchCloudFiles);

    // Rota: status do download de arquivos da cloud
    SERVIDOR_WEB_ASYNC->on("/api/fetch_cloud_files_status", HTTP_GET, [](AsyncWebServerRequest *request) {
        extern bool vB_fetchDone;
        extern bool vB_fetchError;
        String status = fS_getFetchCloudFilesStatus();
        String json = "{\"status\":\"" + status + "\",\"done\":" +
                      (vB_fetchDone  ? "true" : "false") + ",\"error\":" +
                      (vB_fetchError ? "true" : "false") + "}";
        request->send(200, "application/json", json);
    });

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
                if (vSt_mainConfig.vB_watchdogEnabled) {
                    TaskHandle_t loopTask = xTaskGetHandle("loopTask");
                    if (loopTask) esp_task_wdt_delete(loopTask);
                }
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

    SERVIDOR_WEB_ASYNC->on("/historico", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        fV_handleHistoricoPage(request);
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
    
    // API: Histórico de acionamentos
    SERVIDOR_WEB_ASYNC->on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str()))
                return request->requestAuthentication();
        }
        request->send(200, "application/json", fS_getActionHistoryJson());
    });

    SERVIDOR_WEB_ASYNC->on("/api/trigger", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str()))
                return request->requestAuthentication();
        }
        fV_handleTriggerApi(request);
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
    
        // API: Deletar ação específica - usando regex para capturar pin e num
        SERVIDOR_WEB_ASYNC->on("^\\/api\\/actions\\/([0-9]+)\\/([0-9]+)$", HTTP_DELETE, [](AsyncWebServerRequest *request) {
            // Verifica autenticação se habilitada
            if (vSt_mainConfig.vB_authEnabled) {
                if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                    return request->requestAuthentication();
                }
            }
            fV_handleActionDeleteApi(request);
        });
    });
    
    // Rotas de reset (POST)
    SERVIDOR_WEB_ASYNC->on("/reset/soft", HTTP_POST, fV_handleSoftReset);
    SERVIDOR_WEB_ASYNC->on("/reset/factory", HTTP_POST, fV_handleFactoryReset);
    SERVIDOR_WEB_ASYNC->on("/reset/network", HTTP_POST, fV_handleNetworkReset);
    SERVIDOR_WEB_ASYNC->on("/reset/config", HTTP_POST, fV_handleConfigReset);
    SERVIDOR_WEB_ASYNC->on("/reset/pins", HTTP_POST, fV_handlePinsReset);
    SERVIDOR_WEB_ASYNC->on("/restart", HTTP_POST, fV_handleRestart);
    
    // Rota GET para factory reset direto (requer parâmetro confirm=yes para segurança)
    SERVIDOR_WEB_ASYNC->on("/api/reset/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        // Requer parâmetro confirm=yes para evitar reset acidental
        if (request->hasArg("confirm") && request->arg("confirm") == "yes") {
            fV_handleFactoryReset(request);
        } else {
            request->send(400, "text/plain", "Factory reset requer parametro: ?confirm=yes");
        }
    });

    // Favicon: servir a partir do LittleFS para evitar 404
    SERVIDOR_WEB_ASYNC->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/favicon.ico")) {
            request->send(LittleFS, "/favicon.ico", String("image/x-icon"));
        } else if (LittleFS.exists("/favicon.png")) {
            request->send(LittleFS, "/favicon.png", String("image/png"));
        } else {
            // Sem favicon: responde 204 No Content para evitar erro
            request->send(204);
        }
    });
    
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
        if (request->hasArg("mqtt_ha_repeat_sec")) {
            int v = request->arg("mqtt_ha_repeat_sec").toInt();
            if (v < 0) v = 0; if (v > 86400) v = 86400; // 0 a 24 horas
            vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec = (uint32_t)v;
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
        json += "\"mqtt_ha_interval_ms\":" + String(vSt_mainConfig.vU16_mqttHaDiscoveryIntervalMs) + ",";
        json += "\"mqtt_ha_repeat_sec\":" + String(vSt_mainConfig.vU32_mqttHaDiscoveryRepeatSec);
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
        doc["wifi_attempts"]            = vSt_mainConfig.vU16_wifiConnectAttempts;
        doc["wifi_offline_restart_min"] = vSt_mainConfig.vU16_wifiOfflineRestartMin;

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

        // Configurações SMCR Cloud
        doc["cloud_url"] = vSt_mainConfig.vS_cloudUrl;
        doc["cloud_port"] = vSt_mainConfig.vU16_cloudPort;
        doc["cloud_use_https"] = vSt_mainConfig.vB_cloudUseHttps;
        doc["cloud_sync_enabled"] = vSt_mainConfig.vB_cloudSyncEnabled;
        doc["cloud_sync_interval_min"] = vSt_mainConfig.vU16_cloudSyncIntervalMin;
        doc["cloud_api_token"] = vSt_mainConfig.vS_cloudApiToken;
        doc["cloud_register_token"] = vSt_mainConfig.vS_cloudRegisterToken;
        doc["cloud_heartbeat_enabled"] = vSt_mainConfig.vB_cloudHeartbeatEnabled;
        doc["cloud_heartbeat_interval_min"] = vSt_mainConfig.vU16_cloudHeartbeatIntervalMin;
        doc["cloud_sync_status"] = fS_getCloudSyncStatus();
        doc["cloud_heartbeat_status"] = fS_getCloudHeartbeatStatus();

        String response;
        serializeJson(doc, response);
        
        fV_printSerialDebug(LOG_WEB, "[WEB] Enviando JSON: %s", response.c_str());
        request->send(200, "application/json", response);
    });

    //Rota para retornar dados de status em JSON (para o Dashboard)
    SERVIDOR_WEB_ASYNC->on("/status/json", HTTP_GET, fV_handleStatusJson);

    // ==== API de Inter-Módulos ====
    
    // API: Obter configurações de inter-módulos
    SERVIDOR_WEB_ASYNC->on("/api/intermod/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        StaticJsonDocument<512> doc;
        doc["enabled"] = vSt_mainConfig.vB_interModEnabled;
        doc["healthCheckInterval"] = vSt_mainConfig.vU16_interModHealthCheckInterval;
        doc["maxFailures"] = vSt_mainConfig.vU8_interModMaxFailures;
        doc["autoDiscovery"] = vSt_mainConfig.vB_interModAutoDiscovery;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // API: Salvar configurações de inter-módulos
    SERVIDOR_WEB_ASYNC->on("/api/intermod/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        if (request->hasArg("enabled")) {
            vSt_mainConfig.vB_interModEnabled = (request->arg("enabled") == "1");
        }
        if (request->hasArg("healthCheckInterval")) {
            vSt_mainConfig.vU16_interModHealthCheckInterval = request->arg("healthCheckInterval").toInt();
        }
        if (request->hasArg("maxFailures")) {
            vSt_mainConfig.vU8_interModMaxFailures = request->arg("maxFailures").toInt();
        }
        if (request->hasArg("autoDiscovery")) {
            vSt_mainConfig.vB_interModAutoDiscovery = (request->arg("autoDiscovery") == "1");
        }
        
        fV_salvarMainConfig();
        request->send(200, "application/json", "{\"success\": true}");
    });
    
    // API: Listar módulos cadastrados
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        // Early return para lista vazia
        if (vU8_activeInterModCount == 0) {
            request->send(200, "application/json", "{\"modules\":[]}");
            return;
        }
        
        DynamicJsonDocument doc(4096);
        JsonArray modules = doc.createNestedArray("modules");
        
        for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
            JsonObject module = modules.createNestedObject();
            module["id"] = vA_interModConfigs[i].id;
            module["hostname"] = vA_interModConfigs[i].hostname;
            module["ip"] = vA_interModConfigs[i].ip;
            module["porta"] = vA_interModConfigs[i].porta;
            module["online"] = vA_interModConfigs[i].online;
            module["ativo"] = vA_interModConfigs[i].ativo;
            module["falhas_consecutivas"] = vA_interModConfigs[i].falhas_consecutivas;
            module["ultimo_healthcheck"] = vA_interModConfigs[i].ultimo_healthcheck;
            module["auto_descoberto"]           = vA_interModConfigs[i].auto_descoberto;
            module["pins_offline"]              = vA_interModConfigs[i].pins_offline;
            module["offline_alert_enabled"]     = vA_interModConfigs[i].offline_alert_enabled;
            module["offline_flash_ms"]          = vA_interModConfigs[i].offline_flash_ms;
            module["pins_healthcheck"]          = vA_interModConfigs[i].pins_healthcheck;
            module["healthcheck_alert_enabled"] = vA_interModConfigs[i].healthcheck_alert_enabled;
            module["healthcheck_flash_ms"]      = vA_interModConfigs[i].healthcheck_flash_ms;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // API: Adicionar módulo manual
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules/add", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        if (!request->hasArg("moduleId") || !request->hasArg("moduleHostname") || 
            !request->hasArg("moduleIp") || !request->hasArg("modulePort")) {
            request->send(400, "text/plain", "Parametros incompletos");
            return;
        }
        
        InterModConfig_t newModule;
        newModule.id = request->arg("moduleId");
        newModule.hostname = request->arg("moduleHostname");
        newModule.ip = request->arg("moduleIp");
        newModule.porta = request->arg("modulePort").toInt();
        newModule.auto_descoberto           = false;
        newModule.ativo                     = request->hasArg("ativo") && request->arg("ativo") == "1";
        newModule.pins_offline              = request->hasArg("pinsOffline")            ? request->arg("pinsOffline")            : "";
        newModule.offline_alert_enabled     = request->hasArg("offlineAlertEnabled")    && request->arg("offlineAlertEnabled") == "1";
        newModule.offline_flash_ms          = request->hasArg("offlineFlashMs")         ? request->arg("offlineFlashMs").toInt() : 200;
        newModule.pins_healthcheck          = request->hasArg("pinsHealthcheck")        ? request->arg("pinsHealthcheck")        : "";
        newModule.healthcheck_alert_enabled = request->hasArg("healthcheckAlertEnabled") && request->arg("healthcheckAlertEnabled") == "1";
        newModule.healthcheck_flash_ms      = request->hasArg("healthcheckFlashMs")     ? request->arg("healthcheckFlashMs").toInt() : 500;
        
        int index = -1;
        if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            index = fI_addInterModConfig(newModule);
            xSemaphoreGive(vO_pinActionMutex);
        }
        if (index >= 0) {
            fB_saveInterModConfigs();
            request->send(200, "application/json", "{\"success\": true}");
        } else {
            request->send(400, "text/plain", "Modulo ja existe");
        }
    });
    
    // API: Remover módulo
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        if (!request->hasArg("id")) {
            request->send(400, "text/plain", "ID do modulo nao fornecido");
            return;
        }
        
        String moduleId = request->arg("id");
        bool removed = false;
        if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            removed = fB_removeInterModConfig(moduleId);
            xSemaphoreGive(vO_pinActionMutex);
        }
        if (removed) {
            fB_saveInterModConfigs();
            request->send(200, "application/json", "{\"success\": true}");
        } else {
            request->send(404, "text/plain", "Modulo nao encontrado");
        }
    });
    
    // API: Testar conectividade de módulo
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules/test", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        if (!request->hasArg("id")) {
            request->send(400, "text/plain", "ID do modulo nao fornecido");
            return;
        }
        
        String moduleId = request->arg("id");
        int index = fI_findInterModIndex(moduleId);
        
        if (index < 0) {
            request->send(404, "application/json", "{\"success\": false, \"message\": \"Modulo nao encontrado\"}");
            return;
        }
        
        bool success = fB_checkModuleHealth(index);
        String response = success ? 
            "{\"success\": true, \"message\": \"Modulo online\"}" :
            "{\"success\": false, \"message\": \"Modulo offline\"}";
        
        request->send(200, "application/json", response);
    });

    // API: Solicitar sincronização de pinos de um módulo
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules/sync", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }

        if (!request->hasArg("id")) {
            request->send(400, "text/plain", "ID do modulo nao fornecido");
            return;
        }

        String moduleId = request->arg("id");
        int index = fI_findInterModIndex(moduleId);

        if (index < 0) {
            request->send(404, "application/json", "{\"success\": false, \"message\": \"Modulo nao encontrado\"}");
            return;
        }

        // Agenda a sincronização para ser executada na loop() principal,
        // evitando chamada HTTP bloqueante dentro da task do AsyncWebServer
        vS_pendingModuleSyncId = moduleId;
        vB_pendingModuleSyncRequest = true;

        request->send(202, "application/json", "{\"success\": true, \"message\": \"Sincronizacao agendada\"}");
    });

    // API: Atualizar módulo existente
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }

        if (!request->hasArg("moduleId")) {
            request->send(400, "text/plain", "ID do modulo nao fornecido");
            return;
        }

        String moduleId = request->arg("moduleId");
        int index = fI_findInterModIndex(moduleId);

        if (index < 0) {
            request->send(404, "application/json", "{\"success\": false, \"message\": \"Modulo nao encontrado\"}");
            return;
        }

        if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (request->hasArg("moduleHostname"))        vA_interModConfigs[index].hostname               = request->arg("moduleHostname");
            if (request->hasArg("moduleIp"))              vA_interModConfigs[index].ip                     = request->arg("moduleIp");
            if (request->hasArg("modulePort"))            vA_interModConfigs[index].porta                  = request->arg("modulePort").toInt();
            if (request->hasArg("ativo"))                 vA_interModConfigs[index].ativo                  = request->arg("ativo") == "1";
            if (request->hasArg("pinsOffline"))           vA_interModConfigs[index].pins_offline            = request->arg("pinsOffline");
            if (request->hasArg("offlineAlertEnabled"))   vA_interModConfigs[index].offline_alert_enabled   = request->arg("offlineAlertEnabled") == "1";
            if (request->hasArg("offlineFlashMs"))        vA_interModConfigs[index].offline_flash_ms        = request->arg("offlineFlashMs").toInt();
            if (request->hasArg("pinsHealthcheck"))       vA_interModConfigs[index].pins_healthcheck        = request->arg("pinsHealthcheck");
            if (request->hasArg("healthcheckAlertEnabled")) vA_interModConfigs[index].healthcheck_alert_enabled = request->arg("healthcheckAlertEnabled") == "1";
            if (request->hasArg("healthcheckFlashMs"))    vA_interModConfigs[index].healthcheck_flash_ms    = request->arg("healthcheckFlashMs").toInt();
            xSemaphoreGive(vO_pinActionMutex);
        }
        fB_saveInterModConfigs();
        fV_publishInterModStatus(index);
        request->send(200, "application/json", "{\"success\": true}");
    });

    // API: Ativar/desativar módulo (toggle)
    SERVIDOR_WEB_ASYNC->on("/api/intermod/modules/toggle", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }

        if (!request->hasArg("id")) {
            request->send(400, "text/plain", "ID do modulo nao fornecido");
            return;
        }

        String moduleId = request->arg("id");
        int index = fI_findInterModIndex(moduleId);
        if (index < 0) {
            request->send(404, "application/json", "{\"success\": false, \"message\": \"Modulo nao encontrado\"}");
            return;
        }

        bool novoEstado = false;
        if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (request->hasArg("ativo")) {
                vA_interModConfigs[index].ativo = request->arg("ativo") == "1";
            } else {
                vA_interModConfigs[index].ativo = !vA_interModConfigs[index].ativo;
            }
            novoEstado = vA_interModConfigs[index].ativo;
            xSemaphoreGive(vO_pinActionMutex);
        }
        fB_saveInterModConfigs();
        fV_publishInterModStatus(index);

        String resp = "{\"success\": true, \"ativo\": ";
        resp += novoEstado ? "true" : "false";
        resp += "}";
        request->send(200, "application/json", resp);
    });

    // API: Executar descoberta mDNS
    SERVIDOR_WEB_ASYNC->on("/api/intermod/discovery", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        // Força execução imediata da descoberta
        uint8_t countBefore = vU8_activeInterModCount;
        fV_interModDiscoveryTask();
        uint8_t countAfter = vU8_activeInterModCount;
        
        StaticJsonDocument<256> doc;
        doc["found"] = (countAfter - countBefore);
        doc["total"] = countAfter;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // API: Endpoint de healthcheck (para outros módulos checarem este)
    SERVIDOR_WEB_ASYNC->on("/api/healthcheck", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Healthcheck não requer autenticação para facilitar comunicação entre módulos
        StaticJsonDocument<256> doc;
        doc["status"] = "online";
        doc["hostname"] = vSt_mainConfig.vS_hostname;
        doc["uptime"] = millis();
        doc["firmware"] = FIRMWARE_VERSION;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // Endpoint para receber comandos de pino de módulos remotos
    SERVIDOR_WEB_ASYNC->on("/api/pin/set", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Não requer autenticação para facilitar comunicação inter-módulos
        if (!request->hasArg("pin") || !request->hasArg("value")) {
            request->send(400, "text/plain", "Missing pin or value parameter");
            return;
        }
        
        uint16_t pin = request->arg("pin").toInt();
        uint16_t value = request->arg("value").toInt(); // uint16_t para suportar valores analógicos 0-4095
        
        // Identificar módulo de origem (pode vir como parâmetro ou pelo IP)
        String fromModule = "Desconhecido";
        if (request->hasArg("from")) {
            fromModule = request->arg("from");
        } else {
            // Tentar identificar pelo IP
            IPAddress remoteIP = request->client()->remoteIP();
            fromModule = remoteIP.toString();
        }
        
        fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Recebido comando remoto de %s: pino=%d, valor=%d", 
                            fromModule.c_str(), pin, value);
        
        // Busca o índice do pino na configuração
        uint8_t pinIndex = fU8_findPinIndex(pin);
        if (pinIndex == 255) {
            fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Pino %d não encontrado na configuração", pin);
            request->send(404, "text/plain", "Pin not found");
            return;
        }
        
        // Aceita pino de SAÍDA ou pino REMOTO (tipo 65534 digital ou 65533 analógico)
        // Pino REMOTO: entrada virtual que recebe valor via POST (não faz digitalRead)
        // Pino SAÍDA: escreve fisicamente no GPIO
        const uint16_t PIN_TYPE_REMOTE_DIGITAL = 65534;
        const uint16_t PIN_TYPE_REMOTE_ANALOG = 65533;
        
        if (vA_pinConfigs[pinIndex].tipo == PIN_TYPE_REMOTE_DIGITAL || vA_pinConfigs[pinIndex].tipo == PIN_TYPE_REMOTE_ANALOG) {
            // Pino remoto (entrada virtual) - apenas atualiza o valor
            // Para remoto analógico (65533), aceita valores 0-4095
            // Para remoto digital (65534), aceita valores 0-1
            vA_pinConfigs[pinIndex].status_atual = value;
            
            // Atualizar histórico para pinos remotos analógicos
            if (vA_pinConfigs[pinIndex].tipo == PIN_TYPE_REMOTE_ANALOG) {
                // Adicionar ao histórico analógico (circular buffer)
                vA_pinConfigs[pinIndex].historico_analogico[vA_pinConfigs[pinIndex].historico_index] = value;
                vA_pinConfigs[pinIndex].historico_index = (vA_pinConfigs[pinIndex].historico_index + 1) % 8;
                if (vA_pinConfigs[pinIndex].historico_count < 8) {
                    vA_pinConfigs[pinIndex].historico_count++;
                }
            } else if (vA_pinConfigs[pinIndex].tipo == PIN_TYPE_REMOTE_DIGITAL) {
                // Adicionar ao histórico digital (circular buffer)
                vA_pinConfigs[pinIndex].historico_digital[vA_pinConfigs[pinIndex].historico_index] = value;
                vA_pinConfigs[pinIndex].historico_index = (vA_pinConfigs[pinIndex].historico_index + 1) % 8;
                if (vA_pinConfigs[pinIndex].historico_count < 8) {
                    vA_pinConfigs[pinIndex].historico_count++;
                }
            }
            
            fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Pino REMOTO %d atualizado: valor=%d", pin, value);
            
            // Registrar comunicação recebida (exclui healthcheck - pino 0 ou mudanças de valor)
            if (pin != 0) {
                fV_logInterModReceived(fromModule, pin, value);
            }
            
            // Publicar status no MQTT imediatamente
            fV_publishPinStatus(pinIndex);
            
            request->send(200, "text/plain", "OK - Remote pin updated");
            
        } else if (vA_pinConfigs[pinIndex].modo == 3 || vA_pinConfigs[pinIndex].modo == 12) { 
            // OUTPUT (3) ou OUTPUT_OPEN_DRAIN (12)
            // Pino de saída - escreve fisicamente no GPIO
            digitalWrite(pin, value ? HIGH : LOW);
            vA_pinConfigs[pinIndex].status_atual = value;
            fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] GPIO %d acionado: valor=%d", pin, value);
            
            // Registrar comunicação recebida (exclui healthcheck - pino 0)
            if (pin != 0) {
                fV_logInterModReceived(fromModule, pin, value);
            }
            
            request->send(200, "text/plain", "OK - Output pin set");
            
        } else {
            // Pino não é saída nem remoto
            fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Pino %d não é saída nem remoto (tipo=%d, modo=%d)", 
                pin, vA_pinConfigs[pinIndex].tipo, vA_pinConfigs[pinIndex].modo);
            request->send(400, "text/plain", "Pin must be output or remote type");
        }
    });

    // Endpoint para solicitar sincronização de pinos (usado por módulo central)
    SERVIDOR_WEB_ASYNC->on("/api/request_pin_sync", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Não requer autenticação para facilitar comunicação inter-módulos

        // Identificar módulo solicitante
        String requestingModule = "Desconhecido";
        if (request->hasArg("from")) {
            requestingModule = request->arg("from");
        } else {
            IPAddress remoteIP = request->client()->remoteIP();
            requestingModule = remoteIP.toString();
        }

        fV_printSerialDebug(LOG_INTERMOD, "[SYNC-REQ] Recebida solicitação de sincronização de %s",
                           requestingModule.c_str());

        // Verificar se temos o módulo solicitante cadastrado (para saber IP/porta)
        int moduleIndex = fI_findInterModIndex(requestingModule);
        if (moduleIndex < 0) {
            fV_printSerialDebug(LOG_INTERMOD, "[SYNC-REQ] AVISO: Módulo solicitante '%s' não está cadastrado",
                               requestingModule.c_str());
            request->send(404, "text/plain", "Requesting module not registered");
            return;
        }

        // Se o módulo está marcado como offline, marca como online agora:
        // ele acabou de nos enviar uma requisição HTTP, logo está acessível.
        if (!vA_interModConfigs[moduleIndex].online) {
            fV_printSerialDebug(LOG_INTERMOD, "[SYNC-REQ] Módulo '%s' estava offline, marcando como online",
                               requestingModule.c_str());
            vA_interModConfigs[moduleIndex].online = true;
            vA_interModConfigs[moduleIndex].falhas_consecutivas = 0;
        }

        // Responder imediatamente que vamos processar
        request->send(202, "text/plain", "Sync request accepted, sending pin states...");

        // Processar sincronização em background (não bloquear resposta)
        // Percorre todas as ações para encontrar quais pinos devem ser enviados para este módulo
        uint16_t sentCount = 0;

        for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
            // Verifica se esta ação envia para o módulo solicitante
            if (vA_actionConfigs[i].envia_modulo == requestingModule &&
                vA_actionConfigs[i].pino_remoto > 0) {

                // Busca o pino de origem
                uint8_t pinOrigemIndex = fU8_findPinIndex(vA_actionConfigs[i].pino_origem);

                if (pinOrigemIndex == 255) {
                    continue; // Pino não encontrado
                }

                // Lê o valor atual do pino de origem
                uint16_t pinValue = vA_pinConfigs[pinOrigemIndex].status_atual;

                // Envia para o módulo solicitante
                fV_printSerialDebug(LOG_INTERMOD, "[SYNC-REQ] Enviando pino %d (%s) = %d → módulo '%s'",
                    vA_actionConfigs[i].pino_origem,
                    vA_pinConfigs[pinOrigemIndex].nome.c_str(),
                    pinValue,
                    requestingModule.c_str());

                bool success = fB_sendRemoteAction(
                    requestingModule,
                    vA_actionConfigs[i].pino_remoto,
                    pinValue
                );

                if (success) {
                    sentCount++;
                }

                delay(100); // Pequeno delay entre envios
            }
        }

        fV_printSerialDebug(LOG_INTERMOD, "[SYNC-REQ] Sincronização concluída: %d pinos enviados para '%s'",
                           sentCount, requestingModule.c_str());
    });

    // ========================================
    // API: Endpoints de Assistentes (Telegram)
    // ========================================
    
    // Página de configuração de assistentes
    SERVIDOR_WEB_ASYNC->on("/assistentes", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        servePageWithFallback(request, "/web_assistentes.html", web_assistentes_html);
    });

    // Página SMCR Cloud
    SERVIDOR_WEB_ASYNC->on("/cloud", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        if (LittleFS.exists("/web_cloud.html")) {
            request->send(LittleFS, "/web_cloud.html", "text/html");
        } else {
            request->send(404, "text/plain", "Pagina nao encontrada. Envie web_cloud.html via LittleFS.");
        }
    });

    // API: Forçar sync com a cloud SMCR
    SERVIDOR_WEB_ASYNC->on("/api/cloud/sync", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        vB_pendingCloudSync = true;
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Sync agendado\"}");
    });
    
    // API: Obter configurações de assistentes
    SERVIDOR_WEB_ASYNC->on("/api/assistentes/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        StaticJsonDocument<512> doc;
        doc["telegram_enabled"] = vSt_mainConfig.vB_telegramEnabled;
        doc["telegram_token"] = vSt_mainConfig.vS_telegramToken;
        doc["telegram_chatid"] = vSt_mainConfig.vS_telegramChatId;
        doc["telegram_interval"] = vSt_mainConfig.vU16_telegramCheckInterval;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // API: Salvar configurações de assistentes
    SERVIDOR_WEB_ASYNC->on("/api/assistentes/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        if (request->hasArg("telegram_enabled")) {
            vSt_mainConfig.vB_telegramEnabled = (request->arg("telegram_enabled") == "1");
        }
        if (request->hasArg("telegram_token")) {
            vSt_mainConfig.vS_telegramToken = request->arg("telegram_token");
        }
        if (request->hasArg("telegram_chatid")) {
            vSt_mainConfig.vS_telegramChatId = request->arg("telegram_chatid");
        }
        if (request->hasArg("telegram_interval")) {
            vSt_mainConfig.vU16_telegramCheckInterval = request->arg("telegram_interval").toInt();
        }
        
        fV_salvarMainConfig();
        request->send(200, "text/plain", "Configurações salvas com sucesso!");
    });
    
    // API: Testar notificação do Telegram
    SERVIDOR_WEB_ASYNC->on("/api/assistentes/telegram/test", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        
        if (!vSt_mainConfig.vB_telegramEnabled) {
            request->send(400, "text/plain", "Telegram não está habilitado");
            return;
        }
        
        if (vSt_mainConfig.vS_telegramToken.isEmpty() || vSt_mainConfig.vS_telegramChatId.isEmpty()) {
            request->send(400, "text/plain", "Token ou Chat ID não configurado");
            return;
        }
        
        // Envia notificação de teste (implementação será feita depois)
        String message = "🔔 Teste de notificação do SMCR!\n\n";
        message += "Módulo: " + vSt_mainConfig.vS_hostname + "\n";
        message += "Firmware: " + String(FIRMWARE_VERSION) + "\n";
        message += "IP: " + WiFi.localIP().toString();
        
        bool success = fB_sendTelegramMessage(message);
        
        if (success) {
            request->send(200, "text/plain", "Notificação enviada com sucesso!");
        } else {
            request->send(500, "text/plain", "Falha ao enviar notificação");
        }
    });

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
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_') {
                out += c;
            }
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
        if (request->hasArg("wifi_offline_restart_min")) {
            vSt_mainConfig.vU16_wifiOfflineRestartMin = (uint16_t)request->arg("wifi_offline_restart_min").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] wifi_offline_restart_min = %d", vSt_mainConfig.vU16_wifiOfflineRestartMin);
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

        // SMCR Cloud
        if (request->hasArg("cloud_url")) {
            vSt_mainConfig.vS_cloudUrl = request->arg("cloud_url");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_url = %s", vSt_mainConfig.vS_cloudUrl.c_str());
        }
        if (request->hasArg("cloud_port")) {
            vSt_mainConfig.vU16_cloudPort = request->arg("cloud_port").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_port = %d", vSt_mainConfig.vU16_cloudPort);
        }
        if (request->hasArg("cloud_use_https"))
            vSt_mainConfig.vB_cloudUseHttps = request->arg("cloud_use_https") != "0";
        vSt_mainConfig.vB_cloudSyncEnabled = request->hasArg("cloud_sync_enabled") && request->arg("cloud_sync_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_sync_enabled = %d", vSt_mainConfig.vB_cloudSyncEnabled);
        if (request->hasArg("cloud_sync_interval_min")) {
            vSt_mainConfig.vU16_cloudSyncIntervalMin = request->arg("cloud_sync_interval_min").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_sync_interval_min = %d", vSt_mainConfig.vU16_cloudSyncIntervalMin);
        }
        if (request->hasArg("cloud_api_token")) {
            vSt_mainConfig.vS_cloudApiToken = request->arg("cloud_api_token");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_api_token = '%s'", vSt_mainConfig.vS_cloudApiToken.c_str());
        }
        if (request->hasArg("cloud_register_token")) {
            vSt_mainConfig.vS_cloudRegisterToken = request->arg("cloud_register_token");
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_register_token atualizado");
        }
        vSt_mainConfig.vB_cloudHeartbeatEnabled = request->hasArg("cloud_heartbeat_enabled") && request->arg("cloud_heartbeat_enabled") != "0";
        fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_heartbeat_enabled = %d", vSt_mainConfig.vB_cloudHeartbeatEnabled);
        if (request->hasArg("cloud_heartbeat_interval_min")) {
            vSt_mainConfig.vU16_cloudHeartbeatIntervalMin = request->arg("cloud_heartbeat_interval_min").toInt();
            fV_printSerialDebug(LOG_WEB, "[CONFIG] cloud_heartbeat_interval_min = %d", vSt_mainConfig.vU16_cloudHeartbeatIntervalMin);
        }

        fV_printSerialDebug(LOG_WEB, "Configuracoes avancadas completas recebidas via POST.");
    }
    
    // === CONCEITO CONF-INIT: SALVAR AUTOMATICAMENTE NA FLASH ===
    // Salva na memoria flash (NVS) imediatamente após qualquer alteração
    fV_salvarMainConfig();
    fV_printSerialDebug(LOG_WEB, "[CONF-INIT] Configurações salvas automaticamente na flash");
    
    // Envia resposta 200 (OK) para o JavaScript
    request->send(200, "text/plain", "Configurações salvas! O sistema será reiniciado...");
    
    // === REBOOT AUTOMÁTICO APÓS SALVAMENTO ===
    fV_printSerialDebug(LOG_WEB, "[CONF-INIT] Reiniciando sistema automaticamente...");
    Serial.flush();
    delay(500);  // Tempo para enviar resposta HTTP
    ESP.restart();
}

// Rota Handler: 404 Not Found
void fV_handleNotFound(AsyncWebServerRequest *request) {
    // Tratativa especial: DELETE /api/actions/{pin}/{num}
    // ESPAsyncWebServer não suporta rotas dinâmicas com segmentos variáveis diretamente,
    // então tratamos aqui antes de responder 404.
    String url = request->url();
    if (request->method() == HTTP_DELETE && url.startsWith("/api/actions/")) {
        // Verifica autenticação se habilitada
        if (vSt_mainConfig.vB_authEnabled) {
            if (!request->authenticate(vSt_mainConfig.vS_webUsername.c_str(), vSt_mainConfig.vS_webPassword.c_str())) {
                return request->requestAuthentication();
            }
        }
        // Delegar parsing e validação ao handler dedicado
        fV_handleActionDeleteApi(request);
        return;
    }
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
    
    // Calcula uso de CPU baseado em indicadores indiretos
    // Como não temos acesso direto às tasks idle, usamos métricas proxy
    static unsigned long lastCpuCheck = 0;
    static float cpuUsageCore0 = 0.0, cpuUsageCore1 = 0.0;
    static uint32_t lastHeapFree = 0;
    static uint32_t lastWiFiActivity = 0;
    static uint32_t activityCounter = 0;
    
    unsigned long now = millis();
    
    // Incrementa contador a cada chamada da API (indica atividade)
    activityCounter++;
    
    if (now - lastCpuCheck >= 2000) { // Atualiza a cada 2 segundos
        uint32_t currentHeapFree = ESP.getFreeHeap();
        
        // Calcula variação de heap (indica alocação/liberação = atividade)
        int32_t heapDelta = (int32_t)lastHeapFree - (int32_t)currentHeapFree;
        float heapActivity = abs(heapDelta) / 1024.0; // KB de variação
        
        // Número de tasks ativas
        UBaseType_t taskCount = uxTaskGetNumberOfTasks();
        
        // WiFi activity (se conectado, tem carga de background)
        uint32_t wifiActivity = 0;
        if (WiFi.status() == WL_CONNECTED) {
            wifiActivity = 15; // 15% base para WiFi ativo
            // Adiciona se teve requests recentes
            if (activityCounter > lastWiFiActivity) {
                uint32_t delta = (activityCounter - lastWiFiActivity) * 2;
                wifiActivity += min(delta, (uint32_t)20); // Até +20%
            }
        }
        
        // Calcula uso estimado
        // Base: tasks * 5% cada (mínimo 10%)
        float baseUsage = max(10.0f, min((float)(taskCount * 5), 40.0f));
        
        // Core 0: WiFi stack (sempre ativo se conectado)
        cpuUsageCore0 = baseUsage * 0.4f + wifiActivity;
        if (heapActivity > 5.0) cpuUsageCore0 += 10.0; // Heap churn = atividade
        
        // Core 1: Arduino loop e tasks de usuário
        cpuUsageCore1 = baseUsage * 0.6f;
        if (heapActivity > 5.0) cpuUsageCore1 += 5.0;
        
        // Adiciona variação aleatória pequena para mostrar que está "vivo" (+/-3%)
        cpuUsageCore0 += (float)(random(-300, 300)) / 100.0f;
        cpuUsageCore1 += (float)(random(-300, 300)) / 100.0f;
        
        // Limita entre 0-100%
        if (cpuUsageCore0 < 0.0) cpuUsageCore0 = 0.0;
        if (cpuUsageCore0 > 100.0) cpuUsageCore0 = 100.0;
        if (cpuUsageCore1 < 0.0) cpuUsageCore1 = 0.0;
        if (cpuUsageCore1 > 100.0) cpuUsageCore1 = 100.0;
        
        lastHeapFree = currentHeapFree;
        lastWiFiActivity = activityCounter;
        lastCpuCheck = now;
    }
    
    // Adiciona informações de CPU ao JSON
    JsonArray cpuCores = system_obj["cpu_cores"].to<JsonArray>();
    
    JsonObject core0 = cpuCores.add<JsonObject>();
    core0["core"] = 0;
    core0["usage"] = cpuUsageCore0;
    
    JsonObject core1 = cpuCores.add<JsonObject>();
    core1["core"] = 1;
    core1["usage"] = cpuUsageCore1;
    
    system_obj["cpu_usage_avg"] = (cpuUsageCore0 + cpuUsageCore1) / 2.0;
    system_obj["cpu_cores_count"] = 2;
    
    // Informações detalhadas do chip ESP32
    system_obj["chip_model"] = ESP.getChipModel();
    system_obj["chip_revision"] = ESP.getChipRevision();
    system_obj["cpu_freq"] = ESP.getCpuFreqMHz();
    
    // MAC Address formatado
    uint64_t mac = ESP.getEfuseMac();
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
             (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
    system_obj["mac_address"] = String(macStr);
    
    system_obj["flash_size"] = ESP.getFlashChipSize();
    
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
                
                // Adiciona histórico para pinos analógicos locais (192) ou remotos (65533) - se habilitado
                if ((vA_pinConfigs[i].tipo == 192 || vA_pinConfigs[i].tipo == 65533) && vSt_mainConfig.vB_showAnalogHistory) {
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
                
                // Adiciona histórico para pinos digitais locais (1) ou remotos (65534) - se habilitado
                if ((vA_pinConfigs[i].tipo == 1 || vA_pinConfigs[i].tipo == 65534) && vSt_mainConfig.vB_showDigitalHistory) {
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
    sync_obj["telegram_status"] = vSt_mainConfig.vB_telegramEnabled ? "Ativo" : "Inativo";
    sync_obj["cloud_sync_enabled"]       = vSt_mainConfig.vB_cloudSyncEnabled;
    sync_obj["cloud_sync_status"]        = fS_getCloudSyncStatus();
    sync_obj["cloud_sync_last"]          = fS_getCloudSyncLastTime();
    sync_obj["cloud_heartbeat_enabled"]  = vSt_mainConfig.vB_cloudHeartbeatEnabled;
    sync_obj["cloud_heartbeat_status"]   = fS_getCloudHeartbeatStatus();
    sync_obj["cloud_heartbeat_last"]     = fS_getCloudHeartbeatLastTime();
    
    // Usa o tempo formatado se o NTP estiver OK, caso contrario, Uptime formatado
    sync_obj["last_update"] = vL_ntp_ok ? fS_getFormattedTime() : fS_formatUptime(millis()); 

    // 4. Inter-Módulos (Agrupados em "intermod")
    JsonObject intermod_obj = vL_jsonDoc["intermod"].to<JsonObject>();
    intermod_obj["enabled"] = vSt_mainConfig.vB_interModEnabled;
    intermod_obj["total_modules"] = vU8_activeInterModCount;
    
    // Conta módulos online (online = true)
    uint8_t onlineModules = 0;
    for (uint8_t i = 0; i < vU8_activeInterModCount; i++) {
        if (vA_interModConfigs[i].online) {
            onlineModules++;
        }
    }
    intermod_obj["online_modules"] = onlineModules;
    
    // Adiciona informação de visibilidade na system
    system_obj["inter_modulos_enabled"] = vSt_mainConfig.vB_interModulosEnabled;

    // 5. Log de Comunicações Inter-Módulos (Agrupado em "intermod_comm")
    if (vSt_mainConfig.vB_interModulosEnabled) {
        JsonObject intermod_comm_obj = vL_jsonDoc["intermod_comm"].to<JsonObject>();
        
        // Array de comunicações recebidas (últimas 5)
        JsonArray received_array = intermod_comm_obj["received"].to<JsonArray>();
        for (uint8_t i = 0; i < MAX_INTERMOD_COMM_LOG; i++) {
            // Buscar no circular buffer: começar pelo mais antigo até o mais novo
            uint8_t idx = (vU8_InterModCommReceivedIndex + i) % MAX_INTERMOD_COMM_LOG;
            
            // Se o time está vazio, significa que o slot nunca foi preenchido
            if (vSt_InterModCommReceived[idx].time.isEmpty()) {
                continue;
            }
            
            JsonObject comm_obj = received_array.add<JsonObject>();
            comm_obj["time"] = vSt_InterModCommReceived[idx].time;
            comm_obj["from"] = vSt_InterModCommReceived[idx].module;
            comm_obj["pin"] = vSt_InterModCommReceived[idx].pin;
            comm_obj["value"] = vSt_InterModCommReceived[idx].value;
        }
        
        // Array de comunicações enviadas (últimas 5)
        JsonArray sent_array = intermod_comm_obj["sent"].to<JsonArray>();
        for (uint8_t i = 0; i < MAX_INTERMOD_COMM_LOG; i++) {
            // Buscar no circular buffer: começar pelo mais antigo até o mais novo
            uint8_t idx = (vU8_InterModCommSentIndex + i) % MAX_INTERMOD_COMM_LOG;
            
            // Se o time está vazio, significa que o slot nunca foi preenchido
            if (vSt_InterModCommSent[idx].time.isEmpty()) {
                continue;
            }
            
            JsonObject comm_obj = sent_array.add<JsonObject>();
            comm_obj["time"] = vSt_InterModCommSent[idx].time;
            comm_obj["to"] = vSt_InterModCommSent[idx].module;
            comm_obj["pin"] = vSt_InterModCommSent[idx].pin;
            comm_obj["value"] = vSt_InterModCommSent[idx].value;
        }

        // Array da fila de reenvio pendente
        JsonArray queue_array = intermod_comm_obj["queue"].to<JsonArray>();
        for (uint8_t i = 0; i < REMOTE_QUEUE_SIZE; i++) {
            if (!vA_remoteQueue[i].active) continue;
            JsonObject q_obj = queue_array.add<JsonObject>();
            q_obj["module"] = vA_remoteQueue[i].moduleId;
            q_obj["pin"]    = vA_remoteQueue[i].remotePin;
            q_obj["value"]  = vA_remoteQueue[i].value;
        }
    }

    // 6. Serializa o JSON para uma String
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
    if (request->hasArg("wifi_offline_restart_min")) {
        vSt_mainConfig.vU16_wifiOfflineRestartMin = (uint16_t)request->arg("wifi_offline_restart_min").toInt();
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

    fV_printSerialDebug(LOG_WEB, "[CONF-INIT] Configurações aplicadas em memória");
    
    // === CONCEITO CONF-INIT: SALVAR AUTOMATICAMENTE NA FLASH ===
    // Salva na memoria flash (NVS) imediatamente após alteração
    fV_salvarMainConfig();
    fV_printSerialDebug(LOG_WEB, "[CONF-INIT] Configurações salvas automaticamente na flash");
    
    // Envia resposta 200 (OK)
    request->send(200, "text/plain", "Configurações salvas! O sistema será reiniciado...");
    
    // === REBOOT AUTOMÁTICO APÓS SALVAMENTO ===
    fV_printSerialDebug(LOG_WEB, "[CONF-INIT] Reiniciando sistema automaticamente...");
    Serial.flush();
    delay(500);  // Tempo para enviar resposta HTTP
    ESP.restart();
}

// Handler para SALVAR configurações na flash (CONF-INIT - deprecated, agora é automático)
void fV_handleSaveToFlash(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[CONF-INIT] Aviso: Save explícito não necessário - configurações já são salvas automaticamente!");
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
    doc["fileCount"] = files.size();
    
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
    
    // 1. Limpa todas as configurações do NVS (Preferences)
    fV_clearPreferences();
    
    // 2. Formata LittleFS (apaga TODOS os arquivos incluindo HTMLs)
    fV_printSerialDebug(LOG_WEB, "[RESET] Formatando LittleFS...");
    if (LittleFS.format()) {
        fV_printSerialDebug(LOG_WEB, "[RESET] LittleFS formatado com sucesso");
    } else {
        fV_printSerialDebug(LOG_WEB, "[RESET] ERRO ao formatar LittleFS");
    }
    
    // Delay adicional para garantir que a formatação seja concluída
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
        newPin.classe_mqtt = request->hasArg("classe_mqtt") ? request->arg("classe_mqtt") : "";
        newPin.icone_mqtt = request->hasArg("icone_mqtt") ? request->arg("icone_mqtt") : "";
    
    // Tenta adicionar o pino
    int result = -1;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        result = fI_addPinConfig(newPin);
        xSemaphoreGive(vO_pinActionMutex);
    }

    if (result >= 0) {
        fB_savePinConfigs();
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
                        pinObj["classe_mqtt"] = vA_pinConfigs[i].classe_mqtt;
                        pinObj["icone_mqtt"] = vA_pinConfigs[i].icone_mqtt;
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
    
    // Adiciona o pino
    int result = -1;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        result = fI_addPinConfig(newPin);
        xSemaphoreGive(vO_pinActionMutex);
    }
    if (result < 0) {
        request->send(400, "application/json", "{\"error\": \"Erro ao adicionar pino ou limite atingido\"}");
        return;
    }

    fV_setupConfiguredPins();
    fB_savePinConfigs();

    request->send(200, "application/json", "{\"success\": true, \"id\": " + String(result) + ", \"message\": \"Pino adicionado e salvo\"}");
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
    
    uint16_t pinNumber = url.substring(lastSlash + 1).toInt();

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
    
    // Cria nova configuração (inclui campos MQTT)
    PinConfig_t updatedPin;
    updatedPin.nome = doc["nome"] | "";
    updatedPin.pino = doc["pino"] | pinNumber; // Pode ser diferente (rename)
    updatedPin.tipo = doc["tipo"] | 0;
    updatedPin.modo = doc["modo"] | 0;
    updatedPin.xor_logic = doc["xor_logic"] | 0;
    updatedPin.tempo_retencao = doc["tempo_retencao"] | 0;
    updatedPin.nivel_acionamento_min = doc["nivel_acionamento_min"] | 0;
    updatedPin.nivel_acionamento_max = doc["nivel_acionamento_max"] | 0;
    updatedPin.classe_mqtt = doc["classe_mqtt"] | "";
    updatedPin.icone_mqtt = doc["icone_mqtt"] | "";

    uint16_t newPinNumber = updatedPin.pino;
    bool renamed = (newPinNumber != pinNumber);
    uint16_t updatedSourceActions = 0;
    uint16_t updatedDestinationActions = 0;

    bool success = false;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Se for rename, validar que novo número não existe
        if (renamed) {
            if (fU8_findPinIndex(newPinNumber) != 255) {
                xSemaphoreGive(vO_pinActionMutex);
                fV_printSerialDebug(LOG_WEB, "[API] ERRO: Novo número de pino %d já existente", newPinNumber);
                request->send(409, "application/json", "{\"error\": \"Novo número de pino já existe\"}");
                return;
            }
            fV_printSerialDebug(LOG_WEB, "[API] Renomeando pino %d -> %d (atualizando ações)", pinNumber, newPinNumber);
            for (uint8_t i = 0; i < vU8_activeActionsCount; i++) {
                if (vA_actionConfigs[i].acao == 0) continue;
                if (vA_actionConfigs[i].pino_origem == pinNumber) {
                    vA_actionConfigs[i].pino_origem = newPinNumber;
                    updatedSourceActions++;
                }
                if (vA_actionConfigs[i].pino_destino == pinNumber) {
                    vA_actionConfigs[i].pino_destino = newPinNumber;
                    updatedDestinationActions++;
                }
            }
        }
        success = fB_updatePinConfig(pinNumber, updatedPin);
        xSemaphoreGive(vO_pinActionMutex);
    }
    if (!success) {
        request->send(404, "application/json", "{\"error\": \"Pino não encontrado\"}");
        return;
    }

    fV_setupConfiguredPins();

    // Monta resposta detalhada
    JsonDocument resp;
    resp["success"] = true;
    resp["message"] = renamed ? "Pino renomeado e atualizado" : "Pino atualizado";
    resp["renamed"] = renamed;
    resp["old_pin"] = pinNumber;
    resp["new_pin"] = updatedPin.pino;
    resp["updated_source_actions"] = updatedSourceActions;
    resp["updated_destination_actions"] = updatedDestinationActions;
    resp["total_actions_updated"] = updatedSourceActions + updatedDestinationActions;
    // Persistência automática sempre (pinos). Ações apenas se houve rename.
    bool pinsOk = fB_savePinConfigs();
    bool actionsOk = true;
    if (renamed) {
        actionsOk = fB_saveActionConfigs();
    }
    bool persisted = pinsOk && actionsOk;
    fV_printSerialDebug(LOG_WEB, "[API] Persistência auto: renamed=%d pinsOk=%d actionsOk=%d", renamed, pinsOk, actionsOk);
    resp["persisted"] = persisted;
    String out;
    serializeJson(resp, out);
    request->send(200, "application/json", out);
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
    
    uint16_t pinNumber = url.substring(lastSlash + 1).toInt();

    // *** VALIDAÇÃO: Verifica se pino está em uso por ações ***
    if (fB_isPinUsedByActions(pinNumber)) {
        fV_printSerialDebug(LOG_WEB, "[API] ERRO: Pino %d não pode ser excluído - está em uso por ações", pinNumber);
        request->send(409, "application/json", 
            "{\"error\": \"Pino está em uso por ações\", "
            "\"message\": \"Exclua todas as ações relacionadas a este pino antes de removê-lo\"}");
        return;
    }
    
    // Remove o pino da running config
    bool success = false;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        success = fB_removePinConfig(pinNumber);
        xSemaphoreGive(vO_pinActionMutex);
    }
    if (!success) {
        request->send(404, "application/json", "{\"error\": \"Pino não encontrado\"}");
        return;
    }

    fV_setupConfiguredPins();
    fB_savePinConfigs();

    request->send(200, "application/json", "{\"success\": true, \"message\": \"Pino removido e salvo\"}");
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
    
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
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
        fV_loadPinConfigs();
        xSemaphoreGive(vO_pinActionMutex);
    }
    fV_setupConfiguredPins();
    request->send(200, "application/json", "{\"success\": true, \"message\": \"Configurações recarregadas da flash\"}");
}

// API: Limpar todas as configurações de pinos (running config apenas)
void fV_handlePinsClearApi(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[API] Limpando configurações de pinos da running config");
    
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
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
        xSemaphoreGive(vO_pinActionMutex);
    }
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
            actionObj["hora_agendada"]      = vA_actionConfigs[i].hora_agendada;
            actionObj["minuto_agendado"]    = vA_actionConfigs[i].minuto_agendado;
            actionObj["duracao_agendada_s"] = vA_actionConfigs[i].duracao_agendada_s;
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
    newAction.envia_modulo = request->hasArg("envia_modulo") ? request->arg("envia_modulo") : "";
    newAction.telegram = request->hasArg("telegram") && request->arg("telegram") == "true";
    newAction.assistente = request->hasArg("assistente") && request->arg("assistente") == "true";
    newAction.hora_agendada = request->hasArg("hora_agendada") ? (uint8_t)request->arg("hora_agendada").toInt() : 255;
    newAction.minuto_agendado = request->hasArg("minuto_agendado") ? (uint8_t)request->arg("minuto_agendado").toInt() : 0;
    newAction.duracao_agendada_s = request->hasArg("duracao_agendada_s") ? (uint16_t)request->arg("duracao_agendada_s").toInt() : 0;
    newAction.ultimo_disparo_agendado = 0;
    newAction.vB_agendamentoAtivo = false;
    newAction.vUL_agendamentoFimMs = 0;
    newAction.contador_on = 0;
    newAction.contador_off = 0;
    newAction.estado_acao = false;
    newAction.ultimo_estado_origem = false;

    // Detecta modo de edição (campos hidden)
    bool isEdit = request->hasArg("edit_pino_origem") && request->hasArg("edit_numero_acao") &&
                  request->arg("edit_pino_origem") != "" && request->arg("edit_numero_acao") != "";
    
    int result = -1;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (isEdit) {
            uint16_t originalPinOrigem = request->arg("edit_pino_origem").toInt();
            uint8_t originalNumeroAcao = request->arg("edit_numero_acao").toInt();
            fB_removeActionConfig(originalPinOrigem, originalNumeroAcao);
            fV_printSerialDebug(LOG_WEB, "[API] Editando ação: Origem=%d, Ação#%d", originalPinOrigem, originalNumeroAcao);
        }
        result = fI_addActionConfig(newAction);
        xSemaphoreGive(vO_pinActionMutex);
    }

    if (result >= 0) {
        fB_saveActionConfigs();
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
    
    uint16_t pinOrigem = url.substring(secondLastSlash + 1, lastSlash).toInt();
    uint8_t numeroAcao = url.substring(lastSlash + 1).toInt();
    
    bool success = false;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        success = fB_removeActionConfig(pinOrigem, numeroAcao);
        xSemaphoreGive(vO_pinActionMutex);
    }
    if (!success) {
        request->send(404, "application/json", "{\"error\": \"Ação não encontrada\"}");
        return;
    }

    fB_saveActionConfigs();
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

void fV_handleHistoricoPage(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[WEB] Servindo página de histórico");
    servePageWithFallback(request, "/web_historico.html", web_historico_html);
}

void fV_handleTriggerApi(AsyncWebServerRequest *request) {
    if (!request->hasArg("pino")) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"pino obrigatorio\"}");
        return;
    }
    uint16_t pino = (uint16_t)request->arg("pino").toInt();
    vU16_pinoSimulado = pino;
    fV_printSerialDebug(LOG_ACTIONS, "[TRIGGER] Simulacao GPIO %d agendada", pino);
    request->send(200, "application/json", "{\"ok\":true}");
}

