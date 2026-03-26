#ifndef ARDUINO_H
#define ARDUINO_H

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

using byte = uint8_t;

#ifndef HIGH
#define HIGH 0x1
#endif
#ifndef LOW
#define LOW 0x0
#endif
#ifndef INPUT
#define INPUT 0x0
#endif
#ifndef OUTPUT
#define OUTPUT 0x1
#endif

#ifndef A0
#define A0 14
#endif
#ifndef A1
#define A1 15
#endif
#ifndef A2
#define A2 16
#endif
#ifndef A3
#define A3 17
#endif
#ifndef A4
#define A4 18
#endif
#ifndef A5
#define A5 19
#endif

inline unsigned long __mock_millis = 0;
inline std::unordered_map<int, int> __mock_pin_mode;
inline std::unordered_map<int, int> __mock_digital_read;
inline std::unordered_map<int, int> __mock_digital_write;
inline std::unordered_map<int, int> __mock_analog_read;
inline std::unordered_map<int, int> __mock_analog_write;
inline std::unordered_map<int, unsigned long> __mock_pulse_in;

inline void resetArduinoMocks() {
    __mock_millis = 0;
    __mock_pin_mode.clear();
    __mock_digital_read.clear();
    __mock_digital_write.clear();
    __mock_analog_read.clear();
    __mock_analog_write.clear();
    __mock_pulse_in.clear();
}

inline void pinMode(int pin, int mode) { __mock_pin_mode[pin] = mode; }
inline int digitalRead(int pin) {
    auto it = __mock_digital_read.find(pin);
    return it == __mock_digital_read.end() ? LOW : it->second;
}
inline void digitalWrite(int pin, int value) { __mock_digital_write[pin] = value; }
inline int analogRead(int pin) {
    auto it = __mock_analog_read.find(pin);
    return it == __mock_analog_read.end() ? 0 : it->second;
}
inline void analogWrite(int pin, int value) { __mock_analog_write[pin] = value; }
inline unsigned long millis() { return __mock_millis; }
inline void delay(unsigned long ms) { __mock_millis += ms; }
inline void delayMicroseconds(unsigned int) {}
inline unsigned long pulseIn(int pin, int, unsigned long = 1000000UL) {
    auto it = __mock_pulse_in.find(pin);
    return it == __mock_pulse_in.end() ? 0UL : it->second;
}

#ifndef constrain
#define constrain(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#endif

class String {
private:
    std::string _value;

public:
    String() = default;
    String(const char* s) : _value(s ? s : "") {}
    String(const std::string& s) : _value(s) {}
    String(char c) : _value(1, c) {}
    String(int v) : _value(std::to_string(v)) {}
    String(unsigned long v) : _value(std::to_string(v)) {}
    String(float v, unsigned char decimals = 2) {
        char fmt[16];
        char out[64];
        std::snprintf(fmt, sizeof(fmt), "%%.%df", decimals);
        std::snprintf(out, sizeof(out), fmt, static_cast<double>(v));
        _value = out;
    }

    size_t length() const { return _value.length(); }
    void reserve(size_t) {}
    const char* c_str() const { return _value.c_str(); }

    char operator[](size_t i) const { return _value[i]; }

    String& operator+=(const String& other) {
        _value += other._value;
        return *this;
    }

    String& operator+=(const char* s) {
        _value += (s ? s : "");
        return *this;
    }

    String& operator+=(char c) {
        _value += c;
        return *this;
    }

    bool operator==(const char* s) const {
        return _value == (s ? s : "");
    }

    bool operator==(const String& other) const {
        return _value == other._value;
    }

    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        std::string p(prefix);
        return _value.rfind(p, 0) == 0;
    }

    String substring(size_t start) const {
        if (start >= _value.length()) return String("");
        return String(_value.substr(start));
    }

    String substring(size_t start, size_t end) const {
        if (start >= _value.length() || end <= start) return String("");
        size_t len = std::min(end, _value.length()) - start;
        return String(_value.substr(start, len));
    }

    int toInt() const {
        try {
            return std::stoi(_value);
        } catch (...) {
            return 0;
        }
    }

    float toFloat() const {
        try {
            return std::stof(_value);
        } catch (...) {
            return 0.0f;
        }
    }

    void trim() {
        size_t start = 0;
        while (start < _value.length() && std::isspace(static_cast<unsigned char>(_value[start]))) {
            ++start;
        }

        size_t end = _value.length();
        while (end > start && std::isspace(static_cast<unsigned char>(_value[end - 1]))) {
            --end;
        }

        _value = _value.substr(start, end - start);
    }

    void toUpperCase() {
        for (char& c : _value) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }

    std::string std() const { return _value; }
};

class MockSerial {
public:
    std::vector<std::string> logs;
    std::string inputBuffer;
    size_t inputCursor = 0;

    void begin(unsigned long) {}

    void println(const char* msg) {
        logs.emplace_back(msg ? msg : "");
    }
    void println(const std::string& msg) {
        logs.push_back(msg);
    }
    void println(const String& msg) {
        logs.push_back(msg.std());
    }
    void println(int value) {
        logs.push_back(std::to_string(value));
    }
    void println(float value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", value);
        logs.emplace_back(buf);
    }
    void print(const char* msg) {
        logs.emplace_back(msg ? msg : "");
    }
    void print(const String& msg) {
        logs.push_back(msg.std());
    }
    void print(int value) {
        logs.push_back(std::to_string(value));
    }
    void print(float value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", value);
        logs.emplace_back(buf);
    }

    int available() const {
        return static_cast<int>(inputBuffer.size() - inputCursor);
    }

    int read() {
        if (inputCursor >= inputBuffer.size()) {
            return -1;
        }

        return static_cast<unsigned char>(inputBuffer[inputCursor++]);
    }

    void injectInput(const std::string& data) {
        inputBuffer += data;
    }

    void clear() {
        logs.clear();
        inputBuffer.clear();
        inputCursor = 0;
    }
};

inline MockSerial Serial;

inline char* dtostrf(double val, signed char width, unsigned char prec, char* s) {
    char format[16];
    std::snprintf(format, sizeof(format), "%%%d.%df", width, prec);
    std::snprintf(s, 32, format, val);
    return s;
}

inline bool isnan(float v) {
    return std::isnan(v);
}

#endif
