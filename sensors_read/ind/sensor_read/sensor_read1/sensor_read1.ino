/*************************************************
 *   MEDICIÓN COMPLETA:
 *   - Voltaje RMS
 *   - Frecuencia 
 *   - Corriente 
 *   - Potencia, Energía, Factor de potencia
 *************************************************/


// CONFIG ZMPT101B //
const int pinZMPT = 4;  
const float Vref = 3.3;
const int ADCmax = 4095;
float offsetZMPT = 1.65;
float calibrationFactor = 267.890625;

// Frecuencia
unsigned long lastCrossTime = 0;
bool lastAbove = false;
float frequency = 0.0;

// CONFIG ACS712 5A   //
const int pinACS = 1;
const float R1 = 10000.0;
const float R2 = 20000.0;
const float DIV_FACTOR = (R1 + R2) / R2; 
const float SENS = 0.185; 
float VoffsetACS = 0.0;   

// ENERGÍA
float energy_Wh = 0.0;
unsigned long lastEnergyUpdate = 0;



// Promedia N lecturas del ACS712
long readAverageACS(int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pinACS);
    delay(2);
  }
  return sum / samples;
}


void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  delay(500);

  // Calibración ACS712 en vacío
  Serial.println("\nCalibrando ACS712...");
  long rawAvg = readAverageACS(500);
  float Vadc = (rawAvg * Vref) / ADCmax;
  VoffsetACS = Vadc * DIV_FACTOR;
  Serial.print("Offset ACS712 (V): ");
  Serial.println(VoffsetACS, 5);

  lastEnergyUpdate = millis();
}


void loop() {

  const int samples = 2000;  
  double sumSqVolt = 0;  
  double sumSqCurr = 0;  
  float realPowerSum = 0;

  // Para factor de potencia
  float lastV = 0, lastI = 0;

  for (int i = 0; i < samples; i++) {

    // ==========================
    // VOLTAJE (ZMPT101B)
    int rawV = analogRead(pinZMPT);
    float Vzmpt_adc = rawV * (Vref / ADCmax);
    float centeredV = Vzmpt_adc - offsetZMPT;

    // frecuencia
    bool above = (centeredV > 0);
    if (!lastAbove && above) {
      unsigned long now = micros();
      if (lastCrossTime != 0) {
        unsigned long period = now - lastCrossTime;
        frequency = 1e6 / period;
      }
      lastCrossTime = now;
    }
    lastAbove = above;

    sumSqVolt += centeredV * centeredV;

    // ==========================
    // CORRIENTE (ACS712)
    int rawI = analogRead(pinACS);
    float Iadc = (rawI * Vref) / ADCmax;
    float Iout = Iadc * DIV_FACTOR;

    float instCurrent = (Iout - VoffsetACS) / SENS;
    sumSqCurr += instCurrent * instCurrent;

    // ==========================
    // POTENCIA INSTANTÁNEA
    // aproximada: P = Vinstant * Iinstant
    realPowerSum += (centeredV * calibrationFactor) * instCurrent;

    delayMicroseconds(300);
  }

  // ========= RMS =========
  float Vrms = sqrt(sumSqVolt / samples) * calibrationFactor;
  float Irms = sqrt(sumSqCurr / samples);

  if (Vrms < 100) {
    Vrms = 0;
  }
  if (Irms < 0.100) {
    Irms = 0;
  }

  // ========= Potencia =========
  float realPower = realPowerSum / samples;       // Watts
  if (Vrms == 0 || Irms == 0) {
      realPower = 0;
  }

  float apparentPower = Vrms * Irms;              // VA
  float powerFactor = 0;

  if (apparentPower > 0.1) {
    powerFactor = realPower / apparentPower;
  }

  // ========= Energía =========
  unsigned long now = millis();
  float dt_h = (now - lastEnergyUpdate) / 3600000.0;  // horas
  energy_Wh += realPower * dt_h;
  lastEnergyUpdate = now;

  // ========== OUTPUT ==========
  Serial.println("\n===== MEDICIONES =====");

  Serial.print("Voltaje RMS: ");
  Serial.print(Vrms, 2);
  Serial.println(" V");

  Serial.print("Corriente RMS: ");
  Serial.print(Irms, 3);
  Serial.println(" A");

  Serial.print("Frecuencia: ");
  Serial.print(frequency, 2);
  Serial.println(" Hz");

  Serial.print("Potencia Real (P): ");
  Serial.print(realPower, 2);
  Serial.println(" W");

  Serial.print("Factor de Potencia (PF): ");
  Serial.println(powerFactor, 2);

  Serial.print("Energía acumulada: ");
  Serial.print(energy_Wh, 3);
  Serial.println(" Wh");

  Serial.println("=======================\n");

  delay(300);
}
