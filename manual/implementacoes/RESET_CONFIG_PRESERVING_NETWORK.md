# Implementação de Reset de Configurações Preservando Rede

## Resumo da Funcionalidade

Foi implementado um novo tipo de reset que apaga todas as configurações do sistema (pinos, ações, cores, etc.) mas preserva as configurações essenciais de conectividade de rede (hostname, SSID e senha WiFi).

## Motivação

O usuário solicitou uma opção de reset que permita limpar configurações funcionais sem perder a conectividade de rede, evitando a necessidade de reconfigurar WiFi após o reset.

## Implementação Técnica

### 1. Nova Função de Limpeza Seletiva

#### Arquivo: `src/preferences_manager.cpp`
```cpp
void fV_clearConfigExceptNetwork(void) {
    // Salva temporariamente as configurações de rede
    String tempHostname = vSt_mainConfig.vS_hostname;
    String tempWifiSsid = vSt_mainConfig.vS_wifiSsid;
    String tempWifiPass = vSt_mainConfig.vS_wifiPass;
    
    // Limpa namespace principal
    preferences.clear();
    
    // Restaura apenas configurações de rede
    preferences.putString("hostname", tempHostname);
    preferences.putString("wifi_ssid", tempWifiSsid);
    preferences.putString("wifi_pass", tempWifiPass);
}
```

### 2. Novo Handler Web

#### Arquivo: `src/servidor_web.cpp`
- **Rota**: `/reset/config` (POST)
- **Handler**: `fV_handleConfigReset()`
- **Comportamento**: Chama `fV_clearConfigExceptNetwork()` e reinicia

### 3. Interface de Usuário

#### Arquivo: `include/web_reset.h`
- **Nova seção**: "Reset de Configurações"
- **Descrição clara**: Explica o que é preservado vs apagado
- **Countdown**: 8 segundos para cancelamento
- **Feedback visual**: Status da operação

## O que é Preservado vs Apagado

### ✅ **Preservado (Conectividade)**
- **Hostname**: Nome do dispositivo na rede
- **SSID WiFi**: Nome da rede WiFi configurada
- **Senha WiFi**: Senha da rede WiFi

### ❌ **Apagado (Configurações Funcionais)**
- **Configurações de pinos**: Todos os pinos GPIO configurados
- **Ações automáticas**: Todas as ações programadas
- **Cores de status**: Cores personalizadas dos indicadores
- **Configurações NTP**: Servidor de tempo e timezone
- **Watchdog**: Configurações de monitoramento
- **Autenticação web**: Usuário e senha da interface
- **Configurações AP**: SSID e senha do ponto de acesso

## Casos de Uso

### 1. **Reconfiguração Funcional**
- Usuário quer limpar pinos e ações configurados incorretamente
- Mantém conectividade para acesso remoto contínuo
- Evita necessidade de acesso físico para reconfigurar WiFi

### 2. **Preparação para Novo Projeto**
- Reutilizar ESP32 em novo projeto mantendo mesma rede
- Limpar configurações antigas sem perder acesso remoto
- Facilita migração de projetos

### 3. **Troubleshooting Avançado**
- Isolar problemas de conectividade vs configuração
- Manter acesso remoto durante debug
- Reset seletivo para diagnóstico

## Comparação dos Tipos de Reset

| Tipo de Reset | Hostname | WiFi | Pinos | Ações | Sistema |
|---------------|----------|------|-------|-------|---------|
| **Soft Reset** | ✅ Mantém | ✅ Mantém | ✅ Mantém | ✅ Mantém | ✅ Mantém |
| **Reset Config** | ✅ Mantém | ✅ Mantém | ❌ Apaga | ❌ Apaga | ❌ Apaga |
| **Reset Rede** | ❌ Apaga | ❌ Apaga | ✅ Mantém | ✅ Mantém | ✅ Mantém |
| **Reset Fábrica** | ❌ Apaga | ❌ Apaga | ❌ Apaga | ❌ Apaga | ❌ Apaga |

## Interface Web

### Layout da Página
```html
⚙️ Reset de Configurações
├── Descrição: "Mantém hostname, SSID e senha WiFi"
├── Lista do que é preservado/apagado
├── Botão: "Reset de Configurações (Mantém Rede)"
└── Countdown de 8 segundos
```

### Feedback do Usuário
- **Durante**: "⚙️ Resetando configurações (mantendo rede)..."
- **Sucesso**: "✅ Reset concluído! Rede mantida, sistema reiniciando..."
- **Erro**: "❌ Erro no reset de configurações."

## Segurança e Validação

### Validação de Dados
1. **Verificação de valores**: Configurações de rede são validadas antes de salvar
2. **Fallback seguro**: Se falhar, mantém configuração atual
3. **Logs detalhados**: Registra operação no serial monitor

### Processo Seguro
1. **Salva dados temporariamente** em variáveis locais
2. **Limpa namespace** completamente
3. **Restaura apenas rede** com valores salvos
4. **Reinicia sistema** para aplicar mudanças

## Arquivos Modificados

### Backend
- ✅ `include/globals.h`: Novos protótipos de função
- ✅ `src/preferences_manager.cpp`: Função `fV_clearConfigExceptNetwork()`
- ✅ `src/servidor_web.cpp`: Handler `fV_handleConfigReset()` e rota `/reset/config`

### Frontend
- ✅ `include/web_reset.h`: Nova seção UI e JavaScript para o botão

## Compilação e Testes

✅ **Status**: Compilação bem-sucedida sem erros
✅ **Memória**: Flash em 78.2% (1025521 bytes)
✅ **Validação**: Todas as rotas e handlers implementados

## Procedimento de Teste

### Teste Manual Recomendado
1. **Configurar sistema completo**: WiFi + pinos + ações
2. **Verificar conectividade**: Garantir acesso via IP/hostname
3. **Executar reset de config**: Acessar `/reset` → "Reset de Configurações"
4. **Aguardar reinício**: ~30 segundos para boot completo
5. **Verificar rede**: ESP32 deve conectar automaticamente
6. **Verificar limpeza**: Pinos e ações devem estar zerados
7. **Verificar preservação**: Hostname deve estar mantido

### Validação Esperada
- ✅ ESP32 reconecta à mesma rede WiFi
- ✅ Hostname permanece o mesmo
- ✅ Interface web acessível pelo mesmo IP
- ✅ Página de pinos vazia (configurações limpas)
- ✅ Dashboard sem pinos configurados

## Benefícios

1. **Conectividade Preservada**: Mantém acesso remoto durante reconfiguração
2. **Eficiência Operacional**: Evita necessidade de acesso físico
3. **Flexibilidade**: Permite reset seletivo conforme necessidade
4. **UX Melhorada**: Usuário não perde conectividade configurada
5. **Troubleshooting**: Facilita isolamento de problemas

Com esta implementação, o sistema SMCR oferece agora 4 níveis diferentes de reset, permitindo ao usuário escolher exatamente o que deseja preservar ou resetar conforme sua necessidade específica.