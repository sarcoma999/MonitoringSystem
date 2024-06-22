#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>
#include <DHT.h>
#include "OV7670.h"
#include "BMP.h"
#include <Ticker.h>

#define DHTTYPE DHT11

const int motor1Pin1 = 16; 
const int motor1Pin2 = 17; 
const int enable1Pin = 18;
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

Ticker stepperTicker;
Ticker valveTicker;
bool washingInProgress = false;
bool errorWashing = false;
unsigned long startTime = 0;

const char* ssid = "*****"; // your ssid
const char* password = "*******"; // your password

// Server to handle incoming connections
AsyncWebServer server(80);

// Sensor pins
const int buzzPin = 4;
const int warningLedPin = 12;
const int dhtPin = 5;
const int trigPin = 23;
const int echoPin = 25;

// Camera pins
const int SIOD = 21; // SDA
const int SIOC = 22; // SCL
const int VSYNC = 15;
const int HREF = 2;
const int XCLK = 33;
const int PCLK = 32;
const int D0 = 35;
const int D1 = 34;
const int D2 = 26;
const int D3 = 27;
const int D4 = 14;
const int D5 = 36;
const int D6 = 39;
const int D7 = 13;

Servo valve;

DHT dht11(dhtPin, DHT11);

int valveState = 1;
int tempState = 1;
int waterState = 1;
float temp = 0.0;
float distanceCm = 0.0;

OV7670 *camera;
unsigned char bmpHeader[BMP::headerSize];

String SendHTML() {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<meta charset=\"utf-8\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Monitoring System Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;} #video-container {width: 100%; height: auto;}";
  ptr += "body{font-family: Helvetica; display: flex; flex-direction: column; align-items: center; justify-content: center; margin: 0; background-color: #222; color: #fff;}\n";
  ptr += "#container{display: flex; flex-direction: row; align-items: flex-start; justify-content: space-between; width: 80%; margin-top: 50px;}\n";
  ptr += "#controls{display: flex; flex-direction: column; align-items: center; justify-content: space-between;}\n";
  ptr += "#parameters{margin-right: 20px; text-align: center;}\n";
  ptr += "#liquidBlock{align-items: center;}\n";
  ptr += "#liquid{font-size: 18px; color: #fff;}\n";
  ptr += "#status, #liquidInfo{color: #fff}\n";
  ptr += "#buttons{display: flex; flex-direction: column; align-items: center; justify-content: flex-start;}\n";
  ptr += "#camera-container{margin-top: 50px;}\n";
  ptr += "#video{height: auto; width: 80%; max-width: 640px}\n";
  ptr += "p{font-size: 16px; color: #fff; margin-bottom: 10px;}\n";
  ptr += "select, button{margin: 10px; padding: 10px 20px; background-color: #333; color: #fff; border: none; border-radius: 5px; cursor: pointer;}\n";
  ptr += "button:hover{background-color: #555;}\n";
  ptr += ".status-square{width: 15px; height: 15px; margin-left: 5px; border-radius: 2px; display: inline-block;}\n";
  ptr += ".green{width: 15px; height: 15px; margin-left: 5px; border-radius: 2px; display: inline-block; background-color: green;}\n";
  ptr += ".red{width: 15px; height: 15px; margin-left: 5px; border-radius: 2px; display: inline-block; background-color: red;}\n";
  ptr += "</style>\n";
  ptr += "<script>\n";
  ptr += "var intervalId;\n";
  ptr += "function fetchData() {\n";
  ptr += "  var xhr = new XMLHttpRequest();\n";
  ptr += "  xhr.onreadystatechange = function() {\n";
  ptr += "    if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "      var data = JSON.parse(this.responseText);\n";
  ptr += "      document.getElementById('temp').innerText = data.temperature;\n";
  ptr += "      var tempSquare = document.getElementById('tempSquare');\n";
  ptr += "      tempSquare.className = (data.temperature > 30 ? 'red' : 'green');\n";
  ptr += "      document.getElementById('tempState').innerText = (data.tempState === 0 ? 'critical' : 'normal');\n";
  ptr += "      var tempStateSquare = document.getElementById('tempStateSquare');\n";
  ptr += "      tempStateSquare.className = (data.tempState === 0 ? 'red' : 'green');\n";
  ptr += "      document.getElementById('waterLevel').innerText = data.distance;\n";
  ptr += "      var waterLevelSquare = document.getElementById('waterLevelSquare');\n";
  ptr += "      waterLevelSquare.className = (data.distance < 5 ? 'red' : 'green');\n";
  ptr += "      document.getElementById('waterState').innerText = (data.waterState === 0 ? 'critical' : 'normal');\n";
  ptr += "      var waterStateSquare = document.getElementById('waterStateSquare');\n";
  ptr += "      waterStateSquare.className = (data.waterState === 0 ? 'red' : 'green');\n";
  ptr += "      document.getElementById('valveState').innerText = (data.valveState === 0 ? 'opened' : 'closed');\n";
  ptr += "    }\n";
  ptr += "  };\n";
  ptr += "  xhr.open('GET', '/sensor_data', true);\n";
  ptr += "  xhr.send();\n";
  ptr += "}\n";
  ptr += "function updateVideo() {\n";
  ptr += "  document.getElementById('video').src = '/camera?rand=' + Math.random();\n";
  ptr += "}\n";
  ptr += "function openValve() {\n";
  ptr += "  var xhr = new XMLHttpRequest();\n";
  ptr += "  xhr.open('POST', '/open_valve', true);\n";
  ptr += "  xhr.onreadystatechange = function() {\n";
  ptr += "    if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "      document.getElementById('status').innerText = this.responseText;\n";
  ptr += "    }\n";
  ptr += "  };\n";
  ptr += "  xhr.send();\n";
  ptr += "}\n";
  ptr += "function closeValve() {\n";
  ptr += "  var xhr = new XMLHttpRequest();\n";
  ptr += "  xhr.open('POST', '/close_valve', true);\n";
  ptr += "  xhr.onreadystatechange = function() {\n";
  ptr += "    if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "      document.getElementById('status').innerText = this.responseText;\n";
  ptr += "    }\n";
  ptr += "  };\n";
  ptr += "  xhr.send();\n";
  ptr += "}\n";
  ptr += "function startWashing() {\n";
  ptr += "  var xhr = new XMLHttpRequest();\n";
  ptr += "  xhr.open('POST', '/start_washing', true);\n";
  ptr += "  xhr.onreadystatechange = function() {\n";
  ptr += "    if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "      document.getElementById('status').innerText = this.responseText;\n";
  ptr += "    }\n";
  ptr += "  };\n";
  ptr += "  xhr.send();\n";
  ptr += "}\n";
  ptr += "function updateLiquidInfo() {\n";
  ptr += "  var liquid = document.getElementById('liquidSelect').value;\n";
  ptr += "  var info = '';\n";
  ptr += "  if (liquid === 'Liquid1') { info = 'Information about liquid: VIGON US\\n' +\n";
  ptr += "                                     'Density at 20°C - 0.99 g/cm3\\n' +\n";
  ptr += "                                     'Boiling point - 165 - 212°C\\n' +\n";
  ptr += "                                     'pH - 11.3\\n' +\n";
  ptr += "                                     'Purification temperature - 40 - 60°C\\n' +\n";
  ptr += "                                     'Solubility in water - soluble\\n' +\n";
  ptr += "                                     'Solution concentration - 15 - 30%\\n';\n";
  ptr += "                                      }\n";
  ptr += "  if (liquid === 'Liquid2') { info = 'Information about liquid: ZESTRON FA+\\n' +\n";
  ptr += "                                     'Density at 20°C - 0.99 g/cm3\\n' +\n";
  ptr += "                                     'Boiling point - 162 - 190°C\\n' +\n";
  ptr += "                                     'pH - 10.4\\n' +\n";
  ptr += "                                     'Purification temperature - 40 - 55°C\\n' +\n";
  ptr += "                                     'Solubility in water - soluble\\n' +\n";
  ptr += "                                     'Solution concentration - ready solution';\n";
  ptr += "                                      }\n";;
  ptr += "  document.getElementById('liquidInfo').innerText = info;\n";
  ptr += "}\n";
  ptr += "setInterval(fetchData, 2000);\n";
  ptr += "setInterval(updateVideo, 500);\n";
  ptr += "</script>\n";
  ptr += "</head>\n";
  ptr += "<body onload=\"fetchData(); updateVideo();\">\n";
  ptr += "<h1>Monitoring Control System</h1>\n";
  ptr += "<h3>WiFi mode (localIP)</h3>\n";
  ptr += "<div id='container'>\n";
  ptr += "  <div id='controls'>\n";
  ptr += "    <div id='parameters'>\n";
  ptr += "      <p>Temperature: <span id='temp'>25</span><span id='tempSquare' class='status-square'></span></p>\n";
  ptr += "      <p>Temperature state: <span id='tempState'></span><span id='tempStateSquare' class='status-square'></span></p>\n";
  ptr += "      <p>Water level: <span id='waterLevel'></span><span id='waterLevelSquare' class='status-square'></span></p>\n";
  ptr += "      <p>Water level state: <span id='waterState'></span><span id='waterStateSquare' class='status-square'></span></p>\n";
  ptr += "      <p>Valve state: <span id='valveState'></span></p>\n";
  ptr += "    </div>\n";
  ptr += "    <div id='liquidBlock'>\n";
  ptr += "      <p id='liquid'> Select washing liquid </p>\n";
  ptr += "      <select id='liquidSelect' onchange='updateLiquidInfo()'>\n";
  ptr += "        <option value='Liquid1' class='liquidOption'>VIGON US</option>\n";
  ptr += "        <option value='Liquid2' class='liquidOption'>ZESTRON FA+</option>\n";
  ptr += "      </select>\n";
  ptr += "      <p id='liquidInfo'></p>\n";
  ptr += "    </div>\n";
  ptr += "    <div id='buttons'>\n";
  ptr += "      <button id='openValveButton' onclick='openValve()'>Open valve</button>\n";
  ptr += "      <button id='closeValveButton' onclick='closeValve()'>Close valve</button>\n";
  ptr += "      <button onclick='startWashing()'>Prepare for washing</button>\n";
  ptr += "      <p id='status'></p>\n";
  ptr += "    </div>\n";
  ptr += "  </div\n";
  ptr += "  <div id=\"camera-container\">\n";
  ptr += "    <img id='video' src='/camera' width='320' height='240'>\n";
  ptr += "  </div>\n";
  ptr += "</div>\n";
  ptr += " </body>\n";
  ptr += "</html>\n";
  return ptr;
}

void handleRoot(AsyncWebServerRequest *request){
  request->send(200, "text/html", SendHTML());
}

void handleSensorData(AsyncWebServerRequest *request) {
  tempSensor();
  soundSensor();

  String json = "{";
  json += "\"temperature\":" + String(temp) + ",";
  json += "\"tempState\":" + String(tempState) + ",";
  json += "\"distance\":" + String(distanceCm) + ",";
  json += "\"waterState\":" + String(waterState) + ",";
  json += "\"valveState\":" + String(valveState);
  json += "}";

  request->send(200, "application/json", json);
}

void handleCameraStream(AsyncWebServerRequest *request){
  camera->oneFrame();
  AsyncWebServerResponse *response = request->beginChunkedResponse("image/bmp", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    static size_t headerSize = BMP::headerSize;
    static size_t dataIndex = 0;
    
    if (index < headerSize) {
      size_t len = min(maxLen, headerSize - index);
      memcpy(buffer, bmpHeader + index, len);
      return len;
    }
    
    size_t remainingData = camera->xres * camera->yres * 2 - dataIndex;
    if (remainingData > 0) {
      size_t len = min(maxLen, remainingData);
      memcpy(buffer, camera->frame + dataIndex, len);
      dataIndex += len;
      return len;
    } else {
      dataIndex = 0;
      return 0;
    }
  });
  request->send(response);
}

void handleOpenValve(AsyncWebServerRequest *request) {
  if (distanceCm <= 5) {
    request->send(200, "text/plain", "Error: Water lvl too high!!!");
    return;
  }
//  valve.write(180); // Open valve
  valveState = 0;
  request->send(200, "text/plain", "Valve`s open");
}

void handleCloseValve(AsyncWebServerRequest *request) {
//  valve.write(0); // Close valve
  valveState = 1;
  request->send(200, "text/plain", "Valve`s closed");
}

void handleStartWashing(AsyncWebServerRequest *request) {
  if (temp < 15 || distanceCm > 10) {
    request->send(200, "text/plain", "Warning: temp < 20°C, water lvl > 10 cm");
    return;
  }
  
  if (washingInProgress) {
    request->send(200, "text/plain", "Warning: Preparing`s already started!");
    return;
  }
  
  washingInProgress = true;
  startTime = millis();
  stepperRotate();
  stepperTicker.once(10, stepperStop);

  request->send(200, "text/plain", "Preparing`s started...");
}

void tempSensor() {
  temp = dht11.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  if (temp >= 30.0) {
    tempState = 0;
    digitalWrite(warningLedPin, HIGH);
    digitalWrite(buzzPin, HIGH);
  } else {
    tempState = 1;
    digitalWrite(warningLedPin, LOW);
    digitalWrite(buzzPin, LOW);
  }
  Serial.print("Temperature: ");
  Serial.println(temp);
}

void soundSensor() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;

  Serial.print("Water Level: ");
  Serial.println(distanceCm);

  if (distanceCm <= 5) {
    waterState = 0;
    digitalWrite(warningLedPin, HIGH);
    digitalWrite(buzzPin, HIGH);
    
  } else {
    waterState = 1;
    digitalWrite(warningLedPin, LOW);
    digitalWrite(buzzPin, LOW);
  }
}

void stepperRotate(){
  Serial.println("Moving Forward");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
}

void stepperStop(){
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  Serial.println("Motor stopped");
  washingInProgress = false;
}

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(warningLedPin, OUTPUT);
  valve.attach(19);

  dht11.begin();

  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);
  
  camera = new OV7670(OV7670::Mode::QQQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/sensor_data", HTTP_GET, handleSensorData);
  server.on("/camera", HTTP_GET, handleCameraStream);
  server.on("/open_valve", HTTP_POST, handleOpenValve);
  server.on("/close_valve", HTTP_POST, handleCloseValve);
  server.on("/start_washing", HTTP_POST, handleStartWashing);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // No need to handle client in loop for AsyncWebServer
}
