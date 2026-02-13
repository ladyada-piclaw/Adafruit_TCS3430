/*
 * TCS3430 Color Display Demo for ESP32-S2 TFT Feather
 *
 * Displays live XYZ+IR channel values, a color swatch, CIE chromaticity,
 * CCT, and lux on the built-in 1.14" TFT (240x135).
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license, check license.txt for more information
 */

#include <Adafruit_ST7789.h>
#include <Adafruit_TCS3430.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

Adafruit_TCS3430 tcs;
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);

const uint16_t SCREEN_W = 240;
const uint16_t SCREEN_H = 135;
const uint16_t TITLE_H = 17;

// Swatch on the right
const uint16_t SWATCH_SIZE = 55;
const uint16_t SWATCH_X = SCREEN_W - SWATCH_SIZE - 6;
const uint16_t SWATCH_Y = TITLE_H + 4;

// Convert CIE XYZ to approximate RGB565 for the color swatch
static uint16_t xyzToRGB565(float X, float Y, float Z) {
  float r = 3.2406f * X - 1.5372f * Y - 0.4986f * Z;
  float g = -0.9689f * X + 1.8758f * Y + 0.0415f * Z;
  float b = 0.0557f * X - 0.2040f * Y + 1.0570f * Z;

  if (r < 0)
    r = 0;
  if (g < 0)
    g = 0;
  if (b < 0)
    b = 0;

  float maxC = r;
  if (g > maxC)
    maxC = g;
  if (b > maxC)
    maxC = b;
  if (maxC > 0) {
    r /= maxC;
    g /= maxC;
    b /= maxC;
  }

  r = sqrtf(r);
  g = sqrtf(g);
  b = sqrtf(b);

  return ((uint8_t)(r * 255) & 0xF8) << 8 | ((uint8_t)(g * 255) & 0xFC) << 3 |
         (uint8_t)(b * 255) >> 3;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(100);

  display.init(135, 240);
  display.setRotation(3);

  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ST77XX_WHITE);

  if (!tcs.begin()) {
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setCursor(0, 40);
    canvas.setTextColor(ST77XX_RED);
    canvas.println(" TCS3430 not found!");
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
    while (1)
      delay(10);
  }

  tcs.setALSGain(TCS3430_GAIN_64X);
  tcs.setIntegrationTime(100.0f);

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
}

void loop() {
  uint16_t x, y, z, ir1;
  if (!tcs.getChannels(&x, &y, &z, &ir1)) {
    delay(500);
    return;
  }

  float fx = (float)x / 65535.0f;
  float fy = (float)y / 65535.0f;
  float fz = (float)z / 65535.0f;
  uint16_t swatchColor = xyzToRGB565(fx, fy, fz);

  canvas.fillScreen(ST77XX_BLACK);

  // Title
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(4, 13);
  canvas.print("Adafruit TCS3430 Demo");

  // Row 1: X & Y on one line
  uint16_t row1Y = TITLE_H + 28;
  uint16_t row2Y = row1Y + 24;
  canvas.setFont(&FreeSansBold12pt7b);

  canvas.setTextColor(ST77XX_RED);
  canvas.setCursor(6, row1Y);
  canvas.print("X:");
  canvas.print(x);

  canvas.setTextColor(ST77XX_GREEN);
  canvas.setCursor(90, row1Y);
  canvas.print("Y:");
  canvas.print(y);

  // Row 2: Z & IR on one line
  canvas.setTextColor(0x001F);
  canvas.setCursor(6, row2Y);
  canvas.print("Z:");
  canvas.print(z);

  canvas.setTextColor(0xC81F); // purple
  canvas.setCursor(90, row2Y);
  canvas.print("IR:");
  canvas.print(ir1);

  // Color swatch on the right
  canvas.drawRect(SWATCH_X - 1, SWATCH_Y - 1, SWATCH_SIZE + 2, SWATCH_SIZE + 2,
                  ST77XX_WHITE);
  canvas.fillRect(SWATCH_X, SWATCH_Y, SWATCH_SIZE, SWATCH_SIZE, swatchColor);

  // CIE / CCT / Lux — two lines at the bottom
  float cie_x = 0, cie_y = 0;
  if (tcs.getCIE(&cie_x, &cie_y)) {
    canvas.setFont(&FreeSansBold12pt7b);
    canvas.setTextColor(ST77XX_WHITE);
    canvas.setCursor(6, SCREEN_H - 36);
    canvas.print("CIE ");
    canvas.print(cie_x, 3);
    canvas.print(", ");
    canvas.print(cie_y, 3);

    canvas.setTextColor(ST77XX_YELLOW);
    canvas.setCursor(6, SCREEN_H - 14);
    canvas.print((int)tcs.getCCT());
    canvas.print(" K");

    canvas.setTextColor(ST77XX_WHITE);
    canvas.setCursor(120, SCREEN_H - 14);
    canvas.print(tcs.getLux(), 0);
    canvas.print(" lux");
  }

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
  delay(200);
}
