#define PKEY 13
#define RST 14
#define SerialMon Serial
#define SerialAT Serial2
int ledState = LOW;
const int ledPin =  12;
int counter = 0;
unsigned long cur_time_led, old_time_led;
unsigned long cur_time, old_time;
bool hold = 0;
#define TINY_GSM_MODEM_SIM7600
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif
#define TINY_GSM_DEBUG SerialMon
// set GSM PIN, if any
#define GSM_PIN ""
// Your GPRS credentials, if any
const char apn[]      = "Internet";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char server[]   = "https://tailtrackr-404701-default-rtdb.asia-southeast1.firebasedatabase.app/";  // Replace with your Cloud Run server URL
const char resource[] = "/newData";              // Replace with your Cloud Run endpoint
const int  port       = 443;  // Use port 443 for HTTPS
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

void setup() {
  SerialMon.begin(115200);
  SerialAT.begin(115200);
  delay(250);
  SerialMon.println("test at mulai");
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
  cur_time_led = millis();
  if (cur_time_led - old_time_led >= 1000) {
    counter++;
    switch (counter) {
      case 1:
        send_at("AT+CPIN?");
        break;
      case 2:
        send_at("AT+CSQ");
        break;
      case 3:
        send_at("AT+CPSI?");
        break;
      case 4:
        send_at("AT+IPR?");
        counter = 0;
        break;
    }

    // Generate random latitude and longitude
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
    old_time_led = cur_time_led;
  }
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

void reset_sim() {
  digitalWrite(RST, HIGH);
}

void wakeup_sim() {
  SerialMon.println("wakeup sim7600");
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
  digitalWrite(PKEY, HIGH);
  digitalWrite(RST, HIGH);
  delay(1000);
  digitalWrite(PKEY, LOW);
  digitalWrite(RST, LOW);
  delay(1000);
  wRespon(15000);
}

float generateRandomFloat(float min, float max) {
  return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void sendToCloudRun(String data) {
  SerialMon.println("Sending data to Cloud Run: " + data);
  http.beginRequest();
  http.post(resource, "application/json", data);
  int httpResponseCode = http.responseStatusCode();
  SerialMon.print("HTTP Response code: ");
  SerialMon.println(httpResponseCode);
  http.endRequest();
  delay(1000);  // Adjust as needed
}
