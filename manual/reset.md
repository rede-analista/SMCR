# Menu de Reset do Sistema

O sistema SMCR possui um menu de reset abrangente com múltiplas opções para restauração e manutenção do módulo. Este menu oferece controle granular sobre diferentes aspectos da configuração e dados armazenados.

## Acesso ao Menu de Reset

- Na página inicial, clique em "Gerenciar Arquivos" e depois em "Menu de Reset"
- O menu apresenta opções organizadas com descrições claras e recursos de segurança

![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/menu_reset_t1.png)

## Opções de Reset Disponíveis

### 1. 🔄 Reiniciar Módulo (Soft Reset)
**Finalidade**: Reinicia o módulo mantendo todas as configurações
- **O que faz**: Executa um reboot simples do ESP32
- **Configurações**: Mantidas (não são perdidas)
- **Tempo**: Reinicialização imediata
- **Uso recomendado**: Aplicar mudanças de configuração, resolver travamentos temporários

### 2. 🏭 Reset de Fábrica (Factory Reset)
**Finalidade**: Restaura o módulo ao estado original de fábrica
- **O que apaga**: TODAS as configurações personalizadas
- **O que mantém**: Firmware original intacto
- **Estado final**: Módulo volta ao modo AP para configuração inicial
- **Tempo**: Reset com countdown de 10 segundos
- **Uso recomendado**: Reconfiguração completa, resolução de problemas graves

**⚠️ ATENÇÃO**: Esta operação é irreversível e apagará todas as configurações!

### 3. 🌐 Reset de Configurações de Rede
**Finalidade**: Remove apenas configurações de conectividade
- **O que apaga**: 
  - Credenciais WiFi
  - Configurações de IP estático
  - Configurações de DNS
  - Hostname personalizado
- **O que mantém**: 
  - Configurações de pinos
  - Configurações de interface
  - Ações configuradas
  - Configurações de watchdog
- **Estado final**: Módulo entra em modo AP para reconfiguração de rede
- **Uso recomendado**: Mudança de rede, problemas de conectividade

### 4. 🔧 Reset de Configurações de Pinos
**Finalidade**: Remove configurações específicas de pinos e ações
- **O que apaga**:
  - Definições de pinos configurados
  - Ações associadas aos pinos
  - Estados salvos dos pinos
- **O que mantém**:
  - Configurações de rede
  - Configurações gerais do sistema
  - Interface web
- **Uso recomendado**: Reconfigurar sistema de pinos, resolver problemas de E/S

### 5. ⚙️ Reset de Configurações de Interface
**Finalidade**: Restaura configurações da interface web
- **O que apaga**:
  - Título personalizado da página
  - Configurações de cores
  - Tempo de refresh
  - Configurações de exibição
- **O que mantém**:
  - Configurações de rede
  - Configurações de pinos
  - Sistema funcional
- **Uso recomendado**: Resolver problemas de interface, restaurar aparência padrão

### 6. 🕒 Reset de Configurações NTP
**Finalidade**: Remove configurações de sincronização de tempo
- **O que apaga**:
  - Servidor NTP configurado
  - Fuso horário
  - Configurações de horário de verão
- **O que mantém**: Todas as outras configurações do sistema
- **Estado final**: Sistema usa tempo interno do ESP32
- **Uso recomendado**: Problemas de sincronização de tempo

## 💾 Informações do Sistema

A página de reset também exibe informações detalhadas sobre o hardware e sistema:

### Informações Disponíveis
- **Firmware**: Versão atual do firmware SMCR
- **Uptime**: Tempo de funcionamento desde o último boot
- **Memória Livre**: RAM disponível no momento
- **Chip Model**: Modelo do chip ESP32 (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, etc.)
- **Chip Revision**: Revisão do silício do chip (Rev 0, Rev 1, Rev 3, etc.)
- **CPU Frequência**: Frequência atual do processador em MHz
- **MAC Address**: Endereço MAC único do dispositivo (formato XX:XX:XX:XX:XX:XX)
- **Flash Size**: Tamanho total da memória flash instalada

Estas informações são úteis para:
- **Diagnóstico**: Identificar modelo e versão do hardware
- **Suporte técnico**: Fornecer informações precisas ao reportar problemas
- **Compatibilidade**: Verificar se o hardware suporta determinados recursos
- **Inventário**: Documentar os módulos instalados na rede

## Recursos de Segurança

### Sistema de Countdown
- **Timer de 10 segundos**: Para operações críticas (Factory Reset)
- **Timer de 5 segundos**: Para operações de rede
- **Cancelamento**: Possível durante o countdown
- **Feedback visual**: Contador regressivo em tempo real

### Confirmações Múltiplas
- **Primeiro clique**: Ativa o countdown
- **Segundo clique**: Executa a operação (após timer)
- **Botão cancelar**: Disponível durante todo o processo

### Interface Responsiva
- **Design moderno**: Tailwind CSS com ícones intuitivos
- **Cores codificadas**: 
  - 🟦 Azul: Operações seguras (soft reset)
  - 🟡 Amarelo: Operações com perda de dados específicos
  - 🔴 Vermelho: Operações críticas (factory reset)
- **Feedback visual**: Estados dos botões mudam durante execução

## Cenários de Uso Recomendados

### 🚀 Desenvolvimento e Testes
- **Soft Reset**: Aplicar mudanças de configuração
- **Reset de Pinos**: Testar diferentes configurações de E/S
- **Reset de Interface**: Validar diferentes layouts

### 🔧 Manutenção
- **Reset de Rede**: Mudança de local/rede WiFi
- **Reset NTP**: Problemas de sincronização de tempo
- **Factory Reset**: Preparar módulo para novo ambiente

### 🆘 Resolução de Problemas
- **Factory Reset**: Recuperação de estado inválido
- **Reset de Rede**: Problemas de conectividade
- **Soft Reset**: Travamentos temporários

## Workflow de Segurança Recomendado

1. **Identifique o problema**: Determine qual tipo de reset é necessário
2. **Backup de configurações**: Se possível, anote configurações importantes
3. **Escolha a opção apropriada**: Use o reset mais específico possível
4. **Confirme a operação**: Leia cuidadosamente a descrição
5. **Execute o countdown**: Aguarde ou cancele se necessário
6. **Reconfigure**: Aplique as configurações necessárias após o reset

## ⚠️ Avisos Importantes

### Antes de Executar Reset
- **Backup**: Anote configurações importantes antes de resetar
- **Conectividade**: Certifique-se de ter acesso físico ao módulo
- **Credenciais**: Tenha em mãos as credenciais WiFi para reconfiguração

### Após Reset de Fábrica
- **Modo AP**: Módulo retorna ao modo ponto de acesso
- **Configuração inicial**: Necessário refazer toda configuração
- **Rede padrão**: `esp32modularx Ponto de Acesso` / senha: `senha12345678`

### Recuperação de Emergência
- **Acesso físico**: Sempre possível via modo AP
- **Reset via hardware**: Botão de reset físico do ESP32
- **Reflash de firmware**: Última opção via USB/Serial

![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/menu_reset_countdown.png)

## Logs e Depuração

O sistema registra todas as operações de reset no log serial (se debug estiver habilitado):
- **Timestamp**: Horário da operação
- **Tipo**: Categoria do reset executado
- **Status**: Sucesso ou falha da operação
- **Detalhes**: Informações específicas do processo

Estas informações são valiosas para auditoria e resolução de problemas.