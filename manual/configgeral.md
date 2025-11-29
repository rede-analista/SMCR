# Configurações Gerais

- Nas configurações gerais devem ser cadastradas informações diversas para comportamento da interface web, watchdog, refresh, etc.<br>

- Na página inicial clique em "Configurações".<br>


![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_principal.png)<br>

## Seções de Configuração

A interface de configuração está organizada em 8 seções principais:

### 1. Configurações de Rede
- **Hostname do Módulo**: Nome de identificação na rede
- **SSID da Rede WiFi**: Nome da rede sem fio para conexão
- **Senha da Rede WiFi**: Senha de acesso à rede (campo pré-preenchido)

### 2. Configurações da Interface
- **Status na Página Inicial**: Habilita/desabilita informações de status dos pinos
- **Inter Módulos na Página Inicial**: Habilita/desabilita informações de comunicação entre módulos
- **Cor Status Com Alerta**: Cor para indicar alertas (padrão HTML)
- **Cor Status Sem Alerta**: Cor para indicar normalidade (padrão HTML)
- **Tempo de Refresh**: Intervalo de atualização da página em segundos

### 3. Configurações de Tempo (NTP)
- **Servidor NTP 1**: Servidor principal de sincronização de tempo
- **Fuso Horário (GMT Offset)**: Diferença em relação ao GMT em segundos
- **Horário de Verão Offset**: Ajuste para horário de verão em segundos

### 4. Configurações de Watchdog
- **Executar WatchDog**: Habilita/desabilita o sistema de supervisão
- **Clock do ESP32**: Velocidade do processador em MHz (usado pelo watchdog)
- **Tempo para WatchDog**: Tempo limite em microssegundos para reset automático

### 5. Configurações de Pinos
- **Quantidade Total de Pinos**: Número máximo de pinos configuráveis (máximo 254)

### 6. Configurações de Segurança
- **Habilitar Autenticação**: Ativa sistema de login
- **Usuário Admin**: Nome do usuário administrador
- **Senha Admin**: Senha do administrador

### 7. Configurações de Debug
- **Debug Serial**: Habilita/desabilita mensagens de debug
- **Nível de Debug**: Controla verbosidade das mensagens

### 8. Configurações do Servidor Web
- **Porta do Servidor Web**: Porta TCP para acesso à interface (padrão 8080)
- **Habilitar Autenticação**: Ativa/desativa sistema de login web
- **Usuário Administrador**: Nome do usuário para autenticação
- **Senha Administrador**: Senha para acesso administrativo
- **Dashboard Requer Autenticação**: Se o painel inicial precisa de login

- Necessário para mudanças de rede, hostname, etc.

## Notas Importantes sobre Configuração

### ⚠️ Cuidados Especiais com Pinos
- **Quantidade Total de Pinos**: Influencia diretamente a velocidade das rotinas
- **Alteração**: Deve ser feita antes do cadastro de pinos e ações
- **Formatação**: Recomenda-se fazer baclup dos arquivos de configuração e VNS antes de alterar a quantidade
- **Mínimo**: 2 pinos (uint8_t)
- **Máximo**: 254 pinos (uint8_t)

### 📝 Parâmetros Legados (Referência)
- **Status na Página Inicial**
  - Habilita/desabilita as informações de status dos pinos na página inicial.

- **Inter Módulos na Página Inicial**
  - Habilita/desabilita as informações de comunicação de intermódulos na página inicial.

- **Cor Status Com Alerta**
  - Informe a cor do status quando tiver um alerta. Deve usar nome de cor no padrão HTML.

- **Cor Status Sem Alerta**
  - Informe a cor do status quando não tiver um alerta. Deve usar nome de cor no padrão HTML.

- **Tempo de Refresh**
  - Informe o tempo de refresh em segundos. Tempo de refresh da página inicial.

- **Executar WatchDog**
  - Habilita/desabilita execução do watchdog. Devem ser configurados os demais parâmetros antes de habilitar.

- **Clock do ESP32**
  - Informe a velocidade em Megahertz do clock do chip ESP32. Esta informação é usada no watchdog.

- **Tempo para WatchDog**
  - Informe o tempo de reset do watchdog em microssegundos. Esta informação é usada para decisão de reboot do módulo em caso de falha/travamento.

- **Quantidade Total de Pinos**
  - Informe a quantidade total de pinos que será usado. Você pode configurar uma quantidade adequada à sua necessidade respeitando o máximo de 254 (uint8_t) pinos. Uma quantidade maior irá consumir mais processamento do chip ESP32.
