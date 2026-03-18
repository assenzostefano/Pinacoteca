#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
from collections import deque
import json
import re
import uuid
import os


STATE = {
    "T": 21.8,
    "H": 58.0,
    "L": 170,
    "P": 3,
    "S": 0,
    "M": "AUTO",
    "TT": 22.5,
    "TH": 60.0,
    "TL": 180,
}

ACTUATORS = {
    "GREEN": 0,
    "RED": 1,
    "HEAT": 0,
    "COOL": 0,
    "HUMIDIFIER": 0,
    "PLAFONIERE_PWM": 0,
}

BLE_INFO = {
    "device_name": "Pinacoteca-Mock",
    "service_uuid": "12345678-1234-1234-1234-1234567890ab",
    "rx_char_uuid": "12345678-1234-1234-1234-1234567890ac",
    "tx_char_uuid": "12345678-1234-1234-1234-1234567890ad",
    "protocol": "text-line",
}

BLE_SESSIONS = {}

STRICT_INT_RE = re.compile(r"^[+-]?\d+$")
STRICT_FLOAT_RE = re.compile(r"^[+-]?(?:\d+\.\d+|\d+|\.\d+)$")


def build_state():
    return (
        f"STATE:T={STATE['T']:.1f};H={STATE['H']:.1f};L={STATE['L']};P={STATE['P']};"
        f"S={STATE['S']};M={STATE['M']};TT={STATE['TT']:.1f};TH={STATE['TH']:.1f};TL={STATE['TL']}"
    )


def parse_int(value):
    if not isinstance(value, str) or not STRICT_INT_RE.match(value.strip()):
        return 0, False
    try:
        return int(value.strip()), True
    except Exception:
        return 0, False


def parse_float(value):
    if not isinstance(value, str) or not STRICT_FLOAT_RE.match(value.strip()):
        return 0.0, False
    try:
        return float(value.strip()), True
    except Exception:
        return 0.0, False


def enqueue_for_all_sessions(message):
    for session in BLE_SESSIONS.values():
        session["tx_queue"].append(message)


def push_state_notification():
    enqueue_for_all_sessions(build_state())


def create_session(client_name):
    sid = uuid.uuid4().hex[:12].upper()
    BLE_SESSIONS[sid] = {
        "client": client_name or "APPINVENTOR",
        "connected": True,
        "tx_queue": deque(),
    }
    BLE_SESSIONS[sid]["tx_queue"].append("BLE:CONNECTED")
    BLE_SESSIONS[sid]["tx_queue"].append(build_state())
    return sid


def get_session(sid):
    if not sid:
        return None
    return BLE_SESSIONS.get(sid)


def close_session(sid):
    session = get_session(sid)
    if not session:
        return False
    session["connected"] = False
    del BLE_SESSIONS[sid]
    return True


def set_simulated_sensor(query):
    changed = False

    if "t" in query:
        value, ok = parse_float(query["t"][0])
        if not ok:
            return "ERR:FORMAT:T"
        if value < -20.0 or value > 60.0:
            return "ERR:RANGE:T"
        STATE["T"] = value
        changed = True

    if "h" in query:
        value, ok = parse_float(query["h"][0])
        if not ok:
            return "ERR:FORMAT:H"
        if value < 0.0 or value > 100.0:
            return "ERR:RANGE:H"
        STATE["H"] = value
        changed = True

    if "l" in query:
        value, ok = parse_int(query["l"][0])
        if not ok:
            return "ERR:FORMAT:L"
        if value < 0 or value > 4095:
            return "ERR:RANGE:L"
        STATE["L"] = value
        changed = True

    if "p" in query:
        value, ok = parse_int(query["p"][0])
        if not ok:
            return "ERR:FORMAT:P"
        if value < 0 or value > 5:
            return "ERR:RANGE:P"
        STATE["P"] = value
        changed = True

    if changed:
        push_state_notification()
        return "OK:SENSORS"

    return "ERR:BAD_REQUEST"


def process_command(command):
    command = (command or "").strip().upper()
    if not command:
        return "ERR:EMPTY"

    if command == "PING":
        return "PONG"
    if command == "VER":
        return "VER:1"
    if command == "GET:MODE":
        return f"MODE:{STATE['M']}"
    if command == "GET:STATE":
        return build_state()
    if command == "MANUAL:ON":
        STATE["M"] = "MANUAL"
        push_state_notification()
        return "OK:MANUAL:ON"
    if command == "MANUAL:OFF":
        STATE["M"] = "AUTO"
        push_state_notification()
        return "OK:MANUAL:OFF"

    if command.startswith("SET:TEMP:"):
        value, ok = parse_float(command[9:])
        if not ok:
            return "ERR:FORMAT:TEMP"
        if value < 15.0 or value > 30.0:
            return "ERR:RANGE:TEMP"
        STATE["TT"] = value
        push_state_notification()
        return "OK:TEMP"

    if command.startswith("SET:HUM:"):
        value, ok = parse_float(command[8:])
        if not ok:
            return "ERR:FORMAT:HUM"
        if value < 40.0 or value > 80.0:
            return "ERR:RANGE:HUM"
        STATE["TH"] = value
        push_state_notification()
        return "OK:HUM"

    if command.startswith("SET:LUX:"):
        value, ok = parse_int(command[8:])
        if not ok:
            return "ERR:FORMAT:LUX"
        if value < 50 or value > 1200:
            return "ERR:RANGE:LUX"
        STATE["TL"] = value
        push_state_notification()
        return "OK:LUX"

    if command.startswith("SET:PEOPLE:"):
        value, ok = parse_int(command[11:])
        if not ok:
            return "ERR:FORMAT:PEOPLE"
        if value < 0 or value > 5:
            return "ERR:RANGE:PEOPLE"
        STATE["P"] = value
        push_state_notification()
        return "OK:PEOPLE"

    if command in ("SERVO:OPEN", "SERVO:CLOSE") or command.startswith("SERVO:ANGLE:") or command.startswith("ACT:"):
        if STATE["M"] != "MANUAL":
            return "ERR:MODE"

    if command == "SERVO:OPEN":
        STATE["S"] = 90
        push_state_notification()
        return "OK:SERVO:OPEN"

    if command == "SERVO:CLOSE":
        STATE["S"] = 0
        push_state_notification()
        return "OK:SERVO:CLOSE"

    if command.startswith("SERVO:ANGLE:"):
        value, ok = parse_int(command[12:])
        if not ok:
            return "ERR:FORMAT:SERVO"
        if value < 0 or value > 180:
            return "ERR:RANGE:SERVO"
        STATE["S"] = value
        push_state_notification()
        return "OK:SERVO:ANGLE"

    if command == "ACT:GREEN:ON":
        ACTUATORS["GREEN"] = 1
        return "OK:GREEN:ON"

    if command == "ACT:GREEN:OFF":
        ACTUATORS["GREEN"] = 0
        return "OK:GREEN:OFF"

    if command == "ACT:RED:ON":
        ACTUATORS["RED"] = 1
        return "OK:RED:ON"

    if command == "ACT:RED:OFF":
        ACTUATORS["RED"] = 0
        return "OK:RED:OFF"

    if command == "ACT:HEAT:ON":
        ACTUATORS["HEAT"] = 1
        return "OK:HEAT:ON"

    if command == "ACT:HEAT:OFF":
        ACTUATORS["HEAT"] = 0
        return "OK:HEAT:OFF"

    if command == "ACT:COOL:ON":
        ACTUATORS["COOL"] = 1
        return "OK:COOL:ON"

    if command == "ACT:COOL:OFF":
        ACTUATORS["COOL"] = 0
        return "OK:COOL:OFF"

    if command == "ACT:HUMIDIFIER:ON":
        ACTUATORS["HUMIDIFIER"] = 1
        return "OK:HUMIDIFIER:ON"

    if command == "ACT:HUMIDIFIER:OFF":
        ACTUATORS["HUMIDIFIER"] = 0
        return "OK:HUMIDIFIER:OFF"

    if command.startswith("ACT:PLAFONIERE:PWM:"):
        value, ok = parse_int(command[19:])
        if not ok:
            return "ERR:FORMAT:PWM"
        if value < 0 or value > 255:
            return "ERR:RANGE:PWM"
        ACTUATORS["PLAFONIERE_PWM"] = value
        return "OK:PLAFONIERE:PWM"

    return "ERR:UNKNOWN"


class Handler(BaseHTTPRequestHandler):
    def _reply(self, body):
        encoded = body.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def _reply_json(self, payload):
        encoded = json.dumps(payload).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def do_GET(self):
        parsed = urlparse(self.path)
        query = parse_qs(parsed.query)

        if parsed.path == "/health":
            self._reply("OK")
            return

        if parsed.path == "/ble/info":
            self._reply_json(BLE_INFO)
            return

        if parsed.path == "/ble/connect":
            client = query.get("client", ["APPINVENTOR"])[0]
            sid = create_session(client)
            self._reply(f"OK:CONNECTED:SID={sid}")
            return

        if parsed.path == "/ble/disconnect":
            sid = query.get("sid", [""])[0].strip().upper()
            if close_session(sid):
                self._reply("OK:DISCONNECTED")
            else:
                self._reply("ERR:SESSION")
            return

        if parsed.path == "/ble/read":
            sid = query.get("sid", [""])[0].strip().upper()
            session = get_session(sid)
            if session is None:
                self._reply("ERR:SESSION")
                return
            if session["tx_queue"]:
                self._reply(session["tx_queue"].popleft())
            else:
                self._reply("NO_DATA")
            return

        if parsed.path == "/ble/write":
            sid = query.get("sid", [""])[0].strip().upper()
            session = get_session(sid)
            if session is None:
                self._reply("ERR:SESSION")
                return

            command = query.get("c", [""])[0]
            response = process_command(command)
            session["tx_queue"].append(response)
            session["tx_queue"].append(build_state())
            self._reply("OK:WRITE")
            return

        if parsed.path == "/ble/state":
            sid = query.get("sid", [""])[0].strip().upper()
            session = get_session(sid)
            if session is None:
                self._reply("ERR:SESSION")
                return
            self._reply(build_state())
            return

        if parsed.path == "/ble/set_sensor":
            self._reply(set_simulated_sensor(query))
            return

        if parsed.path == "/ble/actuators":
            self._reply_json(ACTUATORS)
            return

        if parsed.path == "/state":
            self._reply(build_state())
            return

        if parsed.path == "/cmd":
            command = query.get("c", [""])[0]
            self._reply(process_command(command))
            return

        self._reply("ERR:BAD_REQUEST")

    def log_message(self, fmt, *args):
        return


if __name__ == "__main__":
    port = int(os.getenv("MOCK_SERVER_PORT", "8080"))
    server = HTTPServer(("0.0.0.0", port), Handler)
    print(f"Mock Arduino server in ascolto su http://0.0.0.0:{port}")
    print("Endpoint BLE virtuali: /ble/connect /ble/write /ble/read /ble/disconnect")
    print("Compat legacy: /state e /cmd?c=<comando>")
    server.serve_forever()
