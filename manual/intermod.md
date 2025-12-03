
## Sistema de Comunicação Inter-Módulos

O sistema Inter-Módulos permite comunicação direta via WiFi entre múltiplos dispositivos SMCR, com detecção automática de módulos na rede e monitoramento de saúde (healthcheck) de cada dispositivo.

### Funcionalidades Principais

- **Descoberta Automática (mDNS)**: Detecta automaticamente outros módulos SMCR na rede local
- **Healthcheck HTTP**: Monitora continuamente o status de cada módulo cadastrado
- **Gerenciamento Manual**: Adicione, edite ou remova módulos manualmente
- **Status em Tempo Real**: Visualização do estado (online/offline) de cada módulo
- **Teste de Conectividade**: Teste individual de conexão com cada módulo

### Parâmetros de Configuração

#### Habilitar Inter-Módulos
- **Descrição**: Ativa/desativa o sistema de comunicação entre módulos
- **Padrão**: Desabilitado
- **Nota**: Quando habilitado, o sistema inicia o monitoramento de healthcheck e permite descoberta automática

#### Intervalo de Health Check
- **Descrição**: Tempo entre verificações de saúde de cada módulo
- **Padrão**: 60 segundos
- **Valores recomendados**: 30-120 segundos
- **Função**: Envia requisição HTTP POST para `/api/healthcheck` em cada módulo cadastrado
- **Timeout**: 3 segundos por requisição
- **Delay entre módulos**: 300ms (evita sobrecarga de rede)

#### Máximo de Falhas
- **Descrição**: Número de healthchecks consecutivos falhados antes de marcar módulo como offline
- **Padrão**: 3 falhas
- **Valores recomendados**: 2-5 falhas
- **Comportamento**: Após atingir o máximo, o módulo é marcado como "Offline" na interface

#### Descoberta Automática
- **Descrição**: Habilita busca automática de novos módulos via mDNS
- **Padrão**: Desabilitado
- **Primeira execução**: 30 segundos após o boot (aguarda estabilização do WiFi)
- **Intervalo**: A cada 5 minutos (300 segundos)
- **Funcionamento**: 
  - Busca serviços `_http._tcp` na rede local
  - Valida TXT record `device_type=smcr`
  - Adiciona automaticamente módulos descobertos

### Gerenciamento de Módulos

#### Cadastro Manual
1. Clique em "Adicionar Módulo"
2. Informe:
   - **ID do Módulo**: Identificador único (hostname do dispositivo)
   - **Nome**: Nome amigável para exibição
   - **IP**: Endereço IP ou hostname (ex: `192.168.1.100` ou `esp32modular2.local`)
   - **Porta**: Porta do servidor web (padrão: 8080)
3. Clique em "Adicionar"

#### Edição de Módulos
- Clique no ícone de edição (✏️) ao lado do módulo
- Modifique os campos desejados
- Clique em "Salvar Alterações"

#### Remoção de Módulos
- Clique no ícone de lixeira (🗑️) ao lado do módulo
- Confirme a remoção

#### Teste de Conectividade
- Clique no botão "Testar" ao lado de cada módulo
- Verifica conectividade em tempo real
- Resultado exibido em modal (sucesso ou erro)

### Indicadores de Status

- **🟢 Online**: Módulo respondendo aos healthchecks
- **🔴 Offline**: Módulo não responde (máximo de falhas atingido)
- **Badge verde**: Última verificação bem-sucedida
- **Badge vermelho**: Falhas consecutivas detectadas

### Timings do Sistema

| Parâmetro | Valor Padrão | Descrição |
|-----------|--------------|-----------|
| Intervalo do Loop | 15 segundos | Frequência de execução das tasks inter-módulos |
| Healthcheck Interval | 60 segundos | Tempo entre verificações de saúde |
| HTTP Timeout | 3 segundos | Timeout para requisições HTTP |
| Delay entre Healthchecks | 300ms | Pausa entre verificação de cada módulo |
| Primeira Discovery | 30 segundos | Após boot (aguarda WiFi estabilizar) |
| Intervalo Discovery | 5 minutos | Frequência de busca automática |
| Máximo de Falhas | 3 | Falhas consecutivas antes de marcar offline |

### Endpoints API

O sistema expõe os seguintes endpoints REST:

- **GET /api/intermod/config**: Retorna configuração atual
- **POST /api/intermod/config**: Atualiza configuração
- **GET /api/intermod/modules**: Lista todos os módulos
- **POST /api/intermod/modules/add**: Adiciona novo módulo
- **DELETE /api/intermod/modules/delete**: Remove módulo por ID
- **POST /api/intermod/modules/test**: Testa conectividade com módulo
- **POST /api/intermod/discovery**: Força descoberta manual via mDNS
- **POST /api/healthcheck**: Endpoint para receber healthchecks de outros módulos

### Armazenamento

- **Arquivo**: `/intermod_config.json` (LittleFS)
- **NVS Keys**:
  - `imod_en`: Habilitar inter-módulos (bool)
  - `imod_hchk`: Intervalo de healthcheck (uint16_t, segundos)
  - `imod_mfail`: Máximo de falhas (uint8_t)
  - `imod_adisc`: Descoberta automática (bool)

### Detecção mDNS

O sistema utiliza mDNS para descoberta automática com os seguintes parâmetros:

- **Serviço**: `_http._tcp`
- **TXT Records**: 
  - `device_type=smcr` (identificação do tipo de dispositivo)
  - `device=SMCR` (nome do dispositivo)
  - `version=2.1.2` (versão do firmware)

### Troubleshooting

#### Módulos não são detectados automaticamente
- Verifique se "Descoberta Automática" está habilitada
- Confirme que todos os dispositivos estão na mesma rede local
- Aguarde pelo menos 30 segundos após o boot para primeira descoberta
- Verifique logs seriais com flag `LOG_INTERMOD` habilitada

#### Módulos aparecem como Offline
- Verifique conectividade de rede entre os dispositivos
- Confirme que a porta está correta (padrão: 8080)
- Teste manualmente clicando em "Testar"
- Aumente o "Intervalo de Health Check" se houver problemas de rede intermitentes
- Aumente "Máximo de Falhas" para redes instáveis

#### Performance degradada
- Reduza frequência de healthcheck (aumente intervalo para 90-120 segundos)
- Desabilite descoberta automática se não adicionar novos módulos frequentemente
- Verifique quantidade de módulos cadastrados (recomendado: até 10 módulos)

### Ações Inter-Módulos

O sistema permite criar ações que controlam pinos em módulos remotos, possibilitando automação distribuída entre múltiplos dispositivos.

#### Funcionamento

1. **Configuração de Módulos**: Cadastre os módulos destino no sistema Inter-Módulos
2. **Cadastro de Pinos Remotos**: No módulo destino, crie pinos virtuais (tipo REMOTO)
3. **Configuração de Ações**: No módulo origem, crie ações que referenciam:
   - **Pino Origem**: Gatilho local (sensor, botão, etc.)
   - **Pino Destino**: Atuador local (opcional)
   - **Módulo Remoto**: ID do módulo destino
   - **Pino Remoto**: Número do pino virtual no destino

#### Exemplo Prático

**Cenário**: Sensor de temperatura em MOD1 aciona ventilador em MOD2

**MOD1 (Origem)**:
- Pino 15: Sensor temperatura (INPUT, tipo DIGITAL)
- Ação configurada:
  - Pino origem: 15
  - Pino destino: 2 (LED local)
  - Módulo remoto: "esp32modular2"
  - Pino remoto: 201
  - Tipo ação: LIGA

**MOD2 (Destino)**:
- Pino 201: Pino virtual (tipo REMOTO, INPUT)
- Pino 2: Relé ventilador (OUTPUT)
- Ação configurada:
  - Pino origem: 201 (virtual)
  - Pino destino: 2 (relé)
  - Tipo ação: LIGA

#### Sincronização Bidirecional

O sistema sincroniza automaticamente **ambos os estados** (ON e OFF):

- **Ativação**: Quando pino origem = 1 → envia POST com value=1 para módulo remoto
- **Desativação**: Quando pino origem = 0 → envia POST com value=0 para módulo remoto

Isso garante que o pino virtual no destino sempre reflita o estado real do gatilho.

#### Endpoint de Atualização

**POST** `/api/pin/set`

**Parâmetros**:
- `pin`: Número do pino (ex: 201)
- `value`: Estado (0 ou 1)

**Comportamento**:
- **Pinos OUTPUT**: Executa `digitalWrite()` no GPIO físico
- **Pinos REMOTE**: Atualiza apenas `status_atual` (virtual)

#### Tipos de Pinos

1. **DIGITAL (tipo 1)**: GPIO físico, leitura via `digitalRead()`
2. **ANALOG (tipo 2)**: GPIO físico, leitura via `analogRead()`
3. **REMOTE (tipo 65534)**: Pino virtual, atualizado apenas via POST
   - Não executa leitura física
   - Usado como gatilho para ações locais
   - Estado mantido em `status_atual`
   - Suporta nível de acionamento (0 ou 1)

#### Fluxo Completo

```
MOD1: GPIO 15 acionado (1)
  ↓
MOD1: Executa ação local → GPIO 2 = HIGH
  ↓
MOD1: Envia POST → MOD2/api/pin/set?pin=201&value=1
  ↓
MOD2: Atualiza pino 201 status_atual = 1
  ↓
MOD2: Task de ações detecta pino 201 acionado
  ↓
MOD2: Executa ação local → GPIO 2 = HIGH

--- Quando normaliza ---

MOD1: GPIO 15 desativado (0)
  ↓
MOD1: Desliga ação local → GPIO 2 = LOW
  ↓
MOD1: Envia POST → MOD2/api/pin/set?pin=201&value=0
  ↓
MOD2: Atualiza pino 201 status_atual = 0
  ↓
MOD2: Task de ações detecta pino 201 desativado
  ↓
MOD2: Desliga ação local → GPIO 2 = LOW
```

#### Diagnóstico e Logs

Ao habilitar `LOG_ACTIONS` nos logs seriais, você verá:

```
[ACTION] Ação iniciada: Origem=15, Ação#1
[ACTION] LIGA: GPIO 2
[ACTION] Enviando para módulo 'esp32modular2', pino remoto 201, valor: 1
[ACTION] HTTP POST: http://192.168.1.100:8080/api/pin/set?pin=201&value=1
[ACTION] Resposta módulo remoto: OK
```

No módulo destino:

```
[WEB] POST /api/pin/set - pin=201, value=1
[WEB] Pino 201 (REMOTE) atualizado para: 1
[ACTION] Ação iniciada: Origem=201, Ação#1
[ACTION] LIGA: GPIO 2
```

### Boas Práticas

1. **Discovery Automático**: Habilite apenas durante configuração inicial ou quando adicionar novos dispositivos
2. **Healthcheck**: Use intervalo de 60 segundos para redes estáveis, 90-120 para redes congestionadas
3. **Nomenclatura**: Use IDs consistentes (mesmo hostname do dispositivo)
4. **Teste**: Sempre teste conectividade após adicionar novos módulos
5. **Backup**: Exporte configurações (Util > Preferências > Exportar) após configurar módulos
6. **Pinos Virtuais**: Use números altos (200+) para pinos remotos, evitando conflito com GPIOs físicos
7. **Ações em Cascata**: Configure ações locais nos módulos destino para processar pinos remotos
8. **Monitoramento**: Acompanhe dashboard para verificar status online dos módulos
