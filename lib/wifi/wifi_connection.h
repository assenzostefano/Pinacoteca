#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <Arduino.h>

class WifiConnection {
    public:
        virtual ~WifiConnection() {}

        virtual bool begin() = 0;
        virtual bool isConnected() const = 0;
        virtual void poll() = 0;
        virtual bool readMessage(String& message) = 0;
        virtual void sendMessage(const String& message) = 0;
};

#endif
