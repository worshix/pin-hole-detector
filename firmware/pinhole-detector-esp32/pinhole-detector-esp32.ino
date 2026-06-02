#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin definitions
#define PIN_LIGHT_RELAY     4
#define PIN_MOTOR           5
#define PIN_BUTTON          18
#define PIN_BUZZER          25
#define PIN_LED_STOP        26
#define PIN_LED_START       27
#define PIN_LDR1            34
#define PIN_LDR2            35

// WiFi settings
const char* wifiSsid = "pinhole-detector";
const char* wifiPassword = "pinhole123";

// MQTT settings (same broker as ESP32-CAM)
const char* mqttServer = "192.168.131.85";
const int mqttPort = 1883;
const char* mqttClientId = "pinhole-esp32";
const char* topicState = "pinhole/state";
const char* topicPinhole = "pinhole/detected";
const char* topicControl = "pinhole/control";

// Objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// State variables
bool systemRunning = false;
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// LDR variables
int ambientLight = 0;
bool ambientRecorded = false;
const int PINHOLE_THRESHOLD = 50;
unsigned long lastLdrRead = 0;
const unsigned long ldrInterval = 100;

// Buzzer timing
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;
const unsigned long buzzerDuration = 2000;


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[BOOT] ESP32 Pinhole Detector starting");

  // Pin setup
  pinMode(PIN_LIGHT_RELAY, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_STOP, OUTPUT);
  pinMode(PIN_LED_START, OUTPUT);
  pinMode(PIN_LDR1, INPUT);
  pinMode(PIN_LDR2, INPUT);

  // Initial states
  digitalWrite(PIN_LIGHT_RELAY, LOW);
  digitalWrite(PIN_MOTOR, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_STOP, HIGH);
  digitalWrite(PIN_LED_START, LOW);

  // LCD setup
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pinhole Detector");
  lcd.setCursor(0, 1);
  lcd.print("System: STOPPED");

  // Connect WiFi
  connectWiFi();

  // Connect MQTT
  connectMqtt();

  Serial.println("[BOOT] Setup complete");
}

void loop() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Reconnecting...");
    connectWiFi();
  }

  // Check MQTT
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Reconnecting...");
    updateLcdLine(3, "MQTT: Reconnecting..");
    connectMqtt();
  }
  mqttClient.loop();

  // Handle button press (active low, debounced)
  handleButton();

  // Handle buzzer timeout
  if (buzzerActive && (millis() - buzzerStartTime >= buzzerDuration)) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerActive = false;
    Serial.println("[BUZZER] Timeout - stopped");
  }

  // System running logic
  if (systemRunning) {
    // Read LDRs periodically
    if (millis() - lastLdrRead >= ldrInterval) {
      lastLdrRead = millis();
      checkPinhole();
    }
  }
}

void connectWiFi() {
  Serial.print("[WIFI] Connecting to ");
  Serial.println(wifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("[WIFI] Connected, IP: ");
    Serial.println(WiFi.localIP());
    updateLcdLine(2, "WiFi: Connected");
  } else {
    Serial.println();
    Serial.println("[WIFI] Connection failed");
    updateLcdLine(2, "WiFi: Failed");
  }
}

void connectMqtt() {
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting to ");
    Serial.print(mqttServer);
    Serial.print(":");
    Serial.println(mqttPort);
    updateLcdLine(3, "MQTT: Connecting....");

    if (mqttClient.connect(mqttClientId)) {
      Serial.println("[MQTT] Connected");
      mqttClient.subscribe(topicControl);
      Serial.print("[MQTT] Subscribed to ");
      Serial.println(topicControl);
      updateLcdLine(3, "MQTT: Connected     ");
      publishState();
    } else {
      Serial.print("[MQTT] Failed, state=");
      Serial.println(mqttClient.state());
      updateLcdLine(3, "MQTT: Failed-retry..");
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  message.toLowerCase();

  Serial.print("[MQTT] Message on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (message == "start") {
    startSystem();
  } else if (message == "stop") {
    stopSystem();
  }
}

void handleButton() {
  int buttonState = digitalRead(PIN_BUTTON);

  if (buttonState == LOW && !buttonPressed) {
    if (millis() - lastDebounceTime > debounceDelay) {
      buttonPressed = true;
      lastDebounceTime = millis();

      Serial.println("[BUTTON] Pressed - toggling state");

      if (systemRunning) {
        stopSystem();
      } else {
        startSystem();
      }
    }
  } else if (buttonState == HIGH) {
    buttonPressed = false;
  }
}

void startSystem() {
  systemRunning = true;
  ambientRecorded = false;

  // Turn on outputs
  digitalWrite(PIN_LIGHT_RELAY, HIGH);
  digitalWrite(PIN_MOTOR, HIGH);
  digitalWrite(PIN_LED_START, HIGH);
  digitalWrite(PIN_LED_STOP, LOW);

  // LCD update
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pinhole Detector");
  lcd.setCursor(0, 1);
  lcd.print("System: RUNNING");
  lcd.setCursor(0, 2);
  lcd.print("Recording ambient...");

  // Record ambient light after relay stabilizes
  delay(500);
  int ldr1 = analogRead(PIN_LDR1);
  int ldr2 = analogRead(PIN_LDR2);
  ambientLight = (ldr1 + ldr2) / 2;
  ambientRecorded = true;

  lcd.setCursor(0, 2);
  lcd.print("Ambient: ");
  lcd.print(ambientLight);
  lcd.print("    ");

  Serial.print("[SYSTEM] Started, ambient light: ");
  Serial.println(ambientLight);

  publishState();
}

void stopSystem() {
  systemRunning = false;

  // Turn off outputs
  digitalWrite(PIN_LIGHT_RELAY, LOW);
  digitalWrite(PIN_MOTOR, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_START, LOW);
  digitalWrite(PIN_LED_STOP, HIGH);
  buzzerActive = false;

  // LCD update
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pinhole Detector");
  lcd.setCursor(0, 1);
  lcd.print("System: STOPPED");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");

  Serial.println("[SYSTEM] Stopped");

  publishState();
}

void checkPinhole() {
  if (!ambientRecorded) return;

  int ldr1 = analogRead(PIN_LDR1);
  int ldr2 = analogRead(PIN_LDR2);
  int averageLight = (ldr1 + ldr2) / 2;

  // Check if light increased significantly (pinhole detected)
  if (averageLight > ambientLight + PINHOLE_THRESHOLD) {
    Serial.print("[DETECT] Pinhole! LDR1=");
    Serial.print(ldr1);
    Serial.print(" LDR2=");
    Serial.print(ldr2);
    Serial.print(" Avg=");
    Serial.print(averageLight);
    Serial.print(" Threshold=");
    Serial.println(ambientLight + PINHOLE_THRESHOLD);

    // Trigger alarm
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerActive = true;
    buzzerStartTime = millis();

    // Update LCD
    lcd.setCursor(0, 3);
    lcd.print("PINHOLE DETECTED!");

    // Publish detection
    publishPinholeDetected(ldr1, ldr2, averageLight);
  }

  // Update LCD with current readings
  lcd.setCursor(0, 2);
  lcd.print("LDR:");
  lcd.print(averageLight);
  lcd.print(" T:");
  lcd.print(ambientLight + PINHOLE_THRESHOLD);
  lcd.print("    ");
}

void publishState() {
  if (!mqttClient.connected()) return;

  const char* state = systemRunning ? "running" : "stopped";
  bool published = mqttClient.publish(topicState, state, true);

  Serial.print("[MQTT] Published state: ");
  Serial.print(state);
  Serial.print(" -> ");
  Serial.println(published ? "OK" : "FAILED");
}

void publishPinholeDetected(int ldr1, int ldr2, int average) {
  if (!mqttClient.connected()) return;

  // Create JSON payload
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"ldr1\":%d,\"ldr2\":%d,\"average\":%d,\"ambient\":%d,\"timestamp\":%lu}",
           ldr1, ldr2, average, ambientLight, millis());

  bool published = mqttClient.publish(topicPinhole, payload);

  Serial.print("[MQTT] Published pinhole detection: ");
  Serial.println(published ? "OK" : "FAILED");
  Serial.println(payload);
}

void updateLcdLine(int line, const char* text) {
  lcd.setCursor(0, line);
  lcd.print(text);
}