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

// I2C-Address RDA Chip for Index  Access
#define I2C_INDX  0x11

#define QUANTI 3

#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>
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
uint16_t channel = 99;
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

const char *NSSID = "renato"; //SSD
const char *SENHA = "lablab01"; // Senha
//const char *NSSID = "ATBOSQUE"; //SSD
//const char *SENHA = "individual"; // Senha
byte t = 0;
bool erro = 0;

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

  0b00000000,
    // CHAN two least significant bits of 10 in total 
    // 00 = don't bother to program a channel at this time
    
    // DIRECT MODE used only when test
    // 0 = presumably disabled
    
    // TUNE commence tune operation 
    // 0 = disable (i.e. don't tune to selected channel)
    
    // BAND band select
    // 00 = select the 87-108MHz band
    
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

   ((channel & 0b11) << 6 ) | 0b00010000
    // CHAN two least significant bits of 10 in total 
    
    // DIRECT MODE used only when test
    // 0 = presumably disabled
    
    // TUNE commence tune operation 
    // 1 = enable (i.e. tune to selected channel)
    
    // BAND band select
    // 00 = select the 87-108MHz band
    
    // SPACE channel spacing
    // 00 = select spacing of 100kHz between channels  
};
byte dados[10][4];
byte channels[QUANTI] = { 83, 91, 99};
String IDS[QUANTI] = {"010953","010961","010969"};
byte num = 0;


  // channel 37 dec  = 90.7 MHz
   // channel 39 dec  = 90.9 MHz
   // channel 41 dec  = 91.1 MHz
   // channel 51 dec  = 92.1 MHz
   // channel 83 dec  = 95.3 MHz
   // channel 91 dec  = 96.1 MHz
   // channel 177 dec = 104.7 MHz
   // channel 193 dec = 106.3 MHz
   // channel 99 dec  = 96.9 MHz


void setup()
{
  pinMode(15, OUTPUT);
  digitalWrite(15,HIGH);
  delay(1000);
 
  Serial.begin(9600);
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

  for (int i = 1; i<8;i++){
    bitWrite(forca, i, bitRead(d, i));
    }
  return forca;
  }

void printa(byte d[10]){

  if (bitRead(d[2], 0)){
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

  Serial.print("Força do sinal: ");
  Serial.println(forca(d[2]));
}

//*******************************************************************


void verifica(int t){
  Wire.requestFrom(RDA5807M_ADDRESS,4);
    int i = 0;
    while (Wire.available()) {
      dados[t][i] = Wire.read();
      Serial.print(dados[t][i]);
      Serial.print(" " + String(i/2) + " ");
      for (int j = 7; j >= 0 ; j--){
          
          Serial.print(bitRead(dados[t][i], j));
          
        }
      if (i == 16){
        Serial.println();
        }
      else{
      Serial.print(" ");
      }
      i = i+1;
      }
    Wire.endTransmission();    // stop transmitting
    Serial.println();
    //Serial.print(analogRead(34));
    //Serial.print(analogRead(35));
    if (analogRead(34) > 300){
      bitWrite(dados[t][3], 0, 1);
      }
    else{
      bitWrite(dados[t][3], 0, 0);
      }
    if (analogRead(35) > 300){
      bitWrite(dados[t][3], 1, 1);
      }
    else{
      bitWrite(dados[t][3], 1, 0);
      }
    
    //printa(dados[t]);
}

void tune(uint16_t c){
  uint8_t tune_config[] = {
    0b11000000,
    0b00001001, 
    (c >> 2),
    ((c & 0b11) << 6 ) | 0b00010000
  };
  //Serial.print("Setando Frequência...");
  Wire.beginTransmission(RDA5807M_ADDRESS);
  // Write the tuning configuration bytes to the RDA5807M
  Wire.write(tune_config, TUNE_CONFIG_LEN);
  Wire.endTransmission();    // stop transmitting 
  //Serial.println("Feito.");
};

void loop()
{
  //Serial.print(".");
  
  verifica(t);
  t++;
  
  if (t >= 10)
    { 
      t = 0;
      int i = 0;
      erro = 0;
      Serial.println("comecando a enviar");
           
      if (WiFi.status() != WL_CONNECTED){
        WiFi.begin(NSSID, SENHA);
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
        Serial.println("enviando GET");
        
        String stringDados[10] =  {"","","","","","","","","",""};
        
        String stringID = IDS[num];
        
        for (byte k = 0; k<10; k++){
          for (byte l = 0; l<4; l++){
            stringDados[k] = stringDados[k] + String(dados[k][l]) + "-";
            }//Serial.println(stringDados);
          }
    
        String link = "http://172.0.0.106:8080/cgi-bin/cgi1.py?ID="+stringID;

        for (int k = 0; k<10; k++){
          
          link = link+"&dados"+k+"="+stringDados[k];
          }
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
            
            //Serial.print("Payload ");
            //Serial.println(payload);
            
            num++;
            if (num>=QUANTI){
              num = 0;
            }
            tune(channels[num]);
          }
          else{
            Serial.println("Falha Http Server");
            delay(30000);
          }
        } 
        else
        {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          Serial.print("Erro ao enviar");
          //conterr ++;
          //if (conterr > 10){
          //  resetModule();
          //  }
        }
        http.end();
        }
      
      //WiFi.disconnect();
    }
    delay(100);
    
}
