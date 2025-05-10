#include <Arduino.h>

// Morse code timing (in milliseconds)
const int DOT_DURATION = 200;
const int DASH_DURATION = DOT_DURATION * 3;
const int ELEMENT_PAUSE = DOT_DURATION;      // Space between dots and dashes
const int LETTER_PAUSE = DOT_DURATION * 3;   // Space between letters
const int WORD_PAUSE = DOT_DURATION * 7;     // Space between words

// Try all possible LED pins one by one
void testAllPins() {
  Serial.println("Testing all possible LED pins...");
  
  // Test pins 0 through 21 one by one
  for (int pin = 0; pin <= 21; pin++) {
    Serial.print("Testing pin ");
    Serial.print(pin);
    Serial.println("...");
    
    // Set pin as output
    pinMode(pin, OUTPUT);
    
    // Turn on LED for 500ms
    digitalWrite(pin, HIGH);
    delay(500);
    
    // Turn off LED
    digitalWrite(pin, LOW);
    delay(500);
  }
  
  Serial.println("Pin test complete.");
}

// Function to blink a dot on the specified pin
void dot(int pin) {
  digitalWrite(pin, HIGH);
  delay(DOT_DURATION);
  digitalWrite(pin, LOW);
  delay(ELEMENT_PAUSE);
}

// Function to blink a dash on the specified pin
void dash(int pin) {
  digitalWrite(pin, HIGH);
  delay(DASH_DURATION);
  digitalWrite(pin, LOW);
  delay(ELEMENT_PAUSE);
}

// Function to blink I LOVE YOU JENNY in Morse code on a specific pin
void blinkMorseOnPin(int pin) {
  Serial.print("Blinking on pin ");
  Serial.print(pin);
  Serial.println(": I LOVE YOU JENNY");
  
  // I (··)
  dot(pin); dot(pin);
  delay(LETTER_PAUSE);
  
  // Space between words
  delay(WORD_PAUSE - LETTER_PAUSE);
  
  // L (·−··)
  dot(pin); dash(pin); dot(pin); dot(pin);
  delay(LETTER_PAUSE);
  
  // O (−−−)
  dash(pin); dash(pin); dash(pin);
  delay(LETTER_PAUSE);
  
  // V (···−)
  dot(pin); dot(pin); dot(pin); dash(pin);
  delay(LETTER_PAUSE);
  
  // E (·)
  dot(pin);
  delay(LETTER_PAUSE);
  
  // Space between words
  delay(WORD_PAUSE - LETTER_PAUSE);
  
  // Y (−·−−)
  dash(pin); dot(pin); dash(pin); dash(pin);
  delay(LETTER_PAUSE);
  
  // O (−−−)
  dash(pin); dash(pin); dash(pin);
  delay(LETTER_PAUSE);
  
  // U (··−)
  dot(pin); dot(pin); dash(pin);
  delay(LETTER_PAUSE);
  
  // Space between words
  delay(WORD_PAUSE - LETTER_PAUSE);
  
  // J (·−−−)
  dot(pin); dash(pin); dash(pin); dash(pin);
  delay(LETTER_PAUSE);
  
  // E (·)
  dot(pin);
  delay(LETTER_PAUSE);
  
  // N (−·)
  dash(pin); dot(pin);
  delay(LETTER_PAUSE);
  
  // N (−·)
  dash(pin); dot(pin);
  delay(LETTER_PAUSE);
  
  // Y (−·−−)
  dash(pin); dot(pin); dash(pin); dash(pin);
  delay(LETTER_PAUSE);
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(2000);  // Give time to open serial monitor
  
  Serial.println("ESP32-C3-DevKitM-1 LED Test");
  Serial.println("==========================");
  
  // First, try LED_BUILTIN if it's defined
  #ifdef LED_BUILTIN
    Serial.print("LED_BUILTIN is defined as pin ");
    Serial.println(LED_BUILTIN);
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("Blinking built-in LED...");
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  #else
    Serial.println("LED_BUILTIN is not defined for this board");
  #endif
  
  // Test commonly used LED pins for ESP32-C3
  int commonPins[] = {8, 2, 3, 4, 5, 6, 7, 9, 10, 18, 19};
  
  for (int i = 0; i < sizeof(commonPins)/sizeof(commonPins[0]); i++) {
    int pin = commonPins[i];
    Serial.print("Testing common LED pin ");
    Serial.println(pin);
    
    pinMode(pin, OUTPUT);
    
    // Blink the pin 3 times
    for (int j = 0; j < 3; j++) {
      digitalWrite(pin, HIGH);
      delay(200);
      digitalWrite(pin, LOW);
      delay(200);
    }
    
    delay(1000);
  }
  
  // If still no luck, test all pins
  testAllPins();
}

void loop() {
  // Try LED_BUILTIN if defined
  #ifdef LED_BUILTIN
    blinkMorseOnPin(LED_BUILTIN);
  #endif
  
  // Try on commonly used pins
  blinkMorseOnPin(8);  // Most likely for ESP32-C3-DevKitM-1
  delay(1000);
  
  blinkMorseOnPin(2);  // Common on many ESP32 boards
  delay(1000);
  
  blinkMorseOnPin(3);  // Try other pins
  delay(1000);
  
  delay(3000);  // Pause before repeating
}