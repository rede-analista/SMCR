// Conteúdo de main.cpp

#include "globals.h" // Inclui tudo o que precisamos


// Inicialização de Variáveis globais
bool vB_wifiIsConnected = false;

// Variáveis de controle de tempo para tasks periódicas
unsigned long vU32_lastPinReadTime = 0;
const unsigned long vU32_pinReadInterval = 500; // Leitura a cada 500ms

unsigned long vU32_lastActionExecTime = 0;
const unsigned long vU32_actionExecInterval = 100; // Execução de ações a cada 100ms

// Definição do objeto Preferences.
Preferences preferences;

// Definição da instância global da sua configuração em memória
MainConfig_t vSt_mainConfig;
AsyncWebServer* SERVIDOR_WEB_ASYNC = nullptr;

void setup() {

  //fV_clearPreferences(); // Limpa todas as preferências (Reset de Fábrica) --- IGNORE ---

  // 1. Carregar a MainConfig_t (sua startup-config) da Flash para a RAM (running-config)
  fV_carregarMainConfig();

  // 2. Condicionalmente inicializa a Serial e define as flags de log
  // Agora, usa os valores da struct vSt_mainConfig
  if (vSt_mainConfig.vB_serialDebugEnabled) {
    Serial.begin(115200);
    delay(1000); // Dar tempo para o monitor serial conectar
    // fV_printSerialDebug agora usa diretamente vSt_mainConfig.vU32_activeLogFlags
    fV_printSerialDebug(LOG_INIT, "--- Inicializando SMCR ---");
    fV_printSerialDebug(LOG_INIT, "Flags de log ativas: %u", vSt_mainConfig.vU32_activeLogFlags);

  } else {
    // Se o debug serial estiver desabilitado, garante que as flags de log estão desativadas
    // A função fV_printSerialDebug já vai verificar vSt_mainConfig.vU32_activeLogFlags == LOG_NONE
    // Se não houver Serial.begin(), nada será impresso.
    // **Importante:** Se você precisa de Serial.begin() por outros motivos (ex: OTA, que precisa da Serial para mensagens de upload),
    // apenas mantenha Serial.begin(115200) fora deste 'if' e confie que fV_printSerialDebug
    // não imprimirá se vSt_mainConfig.vU32_activeLogFlags for LOG_NONE.
  }

  fV_printSerialDebug(LOG_INIT, "Configuração inicial concluida.");
  
  // 3. INICIA A REDE: Tenta conectar ao Wi-Fi ou ativa o AP de Fallback
  fV_setupWifi();

  // 4. INICIA O NTP: Configura o NTP com base nas configurações carregadas
  fV_setupNtp();

  // 5. INICIA O SERVIDOR WEB: Configura as rotas e inicia o servidor assíncrono
  fV_setupWebServer();  

  // 6. INICIALIZA SISTEMA DE GERENCIAMENTO DE PINOS
  fV_initPinSystem();

  // 7. INICIALIZA SISTEMA DE AÇÕES
  fV_initActionSystem();
  
  // 8. LEITURA INICIAL DOS PINOS: Garante que todos os pinos tenham estado atualizado ANTES da primeira ação
  fV_printSerialDebug(LOG_INIT, "Realizando leitura inicial de pinos...");
  fV_readPinsTask();
  
  // 9. EXECUÇÃO INICIAL DE AÇÕES: Aplica ações baseadas no estado atual dos pinos
  fV_printSerialDebug(LOG_INIT, "Executando sincronização inicial de ações...");
  fV_executeActionsTask();

  fV_printSerialDebug(LOG_INIT, "Hostname: %s", vSt_mainConfig.vS_hostname.c_str());
  fV_printSerialDebug(LOG_INIT, "--- Fim da inicializacao SMCR ---");
  // Use vSt_mainConfig.vS_hostname aqui se precisar
}

void loop() {
  // 1. CHECAGEM E RECONEXAO: Mantém a conexao Wi-Fi ativa (checa a cada 15s)
  fV_checkWifiConnection();
  
  // 2. LEITURA PERIODICA DE PINOS: Atualiza status de pinos físicos (exceto remotos)
  unsigned long vU32_currentTime = millis();
  if (vU32_currentTime - vU32_lastPinReadTime >= vU32_pinReadInterval) {
    vU32_lastPinReadTime = vU32_currentTime;
    fV_readPinsTask();
  }
  
  // 3. EXECUÇÃO DE AÇÕES: Processa ações automáticas (a cada 100ms)
  if (vU32_currentTime - vU32_lastActionExecTime >= vU32_actionExecInterval) {
    vU32_lastActionExecTime = vU32_currentTime;
    fV_executeActionsTask();
  }
  
}