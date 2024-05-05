#include <DS3231.h> 
#include "HX711.h"
#include "LiquidCrystal_I2C.h";

DS3231  rtc(SDA, SCL);
Time t;
HX711 escala;

#define DT A1
#define SCK A0

LiquidCrystal_I2C lcd(0x27, 16, 2);
 
//parâmetros de horário que serão atualizados
int horaAtual, minutoAtual;

//parâmetros primeira alimentação
int horaAlimentacao1, minutoAlimentacao1, demosComida1;
//parâmetros segunda alimentação
int horaAlimentacao2, minutoAlimentacao2, demosComida2;

//#####################################
int peso_desejado = 200; // coloque o peso desejado aqui 
int tempo = 0, peso = 0;
int tempo_maximo = 60000; //tempo máximo para parar de colocar comida mesmo se não atingiu o peso


void setup(){

  escala.begin (DT, SCK);
 
  lcd.init();
  lcd.backlight();
 
  Serial.begin(9600);
  Serial.print("Leitura do Valor ADC:  ");
  Serial.println(escala.read());   // Aguada até o dispositivo estar pronto
  Serial.println("Nao coloque nada na balanca!");
  Serial.println("Iniciando...");
  escala.set_scale(397930.55);     // Substituir o valor encontrado para escala
  escala.tare(20);                // O peso é chamado de Tare.
  Serial.println("Insira o item para Pesar");

  
 //inicia o módulo relógio
 rtc.begin(); 
 //inicia o monitor serial
 //Serial.begin(115200); 

 //determina o pino do relé
 pinMode(8, OUTPUT); 

 //determina o horário da primeira alimentação
 horaAlimentacao1 = 22; 
 minutoAlimentacao1 = 44;

 //determina o horário da segunda alimentação
 horaAlimentacao2 = 22; 
 minutoAlimentacao2 = 45;

 //determina o status de alimentação. 0 equivale a não e 1 a sim
 demosComida1 = 0;   
 demosComida2 = 0; 
 
  //as linhas abaixo devem ser descomentadas para configurar o relógio interno. Descomente, carregue o código para o arduino, comente novamente e suba o código mais uma vez.
  rtc.setDate(24,6,2022);    // determina a data (dia, mes, ano)
  rtc.setDOW(FRIDAY);     // determina o dia da semana
  rtc.setTime(22,43, 0);     // determina o horário (hora, minuto, segundo)

 // desliga o relé para começar.
 digitalWrite(8, HIGH);

} 
void printa_peso(int peso){
  
  lcd.setCursor(4, 0);
  lcd.print("Racao");
  lcd.setCursor(0,1);
  lcd.print("Peso: ");
  lcd.print(peso, 3);
  lcd.println(" g  ");
  
  }
void loop()
{
 
  lcd.setCursor(4, 0);
  lcd.print("Racao");
  lcd.setCursor(0,1);
  lcd.print("Peso: ");
  lcd.print(escala.get_units(20), 3);
  lcd.println(" g  ");
  delay(100);

  //determina o horário atual
  t = rtc.getTime();
  horaAtual = t.hour;
  minutoAtual = t.min;

 
  //verifica se é o horário da primeira alimentação
  if (horaAtual == horaAlimentacao1 && minutoAtual == minutoAlimentacao1 && demosComida1 == 0 ){

    tempo = 0;
    
    digitalWrite(8, LOW);
    while (escala.get_units(20) < peso_desejado && tempo < tempo_maximo){

      printa_peso(escala.get_units(20));
      tempo = tempo + 100;
      delay(100);
      
      }    
    digitalWrite(8, HIGH);
    demosComida1 = 1; //altera status da comida1
    
  }

  //verifica se é o horário da segunda alimentação
  if (horaAtual == horaAlimentacao2 && minutoAtual == minutoAlimentacao2 && demosComida2 == 0 ){

    tempo = 0;
    
    digitalWrite(8, LOW);
    while (escala.get_units(20) < peso_desejado && tempo < tempo_maximo){

      printa_peso(escala.get_units(20));
      tempo = tempo + 100;
      delay(100);
      
      }    
    digitalWrite(8, HIGH);
    demosComida2 = 1; //altera status da comida2
    
  }

  //Imprime o horário da próxima alimentação
  if (demosComida1 == 0 || demosComida1 == 1 && demosComida2 == 1 ){
    Serial.print("Horário atual: ");
    Serial.println(rtc.getTimeStr());
    Serial.print("Próxima alimentação: ");
    Serial.print(horaAlimentacao1);
    Serial.print("h:");
    Serial.print(minutoAlimentacao1);
    Serial.println("min");
    Serial.println("  ");
  }

  if (demosComida1 == 1 && demosComida2 == 0){
     Serial.print("Horário atual: ");
    Serial.println(rtc.getTimeStr());
    Serial.print("Próxima alimentação: ");
    Serial.print(horaAlimentacao2);
    Serial.print("h:");
    Serial.print(minutoAlimentacao2);
    Serial.println("min");
    Serial.println("  ");
  }
 
  //meia noite reseta o status de comida do dia 
  if (horaAtual == 0 && minutoAtual == 0){
    demosComida1 = 0;
    demosComida2 = 0; 
  }
 
  //atualiza monitor serial
  delay (1000);
  
}
