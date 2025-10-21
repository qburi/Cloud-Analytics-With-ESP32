// This is your main .ino file
#include "thingProperties.h" // Manages Arduino IoT Cloud variables

// --- Sensor Libraries ---
#include <Wire.h>
#include "SparkFunBME280.h"

// --- Web API Libraries ---
#include <HTTPClient.h>      // For making web requests
#include <Arduino_JSON.h>    // For parsing the weather data

// --- Global Sensor Objects ---
BME280 bmeSensor; // The BME280 sensor object

// *** IMPORTANT: SET YOUR GAS SENSOR PIN ***
#define GAS_SENSOR_PIN 34 // Change '34' to your actual GPIO pin

// --- Web API Configuration ---
const char* weatherApiUrl = "API";
const long webFetchInterval = 600000; // 10 minutes
long lastWebFetch = 0;
bool initialWebFetchDone = false;

// --- [RE-ADDED] Local Sensor Timer ---
// We will force the local sensors to be read every 2 seconds.
const long sensorReadInterval = 2000; // 2 seconds
long lastSensorRead = 0;
// ------------------------------------

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  delay(2000);

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Serial.println("Starting all sensors...");

  Wire.begin();
  bmeSensor.setI2CAddress(0x76);
  if (bmeSensor.beginI2C() == false) {
    Serial.println("BME280 sensor initialization failed. Check wiring.");
    while (1); // Halt
  }
  Serial.println("BME280 sensor initialized successfully.");

  pinMode(GAS_SENSOR_PIN, INPUT);
  Serial.println("Setup complete. Waiting for cloud connection...");
}

// --- Main Loop ---
void loop() {
  ArduinoCloud.update(); // Keeps the connection alive

  // --- [RE-ADDED] Check if it's time to read local sensors ---
  // This timer runs independently of the cloud settings.
  if (millis() - lastSensorRead > sensorReadInterval) {
    lastSensorRead = millis();
    readLocalSensors(); // Call our function directly
  }

  // --- Check for Web Weather ---
  if (ArduinoCloud.connected()) {
    if (!initialWebFetchDone) {
      Serial.println("Cloud connected. Performing initial web fetch...");
      fetchWebWeather();
      lastWebFetch = millis();
      initialWebFetchDone = true;
    }
    else if (millis() - lastWebFetch > webFetchInterval) {
      lastWebFetch = millis();
      fetchWebWeather();
    }
  }
}

// --- Function to Fetch Web Weather Data ---
// (This function is correct, no changes)
void fetchWebWeather() {
  Serial.println("Fetching web weather data...");
  HTTPClient http;
  http.begin(weatherApiUrl);
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JSONVar myObject = JSON.parse(payload);
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("JSON parsing failed!");
        return;
      }

      double webTempK = myObject["main"]["temp"];
      double webHum = myObject["main"]["humidity"];
      double webPress = myObject["main"]["pressure"];

      float tempC = (float)webTempK - 273.15;
      int hum = (int)webHum;
      float press = (float)webPress;

      webData = "T: " + String(tempC, 2) + " C, " +
                "H: " + String(hum) + " %RH, " +
                "P: " + String(press, 2) + " hPa";

      Serial.println("--- Web Weather Updated ---");
      Serial.print("  webData String: "); Serial.println(webData);

    } else { /* ...error handling... */ }
  } else { /* ...error handling... */ }
  http.end();
}


// --- Function to Read Local Sensors ---
// (This function is correct, no changes)
void readLocalSensors() {
  Serial.println("Reading local sensors...");

  airQuality = analogRead(GAS_SENSOR_PIN);
  localTemp = bmeSensor.readTempC();
  localPressure = bmeSensor.readFloatPressure() / 100.0F;

  Serial.println("  --- BME280 ---");
  Serial.print("  Local Temp: "); Serial.println(localTemp);
  Serial.print("  Local Pressure: "); Serial.println(localPressure);
  Serial.println("  --- Gas Sensor ---");
  Serial.print("  Air Quality (raw): "); Serial.println(airQuality);
  Serial.println("----------------------------------------");
}

// --- Empty Cloud Callbacks ---
void onAirQualityChange() { /* Do nothing */ }
void onWebDataChange()    { /* Do nothing */ }
void onLocalTempChange()  { /* Do nothing */ }
void onLocalPressureChange() { /* Do nothing */ }