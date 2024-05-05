#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <stdlib.h>
#include <esp_system.h>
#include "FS.h"
#include "SPIFFS.h"


void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  ESP.restart();
}
