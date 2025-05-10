#include <Arduino.h>
#include <driver/i2s.h>

// ——— I²S pin assignment ———
#define I2S_DATA_PIN   7   // SD
#define I2S_LRCK_PIN   6   // WS (word select)
#define I2S_BCLK_PIN   5   // SCK (bit clock)

#define I2S_PORT       I2S_NUM_0

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println();
  Serial.println("🚀 Starting INMP441 I2S test…");

  // 1) Install & configure I2S driver
  Serial.println("1) Installing I2S driver…");
  i2s_config_t i2s_cfg = {
    .mode                 = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = 44100,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 1024,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_cfg, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("‼️ i2s_driver_install failed: 0x%02X\n", err);
    while (true) delay(1000);
  }
  Serial.println("✅ I2S driver installed");

  // 2) Attach pins
  Serial.println("2) Configuring I2S pins…");
  i2s_pin_config_t pin_cfg = {
    .bck_io_num   = I2S_BCLK_PIN,
    .ws_io_num    = I2S_LRCK_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_DATA_PIN
  };
  err = i2s_set_pin(I2S_PORT, &pin_cfg);
  if (err != ESP_OK) {
    Serial.printf("‼️ i2s_set_pin failed: 0x%02X\n", err);
    while (true) delay(1000);
  }
  Serial.println("✅ I2S pins set");

  Serial.println("✅ I2S initialization complete");
  Serial.println();
}

void loop() {
  // 3) Read a chunk of samples
  const size_t buf_samples = 512;
  int16_t  buf[buf_samples];
  size_t   bytes_read = 0;

  Serial.println("🔄 Reading I2S samples…");
  esp_err_t err = i2s_read(
    I2S_PORT,
    buf,
    buf_samples * sizeof(buf[0]),
    &bytes_read,
    portMAX_DELAY
  );
  if (err != ESP_OK) {
    Serial.printf("‼️ i2s_read error: 0x%02X\n", err);
    delay(500);
    return;
  }
  Serial.printf("ℹ️ Read %u bytes (%u samples)\n",
                (unsigned)bytes_read,
                (unsigned)(bytes_read / sizeof(buf[0])) );

  // 4) Compute average absolute amplitude
  uint64_t sum = 0;
  size_t   count = bytes_read / sizeof(buf[0]);
  for (size_t i = 0; i < count; ++i) {
    sum += abs(buf[i]);
  }
  uint32_t avg = sum / count;
  Serial.printf("ℹ️ Avg amplitude: %u\n", avg);

  // 5) Threshold detection
  const uint32_t threshold = 1000;
  if (avg > threshold) {
    Serial.println("🎙️ Sound detected!");
  } else {
    Serial.println("🤫 Silence");
  }

  Serial.println();
  delay(500);
}
