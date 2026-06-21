// SancaBot / ESP32-C3 SuperMini
// Prueba mínima: encender y apagar el LED embarcado
// LED embarcado: GPIO 8
#include <Arduino.h>

#define LED_PIN 8

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);  // encender LED
  delay(500);

  digitalWrite(LED_PIN, LOW);   // apagar LED
  delay(500);
}