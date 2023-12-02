#include <TinyGPS++.h>

#define GSM_RXD 16
#define GSM_TXD 17
#define PKEY 13
#define RST 14
#define SerialMon Serial
#define SerialGPS Serial1
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
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

TinyGsmClient client(modem);
HttpClient    http(client, server, port);
TinyGPSPlus gps;

void setup() {
  SerialMon.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, GSM_RXD, GSM_TXD);
  SerialGPS.begin(9600);
  // gps.begin(SerialGPS);
  delay(250);
  SerialMon.println("starting...");
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
  delay(1000);
}

void loop() {
  // if (isPinReady()){
  //   // Read NMEA data from GPS module
  //   while (SerialGPS.available()) {
  //     gps.encode(SerialGPS.read());
  //   }

  //   if (gps.location.isValid()) {
  //     float latitude = gps.location.lat();
  //     float longitude = gps.location.lng();

  //     // Create JSON payload
  //     String payload = "{\"latitude\":" + String(latitude, 6) + ",\"longitude\":" + String(longitude, 6) + "}";
  //     sendToCloudRun(payload);
  //   }
  // } else {
  //   Serial.println("SIM card pin is not ready. Cannot proceed.");
  // }
  // SerialMon.println("sebelum dicek pinnya");
  float latitude = generateRandomFloat(-90.0, 90.0);
  float longitude = generateRandomFloat(-180.0, 180.0);

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

float generateRandomFloat(float min, float max) {
  return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
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