
## Informações do Inter Módulos

  - Parâmetro HABILITA INTER MÒDULOS
    - Habilita o envio de informações para um outro módulo pela rede wifi.<br>
      NOTA 1: Para habilitar esta opção os parâmetros referente aos módulos precisam ser informados.

      Normalmente o módulo que irá enviar as informações é considerado transmissor.<br>
      Normalmente o módulo que irá receber as informações é considerado receptor.<br>
      Um módulo pode enviar e receber informações silmuntâneamente.<br>

      ![image](https://github.com/rede-analista/smcr/assets/66534023/dad9435f-d5d1-4067-8ddc-a44d8a553e3c)


  - Parâmetro TEMPO HANDSHAKE
    - Informe o tempo em milisegundos que a função f_checkHandShake() será executada.
      Esta função enviar informações de handshake para outro(s) módulo(s), pode ser usada para identificar perda de comunicação entre módulos.

  - Parâmetro CICLOS HANDSHAKE
    - Informe o tempo em ciclos que um módulo ficará enviando informações entre eles, este valor é decrementado toda vez que a função "f_checkHandShake()" é executada, se o módulo não receber nenhuma informação durante um período este valor irá chegar a 0(zero) e irá colocar o status do pino ID-Pino_HandShake em 1(alto). Este processo pode ser usado parar gerar um alerta informando que houve perda de comunicação entre os módulos.

  - Parâmetro TEMPO DE VERIFICAÇÃO DAS AÇÕES
    - Informe o tempo em milisegundos que a função f_checkAcoesModulos() será executada.
      Esta é a funçã́o que coloca as informações dos sensores na fila f_regModHist() de envio para outros módulos.
      A função f_enviaAcoesModulos() é responsável pelo envido de informações entre os módulos.

  - Parâmetro NOME / IP / ID-Pino_HandShake / Porta do Receptor
    - Informe o nome do módulo remoto, esta informação é usada apenas para validação quando receber dados de outro ESP. Se o nome do módulo que enviou a informação não for igual a um desses campos os dados serão descartados.<br>
    NOTA 2: Observe que em cada nome/ip de um módulo há um número de "ID", esse ID será usado na configuração das ações no campo "Envia_Módulo".<br>
    No cadastro de ações deve ser informado o ID correspondente ao módulo que irá receber a informação.<br>
    
    ![image](https://github.com/rede-analista/smcr/assets/66534023/738abb75-3fd9-4049-a72f-443d5e67d776)


    No exemplo acima o módulo está configurado para aceitar informações de outros módulos que tenham os hostname "Medidor", "Aquario" ou "Gerador".<br>
    Estas informações também serão usadas caso queira enviar informações para outros módulos.<br><br>
    NOTA 3: Um módulo pode enviar e receber informações, ou seja, pode ser um transmissor e um receptor simultâneamente, basta efetuar o cadastro e configurar nas ações os ID que foram cadastrados nesta página.


  - Parâmetro IP
    - Informe o IP do módulo remoto, a placa que irá receber as informações.<br>
      NOTA 4: Deve ser usado o IP da sua rede usado pela placa que vai recebar a informação.

      O IP deve ser informado no seguinte formato: zzzzzzzzzzzz onde z são os valores em decimais de cada octeto que compõe o IP.<br>
      Quando for informado um IP deve sempre ser ter 3 dígitos em cada octeto sem digitar o .(ponto decimal), por exemplo, para o IP 10.10.1.7 (010.010.001.007) o valor informado no campo deverá ser 010010001007

      Outro exemplo, se quiser informar o IP 192.168.100.7 (192.168.100.007) deve ser digitado 192168100007


  - Parâmetro ID-Pino_HandShake
    - Informe um número entre 1 e 254, este será o ID do módulo remoto.<br>
      Esta informação será usada nas ações para informaqr para qual módulo deve ser enviado o valor do sensor.<br>
      Este ID também podeŕa ser usado para identificar perda de comunicação com o módulo remoto, para isto basta cadastrar um pino com o mesmo valor deste campo ID.


  - Parâmetro PORTA DO RECEPTOR
    - Informe a porta TCP(porta web) do módulo que irá receber as informações.

      Normalmente a placa que irá enviar as informações é considerada transmissora.<br>
      Normalmente a placa que irá receber as informações é considerada receptora.

      Para que a placa transmissora envie requisições de handshake o número da porta deve ser informado.<br>
      Para que a placa transmissora não envie requisições de handshake o número da porta deve ser 0 (zero).



- Não esqueça de aplicar as configurações antes de mudar de página cliancado em "Aplicar (sem salvar)".<br>
  ![image](https://github.com/rede-analista/smcr/assets/66534023/abacbd7c-fa5c-4005-b678-06e97bc20cff)


- Se quiser salvar as informações na memória flash do ESP, na tela inicial clique em "Parâmetros", despois clique em "Salvar na Flash".<br>
  Responda "Sim" à pergunta e clique em "Salvar na Flash".<br>
  
  ![image](https://github.com/rede-analista/smcr/assets/66534023/2c82afff-a3c7-4ba4-ab7e-398530c743da)    
