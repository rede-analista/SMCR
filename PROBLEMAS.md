# SMCR
Programação de um sistema modular de sensores e acionamentos para ESP32.

  - Pinos:

    1. No cadastro de pinos simplificar botões de aplicar/salvar para não causar confusão sobre o funcionamento. Deixe o botão salvar para salvar as informações na flahs seguindo o formato que já está sendo usado.
    O botão aplicar alterar para Reiniciar.
    Atualmente o botão salvar gerar uma mensatgem de falha e não salva as configurações.

    2. Falha em alterar cadastro de pinos, não está salvando a informação do nível de acionamento quando alterado.

    3. Tamanho da variável do número do pino pois não aceita valor acima de 244.

- Inter-módulos

    1. Ao enviar o valor do pino precisa verificar qual é o status que está sendo enviado, o pino de origem ou destino para poder definir a lógica de acionamento remota.
