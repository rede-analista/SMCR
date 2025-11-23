# Implementação do Sistema de Cores para Status dos Pinos

## Resumo da Implementação

O sistema de cores para status dos pinos foi integrado com o sistema de configuração geral do SMCR, permitindo que as cores sejam configuradas pelo usuário e aplicadas dinamicamente no dashboard.

## Estrutura de Dados

### MainConfig_t (globals.h)
```cpp
String vS_corStatusComAlerta;    // Cor para status com alerta/problema
String vS_corStatusSemAlerta;    // Cor para status sem alerta/OK
```

## Backend - Servidor Web

### API /config/json
Retorna todas as configurações do sistema, incluindo:
```json
{
  "cor_com_alerta": "#ff0000",
  "cor_sem_alerta": "#00ff00"
}
```

### API /status/json
Adicionada seção system com cores em tempo real:
```json
{
  "system": {
    "cor_com_alerta": "#ff0000",
    "cor_sem_alerta": "#00ff00"
  }
}
```

## Frontend - Dashboard

### Variáveis JavaScript
```javascript
let systemColors = {
    comAlerta: '#ff0000',    // Padrão vermelho
    semAlerta: '#00ff00'     // Padrão verde
};
```

### Sistema CSS Dinâmico
Função `updateSystemColors()` cria estilos CSS dinamicamente:
```css
.pin-status-ok { color: [cor_sem_alerta] !important; }
.pin-status-alert { color: [cor_com_alerta] !important; }
```

### Lógica de Status
Função `getStatusClass()` determina qual cor aplicar:

- **Pinos Digitais:**
  - HIGH (1): `pin-status-ok` (verde)
  - LOW (0): `text-gray-600` (cinza neutro)

- **Pinos Analógicos:**
  - Valor > 3000 (73%): `pin-status-alert` (vermelho - alerta)
  - Valor ≤ 3000: `pin-status-ok` (verde - OK)

- **Pinos Remotos:**
  - Sempre `text-gray-600` (cinza - indefinido)

## Carregamento e Atualização

### Inicialização
1. `window.onload` → carrega `/config/json`
2. Define cores do sistema
3. Aplica `updateSystemColors()`
4. Inicia loops de atualização

### Tempo Real
1. Intervalo configurável via `/status/json`
2. Atualiza cores se alteradas no sistema
3. Reaplica estilos automaticamente

## Configuração Pelo Usuário

As cores podem ser configuradas na página de **Configuração Geral**:
- Campo "Cor status com alerta" → `vS_corStatusComAlerta`
- Campo "Cor status sem alerta" → `vS_corStatusSemAlerta`
- Valores em formato hexadecimal (#RRGGBB)

## Benefícios

1. **Consistência Visual:** Mesmas cores em todo o sistema
2. **Personalização:** Usuário define cores conforme preferência
3. **Tempo Real:** Mudanças aplicadas imediatamente
4. **Responsivo:** Adapta automaticamente aos novos valores

## Compatibilidade

- Mantém cores padrão se configuração não disponível
- Graceful fallback para Tailwind CSS em casos de erro
- Suporta qualquer valor hexadecimal válido