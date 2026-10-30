#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- OLED settings ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C

// ---------------- LED pins ----------------
#define LED1_PIN 18
#define LED2_PIN 19

// ---------------- PWM settings ----------------
// LED1 uses channel 0 (LEDC timer 0), LED2 uses channel 2 (LEDC timer 1).
// On the original ESP32, channel pairs (0/1, 2/3, 4/5, 6/7) share a timer,
// so using channels 0 and 2 puts the LEDs on different timers.
// This allows two different PWM frequencies at the same time.
#define LED1_CH 0
#define LED2_CH 2
#define LED1_FREQ 1000   // Hz
#define LED2_FREQ 5000   // Hz
#define PWM_RES 8        // 8-bit resolution -> duty range 0..255
#define MAX_DUTY 255

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Current duty values (raw 0..255). The same values are written to
// the LEDs and shown on the OLED, so the display always matches.
int duty1 = 0;
int duty2 = 0;

// Fade step size per loop. Different steps make the LEDs fade
// at different speeds, so they are easy to tell apart.
int step1 = 2;
int step2 = 3;

unsigned long lastDisplay = 0;

// Convert raw duty value (0..255) to percentage (0..100)
int toPercent(int duty) {
  return duty * 100 / MAX_DUTY;
}

// Move duty up or down by one step; reverse direction at the limits
void fadeStep(int &duty, int &step) {
  duty += step;
  if (duty >= MAX_DUTY) {
    duty = MAX_DUTY;
    step = -step;
  } else if (duty <= 0) {
    duty = 0;
    step = -step;
  }
}

// Draw duty cycle and frequency of both LEDs on the OLED
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("PWM MONITOR");
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setCursor(0, 24);
  display.print("LED1: ");
  display.print(toPercent(duty1));
  display.print("% F:");
  display.print(LED1_FREQ);
  display.println("Hz");

  display.setCursor(0, 40);
  display.print("LED2: ");
  display.print(toPercent(duty2));
  display.print("% F:");
  display.print(LED2_FREQ);
  display.println("Hz");

  display.display();
}

void setup() {
  Serial.begin(115200);

  // Start I2C and OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED initialization failed!");
    while (true) {
      delay(100);
    }
  }

  // Configure LEDC PWM (Arduino-ESP32 core 2.x API)
  ledcSetup(LED1_CH, LED1_FREQ, PWM_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);
  ledcSetup(LED2_CH, LED2_FREQ, PWM_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);

  updateDisplay();
}

void loop() {
  // Update duty values
  fadeStep(duty1, step1);
  fadeStep(duty2, step2);

  // Apply the same values to the PWM outputs
  ledcWrite(LED1_CH, duty1);
  ledcWrite(LED2_CH, duty2);

  // Refresh OLED and serial monitor every 100 ms
  if (millis() - lastDisplay >= 100) {
    lastDisplay = millis();
    updateDisplay();

    Serial.print("LED1: ");
    Serial.print(toPercent(duty1));
    Serial.print("% (");
    Serial.print(LED1_FREQ);
    Serial.print(" Hz)  LED2: ");
    Serial.print(toPercent(duty2));
    Serial.print("% (");
    Serial.print(LED2_FREQ);
    Serial.println(" Hz)");
  }

  delay(15);
}