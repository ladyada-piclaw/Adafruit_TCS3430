/*!\
 * @file interrupt_test.ino
 *
 * Interrupt example for TCS3430 XYZ Tristimulus Color Sensor
 * Sets a high threshold on the Y channel — shine a flashlight on the
 * sensor to trigger the interrupt! Connect INT to pin 2 with INPUT_PULLUP.
 * Note: the Adafruit breakout has an open-drain inverter on INT,
 * so the interrupt is active-HIGH at the MCU.
 *
 * Limor 'ladyada' Fried with assistance from Claude Code
 * MIT License
 */

#include "Adafruit_TCS3430.h"

#define INT_PIN 2

Adafruit_TCS3430 tcs = Adafruit_TCS3430();

volatile bool intFired = false;

void intHandler() {
  intFired = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TCS3430 Interrupt Test"));
  Serial.println(F("Connect sensor INT to pin 2"));
  Serial.println();

  if (!tcs.begin()) {
    Serial.println(F("Failed to find TCS3430 chip"));
    while (1) {
      delay(10);
    }
  }

  Serial.println(F("TCS3430 found!"));

  // Setup sensor
  tcs.setALSGain(TCS3430_GAIN_16X);
  tcs.setIntegrationTime(100.0f);

  // Read current ambient level to set threshold above it
  delay(200);
  uint16_t x, y, z, ir1;
  tcs.getChannels(&x, &y, &z, &ir1);

  uint16_t threshold = y + (y / 2); // 50% above current ambient
  if (threshold < y + 100) {
    threshold = y + 100; // at least 100 above
  }
  if (threshold > 65000) {
    threshold = 65000;
  }

  Serial.print(F("Current ambient Y: "));
  Serial.println(y);
  Serial.print(F("Interrupt threshold: "));
  Serial.println(threshold);
  Serial.println();
  Serial.println(F("Shine a flashlight on the sensor to trigger!"));
  Serial.println();

  // Threshold window: low=0, high=threshold
  // Interrupt fires when Y goes ABOVE threshold (outside window)
  tcs.setALSThresholdLow(0);
  tcs.setALSThresholdHigh(threshold);
  tcs.setInterruptPersistence(TCS3430_PERS_EVERY);
  tcs.setInterruptClearOnRead(false);

  // Enable ALS interrupt on the INT pin
  tcs.enableALSInt(true);
  tcs.ALSEnable(false);
  tcs.clearALSInterrupt();
  tcs.ALSEnable(true);

  // Attach hardware interrupt — active HIGH due to breakout inverter
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), intHandler, RISING);
}

void loop() {
  if (intFired) {
    intFired = false;

    uint16_t x, y, z, ir1;
    tcs.getChannels(&x, &y, &z, &ir1);

    Serial.print(F("*** INTERRUPT! *** Y="));
    Serial.print(y);
    Serial.print(F("  X="));
    Serial.print(x);
    Serial.print(F("  Z="));
    Serial.print(z);
    Serial.print(F("  IR1="));
    Serial.println(ir1);

    // Clear interrupt
    tcs.ALSEnable(false);
    tcs.clearALSInterrupt();
    tcs.ALSEnable(true);
  }

  // Also print periodic readings so user can see ambient level
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();

    uint16_t x, y, z, ir1;
    tcs.getChannels(&x, &y, &z, &ir1);
    Serial.print(F("Y="));
    Serial.print(y);
    Serial.print(F("  (waiting for interrupt)"));

    if (digitalRead(INT_PIN) == HIGH) {
      Serial.print(F("  [INT pin HIGH]"));
    }
    Serial.println();
  }
}
