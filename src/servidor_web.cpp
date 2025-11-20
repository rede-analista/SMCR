// Conteúdo de servidor_web.cpp
#include "globals.h"
#include "web_pages.h"

// Funções do Servidor Web
void fV_setupWebServer() {
    fV_printSerialDebug(LOG_WEB, "Configurando o servidor web assincrono...");

    // Criação do servidor web na porta 80 (padrão HTTP)
    if (SERVIDOR_WEB_ASYNC != nullptr) {
        delete SERVIDOR_WEB_ASYNC;
    }
    // A porta 80 é usada aqui, mas deve ser configurável via MainConfig_t em uma versão futura.
    SERVIDOR_WEB_ASYNC = new AsyncWebServer(80); 

    // Rota da página principal (Dashboard ou Configuração Inicial)
    SERVIDOR_WEB_ASYNC->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Se nao estiver conectado, envia a pagina de configuracao inicial
        if (WiFi.status() != WL_CONNECTED) {
            request->send(200, "text/html", web_config_html);
        } else {
            // Placeholder para a pagina principal/dashboard
            request->send(200, "text/html", "<h1>SMCR Dashboard Principal</h1><p>Conectado! IP: " + WiFi.localIP().toString() + "</p><p>Hostname: " + vSt_mainConfig.vS_hostname + "</p>");
        }
    });

    // Rota para salvar a configuracao inicial
    // Chama a funcao fV_handleSaveConfig que atualiza a MainConfig_t e reinicia.
    SERVIDOR_WEB_ASYNC->on("/save_config", HTTP_GET, fV_handleSaveConfig);

    // Rota de nao encontrado
    SERVIDOR_WEB_ASYNC->onNotFound(fV_handleNotFound);
    
    // Inicia o servidor
    SERVIDOR_WEB_ASYNC->begin();
    fV_printSerialDebug(LOG_WEB, "Servidor web assincrono iniciado na porta 80");
}


// Rota Handler: Funcao para salvar a configuracao inicial
void fV_handleSaveConfig(AsyncWebServerRequest *request) {
    if (request->hasArg("hostname") && request->hasArg("ssid") && request->hasArg("password")) {
        // Atualiza a MainConfig_t na RAM com os novos valores
        vSt_mainConfig.vS_hostname = request->arg("hostname");
        vSt_mainConfig.vS_wifiSsid = request->arg("ssid");
        vSt_mainConfig.vS_wifiPass = request->arg("password");
        
        fV_printSerialDebug(LOG_WEB, "Configuracoes iniciais recebidas.");
        fV_printSerialDebug(LOG_WEB, "Novo Hostname: %s", vSt_mainConfig.vS_hostname.c_str());
        fV_printSerialDebug(LOG_WEB, "Novo SSID: %s", vSt_mainConfig.vS_wifiSsid.c_str());
        
        // Salva na memoria flash
        fV_salvarMainConfig();
        
        // Envia resposta 200 (OK) para o JavaScript saber que foi um sucesso
        request->send(200, "text/plain", "OK");

        // Reinicia o ESP32 (necessário para que ele tente conectar com as novas credenciais)
        delay(3000);
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