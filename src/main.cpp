#include <Arduino.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// I2S pin definitions
#define I2S_SCK  2
#define I2S_WS   3
#define I2S_SD   4

// WiFi credentials
const char* ssid = "NBSJK";
const char* password = "12345679";

// UDP settings
const char* udpAddress = "192.168.1.100"; // Your computer's IP address
const int udpPort = 3333;
WiFiUDP udp;

// Audio buffer
const int BUFFER_SIZE = 256; // Number of samples in each UDP packet
int32_t samples[BUFFER_SIZE];

// Audio analysis variables
int32_t peak_value = 0;
bool connected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nINMP441 Audio Streaming");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());
  connected = true;
  
  // Initialize I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,  // Lower sample rate for network streaming
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };
  
  i2s_pin_config_t i2s_pins = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing I2S driver: %d\n", err);
    while (1);
  }
  
  err = i2s_set_pin(I2S_NUM_0, &i2s_pins);
  if (err != ESP_OK) {
    Serial.printf("Failed setting I2S pins: %d\n", err);
    while (1);
  }
  
  Serial.println("I2S driver installed successfully");
  Serial.println("Audio streaming starting - sending to UDP server!");
}

void loop() {
  if (!connected) return;
  
  size_t bytes_read = 0;
  
  // Read audio data from I2S
  esp_err_t err = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
  
  if (err != ESP_OK) {
    Serial.printf("Failed to read I2S data: %d\n", err);
    delay(1000);
    return;
  }
  
  // Process samples (convert to 16-bit for easier handling)
  int samples_count = bytes_read / 4;
  int16_t processed_samples[BUFFER_SIZE];
  
  for (int i = 0; i < samples_count; i++) {
    // Convert 32-bit to 16-bit (normalize volume)
    processed_samples[i] = samples[i] >> 16;
  }
  
  // Calculate simple audio level for display
  int32_t current_peak = 0;
  for (int i = 0; i < samples_count; i++) {
    int16_t abs_sample = abs(processed_samples[i]);
    if (abs_sample > current_peak) {
      current_peak = abs_sample;
    }
  }
  
  // Update peak with exponential decay
  if (current_peak > peak_value) {
    peak_value = current_peak;
  } else {
    peak_value = peak_value * 0.8; // Decay factor
  }
  
  // Send audio data over UDP
  udp.beginPacket(udpAddress, udpPort);
  udp.write((uint8_t*)processed_samples, samples_count * 2); // 2 bytes per sample (16-bit)
  udp.endPacket();
  
  // Display audio level every 500ms
  static unsigned long last_print = 0;
  if (millis() - last_print >= 500) {
    last_print = millis();
    
    int audio_level = map(peak_value, 0, 32767, 0, 20);
    
    // Display level meter
    Serial.print("\rSending [");
    for (int i = 0; i < 20; i++) {
      if (i < audio_level) {
        Serial.print(i < 10 ? "=" : "#");
      } else {
        Serial.print(" ");
      }
    }
    Serial.print("]");
  }
}