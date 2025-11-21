# Configurações dos pinos

- Você deve configurar os pinos do módulo como entrada ou saída, para isto, na página inicial clique na opção "Configurar Pinos".<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_pinos_t0.png)


- Será aberta a página com as informações de todos os pinos. Nesta página pode chegar a 254(uint8_t) posições para configurações dos pinos, o cadastro de um pino pode ser realizado em qualquer posição.<br>
Por padrão o módulo vem configurado com 15 pinos.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_pinos_tm.png)

- Escolha uma posição e clique para realizar o cadastro.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_pinos_t1.png)


- Após inserir as informações clique em "Aplicar (sem salvar)".

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_pinos_t2.png)

O total de pinos pode ser ajustado no cadastro geral. Em testes realizados o valor máximo atingido com boa performance foi de 48 pinos, porém, o sistema aceita o valor até 254.<br>


NOTA 1: Para não limitar a configuração dos pinos não há uma validação de quais pinos podem ser usados, consulte a informações de pinout do ESP32 para identificar pinos reservados e que não devem ser usados nas configurações nesta página.<br>

NOTA 2: Após configurar e clicar no botão "Aplicar (sem salvar)" as configurações já estarão sendo usadas pelo módulo mas **não estarão salvas na flash**. Isto pode ser usado para testar uma configuração sem alterar a configuração que está salva na flash.<br>

NOTA 3: Se o módulo for reiniciado antes de salvar as informações na flash todas as configurações serão perdidas.<br>

- Parâmetro NOME
  - É uma nomenclatura para facilitar na identificação dos pinos, este nome será usado na caso de notificações dos assistentes.

- Parâmetro PINO
  - É a informação do pino físico do chip, aqui será feito a associação do pino físico na placa ESP.<br>
    Esta informação será usada na configuração de Ações(eventos).<br>
    Pode ser um número que corresponde ao pino físico do chip, por exemplo, este número pode ser 2,4,5,12... ou pode ser um número que não corresponda (ver conceito de pino virtual)) ao pino físico do chip, por exemplo, ...251,253,65534<br>

    NOTA 4: Número de pinos que não são reconhecidos pelo chip são chamados de pinos virtuais.<br>

    NOTA 5: O Pino 65535 é reservado para controle interno de status de comunicação entre módulos e não deve ser usado para cadastro de pino.

    NOTA 6: O Pino 65534 deve ser usado para indicar um pino virtual (Pino que não existe no chip).<br>

- Parâmetro TIPO
  - É o tipo do pino, esta informação pode ser:
    - 0 = Sem Uso
    - 1 = Digital
    - 192 = Analógico
    - 65534 = Virtual
  
    - Se o valor 65534 for configurado significa que é um pino virtual(ver conceito). O módulo não irá realizar a atualização de status do pino(local) pela task, esta configuração pode ser usada quando for habilitado o recurso de "Inter Módulos" por exemplo.<br>

    - O recurso de Inter Módulos ativa a comunicação entre dois ou mais módulos onde um módulo transmissor irá atualizar o status de um pino(físico ou virtual) no módulo receptor.<br>
    
    - O cadastro de um pino como tipo 65534 deve ser usado quando o módulo vai receber o status deste pino de forma remota, o status será recebido de outro módulo para este módulo que terá o pino cadastrado como tipo 65534. Quando for cadastrar um pino como 65534 pode ser usado qualquer númeração de pino entre 1 e 65534 pois um pino do tipo 65534 será considerado um pino virtual(não físico do chip) e servirá para disparar ações no módulo que recebe os dados de status de outros módulos. Este recurso pode ser usado para não inutilizar um pino físico do chip em uma ação que não teŕa leitura de sensor localmente.

**Pino virtual (clique na seta abaixo para mais detalhes)**
<details>
<summary>- O conceito de pino virtual foi introduzido com a intenção de aumentar a quantidade de configurações possíveis e também evitar "usar" um pino físico em configurações que o pino físico não será útil.</summary>

- Considere um ambiente em que tem vários módulos com várias funções, neste tipo de ambiente facilmente você pode se impedido de usar pinos iguais entre módulos devido a conflito de identificação de pinos, ou ainda ter poucos pinos disponíveis para realizar suas configurações.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_top_0.png)

<br>
<br>
<br>
<br>
  - Imagine de você possui um módulo que tem um botão e um buzzer como se fosse uma campainha no portão.<br>
  - Imagine que também possui um segundo módulo que fica dentro de casa para receber a informação que a campainha foi acionada.<br>
<br>
<br>
<br>
<br>
<br>
<br>
- Exemplo de funcionamento **SEM USAR** o recurso de pino virtual.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_top_3.png)


<br>
<br>  
  - Veja que o pino de origem precisa ser o mesmo nas duas placas. Considerando que uma placa esp possui em média 25 pinos GPIO, poderíamos ter no máximo 12 pinos de entrada e 12 pinos de saída póis para cada saída a ŕincípio teríamos uma entrada que vai gerar o disparo.<br>
    - Pino Origem == Pino de entrada == Pino de sensor (botão, reed switch, etc.).<br>
    - Pino Destino == Pino de saída == Pino de controle (buzzer, led, relé, etc.).<br>
  - Para acionar uma saída(pino destino) é preciso ter uma entrada(pino origem).<br>
  - Quando o botão for acionado, o transmissor vai enviar a informação que o pino 4 foi acionado para o receptor. O receptor recebe a informação do pino 4 acionado e também aciona o pino 23.<br>
  - Neste caso o pino 4 do módulo receptor **NÃO PODERÁ** mais ser usado para outra função mesmo que não tenha nehum sensor físico associado ao pino 4.<br>
  - Se quiser configurar um módulo como central onde não teŕa nenhum sensor conectado na central poderá ter apenas 12 pinos de saídas para indicar alertas pois outros 12 pinos serão usados como entrada recebendo status de outros módulos transmissores.<br>
<br>
<br>
<br>
<br>
<br>
<br>
- Exemplo de funcionamento **USANDO** o recurso de pino virtual.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_top_4.png)

 
  - Veja agora que o pino de origem não precisa ser o mesmo nas duas placas, porém, você não está usando um número de pino físico válido. Neste caso todas as GPIO poderão ser usadas como saídas (destinos) para informar alertas.<br>
    - Pino Origem == Pino de entrada == Pino de sensor (botão, reed switch, etc.).<br>
    - Pino Destino == Pino de saída == Pino de controle (buzzer, led, relé, etc.).<br>
  - Para acionar uma saída(pino destino) é preciso ter uma entrada(pino origem).<br>
  - Quando o botão for acionado, o transmissor vai enviar a informação que o pino 200 foi acionado para o receptor. O receptor recebe a informação do pino 200 acionado e também aciona o pino 23.<br>
  - Neste caso o pino 4 do módulo receptor **PODERÁ** mais ser usado para outra função.<br>
  - Se quiser configurar um módulo como central onde não terá nenhum sensor conectado na central poderá usar todos os pinos como saídas para indicar alertas pois os pinos de entrada serão virtuais.<br>
<br>
<br>
<br>
<br>
<br>
<br>
- Exemplo de funcionamento USANDO o recurso de pino virtual com 2 módulos transmissores.<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_top_5.png)


  - Veja que o pino de origem pode ser o mesmo nas duas placas, porém, você não está usando um número de pino físico válido. Neste caso todas as GPIO poderão ser usadas como saídas (destinos) para informar alertas.<br>
    - Pino Origem == Pino de entrada == Pino de sensor (botão, reed switch, etc.).<br>
    - Pino Destino == Pino de saída == Pino de controle (buzzer, led, relé, etc.).<br>
  - Para acionar uma saída(pino destino) é preciso ter uma entrada(pino origem).<br>
  - Note que não precisa haver relação entre os pinos das placas transmissores e receptoras. Precisa apenas ficar atento ao conflito de pinos em uma mesma placa.
<br>
<br>
<br>
<br>
</details>

- Parâmetro PINMODE()
  - É o modo que o pino será configurado na função pinMode(), esta informação pode ser:
    - INPUT = 1
    - OUTPUT = 3
    - PULLUP = 4
    - INPUT_PULLUP = 5
    - PULLDOWN = 8
    - INPUT_PULLDOWN = 9
    - OPEN_DRAIN = 10
    - OUTPUT_OPEN_DRAIN = 12
    - ANALOG = 192
    - VIRTUAL = 0

    - Se o pino for do tipo 65534 este valor não será considerado, pode ser 0(zero).

- Parâmetro XOR
  - Indica se deve ser aplicado a operação XoR quando realizar a leitura do status do pino, esta informação pode ser:
    - 0 = Valor igual a leitura do pino
    - 1 = Valor inverso a leitura do pino

- Parâmetro NÍVEL_ACIONAMENTO
  - Informa se a ação cadastrada para este pino será executada quando o pino estiver acima ou igual ao nível cadastrado, este valor pode ser:
    - 0 a 4095

- Parâmetro TEMPO RETENÇÃO
  - Informa quantos clicos a task deixará de ler o status de um pino após um acionamento, esta informação pode ser:
    - 0 = Sem retenção
    - 1 a 65535 = Com Retenção

    - O tempo de retenção não é contado em milisegundos mas sim em tempo de clico do processador, para mais detalhe do tempo de retenção veja [Veja CICLOS HANDSHAKE](intermod.md)


 # Não esqueça de salvar as informações antes de reiniciar/desligar o módulo.
## Se o módulor for reiniciado antes de salvar as informações na flash as configurações realizadas serão perdidas.

- Se quiser salvar as informações na memória flash do ESP32, Vá para tela inicial clique em "Gerenciar Arquivos", despois clique em "Salvar Configurações".<br>
  Responda "Sim" à pergunta e clique em "Confirmar".<br>

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/c_pinos_t3.png)

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t2.png)

![image](https://github.com/rede-analista/smcr/blob/develop/manual/telas/t_salvar_t3.png)