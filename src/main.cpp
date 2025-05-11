#include <Arduino.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// I2S pin definitions
#define I2S_SCK 2
#define I2S_WS 3
#define I2S_SD 4

//=============== ADJUSTABLE PARAMETERS (EDIT THESE VALUES) ===============//
// WiFi credentials
const char* ssid = "NBSJK";
const char* password = "12345679";

// UDP settings
const char* udpAddress = "192.168.0.181"; // Your server IP address
const int udpPort = 3333;

// Audio sample rate (options: 8000, 16000, 22050, 32000, 44100)
const int SAMPLE_RATE = 8000;

// Samples per packet
const int BUFFER_SIZE = 256;

// Audio gain (0-200%)
int gain = 70;

// Noise gate threshold (0-100%)
// - 0: No noise gate (all audio passes through)
// - 50: Medium noise gate (quiet sounds are muted)
// - 100: Maximum noise gate (only loud sounds pass through)
int noise_gate = 0;

// Audio compression (0-100%)
// - 0: No compression (full dynamic range)
// - 50: Medium compression (reduces dynamic range by half)
// - 100: Maximum compression (minimal dynamic range)
int compression = 0;

// Enable UDP streaming (true/false)
bool streaming_enabled = true;
//=======================================================================//

// Audio buffer
int32_t samples[BUFFER_SIZE];
int16_t processed_samples[BUFFER_SIZE];

// Audio analysis variables
int32_t peak_value = 0;
uint32_t packet_counter = 0;

// WiFi and UDP objects
WiFiUDP udp;

// Setup I2S
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
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
    return;
  }

  err = i2s_set_pin(I2S_NUM_0, &i2s_pins);
  if (err != ESP_OK) {
    Serial.printf("Failed setting I2S pins: %d\n", err);
    return;
  }

  Serial.printf("I2S initialized at %d Hz\n", SAMPLE_RATE);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 Microphone UDP Streamer");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed. Restarting...");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println();
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize I2S
  setupI2S();
  
  // Print current settings
  Serial.println("Audio settings:");
  Serial.printf("- Sample rate: %d Hz\n", SAMPLE_RATE);
  Serial.printf("- Buffer size: %d samples\n", BUFFER_SIZE);
  Serial.printf("- Gain: %d%%\n", gain);
  Serial.printf("- Noise gate: %d%%\n", noise_gate);
  Serial.printf("- Compression: %d%%\n", compression);
  Serial.printf("- Streaming: %s\n", streaming_enabled ? "enabled" : "disabled");
  Serial.printf("- UDP target: %s:%d\n", udpAddress, udpPort);
  
  Serial.println("Starting audio streaming...");
}

void loop() {
  // Skip streaming if disabled
  if (!streaming_enabled) {
    delay(100);
    return;
  }
  
  // Read audio data from I2S
  size_t bytes_read = 0;
  esp_err_t err = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
  
  if (err != ESP_OK) {
    Serial.printf("Failed to read I2S data: %d\n", err);
    delay(1000);
    return;
  }
  
  // Process samples
  int samples_count = bytes_read / 4;
  
  for (int i = 0; i < samples_count; i++) {
    // Normalize 32-bit sample to 16-bit
    int32_t normalized = samples[i] >> 16;
    
    // Apply noise gate if enabled
    if (noise_gate > 0) {
      int32_t threshold = (32767 * noise_gate) / 100;
      if (abs(normalized) < threshold) {
        normalized = 0;
      } else {
        // Apply soft threshold
        normalized = (normalized > 0) ? 
                      normalized - threshold : 
                      normalized + threshold;
      }
    }
    
    // Apply gain
    float gain_factor = gain / 100.0;
    normalized = normalized * gain_factor;
    
    // Apply compression if enabled
    if (compression > 0) {
      float comp_factor = compression / 100.0;
      float comp_threshold = 0.5; // 50% level for compression to start
      
      if (abs(normalized) > 32767 * comp_threshold) {
        // Apply compression above threshold
        float excess = abs(normalized) - (32767 * comp_threshold);
        excess = excess * (1.0 - comp_factor);
        normalized = (normalized > 0) ? 
                      (32767 * comp_threshold) + excess : 
                      -((32767 * comp_threshold) + excess);
      }
    }
    
    // Clip to 16-bit range
    if (normalized > 32767) normalized = 32767;
    if (normalized < -32767) normalized = -32767;
    
    processed_samples[i] = normalized;
  }
  
  // Calculate audio level for display
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
  
  // Send audio data to server
  udp.beginPacket(udpAddress, udpPort);
  udp.write((uint8_t*)&packet_counter, sizeof(packet_counter));  // Send packet counter
  udp.write((uint8_t*)&peak_value, sizeof(peak_value));          // Send peak value
  udp.write((uint8_t*)processed_samples, samples_count * 2);     // Send audio samples
  udp.endPacket();
  
  packet_counter++;
  
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
    Serial.print("] ");
    Serial.printf("Gain: %d%% Peak: %d", gain, peak_value);
  }
}