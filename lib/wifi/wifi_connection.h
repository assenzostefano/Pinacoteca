// wifi_connection.h
// Abstract Wi-Fi transport interface.

#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <Arduino.h>

// Abstract base class for a network channel
class WifiConnection {
  public:
    virtual ~WifiConnection() {}

    virtual bool begin() = 0;                        // Initialize the connection
    virtual bool isConnected() const = 0;            // Check if a client is connected
    virtual void poll() = 0;                         // Check for incoming data
    virtual bool readMessage(String& message) = 0;   // Read the next message
    virtual void sendMessage(const String& message) = 0; // Send a message
};

#endif // WIFI_CONNECTION_H
