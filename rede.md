# Funcionamento e Configurações de Rede

No momento inicial(primeiro boot) após a gravação do firmware, o módulo vem com a função "Modo AP" ativada, esta função proporciona uma facilidade de conectar e configurar o ssid e senha para que o módulo consiga se conectar na rede wifi que o módulo vai usar.<br>

## Funcionamento no Modo AP (quando ativado)
- Ao ligar o módulo ele carrega as informações de rede que estão na flash e tenta se conectar na rede wifi que foi configurada, caso não consiga em 4 tentativas, ele aciona o modo AP, neste caso o módulo vai gerar uma rede wifi própria para que você possa se conectar no módulo e realizar a configuração básica do wifi.<br>
Se a configuração default não foi alterada o nome da rede própria do módulo será "esp32modularx Ponto de Acesso" e a senha para se conectar nesta rede será "senha12345678".<br>

  - Conecte-se na rede wifi.<br>
  - Abra o browser e digite o endereço "http://192.168.4.1:8080/wifiinicio".<br>
  - Ao abrir a página informe o nome e senha da sua rede wifi e salve as informações.<br>
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_wifi_inicial_t1.png)


  - Depois de salvar as informações o módulo irá reiniciar e conectar na sua rede wifi.<br>
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_wifi_inicial_t2.png)  

  NOTA 1:
  - Após configurar o wifi o módulo irá reiniciar e conectar na rede wifi que você acabou de informar, a partir deste ponto você deve acessar o módulo pelo IP que foi atribuído pelo seu roteador.<br>
  - A programação configura o recurso de mDNS, você pode tentar descobrir o IP do módulo com o comando "ping esp32modularx.local". Caso não tenha resultado você deverá conectar no seu roteador para identificar qual andereço IP o módulo recebeu.<br>
  - É recomendado realizar o recurso de reserva de IP do seu roteador para que o módulo sempre receba o mesmo endereço IP. Isso é importante pois caso você configure comunicação entre módulos e o IP de um dos módulos mudar, a comunicação entre módulos pode não funcionar.



## Configurações de Rede

- Depois que já estiver realizado a configuração inicial do wifi e já puder acessar o módulo pelo navegador, voce pode acessar mais configurações de rede. Para isto na tela inicial acesse o menu de "Configurações Gerais" e depois "Configuração de Rede".<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_geral_t1.png) 
<br>
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_rede_t2.png) 
<br>
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_wifi_t1.png) 

- Habilita Modo AP
  - Se estiver marcado o módulo irá gerar uma rede wifi própria caso não consiga se conectar no wifi que está configurada.

- Parâmetro SSID
  - É a informação do nome da rede wifi que o módulo vai usar para se conectar durante o funcionamento normal(rede de trabalho/nome do seu wifi).
    
- Parâmetro SENHA
  - É a informação da senha da rede wifi que o módulo vai usar para se conectar durante o funcionamento normal(rede de trabalho/nome do seu wifi).
  
- Parâmetro HOSTNAME  (mDNS)
  - É a informação de nome que o módulo vai usar para configuração de rede, este nome deve ser único na sua rede para não gerar conflito. O Hostname pode ser usado para acessar o módulo sem que você saiba qual foi o IP que o módulo recebeu da sua rede wifi.<br>

  Você pode acessar o módulo pelo navegador usando o seguinte formato "http://" + "hostname" + ".local" + ":" + "porta"<br>
  - Alguns exemplos:
    - http://esp32modularx.local:8080
    - http://192.168.50.10:8080

  O módulo usa o serviço de mDNS para que o acesso por nome funcione na sua rede local.

  NOTA 2: Como sugestão você pode identificar qual é o IP que o módulo está usando na sua rede atraves do seu roteador ou apenas executando o comando "ping esp32modularx.local", a informação de IP pode ser usada para acessar o módulo caso a resolução de nomes não funcione corretamente.<br><br>
  NOTA 3: Ao ativar o recurso de notificação por assistentes o serviço de mDNS não resolve o nome do módulo, sendo assim a informação de IP da "NOTA 1" deve ser usado para acessar o módulo.

- Parâmetro Porta do Servidor WEB
  - É a informação de qual porta tcp será usada para que você consiga acessar o módulo, o padrão é a porta 8080.<br>

- Parâmetro Tentativas de Conexão do Wifi
  - É a informação de quantas tentativas o módulo vai executar para se conectar a serviços remotos durante o boot, por exemplo, conectar no wifi, conectar no MqTT, conectar em um assistente, etc. <br>
  - Após essa quantidade de tentativas o módulo continua a carregar as configurações deixando o serviço que falhou sem ser utilizado.<br>

    NOTA 4: Este parâmetro foi implementado para que o módulo não fique "preso" durante o boot por falta de conexão com algum serviço externo que o impeça de iniciar o funcionamento local do módulo.

- Parâmetro Nome de Usuário WEB
  - É a informação de usuário para acessar módulo pela interface web caso a autenticação esteja ativada.<br>

- Parâmetro Senha WEB
  - É a informação de senha para acessar módulo pela interface web caso a autenticação esteja ativada.<br>

- Parâmetro Servidor 1 NTP
  - É a informação de servidor NTP para ser usado no sincronismo de horário do módulo.

- Parâmetro Servidor 2 NTP
  - É a informação de servidor NTP para ser usado no sincronismo de horário do módulo.



# Não esqueça de salvar as informações antes de reiniciar/desligar o módulo.
## Se o módulor for reiniciado antes de salvar as informações na flash as configurações realizadas serão perdidas.

- Se quiser salvar as informações na memória flash do módulo, Vá para tela inicial clique em "Gerenciar Arquivos", despois clique em "Salvar Configurações".<br>
  Responda "Sim" à pergunta e clique em "Confirmar".<br>
  
![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t1.png)

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t2.png)

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t3.png)