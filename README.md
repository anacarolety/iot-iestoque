# IEstoque: Controle de Estoque de Medicamentos com RFID e IoT

## Descrição do Funcionamento e Uso
Este projeto visa facilitar a gestão de estoque de medicamentos em ambientes hospitalares por meio da tecnologia IoT. Ele utiliza tags RFID para identificar medicamentos e um controlador NodeMCU para enviar e receber dados do servidor MQTT, permitindo o monitoramento em tempo real do estoque. O buzzer é acionado quando o estoque de um medicamento está baixo, indicando a necessidade de reposição.

Para reproduzir, siga as instruções de montagem e instale o software conforme documentação.

## Software Desenvolvido e Documentação de Código
O código-fonte disponível inclui comentários detalhados para facilitar o entendimento e a modificação do sistema.

## Hardware Utilizado
- Plataforma de desenvolvimento: NodeMCU ESP8266
- Sensores: Leitor RFID MFRC522
- Atuadores: Buzzer 5V Ativo
- Outros componentes: Protoboard, tags RFID, fios de conexão

## Interfaces, Protocolos e Módulos de Comunicação
O projeto utiliza o protocolo MQTT para comunicação entre o NodeMCU e o servidor MQTT. Os tópicos MQTT definidos são "esp8266/cadastro", "esp8266/cadastro-medicamento", "esp8266/estoque" e "esp8266/estoque-saida" para o cadastro e atualização de estoque.

