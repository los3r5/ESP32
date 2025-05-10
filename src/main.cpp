#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Wi-Fi credentials - replace with your network details
const char* ssid = "NBSJK";      // IMPORTANT: Replace with your actual WiFi name
const char* password = "12345679"; // IMPORTANT: Replace with your actual WiFi password

// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;      // GMT offset in seconds (0 for GMT, modify for your timezone)
const int daylightOffset_sec = 3600; // Daylight savings offset (3600 seconds = 1 hour)

// Variables
unsigned long lastTimeCheck = 0;
const long timeCheckInterval = 10000; // Check time every 10 seconds
bool wifiConnected = false;

// Function declarations
void connectToWiFi();
void printLocalTime();
bool setupNTP();

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000); // Give time for serial connection to establish
  
  Serial.println("\n\n===== ESP32-C3 Firmware Starting =====");
  Serial.println("Initializing...");
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup NTP time sync
  if (wifiConnected) {
    if (setupNTP()) {
      Serial.println("NTP setup successful");
      // Get and print the time immediately once
      printLocalTime();
    } else {
      Serial.println("NTP setup failed");
    }
  }
  
  Serial.println("Setup complete!");
}

void loop() {
  //print on every single loop

  Serial.println("Hey im looping");
  // Check if WiFi is still connected
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    Serial.println("WiFi connection lost. Reconnecting...");
    wifiConnected = false;
    connectToWiFi();
  }
  
  // Check and print time at regular intervals
  unsigned long currentMillis = millis();
  if (wifiConnected && (currentMillis - lastTimeCheck >= timeCheckInterval)) {
    lastTimeCheck = currentMillis;
    printLocalTime();
  }
  printLocalTime();
  Serial.println("ending the loop, wifiConnected:");
  Serial.println(wifiConnected);
  
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait for connection with timeout
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("");
    Serial.println("WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("");
    Serial.println("WiFi connection failed. Check your credentials or network availability.");
  }
}

bool setupNTP() {
  Serial.println("Setting up NTP time sync...");
  
  // Configure NTP time sync
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Check if we got the time
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time from NTP server");
    return false;
  }
  
  return true;
}

void printLocalTime() {
  struct tm timeinfo;
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain current time");
    return;
  }
  
  // Format and print time details
  Serial.println("=== Current Time Information ===");
  Serial.print("Date: ");
  Serial.print(timeinfo.tm_mday);
  Serial.print("/");
  Serial.print(timeinfo.tm_mon + 1); // tm_mon is months since January (0-11)
  Serial.print("/");
  Serial.println(1900 + timeinfo.tm_year); // tm_year is years since 1900
  
  Serial.print("Time: ");
  Serial.print(timeinfo.tm_hour);
  Serial.print(":");
  if(timeinfo.tm_min < 10) Serial.print("0"); // Add leading zero for single-digit minutes
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  if(timeinfo.tm_sec < 10) Serial.print("0"); // Add leading zero for single-digit seconds
  Serial.println(timeinfo.tm_sec);
  
  char timeStr[50];
  strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(timeStr);
  Serial.println("===============================");
}