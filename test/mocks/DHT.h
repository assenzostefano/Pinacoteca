#ifndef DHT_H
#define DHT_H

#include <cmath>

#ifndef DHT22
#define DHT22 22
#endif

class DHT {
private:
    int _pin;
    int _type;
    float _humidity = NAN;

public:
    DHT(int pin, int type) : _pin(pin), _type(type) {}

    void begin() {}

    float readHumidity() { return _humidity; }

    void setHumidity(float value) { _humidity = value; }

    int pin() const { return _pin; }
    int type() const { return _type; }
};

#endif
