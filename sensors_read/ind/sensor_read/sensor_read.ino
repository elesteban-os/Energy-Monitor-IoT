/**************************************
   MEDICIÓN DE VOLTAJE AC (Vrms) + FRECUENCIA
   Sensor: ZMPT101B
   MCU: ESP32 / ESP32-S3 / C3
***************************************/

const int adcPin = 4;            // Pin ADC donde conectaste el ZMPT
const float Vref = 3.3;          // Referencia ADC
const int ADCmax = 4095;         // Resolución ADC del ESP32
const float offset = 1.65;       // Offset típico del módulo

float calibrationFactor = 267.890625;   

// Variables para frecuencia
unsigned long lastCrossTime = 0;
bool lastAbove = false;
float frequency = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  delay(300);
}

void loop() {

  /************* MEDICIÓN VRMS *************/
  const int samples = 3000;  // Suficiente para una medición estable
  double sumSq = 0;

  for (int i = 0; i < samples; i++) {
    int raw = analogRead(adcPin);

    // Convertir el valor ADC a voltaje
    float voltage = raw * (Vref / ADCmax);

    // Centrar la señal (quitar offset)
    float centered = voltage - offset;

    // Acumular para RMS
    sumSq += centered * centered;

    /************* MEDICIÓN FRECUENCIA *************/
    bool above = (centered > 0);

    // Detectar cruce por cero positivo
    if (!lastAbove && above) {
      unsigned long now = micros();
      if (lastCrossTime != 0) {
        unsigned long period = now - lastCrossTime;
        frequency = 1e6 / period;   // f = 1/T
      }
      lastCrossTime = now;
    }

    lastAbove = above;

    //delayMicroseconds(200);  // ~5000 muestras/s
  }

  // Calcular RMS del módulo
  float Vrms_module = sqrt(sumSq / samples);

  // Convertir a tensión real
  float Vrms = Vrms_module * calibrationFactor;

  /************* IMPRIMIR RESULTADOS *************/
  Serial.println("===== MEDICIONES =====");
  Serial.print("Vrms: ");
  Serial.print(Vrms, 2);
  Serial.println(" V");

  Serial.print("Frecuencia: ");
  Serial.print(frequency, 2);
  Serial.println(" Hz");

  Serial.println("=======================\n");

  //delay(400);
}
