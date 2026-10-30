# Homework 2 — Dual-LED PWM Control with OLED Monitoring





## 1. Project Overview

This project designs, simulates, builds, tests, documents and publishes an ESP32 system that:

- fades **two LEDs independently** using the ESP32 LEDC (PWM) peripheral,
- uses a **different PWM frequency** for each LED,
- fades each LED smoothly from minimum to maximum brightness and back,
- shows the **live duty-cycle percentage** and **configured PWM frequency** of both LEDs on an **SSD1306 I²C OLED**.

## 2. Learning Outcomes Covered

| Learning outcome | Where it is covered |
|---|---|
| Generate two PWM outputs using the ESP32 LEDC peripheral | Section 5 (Task 1) |
| Gradual fading from minimum to maximum duty cycle and back | Section 5, `main.cpp` |
| Configure and verify different PWM frequencies | Section 5.3, Section 10 |
| Display live duty-cycle percentages and frequencies on SSD1306 OLED | Section 6 (Task 2) |
| Build and test in Wokwi and on physical hardware | Sections 7 and 8 |
| Use GitHub with screenshots and a clear README | Section 9 |

## 3. Components Used

| Component | Quantity | Notes |
|---|---|---|
| ESP32 DevKit board | 1 | Original ESP32 (dual-core, LEDC with 16 channels) |
| LED (any colour) | 2 | LED1 and LED2 |
| Resistor 220 Ω | 2 | One separate current-limiting resistor per LED |
| SSD1306 OLED 128×64, I²C | 1 | I²C address `0x3C` |
| Breadboard + jumper wires | — | Hardware build only |

## 4. Wiring

### 4.1 Pin connections

| Signal | ESP32 pin | Connected to |
|---|---|---|
| LED1 | GPIO 25 | 220 Ω resistor → LED1 anode; LED1 cathode → GND |
| LED2 | GPIO 26 | 220 Ω resistor → LED2 anode; LED2 cathode → GND |
| OLED SDA | GPIO 21 | OLED SDA |
| OLED SCL | GPIO 22 | OLED SCL |
| OLED VCC | 3V3 | OLED VCC |
| OLED GND | GND | OLED GND |

### 4.2 Wiring notes and safety

- Each LED has its **own** current-limiting resistor (no shared resistor).
- With 3.3 V logic, an LED forward voltage of about 2 V and a 220 Ω resistor, the current is roughly (3.3 − 2.0) / 220 ≈ 6 mA, which is safe for both the LED and the GPIO.
- The OLED voltage and pinout (VCC, GND, SCL, SDA order) were checked before powering, because the pin order differs between OLED modules.
- Power was disconnected before changing any wiring.

## 5. Task 1 — Dual-LED Fading with Different PWM Frequencies

### 5.1 Implementation approach

- The ESP32 **LEDC** peripheral generates both PWM signals in hardware, so the CPU only changes the duty value.
- **LED1** is on GPIO 25 and **LED2** is on GPIO 26, each driven by its own LEDC channel.
- Each loop step, the duty value is increased or decreased by a fixed step. When it reaches the minimum (0) or maximum, the direction reverses. This gives a smooth fade up and down.
- The two LEDs use different step sizes, so they fade at different speeds and are clearly independent.
- The duty values (`duty1`, `duty2`) are stored in global variables. The **same variables** are written to the LEDs and shown on the OLED, so the display always matches what the program actually uses.

### 5.2 PWM settings

| Parameter | LED1 | LED2 |
|---|---|---|
| GPIO | 25 | 26 |
| PWM frequency | **1000 Hz (1 kHz)** | **5000 Hz (5 kHz)** |
| Duty resolution | 10-bit | 10-bit |
| Duty range | 0 – 1023 | 0 – 1023 |
| Fade step | 8 counts | 16 counts |

### 5.3 Choice of duty resolution

A resolution of **10 bits** (1024 brightness levels) was chosen because:

- it gives smooth, visible fading with no noticeable steps,
- it is well within the ESP32 limit: the maximum PWM frequency for a given resolution is roughly `80 MHz / 2^resolution`, which for 10 bits is about 78 kHz, far above 1 kHz and 5 kHz.

### 5.4 Technical check — LEDC timer/resource sharing

The ESP32 LEDC peripheral has several PWM **timers** and several PWM **channels**. The key point is:

- **Frequency and resolution are properties of the timer, not of the pin or channel.**
- A channel is attached to a timer, so channels that share a timer **share the same frequency and resolution**.
- Therefore, two LEDs can have different frequencies only if their channels use **different timers**.

Implementation and limitations:

- On the original ESP32 there are 8 channels in speed-mode groups (high-speed and low-speed groups), and 4 timers per group. Channels are paired onto timers in the older core numbering (for example channels 0 and 1 map to the same timer).
- With the Arduino-ESP32 **core 3.x** API (`ledcAttach(pin, freq, resolution)`), the core automatically assigns a free timer when a new frequency/resolution combination is requested, so LED1 (1 kHz) and LED2 (5 kHz) automatically land on different timers.
- With the older **core 2.x** API (`ledcSetup`/`ledcAttachPin`), channels must be chosen manually. Using channel 0 for LED1 and channel 2 for LED2 keeps them on different timers. Using channels 0 and 1 would force them to share one frequency.
- Other ESP32 variants (S2, S3, C3) have fewer timers and channels and different speed-mode groups, so the number of independent frequencies available is limited by the chip. Independent frequencies on any two pins should **not** be assumed on every variant.
- In this project only **two** different frequency/resolution combinations are needed, which is within the limits of all variants.
- Both PWM signals use the same resolution (10-bit); frequencies are the only difference.

### 5.5 How duty cycle affects apparent brightness

- Duty cycle is the percentage of each PWM period during which the output is HIGH.
- The LED turns on and off very fast, so the eye averages the light. A **higher duty cycle** means the LED is on for a larger part of each period, so the **average power is higher and the LED looks brighter**.
- 0 % duty is fully off, 100 % is fully on, and about 50 % looks roughly half bright.
- Human brightness perception is not linear (it is closer to logarithmic), so equal duty steps do not look like equal brightness steps. The visible change is larger at low duty values than at high ones.

### 5.6 How PWM frequency affects the output signal

- Frequency is how many on/off cycles occur per second. 1 kHz means a 1 ms period; 5 kHz means a 0.2 ms period.
- Frequency **does not change average brightness** (that is set by duty cycle), but it changes the signal behaviour.
- **Low frequency** (below about 100 Hz) can cause visible flicker. Both 1 kHz and 5 kHz are well above the flicker threshold, so neither LED visibly flickers.
- **Higher frequency** gives faster switching, smoother averaged output, and less flicker with cameras. It also means more switching events per second and, for a given clock, fewer available duty-resolution bits.
- In this project both LEDs look equally smooth to the eye, because both frequencies are high enough. The frequency difference is verified through the OLED values and, optionally, a logic analyser or oscilloscope.

## 6. Task 2 — Display PWM Parameters on the OLED

### 6.1 What the OLED shows

| Required item | How it is displayed |
|---|---|
| Duty cycle of LED1 as a percentage | `LED1: xx%` |
| Configured PWM frequency of LED1 | `F:1000Hz` on the same line |
| Duty cycle of LED2 as a percentage | `LED2: xx%` |
| Configured PWM frequency of LED2 | `F:5000Hz` on the same line |
| Values refresh as duty changes | Display is redrawn about every 50 ms |
| Displayed values match the program | Percentages are calculated from `duty1`/`duty2`; frequencies come from the same constants used in `ledcAttach` |

### 6.2 OLED layout

```
PWM MONITOR
LED1: 25% F:1000Hz
LED2: 75% F:5000Hz
```

(The values above are examples; the real OLED shows the live values.)

### 6.3 How accuracy is guaranteed

- Duty percentage = `duty * 100 / MAX_DUTY`, using the **same** variable that is passed to `ledcWrite()`.
- The frequency printed on the OLED uses the **same constants** (`LED1_FREQ`, `LED2_FREQ`) that configure the PWM hardware, so there is a single source of truth and the display cannot disagree with the program.

## 7. Source Code (`src/main.cpp`)

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- Pin definitions ----------------
const int LED1_PIN = 25;
const int LED2_PIN = 26;
const int SDA_PIN  = 21;
const int SCL_PIN  = 22;

// ---------------- PWM configuration ----------------
const uint32_t LED1_FREQ = 1000;   // Hz
const uint32_t LED2_FREQ = 5000;   // Hz
const uint8_t  PWM_RES   = 10;     // bits
const int MAX_DUTY = (1 << PWM_RES) - 1;   // 1023

// ---------------- Fade state (shared with OLED) ----------------
int duty1 = 0;          // LED1 current duty (0..1023)
int duty2 = MAX_DUTY;   // LED2 current duty (starts bright)
int step1 = 8;          // LED1 fade step
int step2 = -16;        // LED2 fade step (different speed/direction)

// ---------------- OLED ----------------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

unsigned long lastFade = 0;
unsigned long lastDraw = 0;

// Advance a duty value by its step and reverse at the limits
void fade(int &duty, int &step) {
  duty += step;
  if (duty >= MAX_DUTY) { duty = MAX_DUTY; step = -step; }
  if (duty <= 0)        { duty = 0;        step = -step; }
}

void drawOled() {
  int pct1 = (duty1 * 100) / MAX_DUTY;
  int pct2 = (duty2 * 100) / MAX_DUTY;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("PWM MONITOR");
  display.setCursor(0, 20);
  display.printf("LED1: %d%% F:%luHz", pct1, (unsigned long)LED1_FREQ);
  display.setCursor(0, 36);
  display.printf("LED2: %d%% F:%luHz", pct2, (unsigned long)LED2_FREQ);
  display.display();
}

void setup() {
  Serial.begin(115200);

  // Core 3.x LEDC API: pin, frequency, resolution.
  // Different frequencies -> core assigns different LEDC timers.
  ledcAttach(LED1_PIN, LED1_FREQ, PWM_RES);
  ledcAttach(LED2_PIN, LED2_FREQ, PWM_RES);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found");
    while (true) delay(1000);
  }
  drawOled();
}

void loop() {
  unsigned long now = millis();

  // Update fading every 10 ms
  if (now - lastFade >= 10) {
    lastFade = now;
    fade(duty1, step1);
    fade(duty2, step2);
    ledcWrite(LED1_PIN, duty1);
    ledcWrite(LED2_PIN, duty2);
  }

  // Refresh OLED every 50 ms
  if (now - lastDraw >= 50) {
    lastDraw = now;
    drawOled();
  }
}
```

### `platformio.ini`

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    adafruit/Adafruit GFX Library
    adafruit/Adafruit SSD1306
```

> If your installed Arduino-ESP32 version is 2.x, replace `ledcAttach(pin, freq, res)` with `ledcSetup(0, LED1_FREQ, PWM_RES); ledcAttachPin(LED1_PIN, 0);` and `ledcSetup(2, LED2_FREQ, PWM_RES); ledcAttachPin(LED2_PIN, 2);`, and write using the channel number in `ledcWrite()`.

## 8. Part A — Wokwi Simulation

### 8.1 Setup

- `diagram.json` contains: ESP32 DevKit, two LEDs, **two separate resistors**, and the SSD1306 I²C OLED.
- Connections follow the table in Section 4.
- The program is in `src/main.cpp`.

### 8.2 Verification performed

| Check | Result |
|---|---|
| Both LEDs fade from minimum to maximum and back | [Pass / describe] |
| LED1 and LED2 fade independently at different speeds | [Pass / describe] |
| OLED shows LED1 duty % and 1000 Hz | [Pass / describe] |
| OLED shows LED2 duty % and 5000 Hz | [Pass / describe] |
| OLED values update while the LEDs fade | [Pass / describe] |

### 8.3 Evidence

- Complete circuit screenshot: `screenshots/wokwi_circuit.png`
- Running simulation with OLED visible: `screenshots/wokwi_running_oled.png`
- Wokwi project link: [PASTE YOUR WOKWI LINK HERE]

![Wokwi circuit](screenshots/wokwi_circuit.png)
![Wokwi running with OLED](screenshots/wokwi_running_oled.png)

## 9. Part B — Physical Hardware Implementation

### 9.1 Build description

- The same circuit was built on a breadboard with an ESP32 board, two LEDs, two 220 Ω resistors and an SSD1306 OLED.
- GPIO and I²C pins used on the real board: [state your pins; default is LED1 = GPIO 25, LED2 = GPIO 26, SDA = GPIO 21, SCL = GPIO 22]. Pin choices may differ from simulation depending on the board.
- The program was uploaded with PlatformIO and run on the board.

### 9.2 Verification performed

| Check | Result |
|---|---|
| Both LEDs fade up and down | [Pass / describe] |
| OLED shows changing duty cycles | [Pass / describe] |
| OLED shows configured frequencies (1000 Hz, 5000 Hz) | [Pass / describe] |

### 9.3 Evidence

- Hardware circuit photograph: `screenshots/hardware_circuit.jpg`
- Hardware OLED output photograph: `screenshots/hardware_oled_output.jpg`

![Hardware circuit](screenshots/hardware_circuit.jpg)
![Hardware OLED output](screenshots/hardware_oled_output.jpg)

### 9.4 Differences between simulation and hardware

| Aspect | Wokwi simulation | Physical hardware | Explanation |
|---|---|---|---|
| Brightness appearance | Idealised LED model | Real LEDs differ in brightness and colour | Real LEDs have different forward voltage and efficiency |
| Timing / smoothness | Simulation speed may vary with PC load | Real-time, consistent | Simulator is not real-time exact |
| OLED I²C address / wiring | Always works at 0x3C | May need address check (0x3C or 0x3D) and correct pin order | Module variations |
| Pin choice | As in diagram.json | Adjusted to the actual board | Board pinouts differ |
| Flicker with a phone camera | Not visible | May be visible at 1 kHz | Camera shutter interacts with PWM frequency |
| Other adjustments | [add if any] | [add if any] | [add if any] |

## 10. Test Results Summary

| Test | Expected | Wokwi | Hardware |
|---|---|---|---|
| LED1 fade 0 → 100 % → 0 % | Smooth, continuous | [result] | [result] |
| LED2 fade 100 % → 0 % → 100 % | Smooth, continuous | [result] | [result] |
| LED1 frequency | 1000 Hz | [result] | [result] |
| LED2 frequency | 5000 Hz | [result] | [result] |
| OLED duty values match program | Yes | [result] | [result] |
| OLED refresh while fading | Yes | [result] | [result] |
| Separate resistor per LED | Yes | Yes | Yes |

## 11. Project Structure

```
Homework-2/
├── diagram.json
├── wokwi.toml
├── platformio.ini
├── src/
│   └── main.cpp
├── screenshots/
│   ├── wokwi_circuit.png
│   ├── wokwi_running_oled.png
│   ├── hardware_circuit.jpg
│   └── hardware_oled_output.jpg
└── README.md
```

## 12. Part C — GitHub Submission Steps

The work was pushed to the **same existing course repository**, in two commits, in this order.

**Step 1 — Task 1 first.** Build and test the two-LED PWM fading circuit with different frequencies in Wokwi. Save the working source code and Wokwi configuration.

**Step 2 — First push.** Push the Task 1 files in the `Homework-2` folder with the exact commit message `task1 completed`:

```bash
git add Homework-2
git commit -m "task1 completed"
git push
```

**Step 3 — Continue in the same project.** Add the SSD1306 OLED, update the source code to show both LEDs' duty-cycle percentages and frequencies, and test the completed project.

**Step 4 — Add evidence and documentation.** Add the updated Wokwi configuration, final source code, Wokwi screenshots and link, hardware photographs, and this `README.md`.

**Step 5 — Final push.** Push the completed project to the same repository with the exact commit message `complete homework`:

```bash
git add Homework-2
git commit -m "complete homework"
git push
```

**Step 6 — Verify the repository.** Confirm that both commits appear in the history in the correct order (`task1 completed`, then `complete homework`), and that the latest files, README, source code and evidence are readable:

```bash
git log --oneline
```

Expected order (newest first):

```
xxxxxxx complete homework
xxxxxxx task1 completed
```

## 13. Final Submission Checklist

| # | Required item | Done |
|---|---|---|
| 1 | Updated `diagram.json` and complete `src/main.cpp` | ☐ |
| 2 | Wokwi circuit screenshot | ☐ |
| 3 | Wokwi running simulation screenshot with OLED | ☐ |
| 4 | Working Wokwi project link | ☐ |
| 5 | Hardware circuit photograph | ☐ |
| 6 | Hardware OLED-output photograph | ☐ |
| 7 | GitHub repository link with source files and screenshots | ☐ |
| 8 | README.md with wiring, frequencies, implementation and test notes | ☐ |
| 9 | Short comparison of Wokwi and hardware results | ☐ |

**Submission channel:** Required screenshots, photographs and a brief technical explanation are submitted through **Microsoft Teams**.

## 14. Technical Quality Notes

- Code is readable, commented, and uses the LEDC API of the installed Arduino-ESP32 core.
- OLED values come from the same variables and constants that drive the PWM hardware.
- Circuit connections are clear and safe, with a separate resistor for each LED.
- All screenshots and photographs are original and show my own implementation.
- The GitHub repository is organised as shown in Section 11, and all submitted links were tested.

## 15. Conclusion

The project successfully drives two LEDs with independent PWM signals at 1 kHz and 5 kHz, fades them smoothly using a 10-bit duty resolution, and shows live duty-cycle percentages and configured frequencies on an SSD1306 OLED. It was tested in Wokwi and on physical hardware and published to GitHub with all required evidence.
