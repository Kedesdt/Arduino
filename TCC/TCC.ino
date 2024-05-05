#include <M2M_LM75A.h>

#include <MAX30100.h>

#include <Wire.h>
//#include <MAX30100_PulseOximeter.h>
#include <MPU6050.h>
//#include <LM75A.h>

// Inicialize os dispositivos
PulseOximeter pox;
MPU6050 mpu;
M2M_LM75A lm75;

void setup() {
  Serial.begin(115200);

  // Inicialize o MAX30100
  if (!pox.begin()) {
    Serial.println("Falha ao inicializar o MAX30100");
    while (1);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Inicialize o MPU-6050
  Wire.begin();
  mpu.initialize();

  // Inicialize o LM75A
  !lm75.begin()

}

void loop() {
  // Leia e imprima os dados do MAX30100
  pox.update();
  Serial.print("Batimentos Cardíacos: "); Serial.print(pox.getHeartRate()); Serial.println("bpm");
  Serial.print("Oximetria: "); Serial.print(pox.getSpO2()); Serial.println("%");

  // Leia e imprima os dados do MPU-6050
  Serial.print("Aceleração: "); Serial.print(mpu.getAccelerationX()); Serial.print(", ");
  Serial.print(mpu.getAccelerationY()); Serial.print(", "); Serial.println(mpu.getAccelerationZ());
  Serial.print("Giroscópio: "); Serial.print(mpu.getRotationX()); Serial.print(", ");
  Serial.print(mpu.getRotationY()); Serial.print(", "); Serial.println(mpu.getRotationZ());

  // Leia e imprima os dados do LM75A
  Serial.print("Temperatura: "); Serial.print(lm75.getTemperature()); Serial.println("°C");

  delay(1000);
}