#ifndef REMOTE_GATEWAY_H
#define REMOTE_GATEWAY_H

#include <Arduino.h>

#include "command_processor.h"
#include "wifi_connection.h"

class RemoteControlGateway {
    private:
        CommandProcessor _commandProcessor;
        WifiConnection& _wifi;

        unsigned long _statePeriodMs;
        unsigned long _lastStateMillis;

        char _serialBuffer[96];
        size_t _serialBufferLen;
        bool _serialOverflow;

    public:
        RemoteControlGateway(
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
            int plafonierePin,
            WifiConnection& wifi,
            unsigned long statePeriodMs = 1000
        )
            : _commandProcessor(
                  thermostat,
                  humidifier,
                  lighting,
                  turnstile,
                  servo,
                  greenPin,
                  redPin,
                  heatingPin,
                  coolingPin,
                  humidifierPin,
                  plafonierePin
              ),
              _wifi(wifi),
              _statePeriodMs(statePeriodMs),
              _lastStateMillis(0),
              _serialBufferLen(0),
              _serialOverflow(false) {}

        void begin() {
            _wifi.begin();
        }

        bool isManualBypassEnabled() const {
            return _commandProcessor.isManualBypassEnabled();
        }

        void update() {
            handleSerial();
            handleWifi();
            publishStateIfDue();
        }

    private:
        void publishStateIfDue() {
            unsigned long now = millis();
            if (now - _lastStateMillis < _statePeriodMs) {
                return;
            }

            _lastStateMillis = now;
            String state = _commandProcessor.buildStatePayload();
            if (_wifi.isConnected()) {
                _wifi.sendMessage(state);
            }
        }

        void handleSerial() {
            while (Serial.available() > 0) {
                char c = static_cast<char>(Serial.read());
                if (c == '\n' || c == '\r') {
                    if (_serialOverflow) {
                        Serial.println("ERR:TOO_LONG");
                        _serialBufferLen = 0;
                        _serialOverflow = false;
                        continue;
                    }

                    if (_serialBufferLen == 0) {
                        continue;
                    }

                    _serialBuffer[_serialBufferLen] = '\0';
                    String response = _commandProcessor.processCommand(String(_serialBuffer));
                    Serial.println(response);
                    _serialBufferLen = 0;
                    continue;
                }

                if (_serialBufferLen < (sizeof(_serialBuffer) - 1)) {
                    _serialBuffer[_serialBufferLen++] = c;
                } else {
                    _serialOverflow = true;
                }
            }
        }

        void handleWifi() {
            _wifi.poll();

            String command;
            if (_wifi.readMessage(command)) {
                String response = _commandProcessor.processCommand(command);
                _wifi.sendMessage(response);
            }
        }
};

#endif
