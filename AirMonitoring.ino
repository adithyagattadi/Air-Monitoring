#define V_GREEN 1    
#define V_YELLOW 2   
#define V_RED 3     
#define BLYNK_TEMPLATE_ID "TMPL37esYwBB0"
#define BLYNK_TEMPLATE_NAME "Air Monitoring System"
#define BLYNK_AUTH_TOKEN "KOQstusD38dSaNZqZHaLVtzyZmAl029R"

#include <WiFi.h>
#include <DHT.h>
#include <ThingSpeak.h>
#include <BlynkSimpleEsp32.h>

DHT dht(5, DHT11);
#define LED_RED 18
#define LED_YELLOW 19
#define LED_GREEN 21
#define MQ_135 34

WiFiClient client;
long myChannelNumber = 2657089;
const char myWriteAPIKey[] = "8X45EMP76M4YJ4CO";

char ssid[] = "realme narzo 30";
char pass[] = "Adithya@1234";

float m = -0.3376;
float b = 0.7165;
float R0 = 3.12;
int calibrationSamples = 500;

unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 15000;

void setup() {
  Serial.begin(9600);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("ESP32 is connected!");
  Serial.println(WiFi.localIP());

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  dht.begin();
  ThingSpeak.begin(client);

  R0 = calibrateMQ135();
  Serial.print("Calibrated R0: ");
  Serial.println(R0);
}

void loop() {
  Blynk.run();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  float sensor_volt = analogRead(MQ_135) * (3.3 / 4095.0); 
  float RS_gas = ((3.3 * 1.0) / sensor_volt) - 1.0;
  float ratio = RS_gas / R0;
  float ppm_log = (log10(ratio) - b) / m;
  float ppm = pow(10, ppm_log);

  Serial.print("Temperature: ");
  Serial.println(t);
  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("PPM: ");
  Serial.println(ppm);

  if (millis() - lastThingSpeakUpdate >= thingSpeakInterval) {
    ThingSpeak.writeField(myChannelNumber, 1, t, myWriteAPIKey);
    ThingSpeak.writeField(myChannelNumber, 2, h, myWriteAPIKey);
    ThingSpeak.writeField(myChannelNumber, 3, ppm, myWriteAPIKey);
    lastThingSpeakUpdate = millis();
  }

  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, ppm);

  if (ppm <= 1) {  
    Serial.println("Green LED ON");
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    Blynk.virtualWrite(V_GREEN, "Green LED is ON! Air quality is good.");
} 
else if (ppm > 1 && ppm <= 2) {  
    Serial.println("Yellow LED ON");
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, LOW);
    Blynk.virtualWrite(V_YELLOW, "Yellow LED is ON! Air quality is moderate.");
} 
else {  
    Serial.println("Red LED ON");
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    Blynk.virtualWrite(V_RED, "Red LED is ON! Air quality is poor!");
}
  delay(5000);
}

void setLEDState(int green, int yellow, int red) {
  digitalWrite(LED_GREEN, green);
  digitalWrite(LED_YELLOW, yellow);
  digitalWrite(LED_RED, red);
}

float calibrateMQ135() {
  float sensorValue = 0.0;
  for (int x = 0; x < calibrationSamples; x++) {
    sensorValue += analogRead(MQ_135);
    delay(10);
  }
  sensorValue = sensorValue / calibrationSamples;
  float sensor_volt = sensorValue * (3.3 / 4095.0);
  float RS_air = ((3.3 * 1.0) / sensor_volt) - 1.0;
  return RS_air / 3.7;
}
