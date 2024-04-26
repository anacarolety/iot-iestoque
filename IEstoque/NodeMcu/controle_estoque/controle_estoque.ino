#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

//RFID
#define SS_PIN D8
#define RST_PIN D0


MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

// WiFi
const char *ssid = "ESP01"; // Enter your WiFi name
const char *password = "playstation23";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topicEstoque = "esp8266/estoque";
const char *topicEstoqueSaida = "esp8266/estoque-saida";
const char *topicCadastro = "esp8266/cadastro-medicamento";
const char *topicCadastroRetorno = "esp8266/cadastro-medicamento-retorno";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

//Buzzer
int PIN_BUZZER_1 = D4;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi.. ");
  }
  Serial.println("Connected to the WiFi network ");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  pinMode(PIN_BUZZER_1, OUTPUT);
  while (!client.connected()) {
      String client_id = "mqttx_541692d6";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public emqx mqtt broker connected ");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
  
  //subscribe
  client.subscribe(topicEstoqueSaida);
  client.subscribe(topicCadastro);

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
 
  Serial.println();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println();
  Serial.println();

}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic,"esp8266/estoque-saida")==0){
    callbackEstoqueSaida(topic,payload, length);
  }

  if (strcmp(topic,"esp8266/cadastro-medicamento")==0) {
    callbackCadastro(topic,payload, length);
  }
  
}

void callbackEstoqueSaida(char* topic, byte* payload, unsigned int length) {
  Serial.print(" Message arrived in topic: ");
  Serial.println(topic);
  
  // Criar um buffer para armazenar o payload e adicionar um byte extra para o caractere nulo
  char json[length + 1];
  memcpy(json, payload, length);
  json[length] = '\0'; // Adicionar o caractere nulo no final do JSON para formar uma string válida
  
  Serial.print("Message: ");
  Serial.println(json);

  // Parse do JSON
  StaticJsonDocument<200> doc; // Alocar uma quantidade suficiente de memória para o documento JSON
  DeserializationError error = deserializeJson(doc, json);
  
  // Verificar se houve erro no parse do JSON
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  // Acessar a propriedade desejada do JSON
  const char* valueEstoque = doc["estoque"]; 
  const char* medicamento = doc["medicamento"]; 
  // Verificar se a propriedade existe no JSON
  if (valueEstoque && medicamento) {
    int estoqueDisponivel = atoi(valueEstoque);
    if (estoqueDisponivel < 2) {
     
      Serial.print("O Estoque do medicamento: ");
      Serial.print(medicamento);
      Serial.print(" ");
      Serial.print("esta baixo, quantidade atual: ");
      Serial.print(valueEstoque);
      tocarBuzzer(PIN_BUZZER_1, 300);
    } else {
       Serial.print("O Estoque do medicamento: ");
       Serial.print(medicamento);
       Serial.print(" atualmente eh de: ");
       Serial.print(valueEstoque);
    }
  } else {
    Serial.println("Falha ao Localizar a propriedade estoque no json.");
  }
   Serial.println("");
  Serial.println("-----------------------");
}

void callbackCadastro(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  
  // Criar um buffer para armazenar o payload e adicionar um byte extra para o caractere nulo
  char json[length + 1];
  memcpy(json, payload, length);
  json[length] = '\0'; // Adicionar o caractere nulo no final do JSON para formar uma string válida
  
  Serial.print("Message: ");
  Serial.println(json);

  // Parse do JSON
  StaticJsonDocument<200> doc; // Alocar uma quantidade suficiente de memória para o documento JSON
  DeserializationError error = deserializeJson(doc, json);
  
  // Verificar se houve erro no parse do JSON
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  // Acessar a propriedade desejada do JSON
  const char* idMedicamento = doc["id"]; 
  const char* medicamento = doc["medicamento"]; 
  // Verificar se a propriedade existe no JSON
  if (idMedicamento && medicamento) {
        Serial.print("Cadastrar Tag Para o Medicamento: ");
        Serial.print(medicamento);
        Serial.println();
        Serial.print("Aguarando Tag para leitura...");
        Serial.println();
        bool isTagRead = false;
        while(!isTagRead){
            // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
            if ( ! rfid.PICC_IsNewCardPresent()){
               continue;
            }

            // Verify if the NUID has been readed
            if ( ! rfid.PICC_ReadCardSerial())
              continue;

            MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
            Serial.println(rfid.PICC_GetTypeName(piccType));
            String tagId = GetTagIdHex(rfid.uid.uidByte, rfid.uid.size);
            bool tagIsRead = tagId.isEmpty();
            if(!tagIsRead){
               Serial.println();
               Serial.print("Id da Tag Para Cadastro do Medicamento");
               Serial.print(medicamento);
               Serial.print(" : ");
               Serial.print(tagId); 
               isTagRead = true;

               StaticJsonDocument<200> cadastroResponse;
               cadastroResponse["id"] = idMedicamento;
               cadastroResponse["medicamento"] = medicamento;
               cadastroResponse["idTag"] = tagId;

              char buffer[200];
              serializeJson(cadastroResponse, buffer);
              client.publish(topicCadastroRetorno, buffer);
              delay(2000);
            }
        }
     
    
  } else {
    Serial.println("Falha ao Localizar a propriedade estoque no json. ");
  }
   Serial.println("");
  Serial.println("-----------------------");
}

void tocarBuzzer(int buzzer, int ton) {
  int tempo = 5000;
  tone(buzzer, ton, tempo);
}

String GetTagIdHex(byte *buffer, byte bufferSize) {
  String tagID = "";
 	for (byte i = 0; i < bufferSize; i++) {
 		tagID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    tagID += String(rfid.uid.uidByte[i], HEX);
 	}

  return tagID; 
}

void SendRetiradaMedicamento(){
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));
    String tagId = GetTagIdHex(rfid.uid.uidByte, rfid.uid.size);

    StaticJsonDocument<200> saidaEstoqueRequest;
    saidaEstoqueRequest["idTag"] = tagId;

    char buffer[200];
    serializeJson(saidaEstoqueRequest, buffer);
    client.publish(topicEstoque, buffer);
}

void loop() {
  client.loop();
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  SendRetiradaMedicamento();

  //Serial.print(F("PICC type: "));
  //MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  //Serial.println(rfid.PICC_GetTypeName(piccType));

  //Serial.println(F("The NUID tag is: "));
  // Serial.print(F("In hex: "));
  // String tagId = GetTagIdHex(rfid.uid.uidByte, rfid.uid.size);
  // Serial.println();
  // Serial.print("Id da Tag Lida: ");
  // Serial.print(tagId);
  // Serial.println();
 

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}
