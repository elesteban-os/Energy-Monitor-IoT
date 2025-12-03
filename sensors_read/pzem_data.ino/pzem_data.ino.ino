#include <PZEM004Tv30.h>

// Serial1 en ESP32-C3 (UART1)
// RX = 20, TX = 21
PZEM004Tv30 pzem(&Serial1, 20, 21);

void setup() {
  Serial.begin(115200);

  delay(500);

  Serial1.begin(9600, SERIAL_8N1, 20, 21);
  Serial.println("Leyendo PZEM004T v3...");
  pzem.resetEnergy();
}

void loop() {
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();
  float freq    = pzem.frequency();
  float pf      = pzem.pf();

  Serial.println("---------- PZEM004T ----------");

  if(!isnan(voltage))   Serial.println(String("Voltaje: ") + voltage + " V");
  else                  Serial.println("Voltaje: error");

  if(!isnan(current))   Serial.println(String("Corriente: ") + current + " A");
  else                  Serial.println("Corriente: error");

  if(!isnan(power))     Serial.println(String("Potencia: ") + power + " W");
  else                  Serial.println("Potencia: error");

  if(!isnan(energy))    Serial.println(String("Energía: ") + energy + " Wh");
  else                  Serial.println("Energía: error");

  if(!isnan(freq))      Serial.println(String("Frecuencia: ") + freq + " Hz");
  else                  Serial.println("Frecuencia: error");

  if(!isnan(pf))        Serial.println(String("Factor Potencia: ") + pf);
  else                  Serial.println("FP: error");

  Serial.println("-----------------------------\n");

  delay(50);
}