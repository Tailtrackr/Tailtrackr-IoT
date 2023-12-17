#include <TinyGPSPlus.h>

#define GSM_RXD 16
#define GSM_TXD 17
#define GPS_RXD 4
#define GPS_TXD 2
#define PKEY 13
#define RST 14

#define SerialMon Serial
#define neogps Serial1
#define SerialAT Serial2

#define TINY_GSM_MODEM_SIM7600
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

// GPRS credentials
const char apn[]      = "www.xl4g.net";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char server[]   = "tailtrackr-404701-default-rtdb.asia-southeast1.firebasedatabase.app";
const char resource[] = "/newData";
const int  port       = 443;

int ledState = LOW;
const int ledPin =  12;
unsigned long cur_time, old_time;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient    http(client, server, port);
TinyGPSPlus gps;

void setup() {
  SerialMon.begin(9600);
  neogps.begin(9600, SERIAL_8N1, GPS_RXD, GPS_TXD);
  SerialAT.begin(115200, SERIAL_8N1, GSM_RXD, GSM_TXD);
  delay(250);
  initialize();
}

void loop() {
  // Read NMEA data from GPS module
  float latitude, longitude;

  while (neogps.available() > 0){
    if (gps.encode(neogps.read()))
      displayInfo(latitude, longitude);
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      SerialMon.println(F("No GPS detected: check wiring."));
      while (true);
    }
  }

  // Create JSON payload
  String payload = "{\"latitude\":" + String(latitude, 6) + ",\"longitude\":" + String(longitude, 6) + "}";

  // Send data to Cloud Run server
  sendToCloudRun(payload);

  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}

void updateSerial(){
  delay(500);
  while (SerialMon.available()){
    neogps.write(Serial.read()); //Forward what Serial received to Software Serial Port
  }
  while (neogps.available()){
    SerialMon.write(neogps.read()); //Forward what Software Serial received to Serial Port
  }
}

void displayInfo(float latitude, float longitude){
  Serial.print(F("\nLocation: \n"));
  if (gps.location.isValid()){
    Serial.print(F("Latitude: "));
    Serial.print(gps.location.lat(), 6);
    latitude = gps.location.lat();
    Serial.print(F("\n"));
    Serial.print(F("Longitude: "));
    Serial.print(gps.location.lng(), 6);
    longitude = gps.location.lng();
  }
  else
  {
    Serial.print(F("INVALID"));
  }
}

void initialize(){
  SerialMon.println("Starting module...");
  pinMode(ledPin, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(PKEY, OUTPUT);
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
  digitalWrite(PKEY, HIGH);
  digitalWrite(RST, HIGH);
  delay(1000);
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
  SerialMon.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  delay(5000);
  send_at("AT+CPIN?");
  send_at("AT+CSQ");
  send_at("AT+CREG?");
  send_at("AT+CGREG?");
  send_at("AT+CPSI?");
  send_at("AT+CGDCONT=1,\"IP\",\"www.xl4g.net\"");
  send_at("AT+CGACT=1,1");
  send_at("AT+CGACT?");
}

void send_at(char *_command) {
  SerialAT.println(_command);
  wRespon(1000);
}

void wRespon(long waktu) {
  cur_time = millis();
  old_time = cur_time;
  while (cur_time - old_time < waktu) {
    cur_time = millis();
    while (SerialAT.available() > 0) {
      SerialMon.print(SerialAT.readString());
    }
  }
}

void sendToCloudRun(const String &data) {
  // Convert String to char array
  char dataCharArray[data.length() + 1];
  data.toCharArray(dataCharArray, sizeof(dataCharArray));
  String httpDataCommand = "AT+HTTPDATA=" + String(data.length()) + ",5000";
  
  send_at("AT+HTTPINIT");
  send_at("AT+HTTPPARA=\"URL\",\"https://app-backend-wdsuayrd4q-et.a.run.app/newData\"");
  send_at("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  send_at(const_cast<char*>(httpDataCommand.c_str()));  // Pass the char array instead of String
  send_at(dataCharArray);
  send_at("AT+HTTPACTION=1");
  send_at("AT+HTTPTERM");
}