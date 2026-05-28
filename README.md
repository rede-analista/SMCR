# SMCR — Sistema Modular de Controle e Recursos

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Firmware para ESP32 que transforma o módulo em um controlador modular de sensores e acionamentos totalmente configurável pela interface web, sem necessidade de recompilar o código.

Faz parte do ecossistema SMCR:

| Projeto | Descrição |
|---------|-----------|
| **SMCR** | Este repositório — firmware ESP32 |
| [SMCR_CLOUD](https://github.com/rede-analista/SMCR_CLOUD) | Painel web de gerenciamento centralizado |
| [SMCR_HA](https://github.com/rede-analista/SMCR_HA) | Add-on para Home Assistant |

---

## Descrição

O objetivo é criar uma programação para ESP32 de forma que a definição dos pinos não seja estática dentro do código fonte, proporcionando a possibilidade de alterar a definição dos pinos por interface web e configurar ações com base no status dos pinos.

A maioria das configurações está disponível na interface web embutida no ESP32, permitindo configurar pinos, ações, rede, MQTT e muito mais sem precisar de cabo serial ou recompilar o firmware.

![Interface SMCR](https://github.com/rede-analista/SMCR/blob/main/manual/telas/t_top_0.png)

> **Nota:** Este projeto é desenvolvido por entusiasta, não por programador profissional. Não é garantida uma programação dentro das melhores práticas, mas contribuições e sugestões são bem-vindas.

---

## Recursos

### Rede
- Modo AP para configuração inicial
- Conexão WiFi (STA) com múltiplas tentativas
- mDNS habilitado — acesse por `http://<hostname>.local:<porta>/`
- Configuração de hostname, porta do servidor web, servidor NTP
- [Documentação de Rede](manual/rede.md)

### Pinos GPIO
- Até 254 pinos configuráveis (entradas e saídas)
- Nome, ID, tipo (digital/analógico), modo, nível de acionamento, retenção, XoR
- Renomeação inteligente: alterar o GPIO de um pino atualiza automaticamente todas as ações vinculadas
- [Documentação de Pinos](manual/pinos.md)

### Ações Automáticas
- Até 4 ações por pino
- Configuração em cascata (pino A aciona pino B, que aciona pino C...)
- Temporizadores, acionamentos, lógica condicional
- Comunicação com outros módulos ESP32 via MQTT
- [Documentação de Ações](manual/acoes.md)

### MQTT e Home Assistant
- Integração MQTT com auto-discovery do Home Assistant
- Tópicos:
  - Estados: `smcr/<ID_UNICO>/pin/<NUM_PINO>/state`
  - Comandos: `smcr/<ID_UNICO>/pin/<NUM_PINO>/set`
  - Discovery: `homeassistant/sensor/<ID_UNICO>_pin<NUM_PINO>/config`
- ID único baseado no MAC: formato `smcr_XXXXXXXXXXXX`
- Classe MQTT e ícone MDI configuráveis por pino (switch, light, sensor, binary_sensor...)
- Endpoints da API:
  - `GET /api/mqtt/config` — configuração atual
  - `GET /api/mqtt/status` — ID único e status da conexão
  - `POST /api/mqtt/save` — salva e reinicia

### Comunicação entre Módulos (Inter-módulo)
- Comunicação entre ESP32s via MQTT
- Acionamentos remotos entre dispositivos
- [Documentação Inter-módulos](manual/intermod.md)

### Notificações
- Telegram Bot integrado

### Gerenciamento de Arquivos (LittleFS)
- Upload, download e exclusão de arquivos via interface web
- Multi-upload com indicador de progresso
- Favicon customizável (`/favicon.ico` ou `/favicon.png`)
- [Documentação de Arquivos](manual/arquivos.md)

### NVS — Exportar/Importar
- Exportação de toda a configuração em JSON ou texto
- Importação com merge ou substituição completa
- Segredos mascarados na listagem por padrão

### OTA — Atualização de Firmware
- Upload de `.bin` via interface web (`http://<host>:<porta>/firmware`)
- Reinicialização automática após atualização
- [Como gravar o firmware](manual/gravafirmware.md)

### Interface Web
- Design responsivo e mobile-friendly
- Menu de navegação unificado em todas as páginas
- Indicador de tempo de carregamento em todas as páginas
- Zero dependências externas (sem CDN ou internet)

---

## Ambiente de Desenvolvimento

- **IDE:** VSCode + PlatformIO
- **Framework:** Arduino (ESP32)
- **SO:** Debian 12+
- **Bibliotecas:**
  ```
  bblanchon/ArduinoJson@^7.4.2
  esp32async/ESPAsyncWebServer
  esp32async/AsyncTCP
  knolleary/PubSubClient
  ```

---

## Compilação

```bash
platformio run                    # Apenas compilar
platformio run --target upload    # Compilar e gravar no ESP32
platformio device monitor         # Monitor serial (115200 baud)
```

> **Atenção:** Nunca use `platformio run --target uploadfs` — isso apaga toda a partição LittleFS (HTMLs e dados salvos na flash).

Os binários já compilados estão disponíveis em [`firmware/`](https://github.com/rede-analista/SMCR/tree/main/firmware) para gravação direta com ferramentas como `esptool`.

---

## Primeiro Acesso

Após gravar o firmware, o ESP32 inicia em modo AP com a rede:

- **SSID:** `esp32modularx Ponto de Acesso`
- **Senha:** `senha12345678`

Conecte-se à rede e acesse [http://192.168.4.1:8080/wifiinicio](http://192.168.4.1:8080/wifiinicio) para configurar o WiFi.

![Configuração WiFi inicial](https://github.com/rede-analista/SMCR/blob/main/manual/telas/c_wifi_inicial_t1.png)

Após configurar o WiFi, o módulo reinicia e se conecta à sua rede. Acesse pelo IP atribuído pelo roteador ou pelo hostname mDNS: `http://esp32modularx.local:8080/`

> Recomenda-se reservar IP fixo para o módulo no roteador, especialmente ao usar comunicação entre módulos.

---

## Limitações Conhecidas

- Flash com ~97% de uso — evitar adicionar strings longas ou novas features sem remover código existente
- Sem validação de entradas — configurações incorretas de pinagem podem causar travamentos ou danos ao ESP32
- Sem HTTPS — não recomendado em redes públicas ou desprotegidas
- Comunicação inter-módulo depende de broker MQTT disponível

---

## Documentação

- [Configuração de Rede](manual/rede.md)
- [Cadastro de Pinos](manual/pinos.md)
- [Configuração Geral](manual/configgeral.md)
- [Cadastro de Ações](manual/acoes.md)
- [Inter-módulos](manual/intermod.md)
- [Reset do Sistema](manual/reset.md)
- [Gravar Firmware](manual/gravafirmware.md)
- [Prints de Telas](manual/telas/prints.md)
- [Implementações Técnicas](manual/implementacoes/)

---

## Licença

[MIT](LICENSE) © Rede Analista
