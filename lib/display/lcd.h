#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <stdio.h>
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

		int _baseYear;
		byte _baseMonth;
		byte _baseDay;
		byte _baseHour;
		byte _baseMinute;
		byte _baseSecond;

		byte monthFromString(const char* mon) {
			if (strncmp(mon, "Jan", 3) == 0) return 1;
			if (strncmp(mon, "Feb", 3) == 0) return 2;
			if (strncmp(mon, "Mar", 3) == 0) return 3;
			if (strncmp(mon, "Apr", 3) == 0) return 4;
			if (strncmp(mon, "May", 3) == 0) return 5;
			if (strncmp(mon, "Jun", 3) == 0) return 6;
			if (strncmp(mon, "Jul", 3) == 0) return 7;
			if (strncmp(mon, "Aug", 3) == 0) return 8;
			if (strncmp(mon, "Sep", 3) == 0) return 9;
			if (strncmp(mon, "Oct", 3) == 0) return 10;
			if (strncmp(mon, "Nov", 3) == 0) return 11;
			return 12;
		}

		bool isLeapYear(int year) {
			if (year % 400 == 0) return true;
			if (year % 100 == 0) return false;
			return (year % 4 == 0);
		}

		byte daysInMonth(int year, byte month) {
			if (month == 2) {
				return isLeapYear(year) ? 29 : 28;
			}
			if (month == 4 || month == 6 || month == 9 || month == 11) {
				return 30;
			}
			return 31;
		}

		byte weekdayIndex(int year, byte month, byte day) {
			int y = year;
			int m = month;
			if (m < 3) {
				m += 12;
				y -= 1;
			}
			int k = y % 100;
			int j = y / 100;
			int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
			return (h + 5) % 7;
		}

		const char* weekdayName(byte index) {
			static const char* names[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
			if (index > 6) return "---";
			return names[index];
		}

		void getCurrentDateTime(int &year, byte &month, byte &day, byte &hour, byte &minute, byte &second) {
			unsigned long totalSeconds = millis() / 1000UL;

			unsigned long secOfDay = (unsigned long)_baseHour * 3600UL + (unsigned long)_baseMinute * 60UL + (unsigned long)_baseSecond;
			totalSeconds += secOfDay;

			unsigned long daysOffset = totalSeconds / 86400UL;
			unsigned long daySeconds = totalSeconds % 86400UL;

			hour = daySeconds / 3600UL;
			daySeconds %= 3600UL;
			minute = daySeconds / 60UL;
			second = daySeconds % 60UL;

			year = _baseYear;
			month = _baseMonth;
			day = _baseDay;

			while (daysOffset > 0) {
				day++;
				if (day > daysInMonth(year, month)) {
					day = 1;
					month++;
					if (month > 12) {
						month = 1;
						year++;
					}
				}
				daysOffset--;
			}
		}

		void printDateTimePage() {
			int year;
			byte month;
			byte day;
			byte hour;
			byte minute;
			byte second;

			getCurrentDateTime(year, month, day, hour, minute, second);
			byte wd = weekdayIndex(year, month, day);

			char line1[17];
			char line2[17];
			snprintf(line1, sizeof(line1), "%s %02d/%02d/%04d", weekdayName(wd), day, month, year);
			snprintf(line2, sizeof(line2), "Hour %02d:%02d:%02d", hour, minute, second);

			_display.setCursor(0, 0);
			_display.print(line1);
			_display.setCursor(0, 1);
			_display.print(line2);
		}

		void printStatusPage(int currentPeople, float currentTemp, float targetTemp, float currentHum, float targetHum, float currentLux, int targetLux) {
			const float tolerance = 1.0;

			char tempStr[8];
			if (currentTemp <= -900.0) {
				snprintf(tempStr, sizeof(tempStr), "ERR");
			} else {
				dtostrf(currentTemp, 4, 1, tempStr);
			}

			char humStr[8];
			if (currentHum <= -900.0) {
				snprintf(humStr, sizeof(humStr), "ERR");
			} else {
				int humInt = (int)(currentHum + 0.5);
				snprintf(humStr, sizeof(humStr), "%d", humInt);
			}

			const char* climateMode = "OFF";
			if (currentTemp <= -900.0) {
				climateMode = "ERR";
			} else if (currentTemp < (targetTemp - tolerance)) {
				climateMode = "HEAT";
			} else if (currentTemp > (targetTemp + tolerance)) {
				climateMode = "COOL";
			}

			char line1[17];
			char line2[17];
			snprintf(line1, sizeof(line1), "T:%sC H:%s%%", tempStr, humStr);
			snprintf(line2, sizeof(line2), "C:%s TR:%d/%d", climateMode, currentPeople, _maxPeople);

			_display.setCursor(0, 0);
			_display.print(line1);
			_display.setCursor(0, 1);
			_display.print(line2);
		}

		void printErrorPage(int currentPeople, float currentTemp, float targetTemp, float currentHum, float targetHum) {
			char line1[17] = "Error / Fault";
			char line2[17] = "No errors detected";

			if (currentTemp <= -900.0) {
				snprintf(line2, sizeof(line2), "Temperature ERR");
			} else if (currentTemp > (targetTemp + 1.0)) {
				snprintf(line2, sizeof(line2), "Temperature HIGH!");
			} else if (currentHum <= -900.0) {
				snprintf(line2, sizeof(line2), "Humidity ERROR");
			} else if (currentHum > (targetHum + 2.0)) {
				snprintf(line2, sizeof(line2), "Humidity HIGH!");
			} else if (currentHum < (targetHum - 2.0)) {
				snprintf(line2, sizeof(line2), "Humidity LOW!");
			} else if (currentPeople < 0 || currentPeople > _maxPeople) {
				snprintf(line2, sizeof(line2), "Turnstile ERROR");
			}

			_display.setCursor(0, 0);
			_display.print(line1);
			_display.setCursor(0, 1);
			_display.print(line2);
		}

	public:
		DisplayPanel(int rs, int en, int d4, int d5, int d6, int d7, int maxPeople)
			: _display(rs, en, d4, d5, d6, d7), _lastRefresh(0), _lastPageChange(0), _page(0), _maxPeople(maxPeople) {
			_baseYear = atoi(__DATE__ + 7);
			_baseMonth = monthFromString(__DATE__);
			_baseDay = (byte)atoi(__DATE__ + 4);
			_baseHour = (byte)atoi(__TIME__);
			_baseMinute = (byte)atoi(__TIME__ + 3);
			_baseSecond = (byte)atoi(__TIME__ + 6);
		}

		void begin() {
			_display.begin(16, 2);
			_display.clear();
			_display.setCursor(0, 0);
			_display.print("Pinacoteca OK");
			_display.setCursor(0, 1);
			_display.print("Display started");
			delay(800);
			_display.clear();
		}

		void update(int currentPeople, Thermostat &thermostat, HumidifierControl &humidifier, LightingControl &lighting) {
			unsigned long now = millis();

			if (now - _lastRefresh < 1000) {
				return;
			}
			_lastRefresh = now;

			if (now - _lastPageChange >= 5000) {
				_page = (_page + 1) % 3;
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
				printDateTimePage();
			} else if (_page == 1) {
				printStatusPage(currentPeople, currentTemp, targetTemp, currentHum, targetHum, currentLux, targetLux);
			} else {
				printErrorPage(currentPeople, currentTemp, targetTemp, currentHum, targetHum);
			}
		}
};

#endif