#ifndef SERVO_H
#define SERVO_H

class Servo {
private:
    int _pin = -1;
    int _pos = 0;

public:
    void attach(int pin) { _pin = pin; }
    void write(int pos) { _pos = pos; }
    int read() const { return _pos; }
    bool attached() const { return _pin >= 0; }
    int pin() const { return _pin; }
};

#endif
