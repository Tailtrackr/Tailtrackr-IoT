#include <ArduinoHttpClient.h>
#include <StreamDebugger.h>
#include <TinyGsmClient.h>
#include <TinyGPS.h>

#define PKEY 13
#define RST 14
#define SerialMon Serial
#define SerialAT Serial2
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_DEBUG SerialMon
// set GSM PIN, if any
#define GSM_PIN ""
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif
#ifdef DUMP_AT_COMMANDS
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

int ledState = LOW;
const int ledPin =  12;
unsigned long cur_time, old_time;

// GPRS credentials
const char apn[]      = "Internet";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char server[]   = "https://tailtrackr-404701-default-rtdb.asia-southeast1.firebasedatabase.app";
const char resource[] = "/newData";
const int  port       = 443;  // Use port 443 for HTTPS

TinyGsmClient client(modem);
HttpClient    http(client, server, port);
TinyGPS gps;

void setup() {
  SerialMon.begin(115200);
  SerialAT.begin(115200);
  gps.begin(SerialAT);
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
  if (isPinReady()){
    // Read NMEA data from GPS module
    while (SerialAT.available()) {
      gps.encode(SerialAT.read());
    }

    if (gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();

      // Create JSON payload
      String payload = "{\"latitude\":" + String(latitude, 6) + ",\"longitude\":" + String(longitude, 6) + "}";
      sendToCloudRun(payload);
    }
  } else {
    Serial.println("SIM card pin is not ready. Cannot proceed.");
  }

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

bool isPinReady() {
  send_at("AT+CPIN?");
  delay(1000);
  while (SerialAT.available()) {
    String response = SerialAT.readStringUntil('\n');
    if (response.indexOf("READY") != -1) {
      return true;
    }
  }
  return false;
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

void sendToCloudRun(String data) {
  SerialMon.println("Sending data to Cloud Run: " + data);
  http.beginRequest();
  http.post(resource, "application/json", data);
  int httpResponseCode = http.responseStatusCode();
  SerialMon.print("HTTP Response code: ");
  SerialMon.println(httpResponseCode);
  http.endRequest();
  delay(1000);
}