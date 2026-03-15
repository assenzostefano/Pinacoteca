#ifndef BLUETOOTH_CONNECTION_H
#define BLUETOOTH_CONNECTION_H

#include <Arduino.h>

class BluetoothConnection {
    public:
        virtual ~BluetoothConnection() {}

        virtual bool begin() = 0;
        virtual bool isConnected() const = 0;
        virtual void poll() = 0;
        virtual bool readMessage(String& message) = 0;
        virtual void sendMessage(const String& message) = 0;
};

#endif