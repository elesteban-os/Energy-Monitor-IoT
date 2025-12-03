#include <WiFi.h>
#include <HTTPClient.h>

//Configuracion para el wifi
// aqui hay que poner el nombre de la red donde este y la contraseña para que se conecte al Wifi
const char* ssid = "Tec93";
const char* password = "12345679J";

//la url del api, ahorita es una de prueba
String api_url ="https://webhook.site/d2fbeddc-c1a2-4e19-a94c-5994df3f668e";

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

//simulador del sensor de corriente
float leerCorriente(){
  return random(10,200) / 10.0; //entre 1.0 y 20.0 A
}

//simulador del sensor de voltaje
float leerVoltaje(){
  return random(1000,1200) / 10.0; //entre 100 y 120 V
}

//obtención de la potencia P=V*I
float calcularPotencia(float voltaje, float corriente){
  return (voltaje * corriente);
}

//POST EN LA API
void enviarLectura(float corriente,float voltaje, float potencia){
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi caído, reconectando...");
    conectarWiFi();
  }

  HTTPClient http;
  http.begin(api_url);
  http.addHeader("Content-Type","application/json");

  String json = "{";
  json += "\"serial\":\"" + serialDispositivo + "\",";
  json += "\"corriente\":" + String(corriente,2) + ",";
  json += "\"voltaje\":" + String(voltaje,2) + ",";
  json += "\"potencia\":" + String(potencia,2);
  json += "}";

  int code = http.POST(json);

  Serial.print("POST código; ");
  Serial.println(code);
  Serial.println("Respuesta: " + http.getString());

  http.end();

}

// setup
void setup() {
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  serialDispositivo = generarSerial();
  Serial.println("Serial: " + serialDispositivo);

  conectarWiFi();

}

//loop
unsigned long previo = 0;
const unsigned long intervalo = 5000; // son 5 segundos

void loop() {
  
  if (millis() - previo >= intervalo){
    previo = millis();

    float corriente = leerCorriente();
    float voltaje = leerVoltaje();
    float potencia = calcularPotencia(voltaje,corriente);

    //esto lo muestra en el serial monitor
    Serial.print("Corriente: ");
    Serial.print(corriente);
    Serial.print(" A, Voltaje: ");
    Serial.print(voltaje);
    Serial.print(" V, Potencia: ");
    Serial.print(potencia);
    Serial.print(" W");

    enviarLectura(corriente,voltaje,potencia);

  }

}
