/*
 * telegram_manager.cpp
 * 
 * Gerenciamento de notificações via Telegram
 * Envia mensagens quando ações são acionadas
 */

#include "globals.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Cliente WiFi seguro para HTTPS
WiFiClientSecure vO_telegramClient;

// Última verificação de mensagens
unsigned long vUL_lastTelegramCheck = 0;

/**
 * Envia mensagem via Telegram Bot API
 * @param message Texto da mensagem a enviar
 * @return true se enviada com sucesso, false caso contrário
 */
bool fB_sendTelegramMessage(const String& message) {
    if (!vSt_mainConfig.vB_telegramEnabled) {
        fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Notificações desabilitadas");
        return false;
    }
    
    if (vSt_mainConfig.vS_telegramToken.isEmpty() || vSt_mainConfig.vS_telegramChatId.isEmpty()) {
        fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Token ou Chat ID não configurado");
        return false;
    }
    
    if (!vB_wifiIsConnected) {
        fV_printSerialDebug(LOG_WEB, "[TELEGRAM] WiFi desconectado");
        return false;
    }
    
    // Configura cliente HTTPS
    vO_telegramClient.setInsecure(); // Para simplificar, não valida certificado SSL
    
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + vSt_mainConfig.vS_telegramToken + "/sendMessage";
    
    http.begin(vO_telegramClient, url);
    http.addHeader("Content-Type", "application/json");
    
    // Monta payload JSON
    String payload = "{";
    payload += "\"chat_id\":\"" + vSt_mainConfig.vS_telegramChatId + "\",";
    payload += "\"text\":\"" + message + "\",";
    payload += "\"parse_mode\":\"HTML\"";
    payload += "}";
    
    fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Enviando mensagem...");
    
    int httpCode = http.POST(payload);
    bool success = false;
    
    if (httpCode == 200) {
        String response = http.getString();
        fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Mensagem enviada com sucesso!");
        success = true;
    } else {
        fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Erro ao enviar: HTTP %d", httpCode);
        if (httpCode > 0) {
            String response = http.getString();
            fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Resposta: %s", response.c_str());
        }
    }
    
    http.end();
    return success;
}

/**
 * Inicializa o sistema de Telegram
 */
void fV_initTelegram(void) {
    if (!vSt_mainConfig.vB_telegramEnabled) {
        return;
    }
    
    fV_printSerialDebug(LOG_INIT, "[TELEGRAM] Sistema de notificações inicializado");
    fV_printSerialDebug(LOG_INIT, "[TELEGRAM] Chat ID: %s", vSt_mainConfig.vS_telegramChatId.c_str());
    fV_printSerialDebug(LOG_INIT, "[TELEGRAM] Intervalo de verificação: %d segundos", vSt_mainConfig.vU16_telegramCheckInterval);
}

/**
 * Loop do Telegram - verifica mensagens periodicamente
 * Deve ser chamado no loop principal
 */
void fV_telegramLoop(void) {
    if (!vSt_mainConfig.vB_telegramEnabled || !vB_wifiIsConnected) {
        return;
    }
    
    unsigned long now = millis();
    uint32_t checkInterval = vSt_mainConfig.vU16_telegramCheckInterval * 1000;
    
    if (now - vUL_lastTelegramCheck >= checkInterval) {
        vUL_lastTelegramCheck = now;
        
        // Aqui futuramente pode-se implementar verificação de comandos recebidos
        // Por enquanto apenas mantém o loop ativo
        fV_printSerialDebug(LOG_WEB, "[TELEGRAM] Loop ativo - verificação periódica");
    }
}

/**
 * Envia notificação de ação acionada
 * @param pinOrigem Número do pino que disparou a ação
 * @param pinNome Nome do pino origem
 * @param numeroAcao Número da ação (1-4)
 */
void fV_sendTelegramActionNotification(const ActionConfig_t* action, const String& pinOrigemNome, const String& pinDestinoNome) {
    if (!vSt_mainConfig.vB_telegramEnabled) {
        return;
    }
    
    // Determinar texto do tipo de ação
    String tipoAcao;
    switch(action->acao) {
        case 1: tipoAcao = "LIGA"; break;
        case 2: tipoAcao = "LIGA COM DELAY"; break;
        case 3: tipoAcao = "PISCA"; break;
        case 4: tipoAcao = "PULSO"; break;
        case 5: tipoAcao = "PULSO COM DELAY"; break;
        case 65534: tipoAcao = "STATUS"; break;
        case 65535: tipoAcao = "SINCRONISMO"; break;
        default: tipoAcao = "DESCONHECIDO"; break;
    }
    
    String message = "🔔 <b>Notificação SMCR (alerta)</b>\n\n";
    message += "📌 <b>Módulo:</b> " + vSt_mainConfig.vS_hostname + "\n";
    
    // Pino de origem (sensor)
    message += "📍 <b>Pino Origem:</b> " + String(action->pino_origem);
    if (!pinOrigemNome.isEmpty()) {
        message += " (" + pinOrigemNome + ")";
    }
    message += "\n";
    
    // Pino de destino (atuador)
    message += "🎯 <b>Pino Destino:</b> " + String(action->pino_destino);
    if (!pinDestinoNome.isEmpty()) {
        message += " (" + pinDestinoNome + ")";
    }
    message += "\n";
    
    // Informações da ação
    message += "⚡ <b>Ação:</b> #" + String(action->numero_acao) + " - " + tipoAcao + "\n";
    
    // Tempos (se aplicável)
    if (action->acao == 2 || action->acao == 3 || action->acao == 4 || action->acao == 5) {
        if (action->tempo_on > 0) {
            message += "⏱️ <b>Tempo ON:</b> " + String(action->tempo_on) + " ciclos\n";
        }
        if (action->tempo_off > 0 && action->acao == 3) {
            message += "⏱️ <b>Tempo OFF:</b> " + String(action->tempo_off) + " ciclos\n";
        }
    }
    
    // Módulo remoto (se configurado)
    if (action->envia_modulo != "" && action->pino_remoto > 0) {
        message += "🌐 <b>Módulo Remoto:</b> " + action->envia_modulo + " (Pino " + String(action->pino_remoto) + ")\n";
    }
    
    message += "🕐 <b>Horário:</b> " + fS_getFormattedTime();
    
    fB_sendTelegramMessage(message);
}
