// lcd.h
// Multi-page LCD panel for the Pinacoteca gallery.
// Rotates through pages: date/time, status, faults, sensor errors.
// Refreshes once per second and advances every 5 seconds.

#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <stdio.h>
#include "display.h"
#include "../system/error_registry.h"
#include "../temperature/thermostat.h"
#include "../humidity/humidifier.h"
#include "../lighting/lighting_control.h"

class DisplayPanel {
  private:
    DisplayInterface _display;
    unsigned long _lastRefresh;
    unsigned long _lastPageChange;
    unsigned long _logoUntil;
    byte _page;
    int _maxPeople;
  // Default 32x32 placeholder logo (replace with company bitmap).
  static const uint8_t DEFAULT_LOGO_32x32[128];


    // Compile-time clock seed
    int _baseYear;
    byte _baseMonth;
    byte _baseDay;
    byte _baseHour;
    byte _baseMinute;
    byte _baseSecond;

    // Convert month name from __DATE__ macro to a number
    byte monthFromString(const char* mon) {
      if (strncmp(mon, "Jan", 3) == 0) return  1;
      if (strncmp(mon, "Feb", 3) == 0) return  2;
      if (strncmp(mon, "Mar", 3) == 0) return  3;
      if (strncmp(mon, "Apr", 3) == 0) return  4;
      if (strncmp(mon, "May", 3) == 0) return  5;
      if (strncmp(mon, "Jun", 3) == 0) return  6;
      if (strncmp(mon, "Jul", 3) == 0) return  7;
      if (strncmp(mon, "Aug", 3) == 0) return  8;
      if (strncmp(mon, "Sep", 3) == 0) return  9;
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
      if (month == 2) return isLeapYear(year) ? 29 : 28;
      if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
      return 31;
    }

    // Zeller's formula: returns 0=Mon, 1=Tue, ..., 6=Sun
    byte weekdayIndex(int year, byte month, byte day) {
      int y = year;
      int m = month;
      if (m < 3) { m += 12; y -= 1; }
      int k = y % 100;
      int j = y / 100;
      int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
      return (h + 5) % 7;
    }

    const char* weekdayName(byte index) {
      static const char* names[7] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
      };
      return (index > 6) ? "---" : names[index];
    }

    // Calculate current date and time from compile time + millis()
    void getCurrentDateTime(int& year, byte& month, byte& day,
                            byte& hour, byte& minute, byte& second) {
      unsigned long totalSec = millis() / 1000UL;
      unsigned long secOfDay = (unsigned long)_baseHour * 3600UL
                             + (unsigned long)_baseMinute * 60UL
                             + (unsigned long)_baseSecond;
      totalSec += secOfDay;

      unsigned long daysOff = totalSec / 86400UL;
      unsigned long daySec = totalSec % 86400UL;

      hour   = daySec / 3600UL;  daySec %= 3600UL;
      minute = daySec / 60UL;
      second = daySec % 60UL;

      year  = _baseYear;
      month = _baseMonth;
      day   = _baseDay;

      while (daysOff > 0) {
        day++;
        if (day > daysInMonth(year, month)) {
          day = 1;
          month++;
          if (month > 12) { month = 1; year++; }
        }
        daysOff--;
      }
    }

    // --- Page renderers ---

    void printDateTimePage() {
      int yr; byte mo, dy, hr, mn, sc;
      getCurrentDateTime(yr, mo, dy, hr, mn, sc);
      byte wd = weekdayIndex(yr, mo, dy);

#if defined(ARDUINO)
      char line1[22], line2[22], line3[22];
      snprintf(line1, sizeof(line1), "PINACOTECA");
      snprintf(line2, sizeof(line2), "%s %02d/%02d/%04d", weekdayName(wd), dy, mo, yr);
      snprintf(line3, sizeof(line3), "%02d:%02d:%02d", hr, mn, sc);

      _display.setTextSize(1);
      _display.setCursor(0, 0);  _display.print(line1);
      _display.setCursor(0, 18); _display.print(line2);
      _display.setCursor(0, 34); _display.print(line3);
#else
      char line1[17], line2[17];
      snprintf(line1, sizeof(line1), "%s %02d/%02d/%04d", weekdayName(wd), dy, mo, yr);
      snprintf(line2, sizeof(line2), "Hour %02d:%02d:%02d", hr, mn, sc);

      _display.setCursor(0, 0); _display.print(line1);
      _display.setCursor(0, 1); _display.print(line2);
#endif
    }

    void printStatusPage(int people, float temp, float tgtTemp,
                         float hum, float tgtHum, float lux, int tgtLux) {
      const float tol = 1.0;
      char tempStr[8], humStr[8];

      if (temp <= SENSOR_ERROR_VALUE) snprintf(tempStr, sizeof(tempStr), "ERR");
      else dtostrf(temp, 4, 1, tempStr);

      if (hum <= SENSOR_ERROR_VALUE) snprintf(humStr, sizeof(humStr), "ERR");
      else snprintf(humStr, sizeof(humStr), "%d", (int)(hum + 0.5));

      const char* mode = "OFF";
      if      (temp <= SENSOR_ERROR_VALUE) mode = "ERR";
      else if (temp < (tgtTemp - tol))     mode = "HEAT";
      else if (temp > (tgtTemp + tol))     mode = "COOL";

    #if defined(ARDUINO)
      char line1[22], line2[22], line3[22], line4[22];
      snprintf(line1, sizeof(line1), "T:%sC (%s)", tempStr, mode);
      snprintf(line2, sizeof(line2), "H:%s%%", humStr);
      snprintf(line3, sizeof(line3), "P:%d/%d", people, _maxPeople);
      snprintf(line4, sizeof(line4), "L:%d/%d", (int)(lux + 0.5f), tgtLux);

      _display.setTextSize(1);
      _display.setCursor(0, 0);  _display.print(line1);
      _display.setCursor(0, 16); _display.print(line2);
      _display.setCursor(0, 32); _display.print(line3);
      _display.setCursor(0, 48); _display.print(line4);
    #else
      char line1[17], line2[17];
      snprintf(line1, sizeof(line1), "T:%sC H:%s%%", tempStr, humStr);
      snprintf(line2, sizeof(line2), "C:%s TR:%d/%d", mode, people, _maxPeople);

      _display.setCursor(0, 0); _display.print(line1);
      _display.setCursor(0, 1); _display.print(line2);
    #endif
    }

    bool hasSensorError(float temp, float hum, float lux) {
      if (temp <= SENSOR_ERROR_VALUE || hum <= SENSOR_ERROR_VALUE
          || lux <= SENSOR_ERROR_VALUE)
        return true;
      return pinacotecaHasAnyError();
    }

    void printErrorPage(float temp, float hum, float lux) {
#if defined(ARDUINO)
      char line2[22] = "No sensor errors";
      if      (temp <= SENSOR_ERROR_VALUE) snprintf(line2, sizeof(line2), "Temp sensor FAIL");
      else if (hum  <= SENSOR_ERROR_VALUE) snprintf(line2, sizeof(line2), "Hum sensor FAIL");
      else if (lux  <= SENSOR_ERROR_VALUE) snprintf(line2, sizeof(line2), "Light sensor FAIL");
      else if (pinacotecaHasAnyError())    snprintf(line2, sizeof(line2), "%s", pinacotecaFirstErrorText());

      _display.setTextSize(1);
      _display.setCursor(0, 0);  _display.print("ERROR");
      _display.setCursor(0, 18); _display.print(line2);
#else
      char line2[17] = "No sensor errors";
      if      (temp <= SENSOR_ERROR_VALUE) snprintf(line2, sizeof(line2), "Temp sensor FAIL");
      else if (hum  <= SENSOR_ERROR_VALUE) snprintf(line2, sizeof(line2), "Hum sensor FAIL");
      else if (lux  <= SENSOR_ERROR_VALUE) snprintf(line2, sizeof(line2), "Light sensor FAIL");
      else if (pinacotecaHasAnyError())    snprintf(line2, sizeof(line2), "%s", pinacotecaFirstErrorText());

      _display.setCursor(0, 0); _display.print("ERROR");
      _display.setCursor(0, 1); _display.print(line2);
#endif
    }

    void printFaultPage(int people, float temp, float tgtTemp,
                        float hum, float tgtHum) {
#if defined(ARDUINO)
      char line2[22] = "No faults";
      if      (temp > (tgtTemp + 1.0))            snprintf(line2, sizeof(line2), "Temperature HIGH");
      else if (temp < (tgtTemp - 1.0))            snprintf(line2, sizeof(line2), "Temperature LOW");
      else if (hum > (tgtHum + 2.0))              snprintf(line2, sizeof(line2), "Humidity HIGH");
      else if (hum < (tgtHum - 2.0))              snprintf(line2, sizeof(line2), "Humidity LOW");
      else if (people < 0 || people > _maxPeople) snprintf(line2, sizeof(line2), "Turnstile FAULT");

      _display.setTextSize(1);
      _display.setCursor(0, 0);  _display.print("FAULT");
      _display.setCursor(0, 18); _display.print(line2);
#else
      char line2[17] = "No faults";
      if      (temp > (tgtTemp + 1.0))           snprintf(line2, sizeof(line2), "Temperature HIGH");
      else if (temp < (tgtTemp - 1.0))           snprintf(line2, sizeof(line2), "Temperature LOW");
      else if (hum > (tgtHum + 2.0))             snprintf(line2, sizeof(line2), "Humidity HIGH");
      else if (hum < (tgtHum - 2.0))             snprintf(line2, sizeof(line2), "Humidity LOW");
      else if (people < 0 || people > _maxPeople) snprintf(line2, sizeof(line2), "Turnstile FAULT");

      _display.setCursor(0, 0); _display.print("FAULT");
      _display.setCursor(0, 1); _display.print(line2);
#endif
    }

  public:
    DisplayPanel(int maxPeople)
#if defined(ARDUINO)
      : _display(),
#else
      : _display(12, 13, A2, A3, A4, A5),
#endif
        _lastRefresh(0), _lastPageChange(0), _logoUntil(0), _page(0), _maxPeople(maxPeople) {
      _baseYear   = atoi(__DATE__ + 7);
      _baseMonth  = monthFromString(__DATE__);
      _baseDay    = (byte)atoi(__DATE__ + 4);
      _baseHour   = (byte)atoi(__TIME__);
      _baseMinute = (byte)atoi(__TIME__ + 3);
      _baseSecond = (byte)atoi(__TIME__ + 6);
    }

    DisplayPanel(int rs, int en, int d4, int d5, int d6, int d7, int maxPeople)
      : _display(rs, en, d4, d5, d6, d7),
        _lastRefresh(0), _lastPageChange(0), _logoUntil(0), _page(0), _maxPeople(maxPeople) {
      _baseYear   = atoi(__DATE__ + 7);
      _baseMonth  = monthFromString(__DATE__);
      _baseDay    = (byte)atoi(__DATE__ + 4);
      _baseHour   = (byte)atoi(__TIME__);
      _baseMinute = (byte)atoi(__TIME__ + 3);
      _baseSecond = (byte)atoi(__TIME__ + 6);
    }

    void begin() {
#if defined(ARDUINO)
      if (!_display.begin()) {
        Serial.println("ERR: OLED init failed (check I2C wiring/address)");
        return;
      }
      Serial.println("OK: OLED initialized");
      Serial.print("OLED I2C addr: 0x");
      Serial.println(_display.address(), HEX);

      _display.clear();
      _display.drawBitmap(48, 8, DEFAULT_LOGO_32x32, 32, 32);
      _display.setTextSize(1);
      _display.setCursor(8, 48);
      _display.print("Pinacoteca");
      _display.display();

      _logoUntil = millis() + 2500;
#else
      _display.begin(16, 2);
      _display.clear();
      _display.setCursor(0, 0);
      _display.print("Pinacoteca OK");
      _display.setCursor(0, 1);
      _display.print("Display started");
      delay(800);
      _display.clear();
#endif
    }

    // Update the display (called every loop iteration)
    void update(int currentPeople, Thermostat& thermostat,
                HumidifierControl& humidifier, LightingControl& lighting) {
      unsigned long now = millis();

#if defined(ARDUINO)
      if (_logoUntil != 0 && now < _logoUntil) {
        return;
      }
#endif

      if (now - _lastRefresh < 1000) return;
      _lastRefresh = now;

      float currentTemp = thermostat.getCurrentTemperature();
      float targetTemp  = thermostat.getTargetTemperature();
      float currentHum  = humidifier.getCurrentHumidity();
      float targetHum   = humidifier.getTargetHumidity();
      float currentLux  = lighting.getCurrentLux();
      int   targetLux   = lighting.getTargetLux();

      bool sensorError = hasSensorError(currentTemp, currentHum, currentLux);
      byte pageCount = sensorError ? 4 : 3;

      _display.clear();
      if (_page >= pageCount) _page = 0;

      if (_page == 0) {
        printDateTimePage();
      } else if (_page == 1) {
        printStatusPage(currentPeople, currentTemp, targetTemp,
                        currentHum, targetHum, currentLux, targetLux);
      } else if (_page == 2) {
        printFaultPage(currentPeople, currentTemp, targetTemp,
                       currentHum, targetHum);
      } else if (sensorError) {
        printErrorPage(currentTemp, currentHum, currentLux);
      } else {
        printDateTimePage();
      }

      _display.display();

      if (now - _lastPageChange >= 5000) {
        _page = (_page + 1) % pageCount;
        _lastPageChange = now;
      }
    }
};

const uint8_t DisplayPanel::DEFAULT_LOGO_32x32[128] = {
  0x00,0x00,0x00,0x00,0x07,0xFF,0xFF,0xE0,0x08,0x00,0x00,0x10,0x10,0x00,0x00,0x08,
  0x20,0x1F,0xF8,0x04,0x40,0x70,0x0E,0x02,0x40,0xC0,0x03,0x02,0x81,0x83,0xC1,0x01,
  0x81,0x06,0x60,0x81,0x81,0x0C,0x30,0x81,0x81,0x18,0x18,0x81,0x81,0x30,0x0C,0x81,
  0x81,0x60,0x06,0x81,0x81,0x60,0x06,0x81,0x81,0x30,0x0C,0x81,0x81,0x18,0x18,0x81,
  0x81,0x0C,0x30,0x81,0x81,0x06,0x60,0x81,0x81,0x83,0xC1,0x01,0x40,0xC0,0x03,0x02,
  0x40,0x70,0x0E,0x02,0x20,0x1F,0xF8,0x04,0x10,0x00,0x00,0x08,0x08,0x00,0x00,0x10,
  0x07,0xFF,0xFF,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#endif // LCD_H