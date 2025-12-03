/*************************************************
 *   MEDICIÓN COMPLETA:
 *   - Voltaje RMS
 *   - Frecuencia 
 *   - Corriente 
 *   - Potencia, Energía, Factor de potencia
 *************************************************/

// wifi
#include <WiFi.h>
#include <HTTPClient.h>

//Configuracion para el wifi
// aqui hay que poner el nombre de la red donde este y la contraseña para que se conecte al Wifi
const char* ssid = "iPhone";
const char* password = "12123434";

//la url del api, ahorita es una de prueba
String api_url ="https://eocit80vsw6avkx.m.pipedream.net";

//identificacion del microcontrolador
String generarSerial(){
  WiFi.mode(WIFI_STA);
  delay(100);
  String mac = WiFi.macAddress();
  mac.replace(":","");
  return "ESP32C3-" + mac;
}
String serialDispositivo;

// conexion al wifi
void conectarWiFi(){
  Serial.println("Conectando al wifi...");
  WiFi.begin(ssid,password);

  while (WiFi.status() != WL_CONNECTED){
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

//POST EN LA API
void enviarLectura(float voltaje, float corriente, float potencia, float energia, float frecuencia, float fp){
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi caído, reconectando...");
    conectarWiFi();
  }

  HTTPClient http;
  http.begin(api_url);
  http.addHeader("Content-Type","application/json");

  String json = "{";
  json += "\"sensor_name\": \"ZMPT101B + ACS712  \",";
  json += "\"Voltaje\":" + String(voltaje,2) + ",";
  json += "\"Corriente\":" + String(corriente,2) + ",";
  json += "\"Potencia\":" + String(potencia,2) + ",";
  json += "\"Energia\":" + String(energia,2) + ",";
  json += "\"Frecuencia\":" + String(frecuencia,2) + ",";
  json += "\"Factor Potencia\":" + String(fp,2);
  json += "}";

  int code = http.POST(json);

  Serial.print("POST código; ");
  Serial.println(code);
  Serial.println("Respuesta: " + http.getString());

  http.end();

}

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

  lastEnergyUpdate = millis();
  
  // Wifi
  WiFi.mode(WIFI_STA);
  serialDispositivo = generarSerial();
  Serial.println("Serial: " + serialDispositivo);

  conectarWiFi();

  // Calibración ACS712 en vacío
  Serial.println("\nCalibrando ACS712...");
  long rawAvg = readAverageACS(500);
  float Vadc = (rawAvg * Vref) / ADCmax;
  VoffsetACS = Vadc * DIV_FACTOR;
  Serial.print("Offset ACS712 (V): ");
  Serial.println(VoffsetACS, 5);
}

//loop
unsigned long previo = 0;
const unsigned long intervalo = 5000; // son 5 segundos
void loop() {

  const int samples = 2000;
  double sumSqVolt = 0;
  double sumSqCurr = 0;
  double sumPower  = 0;

  unsigned long startMicros = micros();

  for (int i = 0; i < samples; i++) {

    // ==============================
    // LECTURA VOLTAJE
    // ==============================
    float rawV = analogRead(pinZMPT);
    float Vadc = rawV * (Vref / ADCmax);
    float Vin = Vadc - offsetZMPT;

    // Cálculo de frecuencia por cruce de cero
    bool above = (Vin > 0);
    if (!lastAbove && above) {
      unsigned long now = micros();
      if (lastCrossTime != 0) {
        unsigned long periodo = now - lastCrossTime;
        frequency = 1e6 / periodo;
      }
      lastCrossTime = now;
    }
    lastAbove = above;

    sumSqVolt += Vin * Vin;

    // ==============================
    // LECTURA CORRIENTE
    // ==============================
    float rawI = analogRead(pinACS);
    float Iadc = rawI * (Vref / ADCmax);
    float Iout = Iadc * DIV_FACTOR;
    float Iinst = (Iout - VoffsetACS) / SENS;

    sumSqCurr += Iinst * Iinst;

    // ==============================
    // POTENCIA INSTANTÁNEA
    // Usa la misma forma de onda sin delay
    // ==============================
    sumPower += (Vin * calibrationFactor) * Iinst;
  }

  // ===============================
  // CÁLCULOS RMS
  // ===============================
  float Vrms = sqrt(sumSqVolt / samples) * calibrationFactor;
  float Irms = sqrt(sumSqCurr / samples);

  // ===============================
  // TOLERANCIAS
  // ===============================
  if (Vrms < 100) Vrms = 0;
  if (Irms < 0.10) Irms = 0;

  // ===============================
  // POTENCIA
  // ===============================
  float realPower = sumPower / samples;

  if (Vrms == 0 || Irms == 0) realPower = 0;

  float apparentPower = Vrms * Irms;
  float powerFactor = (apparentPower > 1) ? realPower / apparentPower : 0;

  // ===============================
  // ENERGÍA
  // ===============================
  unsigned long now = millis();
  float dt_h = (now - lastEnergyUpdate) / 3600000.0;
  energy_Wh += realPower * dt_h;
  lastEnergyUpdate = now;

  // ===============================
  // IMPRIMIR
  // ===============================
  Serial.println("\n===== MEDICIONES =====");
  Serial.printf("Voltaje RMS: %.2f V\n", Vrms);
  Serial.printf("Corriente RMS: %.3f A\n", Irms);
  Serial.printf("Frecuencia: %.2f Hz\n", frequency);
  Serial.printf("Potencia Real: %.2f W\n", realPower);
  Serial.printf("Factor de Potencia: %.3f\n", powerFactor);
  Serial.printf("Energía: %.3f Wh\n", energy_Wh);
  Serial.println("=======================\n");

  // ===============================
  // ENVÍO A API CADA 5 SEG
  // ===============================
  if (millis() - previo >= intervalo) {
    previo = millis();
    enviarLectura(Vrms, Irms, realPower, energy_Wh, frequency, powerFactor);
  }
}
