// Conteúdo de utils.cpp

#include "globals.h" // Inclui globals.h que já inclui include.h
#include <stdarg.h>  // Necessário para lidar com argumentos variáveis (...)

/*========================================
Gera um ID para o módulo com base nas informações do chip ESP32
*/
String fS_idModulo() {
  uint64_t macAddress = ESP.getEfuseMac();  // Obtenha o MAC do chip ESP32
  // Converta o MAC em um ID hexadecimal
  String idModulo = String(macAddress, HEX);
  idModulo.toLowerCase();
  return idModulo;
}

/*========================================
Formata e imprime na serial do ESP32
*/
void fV_printSerialDebug(uint32_t vU32_messageFlag, const char *vC_format, ...) {
  // A variável vU32_activeLogFlags agora é membro de vSt_mainConfig
  uint32_t vU32_currentActiveFlags = vSt_mainConfig.vU32_activeLogFlags;

  // Se a flag da mensagem NÃO estiver entre as flags ativas, não imprime.
  if (!((vU32_currentActiveFlags & vU32_messageFlag) || (vU32_messageFlag == LOG_NONE && vU32_currentActiveFlags != LOG_NONE))) {
      return; // Sai da função se a flag não estiver habilitada
  }

  // Tratamento específico se vU32_currentActiveFlags for LOG_NONE (tudo desligado)
  if (vU32_currentActiveFlags == LOG_NONE && vU32_messageFlag != LOG_NONE) {
      return; // Se nenhum log está ativo, e a mensagem tem uma flag, não imprime.
  }
  
  char vC_locBuffer[256];
  va_list vL_args;
  va_start(vL_args, vC_format);
  
  // Declarar vI_charsPrinted e atribuir o retorno de vsnprintf
  int vI_charsPrinted = vsnprintf(vC_locBuffer, sizeof(vC_locBuffer), vC_format, vL_args);
  
  va_end(vL_args);

  // Garante que o buffer esteja nulo-terminado caso a string seja maior que o buffer
  if (vI_charsPrinted >= sizeof(vC_locBuffer)) {
    vC_locBuffer[sizeof(vC_locBuffer) - 1] = '\0';
  }

  // Prefixo com timestamp (NTP) ou millis como fallback
  String vS_prefix;
  struct tm vSt_tm;
  if (getLocalTime(&vSt_tm, 0) && vSt_tm.tm_year > 100) {
    char vC_ts[22];
    strftime(vC_ts, sizeof(vC_ts), "[%d/%m/%Y %H:%M:%S]", &vSt_tm);
    vS_prefix = vC_ts;
  } else {
    vS_prefix = "[t=" + String(millis()) + "ms]";
  }
  if (vU32_messageFlag & LOG_INIT)     vS_prefix += "[INIT]";
  if (vU32_messageFlag & LOG_NETWORK)  vS_prefix += "[NET]";
  if (vU32_messageFlag & LOG_PINS)     vS_prefix += "[PINS]";
  if (vU32_messageFlag & LOG_FLASH)    vS_prefix += "[FLASH]";
  if (vU32_messageFlag & LOG_WEB)      vS_prefix += "[WEB]";
  if (vU32_messageFlag & LOG_SENSOR)   vS_prefix += "[SENSOR]";
  if (vU32_messageFlag & LOG_ACTIONS)  vS_prefix += "[ACTIONS]";
  if (vU32_messageFlag & LOG_INTERMOD) vS_prefix += "[INTERMOD]";
  if (vU32_messageFlag & LOG_WATCHDOG) vS_prefix += "[WDOG]";
  vS_prefix += " ";

  Serial.print(vS_prefix);
  Serial.println(vC_locBuffer);
  
  // Broadcast para buffer do Web Serial
  extern void fV_addSerialLogLine(const String &line);
  String fullLine = vS_prefix + String(vC_locBuffer);
  fV_addSerialLogLine(fullLine);
}

/*========================================
Registra comunicação inter-módulos RECEBIDA (circular buffer)
*/
void fV_logInterModReceived(const String& module, uint16_t pin, uint16_t value) {
  // Obter hora formatada
  extern String fS_getFormattedTime(void);
  String timeStr = fS_getFormattedTime();
  
  // Se NTP não estiver sincronizado, usar uptime
  if (timeStr.startsWith("Aguardando") || timeStr.startsWith("Falha")) {
    extern String fS_formatUptime(unsigned long vL_ms);
    timeStr = fS_formatUptime(millis());
  }
  
  // Adicionar ao buffer circular
  vSt_InterModCommReceived[vU8_InterModCommReceivedIndex].time = timeStr;
  vSt_InterModCommReceived[vU8_InterModCommReceivedIndex].module = module;
  vSt_InterModCommReceived[vU8_InterModCommReceivedIndex].pin = pin;
  vSt_InterModCommReceived[vU8_InterModCommReceivedIndex].value = value;
  vSt_InterModCommReceived[vU8_InterModCommReceivedIndex].is_sent = false;
  
  // Avançar índice (circular)
  vU8_InterModCommReceivedIndex = (vU8_InterModCommReceivedIndex + 1) % MAX_INTERMOD_COMM_LOG;
  
  fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Recebido de %s: pino %d = %d às %s", 
                      module.c_str(), pin, value, timeStr.c_str());
}

/*========================================
Registra comunicação inter-módulos ENVIADA (circular buffer)
*/
void fV_logInterModSent(const String& module, uint16_t pin, uint16_t value) {
  // Obter hora formatada
  extern String fS_getFormattedTime(void);
  String timeStr = fS_getFormattedTime();
  
  // Se NTP não estiver sincronizado, usar uptime
  if (timeStr.startsWith("Aguardando") || timeStr.startsWith("Falha")) {
    extern String fS_formatUptime(unsigned long vL_ms);
    timeStr = fS_formatUptime(millis());
  }
  
  // Adicionar ao buffer circular
  vSt_InterModCommSent[vU8_InterModCommSentIndex].time = timeStr;
  vSt_InterModCommSent[vU8_InterModCommSentIndex].module = module;
  vSt_InterModCommSent[vU8_InterModCommSentIndex].pin = pin;
  vSt_InterModCommSent[vU8_InterModCommSentIndex].value = value;
  vSt_InterModCommSent[vU8_InterModCommSentIndex].is_sent = true;
  
  // Avançar índice (circular)
  vU8_InterModCommSentIndex = (vU8_InterModCommSentIndex + 1) % MAX_INTERMOD_COMM_LOG;
  
  fV_printSerialDebug(LOG_INTERMOD, "[INTERMOD] Enviado para %s: pino %d = %d às %s", 
                      module.c_str(), pin, value, timeStr.c_str());
}
