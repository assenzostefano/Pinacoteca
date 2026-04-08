// bluetooth_link.h
// Concrete Bluetooth transport.
// Uses native BLE when ArduinoBLE is available (UNO R4 WiFi),
// with a Serial1 fallback for classic external modules.

#ifndef BLUETOOTH_LINK_H
#define BLUETOOTH_LINK_H

#include <Arduino.h>
#include "bluetooth_connection.h"

#if __has_include(<ArduinoBLE.h>)
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
    static const int MAX_MESSAGE_SIZE = 96;
    BLEService _service;
    BLEStringCharacteristic _rxCharacteristic;
    BLEStringCharacteristic _txCharacteristic;
    bool _connected;
#endif

#if PINACOTECA_HAS_BLUETOOTH_SERIAL1 && !PINACOTECA_HAS_NATIVE_BLE
    HardwareSerial& _serial;
#endif
    unsigned long _baudRate;
    bool _started;

#if !PINACOTECA_HAS_NATIVE_BLE
    String _rxBuffer;
#endif
    String _pendingMessage;

  public:
#if PINACOTECA_HAS_NATIVE_BLE
    explicit BluetoothLink(unsigned long baudRate = 9600)
      : _service("19B10000-E8F2-537E-4F6C-D104768A1214"),
        _rxCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLEWrite | BLEWriteWithoutResponse, MAX_MESSAGE_SIZE),
        _txCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLENotify | BLERead, MAX_MESSAGE_SIZE),
        _connected(false),
        _baudRate(baudRate),
        _started(false),
        _pendingMessage("") {}
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
    explicit BluetoothLink(unsigned long baudRate = 9600, HardwareSerial& serialPort = Serial1)
      : _serial(serialPort),
        _baudRate(baudRate),
        _started(false),
        _rxBuffer(""),
        _pendingMessage("") {}
#else
    explicit BluetoothLink(unsigned long baudRate = 9600)
      : _baudRate(baudRate),
        _started(false),
        _rxBuffer(""),
        _pendingMessage("") {}
#endif

    bool begin() override {
#if PINACOTECA_HAS_NATIVE_BLE
      Serial.println("BT:BLE:NATIVE");
      if (!BLE.begin()) {
        _started = false;
        _connected = false;
        Serial.println("ERR:BLE:BEGIN");
        return false;
      }

      BLE.setLocalName("Pinacoteca");
      BLE.setDeviceName("Pinacoteca");
      BLE.setAdvertisedService(_service);

      _service.addCharacteristic(_rxCharacteristic);
      _service.addCharacteristic(_txCharacteristic);
      BLE.addService(_service);

      _txCharacteristic.writeValue("READY");
      BLE.advertise();

      _pendingMessage = "";
      _started = true;
      _connected = false;
      return true;
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
      Serial.println("BT:SERIAL:FALLBACK");
      _serial.begin(_baudRate);
      _started = true;
      _rxBuffer = "";
      _pendingMessage = "";
      return true;
#else
      Serial.println("ERR:BT:UNAVAILABLE");
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

    bool readMessage(String& message) override {
      if (_pendingMessage.length() == 0) return false;
      message = _pendingMessage;
      _pendingMessage = "";
      return true;
    }

    void sendMessage(const String& message) override {
#if PINACOTECA_HAS_NATIVE_BLE
      if (_started && _connected) {
        _txCharacteristic.writeValue(message);
      }
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
      if (_started) {
        _serial.println(message);
      }
#else
      (void)message;
#endif
    }

    void poll() override {
#if PINACOTECA_HAS_NATIVE_BLE
      if (!_started) return;

      BLE.poll();
      _connected = BLE.connected();

      if (_rxCharacteristic.written()) {
        String message = _rxCharacteristic.value();
        message.trim();
        if (message.length() > 0) {
          _pendingMessage = message;
        }
      }

      if (!_connected) {
        BLE.advertise();
      }
#elif PINACOTECA_HAS_BLUETOOTH_SERIAL1
      if (!_started) return;

      while (_serial.available() > 0) {
        char c = static_cast<char>(_serial.read());

        if (c == '\n' || c == '\r') {
          if (_rxBuffer.length() > 0) {
            _pendingMessage = _rxBuffer;
            _rxBuffer = "";
            break;
          }
          continue;
        }

        if (_rxBuffer.length() < 95) {
          _rxBuffer += c;
        }
      }
#endif
    }
};

#endif // BLUETOOTH_LINK_H
