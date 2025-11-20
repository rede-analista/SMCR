// Conteúdo de rede.cpp
#include "globals.h"

// Variável para controle de tempo da checagem (interna a este módulo)
static unsigned long vL_lastCheckTime = 0; 

//=======================================
// FV_SETUP_WIFI: Orquestra a conexão e o fallback
//=======================================
void fV_setupWifi() {
    fV_printSerialDebug(LOG_NETWORK, "Iniciando configuracao de rede...");
    
    // Tenta conectar em modo STA (bloqueando apenas o setup por um tempo limitado)
    fV_connectWifiSta();

    // Atualiza o estado após a tentativa inicial
    vB_wifiIsConnected = (WiFi.status() == WL_CONNECTED);

    // Se a conexão falhar, verifica o fallback
    if (!vB_wifiIsConnected) {
        if (vSt_mainConfig.vB_apFallbackEnabled) {
            fV_printSerialDebug(LOG_NETWORK, "Falha na conexao STA. Ativando Ponto de Acesso (AP) para configuracao.");
            fV_startWifiAp();
        } else {
            fV_printSerialDebug(LOG_NETWORK, "Falha na conexao STA e Fallback AP desabilitado. Reiniciando...");
            delay(5000);
            ESP.restart();
        }
    } else {
        fV_printSerialDebug(LOG_NETWORK, "Conectado com sucesso ao Wi-Fi!");
        fV_printSerialDebug(LOG_NETWORK, "Hostname: %s", vSt_mainConfig.vS_hostname.c_str());
        fV_printSerialDebug(LOG_NETWORK, "Endereco IP: %s", WiFi.localIP().toString().c_str());
    }
}

//=======================================
// FV_CONNECT_WIFI_STA: Tenta conectar como Station (Auxiliar)
//=======================================
void fV_connectWifiSta() {
    if (vSt_mainConfig.vS_wifiSsid.length() == 0) {
        fV_printSerialDebug(LOG_NETWORK, "Nenhuma SSID configurada. Pulando modo STA.");
        return;
    }
    
    // Configura o modo STA se necessário
    if (WiFi.getMode() != WIFI_STA) {
        WiFi.mode(WIFI_STA);
    }
    
    WiFi.setHostname(vSt_mainConfig.vS_hostname.c_str());
    
    // A função begin já é não-bloqueante no ESP32, mas o loop a seguir bloqueia o SETUP
    WiFi.begin(vSt_mainConfig.vS_wifiSsid.c_str(), vSt_mainConfig.vS_wifiPass.c_str());
    fV_printSerialDebug(LOG_NETWORK, "Tentando conectar em modo STA a %s...", vSt_mainConfig.vS_wifiSsid.c_str());
    
    // Lógica de reconexão inicial (limita o bloqueio no setup para vU16_wifiConnectAttempts segundos)
    uint16_t vL_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && vL_attempts < vSt_mainConfig.vU16_wifiConnectAttempts) {
        delay(1000);
        vL_attempts++;
    }
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
            fV_printSerialDebug(LOG_NETWORK, "Evento: Conectado ao Wi-Fi. IP: %s", WiFi.localIP().toString().c_str());
        } else {
            fV_printSerialDebug(LOG_NETWORK, "Evento: Desconectado do Wi-Fi.");
            // Garante que a reconexão comece imediatamente
            vL_lastCheckTime = millis() - vSt_mainConfig.vU32_wifiCheckInterval; 
        }
    }

    // 2. Lógica de Reconexão Periódica
    // Só tenta reconectar se:
    // a) Não estiver conectado
    // b) Estiver em modo STA (para não tentar reconectar se estiver no AP de Fallback)
    // c) E o tempo de intervalo (vU32_wifiCheckInterval) tiver passado
    if (!vB_wifiIsConnected && (WiFi.getMode() == WIFI_STA) && (millis() - vL_lastCheckTime >= vSt_mainConfig.vU32_wifiCheckInterval)) {
        vL_lastCheckTime = millis();
        
        fV_printSerialDebug(LOG_NETWORK, "Tentando reconexao automatica...");
        
        // A função WiFi.reconnect() é não-bloqueante
        WiFi.reconnect();
    }
}