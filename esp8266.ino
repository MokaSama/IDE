#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "DHT.h"

const char* ssid = ".4";
const char* password = "";

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME  ""
#define AIO_KEY       ""

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

#define RELAY_CHANNEL_1 12  
#define RELAY_CHANNEL_2 13 

DHT dht(DPIN, DTYPE);
ESP8266WebServer server(80);

bool relayState1 = false;
bool relayState2 = false;

void saveData(float temperatureC, float humidity, float soilMoisture, float waterLevel) {
  File file = LittleFS.open("/data.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.printf("%s,%.2f,%.2f,%.2f,%.2f\n", String(millis()).c_str(), temperatureC, humidity, soilMoisture, waterLevel);
  file.close();
}

void handleRoot() {
  
  String html = "<!DOCTYPE html><html lang=\"en\"><head>";
  html += "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Invernadero</title>";
  
   html +="<h2>________________________</h2>";
   html +="<h1>Invernadero</h1>";
   html +="<h2>________________________</h2>";
   
  html += "<style>body { background-color: white; color: black; font-family: Arial, sans-serif; text-align: center; }";
  
  html += "#leftPanel { width: 10%; float: left; border: 2px solid green; padding: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.5); margin-left: 30 px;}";
    html += "#centerPanel { width: 60%; float: left; margin-top: 0px;}";
  html += "#rightPanel { width: 20%; float: right; padding: 20px; margin-right: 25px;}";
  
  html += "canvas { display: inline-block; margin: 20px auto; }";
  html += "button { margin-top: 20px; padding: 10px; background-color: gray; color: black; border: green; width: 80%; cursor: pointer; transition: box-shadow 0.5s; }";
  html += "button:hover { box-shadow: 0 0 15px rgba(76, 175, 80, 0.7); }";
  html += "</style></head><body>";
  
  html += "<div id='leftPanel'><h3>Panel de Control</h3>";
  html += "<h4>_____________</h4>";
    html += "<h4>Módulo relé</h4>";


  html += "<button onclick=\"toggleRelay(1)\">1</button>";
  html += "<button onclick=\"toggleRelay(2)\">2</button>";
    html += "<h4>_____________</h4>";

  html += "<button onclick=\"downloadCharts()\">Descargar Gráficas</button>";
    html += "<h4>_____________</h4>";

  html += "<button onclick=\"goToAdafruit()\">ADAFRUIT IO</button></div>";

  html += "<div id='centerPanel'><h2></h2>";
  html += "<canvas id=\"tempChart\" width=\"700\" height=\"250\"></canvas>";
  html += "<canvas id=\"humChart\" width=\"700\" height=\"250\"></canvas></div>";

  html += "<div id='rightPanel'><h3>           </h3>";
  html += "<div><h4>Tanque de agua</h4>";
  html += "<div id='waterTank' style='width: 100px; height: 160px; border: 2px solid #000; background-color: #ccc;'>";
  html += "<div id='waterFill' style='width: 100%; height: 0%; background-color: #00f;'></div></div>";
  html += "<p>Nivel de Agua: <span id='waterPercentage'>0</span>%</p></div>";

  html += "<div><h4>Humedad de Suelo</h4>";
  html += "<div id='coffeeCup' style='width: 100px; height: 160px; border: 2px solid #000; background-color: #6f4e37;'>";
  html += "<div id='coffeeFill' style='width: 100%; height: 0%; background-color: #00f;'></div></div>";
  html += "<p>Humedad: <span id='soilMoisturePercentage'>0</span>%</p></div>";

  html += "</div>";  

  html += "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>";
  html += "<script>";
  
  html += "function toggleRelay(channel) {";
  html += "  fetch(`/toggleRelay?channel=` + channel)";
  html += "}";

  html += "function downloadCharts() {";
html += "  var canvas = document.getElementById('miCanvas');"; // Obtiene el canvas por su ID
html += "  var enlace = document.createElement('a');"; // Crea un enlace temporal para descargar la imagen
html += "  enlace.href = canvas.toDataURL('image/png');"; // Convierte el canvas a una URL de imagen PNG
html += "  enlace.download = 'grafica.png';"; // Establece el nombre del archivo a descargar
html += "  enlace.click();"; // Simula un clic en el enlace para iniciar la descarga
html += "}";

  html += "function goToAdafruit() {";
  html += "  window.open('https://io.adafruit.com/', '_blank');";
  html += "}";

  html += "const tempData = { labels: [], datasets: [{ label: 'Temperature (°C)', borderColor: 'rgb(255, 99, 132)', data: [] }] };";
  html += "const humData = { labels: [], datasets: [{ label: 'Humidity (%)', borderColor: 'rgb(54, 162, 235)', data: [] }] };";
  html += "const tempConfig = { type: 'line', data: tempData, options: { scales: { x: { display: true, title: { display: true, text: 'Time' } } } } };";
  html += "const humConfig = { type: 'line', data: humData, options: { scales: { x: { display: true, title: { display: true, text: 'Time' } } } } };";
  html += "const tempChart = new Chart(document.getElementById('tempChart'), tempConfig);";
  html += "const humChart = new Chart(document.getElementById('humChart'), humConfig);";

  html += "setInterval(async () => {";
  html += "const response = await fetch('/data');";
  html += "const data = await response.json();";
  html += "const time = new Date().toLocaleTimeString();";
  html += "if (tempData.labels.length > 20) { tempData.labels.shift(); tempData.datasets[0].data.shift(); }";
  html += "if (humData.labels.length > 20) { humData.labels.shift(); humData.datasets[0].data.shift(); }";
  html += "tempData.labels.push(time); tempData.datasets[0].data.push(data.temperatureC); tempChart.update();";
  html += "humData.labels.push(time); humData.datasets[0].data.push(data.humidity); humChart.update();";
  
  html += "document.getElementById('waterFill').style.height = data.waterLevel + '%';";
  html += "document.getElementById('waterPercentage').innerText = data.waterLevel;";
  
  html += "document.getElementById('coffeeFill').style.height = data.soilMoisture + '%';";
  html += "document.getElementById('soilMoisturePercentage').innerText = data.soilMoisture;";
  html += "}, 2000);";
  
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleData() {
  float temperatureC = dht.readTemperature();
  float humidity = dht.readHumidity();

 int rawSoilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisture = map(rawSoilMoisture, 1018, 300, 0, 100);
  soilMoisture = constrain(soilMoisture, 0, 100);


  
  float waterLevel = getWaterLevel();
  saveData(temperatureC, humidity, soilMoisture, waterLevel);

  String jsonData = "{\"temperatureC\":" + String(temperatureC) + ",\"humidity\":" + String(humidity) + ",\"soilMoisture\":" + String(soilMoisture) + ",\"waterLevel\":" + String(waterLevel) + "}";
  server.send(200, "application/json", jsonData);
}

void handleRelayToggle() {
  String channel = server.arg("channel");

  if (channel == "1") {
    relayState1 = !relayState1;
    digitalWrite(RELAY_CHANNEL_1, relayState1 ? LOW : HIGH);
  } else if (channel == "2") {
    relayState2 = !relayState2;
    digitalWrite(RELAY_CHANNEL_2, relayState2 ? LOW : HIGH);
  }

  server.send(200, "text/plain", "OK");
}

float getWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  float duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration / 2) * 0.0343;
  return map(distance, 5, 40, 100, 0);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_CHANNEL_1, OUTPUT);
  pinMode(RELAY_CHANNEL_2, OUTPUT);
  digitalWrite(RELAY_CHANNEL_1, HIGH);
  digitalWrite(RELAY_CHANNEL_2, HIGH);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  dht.begin();
  
  if (!LittleFS.begin()) {
    Serial.println("An error occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
     Serial.println("Conectado a la red WiFi");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
  }
  Serial.println("Connected!");

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/toggleRelay", handleRelayToggle);
  
  server.begin();
}


void loop() {
  server.handleClient();
  mqtt.processPackets(10000);
}
