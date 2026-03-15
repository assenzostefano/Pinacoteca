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
        int _plafonierePin;

        bool _manualBypass;

        bool parseIntStrict(const String& raw, int& out) const {
            const char* str = raw.c_str();
            if (str == nullptr || str[0] == '\0') {
                return false;
            }

            char* end = nullptr;
            long value = strtol(str, &end, 10);
            if (end == nullptr || *end != '\0') {
                return false;
            }

            if (value < INT_MIN || value > INT_MAX) {
                return false;
            }

            out = static_cast<int>(value);
            return true;
        }

        bool parseFloatStrict(const String& raw, float& out) const {
            const char* str = raw.c_str();
            if (str == nullptr || str[0] == '\0') {
                return false;
            }

            size_t i = 0;
            if (str[i] == '+' || str[i] == '-') {
                i++;
            }

            bool hasDigit = false;
            while (str[i] != '\0' && isdigit(static_cast<unsigned char>(str[i]))) {
                hasDigit = true;
                i++;
            }

            if (str[i] == '.') {
                i++;
                while (str[i] != '\0' && isdigit(static_cast<unsigned char>(str[i]))) {
                    hasDigit = true;
                    i++;
                }
            }

            if (!hasDigit || str[i] != '\0') {
                return false;
            }

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
            int greenPin,
            int redPin,
            int heatingPin,
            int coolingPin,
            int humidifierPin,
            int plafonierePin
        )
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
              _plafonierePin(plafonierePin),
              _manualBypass(false) {}

        bool isManualBypassEnabled() const {
            return _manualBypass;
        }

        String buildStatePayload() const {
            char t[12];
            char h[12];
            char l[12];
            char tt[12];
            char th[12];

            dtostrf(_thermostat->getCurrentTemperature(), 0, 1, t);
            dtostrf(_humidifier->getCurrentHumidity(), 0, 1, h);
            dtostrf(_lighting->getCurrentLux(), 0, 0, l);
            dtostrf(_thermostat->getTargetTemperature(), 0, 1, tt);
            dtostrf(_humidifier->getTargetHumidity(), 0, 1, th);

            int servoAngle = (_servo != nullptr) ? _servo->read() : -1;
            const char* mode = _manualBypass ? "MANUAL" : "AUTO";

            char payload[128];
            snprintf(
                payload,
                sizeof(payload),
                "STATE:T=%s;H=%s;L=%s;P=%d;S=%d;M=%s;TT=%s;TH=%s;TL=%d",
                t,
                h,
                l,
                _turnstile->getPeopleCount(),
                servoAngle,
                mode,
                tt,
                th,
                _lighting->getTargetLux()
            );

            return String(payload);
        }

        String processCommand(String command) {
            command.trim();
            command.toUpperCase();

            if (command.length() == 0) {
                return "ERR:EMPTY";
            }

            if (command == "PING") return "PONG";
            if (command == "VER") return "VER:1";
            if (command == "GET:STATE") return buildStatePayload();
            if (command == "GET:MODE") return _manualBypass ? "MODE:MANUAL" : "MODE:AUTO";
            if (command == "MANUAL:ON") {
                _manualBypass = true;
                return "OK:MANUAL:ON";
            }
            if (command == "MANUAL:OFF") {
                _manualBypass = false;
                return "OK:MANUAL:OFF";
            }

            if (command.startsWith("SET:TEMP:")) {
                float value = 0.0f;
                if (!parseFloatStrict(command.substring(9), value)) return "ERR:FORMAT:TEMP";
                if (value < 15.0f || value > 30.0f) return "ERR:RANGE:TEMP";
                _thermostat->setTargetTemperature(value);
                return "OK:TEMP";
            }

            if (command.startsWith("SET:HUM:")) {
                float value = 0.0f;
                if (!parseFloatStrict(command.substring(8), value)) return "ERR:FORMAT:HUM";
                if (value < 40.0f || value > 80.0f) return "ERR:RANGE:HUM";
                _humidifier->setTargetHumidity(value);
                return "OK:HUM";
            }

            if (command.startsWith("SET:LUX:")) {
                int value = 0;
                if (!parseIntStrict(command.substring(8), value)) return "ERR:FORMAT:LUX";
                if (value < 50 || value > 1200) return "ERR:RANGE:LUX";
                _lighting->setTargetLux(value);
                return "OK:LUX";
            }

            if (command.startsWith("SET:PEOPLE:")) {
                int value = 0;
                if (!parseIntStrict(command.substring(11), value)) return "ERR:FORMAT:PEOPLE";
                if (value < 0 || value > _turnstile->getMaxPeople()) return "ERR:RANGE:PEOPLE";
                _turnstile->setPeopleCount(value);
                return "OK:PEOPLE";
            }

            if (command == "SERVO:OPEN") {
                if (!_manualBypass) return "ERR:MODE";
                if (_servo == nullptr) return "ERR:SERVO";
                moveServoAbsolute(90);
                return "OK:SERVO:OPEN";
            }

            if (command == "SERVO:CLOSE") {
                if (!_manualBypass) return "ERR:MODE";
                if (_servo == nullptr) return "ERR:SERVO";
                moveServoAbsolute(0);
                return "OK:SERVO:CLOSE";
            }

            if (command.startsWith("SERVO:ANGLE:")) {
                if (!_manualBypass) return "ERR:MODE";
                if (_servo == nullptr) return "ERR:SERVO";

                int angle = 0;
                if (!parseIntStrict(command.substring(12), angle)) return "ERR:FORMAT:SERVO";
                if (angle < 0 || angle > 180) return "ERR:RANGE:SERVO";

                moveServoAbsolute(angle);
                return "OK:SERVO:ANGLE";
            }

            if (command == "ACT:GREEN:ON") {
                if (!_manualBypass) return "ERR:MODE";
                led(_greenPin, HIGH);
                return "OK:GREEN:ON";
            }

            if (command == "ACT:GREEN:OFF") {
                if (!_manualBypass) return "ERR:MODE";
                led(_greenPin, LOW);
                return "OK:GREEN:OFF";
            }

            if (command == "ACT:RED:ON") {
                if (!_manualBypass) return "ERR:MODE";
                led(_redPin, HIGH);
                return "OK:RED:ON";
            }

            if (command == "ACT:RED:OFF") {
                if (!_manualBypass) return "ERR:MODE";
                led(_redPin, LOW);
                return "OK:RED:OFF";
            }

            if (command == "ACT:HEAT:ON") {
                if (!_manualBypass) return "ERR:MODE";
                led(_heatingPin, HIGH);
                return "OK:HEAT:ON";
            }

            if (command == "ACT:HEAT:OFF") {
                if (!_manualBypass) return "ERR:MODE";
                led(_heatingPin, LOW);
                return "OK:HEAT:OFF";
            }

            if (command == "ACT:COOL:ON") {
                if (!_manualBypass) return "ERR:MODE";
                led(_coolingPin, HIGH);
                return "OK:COOL:ON";
            }

            if (command == "ACT:COOL:OFF") {
                if (!_manualBypass) return "ERR:MODE";
                led(_coolingPin, LOW);
                return "OK:COOL:OFF";
            }

            if (command == "ACT:HUMIDIFIER:ON") {
                if (!_manualBypass) return "ERR:MODE";
                led(_humidifierPin, HIGH);
                return "OK:HUMIDIFIER:ON";
            }

            if (command == "ACT:HUMIDIFIER:OFF") {
                if (!_manualBypass) return "ERR:MODE";
                led(_humidifierPin, LOW);
                return "OK:HUMIDIFIER:OFF";
            }

            if (command.startsWith("ACT:PLAFONIERE:PWM:")) {
                if (!_manualBypass) return "ERR:MODE";

                int pwm = 0;
                if (!parseIntStrict(command.substring(19), pwm)) return "ERR:FORMAT:PWM";
                if (pwm < 0 || pwm > 255) return "ERR:RANGE:PWM";

                ledDimming(_plafonierePin, pwm);
                return "OK:PLAFONIERE:PWM";
            }

            return "ERR:UNKNOWN";
        }

    private:
        void moveServoAbsolute(int targetAngle) {
            if (_servo == nullptr) return;

            int delta = targetAngle - _servo->read();
            antiSufferingServo(delta, *_servo);
        }
};

#endif