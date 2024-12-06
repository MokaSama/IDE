// código para el Arduino UNO
const int sensor1Pin = A0;  
const int sensor2Pin = A1;  
const int rele1Pin = 6;     
const int rele2Pin = 7;     

int humedad1;
int humedad2;

//  humedad
const int humedadMinimaSensor1 = 40;  
const int humedadMaximaSensor1 = 55;  
const int humedadMinimaSensor2 = 60;  
const int humedadMaximaSensor2 = 85;  
void setup() {
 

  pinMode(rele1Pin, OUTPUT);
  pinMode(rele2Pin, OUTPUT);
  
  

  Serial.begin(9600);
}

void loop() {
  

  humedad1 = analogRead(sensor1Pin);
  humedad2 = analogRead(sensor2Pin);
  
  // porcentajes
  int humedad1Porcentaje = map(humedad1, 1018, 300, 0, 100);
  int humedad2Porcentaje = map(humedad2, 1018, 300, 0, 100);

 
  Serial.print("Humedad Sensor 1: ");
  Serial.print(humedad1Porcentaje);
  Serial.print("%, ");
  Serial.print("Humedad Sensor 2: ");
  Serial.print(humedad2Porcentaje);
  Serial.println("%");


  if (humedad1Porcentaje < humedadMinimaSensor1) {
    
    digitalWrite(rele1Pin, LOW);
    Serial.println("Bomba 1 encendida.");
  } else if (humedad1Porcentaje > humedadMaximaSensor1) {
    
    digitalWrite(rele1Pin, HIGH);
    Serial.println("Bomba 1 apagada.");
  }

 
  if (humedad2Porcentaje < humedadMinimaSensor2) {
    
    digitalWrite(rele2Pin, LOW);
    Serial.println("Bomba 2 encendida.");
  } else if (humedad2Porcentaje > humedadMaximaSensor2) {
    
    digitalWrite(rele2Pin, HIGH);
    Serial.println("Bomba 2 apagada.");
  }

  delay(5000);
}
