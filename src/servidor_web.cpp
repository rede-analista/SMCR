// Conteúdo de servidor_web.cpp
#include "globals.h"
#include "web_modoap.h"
#include "web_dashboard.h"

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

    // Rota de nao encontrado
    SERVIDOR_WEB_ASYNC->onNotFound(fV_handleNotFound);
    
    // Inicia o servidor
    SERVIDOR_WEB_ASYNC->begin();
    fV_printSerialDebug(LOG_WEB, "Servidor web assincrono iniciado na porta 8080");
}


// Rota Handler: Funcao para salvar a configuracao inicial
void fV_handleSaveConfig(AsyncWebServerRequest *request) {
    // Verifica se os argumentos (campos do formulário) estão presentes no corpo da requisição POST
    if (request->hasArg("hostname") && request->hasArg("ssid") && request->hasArg("password")) {
        
        // 1. Atualiza a MainConfig_t na RAM
        vSt_mainConfig.vS_hostname = request->arg("hostname");
        vSt_mainConfig.vS_wifiSsid = request->arg("ssid");
        vSt_mainConfig.vS_wifiPass = request->arg("password");
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes iniciais recebidas via POST.");
        fV_printSerialDebug(LOG_WEB, "Novo SSID: %s", vSt_mainConfig.vS_wifiSsid.c_str());
        
        // 2. Salva na memoria flash (WDT está desabilitado aqui)
        fV_salvarMainConfig();
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes salvas na flash. Acionando reboot...");

        // 3. Envia resposta 200 (OK) para o JavaScript (Isto tem prioridade no Async)
        request->send(200, "text/plain", "OK");

        // 4. CORREÇÃO DE TIMING: Força o esvaziamento do buffer da Serial
        // e usa um delay para dar tempo ao sistema assíncrono de enviar a resposta HTTP.
        delay(500); // 300ms devem ser suficientes para o servidor assíncrono finalizar a resposta.

        // 5. Reinicia o ESP32 para tentar conectar com as novas credenciais
        ESP.restart();

    } else {
        fV_printSerialDebug(LOG_WEB, "Erro: Parametros do formulario ausentes.");
        request->send(400, "text/plain", "Erro: Parametros ausentes");
    }
}

// Rota Handler: 404 Not Found
void fV_handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "URL nao encontrada");
}