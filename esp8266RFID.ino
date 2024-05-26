#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Servo.h>  

#define RST_PIN         4           
#define SS_PIN          15          

MFRC522 mfrc522(SS_PIN, RST_PIN);   

const char* ssid = "localhost";          
const char* password = "127.0.0.1";   

const char* serverName = "https://apirfid.onrender.com/cadUid";  

const int ledPin = D2; 

WiFiClientSecure wifiClientSecure;
Servo myServo;  

void setup() {
  pinMode(ledPin, OUTPUT); 

  Serial.begin(115200);

  connectWiFi();

  SPI.begin();                      
  mfrc522.PCD_Init();               
  delay(4);                         
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println(F("Scan PICC to see UID..."));

  wifiClientSecure.setInsecure(); 

  myServo.attach(D1);  
  myServo.write(0);    
  digitalWrite(ledPin, HIGH);  

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
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

      if (WiFi.status() == WL_CONNECTED) {
        sendJsonToServer("testando", uidString.c_str());
      }
    }
  }
  delay(1000);  // Reduzido o tempo de espera na função loop
}

void sendJsonToServer(const char* status, const char* uid) {
  HTTPClient http;

  http.setTimeout(5000); // Reduz o tempo limite da conexão HTTP para 5 segundos

  if (!http.begin(wifiClientSecure, serverName)) {
    Serial.println("Falha ao conectar ao servidor.");
    return;
  }

  http.addHeader("Content-Type", "application/json");  

  StaticJsonDocument<200> doc;
  doc["status"] = status;
  doc["uid"] = uid;

  String requestBody;
  serializeJson(doc, requestBody);

  Serial.println("Enviando JSON ao servidor:");
  Serial.println(requestBody);
  digitalWrite(ledPin, HIGH);
  int httpResponseCode = http.POST(requestBody);  

  if (httpResponseCode > 0) {
    String response = http.getString();  
    handleJsonResponse(response);        
  } else {
    Serial.print("Falha na requisição HTTP. Código de resposta: ");
    Serial.println(httpResponseCode);
  }

  http.end();  
}

void handleJsonResponse(String jsonResponse) {
  const size_t capacity = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonDocument doc(capacity);

  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (!error) {
    if (doc.containsKey("status")) {
      const char* status = doc["status"];
      Serial.print("Status recebido: ");
      Serial.println(status);
      
      if (strcmp(status, "liberado") == 0) {
        abrir(); // Abrir o servo se o status for "liberado"
        delay(5000); // Aguardar 5 segundos
        fechar(); // Fechar o servo após 5 segundos
      }
    } else {
      Serial.println("Chave 'status' não encontrada no JSON.");
    }
  } else {
    Serial.print("Falha ao fazer parsing do JSON: ");
    Serial.println(error.c_str());
  }
}

void connectWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado ao Wi-Fi");
}

void abrir() {
  for (int pos = 0; pos <= 180; pos++) {
    myServo.write(pos);
    delay(15);  
  }
}

void fechar() {
  for (int pos = 180; pos >= 0; pos--) {
    myServo.write(pos);
    delay(15);  
  }
}
