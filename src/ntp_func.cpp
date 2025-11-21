// Conteúdo de ntp_func.cpp
#include "globals.h"

//=======================================
// FV_SETUP_NTP: Configura e inicia o servico NTP (NAO-BLOQUEANTE)
//=======================================
void fV_setupNtp() {
    // Acessa as configuracoes salvas (usando a nova estrutura)
    const char* vC_ntpServer1 = vSt_mainConfig.vS_ntpServer1.c_str(); 
    
    // O valor do GMT Offset e o Daylight Offset e lido da configuracao
    const long vL_gmtOffsetSec = vSt_mainConfig.vI_gmtOffsetSec; 
    const int vI_daylightOffsetSec = vSt_mainConfig.vI_daylightOffsetSec;
    
    fV_printSerialDebug(LOG_NETWORK, "Configurando NTP. Servidor principal: %s, GMT: %ld", vC_ntpServer1, vL_gmtOffsetSec);
    
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, ntpServer2)
    // Usamos o servidor 1 da config e o 'pool.ntp.org' como fallback universal
    configTime(vL_gmtOffsetSec, vI_daylightOffsetSec, vC_ntpServer1, "pool.ntp.org");
    
    fV_printSerialDebug(LOG_NETWORK, "Servico NTP iniciado em segundo plano.");
}

//=======================================
// FS_GET_FORMATTED_TIME: Retorna a hora atual formatada ou fallback
//=======================================
String fS_getFormattedTime() {
    struct tm vSt_timeinfo;
    
    // getLocalTime(timeinfo, timeout) com timeout=0 e nao-bloqueante.
    if (!getLocalTime(&vSt_timeinfo, 0)) { 
        // Se a sincronizacao falhou, retorna uma string indicando o UP TIME (fallback)
        return "NTP Nao Sincronizado";
    }

    char vC_timeString[25]; // Buffer para data/hora
    // Formato: DD/MM/AAAA HH:MM:SS
    strftime(vC_timeString, sizeof(vC_timeString), "%d/%m/%Y %H:%M:%S", &vSt_timeinfo);
    return String(vC_timeString);
}

//=======================================
// FS_GET_NTP_STATUS: Retorna o status da sincronizacao NTP (string)
//=======================================
String fS_getNtpStatus() {
    struct tm vSt_timeinfo;
    // Checa se o tempo ja esta valido (sincronizado)
    if (getLocalTime(&vSt_timeinfo, 0)) {
        return "Sincronizado";
    }

    // Se a conexao Wi-Fi estiver ativa, mas nao sincronizou, esta 'aguardando'
    if (WiFi.status() == WL_CONNECTED) {
        return "Aguardando Sincronizacao";
    }
    
    // Se nem o Wi-Fi esta conectado, nao ha como sincronizar
    return "Wi-Fi Desconectado";
}