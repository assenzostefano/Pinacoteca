# Test App Inventor senza hardware (simulazione Wi-Fi)

Questa guida simula il flusso Wi-Fi reale (connect / write RX / read TX / disconnect) usando il componente Web di App Inventor, senza Arduino fisico.

## 1) Avvio mock server

Nel progetto esegui:

`make mock-server`

Base URL:

- Emulatore sul PC: `http://127.0.0.1:8080`
- Telefono reale in LAN: `http://<IP_DEL_PC>:8080`

## 2) Endpoint Wi-Fi virtuali

- `GET /wifi/info`  
	metadati Wi-Fi (nome device, SSID mock, porta TCP)

- `GET /wifi/connect?client=<nome>`  
	apre una sessione e restituisce `OK:CONNECTED:SID=<session_id>`

- `GET /wifi/write?sid=<session_id>&c=<comando_url_encoded>`  
	simula invio di una riga testuale su socket TCP. Mette in coda risposta e stato.

- `GET /wifi/read?sid=<session_id>`  
	simula lettura della coda messaggi (una riga alla volta). Se vuota: `NO_DATA`.

- `GET /wifi/state?sid=<session_id>`  
	ritorna stato corrente completo in formato protocollo firmware.

- `GET /wifi/disconnect?sid=<session_id>`  
	chiude la sessione.

## 3) Endpoint simulazione sensori e diagnostica

- `GET /wifi/set_sensor?t=<temp>&h=<hum>&l=<lux>&p=<people>`  
	aggiorna valori sensori simulati e genera notifica stato.

- `GET /wifi/actuators`  
	stato attuatori simulati (green/red/heat/cool/humidifier/plafoniere pwm).

## 4) Compatibilità legacy (se ti serve)

- `GET /cmd?c=<comando_url_encoded>`
- `GET /state`

## 5) Sequenza App Inventor consigliata (Wi-Fi TCP)

1. `GET /wifi/connect?client=AI2` -> salva `SID`
2. `GET /wifi/read?sid=<SID>` -> `WIFI:CONNECTED`
3. `GET /wifi/read?sid=<SID>` -> `STATE:...`
4. `GET /wifi/write?sid=<SID>&c=PING`
5. `GET /wifi/read?sid=<SID>` -> `PONG`
6. `GET /wifi/read?sid=<SID>` -> `STATE:...`
7. `GET /wifi/write?sid=<SID>&c=MANUAL%3AON`
8. `GET /wifi/write?sid=<SID>&c=SERVO%3AOPEN`
9. `GET /wifi/read?sid=<SID>` (2 volte) -> `OK:...` e nuovo `STATE:...`
10. `GET /wifi/disconnect?sid=<SID>`

## 6) Comandi supportati (allineati al firmware)

- Base: `PING`, `VER`, `GET:MODE`, `GET:STATE`
- Modalita: `MANUAL:ON`, `MANUAL:OFF`
- Setpoint: `SET:TEMP:<15..30>`, `SET:HUM:<40..80>`, `SET:LUX:<50..1200>`, `SET:PEOPLE:<0..5>`
- Servo: `SERVO:OPEN`, `SERVO:CLOSE`, `SERVO:ANGLE:<0..180>`
- Attuatori: `ACT:GREEN:ON/OFF`, `ACT:RED:ON/OFF`, `ACT:HEAT:ON/OFF`, `ACT:COOL:ON/OFF`, `ACT:HUMIDIFIER:ON/OFF`, `ACT:PLAFONIERE:PWM:<0..255>`

Errori principali: `ERR:EMPTY`, `ERR:MODE`, `ERR:FORMAT:*`, `ERR:RANGE:*`, `ERR:UNKNOWN`, `ERR:SESSION`.

## 7) Limiti noti

Questa simulazione replica il protocollo applicativo Wi-Fi, ma non testa stack Wi-Fi reale (RSSI, pairing, interferenze, timing hardware).