// command_processor.h
// Text-based command parser and executor.
// Accepts SET/GET/ACT/SERVO commands via serial or Bluetooth
// and translates them into actions on the Pinacoteca subsystems.

#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "../servomotor/turnstile.h"
#include "../servomotor/servomotor.h"
#include "../temperature/thermostat.h"
#include "../humidity/humidifier.h"
#include "../lighting/lighting_control.h"
#include "../led/led.h"

class CommandProcessor {
  private:
    Thermostat* _thermostat;
    HumidifierControl* _humidifier;
    LightingControl* _lighting;
    Turnstile* _turnstile;
    Servo* _servo;

    int _greenPin;
    int _redPin;
    int _heatingPin;
    int _coolingPin;
    int _humidifierPin;
    int _ceilingLightsPin;

    bool _manualBypass;

    // Drive an LED only if manual mode is active.
    String manualLed(int pin, bool state, const char* okResponse) {
      if (!_manualBypass) return "ERR:MODE";
      led(pin, state);
      return okResponse;
    }

    // Move the servo to an absolute angle.
    void moveServoAbsolute(int target) {
      if (_servo == nullptr) return;
      int delta = target - _servo->read();
      antiSufferingServo(delta, *_servo);
    }

    // Strict integer parser (rejects trailing garbage).
    static bool parseInt(const String& raw, int& out) {
      const char* s = raw.c_str();
      if (s == nullptr || s[0] == '\0') return false;
      char* end = nullptr;
      long v = strtol(s, &end, 10);
      if (end == nullptr || *end != '\0') return false;
      if (v < INT_MIN || v > INT_MAX) return false;
      out = static_cast<int>(v);
      return true;
    }

    // Strict float parser (digits and optional decimal point only).
    static bool parseFloat(const String& raw, float& out) {
      const char* s = raw.c_str();
      if (s == nullptr || s[0] == '\0') return false;

      size_t i = 0;
      if (s[i] == '+' || s[i] == '-') i++;

      bool hasDigit = false;
      while (s[i] && isdigit(static_cast<unsigned char>(s[i]))) { hasDigit = true; i++; }
      if (s[i] == '.') { i++; while (s[i] && isdigit(static_cast<unsigned char>(s[i]))) { hasDigit = true; i++; } }
      if (!hasDigit || s[i] != '\0') return false;

      out = raw.toFloat();
      return true;
    }

  public:
    CommandProcessor(
        Thermostat* thermostat,
        HumidifierControl* humidifier,
        LightingControl* lighting,
        Turnstile* turnstile,
        Servo* servo,
        int greenPin, int redPin,
        int heatingPin, int coolingPin,
        int humidifierPin, int ceilingLightsPin)
      : _thermostat(thermostat),
        _humidifier(humidifier),
        _lighting(lighting),
        _turnstile(turnstile),
        _servo(servo),
        _greenPin(greenPin),
        _redPin(redPin),
        _heatingPin(heatingPin),
        _coolingPin(coolingPin),
        _humidifierPin(humidifierPin),
        _ceilingLightsPin(ceilingLightsPin),
        _manualBypass(false) {}

    bool isManualBypassEnabled() const { return _manualBypass; }

    // Build the full state payload string.
    String buildStatePayload() const {
      char t[12], h[12], l[12], tt[12], th[12];

      dtostrf(_thermostat->getCurrentTemperature(), 0, 1, t);
      dtostrf(_humidifier->getCurrentHumidity(), 0, 1, h);
      dtostrf(_lighting->getCurrentLux(), 0, 0, l);
      dtostrf(_thermostat->getTargetTemperature(), 0, 1, tt);
      dtostrf(_humidifier->getTargetHumidity(), 0, 1, th);

      int servoAngle = (_servo != nullptr) ? _servo->read() : -1;
      const char* mode = _manualBypass ? "MANUAL" : "AUTO";

      char payload[128];
      snprintf(payload, sizeof(payload),
               "STATE:T=%s;H=%s;L=%s;P=%d;S=%d;M=%s;TT=%s;TH=%s;TL=%d",
               t, h, l,
               _turnstile->getPeopleCount(),
               servoAngle, mode, tt, th,
               _lighting->getTargetLux());

      return String(payload);
    }

    // Process a text command and return the response.
    String processCommand(String command) {
      command.trim();
      command.toUpperCase();

      if (command.length() == 0) return "ERR:EMPTY";

      // Informational commands.
      if (command == "PING")      return "PONG";
      if (command == "VER")       return "VER:1";
      if (command == "GET:STATE") return buildStatePayload();
      if (command == "GET:MODE")  return _manualBypass ? "MODE:MANUAL" : "MODE:AUTO";

      // Mode switching.
      if (command == "MANUAL:ON")  { _manualBypass = true;  return "OK:MANUAL:ON"; }
      if (command == "MANUAL:OFF") { _manualBypass = false; return "OK:MANUAL:OFF"; }

      // Setpoint commands.
      if (command.startsWith("SET:TEMP:")) {
        float v; if (!parseFloat(command.substring(9), v)) return "ERR:FORMAT:TEMP";
        if (v < 15.0 || v > 30.0) return "ERR:RANGE:TEMP";
        _thermostat->setTargetTemperature(v); return "OK:TEMP";
      }
      if (command.startsWith("SET:HUM:")) {
        float v; if (!parseFloat(command.substring(8), v)) return "ERR:FORMAT:HUM";
        if (v < 40.0 || v > 80.0) return "ERR:RANGE:HUM";
        _humidifier->setTargetHumidity(v); return "OK:HUM";
      }
      if (command.startsWith("SET:LUX:")) {
        int v; if (!parseInt(command.substring(8), v)) return "ERR:FORMAT:LUX";
        if (v < 50 || v > 1200) return "ERR:RANGE:LUX";
        _lighting->setTargetLux(v); return "OK:LUX";
      }
      if (command.startsWith("SET:PEOPLE:")) {
        int v; if (!parseInt(command.substring(11), v)) return "ERR:FORMAT:PEOPLE";
        if (v < 0 || v > _turnstile->getMaxPeople()) return "ERR:RANGE:PEOPLE";
        _turnstile->setPeopleCount(v); return "OK:PEOPLE";
      }

      // Servo commands (manual mode only).
      if (command == "SERVO:OPEN") {
        if (!_manualBypass) return "ERR:MODE";
        if (_servo == nullptr) return "ERR:SERVO";
        moveServoAbsolute(90); return "OK:SERVO:OPEN";
      }
      if (command == "SERVO:CLOSE") {
        if (!_manualBypass) return "ERR:MODE";
        if (_servo == nullptr) return "ERR:SERVO";
        moveServoAbsolute(0); return "OK:SERVO:CLOSE";
      }
      if (command.startsWith("SERVO:ANGLE:")) {
        if (!_manualBypass) return "ERR:MODE";
        if (_servo == nullptr) return "ERR:SERVO";
        int a; if (!parseInt(command.substring(12), a)) return "ERR:FORMAT:SERVO";
        if (a < 0 || a > 180) return "ERR:RANGE:SERVO";
        moveServoAbsolute(a); return "OK:SERVO:ANGLE";
      }

      // Actuator commands (manual mode only).
      if (command == "ACT:GREEN:ON")       return manualLed(_greenPin, HIGH, "OK:GREEN:ON");
      if (command == "ACT:GREEN:OFF")      return manualLed(_greenPin, LOW, "OK:GREEN:OFF");
      if (command == "ACT:RED:ON")         return manualLed(_redPin, HIGH, "OK:RED:ON");
      if (command == "ACT:RED:OFF")        return manualLed(_redPin, LOW, "OK:RED:OFF");
      if (command == "ACT:HEAT:ON")        return manualLed(_heatingPin, HIGH, "OK:HEAT:ON");
      if (command == "ACT:HEAT:OFF")       return manualLed(_heatingPin, LOW, "OK:HEAT:OFF");
      if (command == "ACT:COOL:ON")        return manualLed(_coolingPin, HIGH, "OK:COOL:ON");
      if (command == "ACT:COOL:OFF")       return manualLed(_coolingPin, LOW, "OK:COOL:OFF");
      if (command == "ACT:HUMIDIFIER:ON")  return manualLed(_humidifierPin, HIGH, "OK:HUMIDIFIER:ON");
      if (command == "ACT:HUMIDIFIER:OFF") return manualLed(_humidifierPin, LOW, "OK:HUMIDIFIER:OFF");

      if (command.startsWith("ACT:PLAFONIERE:PWM:")) {
        if (!_manualBypass) return "ERR:MODE";
        int p; if (!parseInt(command.substring(19), p)) return "ERR:FORMAT:PWM";
        if (p < 0 || p > 255) return "ERR:RANGE:PWM";
        ledDimming(_ceilingLightsPin, p);
        return "OK:PLAFONIERE:PWM";
      }

      return "ERR:UNKNOWN";
    }
};

#endif // COMMAND_PROCESSOR_H
