/**
 * @file remote_gateway.h
 * @brief Serial + Bluetooth command gateway (zero heap allocation).
 *
 * Reads commands from both Serial and a BluetoothConnection,
 * dispatches them to the CommandProcessor, and periodically
 * publishes the full system state.
 */

#ifndef REMOTE_GATEWAY_H
#define REMOTE_GATEWAY_H

#include <Arduino.h>
#include "command_processor.h"
#include "bluetooth_connection.h"

class RemoteControlGateway {
  private:
    CommandProcessor _cmd;
    BluetoothConnection& _bluetooth;

    unsigned long _statePeriodMs;
    unsigned long _lastStateMillis;
    bool _wasConnected;

    static constexpr uint8_t RX_BUFFER_SIZE = BT_MSG_MAX;
    char _serialBuf[RX_BUFFER_SIZE];
    uint8_t _serialLen;
    bool _serialOverflow;

    /// Response buffer shared by serial and BT handlers
    char _responseBuf[128];

    /// Publish state if enough time has passed.
    void publishStateIfDue() {
      unsigned long now = millis();
      if (now - _lastStateMillis < _statePeriodMs) return;

      _lastStateMillis = now;

      char stateBuf[128];
      _cmd.buildStatePayload(stateBuf, sizeof(stateBuf));
      
      // Log to serial monitor for debugging
      Serial.println(stateBuf);

      if (_bluetooth.isConnected()) {
        _bluetooth.sendMessage(stateBuf);
      }
    }

    /// Handle serial commands.
    void handleSerial() {
      while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());

        if (c == '\n' || c == '\r') {
          if (_serialOverflow) {
            Serial.println(F("ERR:TOO_LONG"));
            _serialLen = 0;
            _serialOverflow = false;
            continue;
          }
          if (_serialLen == 0) continue;

          _serialBuf[_serialLen] = '\0';
          _cmd.processCommand(_serialBuf, _responseBuf);
          Serial.println(_responseBuf);
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

    /// Handle Bluetooth commands.
    void handleBluetooth() {
      _bluetooth.poll();

      char cmdBuf[BT_MSG_MAX];
      if (_bluetooth.readMessage(cmdBuf, sizeof(cmdBuf))) {
        Serial.print(F("BT Rx: ")); Serial.println(cmdBuf);
        _cmd.processCommand(cmdBuf, _responseBuf);
        Serial.print(F("BT Tx: ")); Serial.println(_responseBuf);
        _bluetooth.sendMessage(_responseBuf);
      }
    }

  public:
    RemoteControlGateway(
        Thermostat* thermostat,
        HumidifierControl* humidifier,
        LightingControl* lighting,
        Turnstile* turnstile,
        Servo* servo,
        uint8_t greenPin, uint8_t redPin,
        uint8_t heatingPin, uint8_t coolingPin,
        uint8_t humidifierPin, uint8_t ceilingLightsPin,
        BluetoothConnection& bluetooth,
        unsigned long statePeriodMs = 1000)
      : _cmd(thermostat, humidifier, lighting, turnstile, servo,
             greenPin, redPin, heatingPin, coolingPin,
             humidifierPin, ceilingLightsPin),
        _bluetooth(bluetooth),
        _statePeriodMs(statePeriodMs),
        _lastStateMillis(0),
        _wasConnected(false),
        _serialLen(0),
        _serialOverflow(false) {
      _serialBuf[0] = '\0';
      _responseBuf[0] = '\0';
    }

    void begin() { _bluetooth.begin(); }

    bool isManualBypassEnabled() const {
      return _cmd.isManualBypassEnabled();
    }

    /// Process serial + Bluetooth commands and publish state.
    void update() {
      handleSerial();
      handleBluetooth();

      bool connected = _bluetooth.isConnected();
      if (connected && !_wasConnected) {
        _lastStateMillis = millis();
      }
      _wasConnected = connected;

      publishStateIfDue();
    }
};

#endif // REMOTE_GATEWAY_H
