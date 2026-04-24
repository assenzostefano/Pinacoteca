// humidity.h
// DHT22 humidity sensor driver.
// Wraps the DHT library with error reporting through the Pinacoteca ErrorRegistry.

#ifndef HUMIDITY_H
#define HUMIDITY_H

#include <Arduino.h>
#include <DHT.h>
#include "../system/error_registry.h"

// Default data pin and model for the DHT sensor
#define PINACOTECA_DHT_PIN  2
#define PINACOTECA_DHT_TYPE DHT22

// Class that manages a DHT22 humidity sensor
class HumiditySensor {
  private:
    mutable DHT _dht; // DHT object (mutable because read changes its internal state)
    mutable float _lastValue;
    mutable unsigned long _lastReadMs;
    mutable bool _hasSample;

    static constexpr unsigned long MIN_READ_INTERVAL_MS = 2000UL;

  public:
    // Constructor: pass the data pin and sensor type
    HumiditySensor(uint8_t pin, uint8_t type)
      : _dht(pin, type),
        _lastValue(SENSOR_ERROR_VALUE),
        _lastReadMs(0),
        _hasSample(false) {}

    // Initialize the sensor
    void begin() {
      _dht.begin();
      _lastValue = SENSOR_ERROR_VALUE;
      _lastReadMs = 0;
      _hasSample = false;
    }

    // Read relative humidity in %
    // Returns SENSOR_ERROR_VALUE if the sensor fails
    float read() const {
      unsigned long now = millis();

      // DHT22 is slow and should not be sampled continuously.
      if (_hasSample && (now - _lastReadMs) < MIN_READ_INTERVAL_MS) {
        return _lastValue;
      }

      float value = _dht.readHumidity();
      _lastReadMs = now;

      if (isnan(value)) {
        _hasSample = false;
        pinacotecaSetError(PIN_ERR_HUM_SENSOR);
        return SENSOR_ERROR_VALUE;
      }

      _lastValue = value;
      _hasSample = true;
      pinacotecaClearError(PIN_ERR_HUM_SENSOR);
      return _lastValue;
    }

    // Direct access to the underlying DHT object (used in tests)
    DHT& dht() { return _dht; }
};

// --- Legacy compatibility ---

// Global sensor instance
inline HumiditySensor& _pinacotecaHumSensor() {
  static HumiditySensor instance(PINACOTECA_DHT_PIN, PINACOTECA_DHT_TYPE);
  return instance;
}

// Alias for the old "dht_sensor" global name
#define dht_sensor (_pinacotecaHumSensor().dht())

// Legacy initialization function
inline void beginHumiditySensor() {
  _pinacotecaHumSensor().begin();
}

// Legacy read function
inline float readHumidity() {
  return _pinacotecaHumSensor().read();
}

#endif // HUMIDITY_H