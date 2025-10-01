#pragma once
#include <Arduino.h>

// Pin buzzer
#define BUZZER_PIN 25
#define BUZZER_CH  0  // channel PWM

inline void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
}

// Buzzer untuk sukses
inline void beepSuccess() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

// Buzzer untuk gagal
inline void beepFail() {
    ledcAttachPin(BUZZER_PIN, BUZZER_CH);
    ledcWriteNote(BUZZER_CH, NOTE_C, 7); // Nada A oktaf 5
    delay(200);
    ledcDetachPin(BUZZER_PIN);
    delay(100);
    ledcAttachPin(BUZZER_PIN, BUZZER_CH);
    ledcWriteNote(BUZZER_CH, NOTE_C, 7);
    delay(200);
    ledcDetachPin(BUZZER_PIN);
}

// Buzzer untuk waiting
inline void beepWaiting() {
    ledcAttachPin(BUZZER_PIN, BUZZER_CH);
    ledcWriteNote(BUZZER_CH, NOTE_E, 6); // Nada E oktaf 6
    delay(150);
    ledcDetachPin(BUZZER_PIN);
}
