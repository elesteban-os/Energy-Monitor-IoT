#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <PZEM004Tv30.h>

// Serial1 en ESP32 (UART1). RX = 20, TX = 21
PZEM004Tv30 pzem(&Serial1, 20, 21);

// Config WiFi y API
const char* WIFI_SSID = "nombre exacto del wifi visto desde el cel| importante desactivar el firewall de windows";
const char* WIFI_PASS = "contraseña del wifi";
const char* API_BASE  = "http://#ipv4:8000"; 
const char* SERIAL_ID = "ESP32_A";
const char* SENSOR_NAME = "PZEM004T";

Preferences prefs;
String token = "";
unsigned long lastSend = 0;
bool tokenAvisado = false;

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
  prefs.begin("energy", false);
  connectWifi();

  Serial1.begin(9600, SERIAL_8N1, 20, 21);
  Serial.println("Leyendo PZEM004T v3...");
  pzem.resetEnergy();
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

  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();
  float freq    = pzem.frequency();
  float pf      = pzem.pf();

  // Si no hay sensor, generamos valores simulados razonables
  if (isnan(voltage)) voltage = random(210, 230);
  if (isnan(current)) current = random(0, 200) / 10.0;
  if (isnan(power))   power   = voltage * current;
  if (isnan(energy))  energy  = random(0, 500) / 10.0;
  if (isnan(freq))    freq    = 60.0;
  if (isnan(pf))      pf      = 0.95;

  // Limitamos la frecuencia de envio
  if (millis() - lastSend < 5000) {
    delay(100);
    return;
  }
  lastSend = millis();

  String payload = "{";
  payload += "\"token\":\"" + token + "\",";
  payload += "\"sensor_name\":\"" + String(SENSOR_NAME) + "\",";
  payload += "\"Voltaje\":" + String(voltage, 2) + ",";
  payload += "\"Corriente\":" + String(current, 3) + ",";
  payload += "\"Potencia\":" + String(power, 2) + ",";
  payload += "\"Energia\":" + String(energy, 2) + ",";
  payload += "\"Frecuencia\":" + String(freq, 2) + ",";
  payload += "\"Factor Potencia\":" + String(pf, 2);
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
