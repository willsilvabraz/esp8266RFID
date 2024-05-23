#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Servo.h>  // Biblioteca para controle do servomotor

#define RST_PIN         4           // Configurable, see typical pin layout above
#define SS_PIN          15          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance  

const char* ssid = "Index";          // Substitua com o SSID da sua rede Wi-Fi
const char* password = "12345678";   // Substitua com a senha da sua rede Wi-Fi

const char* serverName = "https://apirfid.onrender.com/cadUid";  // URL da API

const int ledPin = D2; // Pino D2 para o LED


WiFiClientSecure wifiClientSecure;
Servo myServo;  // Instância do servomotor

void setup() {
  pinMode(ledPin, OUTPUT); // Configura o pino do LED como saída

  Serial.begin(115200);

  // Conectando ao Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado ao Wi-Fi");

  SPI.begin();                      // Init SPI bus
  mfrc522.PCD_Init();               // Init MFRC522
  delay(4);                         // Optional delay. Some boards need more time after init to be ready
  mfrc522.PCD_DumpVersionToSerial();// Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID..."));

  // Configurando wifiClientSecure para ignorar a verificação do certificado (opcional e inseguro)
  wifiClientSecure.setInsecure(); // Unsecured connection (for testing only)

  myServo.attach(D1);  // Pino do servomotor (substitua pelo pino que você está usando)
  myServo.write(0);    // Posiciona o servomotor na posição inicial (0 graus)
  digitalWrite(ledPin, HIGH);  // Resposta da API

}

void loop() {
 // moveServo();  // Chama a função para movimentar o servomotor
  readCardUID();
  delay(10000);  // Espera 10 segundos antes de enviar novamente
}

void readCardUID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  digitalWrite(ledPin, LOW);
  Serial.print(F("Card UID: "));
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0";
    }
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println(uidString);

  mfrc522.PICC_HaltA();

  sendJsonToServer("testando", uidString.c_str());


}

void sendJsonToServer(const char* status, const char* uid) {
  HTTPClient http;

  // Use begin() com WiFiClientSecure para URLs HTTPS
  http.begin(wifiClientSecure, serverName);  // URL para enviar o POST
  http.addHeader("Content-Type", "application/json");  // Tipo do conteúdo

  // Criando objeto JSON
  StaticJsonDocument<200> doc;
  doc["status"] = status;
  doc["uid"] = uid;

  String requestBody;
  serializeJson(doc, requestBody);

  // Mostrando o JSON que será enviado
  // Liga o LED

  Serial.println("Enviando JSON ao servidor:");
  Serial.println(requestBody);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  int httpResponseCode = http.POST(requestBody);  // Envia o POST com o JSON
 // delay(1000);
  if (httpResponseCode > 0) {
    String response = http.getString();  // Obtém a resposta
    delay(100);
    Serial.println(httpResponseCode);  // Código de resposta HTTP
    delay(100);
    Serial.println(response);
    delay(100);
    
    delay(100);
    abrir();
    delay(5000);
    fechar();
    delay(100);
  } else{
    if(httpResponseCode == -11){
      delay(100);
    abrir();
    delay(5000);
    fechar();
    delay(100);
    }else{
    Serial.print("Erro no envio do POST: ");
    Serial.println(httpResponseCode);
    }  // Código de resposta HTTP de erro
  }

  http.end();  // Fecha a conexão
    digitalWrite(ledPin, HIGH);  // Resposta da API
}

void abrir() {
  // Movimenta o servomotor de 0 a 180 graus
  for (int pos = 0; pos <= 180; pos++) {
    myServo.write(pos);
    delay(15);  // Ajuste o atraso conforme necessário
  }
}

void fechar() {
  // Movimenta o servomotor de volta de 180 a 0 graus
  for (int pos = 180; pos >= 0; pos--) {
    myServo.write(pos);
    delay(15);  // Ajuste o atraso conforme necessário
  }
}