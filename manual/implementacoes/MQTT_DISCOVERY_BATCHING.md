# MQTT - Descoberta Home Assistant em Lotes

Data: 26/11/2025
Tipo: ⭐ Feature / ⚙️ Performance
Status: ✅ Implementado

## Resumo
A publicação de auto-discovery do Home Assistant passou a ser realizada em **lotes** e de forma **não bloqueante**. Isso reduz burst no broker, evita travamentos da UI e mantém a responsividade do sistema, especialmente com muitos pinos.

## Motivação
- Publicação de discovery de todos os pinos em uma única tacada causava bursts de mensagens, podendo estourar buffers ou deixar a página Web lenta ao abrir.
- Havia dependência de status sendo calculado junto da configuração, o que podia atrasar o carregamento de `/mqtt`.

## Solução
1. Discovery paginado por lotes, publicado dentro do `fV_mqttLoop()`:
   - Novas variáveis de controle: índice, último publish e contabilização por ciclo.
   - Publica até `N` entidades por ciclo, respeitando um intervalo mínimo.
2. Separação dos endpoints:
   - `GET /api/mqtt/config` → retorna somente config (rápido)
   - `GET /api/mqtt/status` → retorna `mqtt_unique_id` e `mqtt_status`
3. UI atualizada (`include/web_mqtt.h`):
   - Timeout e feedback visual no carregamento.
   - Campos configuráveis: "Discovery - Tamanho do Lote" e "Discovery - Intervalo (ms)".

## Configurações Novas (MainConfig_t)
- `vU8_mqttHaDiscoveryBatchSize` (uint8_t) — tamanho do lote (padrão 4)
- `vU16_mqttHaDiscoveryIntervalMs` (uint16_t) — intervalo entre lotes em ms (padrão 100)

Persistência (NVS):
- `mqtt_hab` — batch size
- `mqtt_haim` — intervalo (ms)

## API
- `POST /api/mqtt/save`
  - Campos aceitos: `mqtt_ha_batch`, `mqtt_ha_interval_ms` (além dos já existentes)
- `GET /api/mqtt/config`
  - Inclui `mqtt_ha_batch` e `mqtt_ha_interval_ms`
- `GET /api/mqtt/status`
  - Retorna `mqtt_unique_id` e `mqtt_status`

## Estrutura de Tópicos
- Estados: `smcr/<ID_UNICO>/pin/<NUM_PINO>/state`
- Comandos: `smcr/<ID_UNICO>/pin/<NUM_PINO>/set`
- Discovery: `homeassistant/sensor/<ID_UNICO>_pin<NUM_PINO>/config`

## Arquivos Modificados
- `src/mqtt_manager.cpp` — batch do discovery e loop não bloqueante
- `src/servidor_web.cpp` — split de endpoints e novos campos na API de config
- `include/web_mqtt.h` — novos campos e timeouts na UI
- `include/globals.h` — novos campos da config
- `src/preferences_manager.cpp` — persistência NVS dos novos campos

## Testes & Validação
1. Build e upload OK.
2. Abrir página `/mqtt` — carregamento rápido; status chega em seguida.
3. Ajustar lote/intervalo e aplicar: observar logs publicando discovery em blocos.
4. Verificar no Home Assistant entidades surgindo sem burst.
5. `mosquitto_sub` para inspecionar mensagens:
```bash
mosquitto_sub -h <broker> -u <user> -P <senha> -t 'smcr/#' -v
mosquitto_sub -h <broker> -u <user> -P <senha> -t 'homeassistant/#' -v
```

## Benefícios
- Menos burst no broker e melhor estabilidade.
- UI de configuração mais responsiva.
- Comportamento previsível mesmo com muitos pinos.
