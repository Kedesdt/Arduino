#define FREQ_STEPS 10

#define RADIO_REG_CHIPID  0x00

#define RADIO_REG_CTRL    0x02
#define RADIO_REG_CTRL_OUTPUT 0x8000
#define RADIO_REG_CTRL_UNMUTE 0x4000
#define RADIO_REG_CTRL_MONO   0x2000
#define RADIO_REG_CTRL_BASS   0x1000
#define RADIO_REG_CTRL_SEEKUP 0x0200
#define RADIO_REG_CTRL_SEEK   0x0100
#define RADIO_REG_CTRL_RDS    0x0008
#define RADIO_REG_CTRL_NEW    0x0004
#define RADIO_REG_CTRL_RESET  0x0002
#define RADIO_REG_CTRL_ENABLE 0x0001

#define RADIO_REG_CHAN    0x03
#define RADIO_REG_CHAN_SPACE     0x0003
#define RADIO_REG_CHAN_SPACE_100 0x0000
#define RADIO_REG_CHAN_BAND      0x000C
#define RADIO_REG_CHAN_BAND_FM      0x0000
#define RADIO_REG_CHAN_BAND_FMWORLD 0x0008
#define RADIO_REG_CHAN_TUNE   0x0010
//      RADIO_REG_CHAN_TEST   0x0020
#define RADIO_REG_CHAN_NR     0x7FC0

#define RADIO_REG_R4    0x04
#define RADIO_REG_R4_EM50   0x0800
//      RADIO_REG_R4_RES   0x0400
#define RADIO_REG_R4_SOFTMUTE   0x0200
#define RADIO_REG_R4_AFC   0x0100


#define RADIO_REG_VOL     0x05
#define RADIO_REG_VOL_VOL   0x000F


#define RADIO_REG_RA      0x0A
#define RADIO_REG_RA_RDS       0x8000
#define RADIO_REG_RA_RDSBLOCK  0x0800
#define RADIO_REG_RA_STEREO    0x0400
#define RADIO_REG_RA_NR        0x03FF

#define RADIO_REG_RB          0x0B
#define RADIO_REG_RB_FMTRUE   0x0100
#define RADIO_REG_RB_FMREADY  0x0080


#define RADIO_REG_RDS0  0x0C
#define RADIO_REG_RDSA  0x0C
#define RADIO_REG_RDSB  0x0D
#define RADIO_REG_RDSC  0x0E
#define RADIO_REG_RDSD  0x0F

// I2C-Address RDA Chip for sequential  Access
#define I2C_SEQ  0x10

#define THRESHAUD 50
#define QUANTI 1
#define LED_BUILTIN 2
#define MASTER_RESET 27
#define TERRA 14
#define FORMAT_SPIFFS_IF_FAILED true


// I2C-Address RDA Chip for Index  Access
#define I2C_INDX  0x11

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


//////////////////////////////////

uint16_t registers[16];

///////////////////////////////////

// Select the frequency we want to tune to by way
// of selecting the channel for the desired frequency
uint16_t channel = 39;
   // assuming band starts at 87.0MHz (per settings below)
   // and channel spacing of 100kHz (0.1MHz) (per settings below)
   // then channel can be derived as follows:
   //
   // channel = (<desired freq in MHz> - 87.0) / 0.1 
   //
   // which is the same as:
   //
   // <10 x desired freq in MHz> - 870
   //
   // some examples:
   //
   // channel 37 dec  = 90.7 MHz
   // channel 39 dec  = 90.9 MHz
   // channel 41 dec  = 91.1 MHz
   // channel 51 dec  = 92.1 MHz
   // channel 83 dec  = 95.3 MHz
   // channel 91 dec  = 96.1 MHz
   // channel 177 dec = 104.7 MHz
   // channel 193 dec = 106.3 MHz
   // channel 99 dec  = 96.9 MHz


// address of the RDA5807 on two wire bus
#define RDA5807M_ADDRESS  0b0010000 // 0x10

#define BOOT_CONFIG_LEN 12
#define TUNE_CONFIG_LEN 4


// Socket

const uint16_t port = 8084;
const char * host = "engrd.bandradios.com.br";
//const uint16_t port = 5679;
//const char * host = "kedes.com.br";
//const uint16_t port = 5789;
//const char * host = "kedes.com.br";


// Socket

const char *NSSID = "renato"; //SSD
const char *SENHA = "lablab01"; // Senha
//const char *NSSID = "ATBOSQUE"; //SSD
//const char *SENHA = "individual"; // Senha

hw_timer_t *timer = NULL;

// These bytes set our initial configuration
// We don't bother to tune to a channel at this stage.
// But instead initiate a reset.
uint8_t boot_config[] = {
  /* register 0x02 */
  0b11000000,
    // DHIZ audio output high-z disable
    // 1 = normal operation
    
    // DMUTE mute disable 
    // 1 = normal operation
    
    // MONO mono select
    // 0 = stereo
    
    // BASS bass boost
    // 0 = disabled
    
    // RCLK NON-CALIBRATE MODE 
    // 0 = RCLK is always supplied
    
    // RCLK DIRECT INPUT MODE 
    // 0 = ??? not certain what this does
    
    // SEEKUP
    // 0 = seek in down direction
    
    // SEEK
    // 0 = disable / stop seek (i.e. don't seek)
    
  0b00001011,
    // SKMODE seek mode: 
    // 0 = wrap at upper or lower band limit and contiue seeking
    
    // CLK_MODE clock mode
    //  000 = 32.768kHZ clock rate (match the watch cystal on the module) 
    
    // RDS_EN radio data system enable
    // 1 = enable radio data system
    
    // NEW_METHOD use new demodulate method for improved sensitivity
    // 0 = presumably disabled 
    
    // SOFT_RESET
    // 1 = perform a reset
    
    // ENABLE power up enable: 
    // 1 = enabled 

  /* register 0x03 */
  // Don't bother to tune to a channel at this stage
  0b00000000, 
    // CHAN channel select 8 most significant bits of 10 in total
    // 0000 0000 = don't boher to program a channel at this time

  0b00001000,
    // CHAN two least significant bits of 10 in total 
    // 00 = don't bother to program a channel at this time
    
    // DIRECT MODE used only when test
    // 0 = presumably disabled
    
    // TUNE commence tune operation 
    // 0 = disable (i.e. don't tune to selected channel)
    
    // BAND band select
    // 10 = select the 76-108MHz band
    
    // SPACE channel spacing
    // 00 = select spacing of 100kHz between channels
    
  /* register 0x04 */
  0b00000010, 
    // RESERVED 15
    // 0
    
    // PRESUMABLY RESERVED 14
    // 0
    
    // RESERVED 13:12
    // 00
    
    // DE de-emphasis: 
    // 0 = 75us de-emphasis
    
    // RESERVED
    // 
    
    // SOFTMUTE_EN
    // 1 = soft mute enabled
    
    // AFCD AFC disable
    // 0 = AFC enabled
    
  0b00000000, 
    // Bits 7-0 are not specified, so assume all 0's
    // 0000 0000
  
  /* register 0x05 */
  0b10001000, 
    // INT_MODE
    // 1 = interrupt last until read reg 0x0C
    
    // RESERVED 14:12 
    // 000
    
    // SEEKTH seek signal to noise ratio threshold
    // 1000 = suggested default 
  
  0b00001111, 
    // PRESUMABLY RESERVED 7:6
    // 00
    
    // RESERVED 5:4
    // 00
    
    // VOLUME
    // 1111 = loudest volume
  
  /* register 0x06 */
  0b00000000, 
    // RESERVED 15
    // 0
    
    // OPEN_MODE open reserved registers mode
    // 00 = suggested default
    
    // Bits 12:8 are not specified, so assume all 0's
    // 00000 
   
  0b00000000, 
    // Bits 7:0 are not specified, so assume all 0's
    // 00000000
    
  /* register 0x07 */
  0b01000010, 
    // RESERVED 15 
    // 0
    
    // TH_SOFRBLEND threshhold for noise soft blend setting
    // 10000 = using default value
    
    // 65M_50M MODE 
    // 1 = only applies to BAND setting of 0b11, so could probably use 0 here too
    
    // RESERVED 8
    // 0    
  
  0b00000010, 
    // SEEK_TH_OLD seek threshold for old seek mode
    // 000000
    
    // SOFTBLEND_EN soft blend enable
    // 1 = using default value
    
    // FREQ_MODE
    // 0 = using defualt value  
};

// After reset, we can tune the device
// We only need program the first 4 bytes in order to do this
uint8_t tune_config[] = {
  /* register 0x02 */
  0b11000000, 
    // DHIZ audio output high-z disable
    // 1 = normal operation
    
    // DMUTE mute disable 
    // 1 = normal operation
    
    // MONO mono select
    // 0 = stereo
    
    // BASS bass boost
    // 0 = disabled
    
    // RCLK NON-CALIBRATE MODE 
    // 0 = RCLK is always supplied
    
    // RCLK DIRECT INPUT MODE 
    // 0 = ??? not certain what this does
    
    // SEEKUP
    // 0 = seek in down direction
    
    // SEEK
    // 0 = disable / stop seek (i.e. don't seek)
    
   0b00001001, 
    // SKMODE seek mode: 
    // 0 = wrap at upper or lower band limit and contiue seeking
    
    // CLK_MODE clock mode
    //  000 = 32.768kHZ clock rate (match the watch cystal on the module) 
    
    // RDS_EN radio data system enable
    // 1 = enable radio data system
    
    // NEW_METHOD use new demodulate method for improved sensitivity
    // 0 = presumably disabled 
    
    // SOFT_RESET
    // 0 = don't reset this time around
    
    // ENABLE power up enable: 
    // 1 = enabled 

   /* register 0x03 */
   /* Here's where we set the frequency we want to tune to */
   (channel >> 2), 
    // CHAN channel select 8 most significant bits of 10 in total   

   ((channel & 0b11) << 6 ) | 0b00011000
    // CHAN two least significant bits of 10 in total 
    
    // DIRECT MODE used only when test
    // 0 = presumably disabled
    
    // TUNE commence tune operation 
    // 1 = enable (i.e. tune to selected channel)
    
    // BAND band select
    // 10 = select the 76-108MHz band
    
    // SPACE channel spacing
    // 00 = select spacing of 100kHz between channels  
};

#define bytesEnviados 14
#define MAXERR 10
byte dados[10 * QUANTI][bytesEnviados];
int nAudiosL[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int nAudiosR[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

byte channels[QUANTI] = {193};
//byte channels[QUANTI] = {193, 201, 209};
//byte channels[QUANTI] = {235, 187, 241, 249, 121, 15, 19};
String IDS[QUANTI] = {"010953"};
//String IDS[QUANTI] = {"010953", "010961", "010969"};
//String IDS[QUANTI] = {"010995", "010947", "011001", "011009", "010881", "010775", "010779"};
byte num = 0;
byte conterr = 0;
byte t = 0;
bool erro = 0;
byte radio = 0;
int canal = 0;
String ip, porta;
String IDconfig;
String freq;
const char *ssids = "RXfm";
const char *passwords = "rxband";
String s;
String p;
bool conf = false;
bool masterReset = false;
   // channel = (<desired freq in MHz> - 76.0) / 0.1   modo estendido
    // channel 235 dec  =  99.5 MHz
    // channel 187 dec  =  94,7 MHz
    // channel 241 dec  =  100.1 MHz
    // channel 249 dec  =  100.9 MHz
    // channel 121 dec  =  88.1 MHz
    // channel 15 dec  =  77.5 MHz
    // channel 19 dec  =  77,9 MHz
    // channel 193 dec  = 95.3 MHz
    // channel 201 dec  = 96,1  MHz
    // channel 209 dec  = 96,9  MHz
    // channel  dec  =   MHz
    
   
  // channel = (<desired freq in MHz> - 87.0) / 0.1 
  // channel 37 dec  = 90.7 MHz
   // channel 39 dec  = 90.9 MHz
   // channel 41 dec  = 91.1 MHz
   // channel 51 dec  = 92.1 MHz
   // channel 83 dec  = 95.3 MHz
   // channel 91 dec  = 96.1 MHz
   // channel 177 dec = 104.7 MHz
   // channel 193 dec = 106.3 MHz
   // channel 99 dec  = 96.9 MHz
   //Rede Aleluia: 99.5 = 125 
   //Antena 1: 94.7 = 77
   //Transmerica: 100.1 = 131
   //Jovem pan: 100.9 = 139
   //Gazeta: 88.1 = 11



WiFiServer server(80);
WiFiClient client;

String readFile(fs::FS &fs, const char * path){
    
    Serial.printf("Reading file: %s\r\n", path);
    String lido = "";
    char c;
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return "---";
    }

    Serial.println("- read from file:");
    while(file.available()){
        c = file.read();
        lido = lido + c;
    }
    return lido;
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
void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}


void split(String s, char c, String *lista){
  String temp = "";
  int num = 0;
  for (int i = 0; i <= s.length(); i++){
    if (s[i] == c || i == s.length()){
      lista[num] = temp;
      temp = "";
      num++;
    }
    else{
      temp = temp + s[i];  
    } 
  }
}

bool setupWifi(){

  WiFi.disconnect();
  Serial.println("Configuring access point...");
  
  IPAddress local_IP(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  delay(1000);
  int cc = 0;
  while(!WiFi.softAP(ssids, passwords)){
    Serial.print(".");
    delay(1000);
    cc++;
    if (cc>30){
      break;
      }
    }
  delay(1000);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  delay(1000);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println(ssids);
  Serial.println(passwords);
  server.begin();

  Serial.println("Server started");

  while (!conf){
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
            String strIp = lista[2];
            String strPorta = lista[3];
            String strSsid = lista[4];
            String strSenha = lista[5];
            split(strFreq, '=', lista);
            strFreq = lista[1];
            split(strID, '=', lista);
            strID = lista[1];
            split(strIp, '=', lista);
            strIp = lista[1];
            split(strPorta, '=', lista);
            strPorta = lista[1];
            split(strSsid, '=', lista);
            strSsid = lista[1];
            split(strSenha, '=', lista);
            strSenha = lista[1];

            Serial.println();
            Serial.println(strFreq);
            Serial.println(strID);
            Serial.println(strIp);
            Serial.println(strPorta);
            Serial.println(strSsid);
            Serial.println(strSenha);
            Serial.println();
            if(strID.length() > 0){
              conf = true;
              s = strSsid;
              p = strSenha;
              ip = strIp;
              porta = strPorta;
              freq = strFreq;
              IDconfig = strID;
              
              writeFile(SPIFFS, "/ssid.txt", s.c_str());
              writeFile(SPIFFS, "/password.txt", p.c_str());
              writeFile(SPIFFS, "/ip.txt", strIp.c_str());
              writeFile(SPIFFS, "/porta.txt", strPorta.c_str());
              writeFile(SPIFFS, "/ID.txt", strID.c_str());
              writeFile(SPIFFS, "/freq.txt", freq.c_str());

              String x = readFile(SPIFFS, "/ssid.txt");
              Serial.println(readFile(SPIFFS, "/freq.txt"));
              //deleteFile(SPIFFS, "/ssid.txt");
              //deleteFile(SPIFFS, "/password.txt");
              Serial.println(x);
              Serial.println(s);
              Serial.println(p);
              }
            
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
            client.println("<form method=\"get\" action=\"envio\" accept-charset=\"UTF-8\">");
            client.println("<input type=\"text\" name=\"freq\" placeholder=\"Frequencia\"><br>");
            client.println("<input type=\"text\" name=\"ID\" placeholder=\"ID\"><br>");
            client.println("<input type=\"text\" name=\"ip\" placeholder=\"IP\"><br>");
            client.println("<input type=\"text\" name=\"porta\" placeholder=\"Porta\"><br>");
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

}

void IRAM_ATTR resetModule(){

  if (masterReset){

      deleteFile(SPIFFS, "/ssid.txt");
      deleteFile(SPIFFS, "/password.txt");
      deleteFile(SPIFFS, "/ID.txt");
      deleteFile(SPIFFS, "/ip.txt");
      deleteFile(SPIFFS, "/porta.txt");
    
    }
    Serial.println("Resetando");
    delay(3000);
    ESP.restart();
  
  }

void mReset(){
    masterReset = true;
    Serial.print("Resetando....");
    delay(5000);
    resetModule();
  }

int calcCanal(String freq){

  //channel = (<desired freq in MHz> - 76.0) / 0.1 
  
  float temp;
  int c;
    temp = freq.toFloat();
    temp = (temp - 76) * 10;
    Serial.print(temp);
    c = round(temp);
    Serial.println(freq);
    Serial.println(c);
  return c;
  }

void setup()
{
  Serial.begin(9600);
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

  pinMode(MASTER_RESET, INPUT_PULLUP); 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TERRA, OUTPUT);
  digitalWrite(TERRA, LOW);
  //deleteFile(SPIFFS, "/ssid.txt");
  
  s = readFile(SPIFFS, "/ssid.txt");
  p = readFile(SPIFFS, "/password.txt");
  ip = readFile(SPIFFS, "/ip.txt");
  porta = readFile(SPIFFS, "/porta.txt");
  IDconfig = readFile(SPIFFS, "/ID.txt");
  freq = readFile(SPIFFS, "/freq.txt");

  if (p == "---" || s == "---" || ip == "---" || porta == "---" || IDconfig == "---" || freq == "---"){
    setupWifi();
  }

  canal = calcCanal(freq);
  channels[0] = canal;
  IDS[0] = IDconfig;
  
  Serial.println();
    
  WiFi.disconnect();

  if (WiFi.status() != WL_CONNECTED){
    Serial.println(s);
    Serial.println(p);
    WiFi.begin(s.c_str(), p.c_str());
    Serial.println("conectando");
    }
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
    i++;
    if (i > 50)
    {
      erro = 1;
      Serial.println("Erro ao conectar");
      resetModule();
      break;
    }
  }

  Serial.println("Conectando Servidor");
  
  if (!client.connect(host, port)) { 
        Serial.println("Falha ao conectar servidor");
        delay(1000);
        resetModule();
    }
  Serial.println("Servidor Conectado");
  
  pinMode(15, OUTPUT);
  digitalWrite(15,HIGH);
  delay(1000);
 
  Serial.println("\n\nA20150415 RDA5807M FM Tuner\n\n");
  
  Wire.begin(); // join i2c bus (address optional for master)
  
  Serial.print("Configurando...");
  Wire.beginTransmission(RDA5807M_ADDRESS);
  // Write the boot configuration bytes to the RDA5807M
  Wire.write(boot_config, BOOT_CONFIG_LEN);
  Wire.endTransmission();    // stop transmitting  
  Serial.println("Feito.");

  Serial.print("Setando Frequência...");
  Wire.beginTransmission(RDA5807M_ADDRESS);
  // Write the tuning configuration bytes to the RDA5807M
  Wire.write(tune_config, TUNE_CONFIG_LEN);
  Wire.endTransmission();    // stop transmitting 
  Serial.println("Feito.");
  delay(1000);
  tune(channels[0]);
  delay(1000);

  timer = timerBegin (0, 80, true);
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 120000000, true);
  timerAlarmEnable(timer);
  
}
// Load all status registers from to the chip
// registers 0A through 0F
// using the sequential read access mode.
//*************************************************************


void write16(uint16_t val)
{
  Wire.write(val >> 8); Wire.write(val & 0xFF);
} // _write16

uint16_t read16(void)
{
  uint8_t hiByte = Wire.read();
  uint8_t loByte = Wire.read();
  return(256*hiByte + loByte);
} // _read16

void readRegisters()
{
  Wire.requestFrom (I2C_SEQ, (6 * 2) );
  for (int i = 0; i < 6; i++) {
    registers[0xA+i] = read16();
  }
  Wire.endTransmission();
}

void saveRegister(byte regNr)
{
  Wire.beginTransmission(I2C_INDX);
  Wire.write(regNr);
  write16(registers[regNr]);
  Wire.endTransmission();
} // _saveRegister


byte forca(byte d){
  
  byte forca = 0b00000000;

  for (int i = 1; i<7;i++){
    bitWrite(forca, i, bitRead(d, i+1));
    }
  return forca;
  }

void printa(byte d[10]){

  if (bitRead(d[2], 0)){   // 0 = menos significativo
    Serial.print("Portadora: OK ");
  }
  else{
    Serial.print("Portadora: Fora ");
    }
  if (bitRead(d[0], 2)){
    Serial.print("Stereo: OK ");
  }
  else{
    Serial.print("Stereo: Fora ");
    }
  if (bitRead(d[0], 4)){
    Serial.print("RDS: OK ");
  }
  else{
    Serial.print("RDS: Fora ");
    }
  if (bitRead(d[0], 6)){
    Serial.print("Tune: OK ");
    }
  else{
    Serial.print("Tune: No ");
    }

  Serial.print("Força do sinal: ");
  Serial.println(forca(d[2]));
}

//*******************************************************************
float m(int x[10]){

  double sum = 0;
  for (int i = 0; i<10; i++){
    sum =  sum + x[i];
    //Serial.println(x[i]);
    }
  //Serial.println();
  return sum / 10;
  }

float devP(int x[10]){
  double sum = 0;
  float media = m(x);
  //Serial.println(media);
  for (int i = 0; i<10; i++){
    sum += pow(x[i] - media,2);
    }
  sum = sum / (10-1);

  //Serial.println(sum);
  
  return sqrt(sum);
}
int maxA(int x[10]){

  int maximo = 0;

  for (int i = 0; i < 10; i++){

    if(x[i] > maximo){
      maximo = x[i];
      }
    }
  return maximo;
}

void verifica(int t, int r){
  do {
    Wire.requestFrom(RDA5807M_ADDRESS, bytesEnviados - 2);
      int i = 0;
      while (Wire.available()) {
        while (i==4 || i==5){
          i++;//pular bytes 5 e 6, deixar para audio
          }
        dados[t + r][i] = Wire.read();
        //Serial.print(dados[t][i]);
        //Serial.print(" " + String(i/2) + " ");
        for (int j = 7; j >= 0 ; j--){
            
            //Serial.print(bitRead(dados[t + r][i], j));
            
          }
        if (i == 16){
          //Serial.println();
          }
        else{
        //Serial.print(" ");
        }
        i = i+1;
        }
      Wire.endTransmission();    // stop transmitting
      //Serial.println();
      //Serial.print(analogRead(34));
      //Serial.print(' ');
      //Serial.print(analogRead(35));
  
      //float devPAL = devP(nAudiosL);
      //float devPAR = devP(nAudiosR);
  
      int maxL = map(analogRead(34), 0, 4095, 0, 255);
      int maxR = map(analogRead(35), 0, 4095, 0, 255);
  
      //dados[t][4] = devPAL;
      //dados[t][5] = devPAR;
  
      dados[t + r][4] = maxL;
      dados[t + r][5] = maxR;
  
      //Serial.print(maxL);
      //Serial.print(' ');
      //Serial.println(maxR);
      
      if (maxL > THRESHAUD){
        bitWrite(dados[t][3], 0, 1);
        }
      else{
        bitWrite(dados[t][3], 0, 0);
        }
      if (maxR > THRESHAUD){
        bitWrite(dados[t][3], 1, 1);
        }
      else{
        bitWrite(dados[t][3], 1, 0);
        }
      
      //printa(dados[t + r]);
      //for (byte k = 0; k<10; k++){
      //    for (byte l = 0; l<4; l++){
      //      Serial.print(dados[k][l]);
      //      Serial.print(' ');
      //      }
      //      Serial.println();
      //    }
  }
  while(!bitRead(dados[t][0], 6));
}

void tune(uint16_t c){
  uint8_t tune_config[] = {
    0b11000000,
    0b00001001, 
    (c >> 2),
    ((c & 0b11) << 6 ) | 0b00011000
  };
  //Serial.print("Setando Frequência...");
  Wire.beginTransmission(RDA5807M_ADDRESS);
  // Write the tuning configuration bytes to the RDA5807M
  Wire.write(tune_config, TUNE_CONFIG_LEN);
  Wire.endTransmission();    // stop transmitting 
  //Serial.println("Feito.");
}

void enviaSocket(String dados)
{
    Serial.println("Enviando....");
    
    if (!client.connected()){
      Serial.println("Desconectado do servidor, Reconectando....");
      if (!client.connect(host, port)){
   
          Serial.println("Falha ao conectar servidor");
          
          delay(1000);
          resetModule();
          return;
      }

      Serial.println("Servidor conectado com Sucesso!");
    }
    if (client.print(dados) == 0){
        conterr++;
      }
    else{
        conterr = 0;
      }
    if (conterr > MAXERR){
      resetModule();
      }
 
    //client.stop();
 
    //delay(10000);
}
void envia(int indice){
          t = 0;
      int i = 0;
      erro = 0;
      Serial.println("comecando a enviar");
           
      if (WiFi.status() != WL_CONNECTED){
        Serial.println(s);
        Serial.println(p);
        WiFi.begin(s.c_str(), p.c_str());
        Serial.println("conectando");
        }
      
      while (WiFi.status() != WL_CONNECTED)
      {
        Serial.print(".");
        delay(100);
        i++;
        if (i > 50)
        {
          erro = 1;
          Serial.println("Erro ao conectar");
          //conterr ++;
          //if (conterr > 10){
          //  resetModule();
          //  }
          break;
        }
      }
      if (erro == 0)
      {
        HTTPClient http;
        Serial.println("enviando");
        
        String stringDados[10] =  {"","","","","","","","","",""};
        
        String stringID = IDS[indice];
        
        for (byte k = 0; k<10; k++){
          for (byte l = 0; l<bytesEnviados; l++){
            stringDados[k] = stringDados[k] + String(dados[k + indice * 10][l]) + "-";
            }
            //Serial.println(stringDados[k]);
          }
        //for (byte k = 0; k<10; k++){
        //  for (byte l = 0; l<4; l++){
        //    Serial.print(dados[k][l]);
        //    Serial.print(' ');
        //    }
        //    Serial.println();
        // }
    
        //String link = "http://172.0.0.139:8080/cgi-bin/cgi1.py?ID="+stringID;
        String link = "http://201.63.9.94:8080/cgi-bin/cgi1.py?ID="+stringID;

        String envioSocket = "I|" + stringID;
        
        for (int k = 0; k<10; k++){
          
          link = link+"&dados"+k+"="+stringDados[k];
          envioSocket += "|" + stringDados[k];
          }

        envioSocket = envioSocket + "|F";

        //Serial.println(envioSocket);

        enviaSocket(envioSocket);

        
        }
      
      //WiFi.disconnect();
  }

void loop()
{
  //Serial.print(".");
  timerWrite(timer, 0);
  if (!digitalRead(MASTER_RESET)){
    mReset();
    }
  t = 0;
  for (int m = 0; m < 2; m++){
    for (radio = 0; radio < QUANTI * 10; radio = radio + 10){
      delay(200);
     for (int medida = 0; medida < 5; medida++){
        delay(50);
        verifica(t + medida, radio);
      }
      num++;
      if (num>=QUANTI){
        num = 0;
      }     
      tune(channels[num]);
      
    }
    t++;
    t++;
    t++;
    t++;
    t++;
  }
  //Serial.println(t);
  for (int i = 0; i < QUANTI; i++){
    Serial.print("Enviando: ");
    Serial.println(i);
    envia(i);
  }
}
