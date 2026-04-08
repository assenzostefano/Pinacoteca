/**
 * @file bluetooth_connection.h
 * @brief Abstract Bluetooth text transport interface.
 *
 * Defines a zero-allocation contract for reading/writing
 * text messages over any Bluetooth transport (BLE or classic).
 */

#ifndef BLUETOOTH_CONNECTION_H
#define BLUETOOTH_CONNECTION_H

#include <Arduino.h>

/// Maximum message size for any Bluetooth channel.
static constexpr uint8_t BT_MSG_MAX = 96;

/**
 * @brief Abstract base class for a Bluetooth text channel.
 *
 * All message I/O uses fixed-size char buffers to avoid
 * heap-allocating String objects on constrained MCUs.
 */
class BluetoothConnection {
  public:
    virtual ~BluetoothConnection() {}

    virtual bool begin() = 0;
    virtual bool isConnected() const = 0;
    virtual void poll() = 0;

    /**
     * @brief Read the next pending message into @p buf.
     * @param buf     Destination buffer (at least @p maxLen bytes).
     * @param maxLen  Size of @p buf.
     * @return true if a message was available and copied.
     */
    virtual bool readMessage(char* buf, uint8_t maxLen) = 0;

    /** @brief Send a null-terminated text line. */
    virtual void sendMessage(const char* msg) = 0;
};

#endif // BLUETOOTH_CONNECTION_H
