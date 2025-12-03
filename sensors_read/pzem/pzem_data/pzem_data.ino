#include <PZEM004Tv30.h>

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
  json += "\"sensor_name\": \"PZEM004T \",";
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

// Serial1 en ESP32-C3 (UART1)
// RX = 20, TX = 21
PZEM004Tv30 pzem(&Serial1, 20, 21);

void setup() {
  Serial.begin(115200);

  delay(500);

  Serial1.begin(9600, SERIAL_8N1, 20, 21);
  Serial.println("Leyendo PZEM004T v3...");
  pzem.resetEnergy();

  // Wifi
  WiFi.mode(WIFI_STA);
  serialDispositivo = generarSerial();
  Serial.println("Serial: " + serialDispositivo);

  conectarWiFi();
}

//loop
unsigned long previo = 0;
const unsigned long intervalo = 5000; // son 5 segundos
void loop() {
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();
  float freq    = pzem.frequency();
  float pf      = pzem.pf();

  Serial.println("--------- PZEM004T ---------");

  if(!isnan(voltage))   Serial.println(String("Voltaje: ") + voltage + " V");
  else {
    voltage = 0.0;
    Serial.println("Voltaje: error");
  }                

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

  // ===============================
  // ENVÍO A API CADA 5 SEG
  // ===============================
  if (millis() - previo >= intervalo) {
    previo = millis();
    enviarLectura(voltage, current, power, energy, freq, pf);
  }

  delay(200);
}