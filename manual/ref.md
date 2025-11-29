# Conceitos e termos do sistema SMCR.

- SMCR == Sistema de Monitoramento e Controle Remoto.<br>
- Pino == Porta
- Pino Origem == Porta Origem == Pino de Entrada == Porta de Entrada == Pino de Sensor (botão, reed switch, etc.).<br>
- Pino Destino == Porta Destino == Pino de Saída == Porta de Saída == Pino de Controle (buzzer, led, relé, etc.).<br>
- Cadastro de Pino == Cadastro de Porta
  - Tipo do Pino == Tipo de Porta
    - 0=Sem Uso
    - 1=Digital
    - 192=Analógico
    - 65534=Remoto
    - 65535=Reservado
  - Pino Mode()
    - 0=SEM USO
    - 1=INPUT
    - 3=OUTPUT
    - 4=PULLUP
    - 5=INPUT_PULLUP
    - 8=PULLDOWN
    - 9=INPUT_PULLDOWN
    - 10=OPEN_DRAIN
    - 12=OUTPUT_OPEN_DRAIN
    - 192=ANALOG
    - 254=PINO_VIRTUAL
    - 255=RESERVADO<br>
- Cadastro de Ações
  - Ação
    - 0=NENHUMA
    - 1=LIGA
    - 2=LIGA DELAY
    - 3=PISCA
    - 4=PULSO
    - 5=PULSO DELAY
    - 65534=SÓ ENVIO DE STATUS
    - 65535=RESERVADO

# Definição em esp32-hal-gpio.h
     //GPIO FUNCTIONS
     #define INPUT             0x01
     #define OUTPUT            0x03 
     #define PULLUP            0x04
     #define INPUT_PULLUP      0x05
     #define PULLDOWN          0x08
     #define INPUT_PULLDOWN    0x09
     #define OPEN_DRAIN        0x10
     #define OUTPUT_OPEN_DRAIN 0x12
     #define ANALOG            0xC0

     //Interrupt Modes
     #define DISABLED  0x00
     #define RISING    0x01
     #define FALLING   0x02
     #define CHANGE    0x03
     #define ONLOW     0x04
     #define ONHIGH    0x05
     #define ONLOW_WE  0x0C
     #define ONHIGH_WE 0x0D


# Definição dos pinos em pins_arduino.h
     static const uint8_t TX = 1;
     static const uint8_t RX = 3;

     static const uint8_t SDA = 21;
     static const uint8_t SCL = 22;

     static const uint8_t SS    = 5;
     static const uint8_t MOSI  = 23;
     static const uint8_t MISO  = 19;
     static const uint8_t SCK   = 18;

     static const uint8_t A0 = 36;
     static const uint8_t A3 = 39;
     static const uint8_t A4 = 32;
     static const uint8_t A5 = 33;
     static const uint8_t A6 = 34;
     static const uint8_t A7 = 35;
     static const uint8_t A10 = 4;
     static const uint8_t A11 = 0;
     static const uint8_t A12 = 2;
     static const uint8_t A13 = 15;
     static const uint8_t A14 = 13;
     static const uint8_t A15 = 12;
     static const uint8_t A16 = 14;
     static const uint8_t A17 = 27;
     static const uint8_t A18 = 25;
     static const uint8_t A19 = 26;

     static const uint8_t T0 = 4;
     static const uint8_t T1 = 0;
     static const uint8_t T2 = 2;
     static const uint8_t T3 = 15;
     static const uint8_t T4 = 13;
     static const uint8_t T5 = 12;
     static const uint8_t T6 = 14;
     static const uint8_t T7 = 27;
     static const uint8_t T8 = 33;
     static const uint8_t T9 = 32;

     static const uint8_t DAC1 = 25;
     static const uint8_t DAC2 = 26;

# Retorno Serial da Função f_enviaModulo()
     Usada para enviar informações entre os módulos de status dos pinos e envio de handshake.<br>
     Valor do campo CTRL_Resposta.
     
     "int f_enviaModulo(uint8_t idmodulo, String acao, String pino, String valor)"
     
     HTTPC_ERROR_CONNECTION_REFUSED  (-1)
     HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
     HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
     HTTPC_ERROR_NOT_CONNECTED       (-4)
     HTTPC_ERROR_CONNECTION_LOST     (-5)
     HTTPC_ERROR_NO_STREAM           (-6)
     HTTPC_ERROR_NO_HTTP_SERVER      (-7)
     HTTPC_ERROR_TOO_LESS_RAM        (-8)
     HTTPC_ERROR_ENCODING            (-9)
     HTTPC_ERROR_STREAM_WRITE        (-10)
     HTTPC_ERROR_READ_TIMEOUT        (-11)

# Retorno Serial da Função fV_setupMqtt()
       Usada para conexão com servidor broker mqtt.


     MQTT_CONNECTION_TIMEOUT     -4
     MQTT_CONNECTION_LOST        -3
     MQTT_CONNECT_FAILED         -2
     MQTT_DISCONNECTED           -1
     MQTT_CONNECTED               0
     MQTT_CONNECT_BAD_PROTOCOL    1
     MQTT_CONNECT_BAD_CLIENT_ID   2
     MQTT_CONNECT_UNAVAILABLE     3
     MQTT_CONNECT_BAD_CREDENTIALS 4
     MQTT_CONNECT_UNAUTHORIZED    5
     MQTTCONNECT     1 << 4  // Client request to connect to Server
     MQTTCONNACK     2 << 4  // Connect Acknowledgment
     MQTTPUBLISH     3 << 4  // Publish message
     MQTTPUBACK      4 << 4  // Publish Acknowledgment
     MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
     MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
     MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
     MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
     MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
     MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
     MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
     MQTTPINGREQ     12 << 4 // PING Request
     MQTTPINGRESP    13 << 4 // PING Response
     MQTTDISCONNECT  14 << 4 // Client is Disconnecting
     MQTTReserved    15 << 4 // Reserved

# Retorno Serial da Função fV_checkWifiConnection()
     Usada para conexão com wifi

     WL_NO_SHIELD        = 255   // for compatibility with WiFi Shield library
     WL_IDLE_STATUS      = 0
     WL_NO_SSID_AVAIL    = 1
     WL_SCAN_COMPLETED   = 2
     WL_CONNECTED        = 3
     WL_CONNECT_FAILED   = 4
     WL_CONNECTION_LOST  = 5
     WL_DISCONNECTED     = 6

## Tabela de Pinos do ESP32

| Pino GPIO | Entrada Digital | Saída Digital | Entrada Analógica (ADC) | Saída Analógica (DAC) |
|-----------|-----------------|---------------|------------------------|----------------------|
| 0         | Sim             | Sim           | ADC2                   | Não                  |
| 1 (TX)    | Sim             | Sim           | Não                    | Não                  |
| 2         | Sim             | Sim           | ADC2                   | Não                  |
| 3 (RX)    | Sim             | Sim           | Não                    | Não                  |
| 4         | Sim             | Sim           | ADC2                   | Não                  |
| 5         | Sim             | Sim           | Não                    | Não                  |
| 12        | Sim             | Sim           | ADC2                   | Não                  |
| 13        | Sim             | Sim           | ADC2                   | Não                  |
| 14        | Sim             | Sim           | ADC2                   | Não                  |
| 15        | Sim             | Sim           | ADC2                   | Não                  |
| 16        | Sim             | Sim           | Não                    | Não                  |
| 17        | Sim             | Sim           | Não                    | Não                  |
| 18        | Sim             | Sim           | Não                    | Não                  |
| 19        | Sim             | Sim           | Não                    | Não                  |
| 21        | Sim             | Sim           | Não                    | Não                  |
| 22        | Sim             | Sim           | Não                    | Não                  |
| 23        | Sim             | Sim           | Não                    | Não                  |
| 25        | Sim             | Sim           | ADC2                   | DAC1                 |
| 26        | Sim             | Sim           | ADC2                   | DAC2                 |
| 27        | Sim             | Sim           | ADC2                   | Não                  |
| 32        | Sim             | Sim           | ADC1                   | Não                  |
| 33        | Sim             | Sim           | ADC1                   | Não                  |
| 34        | Sim             | Não           | ADC1                   | Não                  |
| 35        | Sim             | Não           | ADC1                   | Não                  |
| 36        | Sim             | Não           | ADC1                   | Não                  |
| 39        | Sim             | Não           | ADC1                   | Não                  |

**Observações:**
- GPIOs 6 a 11 são reservados para flash interno, não utilizar.
- GPIOs 34 a 39 são apenas entrada (não suportam saída).
- ADC2 pode ter restrições quando WiFi está ativo.
- DAC disponível apenas nos GPIOs 25 e 26.