"""
Reads buffered speed-fluctuation windows from the Arduino over USB
serial and forwards each one to the Flask API's /ingest endpoint. Each
serial line is expected in the format produced by
firmware/vibration_monitor.ino:

    <rpm>,<pulse_rate_hz>,<dev_1>,<dev_2>,...,<dev_N>

pulse_rate_hz is the effective sample rate of the window, since the
signal is sampled once per speed-sensor pulse. Lines that are
malformed or contain fewer than WINDOW_PULSES samples are skipped.

Usage: python serial_reader.py --port COM3 [--host http://127.0.0.1:5000]
"""
import argparse
import time

import requests
import serial

WINDOW_PULSES = 64  # samples per window, must match the firmware
BOARD_RESET_DELAY_S = 2  # the Uno resets when the serial port is opened


def main(port, baud, host):
    ser = serial.Serial(port, baud, timeout=30)
    time.sleep(BOARD_RESET_DELAY_S)
    ser.reset_input_buffer()
    print(f"reading from {port} at {baud} baud, forwarding to {host}/ingest")

    while True:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue

        try:
            values = [float(v) for v in line.split(",")]
        except ValueError:
            print(f"skipping non-data line: {line[:60]}")
            continue

        rpm, sample_rate, window = values[0], values[1], values[2:]
        if len(window) != WINDOW_PULSES:
            print(f"skipping incomplete window ({len(window)} samples)")
            continue

        payload = {
            "window": window,
            "sample_rate": sample_rate,
            "rpm": rpm,
            "signal": "speed",
        }
        try:
            res = requests.post(f"{host}/ingest", json=payload, timeout=5)
            result = res.json()
            print(f"rpm={rpm:.1f} prediction={result.get('prediction')} score={result.get('probability')}")
        except requests.RequestException as e:
            print(f"failed to reach {host}: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--host", default="http://127.0.0.1:5000")
    args = parser.parse_args()
    main(args.port, args.baud, args.host)