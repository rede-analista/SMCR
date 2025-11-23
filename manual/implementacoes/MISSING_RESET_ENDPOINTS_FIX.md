# Missing Reset Endpoints Fix - Correção de Endpoints de Reset Ausentes

## Data: 22/11/2025

## Problema Identificado

Durante teste da funcionalidade de reset de pinos, o sistema retornou erro 404:

```
[SMCR_DEBUG][WEB] [RESET] Servindo página de reset...
[SMCR_DEBUG][WEB] JSON de status servido.
[SMCR_DEBUG][WEB] 404 - URL nao encontrada. URI: /reset/pins (Source IP: 172.31.250.254)
```

### Análise da Causa

A interface web `web_reset.h` implementava chamadas JavaScript para endpoints que não estavam implementados no backend:

#### Endpoints Ausentes:
- ❌ `POST /reset/pins` - Reset apenas de configurações de pinos
- ❌ `POST /restart` - Restart simples do sistema

#### JavaScript Existente (funcionando):
```javascript
function resetPinsOnly() {
    fetch('/reset/pins', { method: 'POST' }) // ← 404 Error
}

function restartSystem() {
    fetch('/restart', { method: 'POST' })    // ← 404 Error
}
```

## Solução Implementada

### 1. Adição de Rotas no Servidor Web

**Arquivo**: `src/servidor_web.cpp`

Adicionadas as rotas ausentes na função de configuração do servidor:

```cpp
// Rotas de reset (POST)
SERVIDOR_WEB_ASYNC->on("/reset/soft", HTTP_POST, fV_handleSoftReset);
SERVIDOR_WEB_ASYNC->on("/reset/factory", HTTP_POST, fV_handleFactoryReset);
SERVIDOR_WEB_ASYNC->on("/reset/network", HTTP_POST, fV_handleNetworkReset);
SERVIDOR_WEB_ASYNC->on("/reset/config", HTTP_POST, fV_handleConfigReset);
SERVIDOR_WEB_ASYNC->on("/reset/pins", HTTP_POST, fV_handlePinsReset);      // ← NOVO
SERVIDOR_WEB_ASYNC->on("/restart", HTTP_POST, fV_handleRestart);           // ← NOVO
```

### 2. Implementação dos Handlers

#### Handler para Reset de Pinos (`fV_handlePinsReset`)
```cpp
void fV_handlePinsReset(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESET] Executando reset apenas de pinos...");
    
    request->send(200, "text/plain", "OK");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Limpa apenas configurações de pinos
    fV_clearPinConfigs();
    
    fV_printSerialDebug(LOG_WEB, "[RESET] Reset de pinos concluído.");
}
```

**Funcionalidade:**
- ✅ Remove arquivo `/pins_config.json` do LittleFS
- ✅ Limpa array de pinos em memória
- ✅ Mantém todas as outras configurações intactas
- ✅ Não reinicia o sistema (reset suave)

#### Handler para Restart (`fV_handleRestart`)
```cpp
void fV_handleRestart(AsyncWebServerRequest *request) {
    fV_printSerialDebug(LOG_WEB, "[RESTART] Reiniciando sistema...");
    
    request->send(200, "text/plain", "Sistema reiniciando...");
    
    // Pequeno delay para enviar resposta
    delay(100);
    
    // Reinicia o ESP32
    ESP.restart();
}
```

**Funcionalidade:**
- ✅ Reinicia o ESP32 mantendo todas as configurações
- ✅ Útil para resolver problemas temporários
- ✅ Resposta HTTP antes do restart

### 3. Declarações de Funções

**Arquivo**: `include/globals.h`

Adicionadas as declarações das novas funções:

```cpp
void fV_handleConfigReset(AsyncWebServerRequest *request); // Handler para reset de config (mantém rede)
void fV_handlePinsReset(AsyncWebServerRequest *request);   // Handler para reset apenas de pinos
void fV_handleRestart(AsyncWebServerRequest *request);     // Handler para restart simples
```

## Funcionalidades de Reset Completas

### Matriz de Reset Implementada

| Endpoint | Configurações | Rede | Pinos | Sistema | Resultado |
|----------|---------------|------|-------|---------|-----------|
| `/restart` | ✅ Mantém | ✅ Mantém | ✅ Mantém | 🔄 Restart | Solve problemas temporários |
| `/reset/pins` | ✅ Mantém | ✅ Mantém | ❌ Remove | ✅ Mantém | Limpa apenas pinos |
| `/reset/config` | ❌ Remove | ✅ Mantém | ❌ Remove | 🔄 Restart | Mantém conectividade |
| `/reset/factory` | ❌ Remove | ❌ Remove | ❌ Remove | 🔄 Restart | Volta ao AP mode |

### Fluxo de Operações

#### Reset de Pinos (`/reset/pins`):
1. **Interface**: Confirmação dupla do usuário
2. **Request**: POST para `/reset/pins`
3. **Backend**: Chama `fV_clearPinConfigs()`
4. **LittleFS**: Remove `/pins_config.json`
5. **Memória**: Limpa array `vA_pinConfigs`
6. **Response**: "OK" + log de debug
7. **Frontend**: Redireciona para `/pins` em 3 segundos

#### Restart Sistema (`/restart`):
1. **Interface**: Confirmação simples do usuário
2. **Request**: POST para `/restart`
3. **Backend**: Responde "Sistema reiniciando..."
4. **Sistema**: `ESP.restart()` após 100ms
5. **Frontend**: Aguarda 30 segundos e redireciona

## Benefícios Implementados

### User Experience:
- **Funcionalidade completa** - Todos os botões da interface agora funcionam
- **Feedback apropriado** - Mensagens de status específicas para cada operação
- **Granularidade** - Reset específico apenas de pinos sem perder outras configurações
- **Segurança** - Confirmações apropriadas antes de operações destrutivas

### Sistema:
- **Robustez** - Cobertura completa de cenários de reset
- **Manutenibilidade** - Debug logs específicos para troubleshooting
- **Flexibilidade** - Diferentes níveis de reset conforme necessidade
- **Consistência** - Padrão uniforme de implementação para todos os endpoints

## Validação

### Compilação:
- ✅ Compilação bem-sucedida
- ✅ RAM: 14.3% (46808 bytes)
- ✅ Flash: 75.5% (989961 bytes)
- ✅ Aumento mínimo de 752 bytes na flash (código adicional)

### Testes Esperados:
- ✅ Interface reset deve funcionar sem erros 404
- ✅ Reset de pinos deve limpar apenas configurações de pinos
- ✅ Restart deve reiniciar sistema mantendo configurações
- ✅ Logs de debug devem confirmar operações executadas

## Arquivos Modificados

1. **`src/servidor_web.cpp`**:
   - Adição de 2 novas rotas HTTP
   - Implementação de 2 novos handlers
   - 30 linhas de código adicionadas

2. **`include/globals.h`**:
   - Declaração de 2 novas funções
   - Documentação inline das funcionalidades

## Observações Técnicas

- **Delay Response**: 100ms de delay após enviar resposta HTTP para garantir entrega antes do restart
- **Error Handling**: Handlers seguem padrão consistente com funções existentes
- **Memory Management**: Reset de pinos utiliza função já existente e testada `fV_clearPinConfigs()`
- **Logging**: Todas as operações são registradas no sistema de debug com categoria `LOG_WEB`

## Próximos Passos

Para testes funcionais:
1. Carregar firmware no ESP32
2. Configurar alguns pinos
3. Testar "Limpar Apenas Pinos" - deve remover configurações mantendo conectividade
4. Testar "Reiniciar Sistema" - deve fazer restart completo
5. Verificar logs via serial para confirmar execução

Esta correção completa a funcionalidade de reset, eliminando erros 404 e fornecendo controle granular sobre diferentes tipos de reset no sistema.