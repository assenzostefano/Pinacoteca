#ifndef WIFI_LINK_H
#define WIFI_LINK_H

#include <Arduino.h>
#include "wifi_connection.h"

#if __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#define PINACOTECA_HAS_WIFI 1
#else
#define PINACOTECA_HAS_WIFI 0
#endif

class WifiLink : public WifiConnection {
    private:
#if PINACOTECA_HAS_WIFI
        const char* _ssid;
        const char* _password;
        uint16_t _port;
        WiFiServer _server;
        WiFiClient _client;
        String _rxBuffer;
        String _pendingMessage;
#endif

    public:
        WifiLink(const char* ssid, const char* password, uint16_t port = 7777)
#if PINACOTECA_HAS_WIFI
            : _ssid(ssid),
              _password(password),
              _port(port),
              _server(port)
#endif
        {}

        bool readMessage(String& message) override {
#if PINACOTECA_HAS_WIFI
            if (_pendingMessage.length() == 0) {
                return false;
            }

            message = _pendingMessage;
            _pendingMessage = "";
            return true;
#else
            (void)message;
            return false;
#endif
        }

        void sendMessage(const String& message) override {
#if PINACOTECA_HAS_WIFI
            if (_client && _client.connected()) {
                _client.println(message);
            }
#else
            (void)message;
#endif
        }

        bool begin() override {
#if PINACOTECA_HAS_WIFI
            if (_ssid == nullptr || _ssid[0] == '\0') {
                return false;
            }

            WiFi.disconnect();
            int status = WiFi.begin(_ssid, _password);
            unsigned long start = millis();

            while (status != WL_CONNECTED && (millis() - start) < 15000UL) {
                delay(500);
                status = WiFi.status();
            }

            if (status != WL_CONNECTED) {
                return false;
            }

            _server.begin();
            _server.setNoDelay(true);
            return true;
#else
            return false;
#endif
        }

        bool isConnected() const override {
#if PINACOTECA_HAS_WIFI
            return static_cast<bool>(_client) && _client.connected();
#else
            return false;
#endif
        }

        void poll() override {
#if PINACOTECA_HAS_WIFI
            if (!_client || !_client.connected()) {
                WiFiClient candidate = _server.available();
                if (candidate) {
                    _client = candidate;
                    _rxBuffer = "";
                    _pendingMessage = "";
                }
            }

            if (!_client || !_client.connected()) {
                return;
            }

            while (_client.available() > 0) {
                char c = static_cast<char>(_client.read());

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

#endif
