/*
 * TCS3430 Color Display Demo for ESP32-S2 TFT Feather
 *
 * Displays a live color swatch from XYZ tristimulus data,
 * channel values, CIE chromaticity, CCT, and lux on the
 * built-in 1.14" TFT (240x135).
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
const uint16_t LEFT_W = (SCREEN_W * 45) / 100;
const uint16_t RIGHT_W = SCREEN_W - LEFT_W;

const uint16_t SWATCH_MARGIN = 4;
const uint16_t SWATCH_X = SWATCH_MARGIN;
const uint16_t SWATCH_Y = TITLE_H + SWATCH_MARGIN;
const uint16_t SWATCH_W = LEFT_W - (SWATCH_MARGIN * 2);
const uint16_t SWATCH_H = SCREEN_H - TITLE_H - (SWATCH_MARGIN * 2);

// Convert CIE XYZ to approximate RGB565 for the color swatch
static uint16_t xyzToRGB565(float X, float Y, float Z) {
  // XYZ to linear sRGB (D65)
  float r = 3.2406f * X - 1.5372f * Y - 0.4986f * Z;
  float g = -0.9689f * X + 1.8758f * Y + 0.0415f * Z;
  float b = 0.0557f * X - 0.2040f * Y + 1.0570f * Z;

  if (r < 0)
    r = 0;
  if (g < 0)
    g = 0;
  if (b < 0)
    b = 0;

  // Normalize to max=1
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

  // Simple gamma (~2.2)
  r = sqrtf(r);
  g = sqrtf(g);
  b = sqrtf(b);

  uint8_t r8 = (uint8_t)(r * 255);
  uint8_t g8 = (uint8_t)(g * 255);
  uint8_t b8 = (uint8_t)(b * 255);

  return ((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Turn on the TFT / I2C power supply
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

  // Configure sensor
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

  // Compute color swatch from XYZ
  float fx = (float)x / 65535.0f;
  float fy = (float)y / 65535.0f;
  float fz = (float)z / 65535.0f;
  uint16_t swatchColor = xyzToRGB565(fx, fy, fz);

  canvas.fillScreen(ST77XX_BLACK);

  // Title bar
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(4, 13);
  canvas.print("Adafruit TCS3430 Demo");

  // Color swatch (left zone)
  canvas.drawRect(SWATCH_X - 1, SWATCH_Y - 1, SWATCH_W + 2, SWATCH_H + 2,
                  ST77XX_WHITE);
  canvas.fillRect(SWATCH_X, SWATCH_Y, SWATCH_W, SWATCH_H, swatchColor);

  // Channel readout (right zone)
  uint16_t textX = LEFT_W + 8;
  uint16_t textY = TITLE_H + 18;
  uint16_t lineStep = 18;
  canvas.setFont(&FreeSansBold12pt7b);

  canvas.setTextColor(ST77XX_RED);
  canvas.setCursor(textX, textY);
  canvas.print("X:");
  canvas.println(x);

  canvas.setTextColor(ST77XX_GREEN);
  canvas.setCursor(textX, textY + lineStep);
  canvas.print("Y:");
  canvas.println(y);

  canvas.setTextColor(0x001F); // blue
  canvas.setCursor(textX, textY + lineStep * 2);
  canvas.print("Z:");
  canvas.println(z);

  // CIE / CCT / Lux in smaller font below
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ST77XX_WHITE);

  float cie_x = 0, cie_y = 0;
  uint16_t infoY = textY + lineStep * 3 + 4;

  if (tcs.getCIE(&cie_x, &cie_y)) {
    canvas.setCursor(textX, infoY);
    canvas.print("CIE ");
    canvas.print(cie_x, 3);
    canvas.print(",");
    canvas.print(cie_y, 3);

    canvas.setCursor(textX, infoY + 16);
    canvas.print((int)tcs.getCCT());
    canvas.print("K  ");
    canvas.print(tcs.getLux(), 0);
    canvas.print(" lux");
  }

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
  delay(200);
}
