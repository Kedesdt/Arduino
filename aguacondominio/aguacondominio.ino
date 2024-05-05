#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <stdlib.h>
#include <esp_system.h>
#include "FS.h"
#include "SPIFFS.h"

WiFiMulti wifiMulti;

#define USE_SERIAL Serial
#define PINSENSOR 15 //pino do sensor
#define PINLED 32 // led do sensor de fluxo, led da placa
#define LEDVERMELHO 33 // led Vermelho, indica erro de conexão
#define LEDVERDE 25 // led verde, Tudo OK placa ligada
#define LEDAZUL 26 // led Azul, indica que a placa está enviando dados para o servidor
#define LEDAMARELO 27 // Led Amarelo, Agua cortada
#define CORTE 18 // Marrom fecha
#define CORTE2 19 // Azul abre
#define FORMAT_SPIFFS_IF_FAILED true

const int PEUL = 288; //Pulsos em um litro
//const char *NSSID = "UPNET"; //SSD
//const char *SENHA = "22371609"; // Senha
//const char *NSSID = "KJAA"; //SSD
//const char *SENHA = "kjaa2017"; // Senha
const char *NSSID = "ATBOSQUE"; //SSD
const char *SENHA = "individual"; // Senha

const int AP = 22 ; //informa para qual apartamento está sendo a contagem
const int BLOCO = 05; // informa para qual bloco está sendo a contagem

hw_timer_t *timer = NULL;

bool erro = 0;
int i = 0;
int conterr = 0;
bool aberto = true;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // para seção crítica

WiFiMulti WiFiMulti;

unsigned long chave;
volatile unsigned long pulsos = 0, pulso;
unsigned int litros, t = 0, pa;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

int readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return -1;
    }
    String x = "";
    char z;
    int y;
    Serial.println("- read from file:");
    while(file.available()){
      z = char(file.read());
        x = x + z;
    }
    Serial.println(x);
    y = atoi(x.c_str());
    return y;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}


void IRAM_ATTR resetModule()
{
  digitalWrite(LEDVERMELHO, HIGH);
  delay(3000);
  //digitalWrite(CORTE, LOW);
  delay(3000);
  ESP.restart();
  }

void cortaAgua()
{
  Serial.println("Agua cortada");
  delay(500);
  digitalWrite(CORTE, LOW);
  digitalWrite(CORTE2, HIGH);
  delay(500);
}

void abreAgua()
{
  Serial.println("Agua liberada");
  delay(500);
  digitalWrite(CORTE, HIGH);
  digitalWrite(CORTE2, LOW);
  delay(500);
}

void IRAM_ATTR incrementa() //função chamada pela interrupção **IRAM_ATTR Roda na RAM 
{  
  portENTER_CRITICAL_ISR(&mux); // inicio da seção crítica
  pulsos++; //incrementa um pulso
  portEXIT_CRITICAL_ISR(&mux); // fim da seção crítica
}

void setup()
{
  Serial.begin(115200);
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        resetModule();
        return;
    }
  if (readFile(SPIFFS, "/data.txt") == -1){
    Serial.println("Não existe arquivo");
    Serial.println("Criando....");
    writeFile(SPIFFS, "/data.txt", "0");
    }
  litros = readFile(SPIFFS, "/data.txt");
  pinMode(PINLED,OUTPUT);
  pinMode(LEDVERMELHO,OUTPUT);
  pinMode(LEDVERDE,OUTPUT);
  pinMode(LEDAZUL,OUTPUT);
  pinMode(LEDAMARELO,OUTPUT);
  pinMode(CORTE,OUTPUT);
  pinMode(CORTE2,OUTPUT);
  digitalWrite(CORTE, HIGH);
  digitalWrite(CORTE2, HIGH);
  attachInterrupt(digitalPinToInterrupt(PINSENSOR), incrementa, RISING); // configura a interrupção pelo sensor de fluxo

  digitalWrite(PINLED, HIGH);
  delay(250);
  digitalWrite(PINLED, LOW);
  digitalWrite(LEDVERMELHO, HIGH);
  delay(250);
  digitalWrite(LEDVERMELHO, LOW);
  digitalWrite(LEDVERDE, HIGH);
  delay(250);
  digitalWrite(LEDVERDE, LOW);
  digitalWrite(LEDAZUL, HIGH);
  delay(250);
  digitalWrite(LEDAZUL, LOW);
  digitalWrite(LEDAMARELO, HIGH);
  delay(250);
  digitalWrite(LEDAMARELO, LOW);
  delay(1000);
  digitalWrite(LEDVERDE, HIGH);
  Serial.println("Setup feito");

  timer = timerBegin(0, 80, true);

  timerAttachInterrupt(timer, &resetModule, true);

  timerAlarmWrite(timer, 60000000, true);
  timerAlarmEnable(timer);
  
}
void loop()
{
  timerWrite(timer, 0);
  pa = pulso;
  if (pulsos >= PEUL)
  {
    litros ++;
    writeFile(SPIFFS, "/data.txt", String(litros).c_str());
    portENTER_CRITICAL_ISR(&mux); // inicio da seção crítica
    pulsos = pulsos - PEUL;
    portEXIT_CRITICAL_ISR(&mux); // fim da seção crítica
    digitalWrite(PINLED, !digitalRead(PINLED));
  }
  //if (litros > 0)
  if(1==1)
  {
    if (t >= 30)
    {
      t = 0;
      i = 0;
      erro = 0;
      Serial.println("comecando a enviar");
      digitalWrite(LEDAZUL, HIGH);
      digitalWrite(LEDVERMELHO, LOW);
      
      WiFi.begin(NSSID, SENHA);
      Serial.println("conectando");
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(100);
        i++;
        if (i > 50)
        {
          digitalWrite(LEDVERMELHO, HIGH);// acende led vermelho de erro de conexao
          digitalWrite(LEDAZUL, LOW); //apaga o led azul pois não houve conexão
          erro = 1;
          Serial.println("Erro ao conectar");
          conterr ++;
          if (conterr > 10){
            resetModule();
            }
          break;
        }
      }
      if (erro == 0)
      {
        HTTPClient http;
        pulso = litros*PEUL;
        chave = (AP + BLOCO + 159314 + pulso);
        String stringPulso =  String(pulso);
        String stringLitros =  String(litros);
        String stringChave = String(chave);
        String stringAP = String(AP);
        String stringBloco = String(BLOCO);
        Serial.println("enviando GET");
        http.begin("http://www.taboaobosque.com.br/Condominio/leitor.php?AP="+stringAP+"&Bloco="+stringBloco+"&Litros="+stringLitros+"&Giros="+stringPulso+"&Chave="+stringChave);
        int httpCode = http.GET();
        if(httpCode > 0) // se erro, httpcode será negativo
        {
          if(httpCode == HTTP_CODE_OK)
          {
            Serial.print("Enviado com sucesso ");
            Serial.println(httpCode);
            Serial.print(t);
            Serial.print(" ");
            Serial.print(litros);
            Serial.print(" ");
            Serial.println(pulsos);
            litros = 0; // Se tudo ocorreu bem Zera os litros
            writeFile(SPIFFS, "/data.txt", String(litros).c_str());// Salva o slitros na memoria
            digitalWrite(LEDVERDE, HIGH);
            String payload = http.getString();
            Serial.print(stringAP);
            Serial.print(" ");
            Serial.print(stringBloco);
            Serial.print(" ");
            Serial.println(payload);
            int Status;
            if (payload == "0")
            {
              abreAgua();
              Status = 0;
              aberto = true;
            }
            else if (payload == "1")
            {
              cortaAgua();
              Status = 1;
              aberto = false;
            }
            digitalWrite(LEDAMARELO, Status);
          }
        conterr = 0;
        } 
        else
        {
          //Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          Serial.print("Erro ao enviar");
          digitalWrite(LEDAMARELO, HIGH);
          digitalWrite(LEDVERDE, LOW);
          conterr ++;
          if (conterr > 10){
            resetModule();
            }
        }
        http.end();
        
        digitalWrite(LEDAZUL, LOW);
        }
      WiFi.disconnect();
      }
    }
    t++;
    //if (aberto == true){
    //  abreAgua();
    //  }
    //else{
    //  cortaAgua();
    //  }
    delay(1000);
    //Serial.println(t);
  }
