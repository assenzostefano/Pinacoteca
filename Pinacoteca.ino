/**
 * @file Pinacoteca.ino
 * @brief Main sketch for the Pinacoteca gallery management system.
 *
 * Manages: turnstile entry, climate control (temperature + humidity),
 * gallery lighting, small OLED status display, and an optional
 * Bluetooth remote-control gateway for App Inventor style commands.
 */

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
#include "lib/bluetooth/bluetooth_link.h"
#include "lib/bluetooth/remote_gateway.h"
#endif

// ── Pin assignments ──────────────────────────────────────────────
constexpr uint8_t PIN_SERVOMOTOR       = 3;
constexpr uint8_t PIN_GREEN_LIGHT      = 4;
constexpr uint8_t PIN_RED_LIGHT        = 5;
constexpr uint8_t PIN_IN_ECHO          = 6;
constexpr uint8_t PIN_IN_TRIG          = 12;  // Separato
constexpr uint8_t PIN_OUT_ECHO         = 7;
constexpr uint8_t PIN_OUT_TRIG         = 12;  // Separato
constexpr uint8_t PIN_COOLING_RGB_BLUE = 8;   // Cooling (digital)
constexpr uint8_t PIN_YELLOW_LIGHT     = 9;   // Heating
constexpr uint8_t PIN_CEILING_LIGHTS   = 10;  // Gallery ceiling lights (PWM)
constexpr uint8_t PIN_HUMIDIFIER_LED   = 11;

constexpr uint8_t PIN_TEMPERATURE_SENSOR = A0;
constexpr uint8_t PIN_PHOTORESISTOR      = A1;

// ── System parameters ────────────────────────────────────────────
constexpr uint8_t       MAX_PEOPLE          = 5;
constexpr float         TURNSTILE_MIN_CM    = 25.0f;
constexpr float         TARGET_TEMP_C       = 20.0f;
constexpr uint16_t      TARGET_LUX          = 200;
constexpr float         TARGET_HUMIDITY     = 65.0f;
constexpr unsigned long THERMOSTAT_PAUSE_MS = 60000UL;

// ── Global objects ───────────────────────────────────────────────
Servo myServo;

Turnstile         entranceTurnstile(PIN_IN_TRIG, PIN_IN_ECHO,
                                    PIN_OUT_TRIG, PIN_OUT_ECHO,
                                    MAX_PEOPLE, TURNSTILE_MIN_CM);
Stoplight         trafficLight(PIN_GREEN_LIGHT, PIN_RED_LIGHT);
Thermostat        mainThermostat(PIN_TEMPERATURE_SENSOR, TARGET_TEMP_C,
                                 PIN_YELLOW_LIGHT, PIN_COOLING_RGB_BLUE,
                                 THERMOSTAT_PAUSE_MS);
LightingControl   galleryLighting(PIN_PHOTORESISTOR, PIN_CEILING_LIGHTS, TARGET_LUX);
HumidifierControl galleryHumidifier(PIN_HUMIDIFIER_LED, TARGET_HUMIDITY);
DisplayPanel      galleryDisplay(MAX_PEOPLE);

// ── Remote channel (Bluetooth) ──────────────────────────────────
#if PINACOTECA_REMOTE_ENABLED
constexpr unsigned long PINACOTECA_BLUETOOTH_BAUD = 9600UL;

BluetoothLink bluetoothConnection(PINACOTECA_BLUETOOTH_BAUD);

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
    bluetoothConnection,
    1000);

bool isManualBypassEnabled() { return remoteGateway.isManualBypassEnabled(); }
void beginRemoteChannel()    { remoteGateway.begin(); }
void updateRemoteChannel()   { remoteGateway.update(); }

#else

bool isManualBypassEnabled() { return false; }
void beginRemoteChannel()    {}
void updateRemoteChannel()   {}

#endif

// ── Setup & Loop ─────────────────────────────────────────────────

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
  // Service BLE early so discovery/ATT traffic is handled promptly.
  updateRemoteChannel();

  const bool manualBypass = isManualBypassEnabled();

  if (!manualBypass) {
    entranceTurnstile.update();
    updateRemoteChannel();

    trafficLight.update(entranceTurnstile.getPeopleCount(), MAX_PEOPLE);
    mainThermostat.update();
    galleryLighting.update();
    galleryHumidifier.update();
  }

  // Give BLE another chance before display refresh (may trigger sensor reads).
  updateRemoteChannel();

  galleryDisplay.update(entranceTurnstile,
                        mainThermostat, galleryHumidifier, galleryLighting);

  updateRemoteChannel();
}
