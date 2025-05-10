#include <Arduino.h>
#include <driver/i2s.h>

// Keep your original pin definitions
#define I2S_SCK  2
#define I2S_WS   3
#define I2S_SD   4

// Optional: Add LED for visual audio level indicator
#define LED_PIN  10  // Change to any available GPIO pin, or remove if not using

// Audio analysis variables
int32_t peak_value = 0;
float avg_rms = 0;
const int HISTORY_SIZE = 10;
float level_history[HISTORY_SIZE] = {0};
int history_index = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\nINMP441 Test");
  
  // Optional: Initialize LED
  #ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  #endif
  
  // Preserved exactly as in your original code
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,  // Try a standard audio rate
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
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
  Serial.println("Audio test starting - make some noise!");
}

void loop() {
  int32_t samples[128];
  size_t bytes_read = 0;
  
  esp_err_t err = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
  
  if (err != ESP_OK) {
    Serial.printf("Failed to read I2S data: %d\n", err);
    delay(1000);
    return;
  }
  
  // Calculate audio metrics
  int samples_count = bytes_read / 4;  // 4 bytes per sample (32-bit)
  int64_t sum = 0;
  int64_t sum_squares = 0;
  int32_t current_peak = 0;
  
  // Process all samples in this batch
  for (int i = 0; i < samples_count; i++) {
    // The INMP441 sends 24-bit audio in MSB format
    // For ESP32, we need to shift and sign-extend the 24-bit value
    int32_t sample = samples[i] >> 8;
    
    // Calculate absolute value for peak detection
    int32_t abs_sample = abs(sample);
    
    // Update peak if this sample is louder
    if (abs_sample > current_peak) {
      current_peak = abs_sample;
    }
    
    // Sum for average and RMS calculations
    sum += sample;
    sum_squares += (int64_t)sample * sample;
  }
  
  // Update peak with exponential decay (keeps peak visible longer)
  if (current_peak > peak_value) {
    peak_value = current_peak;
  } else {
    peak_value = peak_value * 0.8; // Decay factor
  }
  
  // Calculate RMS (Root Mean Square) - useful for audio level
  float rms = sqrt((float)sum_squares / samples_count);
  
  // Smooth RMS using moving average
  level_history[history_index] = rms;
  history_index = (history_index + 1) % HISTORY_SIZE;
  
  float smoothed_rms = 0;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    smoothed_rms += level_history[i];
  }
  smoothed_rms /= HISTORY_SIZE;
  
  // Normalized audio level (0-100%)
  int audio_level = map(smoothed_rms, 0, 1000000, 0, 100);
  audio_level = constrain(audio_level, 0, 100);
  
  // Check if audio is detected
  bool has_audio = (audio_level > 1);  // Threshold to avoid noise
  
  // Optional: Control LED brightness based on audio level
  #ifdef LED_PIN
  analogWrite(LED_PIN, map(audio_level, 0, 100, 0, 255));
  #endif
  
  // Print audio statistics every 200ms
  static unsigned long last_print = 0;
  if (millis() - last_print >= 200) {
    last_print = millis();
    
    // Clear line for a cleaner display
    Serial.print("\r                                                            \r");
    
    if (has_audio) {
      // Display audio level meter in serial console
      Serial.print("Level [");
      int bars = map(audio_level, 0, 100, 0, 20);
      for (int i = 0; i < 20; i++) {
        if (i < bars) {
          Serial.print(i < 10 ? "=" : "#");  // Different characters for louder sounds
        } else {
          Serial.print(" ");
        }
      }
      Serial.print("] ");
      Serial.printf("%d%% ", audio_level);
      
      // Print some raw samples for debugging
      Serial.print("Samples: ");
      for (int i = 0; i < 3; i++) {
        Serial.printf("%d ", samples[i]);
      }
    } else {
      Serial.print("No audio detected (background noise)");
    }
  }
  
  // No delay needed - I2S reading provides timing
}