#!/usr/bin/env python3
import json
import os
import re
import socket
import subprocess
import time
import unittest
from urllib.error import URLError
from urllib.parse import quote
from urllib.request import urlopen


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


class MockServerIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.port = free_port()
        cls.base = f"http://127.0.0.1:{cls.port}"

        env = os.environ.copy()
        env["MOCK_SERVER_PORT"] = str(cls.port)
        cls.proc = subprocess.Popen(
            ["python3", "tools/mock_arduino_server.py"],
            cwd=os.path.join(os.path.dirname(__file__), ".."),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        deadline = time.time() + 5
        while time.time() < deadline:
            try:
                if cls.http_get("/health") == "OK":
                    return
            except Exception:
                time.sleep(0.1)

        cls.tearDownClass()
        raise RuntimeError("Mock server did not start in time")

    @classmethod
    def tearDownClass(cls):
        proc = getattr(cls, "proc", None)
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except Exception:
                proc.kill()

    @classmethod
    def http_get(cls, path):
        with urlopen(cls.base + path, timeout=2) as response:
            return response.read().decode("utf-8")

    @classmethod
    def connect(cls, client="TEST"):
        body = cls.http_get(f"/ble/connect?client={quote(client)}")
        match = re.search(r"SID=([A-Z0-9]+)", body)
        if not match:
            raise AssertionError(f"Missing SID in response: {body}")
        return match.group(1)

    @classmethod
    def write(cls, sid, command):
        return cls.http_get(f"/ble/write?sid={sid}&c={quote(command)}")

    @classmethod
    def read(cls, sid):
        return cls.http_get(f"/ble/read?sid={sid}")

    @classmethod
    def drain(cls, sid):
        messages = []
        for _ in range(32):
            value = cls.read(sid)
            if value == "NO_DATA":
                break
            messages.append(value)
        return messages

    @classmethod
    def read_until(cls, sid, expected, attempts=16):
        for _ in range(attempts):
            value = cls.read(sid)
            if value == expected:
                return value
        raise AssertionError(f"Expected '{expected}' not found in queue for SID={sid}")

    def test_ble_info_exposed(self):
        payload = self.http_get("/ble/info")
        info = json.loads(payload)
        self.assertEqual(info["protocol"], "text-line")
        self.assertIn("service_uuid", info)
        self.assertIn("rx_char_uuid", info)
        self.assertIn("tx_char_uuid", info)

    def test_connect_read_disconnect(self):
        sid = self.connect("AI2")
        first = self.read(sid)
        second = self.read(sid)
        self.assertEqual(first, "BLE:CONNECTED")
        self.assertTrue(second.startswith("STATE:"))
        self.assertEqual(self.http_get(f"/ble/disconnect?sid={sid}"), "OK:DISCONNECTED")
        self.assertEqual(self.http_get(f"/ble/read?sid={sid}"), "ERR:SESSION")

    def test_write_ping_roundtrip(self):
        sid = self.connect("PING")
        self.drain(sid)
        self.assertEqual(self.write(sid, "PING"), "OK:WRITE")
        self.read_until(sid, "PONG")
        self.assertTrue(any(msg.startswith("STATE:") for msg in self.drain(sid)))
        self.http_get(f"/ble/disconnect?sid={sid}")

    def test_mode_enforcement_on_actuators(self):
        sid = self.connect("MODE")
        self.drain(sid)

        self.write(sid, "MANUAL:OFF")
        self.drain(sid)
        self.write(sid, "SERVO:OPEN")
        self.read_until(sid, "ERR:MODE")

        self.write(sid, "MANUAL:ON")
        self.drain(sid)
        self.write(sid, "SERVO:OPEN")
        self.read_until(sid, "OK:SERVO:OPEN")

        self.http_get(f"/ble/disconnect?sid={sid}")

    def test_set_commands_and_ranges(self):
        sid = self.connect("SET")
        self.drain(sid)

        self.write(sid, "SET:TEMP:22.5")
        self.read_until(sid, "OK:TEMP")

        self.write(sid, "SET:TEMP:35")
        self.read_until(sid, "ERR:RANGE:TEMP")

        self.write(sid, "SET:HUM:ABC")
        self.read_until(sid, "ERR:FORMAT:HUM")

        self.write(sid, "SET:LUX:350")
        self.read_until(sid, "OK:LUX")

        self.write(sid, "SET:PEOPLE:4")
        self.read_until(sid, "OK:PEOPLE")

        state = self.http_get(f"/ble/state?sid={sid}")
        self.assertIn("TT=22.5", state)
        self.assertIn("TL=350", state)
        self.assertIn("P=4", state)

        self.http_get(f"/ble/disconnect?sid={sid}")

    def test_simulated_sensor_updates_and_state(self):
        sid = self.connect("SENSORS")
        self.drain(sid)

        self.assertEqual(self.http_get("/ble/set_sensor?t=24.3&h=63.1&l=245&p=2"), "OK:SENSORS")
        state = self.http_get(f"/ble/state?sid={sid}")
        self.assertIn("T=24.3", state)
        self.assertIn("H=63.1", state)
        self.assertIn("L=245", state)
        self.assertIn("P=2", state)

        self.http_get(f"/ble/disconnect?sid={sid}")

    def test_actuators_endpoint_reflects_changes(self):
        sid = self.connect("ACT")
        self.drain(sid)

        self.write(sid, "MANUAL:ON")
        self.drain(sid)

        self.write(sid, "ACT:GREEN:ON")
        self.read_until(sid, "OK:GREEN:ON")
        self.write(sid, "ACT:PLAFONIERE:PWM:128")
        self.read_until(sid, "OK:PLAFONIERE:PWM")

        payload = self.http_get("/ble/actuators")
        actuators = json.loads(payload)
        self.assertEqual(actuators["GREEN"], 1)
        self.assertEqual(actuators["PLAFONIERE_PWM"], 128)

        self.http_get(f"/ble/disconnect?sid={sid}")

    def test_legacy_endpoints_still_available(self):
        self.assertEqual(self.http_get("/cmd?c=PING"), "PONG")
        self.assertTrue(self.http_get("/state").startswith("STATE:"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
