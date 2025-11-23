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

  - [Execução de Funções](manual/funcoes.md):
    - Execução de funções, Visualização serial, reboot, pausar rotinas, tasks, formatação flash, recarregar funções, whatchdog, etc;

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

    1. ESP Async WebServer.
    2. ESP32MQTTClient.
    3. HttpClient.
    4. Universal Telegram Bot Library.
    5. esp8266-google-home-notifier.
    6. MQTTPubSubClient.
    7. Arduino_FreeRTOS_Library.
    8. ESPAsyncHTTPUpdateServer.
    9. asyncHTTPrequest.
     
  NOTA: Na pasta ["build"](https://github.com/rede-analista/SMCR/tree/main/firmware) deixarei o arquivo de fimware .bin já compilado caso queiram simplesmente realizar a gravação no ESP32 sem precisar compilar. Para gravação sem compílar pode usar qualquer ferramenta como exemplo "esptools".

# !!!! IMPORTANTE !!!!
A programação atual não faz nenhuma tratativa com as informações que são inseridas/configuradas, não há tratativas para informações com acentuação e também não há tratativas para configurações incorretas de pinagem.<br>
Desta forma, uma configuração incorreta pode causar travamentos, falhas ou ainda danificar o módulo ESP32, tenha cuidado e atenção para inserir as informações.


# Primeiro Acesso
[Consulte Configuração de Rede](manual/rede.md)
<br>Após realizar a gravação do programa no ESP32 e rebootar o módulo ele irá gerar uma rede wifi (modo AP) com o nome padrão default do módulo "esp32modularx Ponto de Acesso" a senha padrão é "senha12345678".<br>
![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/c_wifi_inicial_t1.png)

Conecte-se na rede e pelo navegador você deve acessar o endereço "http://192.168.4.1:8080/wifiinicio". ao abrir a página informe o nome e senha da sua rede wifi e salve as informações.<br>
![image](https://github.com/rede-analista/SMCR/blob/main/manual/telas/c_wifi_inicial_t2.png)


  - Após configurar o wifi o módulo irá reiniciar e conectar na rede wifi que você acabou de informar, a partir deste ponto você deve acessar o módulo pelo IP que foi atribuído ao módulo pelo seu roteador (ou servidor DHCP).<br>

  - A programação configura o recurso de mDNS, você pode tentar descobrir o IP do módulo com o comando "ping esp32modularx.local". Caso não tenha resultado você deverá conectar no seu roteador para identificar qual andereço IP o módulo recebeu.<br>
    - Por exemplo:
    - http://esp32modularx.local:8080  (esp32modularx é o nome default do módulo)

  - É recomendado realizar o recurso de reserva de IP do seu roteador para que o módulo sempre recebe o mesmo endereço IP. Isso é importante pois caso você configure comunicação entre módulos e o IP de uma das placas for alterado, a comunicação entre módulos pode não funcionar.

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

### Documentação Técnica
- **[Implementações](manual/implementacoes/)** - Documentação detalhada de todas as implementações técnicas, correções de bugs e novas funcionalidades

## 🛠️ Para Desenvolvedores

Se você está contribuindo para o projeto ou quer entender como funciona internamente, consulte a **[documentação técnica](manual/implementacoes/)** que contém:

- Detalhes de implementação de cada funcionalidade
- Análise de problemas encontrados e suas soluções  
- Arquivos modificados em cada implementação
- Procedimentos de teste e validação
- Histórico completo de mudanças técnicas