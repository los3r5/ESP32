#include <WiFi.h>
#include <HTTPClient.h>

// your Wi-Fi credentials
const char* ssid     = "NBSJK";
const char* password = "12345679";

// check interval (ms)
const unsigned long CHECK_INTERVAL = 30UL * 1000UL;

unsigned long lastCheck = 0;

// status LEDs
const int LED_NOT_CONNECTED_PIN = 6;   // turns ON when no internet
const int LED_CONNECTED_PIN     = 15;  // turns ON when internet is reachable

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("→ Starting Wi-Fi connection…");

  // configure LED pins
  pinMode(LED_NOT_CONNECTED_PIN, OUTPUT);
  pinMode(LED_CONNECTED_PIN, OUTPUT);
  digitalWrite(LED_NOT_CONNECTED_PIN, LOW);
  digitalWrite(LED_CONNECTED_PIN, LOW);

  // start Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // wait up to 10 seconds for Wi-Fi
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  // initial status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✅ Connected! IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_CONNECTED_PIN, HIGH);
    digitalWrite(LED_NOT_CONNECTED_PIN, LOW);
  } else {
    Serial.println("❌ Failed to connect to Wi-Fi");
    digitalWrite(LED_CONNECTED_PIN, LOW);
    digitalWrite(LED_NOT_CONNECTED_PIN, HIGH);
  }

  // force first internet check immediately
  lastCheck = millis() - CHECK_INTERVAL;
}

bool checkInternet() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ Wi-Fi not connected");
    return false;
  }

  HTTPClient http;
  http.begin("http://clients3.google.com/generate_204");
  int code = http.GET();
  http.end();

  if (code == 204) {
    return true;
  } else {
    Serial.printf("→ HTTP check returned %d\n", code);
    return false;
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastCheck >= CHECK_INTERVAL) {
    lastCheck = now;

    Serial.println();
    Serial.println("→ Testing Internet connectivity…");

    if (checkInternet()) {
      Serial.println("✅ Internet is reachable!");
      digitalWrite(LED_CONNECTED_PIN, HIGH);
      digitalWrite(LED_NOT_CONNECTED_PIN, LOW);
    } else {
      Serial.println("❌ No Internet connectivity.");
      digitalWrite(LED_CONNECTED_PIN, LOW);
      digitalWrite(LED_NOT_CONNECTED_PIN, HIGH);
    }
  }

  // small delay so we don't starve other tasks
  delay(10);
}
