# Pinacoteca - Unit Test (GoogleTest)

## Requisiti
- CMake >= 3.16
- Compilatore C++17
- Connessione internet alla prima configurazione (FetchContent scarica GoogleTest)

## Build & Run
```bash
cd test
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Note
- I test usano mock locali delle API Arduino in `test/mocks/`:
  - `Arduino.h`
  - `Servo.h`
  - `DHT.h`
  - `LiquidCrystal.h`
- La suite copre i moduli:
  - `led`, `stoplight`
  - `servomotor`, `turnstile`
  - `temperature`, `thermostat`
  - `humidity`, `humidifier`
  - `photoresistor`, `lighting_control`
  - `display`, `lcd`

- Oltre ai test C++, viene eseguito anche un test di integrazione Python per il mock BLE virtuale:
  - `test/mock_server_tests.py`
  - valida connect/write/read/disconnect, endpoint sensori/attuatori e compatibilita legacy.
