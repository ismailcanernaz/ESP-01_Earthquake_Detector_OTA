/*
 * ESP-01 Earthquake Early Warning System 2 - with OTA
 * ismailcanernaz@aol.com
 * https://www.youtube.com/shorts/ICfl4UFt0Gg
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Replace with your network credentials
const char* ssid = "MikroTik-XXXXXX"; // SSID
const char* password = "xxxxxxxx";    // PASSWORD

// Create an instance of the server
ESP8266WebServer server(80);

// Create an instance of the ADXL345 sensor
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Define LED pin
const int ledPin = 3;
bool ledState = LOW;

// Define vibration threshold
float earthquakeThreshold = 11.39; // Adjust this value to set the vibration threshold

// Define variables for counting vibrations and displaying the count on the web page
// Initialize the count of earthquakes detected
int earthquakeCount = 0;
String countString = "";
String thresholdString = "";

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);

  // Define i2c pins
  Wire.pins(0, 2);  // SDA=0, SCL=2

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  // Prepare OTA handler
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize the ADXL345 sensor
  if(!accel.begin()) {
    Serial.println("Failed to initialize ADXL345 sensor!");
    while(1);
  }

  // Set the range of the sensor to +/- 4g
  //accel.setRange(ADXL345_RANGE_4_G);
  // Set the measurement range to +/- 16g
  accel.setRange(ADXL345_RANGE_16_G);

  // Set up the LED pin as an output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  // Update the count string for the web page
  countString = String(earthquakeCount);
  // Update the earthquake threshold for the web page
  thresholdString = String(earthquakeThreshold);
  
  // Set up the web server routes
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.on("/reset", handleReset);
  
  // Start the server
  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  // OTA handlers
  ArduinoOTA.handle();
  // Handle incoming client requests
  server.handleClient();

  // Read the acceleration data from the sensor
  sensors_event_t event;
  accel.getEvent(&event);

  // Calculate the magnitude of the acceleration vector
  //float accelerationMagnitude = sqrt(pow(event.acceleration.x, 2) + pow(event.acceleration.y, 2) + pow(event.acceleration.z, 2));
  float accelerationMagnitude = abs(event.acceleration.x) + abs(event.acceleration.y) + abs(event.acceleration.z);

  // Check if earthquake threshold is exceeded
  if (accelerationMagnitude >= earthquakeThreshold) {
    // Increment the earthquake count
    earthquakeCount++;

    // Blink the LED once
    //digitalWrite(ledPin, HIGH);
    //delay(100);
    //digitalWrite(ledPin, LOW);

    // Toggle LED indicator
    ledState = !ledState;
    digitalWrite(ledPin, ledState);

    // Wait for 0.1 second before turning off the LED
    delay(100);

    // Toggle LED indicator
    ledState = !ledState;
    digitalWrite(ledPin, ledState);

    Serial.println("Earthquake detected!");

    // Update the count string for the web page
    countString = String(earthquakeCount);
  }
}

// Handle the root web page
void handleRoot() {
  // Set the content type
  server.sendHeader("Content-Type", "text/html");

  // Create the HTML response
  String html = "<html><head><title>Earthquake Detector</title></head>";
  html += "<body><h2>Earthquake Early Warning System</h2>";
  html += "<h3>Seismic Wave Count: " + countString + "</h3>";
  html += "<h3>Current Threshold: " + thresholdString + "</h3>";
  html += "<form method='post' action='/update'>";
  html += "<label>New Threshold:</label>";
  html += "<input type='number' step='0.01' name='threshold'>";
  html += "<input type='submit' value='Update'>";
  html += "</form>";
  html += "<p><a href='/reset'>Reset Count</a></p>";
  html += "<h3>ESP-01 & ADXL345</h3>";
  html += "</body></html>";

  // Send the response to the client
  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.method() == HTTP_POST) {
    String newThreshold = server.arg("threshold");
    earthquakeThreshold = newThreshold.toFloat();
  }
  
  // Update the earthquake threshold for the web page
  thresholdString = String(earthquakeThreshold);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleReset() {
  // Reset the earthquake count
  earthquakeCount = 0;

  // Update the count string for the web page
  countString = String(earthquakeCount);

  // Redirect to the home page
  server.sendHeader("Location", "/");
  server.send(302);
}
