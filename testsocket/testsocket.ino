#include <WiFi.h>
#include <ArduinoWebsockets.h>

const char* ssid = "renato"; // Nome da rede
const char* password = "lablab01"; // Senha da rede
const char* websockets_server_host = "172.0.0.106"; // IP do servidor websocket
const int websockets_server_port = 5000; // Porta de conexão do servidor
int led = 2;
using namespace websockets;

WebsocketsClient client;

void connectWifi(){

  Serial.print("Conectando a " + String(ssid));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);  
  }
  Serial.println();
  Serial.println("Conectado");  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(led, OUTPUT);
  
  connectWifi();

  bool connec = client.connect(websockets_server_host, websockets_server_port, "/");

  if(connec)
  {
    Serial.println("Conectado ao servidor");
    client.send("Hello ESP");  
  }
  else
  {
    Serial.println("Não Conectado"); 
    return; 
  }
  client.onMessage([&](WebsocketsMessage message)
    {        
        // Exibimos a mensagem recebida na serial
        Serial.print("Got Message: ");
        Serial.println(message.data());
 
        // Ligamos/Desligamos o led de acordo com o comando
        if(message.data().equalsIgnoreCase("ON"))
            digitalWrite(led, HIGH);
        else
        if(message.data().equalsIgnoreCase("OFF"))
            digitalWrite(led, LOW);
    });
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(WiFi.status() != WL_CONNECTED)
  {
    connectWifi();
  }
  Serial.print(".");
  delay(1000);
}
