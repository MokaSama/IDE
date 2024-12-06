#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "DHT.h"

const char* ssid = "INFINITUM41A4_2.4";
const char* password = "mKMt5Uy2sM";

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Enviromental_green"
#define AIO_KEY         ""

WiFiClient client;  
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);  
Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/R Humidity");
Adafruit_MQTT_Publish soilMoistureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Soil mosture");
Adafruit_MQTT_Publish waterLevelFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Water level");

#define DPIN 2          
#define DTYPE DHT11     
#define SOIL_MOISTURE_PIN A0
#define TRIG_PIN 4
#define ECHO_PIN 5

DHT dht(DPIN, DTYPE);

int getSoilMoisture() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN);
  return constrain(map(rawValue, 1023, 300, 0, 100), 0, 100);
}

float getWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  float duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return 0;
  }
  
  float distance = (duration / 2.0) * 0.0343;
  if (distance < 5 || distance > 40) {
    return 0;
  }
  
  return constrain(map(distance, 5, 40, 100, 0), 0, 100);
}

// Función para reconectar con MQTT
void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando a MQTT... ");
    if (mqtt.connect()) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Error, rc=");
      Serial.println(" Intentando nuevamente en 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();

  // Conexión Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a Wi-Fi...");
  }
  Serial.println("Conectado a Wi-Fi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  
  mqtt.processPackets(10000);

  float temperatureC = dht.readTemperature();
  float humidity = dht.readHumidity();
  int soilMoisture = getSoilMoisture();
  float waterLevel = getWaterLevel();

  if (isnan(temperatureC) || isnan(humidity)) {
    Serial.println("Error leyendo del sensor DHT");
    return;
  }

  if (!temperatureFeed.publish(temperatureC)) {
    Serial.println("Error enviando temperatura");
  }
  if (!humidityFeed.publish(humidity)) {
    Serial.println("Error enviando humedad");
  }
  if (!soilMoistureFeed.publish(soilMoisture)) {
    Serial.println("Error enviando humedad del suelo");
  }
  if (!waterLevelFeed.publish(waterLevel)) {
    Serial.println("Error enviando nivel de agua");
  }

  delay(10000); // Enviar datos cada 10 segundos
}
