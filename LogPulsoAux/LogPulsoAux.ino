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

#define LED_BUILTIN 2
#define PINBNLOCAL 25
#define PINBNREMOTE 26
#define PINBLOCAL 27
#define PINBREMOTE 14
#define PINNLOCAL 12
#define PINNREMOTE 13
#define PIN921LOCAL 32
#define PIN921REMOTE 33
#define PINRBLOCAL 34
#define PINRBREMOTE 35

#define QUANTI 10
#define ERRMAX 10
#define TEMPO 1000

const char *NSSID = "renato"; //SSD
const char *SENHA = "lablab01"; // Senha

String ip = "172.0.0.139";
String porta = "8082";

bool masterReset = false;

int t = 0;
int erro = 0;
int conterr;

bool pulso[QUANTI] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int pinos[QUANTI] = {PINBNLOCAL, PINBLOCAL, PINNLOCAL, PIN921LOCAL, PINRBLOCAL, PINBNREMOTE, PINBREMOTE, PINNREMOTE, PIN921REMOTE, PINRBREMOTE};
String nomes[QUANTI] = {"PulsoBNFM", "PulsoBFM", "PulsoNFM", "Pulso921", "PulsoRBFM", "PulsoBNFM", "PulsoBFM", "PulsoNFM", "Pulso921", "PulsoRBFM"};

bool atual[QUANTI] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool mudou[QUANTI] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool anterior[QUANTI] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
double tempo[QUANTI] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool statusp[QUANTI] = {1, 1, 1, 1, 1, 0, 0, 0, 0, 0};


hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule(){

    //Serial.println("Resetando");
    //delay(3000);
    ESP.restart(); 
  }

void IRAM_ATTR mReset(){
    masterReset = true;
    Serial.print("Resetando....");
    //delay(5000);
    resetModule();
  }
//***************************************************

//****************************************************


void setup()
{
  
  Serial.begin(9600);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PINBNLOCAL, INPUT_PULLUP); 
  pinMode(PINBNREMOTE, INPUT_PULLUP); 
  pinMode(PINBLOCAL, INPUT_PULLUP); 
  pinMode(PINBREMOTE, INPUT_PULLUP); 
  pinMode(PINNLOCAL, INPUT_PULLUP); 
  pinMode(PINNREMOTE, INPUT_PULLUP); 
  pinMode(PIN921LOCAL, INPUT_PULLUP); 
  pinMode(PIN921REMOTE, INPUT_PULLUP); 
  pinMode(PINRBLOCAL, INPUT_PULLUP); 
  pinMode(PINRBREMOTE, INPUT_PULLUP); 

  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
    
  WiFi.disconnect();

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 5000000, true);
  timerAlarmEnable(timer);
  Serial.println("Setup realizado.");  
  delay(3000);
  digitalWrite(15, HIGH);
}


void compara(){

  for(int i = 0; i<QUANTI; i++){

    bool p = digitalRead(pinos[i]); 
    atual[i] = !p;
  }

  for (int i = 0; i<QUANTI;i++){

    if (anterior[i] != atual[i]){
        
      mudou[i] = true;
      anterior[i] = atual[i];
      }
    }
  }

bool envia(int indice){

  bool foi = false;
  
  Serial.println("comecando a enviar");
       
  if (WiFi.status() != WL_CONNECTED){
    Serial.println(NSSID);
    Serial.println(SENHA);
    WiFi.begin(NSSID, SENHA);
    Serial.println("conectando");
    }
  
  int i = 0;
  
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
    i++;
    if (i > 50)    {
      erro = 1;
      Serial.println("Erro ao conectar");
      conterr ++;
      if (conterr > ERRMAX){
        //ESP.restart();
        }
      break;
    }
  }
  if (erro == 0){
    HTTPClient http;
    Serial.println("enviando GET");
    
    //String link = "http://172.0.0.106:8080/cgi-bin/cgi1.py?ID="+stringID;
    String link = "http://" + ip + ":" + porta + "/cgi-bin/logpulso.py?";

    link = link + "nome=" + nomes[indice] + "&status=" + statusp[indice]; 
    Serial.println(link);
    http.begin(link);
    
    //http.begin("http://172.0.0.106:8080/cgi-bin/cgi1.py?ID="+stringID+"&dados="+stringDados[0]+"&dados2="+stringDados[1]+"&dados3="+stringDados[2]+"&dados4="+stringDados[3]+"&dados5="+stringDados[4]+"&dados6="+stringDados[5]+"&dados7="+stringDados[6]+"&dados8="+stringDados[7]+"&dados9="+stringDados[8]+"&dados10="+stringDados[9]);
    //Serial.print("http://172.0.0.114:8080/cgi-bin/cgi1.py?ID="+stringID+"&dados="+stringDados);
    int httpCode = http.GET();
    if(httpCode > 0) // se erro, httpcode será negativo
    {
      if(httpCode == HTTP_CODE_OK)
      {
        Serial.print("Enviado com sucesso ");
        Serial.println(httpCode);
        
        //writeFile(SPIFFS, "/data.txt", String(litros).c_str());// Salva o slitros na memoria
        
        String payload = http.getString();
        
        Serial.print("Payload ");
        Serial.println(payload);

        if (atoi(payload.c_str()) == 1){
          
          masterReset = true;
          //ESP.restart();            
        }
        conterr = 0;

        foi = true;
      }
      else{
        Serial.println("Falha Http Server");
        conterr ++;
        //ESP.restart();
      }
    } 
    else{
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      Serial.print("Erro ao enviar");
      conterr ++;
      if (conterr > ERRMAX){
        //ESP.restart();
        }
    }
    http.end();
    }
  return foi;
}

void loop(){

  timerWrite(timer, 0);
  compara();

   for (int i = 0; i < QUANTI; i ++){
  
      //Serial.print(pulso[i]);
      //Serial.print(" ");
      //Serial.print(atual[i]);
      //Serial.print(" ");
      //Serial.println(mudou[i]);
  }  
  for (int i = 0; i < QUANTI;i++){

    if (mudou[i] == true){
      if (atual[i] == true){
        if (envia(i)){
          mudou[i] = false;
          //atual[i] = false;
          //pulso[i] = false;
          //anterior[i] = false;
        }
      }
      else{
        mudou[i] = false;
        }
    } 
  }
  delay(100);    
}
