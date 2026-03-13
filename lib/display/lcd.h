#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include "display.h"
#include "../temperature/thermostat.h"
#include "../humidity/humidifier.h"
#include "../lighting/lighting_control.h"

class DisplayPanel {
	private:
		DisplayInterface _display;
		unsigned long _lastRefresh;
		unsigned long _lastPageChange;
		byte _page;
		int _maxPeople;

	public:
		DisplayPanel(int rs, int en, int d4, int d5, int d6, int d7, int maxPeople)
			: _display(rs, en, d4, d5, d6, d7), _lastRefresh(0), _lastPageChange(0), _page(0), _maxPeople(maxPeople) {}

		void begin() {
			_display.begin(16, 2);
			_display.clear();
			_display.setCursor(0, 0);
			_display.print("Pinacoteca OK");
			_display.setCursor(0, 1);
			_display.print("Display avviato");
			delay(800);
			_display.clear();
		}

		void update(int currentPeople, Thermostat &thermostat, HumidifierControl &humidifier, LightingControl &lighting) {
			unsigned long now = millis();

			if (now - _lastRefresh < 1000) {
				return;
			}
			_lastRefresh = now;

			if (now - _lastPageChange >= 3000) {
				_page = (_page + 1) % 2;
				_lastPageChange = now;
			}

			float currentTemp = thermostat.getCurrentTemperature();
			float targetTemp = thermostat.getTargetTemperature();
			float currentHum = humidifier.getCurrentHumidity();
			float targetHum = humidifier.getTargetHumidity();
			float currentLux = lighting.getCurrentLux();
			int targetLux = lighting.getTargetLux();

			_display.clear();

			if (_page == 0) {
				_display.setCursor(0, 0);
				_display.print("P:");
				_display.print(currentPeople);
				_display.print("/");
				_display.print(_maxPeople);

				_display.setCursor(0, 1);
				_display.print("T:");
				if (currentTemp <= -900.0) {
					_display.print("ERR");
				} else {
					_display.print(currentTemp, 1);
				}
				_display.print("/");
				_display.print(targetTemp, 1);
				_display.print("C");
			} else {
				_display.setCursor(0, 0);
				_display.print("H:");
				if (currentHum <= -900.0) {
					_display.print("ERR");
				} else {
					_display.print(currentHum, 0);
				}
				_display.print("/");
				_display.print(targetHum, 0);
				_display.print("%");

				_display.setCursor(0, 1);
				_display.print("L:");
				_display.print(currentLux, 0);
				_display.print("/");
				_display.print(targetLux);
				_display.print("lx");
			}
		}
};

#endif
