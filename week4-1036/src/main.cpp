// Week3-Lecture2
// Timer Interrupt (Internal)
// Embedded IoT System Fall-2026
// Name:Humna Gulzar
// Reg#: 24_NTU_CS_FL_1036

#include <Arduino.h>

#define LED 2

hw_timer_t *My_timer = NULL;

void IRAM_ATTR onTimer()
{
    digitalWrite(LED, !digitalRead(LED));
}

void setup()
{
    pinMode(LED, OUTPUT);

    // Timer 0, 80 MHz / 80 = 1 MHz
    // 1 tick = 1 microsecond
    My_timer = timerBegin(0, 80, true);

    // Attach interrupt
    timerAttachInterrupt(My_timer, &onTimer, true);

    // Generate interrupt every 1,000,000 microseconds = 1 second
    timerAlarmWrite(My_timer, 1000000, true);

    // Enable timer alarm
    timerAlarmEnable(My_timer);
}

void loop()
{
}