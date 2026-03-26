// Pinacoteca.ino
// Main sketch for the Pinacoteca gallery management system.
// Manages: turnstile entry, climate control (temperature + humidity),
// gallery lighting, 16x2 LCD status display, and an optional
// Wi-Fi remote-control gateway (Arduino UNO R4 WiFi only).

#include <Servo.h>

#include "lib/servomotor/turnstile.h"
#include "lib/led/stoplight.h"

#include "lib/temperature/thermostat.h"
#include "lib/humidity/humidity.h"
#include "lib/humidity/humidifier.h"
#include "lib/lighting/photoresistor.h"
#include "lib/lighting/lighting_control.h"
#include "lib/display/lcd.h"

#if defined(ARDUINO_ARCH_AVR)
#define PINACOTECA_REMOTE_ENABLED 0
#else
#define PINACOTECA_REMOTE_ENABLED 1
#endif

#if PINACOTECA_REMOTE_ENABLED
#include "lib/wifi/wifi_link.h"
#include "lib/wifi/remote_gateway.h"
#endif

// --- Wi-Fi credentials ---
#define PINACOTECA_WIFI_SSID     ""
#define PINACOTECA_WIFI_PASSWORD ""
#define PINACOTECA_WIFI_PORT     7777

// --- Pin assignments ---
const int PIN_SERVOMOTOR       = 3;
const int PIN_GREEN_LIGHT      = 4;
const int PIN_RED_LIGHT        = 5;
const int PIN_IN_ULTRASONIC    = 6;
const int PIN_OUT_ULTRASONIC   = 7;
const int PIN_COOLING_RGB_BLUE = 8;   // Cooling (digital)
const int PIN_YELLOW_LIGHT     = 9;   // Heating
const int PIN_CEILING_LIGHTS   = 10;  // Gallery ceiling lights (PWM)
const int PIN_HUMIDIFIER_LED   = 11;

const int PIN_TEMPERATURE_SENSOR = A0;
const int PIN_PHOTORESISTOR      = A1;

const int PIN_LCD_RS = 12;
const int PIN_LCD_EN = 13;
const int PIN_LCD_D4 = A2;
const int PIN_LCD_D5 = A3;
const int PIN_LCD_D6 = A4;
const int PIN_LCD_D7 = A5;

// --- Global objects ---
Servo myServo;

const int           MAX_PEOPLE          = 5;
const float         TURNSTILE_MIN_CM    = 25.0;
const float         TARGET_TEMP_C       = 20.0;
const int           TARGET_LUX          = 200;
const float         TARGET_HUMIDITY     = 65.0;
const unsigned long THERMOSTAT_PAUSE_MS = 60000; // 60 seconds for real deployment

Turnstile         entranceTurnstile(PIN_IN_ULTRASONIC, PIN_OUT_ULTRASONIC,
                                    MAX_PEOPLE, TURNSTILE_MIN_CM);
Stoplight         trafficLight(PIN_GREEN_LIGHT, PIN_RED_LIGHT);
Thermostat        mainThermostat(PIN_TEMPERATURE_SENSOR, TARGET_TEMP_C,
                                 PIN_YELLOW_LIGHT, PIN_COOLING_RGB_BLUE,
                                 THERMOSTAT_PAUSE_MS);
LightingControl   galleryLighting(PIN_PHOTORESISTOR, PIN_CEILING_LIGHTS, TARGET_LUX);
HumidifierControl galleryHumidifier(PIN_HUMIDIFIER_LED, TARGET_HUMIDITY);
DisplayPanel      galleryDisplay(PIN_LCD_RS, PIN_LCD_EN,
                                 PIN_LCD_D4, PIN_LCD_D5,
                                 PIN_LCD_D6, PIN_LCD_D7, MAX_PEOPLE);

// --- Remote channel ---
#if PINACOTECA_REMOTE_ENABLED
WifiLink wifiConnection(PINACOTECA_WIFI_SSID,
                         PINACOTECA_WIFI_PASSWORD,
                         PINACOTECA_WIFI_PORT);

RemoteControlGateway remoteGateway(
    &mainThermostat,
    &galleryHumidifier,
    &galleryLighting,
    &entranceTurnstile,
    &myServo,
    PIN_GREEN_LIGHT,
    PIN_RED_LIGHT,
    PIN_YELLOW_LIGHT,
    PIN_COOLING_RGB_BLUE,
    PIN_HUMIDIFIER_LED,
    PIN_CEILING_LIGHTS,
    wifiConnection,
    1000);

bool isManualBypassEnabled() { return remoteGateway.isManualBypassEnabled(); }
void beginRemoteChannel()    { remoteGateway.begin(); }
void updateRemoteChannel()   { remoteGateway.update(); }

#else

bool isManualBypassEnabled() { return false; }
void beginRemoteChannel()    {}
void updateRemoteChannel()   {}

#endif

// --- Setup and Loop ---

void setup() {
  Serial.begin(9600);

  myServo.attach(PIN_SERVOMOTOR);
  myServo.write(0);
  entranceTurnstile.begin(&myServo);

  trafficLight.begin();
  mainThermostat.begin();
  galleryLighting.begin();
  galleryHumidifier.begin();
  galleryDisplay.begin();

  beginRemoteChannel();
}

void loop() {
  bool manualBypass = isManualBypassEnabled();

  if (!manualBypass) {
    // Turnstile
    entranceTurnstile.update();

    int currentPeople = entranceTurnstile.getPeopleCount();
    trafficLight.update(currentPeople, MAX_PEOPLE);

    // Climate
    mainThermostat.update();

    // Lighting
    galleryLighting.update();

    // Humidity
    galleryHumidifier.update();
  }

  int currentPeople = entranceTurnstile.getPeopleCount();

  // LCD display
  galleryDisplay.update(currentPeople, mainThermostat,
                        galleryHumidifier, galleryLighting);

  // Remote channel (Wi-Fi)
  updateRemoteChannel();
}