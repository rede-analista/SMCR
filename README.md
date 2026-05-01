# SMCR
Programação de um sistema modular de sensores e acionamentos para ESP32.

# Descrição:
O objetivo é criar uma programação para ESP32, de forma que a definição os pinos não seja estática dentro do código fonte, proporcionando a possibilidade de alterar a definição dos pinos por interface web e configurar ações com base no status dos pinos.

# Resumo:
A programação é feita de forma que a maioria das configurações estejam disponíveis em uma interface web no ESP32, para que o usuário possa acessar pelo navegador e realizar as configurações desejadas, salvando as informações em arquivos na flash do ESP32.<br>
Algumas das características são proporcionar comunicação com outros módulos ESP32, notificação por Telegram, comunicação com MqTT e notificação por assistentes.

A interface web pode disponibilizar até 254(uint8_t) posições de configurações de pinos, ou seja, você poderá configurar até 254 pinos como entrada ou saída. também pela interface poderá configurar até 4 ações para cada pino, estas ações serão executadas quando por exemplo um sensor for acionado. Na configuração de ações é possível realizar uma configuração em cascata por exemplo quando o pino 2 aciona o pino 4, o pino 4 aciona o pino 5 e assim por diante, há várias formas de realizar as configurações de ações, depende um pouco da imaginação, espero que as quantidades sejam suficientes para atender uma boa parte das necessidades.

![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/t_top_0.png)

**NOTA 1: Não sou programador e nem técnico em eletrônica, somente me interesso por programação, sendo assim, não garanto uma programação "bonita" e/ou dentro das melhores práticas, mas fique a vontade para trocar informações ou sugerir alguma melhoria.**

[Print de Telas](manual/telas/prints.md)

## Recursos:
- Recursos já disponíveis: 

  - [Recursos de Rede](manual/rede.md):
    - Configurações relacionadas da rede, modo AP, Wifi (ssid/senha), hostname, porta servidor web, tentativas de conexões, servidor NTP, etc.

  - [Recursos Gerais](manual/configgeral.md):
    - Configurações relacionadas a status, cores, temporizadores, whatchdog, quantidade de pinos, etc.

  - [Recursos Pinagem](manual/pinos.md):
    - Configurações relacionadas aos pinos, nome, id de pino, tipo, modo, XoR, nível de acionamento, retenção, etc.

  - [Recursos Ações](manual/acoes.md):
    - Configurações relacionadas as ações, pinos origem/destino, tipo de ação, temporizadores, acionamentos, MqTT, acionamento remoto, etc.

  - [Recursos Arquivos](manual/arquivos.md):
    - Configurações relacionadas a arquivos, listar, upload, doanload, salvar, deletar, atualizar firmware, etc.

  - [Conceitos, Definições e Códigos para Referências](manual/ref.md):

- Atividades/Recursos previstos:
  - Comando de voz por assistentes.

- Atividades/Recursos não previstos:
  - Não é previso comunicação usando ssl, não é aconselhado ser usado em uma rede pública ou redes desprotegidas, não usa acesso https.

# Ambiente Desenvolvimento:
- O desenvolvimento foi realizado no VSCode + platformio com sistema operacional Debian 12.1 Bookworm. Para compilação do sketch algumas bibliotecas precisam ser instaladas. Em meu ambiente tenho bibliotecas adicionais que não são necessárias para este sketch, vou listar aqui as que precisei instalar, porém, algumas já estavam instaladas em meu ambiente. Como cada ambiente tem sua peculiaridade acredito que vai precisar de algumas pesquisas para ajustar sua IDE para compilação.

  - Bibliotecas:

	bblanchon/ArduinoJson@^7.4.2
	esp32async/ESPAsyncWebServer
	esp32async/AsyncTCP
	knolleary/PubSubClient
     
  NOTA: Na pasta ["build"](https://github.com/rede-analista/SMCR/tree/main/firmware) deixarei o arquivo de fimware .bin já compilado caso queiram simplesmente realizar a gravação no ESP32 sem precisar compilar. Para gravação sem compílar pode usar qualquer ferramenta como exemplo "esptools".

# !!!! IMPORTANTE !!!!
A programação atual não faz nenhuma tratativa com as informações que são inseridas/configuradas, não há tratativas para informações com acentuação e também não há tratativas para configurações incorretas de pinagem.<br>
Desta forma, uma configuração incorreta pode causar travamentos, falhas ou ainda danificar o módulo ESP32, tenha cuidado e atenção para inserir as informações.


# Primeiro Acesso
[Consulte Configuração de Rede](manual/rede.md)
<br>Após realizar a gravação do programa no ESP32 e rebootar o módulo ele irá gerar uma rede wifi (modo AP) com o nome padrão default do módulo "esp32modularx Ponto de Acesso" a senha padrão é "senha12345678".<br>
![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/c_wifi_inicial_t1.png)

Conecte-se na rede e pelo navegador você deve acessar o endereço [Novo Módulo](http://192.168.4.1:8080/wifiinicio). ao abrir a página informe o nome e senha da sua rede wifi e salve as informações.<br>
![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/c_wifi_inicial_t2.png)


  - Após configurar o wifi o módulo irá reiniciar e conectar na rede wifi que você acabou de informar, a partir deste ponto você deve acessar o módulo pelo IP que foi atribuído ao módulo pelo seu roteador (ou servidor DHCP).<br>

  - **mDNS Habilitado**: O sistema configura automaticamente o mDNS ao conectar no WiFi. Você pode acessar o módulo usando o hostname configurado:
    - Navegador: `http://esp32modularx.local:8080/` (esp32modularx é o nome padrão)
    - Terminal: `ping esp32modularx.local` ou `avahi-browse -a` (Linux)
    - O mDNS é reiniciado automaticamente após reconexões WiFi
    - Caso não funcione, verifique se seu roteador suporta mDNS/Bonjour ou conecte no roteador para identificar o IP atribuído

  - É recomendado realizar o recurso de reserva de IP do seu roteador para que o módulo sempre recebe o mesmo endereço IP. Isso é importante pois caso você configure comunicação entre módulos e o IP de uma das placas for alterado, a comunicação entre módulos pode não funcionar.

## ⚡ Recursos de Interface

### Indicador de Performance
Todas as páginas web do sistema exibem um **indicador de tempo de carregamento** no canto inferior direito:
- Mostra o tempo preciso que a página levou para carregar (em milissegundos)
- Aparece automaticamente após o carregamento completo
- Útil para monitorar a performance do ESP32 em tempo real
- Também registra o tempo no console do navegador para debugging

### Interface Moderna
- Design responsivo e mobile-friendly
- Menu de navegação unificado em todas as páginas
- Dropdowns nativos HTML5 (sem JavaScript)
- Zero dependências externas (sem CDN ou internet necessária)
- Botões com visual moderno e efeitos hover

### Favicon (Ícone do Site)
- O servidor web atende `/favicon.ico` a partir do LittleFS.
- Para evitar 404 e customizar o ícone, envie `favicon.ico` (recomendado) ou `favicon.png` via página `Serviços > Arquivos (LittleFS)`.
- Nome exato dos arquivos: `/favicon.ico` ou `/favicon.png`.
- Se nenhum arquivo existir, o servidor responde `204 No Content` para evitar erro no console.

## 📚 Documentação

### Manual do Usuário
- **[Configuração de Rede](manual/rede.md)** - Como configurar WiFi e acesso inicial
- **[Cadastro de Pinos](manual/pinos.md)** - Como configurar pinos GPIO
- **[Configuração Geral](manual/configgeral.md)** - Configurações globais do sistema
- **[Cadastro de Ações](manual/acoes.md)** - Como criar ações automáticas
- **[Inter Módulos](manual/intermod.md)** - Comunicação entre ESP32s
- **[Reset do Sistema](manual/reset.md)** - Procedimentos de reset
- **[Gravar Firmware](manual/gravafirmware.md)** - Como atualizar o firmware
- **[Prints de Telas](manual/telas/prints.md)** - Screenshots da interface

### MQTT e Home Assistant
- Integração MQTT com auto-discovery do Home Assistant.
- Estrutura de tópicos:
  - Estados de pinos: `smcr/<ID_UNICO>/pin/<NUM_PINO>/state`
  - Comandos de pinos: `smcr/<ID_UNICO>/pin/<NUM_PINO>/set`
  - Discovery HA: `homeassistant/sensor/<ID_UNICO>_pin<NUM_PINO>/config`
- ID único do módulo: baseado no MAC, formato `smcr_XXXXXXXXXXXX`.
- Página Web “MQTT/Serviços”: permite configurar broker, intervalo de publicação, auto-discovery e agora também “Discovery - Tamanho do Lote” e “Discovery - Intervalo (ms)”.
- Endpoints:
  - `GET /api/mqtt/config` → apenas dados de configuração (leve)
  - `GET /api/mqtt/status` → `mqtt_unique_id` e `mqtt_status`
  - `POST /api/mqtt/save` → salva e reinicia via UI
- Exemplos de teste via terminal:
```bash
mosquitto_sub -h <broker> -u <user> -P <senha> -t 'smcr/#' -v
mosquitto_sub -h <broker> -u <user> -P <senha> -t 'homeassistant/#' -v
```
 
#### Definição de Classe e Ícone (Discovery)
Agora configurados diretamente no cadastro de pinos:
- Classe MQTT (`switch`, `light`, `sensor`, `binary_sensor`, etc.) define o tipo da entidade no Home Assistant.
- Ícone MDI (`mdi:light-switch`, `mdi:led-on`, etc.) personaliza a exibição.
- Se não definidos: classe padrão `sensor` e ícone baseado no tipo (digital/analógico).
- Ações não possuem mais campos de MQTT; elas apenas disparam lógica interna.

### Renomeação Inteligente de Pinos
- Alterar o número (GPIO) de um pino na página de PINOS atualiza automaticamente todas as ações que referenciam esse pino.
- Persistência automática: sempre salva pinos; salva ações somente se houve renomeação.
- Resposta da API inclui detalhes (`renamed`, `old_pin`, `new_pin`, ações atualizadas e `persisted`).
- Evita recriação manual de cadeias de ações em configurações complexas.

### Multi-Upload de Arquivos (LittleFS)
- Campo de upload agora aceita múltiplos arquivos.
- Envio sequencial com indicação de progresso por arquivo.
- Usa mesma rota `/api/files/upload` para cada arquivo (compatibilidade mantida, sem impacto em memória).
 - Suporte a barra de progresso agregada e cancelamento em lote.

### Aviso: Operações em Massa de Arquivos
⚠️ Executar uploads, downloads ou deleções de muitos arquivos em sequência pode causar uso elevado de memória, lentidão ou reinício automático do ESP32. Recomenda-se:
- Processar em lotes pequenos (5–10 arquivos)
- Aguardar alguns segundos entre lotes maiores
- Evitar iniciar novos uploads enquanto uma sequência de deleções ou downloads ainda ocorre
- Verificar integridade de arquivos críticos (ex: configs de pinos/ações) após reinício inesperado

Veja detalhes e boas práticas em `manual/arquivos.md`.

### NVS: Exportar/Importar
- Página "Arquivos": agora lista todas as chaves do NVS (todas as namespaces) com segredos mascarados por padrão.
- Exportar NVS:
  - JSON: `GET /api/nvs/export` (arquivo `nvs_export_<HOSTNAME>_<millis>.json`)
  - Texto: `GET /api/nvs/export?format=text`
  - Incluir segredos: `include_secrets=1`
- Importar NVS (mesmo formato do export JSON):
  - `POST /api/nvs/import` (merge)
  - `POST /api/nvs/import?clear=1` (apaga namespaces presentes no arquivo antes de importar)
- Retorno do import: `{ ok, skipped, errors, cleared }`.

### Atualizar Firmware (OTA)
- Página "Firmware": acesse `http://<host>:<porta>/firmware`.
- Selecione o arquivo `.bin` gerado pelo build do SMCR e clique em "Enviar e Atualizar".
- O dispositivo aplica o update e reinicia automaticamente. Não desligue durante o processo.
- Observações:
  - Use binários construídos para a mesma placa/partições do projeto.
  - Após reiniciar, o dashboard volta a responder no mesmo endereço.

### Documentação Técnica
- **[Implementações](manual/implementacoes/)** - Documentação detalhada de todas as implementações técnicas, correções de bugs e novas funcionalidades

## 🛠️ Para Desenvolvedores

Se você está contribuindo para o projeto ou quer entender como funciona internamente, consulte a **[documentação técnica](manual/implementacoes/)** que contém:

- Detalhes de implementação de cada funcionalidade
- Análise de problemas encontrados e suas soluções  
- Arquivos modificados em cada implementação
- Procedimentos de teste e validação
- Histórico completo de mudanças técnicas