# Homework 2 — Dual-LED PWM Control with OLED Monitoring

**Course:** Embedded IoT Systems
**Student:** [Your Name] — [Roll No.] — BSCS Section 5A, Fall 2026
**Platform:** ESP32 (Arduino-ESP32 core 3.x, PlatformIO) · Wokwi simulation · Physical ESP32 kit
**Wokwi project link:** [PASTE YOUR WOKWI LINK HERE]
**GitHub repository:** [PASTE YOUR REPOSITORY LINK HERE]

---

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


# Evidence


- Running simulation with OLED visible: `screenshots/wokwi_running_oled.png`
- Wokwi project link:

https://wokwi.com/projects/new/esp32
https://wokwi.com/projects/new/esp32




## Part B — Physical Hardware Implementation

###  Build description

- The same circuit was built on a breadboard with an ESP32 board, two LEDs, two 220 Ω resistors and an SSD1306 OLED.
- GPIO and I²C pins used on the real board: [state your pins; default is LED1 = GPIO 25, LED2 = GPIO 26, SDA = GPIO 21, SCL = GPIO 22]. Pin choices may differ from simulation depending on the board.
- The program was uploaded with PlatformIO and run on the board.

###  Verification performed

| Check | Result |
|---|---|
| Both LEDs fade up and down | [Pass / describe] |
| OLED shows changing duty cycles | [Pass / describe] |
| OLED shows configured frequencies (1000 Hz, 5000 Hz) | [Pass / describe] |




###  Differences between simulation and hardware

| Aspect | Wokwi simulation | Physical hardware | Explanation |
|---|---|---|---|
| Brightness appearance | Idealised LED model | Real LEDs differ in brightness and colour | Real LEDs have different forward voltage and efficiency |
| Timing / smoothness | Simulation speed may vary with PC load | Real-time, consistent | Simulator is not real-time exact |
| OLED I²C address / wiring | Always works at 0x3C | May need address check (0x3C or 0x3D) and correct pin order | Module variations |
| Pin choice | As in diagram.json | Adjusted to the actual board | Board pinouts differ |
| Flicker with a phone camera | Not visible | May be visible at 1 kHz | Camera shutter interacts with PWM frequency |
| Other adjustments | [add if any] | [add if any] | [add if any] |


