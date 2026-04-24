/**
 * @file bluetooth_link.h
 * @brief Concrete Bluetooth transport (zero-allocation).
 *
 * Uses native BLE when ArduinoBLE is available (UNO R4 WiFi),
 * with a Serial1 fallback for classic external modules (HC-05/06).
 * All internal buffers are stack-allocated char arrays.
 */

#ifndef BLUETOOTH_LINK_H
#define BLUETOOTH_LINK_H

#include <Arduino.h>
#include "bluetooth_connection.h"

// Arduino CLI include discovery may not expose ArduinoBLE to __has_include
// on UNO R4 unless the library is selected upfront by board macros.
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_ARCH_RENESAS_UNO)
#include <ArduinoBLE.h>
#define PINACOTECA_HAS_NATIVE_BLE 1
#elif __has_include(<ArduinoBLE.h>)
#include <ArduinoBLE.h>
#define PINACOTECA_HAS_NATIVE_BLE 1
#else
#define PINACOTECA_HAS_NATIVE_BLE 0
#endif

#if defined(HAVE_HWSERIAL1)
#define PINACOTECA_HAS_BLUETOOTH_SERIAL1 1
#else
#define PINACOTECA_HAS_BLUETOOTH_SERIAL1 0
#endif

class BluetoothLink : public BluetoothConnection {
  private:
#if PINACOTECA_HAS_NATIVE_BLE
    BLEService _service;
    BLEStringCharacteristic _rxCharacteristic;
    BLEStringCharacteristic _txCharacteristic;
    bool _connected;
    bool _advertising;
#endif

#if PINACOTECA_HAS_BLUETOOTH_SERIAL1 && !PINACOTECA_HAS_NATIVE_BLE
    HardwareSerial& _serial;
#endif
    unsigned long _baudRate;
    bool _started;

    // Fixed-size receive & pending buffers (no heap)
#if !PINACOTECA_HAS_NATIVE_BLE
    char _rxBuffer[BT_MSG_MAX];
    uint8_t _rxLen;
#endif
    char _pendingMessage[BT_MSG_MAX];
    uint8_t _pendingLen;

  public:
#if PINACOTECA_HAS_NATIVE_BLE
    explicit BluetoothLink(unsigned long baudRate = 9600)
      : _service("19B10000-E8F2-537E-4F6C-D104768A1214"),
        _rxCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214",
                          BLEWrite | BLEWriteWithoutResponse, BT_MSG_MAX),
        _txCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214",
                          BLENotify | BLERead, BT_MSG_MAX),
        _connected(false),
        _advertising(false),
        _baudRate(baudRate),
        _started(false),
        _pendingLen(0) {
      _pendingMessage[0] = '\0';
    }
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
    explicit BluetoothLink(unsigned long baudRate = 9600,
                           HardwareSerial& serialPort = Serial1)
      : _serial(serialPort),
        _baudRate(baudRate),
        _started(false),
        _rxLen(0),
        _pendingLen(0) {
      _rxBuffer[0] = '\0';
      _pendingMessage[0] = '\0';
    }
#else
    explicit BluetoothLink(unsigned long baudRate = 9600)
      : _baudRate(baudRate),
        _started(false),
        _rxLen(0),
        _pendingLen(0) {
      _rxBuffer[0] = '\0';
      _pendingMessage[0] = '\0';
    }
#endif

    bool begin() override {
#if PINACOTECA_HAS_NATIVE_BLE
      Serial.println(F("BT:BLE:NATIVE"));
      if (!BLE.begin()) {
        _started = false;
        _connected = false;
        _advertising = false;
        Serial.println(F("ERR:BLE:BEGIN"));
        return false;
      }

      // Keep advertised local name short to fit 31-byte BLE advertising payload
      // together with a 128-bit custom service UUID.
      BLE.setLocalName("PINA-R4");
      BLE.setDeviceName("Pinacoteca");
      BLE.setConnectable(true);
      BLE.setAdvertisedService(_service);

      _service.addCharacteristic(_rxCharacteristic);
      _service.addCharacteristic(_txCharacteristic);
      BLE.addService(_service);

      _txCharacteristic.writeValue("READY");
      BLE.advertise();

      _pendingMessage[0] = '\0';
      _pendingLen = 0;
      _started = true;
      _connected = false;
      _advertising = true;
      Serial.println(F("BT:BLE:ADV:PINA-R4"));
      return true;
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
      Serial.println(F("BT:SERIAL:FALLBACK"));
      _serial.begin(_baudRate);
      _started = true;
      _rxBuffer[0] = '\0';
      _rxLen = 0;
      _pendingMessage[0] = '\0';
      _pendingLen = 0;
      return true;
#else
      Serial.println(F("ERR:BT:UNAVAILABLE"));
      (void)_baudRate;
      _started = false;
      return false;
#endif
    }

    bool isConnected() const override {
#if PINACOTECA_HAS_NATIVE_BLE
      return _started && _connected;
#else
      return _started;
#endif
    }

    bool readMessage(char* buf, uint8_t maxLen) override {
      if (_pendingLen == 0) return false;
      uint8_t copyLen = (_pendingLen < maxLen - 1) ? _pendingLen : (maxLen - 1);
      memcpy(buf, _pendingMessage, copyLen);
      buf[copyLen] = '\0';
      _pendingMessage[0] = '\0';
      _pendingLen = 0;
      return true;
    }

    void sendMessage(const char* msg) override {
#if PINACOTECA_HAS_NATIVE_BLE
      if (_started && _connected) {
        _txCharacteristic.writeValue(msg);
      }
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
      if (_started) {
        _serial.println(msg);
      }
#else
      (void)msg;
#endif
    }

    void poll() override {
#if PINACOTECA_HAS_NATIVE_BLE
      if (!_started) return;

      BLE.poll();
      const bool nowConnected = BLE.connected();

      if (nowConnected && !_connected) {
        Serial.println(F("BT:BLE:CONN"));
      } else if (!nowConnected && _connected) {
        Serial.println(F("BT:BLE:DISC"));
      }

      if (nowConnected) {
        _advertising = false;
      } else if (_connected || !_advertising) {
        BLE.advertise();
        _advertising = true;
      }

      _connected = nowConnected;

      if (_rxCharacteristic.written()) {
        String message = _rxCharacteristic.value();
        message.trim();
        uint8_t len = (message.length() < BT_MSG_MAX - 1)
                        ? message.length() : (BT_MSG_MAX - 1);
        if (len > 0) {
          memcpy(_pendingMessage, message.c_str(), len);
          _pendingMessage[len] = '\0';
          _pendingLen = len;
        }
      }
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
      if (!_started) return;

      while (_serial.available() > 0) {
        char c = static_cast<char>(_serial.read());

        if (c == '\n' || c == '\r') {
          if (_rxLen > 0) {
            _rxBuffer[_rxLen] = '\0';
            memcpy(_pendingMessage, _rxBuffer, _rxLen + 1);
            _pendingLen = _rxLen;
            _rxLen = 0;
            break;
          }
          continue;
        }

        if (_rxLen < BT_MSG_MAX - 2) {
          _rxBuffer[_rxLen++] = c;
        }
      }
#endif
    }
};

#endif // BLUETOOTH_LINK_H
