#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

const char *wifiSsid = "pinhole-detector";
const char *wifiPassword = "pinhole123";
const char *serverUrl = "http://192.168.131.85:8000/api/images/receive/";
const char *mqttServer = "192.168.131.85";
const int mqttPort = 1883;
const char *mqttClientId = "pinhole-espcam";
const char *commandTopic = "pinhole/state";
const char *statusTopic = "pinhole/espcam/status";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool capturing = false;
unsigned long lastCaptureAt = 0;
const unsigned long captureIntervalMs = 1000;

void publishStatus(const char *status) {
  if (mqttClient.connected()) {
    bool published = mqttClient.publish(statusTopic, status, true);
    Serial.print("[MQTT] Status publish '");
    Serial.print(status);
    Serial.print("' -> ");
    Serial.println(published ? "OK" : "FAILED");
  } else {
    Serial.print("[MQTT] Cannot publish status, disconnected: ");
    Serial.println(status);
  }
}

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  message.toLowerCase();

  Serial.print("[MQTT] Message on topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (message == "running") {
    capturing = true;
    Serial.println("[CONTROL] Capture started");
    publishStatus("capturing");
  }

  if (message == "stopped") {
    capturing = false;
    Serial.println("[CONTROL] Capture stopped");
    publishStatus("idle");
  }
}

void connectWiFi() {
  Serial.print("[WIFI] Connecting to ");
  Serial.println(wifiSsid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("[WIFI] Connected");
  Serial.print("[WIFI] IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WIFI] RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void connectMqtt() {
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(onMqttMessage);

  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting to ");
    Serial.print(mqttServer);
    Serial.print(":");
    Serial.println(mqttPort);

    if (mqttClient.connect(mqttClientId)) {
      Serial.println("[MQTT] Connected");
      mqttClient.subscribe(commandTopic);
      Serial.print("[MQTT] Subscribed to ");
      Serial.println(commandTopic);
      publishStatus("idle");
    } else {
      Serial.print("[MQTT] Connection failed, state=");
      Serial.println(mqttClient.state());
      delay(1000);
    }
  }
}

void setupCamera() {
  Serial.println("[CAMERA] Initializing camera");
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t cameraStatus = esp_camera_init(&config);
  if (cameraStatus != ESP_OK) {
    Serial.print("[CAMERA] Init failed, error=0x");
    Serial.println(cameraStatus, HEX);
    while (true) {
      delay(1000);
    }
  }

  Serial.println("[CAMERA] Ready");
}

void sendPhoto() {
  Serial.println("[CAPTURE] Taking photo");
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    Serial.println("[CAPTURE] Camera frame failed");
    publishStatus("camera_error");
    return;
  }

  Serial.print("[CAPTURE] Frame size: ");
  Serial.print(frame->len);
  Serial.println(" bytes");
  Serial.print("[HTTP] Uploading to ");
  Serial.println(serverUrl);

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-Device-ID", mqttClientId);

  int responseCode = http.POST(frame->buf, frame->len);

  Serial.print("[HTTP] Response code: ");
  Serial.println(responseCode);

  if (responseCode >= 200 && responseCode < 300) {
    Serial.println("[HTTP] Upload successful");
    publishStatus("image_sent");
  } else {
    Serial.print("[HTTP] Upload failed: ");
    Serial.println(http.errorToString(responseCode));
    publishStatus("upload_failed");
  }

  http.end();
  esp_camera_fb_return(frame);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("[BOOT] ESP32-CAM pinhole detector starting");
  Serial.print("[BOOT] Device ID: ");
  Serial.println(mqttClientId);
  setupCamera();
  connectWiFi();
  connectMqtt();
  Serial.println("[BOOT] Setup complete");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Disconnected, reconnecting");
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Disconnected, reconnecting");
    connectMqtt();
  }

  mqttClient.loop();

  if (capturing && millis() - lastCaptureAt >= captureIntervalMs) {
    lastCaptureAt = millis();
    sendPhoto();
  }
}
