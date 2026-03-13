#ifndef LIQUIDCRYSTAL_H
#define LIQUIDCRYSTAL_H

#include <array>
#include <string>

class LiquidCrystal {
public:
    static LiquidCrystal* lastInstance;

private:
    int _cols = 16;
    int _rows = 2;
    int _cursorCol = 0;
    int _cursorRow = 0;
    std::array<std::string, 2> _buffer;

public:
    LiquidCrystal(int, int, int, int, int, int) {
        _buffer[0] = std::string(16, ' ');
        _buffer[1] = std::string(16, ' ');
        lastInstance = this;
    }

    void begin(int cols, int rows) {
        _cols = cols;
        _rows = rows;
        clear();
    }

    void clear() {
        for (auto& row : _buffer) row = std::string(_cols, ' ');
        _cursorCol = 0;
        _cursorRow = 0;
    }

    void setCursor(int col, int row) {
        _cursorCol = col;
        _cursorRow = row;
    }

    void print(const char* text) {
        if (!text) return;
        while (*text) {
            putChar(*text++);
        }
    }

    void print(int value) {
        print(std::to_string(value).c_str());
    }

    void print(float value, int digits) {
        std::string s = std::to_string(value);
        auto dot = s.find('.');
        if (dot != std::string::npos && dot + 1 + digits < s.size()) {
            s = s.substr(0, dot + 1 + digits);
        }
        print(s.c_str());
    }

    std::string line(int index) const {
        if (index < 0 || index >= _rows) return "";
        return _buffer[index];
    }

private:
    void putChar(char c) {
        if (_cursorRow < 0 || _cursorRow >= _rows) return;
        if (_cursorCol >= 0 && _cursorCol < _cols) {
            _buffer[_cursorRow][_cursorCol] = c;
        }
        _cursorCol++;
    }
};

inline LiquidCrystal* LiquidCrystal::lastInstance = nullptr;

#endif
