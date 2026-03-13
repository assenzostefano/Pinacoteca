#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal.h>

class DisplayInterface {
    private:
        LiquidCrystal _lcd;

    public:
        DisplayInterface(int rs, int en, int d4, int d5, int d6, int d7)
            : _lcd(rs, en, d4, d5, d6, d7) {}

        void begin(uint8_t columns, uint8_t rows) {
            _lcd.begin(columns, rows);
        }

        void clear() {
            _lcd.clear();
        }

        void setCursor(uint8_t column, uint8_t row) {
            _lcd.setCursor(column, row);
        }

        void print(const char* text) {
            _lcd.print(text);
        }

        void print(int value) {
            _lcd.print(value);
        }

        void print(float value, int digits) {
            _lcd.print(value, digits);
        }
};

#endif