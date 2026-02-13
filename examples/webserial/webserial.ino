/*!
 * @file webserial.ino
 *
 * This example reads color data from the TCS3430 sensor and outputs it
 * in a format suitable for displaying on a web page using Web Serial API.
 *
 * It continuously measures CIE x,y coordinates, lux, and color temperature.
 *
 * This sketch works with the OPT4048 CIE Color Plotter web interface:
 * https://adafruit.github.io/Adafruit_OPT4048/webserial/
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license, check license.txt for more information
 */

#include "Adafruit_TCS3430.h"

Adafruit_TCS3430 tcs = Adafruit_TCS3430();

// Set how often to read data (in milliseconds)
const unsigned long READ_INTERVAL = 100;
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println(F("Adafruit TCS3430 WebSerial Example"));
  Serial.println(
      F("This sketch works with the OPT4048 CIE Color Plotter web page"));

  if (!tcs.begin()) {
    Serial.println(F("Failed to find TCS3430 chip"));
    while (1) {
      delay(10);
    }
  }

  Serial.println(F("TCS3430 sensor found!"));

  // Configure sensor
  tcs.setALSGain(TCS3430_GAIN_64X);
  tcs.setIntegrationTime(100.0f);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;

    float CIEx, CIEy;
    if (tcs.getCIE(&CIEx, &CIEy)) {
      float lux = tcs.getLux();
      float colorTemp = tcs.getCCT();

      // Print in the same format as OPT4048 for web page compatibility
      Serial.println(F("---CIE Data---"));
      Serial.print(F("CIE x: "));
      Serial.println(CIEx, 8);
      Serial.print(F("CIE y: "));
      Serial.println(CIEy, 8);
      Serial.print(F("Lux: "));
      Serial.println(lux, 4);
      Serial.print(F("Color Temperature: "));
      Serial.print(colorTemp, 2);
      Serial.println(F(" K"));
      Serial.println(F("-------------"));
    } else {
      Serial.println(F("Error reading sensor data"));
    }
  }

  // Check for any incoming serial commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
  }
}
