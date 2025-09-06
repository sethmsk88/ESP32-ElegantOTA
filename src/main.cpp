#include <Arduino.h>
#include "OTA.h"

void setup(void) {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  setupOTA();
}

void loop(void) {
  ElegantOTA.loop();

  static unsigned long counter = 0;
  static unsigned long lastCounterTime = 0;
  
  // Print counter every 2 seconds
  if (millis() - lastCounterTime >= 2000) {
    Serial.println(counter);
    counter++;
    lastCounterTime = millis();
  }

  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
}
