/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED
#define SOLENOIDE 25
#define BOBINA 26
#define PINREED 15



// Set these to your desired credentials.
const char *ssid = "RedeRua";
const char *password = "MeuCarroBloqueado";
const int TEMPO = 30;
const int TEMPOREED = 5;
double cont = TEMPO + 1, cont2 = 0;
bool bloc = true, reed = false;
int conec = 0;

WiFiServer server(80);
void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case SYSTEM_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi access point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
            conec++;
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            conec--;
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            Serial.println("IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
    }}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PINREED, INPUT);
  pinMode(SOLENOIDE, OUTPUT);
  pinMode(BOBINA, OUTPUT);
  digitalWrite(SOLENOIDE, LOW);
  digitalWrite(BOBINA, LOW);


  WiFi.onEvent(WiFiEvent);
  Serial.begin(9600);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  if (conec < 1){
    cont ++;
    }
  else{
    cont = 0;
  }
  if ((cont > TEMPO && bloc == false) && reed == false){
    bloc = true;
    Serial.println("Carro Bloqueado");
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(SOLENOIDE, LOW);
    delay(1000);
    digitalWrite(BOBINA, HIGH);
  }

  if ((cont < TEMPO || reed == true) && bloc == true){

    bloc = false;
    Serial.println("Carro Liberado");
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(SOLENOIDE, HIGH);
    delay(1000);
    digitalWrite(BOBINA, LOW);
    }
  if (digitalRead(PINREED) == true){
    cont2 ++;
    }
  else{
    cont2 = 0;
    }
  if (cont2 >= TEMPOREED){
    reed = true;
    }
  if (cont > TEMPO*10){
    cont = TEMPO+1;
    }
  Serial.print(cont);
  Serial.print(" ");
  Serial.print(reed);
  Serial.print(" ");
  Serial.println(bloc);
  delay(1000);
}
