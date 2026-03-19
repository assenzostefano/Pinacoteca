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
#include "lib/bluetooth/ble_link.h"
#include "lib/bluetooth/remote_gateway.h"
#endif

// Setup PIN
const int IS_HUMIDIFIER_LED = 11;
const int PIN_SERVOMOTOR = 3;
const int PIN_GREEN_LIGHT = 4;
const int PIN_RED_LIGHT = 5;
const int PIN_IN_BUTTON = 6;
const int PIN_OUT_BUTTON = 7;

const int PIN_BLU_LIGHT = 8; // Cooling
const int PIN_YELLOW_LIGHT = 9; // Heating

const int PIN_PLAFONIERE = 10;

const int PIN_TEMPERATURE_SENSOR = A0;
const int PIN_PHOTORESISTOR = A1;

const int PIN_LCD_RS = 12;
const int PIN_LCD_EN = 13;
const int PIN_LCD_D4 = A2;
const int PIN_LCD_D5 = A3;
const int PIN_LCD_D6 = A4;
const int PIN_LCD_D7 = A5;

Servo myServo;

const int MAX_PEOPLE = 5;
const float TARGET_TEMP_C = 20.0;
const int TARGET_LUX = 200;
const float TARGET_HUMIDITY = 65.0;
const unsigned long THERMOSTAT_PAUSE_MS = 60000; // Impostato a 60s per il deploy reale

Turnstile entranceTurnstile(PIN_IN_BUTTON, PIN_OUT_BUTTON, MAX_PEOPLE);
Stoplight trafficLight(PIN_GREEN_LIGHT, PIN_RED_LIGHT);
Thermostat mainThermostat(PIN_TEMPERATURE_SENSOR, TARGET_TEMP_C, PIN_YELLOW_LIGHT, PIN_BLU_LIGHT, THERMOSTAT_PAUSE_MS);
LightingControl galleryLighting(PIN_PHOTORESISTOR, PIN_PLAFONIERE, TARGET_LUX);
HumidifierControl galleryHumidifier(IS_HUMIDIFIER_LED, TARGET_HUMIDITY);
DisplayPanel galleryDisplay(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7, MAX_PEOPLE);

#if PINACOTECA_REMOTE_ENABLED
BleLink bluetoothConnection;
RemoteControlGateway remoteGateway(
    &mainThermostat,
    &galleryHumidifier,
    &galleryLighting,
    &entranceTurnstile,
    &myServo,
    PIN_GREEN_LIGHT,
    PIN_RED_LIGHT,
    PIN_YELLOW_LIGHT,
    PIN_BLU_LIGHT,
    IS_HUMIDIFIER_LED,
    PIN_PLAFONIERE,
    bluetoothConnection,
    1000
);
#endif

void setup() {
    Serial.begin(9600);
    
    // Initialize the servo motor
    myServo.attach(PIN_SERVOMOTOR);
    myServo.write(0);
    entranceTurnstile.begin(&myServo);
    
    trafficLight.begin(); // Initialize the traffic light
    mainThermostat.begin(); // Initialize the thermostat
    galleryLighting.begin(); // Initialize lighting control
    galleryHumidifier.begin(); // Initialize humidity control
    galleryDisplay.begin(); // Initialize LCD display

#if PINACOTECA_REMOTE_ENABLED
    remoteGateway.begin();
#endif
}

void loop() {

#if PINACOTECA_REMOTE_ENABLED
    bool isManualBypass = remoteGateway.isManualBypassEnabled();
#else
    bool isManualBypass = false;
#endif

    if (!isManualBypass) {
        // Turnstile
        entranceTurnstile.update();

        // Current people count
        int currentPeople = entranceTurnstile.getPeopleCount();

        // Update traffic light based on current people count
        trafficLight.update(currentPeople, 5);

        // Thermostat
        mainThermostat.update();

        // Lighting (max 200 LUX)
        galleryLighting.update();

        // Humidity Control (target 65%)
        galleryHumidifier.update();
    }

    int currentPeople = entranceTurnstile.getPeopleCount();

    // LCD Display
    galleryDisplay.update(currentPeople, mainThermostat, galleryHumidifier, galleryLighting);

    // Remote channel (Bluetooth LE)

#if PINACOTECA_REMOTE_ENABLED
    remoteGateway.update();
#endif
}