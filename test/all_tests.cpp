#include <gtest/gtest.h>

#include "Arduino.h"
#include "Servo.h"
#include "DHT.h"
#include "LiquidCrystal.h"

#include "../lib/led/led.h"
#include "../lib/led/stoplight.h"
#include "../lib/servomotor/servomotor.h"
#include "../lib/servomotor/turnstile.h"
#include "../lib/temperature/temperature.h"
#include "../lib/temperature/thermostat.h"
#include "../lib/humidity/humidity.h"
#include "../lib/humidity/humidifier.h"
#include "../lib/lighting/photoresistor.h"
#include "../lib/lighting/lighting_control.h"
#include "../lib/display/display.h"
#include "../lib/display/lcd.h"

class BaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        resetArduinoMocks();
        Serial.clear();
    }
};

TEST_F(BaseTest, LedWriteDigitalState) {
    EXPECT_TRUE(led(4, HIGH));
    EXPECT_EQ(__mock_digital_write[4], HIGH);

    EXPECT_TRUE(led(4, LOW));
    EXPECT_EQ(__mock_digital_write[4], LOW);
}

TEST_F(BaseTest, LedDimmingWritesPwm) {
    EXPECT_TRUE(ledDimming(10, 128));
    EXPECT_EQ(__mock_analog_write[10], 128);
}

TEST_F(BaseTest, StoplightBelowMaxSetsGreen) {
    Stoplight sl(4, 5);
    sl.begin();
    sl.update(2, 5);
    EXPECT_EQ(__mock_pin_mode[4], OUTPUT);
    EXPECT_EQ(__mock_pin_mode[5], OUTPUT);
    EXPECT_EQ(__mock_digital_write[4], HIGH);
    EXPECT_EQ(__mock_digital_write[5], LOW);
}

TEST_F(BaseTest, StoplightAtMaxSetsRed) {
    Stoplight sl(4, 5);
    sl.update(5, 5);
    EXPECT_EQ(__mock_digital_write[4], LOW);
    EXPECT_EQ(__mock_digital_write[5], HIGH);
}

TEST_F(BaseTest, AntiSufferingServoAllowsSafeMove) {
    Servo s;
    s.write(90);
    EXPECT_TRUE(antiSufferingServo(20, s));
    EXPECT_EQ(s.read(), 110);
}

TEST_F(BaseTest, AntiSufferingServoBlocksOutOfRange) {
    Servo s;
    s.write(170);
    EXPECT_FALSE(antiSufferingServo(20, s));
    ASSERT_FALSE(Serial.logs.empty());
    EXPECT_EQ(Serial.logs.back(), "Limit movement Servo Motor");
}

TEST_F(BaseTest, TurnstileIncrementsAndDecrementsPeople) {
    Turnstile t(6, 7, 5);
    Servo s;
    s.write(0);
    t.begin(&s);

    __mock_digital_read[6] = HIGH;
    __mock_digital_read[7] = LOW;
    t.update();
    EXPECT_EQ(t.getPeopleCount(), 1);

    __mock_digital_read[6] = LOW;
    __mock_digital_read[7] = HIGH;
    t.update();
    EXPECT_EQ(t.getPeopleCount(), 0);
}

TEST_F(BaseTest, TurnstileRespectsMaxPeople) {
    Turnstile t(6, 7, 1);
    Servo s;
    s.write(0);
    t.begin(&s);

    __mock_digital_read[6] = HIGH;
    t.update();
    t.update();
    EXPECT_EQ(t.getPeopleCount(), 1);
}

TEST_F(BaseTest, TurnstileBeginConfiguresPins) {
    Turnstile t(6, 7, 5);
    Servo s;
    t.begin(&s);
    EXPECT_EQ(__mock_pin_mode[6], INPUT);
    EXPECT_EQ(__mock_pin_mode[7], INPUT);
}

TEST_F(BaseTest, TemperatureReaderReturnsErrorAtExtremes) {
    __mock_analog_read[A0] = 0;
    EXPECT_FLOAT_EQ(readTemperatureCelsius(A0), -999.0f);

    __mock_analog_read[A0] = 1023;
    EXPECT_FLOAT_EQ(readTemperatureCelsius(A0), -999.0f);
}

TEST_F(BaseTest, TemperatureReaderReturnsFiniteForValidInput) {
    __mock_analog_read[A0] = 512;
    float t = readTemperatureCelsius(A0);
    EXPECT_TRUE(std::isfinite(t));
    EXPECT_NE(t, -999.0f);
}

TEST_F(BaseTest, ThermostatSensorErrorTurnsEverythingOff) {
    Thermostat th(A0, 20.0f, 9, 8);
    th.begin();

    __mock_analog_read[A0] = 0;
    EXPECT_FALSE(th.update());
    EXPECT_EQ(__mock_digital_write[9], LOW);
    EXPECT_EQ(__mock_digital_write[8], LOW);
}

TEST_F(BaseTest, ThermostatBeginSetsPinModesAndSetGetTarget) {
    Thermostat th(A0, 20.0f, 9, 8);
    th.begin();
    EXPECT_EQ(__mock_pin_mode[A0], INPUT);
    EXPECT_EQ(__mock_pin_mode[9], OUTPUT);
    EXPECT_EQ(__mock_pin_mode[8], OUTPUT);

    EXPECT_FLOAT_EQ(th.getTargetTemperature(), 20.0f);
    th.setTargetTemperature(22.5f);
    EXPECT_FLOAT_EQ(th.getTargetTemperature(), 22.5f);
}

TEST_F(BaseTest, ThermostatHeatingBranch) {
    Thermostat th(A0, 25.0f, 9, 8);
    __mock_analog_read[A0] = 700;
    __mock_millis = 5000;
    EXPECT_TRUE(th.update());
    EXPECT_TRUE(th.update());
    EXPECT_EQ(__mock_digital_write[9], HIGH);
    EXPECT_EQ(__mock_digital_write[8], LOW);
}

TEST_F(BaseTest, ThermostatCoolingBranch) {
    Thermostat th(A0, 10.0f, 9, 8);
    __mock_analog_read[A0] = 500;
    __mock_millis = 5000;
    EXPECT_TRUE(th.update());
    EXPECT_TRUE(th.update());
    EXPECT_EQ(__mock_digital_write[9], LOW);
    EXPECT_EQ(__mock_digital_write[8], HIGH);
}

TEST_F(BaseTest, HumidityReadAndError) {
    dht_sensor.setHumidity(66.0f);
    EXPECT_FLOAT_EQ(readHumidity(), 66.0f);

    dht_sensor.setHumidity(NAN);
    EXPECT_FLOAT_EQ(readHumidity(), -999.0f);
}

TEST_F(BaseTest, HumidifierHighHumidityTurnsOnPin) {
    HumidifierControl hc(11, 65.0f);
    hc.begin();
    dht_sensor.setHumidity(80.0f);
    EXPECT_TRUE(hc.update());
    EXPECT_EQ(__mock_digital_write[11], HIGH);
}

TEST_F(BaseTest, HumidifierLowHumidityTurnsOffPin) {
    HumidifierControl hc(11, 65.0f);
    hc.begin();
    dht_sensor.setHumidity(50.0f);
    EXPECT_TRUE(hc.update());
    EXPECT_EQ(__mock_digital_write[11], LOW);
}

TEST_F(BaseTest, HumidifierSensorErrorReturnsFalse) {
    HumidifierControl hc(11, 65.0f);
    hc.begin();
    dht_sensor.setHumidity(NAN);
    EXPECT_FALSE(hc.update());
    EXPECT_EQ(__mock_digital_write[11], LOW);
}

TEST_F(BaseTest, HumidifierSetGetTargetWorks) {
    HumidifierControl hc(11, 65.0f);
    EXPECT_FLOAT_EQ(hc.getTargetHumidity(), 65.0f);
    hc.setTargetHumidity(70.0f);
    EXPECT_FLOAT_EQ(hc.getTargetHumidity(), 70.0f);
}

TEST_F(BaseTest, PhotoresistorLuxEdgeCases) {
    __mock_analog_read[A1] = 0;
    EXPECT_FLOAT_EQ(readLux(A1), 0.0f);

    __mock_analog_read[A1] = 1023;
    EXPECT_FLOAT_EQ(readLux(A1), 9999.0f);
}

TEST_F(BaseTest, LightingControlWritesPwm) {
    LightingControl lc(A1, 10, 200);
    lc.begin();

    __mock_analog_read[A1] = 0;
    EXPECT_TRUE(lc.update());
    EXPECT_EQ(__mock_analog_write[10], 255);

    __mock_analog_read[A1] = 1023;
    EXPECT_TRUE(lc.update());
    EXPECT_EQ(__mock_analog_write[10], 0);
}

TEST_F(BaseTest, LightingControlBeginAndSetGetTarget) {
    LightingControl lc(A1, 10, 200);
    lc.begin();
    EXPECT_EQ(__mock_pin_mode[A1], INPUT);
    EXPECT_EQ(__mock_pin_mode[10], OUTPUT);

    EXPECT_EQ(lc.getTargetLux(), 200);
    lc.setTargetLux(250);
    EXPECT_EQ(lc.getTargetLux(), 250);
}

TEST_F(BaseTest, DisplayInterfaceBasicOperations) {
    DisplayInterface di(12, 13, A2, A3, A4, A5);
    di.begin(16, 2);
    di.clear();
    di.setCursor(0, 0);
    di.print("HELLO");

    ASSERT_NE(LiquidCrystal::lastInstance, nullptr);
    EXPECT_TRUE(LiquidCrystal::lastInstance->line(0).find("HELLO") != std::string::npos);
}

TEST_F(BaseTest, DisplayPanelRotatesPages) {
    Thermostat th(A0, 20.0f, 9, 8);
    HumidifierControl hc(11, 65.0f);
    LightingControl lc(A1, 10, 200);
    DisplayPanel dp(12, 13, A2, A3, A4, A5, 5);

    th.begin();
    hc.begin();
    lc.begin();
    dp.begin();

    __mock_analog_read[A0] = 500;
    __mock_analog_read[A1] = 200;
    dht_sensor.setHumidity(60.0f);

    __mock_millis = 1000;
    dp.update(2, th, hc, lc);
    std::string page0 = LiquidCrystal::lastInstance->line(0);
    EXPECT_TRUE(page0.find("/") != std::string::npos);

    __mock_millis = 6000;
    dp.update(2, th, hc, lc);
    std::string page1 = LiquidCrystal::lastInstance->line(0);
    EXPECT_TRUE(page1.find("T:") != std::string::npos);

    __mock_millis = 11000;
    dp.update(2, th, hc, lc);
    std::string page2 = LiquidCrystal::lastInstance->line(0);
    EXPECT_TRUE(page2.find("Error") != std::string::npos || page2.find("Fault") != std::string::npos);
}

TEST_F(BaseTest, DisplayDatePageHasValidWeekdayPrefix) {
    Thermostat th(A0, 20.0f, 9, 8);
    HumidifierControl hc(11, 65.0f);
    LightingControl lc(A1, 10, 200);
    DisplayPanel dp(12, 13, A2, A3, A4, A5, 5);

    th.begin();
    hc.begin();
    lc.begin();
    dp.begin();

    __mock_analog_read[A0] = 500;
    __mock_analog_read[A1] = 200;
    dht_sensor.setHumidity(60.0f);

    __mock_millis = 1000;
    dp.update(1, th, hc, lc);

    std::string line0 = LiquidCrystal::lastInstance->line(0);
    std::string line1 = LiquidCrystal::lastInstance->line(1);

    std::string day = line0.substr(0, 3);
    bool validDay = (day == "Mon" || day == "Tue" || day == "Wed" || day == "Thu" || day == "Fri" || day == "Sat" || day == "Sun");
    EXPECT_TRUE(validDay);
    EXPECT_TRUE(line1.find("Hour") != std::string::npos);
}

TEST_F(BaseTest, DisplayStatusPageShowsTurnstileCountExactly) {
    Thermostat th(A0, 20.0f, 9, 8);
    HumidifierControl hc(11, 65.0f);
    LightingControl lc(A1, 10, 200);
    DisplayPanel dp(12, 13, A2, A3, A4, A5, 5);

    th.begin();
    hc.begin();
    lc.begin();
    dp.begin();

    __mock_analog_read[A0] = 500;
    __mock_analog_read[A1] = 200;
    dht_sensor.setHumidity(60.0f);

    __mock_millis = 6000;
    dp.update(5, th, hc, lc);
    std::string line1 = LiquidCrystal::lastInstance->line(1);
    EXPECT_TRUE(line1.find("TR:5/5") != std::string::npos);
}

TEST_F(BaseTest, DisplayErrorPageShowsHumidityHigh) {
    Thermostat th(A0, 20.0f, 9, 8);
    HumidifierControl hc(11, 65.0f);
    LightingControl lc(A1, 10, 200);
    DisplayPanel dp(12, 13, A2, A3, A4, A5, 5);

    th.begin();
    hc.begin();
    lc.begin();
    dp.begin();

    __mock_analog_read[A0] = 569;
    __mock_analog_read[A1] = 200;
    dht_sensor.setHumidity(90.0f);

    __mock_millis = 6000;
    dp.update(2, th, hc, lc); // page 1

    __mock_millis = 11000;
    dp.update(2, th, hc, lc);
    std::string line1 = LiquidCrystal::lastInstance->line(1);
    EXPECT_TRUE(line1.find("Humidity HIGH!") != std::string::npos);
}

TEST_F(BaseTest, DisplayErrorPageShowsTurnstileError) {
    Thermostat th(A0, 20.0f, 9, 8);
    HumidifierControl hc(11, 65.0f);
    LightingControl lc(A1, 10, 200);
    DisplayPanel dp(12, 13, A2, A3, A4, A5, 5);

    th.begin();
    hc.begin();
    lc.begin();
    dp.begin();

    __mock_analog_read[A0] = 569;
    __mock_analog_read[A1] = 200;
    dht_sensor.setHumidity(65.0f);

    __mock_millis = 6000;
    dp.update(7, th, hc, lc); // page 1

    __mock_millis = 11000;
    dp.update(7, th, hc, lc);
    std::string line1 = LiquidCrystal::lastInstance->line(1);
    EXPECT_TRUE(line1.find("Turnstile ERROR") != std::string::npos);
}
