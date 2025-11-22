# Funcionamento e Configurações de Rede

O SMCR possui um sistema avançado de configuração de rede com interface empresarial profissional, suportando configurações estáticas, fallback automático e sincronização NTP.

No momento inicial (primeiro boot) após a gravação do firmware, o módulo vem com a função "Modo AP" ativada, proporcionando facilidade para conectar e configurar o SSID e senha para que o módulo consiga se conectar na rede WiFi de trabalho.<br>

## Funcionamento no Modo AP (quando ativado)
- Ao ligar o módulo ele carrega as informações de rede que estão na flash e tenta se conectar na rede wifi que foi configurada, caso não consiga em 4 tentativas, ele aciona o modo AP, neste caso o módulo vai gerar uma rede wifi própria para que você possa se conectar no módulo e realizar a configuração básica do wifi.<br>
Se a configuração default não foi alterada o nome da rede própria do módulo será "SMCR_AP_SETUP" e a senha para se conectar nesta rede será "senha12345678".<br>

  - Conecte-se na rede wifi.<br>
  - Abra o browser e digite o endereço "http://192.168.4.1:8080".<br>
  - Ao abrir a página informe o nome e senha da sua rede wifi e salve as informações.<br>
![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/c_wifi_inicial_t1.png)


  - Depois de salvar as informações o módulo irá reiniciar e conectar na sua rede wifi.<br>


  NOTA 1:
  - Após configurar o wifi o módulo irá reiniciar e conectar na rede wifi que você acabou de informar, a partir deste ponto você deve acessar o módulo pelo IP que foi atribuído pelo seu roteador.<br>
  - A programação configura o recurso de mDNS, você pode tentar descobrir o IP do módulo com o comando "ping esp32modularx.local". Caso não tenha resultado você deverá conectar no seu roteador para identificar qual andereço IP o módulo recebeu.<br>
  - É recomendado realizar o recurso de reserva de IP do seu roteador para que o módulo sempre receba o mesmo endereço IP. Isso é importante pois caso você configure comunicação entre módulos e o IP de um dos módulos mudar, a comunicação entre módulos pode não funcionar.



## Nova Interface de Configuração de Rede

O sistema agora possui uma interface moderna e profissional para configurações de rede, acessível através do menu **"Configurações Gerais" → "Configuração Geral"**.

### Sistema Profissional de Configuração

A interface utiliza o conceito de **Configuração Ativa** vs **Configuração Persistente**:
- **Configuração Ativa**: Configurações ativas na memória (temporárias)
- **Configuração Persistente**: Configurações salvas na flash (permanentes)
- **Aplicar**: Aplica configurações na memória para teste
- **Salvar na Flash**: Salva configurações testadas permanentemente
- **Salvar e Reiniciar**: Salva e reinicia para aplicar configurações gerais


## Configurações Detalhadas de Rede

### 1. Configurações Básicas de WiFi

#### **SSID da Rede WiFi**
- Nome da rede WiFi que o módulo usará para conexão durante o funcionamento normal
- Campo pré-preenchido com valor atual para facilitar edição
- Essencial para conectividade básica do módulo

#### **Senha da Rede WiFi**
- Senha de acesso à rede WiFi configurada
- Campo pré-preenchido com senha atual (mesmo que seja alterada)
- Suporte a WPA/WPA2/WPA3

#### **Hostname do Módulo (mDNS)**
- Nome único do módulo na rede local
- Usado para acesso sem conhecer o IP: `http://hostname.local:8080`
- Deve ser único na rede para evitar conflitos
- Exemplos de acesso:
  - `http://esp32modularx.local:8080`
  - `http://192.168.50.10:8080`

💡 **Dica**: Use `ping hostname.local` para descobrir o IP do módulo

#### **Tentativas de Conexão WiFi**
- Número de tentativas antes de ativar o modo AP fallback
- Padrão: 15 tentativas
- Evita travamento do módulo por problemas de rede
- Após as tentativas, o módulo continua funcionando localmente

### 2. Configurações Avançadas de Rede

#### **IP Estático (Opcional)**
- Endereço IP fixo para o módulo
- Deixar vazio para DHCP automático
- Formato: `192.168.1.100`
- Recomendado para comunicação inter-módulos

#### **Gateway Padrão**
- Endereço do roteador da rede
- Necessário apenas se usar IP estático
- Formato: `192.168.1.1`

#### **Máscara de Sub-rede**
- Máscara de rede para IP estático
- Padrão: `255.255.255.0`
- Configure conforme sua rede local

#### **DNS Primário**
- Servidor DNS principal para resolução de nomes
- Padrão: `8.8.8.8` (Google DNS)
- Importante para conectividade com serviços externos

#### **DNS Secundário**
- Servidor DNS alternativo
- Padrão: `8.8.4.4` (Google DNS secundário)
- Backup caso o DNS primário falhe

### 3. Configurações de Access Point (Fallback)

#### **SSID do Modo AP**
- Nome da rede WiFi criada pelo módulo em modo fallback
- Padrão: `esp32modularx Ponto de Acesso`
- Ativado automaticamente se a conexão WiFi falhar

#### **Senha do Modo AP**
- Senha para acessar a rede do módulo em modo AP
- Padrão: `senha12345678`
- Mínimo 8 caracteres para segurança WPA2

#### **Habilitar Fallback AP**
- Ativa/desativa o modo AP automático
- Recomendado: **Habilitado** para recuperação remota
- Permite reconfiguração mesmo com problemas de rede

### 4. Configurações de Tempo (NTP)

#### **Servidor NTP Primário**
- Servidor para sincronização de horário
- Padrão: `pool.ntp.br` (servidores brasileiros)
- Importante para logs com timestamp correto

#### **Fuso Horário (GMT Offset)**
- Diferença em relação ao GMT em segundos
- Brasil: `-10800` (GMT-3) ou `-7200` (GMT-2 no horário de verão)
- Calculado automaticamente para horário de Brasília

#### **Offset Horário de Verão**
- Ajuste adicional para horário de verão
- Brasil: `3600` (1 hora) quando aplicável
- Configuração automática baseada no período do ano

### 5. Configurações de Monitoramento

#### **Intervalo de Verificação WiFi**
- Tempo em milissegundos entre verificações de conectividade
- Padrão: `15000` ms (15 segundos)
- Detecta e reconecta automaticamente em caso de queda

### 6. Configurações de Segurança Web

#### **Porta do Servidor Web**
- Porta TCP utilizada para acesso à interface web
- Padrão: `8080`
- Intervalo válido: 80 a 65535
- **Importante**: Alterações de porta requerem "Salvar e Reiniciar"

#### **Habilitar Autenticação Web**
- Ativa/desativa sistema de autenticação HTTP Basic
- **Padrão**: Desabilitado (acesso livre)
- Quando habilitado, protege páginas de configuração
- Credenciais necessárias para acesso às funções administrativas

#### **Usuário Administrador**
- Nome de usuário para autenticação web
- Campo obrigatório se autenticação estiver ativa
- Use nomes seguros e não óbvios
- **Padrão**: `admin`

#### **Senha Administrador**
- Senha para autenticação na interface web
- Mínimo 8 caracteres recomendado
- Campo pré-preenchido para facilitar alterações
- **Padrão**: `admin123`

#### **Dashboard Requer Autenticação**
- Controla se a página inicial (dashboard) requer login
- **Opções**:
  - ✅ **Habilitado**: Dashboard protegido por autenticação
  - ❌ **Desabilitado**: Dashboard funciona como painel público
- **Padrão**: Desabilitado
- Permite usar dashboard como display de informações sem comprometer segurança

### 🔐 **Sistema de Autenticação Inteligente**

#### **Funcionamento da Autenticação**
- **Autenticação Desabilitada**: Acesso livre a todas as páginas
- **Autenticação Habilitada**:
  - Páginas protegidas: `/configuracao`, `/reset`, `/config/json`, `/apply_config`, `/save_to_flash`
  - Dashboard (`/`): Conforme configuração `Dashboard Requer Autenticação`

#### **Cenários de Uso**
1. **Ambiente Doméstico**: Autenticação desabilitada
2. **Ambiente Corporativo**: Autenticação habilitada, dashboard público
3. **Máxima Segurança**: Autenticação habilitada, dashboard protegido



## Workflow de Configuração Profissional

### 🔧 Processo Recomendado de Configuração

1. **Configurar Parâmetros**: Preencha os campos necessários na interface
2. **Aplicar**: Testa as configurações na Configuração Ativa (memória)
3. **Validar Funcionamento**: Verifique se a conectividade funciona corretamente
4. **Salvar na Flash**: Salva as configurações testadas permanentemente
5. **Salvar e Reiniciar**: Use apenas se necessário (mudanças de IP/rede)

### ⚡ Comandos de Configuração

#### **Aplicar Configurações**
- ✅ Aplica configurações na **Configuração Ativa** (temporário)
- ✅ Permite testar sem salvar permanentemente
- ✅ Ideal para validar conectividade
- ⚠️ Configurações serão perdidas se reiniciar sem salvar

#### **Salvar na Flash**
- 💾 Persiste **Configuração Ativa** na **Configuração Persistente**
- 💾 Configurações mantidas mesmo após reboot
- 💾 Recomendado após validar funcionamento

#### **Salvar e Reiniciar**
- 🔄 Salva e reinicia imediatamente
- 🔄 Necessário para mudanças de rede/IP
- 🔄 Aplica todas as configurações de sistema

## Cenários de Uso e Exemplos

### 🏠 Configuração Doméstica Simples
```
SSID: MinhaRedeWiFi
Senha: minhasenha123
Hostname: esp32-sala
IP: DHCP (automático)
DNS: Automático
```

### 🏢 Configuração Corporativa com IP Fixo
```
SSID: RedeEmpresa
Senha: senhaSegura2024
Hostname: smcr-producao-01
IP Estático: 192.168.100.50
Gateway: 192.168.100.1
DNS Primário: 192.168.100.10
DNS Secundário: 8.8.8.8
```

### 🔧 Configuração para Múltiplos Módulos
```
SSID: RedeIoT
Senha: iot@2024!
Hostname: smcr-modulo-[01-99]
IP Estático: 192.168.200.10-99
Gateway: 192.168.200.1
Fallback AP: Habilitado
```

## Resolução de Problemas de Rede

### 🚨 Módulo Não Conecta ao WiFi
1. **Verifique credenciais**: SSID/senha corretos
2. **Teste modo AP**: Conecte em `SMCR_AP_SETUP`
3. **Reconfigure**: Use interface AP em `192.168.4.1:8080`
4. **Reset de rede**: Use menu Reset → "Reset Configurações de Rede"

### 🚨 Não Consegue Acessar por Hostname
1. **Teste IP direto**: Use `ping hostname.local` para descobrir IP
2. **Verifique mDNS**: Alguns roteadores bloqueiam mDNS
3. **Use IP estático**: Configure IP fixo para acesso confiável
4. **Verificar porta**: Confirme se está usando porta 8080

### 🚨 Problemas de Sincronização NTP
1. **Verifique conectividade**: Módulo deve ter acesso à internet
2. **Teste servidor NTP**: Use `pool.ntp.br` para servidores brasileiros
3. **Configurar firewall**: Liberação da porta 123 UDP
4. **Reset NTP**: Menu Reset → "Reset Configurações NTP"

## ⚠️ Avisos Importantes sobre Configuração de Rede

### 🔒 Segurança
- **Senhas fortes**: Use senhas com 8+ caracteres
- **Rede isolada**: Considere VLAN separada para dispositivos IoT
- **Autenticação**: Habilite login web em ambientes corporativos
- **Backup**: Anote configurações antes de mudanças críticas

### 🌐 Conectividade
- **DHCP vs Estático**: IP estático recomendado para inter-módulos
- **DNS confiável**: Use servidores DNS estáveis (Google 8.8.8.8)
- **Fallback AP**: Sempre habilite para recuperação remota
- **Monitoramento**: Intervalo de 15s adequado para a maioria dos casos

### ⚡ Performance
- **Qualidade do sinal**: WiFi com -70dBm ou melhor
- **Largura de banda**: 2.4GHz suficiente para IoT
- **Latência**: <100ms para comunicação inter-módulos
- **Interferência**: Evite canais congestionados

## Sistema de Configuração Dual

### 📋 Configuração Ativa (Memória)
- Configurações carregadas na memória RAM
- Aplicadas imediatamente para teste
- Perdidas se o módulo for reiniciado sem salvar
- Permite validação sem comprometimento

### 💾 Configuração Persistente (Flash)
- Configurações salvas na memória flash NVS
- Carregadas automaticamente no boot
- Mantidas mesmo após reinicializações
- Base para a configuração inicial

### 🔄 Sincronização de Configurações
```
Boot → Carregar Configuração Persistente → Configuração Ativa
Aplicar → Atualizar Configuração Ativa (temporário)
Salvar na Flash → Configuração Ativa → Configuração Persistente
Reiniciar → Carregar nova Configuração Persistente → Configuração Ativa
```

# ⚠️ IMPORTANTE: Sistema de Configuração Profissional
## Sempre use "Salvar na Flash" após validar configurações com "Aplicar"
### Configurações aplicadas apenas com "Aplicar" serão perdidas no próximo reboot!
