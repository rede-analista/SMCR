# Parâmetros
 - PINO ORGIEM: Será listado os pinos cadastrados no cadastro de portas.
 - PINO DESTINO: 1 a 255 (deve ser um pino que já foi realizado cadastro no cadastro de portas)
 - AÇÃO: 0=NENHUMA / 1=LIGA / 2=LIGA DELAY / 3=PISCA / 4=PULSO / 5=PULSO DELAY
 - TEMPO AÇÃO ON: 0 a 65535 (Tempo em ciclos)
 - TEMPO AÇÃO OFF: 0 a 65535 (Tempo em ciclos)
 - PINO DESTINO REMOTO MASK: 1 a 254 (Número do pino no módumo remoto)
 - ENVIA MÓDULO: 0=Não / 1 A 254=ID do Módulo(receptor) que será enviado o status do pino
 - ENVIA TELEGRAM: 0=Nao / 1=Sim
 - NOTIFICA ASSISTENTES: 0=Nao / 1=Sim
 - ENVIA MQTT:  0=Não / 1=Sim
 - CLASSE MQTT: [Veja](https://www.home-assistant.io/integrations/mqtt/)

# Exemplo de Cadastro de Ações

- Como exemplo pode usar os pinos [cadastrados anteriormente](excadpino.md).<br>
  - Pino Origem: 4 (físico)
    - Pino Destino: 18 (físico)
    - Ação: 3 (pisca)


    ![image](https://github.com/rede-analista/smcr/assets/66534023/30be4695-dd1e-4df3-90c1-545e41302ec2)



- Depois de preencher as informações clique em "Aplicar(sem salvar)".<br>
  Este exemplo cadastra uma ação porém não salva as informações da flash do ESP. Se o módulo for reiniciado antes de salvar as informações serão perdidas.<br>

  Esta ação cadastrada faz com que enquanto pino 4 estiver acionado(nível alto) o pino 18(led) irá ficar piscando.<br>

  Para testar alterne o pino 4 de nível baixo(GND) para nível alto(SEM GND).<br>

  NOTA 1: Observe que para o pino 18 não precisa de configuração de ação pois ele não dispara nenhuma ação, trabalha somente como o destino de ação do pino 4.<br>

- Se quiser salvar as informações na memória flash do ESP, na tela inicial clique em "Parâmetros", despois clique em "Salvar na Flash".<br>
  Responda "Sim" à pergunta e clique em "Salvar na Flash".<br>
  
  ![image](https://github.com/rede-analista/smcr/assets/66534023/2c82afff-a3c7-4ba4-ab7e-398530c743da)