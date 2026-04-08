// bluetooth_connection.h
// Abstract Bluetooth text transport interface.

#ifndef BLUETOOTH_CONNECTION_H
#define BLUETOOTH_CONNECTION_H

#include <Arduino.h>

// Abstract base class for a Bluetooth text channel.
class BluetoothConnection {
  public:
    virtual ~BluetoothConnection() {}

    virtual bool begin() = 0;                        // Initialize the channel
    virtual bool isConnected() const = 0;            // True when channel can exchange data
    virtual void poll() = 0;                         // Read pending data from transport
    virtual bool readMessage(String& message) = 0;   // Read next full text line
    virtual void sendMessage(const String& message) = 0; // Send one text line
};

#endif // BLUETOOTH_CONNECTION_H
