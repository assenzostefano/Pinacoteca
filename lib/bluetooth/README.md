# Remote Control Gateway (Native BLE + App Inventor)

Questo modulo espone lo stesso protocollo remoto su BLE nativo (UNO R4 WiFi),
con fallback seriale solo se ArduinoBLE non e disponibile in compilazione.

## Struttura file

- bluetooth_connection.h: interfaccia astratta del canale Bluetooth.
- command_processor.h: parsing protocollo e applicazione comandi ai moduli hardware.
- bluetooth_link.h: implementazione concreta BLE nativa (ArduinoBLE).
- remote_gateway.h: orchestrazione seriale/Bluetooth e invio periodico stato.

## Configurazione

Su UNO R4 WiFi non serve modulo HC-05 esterno.

Per App Inventor devi usare il componente `BluetoothLE` (non `BluetoothClient`).

Identificativi BLE esposti dal firmware:

- Nome periferica BLE: `Pinacoteca`
- Service UUID: `19B10000-E8F2-537E-4F6C-D104768A1214`
- Characteristic RX (write): `19B10001-E8F2-537E-4F6C-D104768A1214`
- Characteristic TX (notify/read): `19B10002-E8F2-537E-4F6C-D104768A1214`

Con fallback seriale (solo se BLE non disponibile), resta usata:

- PINACOTECA_BLUETOOTH_BAUD (default 9600)

## Protocollo comandi

Comandi supportati:

- PING -> PONG
- VER -> VER:1
- GET:STATE -> stato completo
- GET:MODE -> MODE:AUTO o MODE:MANUAL
- MANUAL:ON -> abilita bypass totale (OK:MANUAL:ON)
- MANUAL:OFF -> ripristina automazioni (OK:MANUAL:OFF)
- SET:TEMP:<valore> (range 15.0..30.0) -> OK:TEMP
- SET:HUM:<valore> (range 40.0..80.0) -> OK:HUM
- SET:LUX:<valore> (range 50..1200) -> OK:LUX
- SET:PEOPLE:<valore> (range 0..max) -> OK:PEOPLE
- SERVO:OPEN -> OK:SERVO:OPEN
- SERVO:CLOSE -> OK:SERVO:CLOSE
- SERVO:ANGLE:<0..180> -> OK:SERVO:ANGLE
- ACT:GREEN:ON|OFF
- ACT:RED:ON|OFF
- ACT:HEAT:ON|OFF
- ACT:COOL:ON|OFF
- ACT:HUMIDIFIER:ON|OFF
- ACT:PLAFONIERE:PWM:<0..255>

I comandi SERVO:* e ACT:* funzionano quando MANUAL:ON e' attivo.

Errori principali:

- ERR:EMPTY
- ERR:UNKNOWN
- ERR:RANGE:TEMP
- ERR:RANGE:HUM
- ERR:RANGE:LUX
- ERR:RANGE:SERVO
- ERR:RANGE:PEOPLE
- ERR:RANGE:PWM
- ERR:FORMAT:TEMP
- ERR:FORMAT:HUM
- ERR:FORMAT:LUX
- ERR:FORMAT:PEOPLE
- ERR:FORMAT:SERVO
- ERR:FORMAT:PWM
- ERR:SERVO
- ERR:MODE
- ERR:TOO_LONG

## Trasporto Bluetooth

Su BLE nativo, ogni comando viene scritto come stringa nella characteristic RX.
Le risposte vengono pubblicate nella characteristic TX.

In App Inventor:

- connettiti al dispositivo `Pinacoteca`
- fai `WriteStringValue` sulla characteristic RX
- leggi/notifica la characteristic TX per ricevere risposte e stati
