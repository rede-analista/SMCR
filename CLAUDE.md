# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SMCR (Sistema Modular de Controle e Recursos) is an ESP32-based modular sensor/actuator control system written in C++ using the Arduino framework. It provides dynamic GPIO configuration (up to 254 pins), web-based management, MQTT integration with Home Assistant auto-discovery, inter-module communication, and Telegram notifications.

## Build Commands

```bash
platformio run                    # Build only
platformio run --target upload    # Build and flash to ESP32
platformio device monitor         # Serial monitor (115200 baud)
```

## Architecture

### Layered Design
```
Web Interface Layer (servidor_web.cpp)
    └─→ 12 async web pages, JSON APIs, file management
Application Layer
    ├─→ pin_manager.cpp      - GPIO config & state management
    ├─→ action_manager.cpp   - Automation engine (cascading actions)
    ├─→ mqtt_manager.cpp     - MQTT client & Home Assistant discovery
    ├─→ intermod_manager.cpp - ESP32-to-ESP32 communication via MQTT
    └─→ telegram_manager.cpp - Telegram bot notifications
System Services Layer
    ├─→ rede.cpp             - WiFi STA/AP dual mode, mDNS
    ├─→ preferences_manager.cpp - NVS configuration persistence
    ├─→ ntp_func.cpp         - NTP time synchronization
    └─→ utils.cpp            - Bitmasked debug logging
```

### Key Files
- **globals.h**: Central header with all struct definitions (`MainConfig_t`, `PinConfig_t`, `ActionConfig_t`, `InterModConfig_t`), extern declarations, and function prototypes
- **main.cpp**: Startup sequence and non-blocking event loop
- **servidor_web.cpp**: Async web server (~3000 lines), routes, JSON APIs

### Data Flow
The `loop()` function runs non-blocking checks at intervals:
1. WiFi connection check (15s)
2. Pin reading (500ms) → state updates
3. Action execution (100ms) → cascading logic
4. MQTT loop (450ms) → publish/subscribe
5. Inter-module tasks (15s) → health checks

## Naming Conventions (Hungarian Notation)

**Variables:**
- `vB_` = bool, `vS_` = String, `vI_` = int
- `vU8_` = uint8_t, `vU16_` = uint16_t, `vU32_` = uint32_t
- `vUL_` = unsigned long, `vSt_` = struct, `vO_` = object

**Functions:**
- `fV_functionName()` = void return
- `fS_functionName()` = String return
- `fB_functionName()` = bool return
- `fI_/fU8_/fU16_/fU32_functionName()` = numeric returns

**Constants:**
- `KEY_CONFIG_NAME` = NVS keys
- `LOG_CATEGORY` = debug flags (LOG_INIT, LOG_NETWORK, LOG_PINS, etc.)

## Configuration Patterns

**Runtime config**: `vSt_mainConfig` struct (in-memory)
**Persistence**: NVS with namespace `smcrconf`

```cpp
// Standard update pattern
vSt_mainConfig.vS_hostname = "newname";
fV_salvarMainConfig();  // Persist to NVS
```

**Web API handler pattern:**
```cpp
SERVIDOR_WEB_ASYNC->on("/endpoint", HTTP_POST, [](AsyncWebServerRequest *req) {
    // Update vSt_mainConfig
    // Call fV_salvarMainConfig()
    req->send(200, "text/plain", "OK");
});
```

## Debug Logging

```cpp
fV_printSerialDebug(LOG_NETWORK | LOG_PINS, "Message: %s", value);
```
Categories: `LOG_INIT`, `LOG_NETWORK`, `LOG_PINS`, `LOG_FLASH`, `LOG_WEB`, `LOG_SENSOR`, `LOG_ACTIONS`, `LOG_INTERMOD`, `LOG_WATCHDOG`, `LOG_MQTT`

Controlled by `vSt_mainConfig.vB_serialDebugEnabled` and `vSt_mainConfig.vU32_activeLogFlags`.

## Critical Constraints

- **Flash usage: ~97%** - Minimize string additions and new features
- **No input validation** - Incorrect pin configs can damage ESP32
- **Non-blocking design** - Never use `delay()` in main loop; use interval-based checks
- **Portuguese language** - All UI and debug messages in Portuguese
- **PROGMEM for HTML** - Web pages stored as PROGMEM strings with LittleFS fallback

## Web Interface Structure

All 12 pages include unified navigation menu (dropdowns for Serviços, Arquivos, Util), inline CSS (no external dependencies), and performance indicator showing load time.

Web content is in `include/web_*.h` files as PROGMEM strings, with corresponding HTML in `data/` for LittleFS.

## Common Development Tasks

**Add new setting:**
1. Add field to `MainConfig_t` in `globals.h`
2. Add NVS key constant and load/save logic in `preferences_manager.cpp`
3. Add web form/API endpoint in `servidor_web.cpp`

**Add new action type:**
1. Define constant in `action_manager.cpp`
2. Add execution logic in action processing
3. Update web UI in `web_actions.h`

**MQTT topic:**
1. Implement in `mqtt_manager.cpp`
2. Add to Home Assistant discovery payload if needed
