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

    //Rota para retornar dados de status em JSON (para o Dashboard)
    SERVIDOR_WEB_ASYNC->on("/status/json", HTTP_GET, fV_handleStatusJson);    

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
        Serial.flush(); 
        delay(300); // 300ms devem ser suficientes para o servidor assíncrono finalizar a resposta.

        // 5. Reinicia o ESP32 para tentar conectar com as novas credenciais
        ESP.restart();

    } else {
        fV_printSerialDebug(LOG_WEB, "Erro: Parametros do formulario ausentes.");
        request->send(400, "text/plain", "Erro: Parametros ausentes");
    }
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
    // Usamos StaticJsonDocument com um tamanho estimado (aprox. 512 bytes deve ser suficiente para os dados de status).
    StaticJsonDocument<512> vL_jsonDoc; 

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