// Conteúdo de rede.cpp
#include "globals.h"

// Variável para controle de tempo da checagem (interna a este módulo)
static unsigned long vL_lastCheckTime = 0;
static uint16_t vU16_reconnectAttempts = 0; // Contador de tentativas de reconexão

//=======================================
// FV_SETUP_MDNS: Inicializa o mDNS
//=======================================
void fV_setupMdns() {
    if (!vB_wifiIsConnected) {
        fV_printSerialDebug(LOG_NETWORK, "mDNS nao iniciado: WiFi nao conectado");
        return;
    }

    // Inicia o mDNS com o hostname configurado
    if (MDNS.begin(vSt_mainConfig.vS_hostname.c_str())) {
        fV_printSerialDebug(LOG_NETWORK, "mDNS iniciado com sucesso!");
        fV_printSerialDebug(LOG_NETWORK, "Acesse o dispositivo em: http://%s.local:%d/",
                           vSt_mainConfig.vS_hostname.c_str(),
                           vSt_mainConfig.vU16_webServerPort);

        // Adiciona serviço HTTP para descoberta
        MDNS.addService("http", "tcp", vSt_mainConfig.vU16_webServerPort);
        fV_printSerialDebug(LOG_NETWORK, "Servico HTTP mDNS adicionado na porta %d", vSt_mainConfig.vU16_webServerPort);

        // Adiciona informações adicionais do serviço (device_type é usado para identificar módulos SMCR)
        MDNS.addServiceTxt("http", "tcp", "version", FIRMWARE_VERSION);
        MDNS.addServiceTxt("http", "tcp", "device_type", "smcr");
        MDNS.addServiceTxt("http", "tcp", "device", "SMCR");
        fV_printSerialDebug(LOG_NETWORK, "TXT records mDNS adicionados: device_type=smcr, device=SMCR, version=%s", FIRMWARE_VERSION);

        // ESP32 mDNS funciona automaticamente em background, não precisa de update() manual
    } else {
        fV_printSerialDebug(LOG_NETWORK, "Erro ao iniciar mDNS!");
    }
}

//=======================================
// FV_SETUP_WIFI: Orquestra a conexão e o fallback
//=======================================
void fV_setupWifi() {
    fV_printSerialDebug(LOG_NETWORK, "Iniciando configuracao de rede...");
    
    // Tenta conectar em modo STA (AGORA NAO-BLOQUEANTE)
    fV_connectWifiSta();

    // Atualiza o estado após a tentativa inicial
    vB_wifiIsConnected = (WiFi.status() == WL_CONNECTED);

    // Se a conexão falhou, verifica o fallback
    if (!vB_wifiIsConnected) {
        if (vSt_mainConfig.vB_apFallbackEnabled) {
            fV_printSerialDebug(LOG_NETWORK, "Tentativa de conexao em background. Configurando AP se necessario.");
            // O servidor inicia. O loop() ira monitorar e tentar conectar.
            // A ativacao do AP so e feita se o SSID for vazio.
        } else {
            fV_printSerialDebug(LOG_NETWORK, "Falha na conexao STA e Fallback AP desabilitado. Reiniciando...");
            delay(5000);
            ESP.restart();
        }
    } else {
        fV_printSerialDebug(LOG_NETWORK, "Conectado com sucesso ao Wi-Fi!");
        fV_printSerialDebug(LOG_NETWORK, "Hostname: %s", vSt_mainConfig.vS_hostname.c_str());
        fV_printSerialDebug(LOG_NETWORK, "Endereco IP: %s", WiFi.localIP().toString().c_str());
        
        // Inicia o mDNS após conexão bem-sucedida
        fV_setupMdns();
    }
}

//=======================================
// FV_CONNECT_WIFI_STA: Tenta conectar como Station (Auxiliar)
//=======================================
void fV_connectWifiSta() {
    if (vSt_mainConfig.vS_wifiSsid.length() == 0) {
        fV_printSerialDebug(LOG_NETWORK, "Nenhuma SSID configurada. Pulando modo STA.");
        // Se o SSID for vazio, inicia o AP de setup imediatamente (para a primeira execução)
        fV_startWifiAp(); 
        return;
    }
    
    // Configura o modo STA se necessário
    if (WiFi.getMode() != WIFI_STA) {
        WiFi.mode(WIFI_STA);
    }
    
    WiFi.setHostname(vSt_mainConfig.vS_hostname.c_str());
    // Inicia a conexao de forma NAO-BLOQUEANTE
    WiFi.begin(vSt_mainConfig.vS_wifiSsid.c_str(), vSt_mainConfig.vS_wifiPass.c_str());
    fV_printSerialDebug(LOG_NETWORK, "Tentando conectar em modo STA a %s...", vSt_mainConfig.vS_wifiSsid.c_str());
 
}

//=======================================
// FV_START_WIFI_AP: Inicia o Access Point para configuração (Auxiliar)
//=======================================
void fV_startWifiAp() {
    // Configura o modo AP
    WiFi.mode(WIFI_AP);
    
    const char* vC_apSsid = vSt_mainConfig.vS_apSsid.c_str();
    const char* vC_apPass = vSt_mainConfig.vS_apPass.c_str();

    // Requisito: Senha deve ter pelo menos 8 caracteres
    if (vSt_mainConfig.vS_apPass.length() >= 8) {
        WiFi.softAP(vC_apSsid, vC_apPass);
        fV_printSerialDebug(LOG_NETWORK, "AP iniciado. SSID: %s | Senha: %s", vC_apSsid, vC_apPass);
    } else {
        // Isso nao deve ocorrer se o default for 12345678, mas e um bom fallback.
        WiFi.softAP(vC_apSsid);
        fV_printSerialDebug(LOG_NETWORK, "AP iniciado. SSID: %s (SEM SENHA, inseguro!)", vC_apSsid);
    }
    
    IPAddress vL_apIP = WiFi.softAPIP();
    fV_printSerialDebug(LOG_NETWORK, "Endereco IP do AP: %s", vL_apIP.toString().c_str());
}

//=======================================
// FV_CHECK_WIFI_CONNECTION: Checa a conexao e tenta reconectar se desconectado
// Deve ser chamado no loop()
//=======================================
void fV_checkWifiConnection(void) {
    // 1. Atualiza a flag de estado
    bool vL_currentStatus = (WiFi.status() == WL_CONNECTED);
    
    if (vL_currentStatus != vB_wifiIsConnected) {
        vB_wifiIsConnected = vL_currentStatus;
        if (vB_wifiIsConnected) {
            vU16_reconnectAttempts = 0; // Reseta contador ao conectar
            fV_printSerialDebug(LOG_NETWORK, "Evento: Conectado ao Wi-Fi. IP: %s", WiFi.localIP().toString().c_str());
            fV_setupMdns();
        } else {
            fV_printSerialDebug(LOG_NETWORK, "Evento: Desconectado do Wi-Fi.");
            vL_lastCheckTime = millis() - vSt_mainConfig.vU32_wifiCheckInterval;
        }
    }

    // 2. Lógica de Reconexão Periódica
    if (!vB_wifiIsConnected && (WiFi.getMode() == WIFI_STA) && (millis() - vL_lastCheckTime >= vSt_mainConfig.vU32_wifiCheckInterval)) {
        vL_lastCheckTime = millis();
        vU16_reconnectAttempts++;

        fV_printSerialDebug(LOG_NETWORK, "Tentando reconexao automatica... (tentativa %d/%d)",
                            vU16_reconnectAttempts, vSt_mainConfig.vU16_wifiConnectAttempts);

        // Verifica se esgotou as tentativas configuradas
        if (vU16_reconnectAttempts >= vSt_mainConfig.vU16_wifiConnectAttempts) {
            if (vSt_mainConfig.vB_apFallbackEnabled) {
                fV_printSerialDebug(LOG_NETWORK, "Limite de tentativas atingido. Ativando modo AP de fallback.");
                fV_startWifiAp();
                vU16_reconnectAttempts = 0;
            } else {
                fV_printSerialDebug(LOG_NETWORK, "Limite de tentativas atingido. Fallback AP desabilitado. Reiniciando...");
                delay(1000);
                ESP.restart();
            }
            return;
        }

        WiFi.reconnect();
    }
    
    // CORREÇÃO CRÍTICA: Adiciona um pequeno atraso para ceder tempo
    // de execução ao FreeRTOS (scheduler) e ao servidor assíncrono.
    // Isso evita o bloqueio da loopTask.
    delay(10); // Cedemos 10ms
}

//=======================================
// Formata o Uptime de milissegundos para string
//=======================================
String fS_formatUptime(unsigned long vL_ms) {
    // Converte milissegundos para dias, horas, minutos e segundos
    unsigned long vL_segundos = vL_ms / 1000;
    unsigned int vI_dias = vL_segundos / 86400; // 24 * 60 * 60
    vL_segundos %= 86400;
    unsigned int vI_horas = vL_segundos / 3600; // 60 * 60
    vL_segundos %= 3600;
    unsigned int vI_minutos = vL_segundos / 60;
    unsigned int vI_segundos = vL_segundos % 60;

    String vS_uptime = "";
    if (vI_dias > 0) vS_uptime += String(vI_dias) + "d ";
    if (vI_horas > 0 || vI_dias > 0) vS_uptime += String(vI_horas) + "h ";
    if (vI_minutos > 0 || vI_horas > 0 || vI_dias > 0) vS_uptime += String(vI_minutos) + "m ";
    vS_uptime += String(vI_segundos) + "s";
    
    return vS_uptime;
}