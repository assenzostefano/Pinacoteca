#ifndef BLE_LINK_H
#define BLE_LINK_H

#include <Arduino.h>
#include "bluetooth_connection.h"

#if __has_include(<ArduinoBLE.h>)
#include <ArduinoBLE.h>
#define PINACOTECA_HAS_BLE 1
#else
#define PINACOTECA_HAS_BLE 0
#endif

class BleLink : public BluetoothConnection {
    private:
#if PINACOTECA_HAS_BLE
        BLEService _service;
        BLEStringCharacteristic _rxCharacteristic;
        BLEStringCharacteristic _txCharacteristic;
#endif

    public:
        BleLink()
#if PINACOTECA_HAS_BLE
            : _service("19B10000-E8F2-537E-4F6C-D104768A1214"),
              _rxCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLEWrite, 96),
              _txCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 160)
#endif
        {}

        bool readMessage(String& message) override {
#if PINACOTECA_HAS_BLE
            if (_rxCharacteristic.written()) {
                message = _rxCharacteristic.value();
                return true;
            }
#endif
            return false;
        }

        void sendMessage(const String& message) override {
#if PINACOTECA_HAS_BLE
            _txCharacteristic.writeValue(message);
#else
            (void)message;
#endif
        }

        bool begin() override {
#if PINACOTECA_HAS_BLE
            if (!BLE.begin()) {
                return false;
            }

            BLE.setLocalName("Pinacoteca-R4");
            BLE.setAdvertisedService(_service);
            _service.addCharacteristic(_rxCharacteristic);
            _service.addCharacteristic(_txCharacteristic);
            BLE.addService(_service);
            _txCharacteristic.writeValue("READY");
            BLE.advertise();
            return true;
#else
            return false;
#endif
        }

        bool isConnected() const override {
#if PINACOTECA_HAS_BLE
            return BLE.connected();
#else
            return false;
#endif
        }

        void poll() override {
#if PINACOTECA_HAS_BLE
            BLE.poll();
#endif
        }
};

#endif