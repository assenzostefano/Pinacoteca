/**
 * @file command_processor.h
 * @brief Text-based command parser and executor (zero heap allocation).
 *
 * Accepts SET/GET/ACT/SERVO commands via serial or Bluetooth
 * and translates them into actions on the Pinacoteca subsystems.
 * All parsing uses fixed-size stack buffers.
 */

#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

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

    uint8_t _greenPin;
    uint8_t _redPin;
    uint8_t _heatingPin;
    uint8_t _coolingPin;
    uint8_t _humidifierPin;
    uint8_t _ceilingLightsPin;

    bool _manualBypass;

    /// Maximum response buffer size
    static constexpr uint8_t RESP_MAX = 128;

    /// Drive an LED only if manual mode is active.
    void manualLed(uint8_t pin, bool state, const char* okResponse, char* out) {
      if (!_manualBypass) { strcpy(out, "ERR:MODE"); return; }
      led(pin, state);
      strcpy(out, okResponse);
    }

    /// Move the servo to an absolute angle.
    void moveServoAbsolute(int target) {
      if (_servo == nullptr) return;
      int delta = target - _servo->read();
      antiSufferingServo(delta, *_servo);
    }

    /// In-place uppercase + trim for a C-string.
    static void normalizeInPlace(char* s) {
      // Trim leading whitespace
      char* src = s;
      while (*src == ' ' || *src == '\t') ++src;

      // Uppercase + copy forward
      char* dst = s;
      while (*src) {
        *dst++ = static_cast<char>(toupper(static_cast<unsigned char>(*src++)));
      }
      *dst = '\0';

      // Trim trailing whitespace
      while (dst > s && (*(dst - 1) == ' ' || *(dst - 1) == '\t')) {
        --dst;
        *dst = '\0';
      }
    }

    /// Strict integer parser (rejects trailing garbage).
    static bool parseInt(const char* s, int& out) {
      if (s == nullptr || s[0] == '\0') return false;
      char* end = nullptr;
      long v = strtol(s, &end, 10);
      if (end == nullptr || *end != '\0') return false;
      if (v < INT_MIN || v > INT_MAX) return false;
      out = static_cast<int>(v);
      return true;
    }

    /// Strict float parser (digits and optional decimal point only).
    static bool parseFloat(const char* s, float& out) {
      if (s == nullptr || s[0] == '\0') return false;

      const char* p = s;
      if (*p == '+' || *p == '-') p++;

      bool hasDigit = false;
      while (*p && isdigit(static_cast<unsigned char>(*p))) { hasDigit = true; p++; }
      if (*p == '.') { p++; while (*p && isdigit(static_cast<unsigned char>(*p))) { hasDigit = true; p++; } }
      if (!hasDigit || *p != '\0') return false;

      out = static_cast<float>(atof(s));
      return true;
    }

    /// Check if str starts with prefix
    static bool startsWith(const char* str, const char* prefix) {
      return strncmp(str, prefix, strlen(prefix)) == 0;
    }

  public:
    CommandProcessor(
        Thermostat* thermostat,
        HumidifierControl* humidifier,
        LightingControl* lighting,
        Turnstile* turnstile,
        Servo* servo,
        uint8_t greenPin, uint8_t redPin,
        uint8_t heatingPin, uint8_t coolingPin,
        uint8_t humidifierPin, uint8_t ceilingLightsPin)
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

    /**
     * @brief Build the full state payload string into @p buf.
     * @param buf     Destination buffer (at least RESP_MAX bytes).
     * @param bufSize Size of @p buf.
     */
    void buildStatePayload(char* buf, uint8_t bufSize) const {
      char t[12], h[12], l[12], tt[12], th[12];

      dtostrf(_thermostat->getCurrentTemperature(), 0, 1, t);
      dtostrf(_humidifier->getCurrentHumidity(), 0, 1, h);
      dtostrf(_lighting->getCurrentLux(), 0, 0, l);
      dtostrf(_thermostat->getTargetTemperature(), 0, 1, tt);
      dtostrf(_humidifier->getTargetHumidity(), 0, 1, th);

      int servoAngle = (_servo != nullptr) ? _servo->read() : -1;
      const char* mode = _manualBypass ? "MANUAL" : "AUTO";

      snprintf(buf, bufSize,
               "STATE:T=%s;H=%s;L=%s;P=%d;S=%d;M=%s;TT=%s;TH=%s;TL=%d;PM=%d",
               t, h, l,
               _turnstile->getPeopleCount(),
               servoAngle, mode, tt, th,
               _lighting->getTargetLux(),
               _turnstile->getMaxPeople());
    }

    /**
     * @brief Process a text command and write the response into @p out.
     * @param command  Mutable command buffer (will be normalised in-place).
     * @param out      Response buffer (at least RESP_MAX bytes).
     */
    void processCommand(char* command, char* out) {
      normalizeInPlace(command);

      if (command[0] == '\0') { strcpy(out, "ERR:EMPTY"); return; }

      // Informational commands
      if (strcmp(command, "PING") == 0)      { strcpy(out, "PONG"); return; }
      if (strcmp(command, "VER") == 0)       { strcpy(out, "VER:1"); return; }
      if (strcmp(command, "GET:STATE") == 0) { buildStatePayload(out, RESP_MAX); return; }
      if (strcmp(command, "GET:MODE") == 0) {
        strcpy(out, _manualBypass ? "MODE:MANUAL" : "MODE:AUTO"); return;
      }

      // Mode switching
      if (strcmp(command, "MANUAL:ON") == 0)  { _manualBypass = true;  strcpy(out, "OK:MANUAL:ON");  return; }
      if (strcmp(command, "MANUAL:OFF") == 0) { _manualBypass = false; strcpy(out, "OK:MANUAL:OFF"); return; }

      // Setpoint commands
      if (startsWith(command, "SET:TEMP:")) {
        float v; if (!parseFloat(command + 9, v)) { strcpy(out, "ERR:FORMAT:TEMP"); return; }
        if (v < 15.0f || v > 30.0f) { strcpy(out, "ERR:RANGE:TEMP"); return; }
        _thermostat->setTargetTemperature(v); strcpy(out, "OK:TEMP"); return;
      }
      if (startsWith(command, "SET:HUM:")) {
        float v; if (!parseFloat(command + 8, v)) { strcpy(out, "ERR:FORMAT:HUM"); return; }
        if (v < 40.0f || v > 80.0f) { strcpy(out, "ERR:RANGE:HUM"); return; }
        _humidifier->setTargetHumidity(v); strcpy(out, "OK:HUM"); return;
      }
      if (startsWith(command, "SET:LUX:")) {
        int v; if (!parseInt(command + 8, v)) { strcpy(out, "ERR:FORMAT:LUX"); return; }
        if (v < 50 || v > 1200) { strcpy(out, "ERR:RANGE:LUX"); return; }
        _lighting->setTargetLux(v); strcpy(out, "OK:LUX"); return;
      }
      if (startsWith(command, "SET:PEOPLE:")) {
        int v; if (!parseInt(command + 11, v)) { strcpy(out, "ERR:FORMAT:PEOPLE"); return; }
        if (v < 1 || v > 255) { strcpy(out, "ERR:RANGE:PEOPLE"); return; }
        _turnstile->setMaxPeople(static_cast<uint8_t>(v)); strcpy(out, "OK:PEOPLE"); return;
      }

      // Servo commands (manual mode only)
      if (strcmp(command, "SERVO:OPEN") == 0) {
        if (!_manualBypass) { strcpy(out, "ERR:MODE"); return; }
        if (_servo == nullptr) { strcpy(out, "ERR:SERVO"); return; }
        moveServoAbsolute(90); strcpy(out, "OK:SERVO:OPEN"); return;
      }
      if (strcmp(command, "SERVO:CLOSE") == 0) {
        if (!_manualBypass) { strcpy(out, "ERR:MODE"); return; }
        if (_servo == nullptr) { strcpy(out, "ERR:SERVO"); return; }
        moveServoAbsolute(0); strcpy(out, "OK:SERVO:CLOSE"); return;
      }
      if (startsWith(command, "SERVO:ANGLE:")) {
        if (!_manualBypass) { strcpy(out, "ERR:MODE"); return; }
        if (_servo == nullptr) { strcpy(out, "ERR:SERVO"); return; }
        int a; if (!parseInt(command + 12, a)) { strcpy(out, "ERR:FORMAT:SERVO"); return; }
        if (a < 0 || a > 180) { strcpy(out, "ERR:RANGE:SERVO"); return; }
        moveServoAbsolute(a); strcpy(out, "OK:SERVO:ANGLE"); return;
      }

      // Actuator commands (manual mode only)
      if (strcmp(command, "ACT:GREEN:ON") == 0)       { manualLed(_greenPin, HIGH, "OK:GREEN:ON", out); return; }
      if (strcmp(command, "ACT:GREEN:OFF") == 0)      { manualLed(_greenPin, LOW, "OK:GREEN:OFF", out); return; }
      if (strcmp(command, "ACT:RED:ON") == 0)         { manualLed(_redPin, HIGH, "OK:RED:ON", out); return; }
      if (strcmp(command, "ACT:RED:OFF") == 0)        { manualLed(_redPin, LOW, "OK:RED:OFF", out); return; }
      if (strcmp(command, "ACT:HEAT:ON") == 0)        { manualLed(_heatingPin, HIGH, "OK:HEAT:ON", out); return; }
      if (strcmp(command, "ACT:HEAT:OFF") == 0)       { manualLed(_heatingPin, LOW, "OK:HEAT:OFF", out); return; }
      if (strcmp(command, "ACT:COOL:ON") == 0)        { manualLed(_coolingPin, HIGH, "OK:COOL:ON", out); return; }
      if (strcmp(command, "ACT:COOL:OFF") == 0)       { manualLed(_coolingPin, LOW, "OK:COOL:OFF", out); return; }
      if (strcmp(command, "ACT:HUMIDIFIER:ON") == 0)  { manualLed(_humidifierPin, HIGH, "OK:HUMIDIFIER:ON", out); return; }
      if (strcmp(command, "ACT:HUMIDIFIER:OFF") == 0) { manualLed(_humidifierPin, LOW, "OK:HUMIDIFIER:OFF", out); return; }

      if (startsWith(command, "ACT:PLAFONIERE:PWM:")) {
        if (!_manualBypass) { strcpy(out, "ERR:MODE"); return; }
        int p; if (!parseInt(command + 19, p)) { strcpy(out, "ERR:FORMAT:PWM"); return; }
        if (p < 0 || p > 255) { strcpy(out, "ERR:RANGE:PWM"); return; }
        ledDimming(_ceilingLightsPin, p);
        strcpy(out, "OK:PLAFONIERE:PWM"); return;
      }

      strcpy(out, "ERR:UNKNOWN");
    }
};

#endif // COMMAND_PROCESSOR_H
