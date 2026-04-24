#!/usr/bin/env python3
"""
Simple BLE client to test Pinacoteca remote protocol from a Linux PC.

It scans for the Arduino BLE peripheral, connects, writes commands to RX,
and listens for responses/state updates from TX.
"""

import argparse
import asyncio
import sys
from typing import List, Optional

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    BleakClient = None
    BleakScanner = None

DEFAULT_DEVICE_NAME = "PINA-R4"
DEFAULT_SERVICE_UUID = "19b10000-e8f2-537e-4f6c-d104768a1214"
DEFAULT_RX_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214"
DEFAULT_TX_UUID = "19b10002-e8f2-537e-4f6c-d104768a1214"
DEFAULT_COMMANDS = ["PING", "VER", "GET:MODE", "GET:STATE"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="BLE tester for Pinacoteca (UNO R4 WiFi)."
    )
    parser.add_argument(
        "--name",
        default=DEFAULT_DEVICE_NAME,
        help="Advertised BLE name to scan (default: %(default)s)",
    )
    parser.add_argument(
        "--address",
        default="",
        help="Direct BLE address/MAC to connect (skips scan)",
    )
    parser.add_argument(
        "--service-uuid",
        default=DEFAULT_SERVICE_UUID,
        help="Service UUID (default: %(default)s)",
    )
    parser.add_argument(
        "--rx-uuid",
        default=DEFAULT_RX_UUID,
        help="RX characteristic UUID (write) (default: %(default)s)",
    )
    parser.add_argument(
        "--tx-uuid",
        default=DEFAULT_TX_UUID,
        help="TX characteristic UUID (notify/read) (default: %(default)s)",
    )
    parser.add_argument(
        "--scan-timeout",
        type=float,
        default=8.0,
        help="Seconds for BLE discovery (default: %(default)s)",
    )
    parser.add_argument(
        "--response-timeout",
        type=float,
        default=2.0,
        help="Seconds waiting an immediate response per command (default: %(default)s)",
    )
    parser.add_argument(
        "--pause",
        type=float,
        default=0.35,
        help="Pause between commands in seconds (default: %(default)s)",
    )
    parser.add_argument(
        "--listen",
        type=float,
        default=2.0,
        help="Extra seconds to keep listening after last command (default: %(default)s)",
    )
    parser.add_argument(
        "--connect-retries",
        type=int,
        default=2,
        help="Extra reconnect attempts after the first failure (default: %(default)s)",
    )
    parser.add_argument(
        "--retry-delay",
        type=float,
        default=1.0,
        help="Seconds to wait before a reconnect attempt (default: %(default)s)",
    )
    parser.add_argument(
        "--cmd",
        action="append",
        dest="commands",
        help="Command to send (repeatable). If omitted, sends a default test set.",
    )
    return parser.parse_args()


def decode_payload(data: bytes) -> str:
    return data.decode("utf-8", errors="replace").strip("\0\r\n ")


async def find_device_by_name(name: str, timeout_s: float):
    print(f"[scan] Looking for BLE device name '{name}' for up to {timeout_s:.1f}s...")
    devices = await BleakScanner.discover(timeout=timeout_s)

    exact = [d for d in devices if (d.name or "") == name]
    if exact:
        dev = exact[0]
        print(f"[scan] Found exact match: {dev.name} ({dev.address})")
        return dev

    partial = [d for d in devices if name.lower() in (d.name or "").lower()]
    if partial:
        dev = partial[0]
        print(f"[scan] Found partial match: {dev.name} ({dev.address})")
        return dev

    print("[scan] Device not found. Nearby BLE devices seen:")
    for d in devices[:20]:
        dname = d.name if d.name else "<no-name>"
        print(f"  - {dname} ({d.address})")
    return None


async def run_session(args: argparse.Namespace) -> int:
    if BleakClient is None or BleakScanner is None:
        print("[error] Python package 'bleak' is missing.")
        print("Install with one of these commands:")
        print("  sudo apt install python3-bleak")
        print("  # or")
        print("  sudo apt install python3-pip && python3 -m pip install --user bleak")
        return 2

    commands: List[str] = args.commands if args.commands else list(DEFAULT_COMMANDS)

    if args.address:
        target = args.address
        target_label = args.address
        print(f"[connect] Using direct BLE address: {target_label}")
    else:
        dev = await find_device_by_name(args.name, args.scan_timeout)
        if dev is None:
            print("[hint] Ensure Arduino is powered and advertising (serial log: BT:BLE:ADV:PINA-R4).")
            return 3
        # Passing the BLEDevice object is generally more reliable than raw address
        # on Linux/BlueZ for immediate connect after scan.
        target = dev
        target_label = dev.address

    target_candidates = [target]
    if target != target_label:
        # Some BlueZ backends may invalidate scanned BLEDevice handles quickly;
        # fallback to direct address when needed.
        target_candidates.append(target_label)

    response_queue: asyncio.Queue[str] = asyncio.Queue()

    def on_notify(_handle: int, payload: bytearray):
        message = decode_payload(bytes(payload))
        if message:
            print(f"<< {message}")
            response_queue.put_nowait(message)

    max_attempts = max(1, args.connect_retries + 1)

    for attempt in range(1, max_attempts + 1):
        while not response_queue.empty():
            response_queue.get_nowait()

        if max_attempts > 1:
            print(f"[connect] Attempt {attempt}/{max_attempts} to {target_label}...")
        else:
            print(f"[connect] Connecting to {target_label}...")

        last_exc: Optional[Exception] = None

        for target_candidate in target_candidates:
            if target_candidate == target_label and target_candidate != target:
                print(f"[connect] Retrying with direct address {target_label}...")

            client_kwargs = {"timeout": 10.0}
            if args.service_uuid:
                client_kwargs["services"] = [args.service_uuid]

            try:
                client = BleakClient(target_candidate, **client_kwargs)
            except TypeError:
                client_kwargs.pop("services", None)
                client = BleakClient(target_candidate, **client_kwargs)

            try:
                async with client:
                    if not client.is_connected:
                        raise RuntimeError("BLE connection failed")

                    print("[connect] Connected.")
                    notifications_enabled = False

                    try:
                        await client.start_notify(args.tx_uuid, on_notify)
                        notifications_enabled = True
                        print(f"[notify] Listening on TX {args.tx_uuid}")
                    except Exception as exc:
                        print(f"[warn] Could not enable notifications: {exc}")
                        print("[warn] Falling back to read-after-write mode.")

                    # Read initial TX value if available.
                    try:
                        initial = await client.read_gatt_char(args.tx_uuid)
                        initial_text = decode_payload(bytes(initial))
                        if initial_text:
                            print(f"<< {initial_text}")
                    except Exception:
                        pass

                    for command in commands:
                        while not response_queue.empty():
                            response_queue.get_nowait()

                        print(f">> {command}")
                        await client.write_gatt_char(args.rx_uuid, command.encode("utf-8"), response=False)

                        if notifications_enabled:
                            try:
                                await asyncio.wait_for(response_queue.get(), timeout=args.response_timeout)
                            except asyncio.TimeoutError:
                                print("[warn] No immediate response received.")
                        else:
                            await asyncio.sleep(min(args.response_timeout, 0.8))
                            try:
                                data = await client.read_gatt_char(args.tx_uuid)
                                text = decode_payload(bytes(data))
                                if text:
                                    print(f"<< {text}")
                                else:
                                    print("[warn] Empty TX read.")
                            except Exception as exc:
                                print(f"[warn] TX read failed: {exc}")

                        if args.pause > 0:
                            await asyncio.sleep(args.pause)

                    if args.listen > 0:
                        print(f"[listen] Waiting {args.listen:.1f}s for async notifications...")
                        await asyncio.sleep(args.listen)

                    if notifications_enabled:
                        try:
                            await client.stop_notify(args.tx_uuid)
                        except Exception:
                            pass

                    print("[done] Session completed.")
                    return 0
            except Exception as exc:
                last_exc = exc

                # When scan result object is stale on BlueZ, address fallback may work.
                stale_object = (
                    target_candidate != target_label
                    and (
                        "not found" in str(exc).lower()
                        or "removed from bluez" in str(exc).lower()
                    )
                )
                if stale_object:
                    continue

                break

        exc = last_exc if last_exc is not None else RuntimeError("BLE connection failed")
        try:
            retryable = attempt < max_attempts
            if retryable:
                print(f"[warn] BLE attempt failed: {exc}")
                if args.retry_delay > 0:
                    print(f"[retry] Waiting {args.retry_delay:.1f}s before reconnect...")
                    await asyncio.sleep(args.retry_delay)
                continue

            print(f"[error] BLE session failed: {exc}")
            return 5
        except Exception as exc2:
            print(f"[error] BLE session failed: {exc2}")
            return 5

    return 5


def main() -> int:
    args = parse_args()
    return asyncio.run(run_session(args))


if __name__ == "__main__":
    sys.exit(main())
