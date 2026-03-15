# Remote Control Gateway (Arduino UNO R4 WiFi)

Questo modulo espone lo stesso protocollo su:

- `Bluetooth LE` (BLE)

La comunicazione App Inventor avviene solo su BLE.

## Struttura file

- `bluetooth_connection.h`: interfaccia astratta del canale Bluetooth (contratto comune).
- `command_processor.h`: parsing protocollo e applicazione comandi ai moduli hardware.
- `ble_link.h`: implementazione concreta BLE di `BluetoothConnection`.
- `remote_gateway.h`: orchestrazione seriale/Bluetooth e invio periodico stato.

Per cambiare componente di comunicazione, basta creare una nuova classe che implementa `BluetoothConnection` e sostituire l'istanza passata al gateway in `Pinacoteca.ino`.

## Configurazione

Non serve configurare SSID o password.

## Protocollo comandi

Comandi supportati:

- `PING` -> `PONG`
- `VER` -> `VER:1`
- `GET:STATE` -> stato completo
- `GET:MODE` -> `MODE:AUTO` o `MODE:MANUAL`
- `MANUAL:ON` -> abilita bypass totale (`OK:MANUAL:ON`)
- `MANUAL:OFF` -> ripristina automazioni (`OK:MANUAL:OFF`)
- `SET:TEMP:<valore>` (range `15.0..30.0`) -> `OK:TEMP`
- `SET:HUM:<valore>` (range `40.0..80.0`) -> `OK:HUM`
- `SET:LUX:<valore>` (range `50..1200`) -> `OK:LUX`
- `SET:PEOPLE:<valore>` (range `0..max`) -> `OK:PEOPLE`
- `SERVO:OPEN` -> `OK:SERVO:OPEN`
- `SERVO:CLOSE` -> `OK:SERVO:CLOSE`
- `SERVO:ANGLE:<0..180>` -> `OK:SERVO:ANGLE`
- `ACT:GREEN:ON|OFF`
- `ACT:RED:ON|OFF`
- `ACT:HEAT:ON|OFF`
- `ACT:COOL:ON|OFF`
- `ACT:HUMIDIFIER:ON|OFF`
- `ACT:PLAFONIERE:PWM:<0..255>`

I comandi `SERVO:*` e `ACT:*` funzionano quando `MANUAL:ON` è attivo.

Errori principali:

- `ERR:EMPTY`
- `ERR:UNKNOWN`
- `ERR:RANGE:TEMP`
- `ERR:RANGE:HUM`
- `ERR:RANGE:LUX`
- `ERR:RANGE:SERVO`
- `ERR:RANGE:PEOPLE`
- `ERR:RANGE:PWM`
- `ERR:FORMAT:TEMP`
- `ERR:FORMAT:HUM`
- `ERR:FORMAT:LUX`
- `ERR:FORMAT:PEOPLE`
- `ERR:FORMAT:SERVO`
- `ERR:FORMAT:PWM`
- `ERR:SERVO`
- `ERR:MODE` (comando manuale usato in `AUTO`)
- `ERR:TOO_LONG` (comando seriale oltre buffer)

Formato stato:

`STATE:T=21.8;H=58.0;L=170;P=3;S=90;M=MANUAL;TT=22.5;TH=60.0;TL=180`

## BLE (App Inventor)

Service UUID:

- `19B10000-E8F2-537E-4F6C-D104768A1214`

Characteristic RX (app -> board, write):

- `19B10001-E8F2-537E-4F6C-D104768A1214`

Characteristic TX (board -> app, read/notify):

- `19B10002-E8F2-537E-4F6C-D104768A1214`

## Nota per App Inventor

Invia i comandi testuali esattamente come sopra. Scrivi su RX e leggi/notifica da TX.