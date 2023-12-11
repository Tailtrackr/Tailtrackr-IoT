#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
TinyGPSPlus  gps;
HardwareSerial SerialPort(1);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, 4, 2);
  delay(3000);
}

void updateSerial(){
  delay(500);
  while (Serial.available()){
    Serial1.write(Serial.read()); //Forward what Serial received to Software Serial Port
  }
  while (Serial1.available()){
    Serial.write(Serial1.read()); //Forward what Software Serial received to Serial Port
  }
}

void displayInfo(){
  Serial.print(F("\nLocation: \n"));
  if (gps.location.isValid()){
    Serial.print(F("Latitude: "));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F("\n"));
    Serial.print(F("Longitude: "));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  //updateSerial();
  while (Serial1.available() > 0)
    if (gps.encode(Serial1.read()))
      displayInfo();
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
}
