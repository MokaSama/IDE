#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <DS18B20.h>
#include <DallasTemperature.h>

const char* ssid = "INFINITUM41A4_2.4"; 
const char* password = "mKMt5Uy2sM";     

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "" 
#define AIO_KEY         "" 

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/T_compost");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/H_compost");
Adafruit_MQTT_Publish compostPhaseFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Alertas");

#define ONE_WIRE_BUS 4  
#define HUMIDITY_PIN A0 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

ESP8266WebServer server(80);
String currentPhase = "";

String determinePhase(float temperature) {
  if (temperature >= 20 && temperature < 40) {
    return "Fase inicial (Mesófila)";
  } else if (temperature >= 40 && temperature <= 70) {
    return "Fase Termófila";
  } else if (temperature < 20) {
    return "Fase de enfriamiento";
  } else {
    return "Temperatura fuera de rango";
  }
}

void connectWiFi() {
  Serial.print("Conectando a Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("¡Conectado a Wi-Fi!");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando a MQTT...");
    if (mqtt.connect()) {
      Serial.println("¡Conectado a MQTT!");
    } else {
      Serial.print("Error de conexión: ");
      Serial.println("Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><body>";
  html += "<h1>Monitoreo de Compostaje</h1>";
  html += "<p>Conexión establecida. Datos de sensores enviados a Adafruit IO.</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void sendDataToAdafruit(float temperature, float humidity, String phase) {
  if (!temperatureFeed.publish(temperature)) {
    Serial.println("Error al publicar temperatura.");
  }
  if (!humidityFeed.publish(humidity)) {
    Serial.println("Error al publicar humedad.");
  }
  if (!compostPhaseFeed.publish(phase.c_str())) {
    Serial.println("Error al publicar fase.");
  }
}

void setup() {
  Serial.begin(115200);
  sensors.begin();

  connectWiFi();
  
  if (!LittleFS.begin()) {
    Serial.println("Error al montar el sistema de archivos.");
    return;
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient();

  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.processPackets(10000);

  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  int analogValue = analogRead(HUMIDITY_PIN);
  float humidity = map(analogValue, 1023, 0, 0, 100); 

  if (temperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Error al leer datos del DS18B20.");
    return;
  }

  if (analogValue < 0 || analogValue > 1023) {
    Serial.println("Error al leer datos del YL-69.");
    return;
  }

  String newPhase = determinePhase(temperature);
  if (newPhase != currentPhase) {
    currentPhase = newPhase;
    Serial.println("Nueva fase detectada: " + currentPhase);
  }

  sendDataToAdafruit(temperature, humidity, currentPhase);

  delay(2000);
}
