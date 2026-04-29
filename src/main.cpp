// Conteúdo de main.cpp

#include "globals.h" // Inclui tudo o que precisamos


// Inicialização de Variáveis globais
bool vB_wifiIsConnected = false;

// Mutex e task FreeRTOS para leitura de pinos e execução de ações (Core 1)
SemaphoreHandle_t vO_pinActionMutex = nullptr;
TaskHandle_t vO_pinActionTaskHandle = nullptr;

// Variáveis de controle de tempo para tasks periódicas
unsigned long vU32_lastMqttLoopTime = 0;
const unsigned long vU32_mqttLoopInterval = 450; // Loop MQTT a cada 50ms (não bloqueante)

unsigned long vU32_lastInterModTaskTime = 0;
const unsigned long vU32_interModTaskInterval = 15000; // Tasks inter-módulos a cada 15 segundos

unsigned long vU32_lastAlertFlashTime = 0;
const unsigned long vU32_alertFlashInterval = 50; // Polling de flash a cada 50ms (cada módulo gerencia seu próprio intervalo)

unsigned long vU32_lastCloudSyncTime = 0;
unsigned long vU32_lastHeartbeatTime = 0;
unsigned long vU32_lastRemoteQueueTime = 0;
const unsigned long vU32_remoteQueueInterval = 5000; // Reenvio da fila a cada 5s

// Definição do objeto Preferences.
Preferences preferences;

// Definição da instância global da sua configuração em memória
MainConfig_t vSt_mainConfig;
AsyncWebServer* SERVIDOR_WEB_ASYNC = nullptr;

//========================================
// Task FreeRTOS: leitura de pinos + execução de ações (Core 1, 100ms)
// Isolada do loop principal para não ser afetada por chamadas HTTP bloqueantes
//========================================
void fV_pinActionTask(void* pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t vU8_pinReadCycle = 0;

    fV_printSerialDebug(LOG_INIT, "[TASK] Task pinos/acoes iniciada no Core %d", xPortGetCoreID());

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Leitura de pinos a cada 500ms (5 ciclos de 100ms)
            vU8_pinReadCycle++;
            if (vU8_pinReadCycle >= 5) {
                vU8_pinReadCycle = 0;
                fV_readPinsTask();
            }
            // Execução de ações a cada 100ms
            fV_executeActionsTask();
            xSemaphoreGive(vO_pinActionMutex);
        } else {
            fV_printSerialDebug(LOG_ACTIONS, "[TASK] Mutex nao obtido, ciclo ignorado");
        }

        // Processa envios HTTP/HTTPS pendentes FORA do mutex (evita bloquear a task)
        fV_processPendingRemoteSends();
        fV_processPendingTelegramSend();
    }
}

void setup() {
  // 0. Cria mutex antes de qualquer inicialização que acesse arrays compartilhados
  vO_pinActionMutex = xSemaphoreCreateMutex();

  // 0b. Inicializa LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("ERRO: Falha ao montar LittleFS");
  }
  
  // 1. Carregar a MainConfig_t (sua startup-config) da Flash para a RAM (running-config)
  fV_carregarMainConfig();

  // 1b. Ajusta frequência do clock do ESP32 (antes do Serial.begin para não afetar baud rate)
  setCpuFrequencyMhz(vSt_mainConfig.vU16_clockEsp32Mhz);

  // 1c. Inicializa o Task Watchdog Timer se habilitado
  if (vSt_mainConfig.vB_watchdogEnabled) {
    uint32_t vU32_wdtTimeoutSec = vSt_mainConfig.vU32_tempoWatchdogUs / 1000000UL;
    if (vU32_wdtTimeoutSec < 1) vU32_wdtTimeoutSec = 8;
    esp_task_wdt_init(vU32_wdtTimeoutSec, true);
    esp_task_wdt_add(NULL);
  }

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
  if (vSt_mainConfig.vB_watchdogEnabled) {
    uint32_t vU32_wdtSec = vSt_mainConfig.vU32_tempoWatchdogUs / 1000000UL;
    if (vU32_wdtSec < 1) vU32_wdtSec = 8;
    fV_printSerialDebug(LOG_WATCHDOG, "Watchdog habilitado: timeout=%us, clock=%uMHz", vU32_wdtSec, vSt_mainConfig.vU16_clockEsp32Mhz);
  } else {
    fV_printSerialDebug(LOG_WATCHDOG, "Watchdog desabilitado. Clock=%uMHz", vSt_mainConfig.vU16_clockEsp32Mhz);
  }
  
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
  
  // 8. INICIALIZA SISTEMA MQTT (após WiFi e sistema de pinos)
  fV_initMqtt();
  if (vSt_mainConfig.vB_mqttEnabled && vB_wifiIsConnected) {
    fV_printSerialDebug(LOG_INIT, "Tentando conexão inicial MQTT...");
    fV_setupMqtt();
  }
  
  // 9. INICIALIZA SISTEMA DE INTER-MÓDULOS
  fV_printSerialDebug(LOG_INIT, "Inicializando sistema de inter-módulos...");
  fV_initInterModSystem();
  if (vSt_mainConfig.vB_interModEnabled && vB_wifiIsConnected) {
    fV_printSerialDebug(LOG_INIT, "Inter-módulos habilitado: %d módulo(s) cadastrado(s)", vU8_activeInterModCount);
  }

  // 10. INICIALIZA SISTEMA DE TELEGRAM
  fV_printSerialDebug(LOG_INIT, "Inicializando sistema de notificações Telegram...");
  fV_initTelegram();

  // 11. LEITURA INICIAL DOS PINOS: Garante que todos os pinos tenham estado atualizado ANTES da primeira ação
  fV_printSerialDebug(LOG_INIT, "Realizando leitura inicial de pinos...");
  fV_readPinsTask();
  
  // 12. EXECUÇÃO INICIAL DE AÇÕES: Aplica ações baseadas no estado atual dos pinos
  fV_printSerialDebug(LOG_INIT, "Executando sincronização inicial de ações...");
  fV_executeActionsTask();

  // 13. SINCRONIZAÇÃO DE PINOS REMOTOS: Envia estado atual para módulos remotos após boot
  if (vSt_mainConfig.vB_interModEnabled && vB_wifiIsConnected) {
    fV_printSerialDebug(LOG_INIT, "Aguardando 5 segundos antes de sincronizar pinos remotos...");
    delay(5000); // Aguarda 5 segundos para garantir que módulos remotos estejam prontos
    fV_syncRemotePinsOnBoot();
  }

  fV_printSerialDebug(LOG_INIT, "Hostname: %s", vSt_mainConfig.vS_hostname.c_str());

  // 14. CRIA TASK FREERTOS: leitura de pinos + execução de ações no Core 1
  xTaskCreatePinnedToCore(
    fV_pinActionTask,       // Função da task
    "PinActionTask",        // Nome para debug
    12288,                  // Stack em bytes (12KB) - necessário para chamadas HTTPS (WiFiClientSecure)
    nullptr,                // Parâmetro
    2,                      // Prioridade (acima do loop=1, abaixo do WiFi)
    &vO_pinActionTaskHandle,// Handle
    1                       // Core 1
  );
  fV_printSerialDebug(LOG_INIT, "--- Fim da inicializacao SMCR ---");
}

void loop() {
  // 1. CHECAGEM E RECONEXAO: Mantém a conexao Wi-Fi ativa (intervalo configurável, default 32s)
  fV_checkWifiConnection();

  // 2. Leitura de pinos e execução de ações movidas para fV_pinActionTask (Core 1)
  unsigned long vU32_currentTime = millis();

  // 3. LOOP MQTT: Mantém conexão MQTT ativa e processa mensagens (não bloqueante)
  if (vB_wifiIsConnected && vU32_currentTime - vU32_lastMqttLoopTime >= vU32_mqttLoopInterval) {
    vU32_lastMqttLoopTime = vU32_currentTime;
    fV_mqttLoop();
  }
  
  // 5. TASKS DE INTER-MÓDULOS: Healthcheck e descoberta automática
  if (vB_wifiIsConnected && vU32_currentTime - vU32_lastInterModTaskTime >= vU32_interModTaskInterval) {
    vU32_lastInterModTaskTime = vU32_currentTime;

    // Executa healthcheck se inter-módulos estiver habilitado
    fV_interModHealthCheckTask();

    // Executa descoberta automática se habilitada
    fV_interModDiscoveryTask();
  }

  // 6. LOOP TELEGRAM: Verificação periódica de mensagens
  if (vB_wifiIsConnected) {
    fV_telegramLoop();
  }

  // 7. SINCRONIZAÇÃO MANUAL DE MÓDULO: Agendada pelo botão "Sincronizar" na interface web
  if (vB_pendingModuleSyncRequest) {
    vB_pendingModuleSyncRequest = false;
    fB_requestPinSyncFromModule(vS_pendingModuleSyncId);
  }

  // 8. FLASH DE ALERTA: Pisca pinos associados a módulos offline (a cada 200ms)
  if (vU32_currentTime - vU32_lastAlertFlashTime >= vU32_alertFlashInterval) {
    vU32_lastAlertFlashTime = vU32_currentTime;
    fV_interModAlertFlashTask();
  }

  // 9a. AUTO-REGISTRO: Tenta registrar na cloud quando api_token está vazio
  if (vB_wifiIsConnected &&
      vSt_mainConfig.vS_cloudApiToken.length() == 0 &&
      vSt_mainConfig.vS_cloudRegisterToken.length() > 0 &&
      vU32_currentTime - vU32_lastCloudSyncTime >= 30000UL) {
    fV_cloudAutoRegisterTask();
  }

  // 9. CLOUD SYNC: Busca configurações na cloud SMCR (periódico ou forçado)
  unsigned long vU32_cloudSyncIntervalMs = (unsigned long)vSt_mainConfig.vU16_cloudSyncIntervalMin * 60000UL;
  if ((vB_pendingCloudSync && vB_wifiIsConnected) ||
      (vSt_mainConfig.vB_cloudSyncEnabled && vB_wifiIsConnected &&
       vU32_currentTime - vU32_lastCloudSyncTime >= vU32_cloudSyncIntervalMs)) {
    vB_pendingCloudSync = false;
    vU32_lastCloudSyncTime = vU32_currentTime;
    fV_cloudSyncTask();
  }

  // 10. FETCH CLOUD FILES: Download de arquivos HTML da cloud para LittleFS (setup inicial)
  if (vB_pendingFetchCloudFiles && vB_wifiIsConnected) {
    vB_pendingFetchCloudFiles = false;
    fV_fetchCloudFilesTask();
  }

  // 11. FILA DE REENVIO: Retenta alertas inter-módulos que falharam (a cada 5s)
  if (vB_wifiIsConnected && vU32_currentTime - vU32_lastRemoteQueueTime >= vU32_remoteQueueInterval) {
    vU32_lastRemoteQueueTime = vU32_currentTime;
    if (xSemaphoreTake(vO_pinActionMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      fV_processRemoteQueue();
      xSemaphoreGive(vO_pinActionMutex);
    }
  }

  // 12. CLOUD HEARTBEAT: Envia status periódico para manter dispositivo online no SMCR Cloud HA
  if (vSt_mainConfig.vB_cloudHeartbeatEnabled && vB_wifiIsConnected &&
      vSt_mainConfig.vS_cloudApiToken.length() > 0) {
    unsigned long vU32_hbIntervalMs = (unsigned long)vSt_mainConfig.vU16_cloudHeartbeatIntervalMin * 60000UL;
    if (vU32_lastHeartbeatTime > 0 && vU32_currentTime - vU32_lastHeartbeatTime >= vU32_hbIntervalMs) {
      vU32_lastHeartbeatTime = vU32_currentTime;
      fV_cloudHeartbeatTask();
    } else if (vU32_lastHeartbeatTime == 0) {
      vU32_lastHeartbeatTime = vU32_currentTime; // marca início do intervalo sem bloquear no boot
    }
  }

  // 13. WATCHDOG FEED: Alimenta o Task WDT ao final de cada ciclo completo do loop
  if (vSt_mainConfig.vB_watchdogEnabled) {
    esp_task_wdt_reset();
  }

}