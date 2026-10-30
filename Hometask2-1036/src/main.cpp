// Homework 2 - Task 1: Dual-LED fading with different PWM frequencies
// Board : ESP32 DevKit (LEDC peripheral)
// LED1  : GPIO 25, 1000 Hz
// LED2  : GPIO 26, 5000 Hz
// Works with Arduino-ESP32 core 2.x and 3.x.

#include <Arduino.h>

// ---------------- Pins ----------------
const int LED1_PIN = 25;
const int LED2_PIN = 26;

// ---------------- PWM configuration ----------------
const uint32_t LED1_FREQ = 1000;   // Hz
const uint32_t LED2_FREQ = 5000;   // Hz
const uint8_t  PWM_RES   = 10;     // 10-bit -> duty 0..1023
const int MAX_DUTY = (1 << PWM_RES) - 1;

// Core 2.x only: LEDC channels. Channels 0 and 2 sit on DIFFERENT
// timers (timer = channel / 2), so they can have different frequencies.
// Channels 0 and 1 would share one timer and be forced to one frequency.
const int LED1_CH = 0;
const int LED2_CH = 2;

// ---------------- Fade state (kept global for Task 2 OLED) ----------------
int duty1 = 0;          // LED1 current duty (0..1023)
int duty2 = MAX_DUTY;   // LED2 current duty (starts bright)
int step1 = 8;          // LED1 fade step
int step2 = -16;        // LED2 fade step (different speed and direction)

unsigned long lastFade = 0;
unsigned long lastPrint = 0;

// Write a duty value to an LED (hides the core 2.x / 3.x API difference)
void pwmWrite(int pin, int channel, int duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);        // core 3.x: addressed by pin
#else
  ledcWrite(channel, duty);    // core 2.x: addressed by channel
#endif
}

// Move duty by its step and reverse direction at 0 and MAX_DUTY
void fade(int &duty, int &step) {
  duty += step;
  if (duty >= MAX_DUTY) { duty = MAX_DUTY; step = -step; }
  if (duty <= 0)        { duty = 0;        step = -step; }
}

void setup() {
  Serial.begin(115200);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3.x: core picks a free timer for each different frequency
  ledcAttach(LED1_PIN, LED1_FREQ, PWM_RES);
  ledcAttach(LED2_PIN, LED2_FREQ, PWM_RES);
#else
  // Core 2.x: set up channel (frequency + resolution), then attach pin
  ledcSetup(LED1_CH, LED1_FREQ, PWM_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);
  ledcSetup(LED2_CH, LED2_FREQ, PWM_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);
#endif

  Serial.println("Dual PWM LED fading started");
  Serial.printf("LED1: GPIO %d, %lu Hz, %u-bit\n", LED1_PIN, (unsigned long)LED1_FREQ, PWM_RES);
  Serial.printf("LED2: GPIO %d, %lu Hz, %u-bit\n", LED2_PIN, (unsigned long)LED2_FREQ, PWM_RES);
}

void loop() {
  unsigned long now = millis();

  // Update both LEDs every 10 ms (non-blocking, no delay())
  if (now - lastFade >= 10) {
    lastFade = now;
    fade(duty1, step1);
    fade(duty2, step2);
    pwmWrite(LED1_PIN, LED1_CH, duty1);
    pwmWrite(LED2_PIN, LED2_CH, duty2);
  }

  // Print duty percentages twice per second (Serial check of values)
  if (now - lastPrint >= 500) {
    lastPrint = now;
    Serial.printf("LED1: %3d%% @ %luHz | LED2: %3d%% @ %luHz\n",
                  (duty1 * 100) / MAX_DUTY, (unsigned long)LED1_FREQ,
                  (duty2 * 100) / MAX_DUTY, (unsigned long)LED2_FREQ);
  }
}