# Reset Pin Configs Fix - Correção da Limpeza de Configurações de Pinos

## Data: 22/11/2025

## Problema Identificado

Após executar um reset seletivo (que preserva configurações de rede), o sistema apresentava uma inconsistência:

- **Interface Web**: Mostrava "0 pinos" na página inicial e "Nenhum pino configurado" no cadastro de pinos
- **Serial Debug**: Continuava mostrando que 1 pino estava carregado e configurado:
  ```
  [SMCR_DEBUG][PINS] [PIN] Processando 1 pinos do arquivo...
  [SMCR_DEBUG][PINS] [PIN] Carregado: Saída um (GPIO 2, tipo 1)
  [SMCR_DEBUG][PINS] [PIN] Carregadas 1 configurações de pinos.
  [SMCR_DEBUG][PINS] [PIN] Sistema de pinos inicializado. Máximo: 16, Ativos: 1
  ```

### Análise da Causa

O problema foi identificado na função `fV_clearConfigExceptNetwork()` em `preferences_manager.cpp`:

1. **O que estava sendo limpo**: Apenas configurações no NVS (namespaces `smcrconf` e `smcr_generic_configs`)
2. **O que NÃO estava sendo limpo**: Arquivos no LittleFS, especificamente `pins_config.json`

### Função Existente mas Não Utilizada

A função `fV_clearPinConfigs()` já existia em `pin_manager.cpp` e implementava corretamente a limpeza:
- Removia o arquivo `/pins_config.json` do LittleFS
- Limpava o array em memória de configurações de pinos
- Resetava contadores (`vU8_activePinsCount = 0`)

Porém, esta função não estava sendo chamada durante o reset seletivo.

## Solução Implementada

### Modificação em `preferences_manager.cpp`

Adicionada a chamada para `fV_clearPinConfigs()` na função `fV_clearConfigExceptNetwork()`:

```cpp
// 3. Limpa o namespace genérico (chaves avulsas) - mantém tudo limpo
preferences.begin(NVS_NAMESPACE_GENERIC, false);
preferences.clear();
preferences.end();

// 4. Limpa configurações de pinos (arquivo LittleFS)
fV_clearPinConfigs();

fV_printSerialDebug(LOG_FLASH, "Limpeza seletiva concluida. Rede preservada: %s", tempHostname.c_str());
```

### Ordem de Limpeza no Reset Seletivo

1. **Salvar configurações de rede** (hostname, SSID, senha)
2. **Limpar namespace principal** (MainConfig_t)
3. **Restaurar configurações de rede** no namespace principal
4. **Limpar namespace genérico** (chaves avulsas)
5. **Limpar configurações de pinos** ← **NOVO**
   - Remove `/pins_config.json` do LittleFS
   - Limpa array em memória
   - Reseta contador de pinos ativos

## Resultado Esperado

Após a correção, o reset seletivo deve:

- ✅ **Preservar configurações de rede** (hostname, SSID, senha WiFi)
- ✅ **Limpar todas as outras configurações** incluindo configurações de pinos
- ✅ **Interface web consistente** mostrando realmente 0 pinos configurados
- ✅ **Serial debug consistente** sem carregar pinos fantasma

## Validação

### Compilação
- ✅ Compilação bem-sucedida
- ✅ RAM: 14.3% (46808 bytes)
- ✅ Flash: 75.9% (994273 bytes)

### Teste Manual Necessário
1. Configure alguns pinos no sistema
2. Execute reset seletivo via interface web
3. Verificar que:
   - Interface web mostra 0 pinos
   - Serial debug não mostra pinos carregados
   - Configurações de rede são preservadas
   - Sistema pode configurar novos pinos normalmente

## Arquivos Modificados

- `src/preferences_manager.cpp`: Adicionada chamada para `fV_clearPinConfigs()`

## Impacto

- **Compatibilidade**: Mantém total compatibilidade com sistema existente
- **Performance**: Impacto mínimo (apenas uma chamada adicional durante reset)
- **Funcionalidade**: Corrige inconsistência crítica na limpeza de configurações
- **User Experience**: Elimina confusão entre interface e logs de debug

## Observações Técnicas

A função `fV_clearPinConfigs()` já estava bem implementada e testada, apenas não estava sendo utilizada no fluxo correto. Esta correção estabelece o comportamento esperado e documentado para o reset seletivo.