#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
---
#define SS_PIN 5
#define RST_PIN 0
 
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; 
byte nuidPICC[4];

const char* ssid = "";
const char* password = "";
const char* supabaseURL = "";
const char* supabaseKey = "";

void setup() {
  Serial.begin(9600);
  SPI.begin();
  pinMode(15, OUTPUT);
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Este código lê o NUID do MIFARE Classic."));
  Serial.print(F("Usando a seguinte chave:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");

  }
  Serial.println("Conectado ao WiFi");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("Tipo PICC: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Seu cartão não é do tipo MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3]) {
    Serial.println(F("Um novo cartão foi detectado."));
    turnSpeakerOn(500);

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("O NUID do cartão é:"));
    Serial.print(F("Em hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("Em dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    // Enviar dados ao Supabase
    sendToSupabase();
  } else {
    Serial.println(F("Cartão lido anteriormente."));

    turnSpeakerOn(400);
    turnSpeakerOn(400);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void sendToSupabase() {
  HTTPClient http;

  String jsonData = "{\"rfid_uid\": \"" + getUIDAsString(rfid.uid.uidByte, rfid.uid.size) + "\"}";

  http.begin(supabaseURL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(jsonData.length()));
  http.addHeader("apikey", supabaseKey);

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("Resposta do Supabase: ");
    Serial.println(http.getString());
    turnSpeakerOn(100);
    turnSpeakerOn(100);
    turnSpeakerOn(100);
  } else {
    Serial.print("Erro ao enviar dados ao Supabase. Código de resposta: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

String getUIDAsString(byte* buffer, byte bufferSize) {
  String uidString = "";
  for (byte i = 0; i < bufferSize; i++) {
    uidString += String(buffer[i], HEX);
  }
  return uidString;
}

void printHex(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void turnSpeakerOn(int numde){
  digitalWrite(15, HIGH);
  delay(numde);
  digitalWrite(15, LOW);
  delay(numde);
}
