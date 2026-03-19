#ifndef SERVOMOTOR_H
#define SERVOMOTOR_H

#include <Arduino.h>
#include <Servo.h>
#include "../system/error_registry.h"

bool antiSufferingServo(int angle, Servo &s ){
  if (!s.attached()) {
    pinacotecaSetError(PIN_ERR_SERVO);
    return false;
  }

  int pos_attuale = s.read();
  int calcolo = pos_attuale + angle;
  
  if (calcolo > 180 || calcolo < 0){
    Serial.println("Limit movement Servo Motor");
    pinacotecaSetError(PIN_ERR_SERVO);
    
    return false;

  } else {
    s.write(calcolo);
    pinacotecaClearError(PIN_ERR_SERVO);
    
    return true;
  }
}

#endif