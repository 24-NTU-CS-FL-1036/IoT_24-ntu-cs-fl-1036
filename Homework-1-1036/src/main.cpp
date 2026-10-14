// Homework-01: ESP32 + SSD1306 OLED + debounced push button
// Wiring: OLED SDA->GPIO21, SCL->GPIO22, VCC->3V3, GND->GND
//         Button: GPIO18 <-> GND (INPUT_PULLUP, pressed = LOW)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS  0x3C
#define SDA_PIN       21
#define SCL_PIN       22
#define BUTTON_PIN    18

const unsigned long DEBOUNCE_MS = 50;   // input must stay stable this long

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int pressCount = 0;                 // number of accepted presses
int lastReading = HIGH;             // previous raw pin reading
int stableState = HIGH;             // debounced (accepted) state
unsigned long lastChangeTime = 0;   // when the raw reading last changed

void drawScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 Button Counter");

  display.setCursor(0, 20);
  display.print("Button: ");
  display.println(stableState == LOW ? "PRESSED" : "RELEASED");

  display.setTextSize(2);
  display.setCursor(0, 40);
  display.print("Count: ");
  display.print(pressCount);

  display.display();   // push buffer to the screen
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found! Check SDA/SCL/VCC/GND wiring.");
    while (true) { delay(1000); }
  }

  stableState = digitalRead(BUTTON_PIN);
  lastReading = stableState;
  drawScreen();
  Serial.println("System ready. Press the button.");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  // Raw input changed (could be bounce): restart the stability timer
  if (reading != lastReading) {
    lastChangeTime = millis();
    lastReading = reading;
  }

  // Input stable for DEBOUNCE_MS and different from accepted state: accept it
  if ((millis() - lastChangeTime) >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;

    if (stableState == LOW) {          // count only on press, once
      pressCount++;
      Serial.print("Press accepted. Count = ");
      Serial.println(pressCount);
    } else {
      Serial.println("Button released");
    }
    drawScreen();
  }
}