// display.h
// Display wrapper.
// On Arduino targets it uses a small I2C OLED (SSD1306 128x64).
// On host tests it falls back to LiquidCrystal mock for compatibility.

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

#if defined(ARDUINO)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#else
#include <LiquidCrystal.h>
#endif

class DisplayInterface {
  private:
#if defined(ARDUINO)
    static const uint8_t OLED_WIDTH = 128;
    static const uint8_t OLED_HEIGHT = 64;
    static const int8_t OLED_RESET = -1;
  static const uint8_t OLED_I2C_ADDR_PRIMARY = 0x3C;
  static const uint8_t OLED_I2C_ADDR_SECONDARY = 0x3D;
    Adafruit_SSD1306 _oled;
  uint8_t _activeAddress;
#else
    LiquidCrystal _lcd;
#endif

  public:
#if defined(ARDUINO)
    DisplayInterface()
      : _oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET),
        _activeAddress(0) {}

    // Kept for API compatibility with older LCD-based call sites.
    DisplayInterface(int, int, int, int, int, int)
      : _oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET),
        _activeAddress(0) {}

    bool begin() {
      Wire.begin();

      if (_oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR_PRIMARY)) {
        _activeAddress = OLED_I2C_ADDR_PRIMARY;
      } else if (_oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR_SECONDARY)) {
        _activeAddress = OLED_I2C_ADDR_SECONDARY;
      } else {
        _activeAddress = 0;
        return false;
      }

      _oled.clearDisplay();
      _oled.setTextWrap(false);
      _oled.setTextColor(SSD1306_WHITE);
      _oled.setTextSize(1);
      _oled.display();
      return true;
    }

    void clear() { _oled.clearDisplay(); }
    void setCursor(uint8_t col, uint8_t row) { _oled.setCursor(col, row); }

    void setTextSize(uint8_t size) { _oled.setTextSize(size); }
    void drawBitmap(int16_t x, int16_t y,
                    const uint8_t* bitmap,
                    int16_t width, int16_t height) {
      _oled.drawBitmap(x, y, bitmap, width, height, SSD1306_WHITE);
    }

    void print(const char* text) { _oled.print(text); }
    void print(int value) { _oled.print(value); }
    void print(float value, int digits) { _oled.print(value, digits); }

    void display() { _oled.display(); }
  uint8_t address() const { return _activeAddress; }

#else
    DisplayInterface(int rs, int en, int d4, int d5, int d6, int d7)
      : _lcd(rs, en, d4, d5, d6, d7) {}

    void begin(uint8_t columns, uint8_t rows) { _lcd.begin(columns, rows); }
    void clear() { _lcd.clear(); }
    void setCursor(uint8_t col, uint8_t row) { _lcd.setCursor(col, row); }

    void print(const char* text) { _lcd.print(text); }
    void print(int value) { _lcd.print(value); }
    void print(float value, int digits) { _lcd.print(value, digits); }

    void setTextSize(uint8_t) {}
    void drawBitmap(int16_t, int16_t, const uint8_t*, int16_t, int16_t) {}
    void display() {}
    uint8_t address() const { return 0; }
  #endif
};

#endif // DISPLAY_H