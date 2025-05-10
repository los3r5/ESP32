#include <Arduino.h>
#include <driver/i2s.h>

// Try these pins instead
#define I2S_SCK  2
#define I2S_WS   3
#define I2S_SD   4

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\nINMP441 Test");
  
  // Simplified I2S config for ESP32-C3 with INMP441
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
  
  // Check any of the samples are non-zero
  bool has_data = false;
  for (int i = 0; i < bytes_read/4; i++) {
    if (samples[i] != 0) {
      has_data = true;
      break;
    }
  }
  
  if (has_data) {
    Serial.println("Got audio data!");
    // Print the first 5 samples
    for (int i = 0; i < 5; i++) {
      Serial.printf("%d ", samples[i]);
    }
    Serial.println();
  } else {
    Serial.println("No audio data (all zeros)");
  }
  
  delay(500);
}