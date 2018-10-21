#include <SoftwareSerial.h> //Biblioteca SoftwareSerial para usar a comunicação serial em outros pinos digitais

const String STR_COMPANY_NAME = "*** ACOA - Aplicação para acompanhamento de consumo de água ***";
const String STR_WIFI_REBOOTING = "*** Reiniciando o componente WIFI... ***";
const String STR_WIFI_REBOOTED = "*** Componente reiniciado! ***";
const String STR_INIT_MODULE = "*** Inicialização do módulo inteligente ***";
const String STR_CONNECTING_WIFI = "*** Conectando à rede wireless local ***";
const String STR_CONNECT_WIFI_SUCCESS = "*** Módulo conectado! ***";
const String STR_CONNECT_WIFI_ERROR = "*** Não foi possível conectar à rede Wifi... \nTentando novamente... ***";
const String STR_STARTING_COUNTING = "*** Iniciando a contagem ***";
const String STR_INIT_TCP_CONNECTION = "*** Conexão TCP iniciada ***";
const String STR_SENDING_DATA = "*** Enviando dados ao servidor ***";
const String STR_SENT_DATA = "*** Pacote de dados enviado com sucesso! ***";
const String STR_AVERAGE_PER_HOUR = "Média de consumo por hora: ";
const String STR_SECOND = "Segundo ";
const String STR_LITERS = " Litros - ";
const String STR_LITERS_PER_HOUR = " L/hora";
const String STR_SINGLE_LINE_BREAK = "\n";
const String STR_ESP_STATUS = "Status: ";
const String STR_SERVER = "http://localhost";
const String STR_URI = "/api/liters";
const String STR_SSID = "ssid";
const String STR_PASSWORD = "password";
const int ACCOUNT_ID = 1;
const String MODULE_ID = "";
const byte rxPin = 2; // Ligado diretamente ao pino tx do exp8266
const byte txPin = 3; // Ligado ao pino rx do esp8266(usar resistor para diminuir a voltagem)
float flowRate;
float average = 0;
int pulseCounter;
int i=0;

// Biblioteca do arduino que faz com que outros pinos digitais se comuniquem via serial
SoftwareSerial esp8266(rxPin, txPin);

void setup(){
  Serial.begin(9600);
  while (!Serial) {
    ; // Espera a conexão com a porta serial. Necessário somente para porta USB nativa
  }
  esp8266.begin(115200);
  while(!esp8266) {
    ; //Espera a conexão com o módulo esp8266
  }

  Serial.println(STR_COMPANY_NAME);
  Serial.println(STR_SINGLE_LINE_BREAK + STR_INIT_MODULE);
  
  resetWifiModule();
  connectWifiModule();

  //Configura o pino 5(Interrupção 0) para trabalhar como interrupção
  pinMode(7, INPUT);
  attachInterrupt(0, incrementPulse, RISING);
  Serial.println(STR_STARTING_COUNTING + STR_SINGLE_LINE_BREAK);
} 

void loop (){  
  pulseCounter = 0;   //Zera a variável para contar os giros por segundos
  
  sei();              //Habilita interrupção
  delay(1000);        //Aguarda 1 segundo
  cli();              //Desabilita interrupção

  flowRate = pulseCounter / 5.5;  //Converte para litros
  average = average + flowRate;   //Soma a vazão para o calculo da media
  i++;
  Serial.print(flowRate + STR_LITERS + STR_SECOND + i + STR_SINGLE_LINE_BREAK);
  
  if(i == 3600){
    average = average / 3600;
    Serial.print(STR_SINGLE_LINE_BREAK + STR_AVERAGE_PER_HOUR + average + STR_LITERS_PER_HOUR);
    
    //JSON exemplo - "{flowRate:" + average + ",dateCollection: \"2012-04-23T18:25:43.511Z\",account: {account_id: 1}}";
    String data;
    data += F("{\"flowRate\":");
    data += String(average, 2);
    data += F(", \"account\": {\"account_id\": ");
    data += String(ACCOUNT_ID);
    data += F("}}");
    sendData(data);
    average = 0;
    i=0;
    
    Serial.println(STR_SINGLE_LINE_BREAK + STR_STARTING_COUNTING + STR_SINGLE_LINE_BREAK);
  }
}

void incrementPulse(){
  pulseCounter++;
}

void resetWifiModule() {
  Serial.println(STR_WIFI_REBOOTING);
  esp8266.println("AT+RST");
  delay(1000);  
  if(esp8266.find("OK")){
    Serial.println(STR_WIFI_REBOOTED);
  } else {
    resetWifiModule();
  }
}

void connectWifiModule() {
  Serial.println(STR_CONNECTING_WIFI);
  
  String command = "AT+CWJAP=\"" + STR_SSID + "\",\"" + STR_PASSWORD + "\"";
  esp8266.println(command);
  delay(4000);  
  if(esp8266.find("OK")) {
    Serial.println(STR_CONNECT_WIFI_SUCCESS);
  } else {
    Serial.println(STR_CONNECT_WIFI_ERROR);
    connectWifiModule();
  }
}

void sendData(String data) {
  esp8266.println("AT+CIPSTART=\"TCP\",\"" + STR_SERVER + "\",8080"); //Inicia uma conexão TCP
  
  if(esp8266.find("OK")) {
    Serial.println(STR_INIT_TCP_CONNECTION);
    delay(1000);
    
    String postRequest = "POST " + STR_URI + " HTTP/1.0\r\n" + "Host: " + STR_SERVER + "\r\n" +
      "Accept: *" + "/" + "*\r\n" + "Content-Length: " + data.length() + "\r\n" +
      "Content-Type: application/json\r\n" + "\r\n" + data;
      String sendCmd = "AT+CIPSEND="; //Determina o número de caracteres a serem enviados
      esp8266.print(sendCmd);
      esp8266.println(postRequest.length());
      delay(500);
      
      if(esp8266.find(">")) {
        Serial.println(STR_SENDING_DATA);
        esp8266.print(postRequest);
        
        if(esp8266.find("SEND OK")) { 
          Serial.println(STR_SENT_DATA);
          
          while(esp8266.available()) {
            String tmpResp = esp8266.readString();
            Serial.println(tmpResp);
          }
          esp8266.println("AT+CIPCLOSE");   // Finaliza a conexão
        }
      }
  } else {
    sendData(data);
  }
}
