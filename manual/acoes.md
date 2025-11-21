# Configuração de Ações

- Ações podem ser configuradas para que o múdulo execute uma funcionalidade, como acionar um led/relé, notificar quando um sensor for acionado, comunicar com um broker mqtt, etc.<br>

- Cada pino pode ter configurado até 4 ações, primeira ação, segunda ação, terceira ação e quarta ação.<br>

- Uma ação é algo que o ESP32 irá executar quando um evento ocorrer. Um evento ocorre quando a leitura de um pino cadastrado como entrada atingir ou ultrapassar o valor informado no cadastro de pinos. 
- Existe uma task no ESP32 que fica lendo as informação dos pinos de entrada, e este valor é comparado com o nível de acionamento.
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_acoes_t4.png)<br>
Quando o valor de um pino ṕe igual ou maior que o valor cadastrado no campo "Nível Acionamento:" o módulo entende que houve um alerta.

<br><br><br><br>

- Para configurar ações, clique na opção "Configurar Ações"na página inicial.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_acoes_t0.png)


- Será aberta a página com todas as informações de ações mas para cadastrar uma ação é necessário que já tenha pelo menos um pino cadastrado. O cadastro das ações pode ser realizada considerando a informação do "Pino Origem".<br>
<br>
- Pino Origem == Pino de entrada == Pino de sensor (botão, reed switch, etc.).<br>
Pino que irá disparar uma ação.<br>
<br>
- Pino Destino == Pino de saída == Pino de controle (buzzer, led, relé, etc.).<br>
Pino que irá sofrer uma ação.<br>
<br>
<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_acoes_t1.png)

NOTA 2: <strong>Se o módulo for reiniciado antes de salvar as informações na flash todas as configurações serão perdidas</strong>.  

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_acoes_t2.png)

  NOTA 1: Após configurar e clicar no botão "Aplicar (sem salvar)" as configurações já estarão sendo usadas pelo módulo mas não estarão salvas na flash. Isto pode ser usado para testar uma configuração sem alterar a configuração que está salva na flash.

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_acoes_t3.png)
  
- Parâmetro PINO ORIGEM
  - É o pino que irá iniciar a execução da ação com base no seu status.

- Parâmetro PINO DESTINO
  - É o pino que irá sofrer uma alteração durante a execução da ação.

- Parâmetro AÇÃO
  - É a ação que será executada, esta informação pode ser:
    - NENHUMA = 0
    - LIGA = 1
    - LIGA DELAY = 2
    - PISCA = 3
    - PULSO = 4
    - PULSO DELAY ON = 5
    - STATUS = 65534(reservada)
    - SINCRONISMO = 65535(reservada)

    NOTA 3: A ação 65534 é usada para atualizar o status de um pino virtual.[Veja TIPO](pinos.md)<br>
    NOTA 4: A ação 65535 é reservada para controle de status de comunicação entre módulos. [Veja CICLOS HANDSHAKE](intermod.md)

- Parâmetro TEMPO ON
  - Informa o tempo em contagem de ciclos em determinadas ações.  Este valor pode ser de 0 a 65535. 
    1. Se a ação for LIGA DELAY=2 aguarda a contagem de "Tempo ON" para executar a ação depois que o pino for acionado.
    2. Se a ação for PISCA=3 será a contagem de "Tempo ON" quando a saída(Pino Destino) ficara ligada.
    3. Se a ação for PULSO=4 será o tempo que a saída(Pino Destino) ficará acionada antes de desligar.
    4. Se a ação for PULSO DELAY ON=5 será o tempo que vai aguardar antes de acionar a saída(Pino Destino).(veja Tempo OFF)

- Parâmetro TEMPO OFF
  - Informa o tempo em contagem de ciclos em determinadas ações.  Este valor pode ser de 0 a 65535. 
    1. Se a ação for PISCA=3 será a contagem de "Tempo OFF" quando a saída(Pino Destino) ficara desligada.
    2. Se a ação for PULSO DELAY ON=5 será o tempo que vai aguardar antes de desligar a saída(Pino Destino).(veja Tempo ON)

- Parâmetro PINO_DESTINO_REMOTO_(MASK)
  - Informa qual pino será informado ao módulo remoto. Server como uma máscara, você pode usar em um módulo o pino 4 e em outro o pino 23. Normalmente usado com recurso de pino virtual. ([Veja](pinos.md).

- Parâmetro ENVIA PARA MÓDULO
  - Ativa o envio de informação de status do pino a outro módulo ESP32, esta informaçẽo pode ser:
    - 0 = NENHUM (não envia status)
    - 1 a 65533 esta numeração corresponde ao ID do módulo que será enviado o status dos pinos<br>

    NOTA 5: Antes de habilitar este item você deve ser configurar as informações do módulo receptor.[Veja Configurações Gerais](configgeral.md)

- Parâmetro ENVIA TELEGRAM
  - Se marcado, ativa as notificações desta ação para enviar ao Telegram.<br>
  
    NOTA 6: Antes de habilitar este item você configurar as informações do Telegram.[Veja Configurações Gerais](configgeral.md)

- Parâmetro NOT. ASSIST.
  - Se marcadom ativa as notificações desta ação para notificar aos assistentes.<br>
    NOTA 7: Antes de habilitar este item você configurar as informações dos Assistentes.[Veja Configurações Gerais](configgeral.md)<br>

- Parâmetro ENVIA MQTT
  - se marcado, ativa as notificações desta ação para envior a um MqTT.<br>
    NOTA 8: Antes de habilitar este item você configurar as informações do MQTT.[Veja Configurações Gerais](configgeral.md)

- Parâmetro CLASSE MQTT
  - É a informação de qual classe esta ação será informada ao servidor mqtt<br>
    Deve ser usado uma classe suportado pelo MqTT, mais informações podem ser consultadas em [MQTT](https://www.home-assistant.io/integrations/mqtt/)

- Parâmetro ÍCONE MQTT
  - É a informação de qual ícone esta ação será informada ao servidor mqtt<br>
    Deve ser usado um ícone suportado pelo MqTT, mais informações podem ser consultadas em [MQTT](https://mdi.bessarabov.com/)

  
 # Não esqueça de salvar as informações antes de reiniciar/desligar o módulo.
## Se o módulor for reiniciado antes de salvar as informações na flash as configurações realizadas serão perdidas.

- Se quiser salvar as informações na memória flash do ESP32, Vá para tela inicial clique em "Gerenciar Arquivos", despois clique em "Salvar Configurações".<br>
  Responda "Sim" à pergunta e clique em "Confirmar".<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_acoes_t5.png)

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t2.png)

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t3.png)