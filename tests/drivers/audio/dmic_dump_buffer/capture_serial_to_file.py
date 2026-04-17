#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
"""Capture raw binary data from a USB ACM serial port to a file.

Typical use with Nordic ``dmic_dump_buffer`` test: 921600 8N1, binary PCM on the
**Zephyr console** UART for that build (see board overlay). Open the matching
``/dev/ttyACM*`` from ``dmesg`` / ``/dev/serial/by-id/``.
"""

from __future__ import annotations

import argparse
import sys


def main() -> int:
    try:
        import serial
    except ImportError:
        print("Install pyserial: pip install pyserial", file=sys.stderr)
        return 1

    p = argparse.ArgumentParser(
        description="Read binary data from a serial port and append/write to a file."
    )
    p.add_argument(
        "-p",
        "--port",
        default="/dev/ttyACM0",
        help="Serial device (default: /dev/ttyACM0)",
    )
    p.add_argument(
        "-o",
        "--output",
        default="dmic_capture.raw",
        help="Output file path (default: dmic_capture.raw)",
    )
    p.add_argument(
        "-b",
        "--baud",
        type=int,
        default=921600,
        help="Baud rate (default: 921600, matches dmic_dump_buffer README)",
    )
    p.add_argument(
        "--chunk",
        type=int,
        default=65536,
        metavar="N",
        help="Max bytes per read (default: 65536)",
    )
    p.add_argument(
        "-a",
        "--append",
        action="store_true",
        help="Append to output file instead of truncating",
    )
    args = p.parse_args()

    mode = "ab" if args.append else "wb"
    serial_kw: dict = {
        "port": args.port,
        "baudrate": args.baud,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 0.25,
        "xonxoff": False,
        "rtscts": False,
        "dsrdtr": False,
    }
    # Linux: avoid sharing /dev/ttyACM* with another process (corrupts binary PCM).
    serial_kw["exclusive"] = True

    try:
        ser = serial.Serial(**serial_kw)
    except (TypeError, ValueError):
        serial_kw.pop("exclusive", None)
        try:
            ser = serial.Serial(**serial_kw)
        except serial.SerialException as e:
            print(e, file=sys.stderr)
            return 1
    except serial.SerialException as e:
        print(e, file=sys.stderr)
        return 1

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print(
        f"Capturing from {args.port} @ {args.baud} -> {args.output} ({mode}); Ctrl+C to stop",
        flush=True,
    )
    print(
        "Tip: only one program may open the port. If audio is noisy or players reject "
        "the WAV, stop ModemManager probing (e.g. pause ModemManager) or add a udev "
        "ID_MM_DEVICE_IGNORE rule for this interface.",
        file=sys.stderr,
        flush=True,
    )
    try:
        with open(args.output, mode) as out:
            while True:
                chunk = ser.read(args.chunk)
                if chunk:
                    out.write(chunk)
                    out.flush()
    except KeyboardInterrupt:
        print("\nStopped.", flush=True)
    finally:
        ser.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
