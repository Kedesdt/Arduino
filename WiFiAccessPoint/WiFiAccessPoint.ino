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
#include <vector>
#include <sstream>

#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

// Set these to your desired credentials.
const char *ssid = "RXfm";
const char *password = "rxband";

WiFiServer server(80);

void split(String s, char c, String *lista){
    String temp = "";
    int num = 0;
    for (int i = 0; i <= s.length(); i++){
      if (s[i] == c || i == s.length()){
        lista[num] = temp;
        temp = "";
        num++;
      }else{
        temp = temp + s[i];  
      } 
    }
  }

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
  
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAP(ssid, password);
  delay(1000);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  delay(1000);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          if (currentLine.startsWith("GET")) {
            String lista[10];
            split(currentLine, '?', lista);
            String dados = lista[1];
            split(dados, '&', lista);
            String strFreq = lista[0];
            String strID = lista[1];
            String strLink = lista[2];
            String strSsid = lista[3];
            String strSenha = lista[4];
            split(strFreq, '=', lista);
            strFreq = lista[1];
            split(strID, '=', lista);
            strID = lista[1];
            split(strLink, '=', lista);
            strLink = lista[1];
            split(strSsid, '=', lista);
            strSsid = lista[1];
            split(strSenha, '=', lista);
            strSenha = lista[1];

            Serial.println();
            Serial.println(strFreq);
            Serial.println(strID);
            Serial.println(strLink);
            Serial.println(strSsid);
            Serial.println(strSenha);
            Serial.println();
            
            
          }
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.println("<form method=\"get\" action=\"envio\">");
            client.println("<input type=\"text\" name=\"freq\" placeholder=\"Frequencia\"><br>");
            client.println("<input type=\"text\" name=\"ID\" placeholder=\"ID\"><br>");
            client.println("<input type=\"text\" name=\"link\" placeholder=\"Link\"><br>");
            client.println("<input type=\"text\" name=\"ssid\" placeholder=\"SSID\"><br>");
            client.println("<input type=\"text\" name=\"senha\" placeholder=\"Senha\"><br>");
            client.println("<input type=\"submit\" name=\"enviar\" value=\"Enviar\"><br>");    
            client.println("</form>");
            //client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
            //client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
        }
        //if (currentLine.startsWith("GET")) {
        //  String lista[10];
        //  split(currentLine, '?', lista);
        //  Serial.println(lista[0]);
        //  Serial.println(lista[1]);
        //} 
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
