bool led(int pin, bool state) {
    digitalWrite(pin, state);
    return true;
}