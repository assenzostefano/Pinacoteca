# Remote Control Gateway (Arduino UNO R4 WiFi)

Questo modulo espone lo stesso protocollo remoto via Wi-Fi (TCP testuale).

## Struttura file

- wifi_connection.h: interfaccia astratta del canale Wi-Fi.
- command_processor.h: parsing protocollo e applicazione comandi ai moduli hardware.
- wifi_link.h: implementazione concreta Wi-Fi TCP di WifiConnection.
- remote_gateway.h: orchestrazione seriale/Wi-Fi e invio periodico stato.

## Configurazione

Nel file Pinacoteca.ino imposta:

- PINACOTECA_WIFI_SSID
- PINACOTECA_WIFI_PASSWORD
- PINACOTECA_WIFI_PORT

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

## Trasporto Wi-Fi

Il canale usa socket TCP testuale (una riga per comando/risposta) sulla porta configurata.
