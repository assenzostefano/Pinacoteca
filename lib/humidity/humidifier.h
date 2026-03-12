#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include <Arduino.h>
#include "humidity.h"
#include "../led/led.h"

class HumidifierControl {
    private:
        int _humidifierPin;
        float _targetHumidity;

    public:
        HumidifierControl(int humidifierPin, float targetHumidity)
            : _humidifierPin(humidifierPin), _targetHumidity(targetHumidity) {}

        void begin() {
            beginHumiditySensor(); // Inizializza l'hardware DHT
            pinMode(_humidifierPin, OUTPUT);
        }

        bool update() {
            float currentHumidity = readHumidity();
            
            // Sicurezza: se il sensore è in errore, spegni l'umidificatore per evitare danni all'aria
            if (currentHumidity == -999.0) {
                led(_humidifierPin, LOW);
                return false;
            }

            // Da specifiche: l'umidificatore (che preleva l'umidità, quindi DEUMIDIFICATORE in questo caso)
            // deve accendersi per abbassare l'umidità. 
            float tolerance = 2.0; 

            if (currentHumidity > (_targetHumidity + tolerance)) {
                // Troppa umidità -> Accendi il macchinario per rimuoverla (Deumidificatore ON)
                led(_humidifierPin, HIGH);

            } else if (currentHumidity < (_targetHumidity - tolerance)) {
                // Troppo secco o target raggiunto -> Spegni il macchinario
                led(_humidifierPin, LOW);
                
            } else {
                // Valore ottimale all'interno della tolleranza -> Macchinario SPENTO
                led(_humidifierPin, LOW);
            }
            
            return true;
        }

        // Metodi per il Pannello di Controllo (Punto 5)
        void setTargetHumidity(float newHumidityTarget) {
            _targetHumidity = newHumidityTarget;
        }

        float getTargetHumidity() const {
            return _targetHumidity;
        }

        float getCurrentHumidity() const {
            return readHumidity();
        }
};

#endif // HUMIDIFIER_H
