# SMCR - Sistema Modular de Controle e Recursos

## Architecture Overview

This is an ESP32-based modular sensor/actuator system with web configuration interface. The architecture follows a layered design:

- **Hardware Layer**: ESP32 with configurable GPIO pins (up to 254 pins supported)
- **Network Layer**: WiFi STA/AP dual mode with automatic fallback (`rede.cpp`)
- **Configuration Layer**: NVS-based persistent storage with runtime config struct (`preferences_manager.cpp`)
- **Web Interface**: Async web server with RESTful configuration endpoints (`servidor_web.cpp`)
- **Application Layer**: Pin management, actions, and inter-module communication

## Key Components & Data Flow

### Configuration System (`MainConfig_t` struct in `globals.h`)
- **Runtime Config**: `vSt_mainConfig` - in-memory configuration state including NTP settings
- **Persistent Storage**: NVS with separate namespaces (`smcrconf` for main, `smcr_generic_configs` for misc)
- **Startup Pattern**: Load config → Initialize systems → Start NTP → Start web server
- **NTP Integration**: `vS_ntpServer1`, `vI_gmtOffsetSec`, `vI_daylightOffsetSec` in config struct

### Network Management (`rede.cpp`)
- **Dual Mode**: Attempts WiFi STA connection, falls back to AP mode for configuration
- **State Tracking**: `vB_wifiIsConnected` global flag with event-driven updates
- **Auto-Recovery**: Non-blocking reconnection attempts every 15 seconds in `loop()`
- **mDNS Service**: `fV_setupMdns()` initializes mDNS with configured hostname
  - Automatically started after successful WiFi connection
  - Adds HTTP service advertisement on configured port
  - Includes device metadata (version, device type)
  - Auto-reinitializes on WiFi reconnection
  - Access device via `http://<hostname>.local:<port>/`
- **Uptime Formatting**: `fS_formatUptime()` provides human-readable uptime strings

### Time Synchronization (`ntp_func.cpp`)
- **Non-blocking NTP**: `fV_setupNtp()` configures NTP service with pool.ntp.br as primary
- **Time Formatting**: `fS_getFormattedTime()` returns "DD/MM/YYYY HH:MM:SS" or fallback message
- **Status Checking**: `fS_getNtpStatus()` returns sync state ("Sincronizado", "Aguardando", etc.)
- **Fallback Behavior**: Uses uptime display when NTP sync fails

### Debugging System (`utils.cpp`)
- **Bitmasked Logging**: Use `fV_printSerialDebug(LOG_NETWORK | LOG_WEB, "message", args...)`
- **Flag Categories**: `LOG_INIT`, `LOG_NETWORK`, `LOG_PINS`, `LOG_FLASH`, `LOG_WEB`, `LOG_SENSOR`, `LOG_ACTIONS`, `LOG_INTERMOD`, `LOG_WATCHDOG`
- **Runtime Control**: Debug output controlled by `vSt_mainConfig.vB_serialDebugEnabled`

## Development Workflows

### Building & Flashing
```bash
# Standard PlatformIO commands from project root
platformio run                    # Build only
platformio run --target upload    # Build and upload
platformio device monitor         # Serial monitor
```

### First-Time Setup Workflow
1. Flash firmware → ESP32 boots in AP mode
2. Connect to `esp32modularx Ponto de Acesso` (password: `senha12345678`)
3. Navigate to `http://192.168.4.1:8080/` for initial config (`web_modoap.h`)
4. Configure hostname, WiFi credentials → ESP32 restarts in STA mode
5. Access via assigned IP or `http://esp32modularx.local:8080/` → Dashboard (`web_dashboard.h`)

### Web Interface Workflow
- **AP Mode**: Serves configuration page from `web_modoap.h` (Tailwind CSS styling)
- **STA Mode**: Serves dashboard from `web_dashboard.h` with real-time status cards
- **API Endpoint**: `/status/json` provides live data (IP, uptime, heap, NTP status)
- **Polling**: Dashboard updates every 15 seconds via JavaScript fetch
- **Performance Monitoring**: All 10 web pages include load time indicator
  - Shows page load time in milliseconds (bottom-right corner)
  - Non-intrusive display with dark semi-transparent background
  - Logs performance data to browser console
  - Pattern: `const pageStartTime = performance.now()` → display after window.onload

### Configuration Management
- **Load sequence**: `fV_carregarMainConfig()` → validate defaults → populate runtime struct
- **Save pattern**: Update `vSt_mainConfig` → `fV_salvarMainConfig()` → persist to NVS
- **Web endpoints**: POST to `/save_config` with form data triggers config update + restart

## Project-Specific Conventions

### Naming Patterns
- **Variables**: Hungarian notation with type prefix (`vB_` bool, `vS_` String, `vU32_` uint32_t, `vSt_` struct)
- **Functions**: `f[ReturnType]_functionName` (`fV_` void return, `fS_` String return, etc.)
- **Constants**: `KEY_CONFIG_NAME` for NVS keys, `LOG_CATEGORY` for debug flags

### File Organization
- **`globals.h`**: Central header with struct definitions, function prototypes, extern declarations
- **Module pattern**: Each `.cpp` file handles specific domain (network, web, preferences, utilities)
- **Web content**: HTML stored as `PROGMEM` strings in `web_pages.h`

### Web Pages Structure
All 10 web pages follow consistent patterns:
1. **Dashboard** (`web_dashboard.h`) - Status overview with auto-refresh
2. **Configurações** (`web_config_gerais.h`) - General settings
3. **Pinos** (`web_pins.h`) - GPIO pin configuration
4. **Ações** (`web_actions.h`) - Action automation rules
5. **MQTT** (`web_mqtt.h`) - MQTT broker settings
6. **Inter-Módulos** (`web_intermod.h`) - Module communication (placeholder)
7. **Firmware** (`web_firmware.h`) - OTA firmware upload
8. **Preferências** (`web_preferencias.h`) - NVS export/import
9. **LittleFS** (`web_littlefs.h`) - File system management
10. **Web Serial** (`web_serial.h`) - Live serial monitor
11. **Reset** (`web_reset.h`) - System reset options
12. **AP Mode** (`web_modoap.h`) - Initial WiFi setup

Each page includes:
- Unified navigation menu with dropdowns (Status | Configurações | Pinos | Ações | [Serviços▾] | [Arquivos▾] | [Util▾])
- Performance indicator (load time display)
- Inline CSS for zero external dependencies
- Mobile-responsive layout with flexbox

### Memory Management
- **Async operations**: Uses `ESPAsyncWebServer` - avoid blocking operations in handlers
- **Global state**: Minimal globals, main config centralized in `vSt_mainConfig` struct
- **Resource cleanup**: Proper deletion of `AsyncWebServer` instance on reconfiguration
- **Flash usage**: Critical at 97.2% (1274125/1310720 bytes) - minimize additions

## Integration Points

### External Dependencies (platformio.ini)
- **Core**: `ArduinoJson` for API responses, `ESPAsyncWebServer`/`AsyncTCP` for web interface
- **Network**: Built-in WiFi and mDNS capabilities
- **Storage**: ESP32 Preferences library for NVS operations

### Pin Configuration System
- **Dynamic allocation**: Pin definitions stored in NVS, not hardcoded
- **Action chaining**: Pins can trigger cascading actions (pin 2 → pin 4 → pin 5)
- **Future extensions**: Supports MQTT, Telegram notifications, inter-module communication

### Web API Patterns
```cpp
// Standard request handler pattern
SERVIDOR_WEB_ASYNC->on("/endpoint", HTTP_POST, [](AsyncWebServerRequest *req) {
    if (req->hasArg("param")) {
        // Update vSt_mainConfig
        // Call fV_salvarMainConfig()
        req->send(200, "text/plain", "OK");
    } else {
        req->send(400, "text/plain", "Missing params");
    }
});

// JSON API response pattern (like /status/json)
void fV_handleStatusJson(AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;
    doc["system"]["hostname"] = vSt_mainConfig.vS_hostname;
    doc["wifi"]["status"] = vB_wifiIsConnected ? "Conectado" : "Desconectado";
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}
```

## Critical Implementation Notes

- **Non-blocking design**: Network operations use callbacks, avoid `delay()` in main loop except for initialization
- **Configuration validation**: No input sanitization currently implemented - validate all web inputs
- **Memory constraints**: ESP32 has limited RAM - use `PROGMEM` for large strings, monitor heap usage
- **Portuguese language**: UI and debug messages in Portuguese, maintain consistency
- **Module ID generation**: Uses `ESP.getEfuseMac()` for unique device identification

When extending this system, follow the established patterns: update `MainConfig_t` for new settings, add corresponding NVS keys, implement web endpoints for configuration, and use the logging system for debugging.