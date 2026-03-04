#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESP32Servo.h>   // Added for servo motors

// ================== SERVO PART ==================
Servo servo1;
Servo servo2;

// Safe PWM-capable pins (avoid 13,14,25–27,32,33)
const int servo1Pin = 4;  // GPIO4
const int servo2Pin = 5;  // GPIO5

// ================== SENSOR PART ==================

// Wi-Fi credentials (for sending data)
const char* ssid = "USE YOUR WIFI";
const char* password = "*******";

// DHT11 sensors
#define DHTPIN1 13
#define DHTPIN2 14
#define DHTTYPE DHT11

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// Soil moisture sensor
const int sensor_pin = 32;   // GPIO34 (ADC1 channel)
int sensor_analog;
int moisture;

// ================== CAR CONTROL PART ==================
const int in1 = 26;
const int in2 = 25;
const int in3 = 33;
const int in4 = 27;

const char* ssid1 = "ESP32_CAR";
const char* password1 = "12345678";

WebServer server(80);

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // ---- Initialize servos ----
  servo1.attach(servo1Pin, 500, 2400);
  servo2.attach(servo2Pin, 500, 2400);
  Serial.println("Servo initialized");

  // ---- Initialize sensors ----
  dht1.begin();
  dht2.begin();

  pinMode(sensor_pin, INPUT);

  // ---- Initialize motors ----
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // ---- Connect to Wi-Fi for data sending ----
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi (for sensors)");

  // ---- Start Access Point for car control ----
  WiFi.softAP(ssid1, password1);
  Serial.println("WiFi AP Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // ---- Web control routes ----
  server.on("/", handleRoot);
  server.on("/F", forward);
  server.on("/B", backward);
  server.on("/L", left);
  server.on("/R", right);
  server.on("/S", stopCar);

  server.begin();
}

// ================== LOOP ==================
void loop() {
  loopSensors();
  loopServo();
  server.handleClient();
}

// ================== SERVO LOOP ==================
void loopServo() {
  static unsigned long lastMove = 0;
  static bool goingUp = true;
  static int pos = 0;

  // Move every few milliseconds
  if (millis() - lastMove > 5) {  // fast movement
    lastMove = millis();

    if (goingUp) pos++;
    else pos--;

    // Move servos
    servo1.write(pos);
    servo2.write(pos);

    // Check limits
    if (pos >= 180) {
      goingUp = false;
      delay(3000); // 3s pause at top
    } else if (pos <= 0) {
      goingUp = true;
      delay(3000); // 3s pause at bottom
    }
  }
}

// ================== SENSOR LOOP ==================
void loopSensors() {
  static unsigned long lastRead = 0;
  unsigned long now = millis();

  if (now - lastRead >= 5000) {  // read every 5 seconds
    lastRead = now;

    // Read DHT sensors
    float h1 = dht1.readHumidity();
    float t1 = dht1.readTemperature();
    float h2 = dht2.readHumidity();
    float t2 = dht2.readTemperature();

    // Read soil sensor
    sensor_analog = analogRead(sensor_pin);
    moisture = (100 - ((sensor_analog / 4095.0) * 100));

    if (isnan(h1) || isnan(t1) || isnan(h2) || isnan(t2)) {
      Serial.println("Sensor read failed");
      return;
    }

    float maxH = max(h1, h2);
    float maxT = max(t1, t2);

    Serial.printf("Temp: %.1f °C, Hum: %.1f %%, Soil: %d%%\n", maxT, maxH, moisture);

    // Send data to server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "http://10.15.8.102/plant_disease_detection-main/savedata.php?temp=" 
                    + String(maxT) + "&hum=" + String(maxH) + "&soil=" + String(moisture);
      http.begin(url);
      int code = http.GET();
      if (code > 0) Serial.println("Data sent successfully!");
      else Serial.println("HTTP Error: " + String(code));
      http.end();
    }
  }
}

// ================== HTML PAGE ==================
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html><html>
    <head>
      <title>ESP32 Car Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
        body { text-align: center; font-family: sans-serif; }
        button {
          width: 100px; height: 50px;
          font-size: 16px; margin: 10px;
        }
      </style>
      <script>
        function sendCommand(cmd) {
          fetch("/" + cmd);
        }

        function setupButton(id, command) {
          const btn = document.getElementById(id);
          btn.addEventListener('mousedown', () => sendCommand(command));
          btn.addEventListener('mouseup', () => sendCommand('S'));
          btn.addEventListener('touchstart', () => sendCommand(command));
          btn.addEventListener('touchend', () => sendCommand('S'));
        }

        window.onload = () => {
          setupButton("forward", "F");
          setupButton("backward", "B");
          setupButton("left", "L");
          setupButton("right", "R");
        };
      </script>
    </head>
    <body>
      <h2>ESP32 Web Controlled Car</h2>
      <div>
        <button id="forward">Forward</button><br>
        <button id="left">Left</button>
        <button id="right">Right</button><br>
        <button id="backward">Backward</button>
      </div>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// ================== MOVEMENT ==================
void forward() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  server.send(200, "text/plain", "Forward");
}

void backward() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  server.send(200, "text/plain", "Backward");
}

void left() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  server.send(200, "text/plain", "Left");
}

void right() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  server.send(200, "text/plain", "Right");
}

void stopCar() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
  server.send(200, "text/plain", "Stop");
}