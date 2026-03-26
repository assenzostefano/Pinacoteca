// remote_gateway.h
// Serial + Wi-Fi command gateway.
// Reads commands from both Serial and a WifiConnection,
// dispatches them to the CommandProcessor, and periodically
// publishes the full system state.

#ifndef REMOTE_GATEWAY_H
#define REMOTE_GATEWAY_H

#include <Arduino.h>
#include "command_processor.h"
#include "wifi_connection.h"

class RemoteControlGateway {
  private:
    CommandProcessor _cmd;
    WifiConnection& _wifi;

    unsigned long _statePeriodMs;
    unsigned long _lastStateMillis;

    static const size_t RX_BUFFER_SIZE = 96;
    char _serialBuf[RX_BUFFER_SIZE];
    size_t _serialLen;
    bool _serialOverflow;

    // Publish state if enough time has passed
    void publishStateIfDue() {
      unsigned long now = millis();
      if (now - _lastStateMillis < _statePeriodMs) return;

      _lastStateMillis = now;
      String state = _cmd.buildStatePayload();

      if (_wifi.isConnected()) {
        _wifi.sendMessage(state);
      }
    }

    // Handle serial commands
    void handleSerial() {
      while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());

        if (c == '\n' || c == '\r') {
          if (_serialOverflow) {
            Serial.println("ERR:TOO_LONG");
            _serialLen = 0;
            _serialOverflow = false;
            continue;
          }
          if (_serialLen == 0) continue;

          _serialBuf[_serialLen] = '\0';
          String response = _cmd.processCommand(String(_serialBuf));
          Serial.println(response);
          _serialLen = 0;
          continue;
        }

        if (_serialLen < (RX_BUFFER_SIZE - 1)) {
          _serialBuf[_serialLen++] = c;
        } else {
          _serialOverflow = true;
        }
      }
    }

    // Handle Wi-Fi commands
    void handleWifi() {
      _wifi.poll();

      String command;
      if (_wifi.readMessage(command)) {
        String response = _cmd.processCommand(command);
        _wifi.sendMessage(response);
      }
    }

  public:
    RemoteControlGateway(
        Thermostat* thermostat,
        HumidifierControl* humidifier,
        LightingControl* lighting,
        Turnstile* turnstile,
        Servo* servo,
        int greenPin, int redPin,
        int heatingPin, int coolingPin,
        int humidifierPin, int ceilingLightsPin,
        WifiConnection& wifi,
        unsigned long statePeriodMs = 1000)
      : _cmd(thermostat, humidifier, lighting, turnstile, servo,
             greenPin, redPin, heatingPin, coolingPin,
             humidifierPin, ceilingLightsPin),
        _wifi(wifi),
        _statePeriodMs(statePeriodMs),
        _lastStateMillis(0),
        _serialLen(0),
        _serialOverflow(false) {}

    void begin() { _wifi.begin(); }

    bool isManualBypassEnabled() const {
      return _cmd.isManualBypassEnabled();
    }

    // Process serial + Wi-Fi commands and publish state
    void update() {
      handleSerial();
      handleWifi();
      publishStateIfDue();
    }
};

#endif // REMOTE_GATEWAY_H
