#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "SPIFFS.h"
#include <time.h>
#include <HTTPClient.h>

#define CAMERA_MODEL_AI_THINKER

const char* ssid = "USE YOUR WIFI";
const char* password = "********";

// Database server details
const char* serverURL = "http://192.168.0.103/plant_disease_detection-main/saveimage.php";

WebServer server(80);
camera_fb_t* currentPreview = nullptr;
String lastSavedFile = "";

void setupCamera();
void setupWiFi();
void setupServer();
void handleRoot();
void handleStream();
void handlePreview();
void handleSave();
void handleSaveToDatabase();
void handleDelete();
void handleNotFound();
bool saveImageToDatabase(uint8_t* buf, size_t len, const char* filename);

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; background: #f0f0f0; margin: 0; padding: 20px; }
        .container { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); max-width: 800px; margin: 0 auto; }
        .button { background: #4CAF50; color: white; padding: 12px 24px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 10px; }
        .button.capture { background: #f44336; }
        .button.delete { background: #ff9800; }
        .button.database { background: #2196F3; }
        .video-container { margin: 20px 0; background: black; border-radius: 5px; overflow: hidden; }
        img { max-width: 100%; border-radius: 5px; }
        .status { margin: 10px 0; padding: 10px; border-radius: 5px; display: none; }
        .status.success { background: #d4edda; color: #155724; }
        .status.error { background: #f8d7da; color: #721c24; }

        .popup {
            display: none;
            position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(0,0,0,0.7); z-index: 9999; justify-content: center; align-items: center;
        }
        .popup-content {
            background: white; border-radius: 10px; padding: 20px; max-width: 600px;
        }
        .popup img { max-width: 100%; border-radius: 10px; }
        .popup-buttons { margin-top: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-CAM Controller</h1>
        <div class="video-container">
            <img id="stream" src="/stream?t=0" alt="camera stream">
        </div>
        <div>
            <button class="button capture" onclick="capturePhoto()">Take Picture</button>
            <button class="button delete" onclick="deletePhoto()">Delete</button>
        </div>
        <div id="status" class="status"></div>
    </div>

    <div class="popup" id="reviewPopup">
        <div class="popup-content">
            <h3>Preview Photo</h3>
            <img id="previewImg" src="">
            <div class="popup-buttons">
                <button class="button" onclick="savePhoto()">Save to Device</button>
                <button class="button database" onclick="saveToDatabase()">Go Plantify</button>
                <button class="button capture" onclick="retakePhoto()">Retake</button>
            </div>
        </div>
    </div>

    <script>
        let currentPreviewBlob = null;

        function refreshStream() {
            const streamImg = document.getElementById('stream');
            streamImg.src = '/stream?t=' + Date.now();
        }
        setInterval(refreshStream, 800);
        window.addEventListener('load', refreshStream);

        function capturePhoto() {
            showStatus('Capturing photo...', 'success');
            fetch('/preview')
                .then(res => res.blob())
                .then(blob => {
                    currentPreviewBlob = blob;
                    const url = URL.createObjectURL(blob);
                    document.getElementById('previewImg').src = url;
                    document.getElementById('reviewPopup').style.display = 'flex';
                    showStatus('Photo ready for review.', 'success');
                })
                .catch(err => {
                    console.error(err);
                    showStatus('Error capturing photo', 'error');
                });
        }

        function savePhoto() {
            if (!currentPreviewBlob) {
                showStatus('No photo to save', 'error');
                return;
            }
            
            const url = URL.createObjectURL(currentPreviewBlob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'capture_' + new Date().toISOString().replace(/T/, '_').replace(/:/g, '-').slice(0, 19) + '.jpg';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
            
            // Also save to server
            fetch('/save')
                .then(() => {
                    document.getElementById('reviewPopup').style.display = 'none';
                    showStatus('Photo saved successfully!', 'success');
                    currentPreviewBlob = null;
                })
                .catch(() => showStatus('Failed to save photo', 'error'));
        }

        function saveToDatabase() {
            if (!currentPreviewBlob) {
                showStatus('No photo to save', 'error');
                return;
            }
            
            showStatus('Saving to Plantify database...', 'success');
            
            fetch('/saveToDatabase')
                .then(response => response.text())
                .then(result => {
                    document.getElementById('reviewPopup').style.display = 'none';
                    if (result === 'Success') {
                        showStatus('Photo saved to Plantify database successfully!', 'success');
                    } else {
                        showStatus('Failed to save to database: ' + result, 'error');
                    }
                    currentPreviewBlob = null;
                })
                .catch(error => {
                    console.error('Error:', error);
                    showStatus('Error saving to database', 'error');
                });
        }

        function retakePhoto() {
            document.getElementById('reviewPopup').style.display = 'none';
            currentPreviewBlob = null;
            showStatus('Retake another photo.', 'success');
        }

        function deletePhoto() {
            fetch('/delete')
                .then(res => res.text())
                .then(txt => showStatus(txt === 'Deleted' ? 'Last photo deleted!' : txt, 'success'))
                .catch(() => showStatus('Error deleting photo', 'error'));
        }

        function showStatus(message, type) {
            const div = document.getElementById('status');
            div.textContent = message;
            div.className = 'status ' + type;
            div.style.display = 'block';
            setTimeout(() => div.style.display = 'none', 3000);
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  setupCamera();
  setupWiFi();
  setupServer();
  Serial.print("ESP32-CAM Ready at: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  delay(2);
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }
  Serial.println("Camera OK");
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/preview", HTTP_GET, handlePreview);
  server.on("/save", HTTP_GET, handleSave);
  server.on("/saveToDatabase", HTTP_GET, handleSaveToDatabase);
  server.on("/delete", HTTP_GET, handleDelete);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleStream() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  WiFiClient client = server.client();
  String header = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\nConnection: close\r\n\r\n";
  server.sendContent(header);
  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handlePreview() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera failed");
    return;
  }
  
  // Store the frame buffer for later use
  if (currentPreview) {
    esp_camera_fb_return(currentPreview);
  }
  currentPreview = fb;
  
  WiFiClient client = server.client();
  String header = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\nConnection: close\r\n\r\n";
  server.sendContent(header);
  client.write(fb->buf, fb->len);
}

void handleSave() {
  if (!currentPreview) {
    server.send(404, "text/plain", "No preview available");
    return;
  }

  // Get current time for filename
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    server.send(500, "text/plain", "Failed to get time");
    return;
  }
  char filename[64];
  strftime(filename, sizeof(filename), "capture_%Y-%m-%d_%H-%M-%S.jpg", &timeinfo);

  // Save to SPIFFS
  File dest = SPIFFS.open(String("/") + filename, FILE_WRITE);
  if (dest) {
    dest.write(currentPreview->buf, currentPreview->len);
    dest.close();
    lastSavedFile = String(filename);
    Serial.printf("Saved locally: %s\n", filename);
  }

  server.send(200, "text/plain", "Saved");
}

void handleSaveToDatabase() {
  if (!currentPreview) {
    server.send(404, "text/plain", "No preview available");
    return;
  }

  // Get current time for filename
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    server.send(500, "text/plain", "Failed to get time");
    return;
  }
  char filename[64];
  strftime(filename, sizeof(filename), "plant_%Y-%m-%d_%H-%M-%S.jpg", &timeinfo);

  // Save image to database
  if (saveImageToDatabase(currentPreview->buf, currentPreview->len, filename)) {
    server.send(200, "text/plain", "Success");
    Serial.println("Image saved to database successfully");
  } else {
    server.send(500, "text/plain", "Database Save Failed");
    Serial.println("Failed to save image to database");
  }
}

void handleDelete() {
  if (lastSavedFile != "" && SPIFFS.exists(lastSavedFile.c_str())) {
    SPIFFS.remove(lastSavedFile.c_str());
    lastSavedFile = "";
    server.send(200, "text/plain", "Deleted");
  } else {
    server.send(404, "text/plain", "No photo to delete");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

bool saveImageToDatabase(uint8_t* buf, size_t len, const char* filename) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/octet-stream");
  http.addHeader("X-Filename", filename);
  
  Serial.printf("Sending image to database: %s, Size: %d bytes\n", filename, len);
  
  int httpResponseCode = http.POST(buf, len);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    Serial.printf("Response: %s\n", response.c_str());
    http.end();
    
    if (response == "Success") {
      Serial.println("Image saved to database successfully!");
      return true;
    } else {
      Serial.println("Database returned error");
      return false;
    }
  } else {
    Serial.printf("Error in HTTP request: %s\n", http.errorToString(httpResponseCode).c_str());
    http.end();
    return false;
  }
}
