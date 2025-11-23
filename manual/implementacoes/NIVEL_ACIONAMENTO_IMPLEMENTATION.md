# Implementação do Nível de Acionamento para Pinos

## Resumo da Funcionalidade

O sistema de nível de acionamento permite definir quando um pino deve ser considerado "acionado" ou "ativo", oferecendo maior flexibilidade para diferentes tipos de sensores e configurações.

## Estrutura de Dados

### PinConfig_t (globals.h)
```cpp
struct PinConfig_t {
    // ... campos existentes ...
    uint16_t nivel_acionamento_min;  // Digital: 0 ou 1 | Analógico: valor mínimo do range
    uint16_t nivel_acionamento_max;  // Digital: igual ao min | Analógico: valor máximo do range
};
```

## Tipos de Configuração

### Pinos Digitais
- **Valor único**: 0 (LOW) ou 1 (HIGH)
- **nivel_acionamento_min**: Valor que determina acionamento (0 ou 1)
- **nivel_acionamento_max**: Igual ao valor mínimo
- **Exemplo**: Pin configurado para acionar com HIGH → min=1, max=1

### Pinos Analógicos
- **Range de valores**: Faixa de 0 a 4095 (12-bit ADC)
- **nivel_acionamento_min**: Valor mínimo para considerar acionado
- **nivel_acionamento_max**: Valor máximo para considerar acionado
- **Exemplo**: Sensor de temperatura → min=2000, max=2500 (para compensar ruído)

### Pinos Remotos
- **Não aplicável**: Controlados externamente
- **Valores**: min=0, max=0 (ignorados)

## Interface Web

### Formulário de Cadastro
- **Seção dedicada**: "Nível de Acionamento"
- **Campos dinâmicos**: Aparecem conforme o tipo selecionado
- **Validação**: Valores apropriados para cada tipo de pino

### JavaScript
```javascript
// Controla visibilidade baseada no tipo do pino
function updateAcionamentoVisibility(tipo) {
    // Mostra campos apropriados para cada tipo
}

// Envia dados incluindo níveis de acionamento
pinData.nivel_acionamento_min = valor_configurado;
pinData.nivel_acionamento_max = valor_configurado;
```

## Backend - Lógica de Verificação

### Função Principal
```cpp
bool fB_isPinActivated(uint8_t pinIndex) {
    // Para digitais: compara valor exato
    // Para analógicos: verifica se está no range
    // Para remotos: retorna false
}
```

### Integração com Status
- **Leitura**: `fV_updatePinStatus()` mantém funcionamento normal
- **Verificação**: `fB_isPinActivated()` aplica lógica de acionamento
- **Resultado**: Determina cores no dashboard

## Sistema Visual

### Dashboard
- **Cores dinâmicas**: Baseadas no sistema configurado
- **Lógica atualizada**:
  - **Verde** (sem alerta): Pin está no nível configurado
  - **Vermelho** (com alerta): Pin fora do nível configurado (analógicos)
  - **Cinza**: Pin em estado neutro ou inativo

### Algoritmo de Cores
```javascript
function getStatusClass(status, tipo, nivelMin, nivelMax) {
    if (tipo === 192) { // Analógico
        const isActivated = (status >= min && status <= max);
        return isActivated ? 'pin-status-ok' : 'pin-status-alert';
    } else if (tipo === 1) { // Digital
        const isActivated = (status === min);
        return isActivated ? 'pin-status-ok' : 'text-gray-600';
    }
}
```

## Casos de Uso Práticos

### 1. Sensor Digital de Presença
- **Configuração**: min=1, max=1
- **Comportamento**: Verde quando detecta presença (HIGH), cinza quando não detecta (LOW)

### 2. Sensor Analógico de Temperatura
- **Configuração**: min=2000, max=2200
- **Comportamento**: Verde quando temperatura no range desejado, vermelho fora do range

### 3. Button com Pull-up
- **Configuração**: min=0, max=0  
- **Comportamento**: Verde quando pressionado (LOW), cinza quando solto (HIGH)

### 4. Potenciômetro como Threshold
- **Configuração**: min=1500, max=3500
- **Comportamento**: Verde na faixa intermediária, vermelho nos extremos

## Benefícios

1. **Flexibilidade**: Cada pino pode ter comportamento personalizado
2. **Compensação de ruído**: Ranges para pinos analógicos
3. **Lógica invertida**: Suporte a sensores com lógica LOW-ativa
4. **Feedback visual**: Cores indicam estado real baseado na configuração
5. **Fácil configuração**: Interface intuitiva no cadastro web

## Compatibilidade

- **Retrocompatível**: Pinos antigos ganham valores padrão (min=0, max=0)
- **Migração automática**: Sistema carrega valores existentes sem problemas
- **Armazenamento**: Persistido em JSON junto com outras configurações

## Implementação Técnica

### Arquivos Modificados
- `globals.h`: Estrutura PinConfig_t expandida
- `pin_manager.cpp`: Funções de carga/salva/comparação atualizadas
- `servidor_web.cpp`: APIs incluem novos campos
- `web_pins.h`: Interface de cadastro expandida
- `web_dashboard.h`: Lógica visual atualizada

### Compilação
✅ **Testado**: Compilação bem-sucedida sem erros
✅ **Validado**: Todas as integrações funcionando