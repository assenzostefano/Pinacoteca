#ifndef ARDUINO_H
#define ARDUINO_H

#include <cmath>
#include <cstdint>
#include <cstdio>
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

inline void resetArduinoMocks() {
    __mock_millis = 0;
    __mock_pin_mode.clear();
    __mock_digital_read.clear();
    __mock_digital_write.clear();
    __mock_analog_read.clear();
    __mock_analog_write.clear();
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

inline long constrain(long x, long a, long b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

class MockSerial {
public:
    std::vector<std::string> logs;

    void begin(unsigned long) {}

    void println(const char* msg) {
        logs.emplace_back(msg ? msg : "");
    }
    void println(const std::string& msg) {
        logs.push_back(msg);
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
    void print(int value) {
        logs.push_back(std::to_string(value));
    }
    void print(float value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", value);
        logs.emplace_back(buf);
    }

    void clear() { logs.clear(); }
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
