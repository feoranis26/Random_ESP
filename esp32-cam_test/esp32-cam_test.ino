#include <LittleFS.h>
#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "esp_spiram.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <AsyncTcp.h>
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <FS.h>

// Replace with your network credentials
const char* ssid = "ARMATRON_NETWORK";
const char* password = "16777216";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Last Photo</h2>
    <p>It might take more than 5 seconds to capture a photo.</p>
    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto()">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
    </p>
  </div>
  <div><img src="saved-photo" id="photo" width="70%"></div>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }
</script>
</html>)rawliteral";

void setup() {
    // Serial port for debugging purposes
    Serial.begin(115200);

    esp_spiram_init();
    //Serial.println("spiram size: " + String(esp_spiram_get_size()));

    // OV2640 camera module
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 5000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_HVGA;
    config.jpeg_quality = 32;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        //ESP.restart();
    }

    // Connect to Wi-Fi
    //WiFi.begin(ssid, password);
    //WiFi.config(IPAddress(10, 134, 102, 227), IPAddress(10, 134, 103, 254), IPAddress(255, 255, 0, 0), IPAddress(10, 134, 103, 254), IPAddress(8, 8, 8, 8));
    WiFi.begin("ARMATRON_NETWORK", "16777216");

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        ESP.restart();
    }
    else {
        delay(500);
        Serial.println("SPIFFS mounted successfully");
    }

    // Print ESP32 Local IP Address
    Serial.print("IP Address: http://");
    Serial.println(WiFi.localIP());


    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", index_html);
        });

    server.on("/capture", HTTP_GET, [](AsyncWebServerRequest* request) {
        takeNewPhoto = true;
        request->send_P(200, "text/plain", "Taking Photo");
        });

    server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, FILE_PHOTO, "image/jpg", false);
        });

    // Start server
    server.begin();

    ledcSetup(1, 1000, 8);
    ledcAttachPin(4, 1);
    ledcWrite(1, 0);
}

void loop() {
    if (takeNewPhoto) {
        capturePhotoSaveSpiffs();
        takeNewPhoto = false;
    }
    delay(1);
}

// Check if photo capture was successful
bool checkPhoto() {
    File f_pic = LittleFS.open(FILE_PHOTO);
    unsigned int pic_sz = f_pic.size();
    return (pic_sz > 100);
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs(void) {
    camera_fb_t* fb = NULL; // pointer
    bool ok = 0; // Boolean indicating if the picture has been taken correctly

    // Take a photo with the camera
    Serial.println("Taking a photo...");
    ledcWrite(1, 128);
    delay(10);
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
    fb = esp_camera_fb_get();
    ledcWrite(1, 0);
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    Serial.println("Frame size: " + String(fb->width) + "x" + String(fb->height) + ", format: " + String(fb->format) + ", size: " + String(fb->len));

    do {
        // Photo file name
        Serial.printf("Picture file name: %s\n", FILE_PHOTO);
        File file = LittleFS.open(FILE_PHOTO, FILE_WRITE);

        // Insert the data in the photo file
        if (!file) {
            Serial.println("Failed to open file in writing mode");
        }
        else {
            file.write(fb->buf, fb->len); // payload (image), payload length
            Serial.print("The picture has been saved in ");
            Serial.print(FILE_PHOTO);
            Serial.print(" - Size: ");
            Serial.print(file.size());
            Serial.println(" bytes");

        }
        // Close the file
        file.close();
        esp_camera_fb_return(fb);

        ledcWrite(1, 128);
        delay(100);
        ledcWrite(1, 0);

        // check if file has been correctly saved in SPIFFS
        ok = checkPhoto();
    } while (!ok);
}