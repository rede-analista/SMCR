# Configurações Gerais

- Nas configurações gerais devem ser cadastradas informações diversas para comportamento da interface web, watchdog, refresh, etc.<br>

- Na página inicial clique em "Configurações Gerais" e depois em "Configuração Geral".<br>

## Nova Interface de Configuração Profissional

O sistema agora utiliza uma interface de configuração empresarial avançada, com separação entre **Configuração Ativa** (configuração em memória) e **Configuração Persistente** (configuração salva na flash).

### Conceitos Importantes:

- **Configuração Ativa**: Configurações carregadas na memória, aplicadas imediatamente para testes
- **Configuração Persistente**: Configurações permanentes salvas na memória flash
- **Aplicar**: Aplica configurações para teste sem salvar permanentemente
- **Salvar na Flash**: Salva configurações testadas para a memória flash
- **Salvar e Reiniciar**: Salva e reinicia o módulo para aplicar todas as alterações

### Interface Moderna

A nova interface apresenta:
- Design responsivo com Tailwind CSS
- Campos pré-preenchidos com valores atuais (incluindo senhas)
- Sistema de três botões para controle profissional de configurações
- Validação de campos em tempo real
- Interface intuitiva e profissional

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_geral_novo_t1.png) 
<br>
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_geral_novo_t2.png)


## Seções de Configuração

A interface de configuração está organizada em 8 seções principais:

### 1. Configurações de Rede
- **Hostname do Módulo**: Nome de identificação na rede
- **SSID da Rede WiFi**: Nome da rede sem fio para conexão
- **Senha da Rede WiFi**: Senha de acesso à rede (campo pré-preenchido)
- **Endereço IP Estático**: IP fixo (opcional)
- **Gateway Padrão**: Endereço do gateway da rede
- **DNS Primário/Secundário**: Servidores de DNS para resolução de nomes

### 2. Configurações da Interface
- **Título da Página**: Título exibido na interface web
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

### 8. Configurações MQTT
- **Servidor MQTT**: Endereço do broker MQTT
- **Porta MQTT**: Porta de conexão (padrão 1883)
- **Usuário MQTT**: Nome de usuário para autenticação
- **Senha MQTT**: Senha para autenticação

## Workflow de Configuração Profissional

### 1. Aplicar Configurações
- Aplica as alterações na **Configuração Ativa** (memória RAM)
- Permite testar as configurações sem salvar permanentemente
- Ideal para validar mudanças antes de torná-las permanentes
- Se houver problemas, um reboot retorna às configurações salvas

### 2. Salvar na Flash
- Salva a **Configuração Ativa** atual na **Configuração Persistente** (memória flash)
- Configurações persistem mesmo após reboot
- Recomendado após testar e validar as configurações

### 3. Salvar e Reiniciar
- Salva na flash e reinicia o módulo imediatamente
- Aplica todas as configurações de rede e sistema
- Necessário para mudanças de rede, hostname, etc.

## Notas Importantes sobre Configuração

### ⚠️ Cuidados Especiais com Pinos
- **Quantidade Total de Pinos**: Influencia diretamente a velocidade das rotinas
- **Alteração**: Deve ser feita antes do cadastro de pinos e ações
- **Formatação**: Recomenda-se formatar a flash antes de alterar a quantidade
- **Máximo**: 254 pinos (uint8_t)

### 🔄 Workflow Recomendado
1. **Configurar** → **Aplicar** (testar mudanças)
2. **Validar** funcionamento correto
3. **Salvar na Flash** (salvar permanentemente)
4. **Salvar e Reiniciar** (se necessário reiniciar)

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

## ⚠️ Avisos Importantes sobre o Sistema de Configuração

### Sistema Configuração Ativa vs Configuração Persistente
- **Configuração Ativa**: Configuração carregada em memória (temporária)
- **Configuração Persistente**: Configuração salva na flash (permanente)
- **Aplicar**: Aplica mudanças apenas na Configuração Ativa para testes
- **Salvar na Flash**: Persiste Configuração Ativa na Configuração Persistente
- **Salvar e Reiniciar**: Salva e reinicia para aplicar todas as mudanças

### ⚠️ IMPORTANTE: Não esqueça de salvar!
## Se o módulo for reiniciado antes de usar "Salvar na Flash", as configurações aplicadas com "Aplicar" serão perdidas.

### Recomendações:
1. Use **Aplicar** para testar configurações
2. Valide se tudo funciona corretamente
3. Use **Salvar na Flash** para tornar as configurações permanentes
4. Use **Salvar e Reiniciar** apenas quando necessário (mudanças de rede)