#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

// CONFIG ZMPT101B
const int pinZMPT = 4;
const float Vref = 3.3;
const int ADCmax = 4095;
float offsetZMPT = 1.65;
float calibrationFactor = 267.890625;

// Frecuencia
unsigned long lastCrossTime = 0;
bool lastAbove = false;
float frequency = 0.0;

// CONFIG ACS712 5A
const int pinACS = 1;
const float R1 = 10000.0;
const float R2 = 20000.0;
const float DIV_FACTOR = (R1 + R2) / R2;
const float SENS = 0.185;
float VoffsetACS = 0.0;

// ENERGIA
float energy_Wh = 0.0;
unsigned long lastEnergyUpdate = 0;

// WiFi / API
const char* WIFI_SSID = "nombre exacto del wifi visto desde el cel| importante desactivar el firewall de windows";
const char* WIFI_PASS = "contraseña del wifi";
const char* API_BASE  = "http://#ipv4:8000"; 
const char* SERIAL_ID = "ESP32_B";
const char* SENSOR_NAME = "ZMPT+ACS712";

Preferences prefs;
String token = "";
unsigned long lastSend = 0;
bool tokenAvisado = false;

long readAverageACS(int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pinACS);
    delay(2);
  }
  return sum / samples;
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());
}

bool extractToken(const String& payload, String& outToken) {
  int idx = payload.indexOf("\"token\":\"");
  if (idx < 0) return false;
  int start = idx + 9;
  int end = payload.indexOf("\"", start);
  if (end < 0) return false;
  outToken = payload.substring(start, end);
  return outToken.length() > 0;
}

bool postJson(const String& path, const String& body, String& response) {
  if (WiFi.status() != WL_CONNECTED) connectWifi();
  HTTPClient http;
  http.begin(String(API_BASE) + path);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  response = http.getString();
  http.end();
  Serial.printf("POST %s -> %d\n", path.c_str(), code);
  if (code < 200 || code >= 300) {
    Serial.println("Resp: " + response);
    return false;
  }
  return true;
}

bool ensureToken() {
  token = prefs.getString("token", "");
  if (token.length() > 0) return true;
  String body = String("{\"serial\":\"") + SERIAL_ID + "\",\"location\":\"LAB\",\"type\":\"energy_meter\"}";
  String resp;
  if (!postJson("/register", body, resp)) {
    Serial.println("Registro fallido.");
    return false;
  }
  if (!extractToken(resp, token)) {
    Serial.println("No se pudo extraer token.");
    return false;
  }
  prefs.putString("token", token);
  Serial.println("Token guardado: " + token);
  return true;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  prefs.begin("energy", false);
  connectWifi();

  // Calibracion ACS712 en vacio
  Serial.println("\nCalibrando ACS712...");
  long rawAvg = readAverageACS(500);
  float Vadc = (rawAvg * Vref) / ADCmax;
  VoffsetACS = Vadc * DIV_FACTOR;
  Serial.print("Offset ACS712 (V): ");
  Serial.println(VoffsetACS, 5);

  lastEnergyUpdate = millis();
}

void loop() {
  if (!ensureToken()) {
    delay(2000);
    return;
  }
  if (!tokenAvisado && token.length() > 0) {
    Serial.println("Token cargado desde NVS.");
    tokenAvisado = true;
  }

  const int samples = 2000;
  double sumSqVolt = 0;
  double sumSqCurr = 0;
  float realPowerSum = 0;

  for (int i = 0; i < samples; i++) {
    // VOLTAJE (ZMPT101B)
    int rawV = analogRead(pinZMPT);
    float Vzmpt_adc = rawV * (Vref / ADCmax);
    float centeredV = Vzmpt_adc - offsetZMPT;

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

    // CORRIENTE (ACS712)
    int rawI = analogRead(pinACS);
    float Iadc = (rawI * Vref) / ADCmax;
    float Iout = Iadc * DIV_FACTOR;
    float instCurrent = (Iout - VoffsetACS) / SENS;
    sumSqCurr += instCurrent * instCurrent;

    // POTENCIA INSTANTANEA aproximada
    realPowerSum += (centeredV * calibrationFactor) * instCurrent;

    delayMicroseconds(300);
  }

  // RMS
  float Vrms = sqrt(sumSqVolt / samples) * calibrationFactor;
  float Irms = sqrt(sumSqCurr / samples);
  if (Vrms < 100) Vrms = 0;
  if (Irms < 0.100) Irms = 0;

  // Potencia
  float realPower = realPowerSum / samples;
  if (Vrms == 0 || Irms == 0) realPower = 0;
  float apparentPower = Vrms * Irms;
  float powerFactor = (apparentPower > 0.1) ? (realPower / apparentPower) : 0;

  // Energia acumulada
  unsigned long now = millis();
  float dt_h = (now - lastEnergyUpdate) / 3600000.0;
  energy_Wh += realPower * dt_h;
  lastEnergyUpdate = now;

  // Limitamos la frecuencia de envio
  if (millis() - lastSend < 5000) {
    return;
  }
  lastSend = millis();

  String payload = "{";
  payload += "\"token\":\"" + token + "\",";
  payload += "\"sensor_name\":\"" + String(SENSOR_NAME) + "\",";
  payload += "\"Voltaje\":" + String(Vrms, 2) + ",";
  payload += "\"Corriente\":" + String(Irms, 3) + ",";
  payload += "\"Potencia\":" + String(realPower, 2) + ",";
  payload += "\"Energia\":" + String(energy_Wh, 2) + ",";
  payload += "\"Frecuencia\":" + String(frequency, 2) + ",";
  payload += "\"Factor Potencia\":" + String(powerFactor, 2);
  payload += "}";

  String resp;
  if (postJson("/readings", payload, resp)) {
    Serial.println("Envio exitoso.");
  } else {
    Serial.println("Fallo envio, reintentando registro.");
    prefs.remove("token");
    tokenAvisado = false;
  }
}
