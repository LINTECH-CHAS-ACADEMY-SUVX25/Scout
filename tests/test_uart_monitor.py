#!/usr/bin/env python3
"""
test_uart_monitor.py — Automatic tests for the Scout screen UART monitor.

Connects to the scout_screen node over USB serial and verifies that each
monitor command (STATUS, STREAM, DIAG, HELP) produces the expected output.
Runs without manual interaction and exits 0 on success, 1 on failure.

Usage:
    python tests/test_uart_monitor.py --port /dev/ttyUSB0
    python tests/test_uart_monitor.py --port COM3 --baud 115200

Requirements:
    pip install pyserial
"""

import argparse
import sys
import time
import serial

GREEN = "\033[32m"
RED   = "\033[31m"
RESET = "\033[0m"

_results = []


# ---------------------------------------------------------------------------
# Serial helpers
# ---------------------------------------------------------------------------

def _send(ser, text):
    ser.write((text + "\r\n").encode())
    ser.flush()


def _flush(ser):
    ser.reset_input_buffer()
    time.sleep(0.05)


def _read_lines(ser, idle_s=0.3, max_s=3.0):
    """Collect lines until idle_s of silence or max_s total."""
    lines = []
    ser.timeout = idle_s
    deadline = time.monotonic() + max_s
    while time.monotonic() < deadline:
        raw = ser.readline()
        if not raw:
            break
        text = raw.decode(errors="replace").strip()
        if text:
            lines.append(text)
    return lines


def _read_nonblocking(ser):
    """Drain whatever is already in the receive buffer right now."""
    ser.timeout = 0.05
    chunks = []
    while True:
        chunk = ser.read(256)
        if not chunk:
            break
        chunks.append(chunk)
    raw = b"".join(chunks)
    return [l.decode(errors="replace").strip()
            for l in raw.splitlines() if l.strip()]


# ---------------------------------------------------------------------------
# Assertion helpers
# ---------------------------------------------------------------------------

def _check(test_name, lines, *required_strings):
    combined = "\n".join(lines)
    for s in required_strings:
        if s not in combined:
            print(f"  {RED}FAIL{RESET}  {test_name}: expected '{s}' in response")
            print(f"        Got: {lines!r}")
            _results.append(False)
            return
    print(f"  {GREEN}PASS{RESET}  {test_name}")
    _results.append(True)


# ---------------------------------------------------------------------------
# Individual tests
# ---------------------------------------------------------------------------

def test_help(ser):
    _flush(ser)
    _send(ser, "HELP")
    lines = _read_lines(ser)
    _check("HELP lists STATUS",  lines, "STATUS")
    _check("HELP lists STREAM",  lines, "STREAM")
    _check("HELP lists DIAG",    lines, "DIAG")
    _check("HELP lists HELP",    lines, "HELP")


def test_status(ser):
    _flush(ser)
    _send(ser, "STATUS")
    lines = _read_lines(ser)
    _check("STATUS header",       lines, "--- STATUS ---")
    _check("STATUS uptime field", lines, "uptime")
    _check("STATUS heap field",   lines, "free heap")
    _check("STATUS wifi field",   lines, "WiFi clients")
    _check("STATUS stream field", lines, "cam stream")


def test_diag(ser):
    _flush(ser)
    _send(ser, "DIAG")
    lines = _read_lines(ser)
    _check("DIAG header",      lines, "--- DIAG ---")
    _check("DIAG tasks field", lines, "tasks")
    _check("DIAG min heap",    lines, "min heap")
    _check("DIAG free int",    lines, "free int")
    _check("DIAG free PSRAM",  lines, "free PSRAM")


def test_stream(ser):
    _flush(ser)
    _send(ser, "STREAM")
    # Allow header + first stats frame to arrive (stream refreshes every 200 ms)
    time.sleep(0.6)
    lines = _read_nonblocking(ser)
    # Exit live mode before asserting so the monitor is ready for the next test
    ser.write(b"q")
    ser.flush()
    time.sleep(0.3)
    ser.reset_input_buffer()
    _check("STREAM header",          lines, "=== STREAM ===")
    _check("STREAM Receive section", lines, "Receive")
    _check("STREAM Render section",  lines, "Render")
    _check("STREAM frames field",    lines, "frames")
    _check("STREAM fps field",       lines, "fps")


def test_unknown_command(ser):
    _flush(ser)
    _send(ser, "FOOBAR")
    lines = _read_lines(ser)
    _check("unknown command error", lines, "unknown command")


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

TESTS = [
    ("HELP",            test_help),
    ("STATUS",          test_status),
    ("DIAG",            test_diag),
    ("STREAM",          test_stream),
    ("Unknown command", test_unknown_command),
]


def main():
    parser = argparse.ArgumentParser(
        description="Scout UART monitor test runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Example: python tests/test_uart_monitor.py --port /dev/ttyUSB0",
    )
    parser.add_argument("--port",  required=True,
                        help="Serial port (e.g. /dev/ttyUSB0 or COM3)")
    parser.add_argument("--baud",  type=int, default=115200,
                        help="Baud rate (default 115200)")
    args = parser.parse_args()

    print(f"Connecting to {args.port} at {args.baud} baud ...")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    # Let any startup noise settle
    time.sleep(0.3)
    ser.reset_input_buffer()

    print(f"\nRunning {len(TESTS)} test groups:\n")
    for group_name, fn in TESTS:
        print(f"[{group_name}]")
        fn(ser)

    ser.close()

    passed = sum(_results)
    total  = len(_results)
    color  = GREEN if passed == total else RED
    print(f"\n{color}{passed}/{total} checks passed{RESET}")
    sys.exit(0 if passed == total else 1)


if __name__ == "__main__":
    main()
