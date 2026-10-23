#include <Arduino.h>

#define BUZZER_PIN 16
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8
#define PWM_INITIAL_FREQ 1000

// Generate a tone
void playTone(int frequency, int duration) {
  ledcWriteTone(PWM_CHANNEL, frequency);
  delay(duration);

  ledcWriteTone(PWM_CHANNEL, 0);
  delay(100);
}

// Pattern 1: Rising tones
void toneSequence() {
  playTone(300, 500);
  playTone(600, 500);
  playTone(1000, 500);
  playTone(1500, 500);
  playTone(2000, 500);
}

// Pattern 2: Alarm
void alarmSound() {
  for (int i = 0; i < 5; i++) {
    playTone(1000, 300);
    playTone(2000, 300);
  }
}

// Pattern 3: Doorbell
void doorbell() {
  playTone(1000, 300);
  playTone(1500, 600);
  delay(300);
  playTone(1000, 300);
  playTone(1500, 600);
}

// Pattern 4: Short beeps
void shortBeeps() {
  for (int i = 0; i < 5; i++) {
    playTone(2000, 100);
    delay(100);
  }
}

// Pattern 5: Simple melody
void melody() {
  playTone(523, 300);   // C5
  playTone(587, 300);   // D5
  playTone(659, 300);   // E5
  playTone(698, 300);   // F5
  playTone(784, 300);   // G5
  playTone(880, 300);   // A5
  playTone(988, 300);   // B5
  playTone(1047, 600);  // C6
}

void setup() {
  Serial.begin(115200);

  // Configure PWM using the old API
  ledcSetup(PWM_CHANNEL, PWM_INITIAL_FREQ, PWM_RESOLUTION);

  // Attach PWM channel to GPIO 16
  ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);

  Serial.println("PWM Buzzer Ready!");
}

void loop() {
  Serial.println("1. Rising Tone Sequence");
  toneSequence();
  delay(500);

  Serial.println("2. Alarm");
  alarmSound();
  delay(500);

  Serial.println("3. Doorbell");
  doorbell();
  delay(500);

  Serial.println("4. Short Beeps");
  shortBeeps();
  delay(500);

  Serial.println("5. Melody");
  melody();
  delay(2000);
}