#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
"""Convert raw little-endian S16 PCM to WAV using only the Python standard library.

Ubuntu ships Python 3; no pip packages are required. Typical use::

    python3 raw_to_wav.py dmic_capture.raw -o out.wav

Some desktop players / PipeWire stacks handle 16 kHz poorly (frozen timer, silence).
Use ``--rate-out 48000`` to upsample (nearest 3× for 16 k→48 k) for more reliable playback.
"""

from __future__ import annotations

import argparse
import array
import math
import struct
import sys


def _looks_like_non_pcm(data: bytes) -> bool:
    """Detect common mistakes (e.g. converting a .py script instead of a .raw file)."""
    if not data:
        return False
    if data.startswith(b"#!"):
        return True
    if data.startswith((b'"""', b"'''")):
        return True
    if data.startswith(b"# Copyright") or data.startswith(b"/*"):
        return True
    head = data[:4096]
    textish = sum(1 for b in head if b in (9, 10, 13) or 32 <= b < 127)
    return len(head) >= 80 and textish > len(head) * 0.92


def _pcm_stats(pcm: bytes, channels: int, rate: int) -> tuple[int, float, float, int]:
    samples = array.array("h")
    samples.frombytes(pcm[: len(pcm) // (2 * channels) * (2 * channels)])
    nframes = len(samples) // channels
    if not nframes:
        return 0, 0.0, 0.0, 0
    dur = nframes / float(rate)
    flat = list(samples)
    mean = sum(flat) / len(flat)
    var = sum((x - mean) ** 2 for x in flat) / len(flat)
    rms = math.sqrt(var)
    peak = max(abs(x) for x in flat)
    return nframes, dur, rms, peak


def _resample_s16_triple(pcm: bytes, channels: int, src_hz: int, dst_hz: int) -> tuple[bytes, int]:
    """3× sample rate (e.g. 16 kHz → 48 kHz) by repeating each frame."""
    if src_hz == dst_hz:
        return pcm, src_hz
    if dst_hz != src_hz * 3:
        print(
            f"Unsupported resample {src_hz} → {dst_hz} (only exact ×3, e.g. 16000→48000).",
            file=sys.stderr,
        )
        sys.exit(1)
    fs = 2 * channels
    nframes = len(pcm) // fs
    a = array.array("h")
    a.frombytes(pcm[: nframes * fs])
    out = array.array("h")
    for i in range(0, len(a), channels):
        frame = a[i : i + channels]
        for _ in range(3):
            out.extend(frame)
    return out.tobytes(), dst_hz


def _write_wav_pcm16le(path: str, pcm: bytes, nchannels: int, framerate: int) -> None:
    """Write canonical PCM WAVE (RIFF) without relying on :mod:`wave` (portable headers)."""
    sampwidth = 2
    if len(pcm) % (nchannels * sampwidth):
        raise ValueError("PCM length must be a whole number of frames")
    data_size = len(pcm)
    if data_size % 2:
        pcm = pcm + b"\x00"
        data_size = len(pcm)
    byte_rate = framerate * nchannels * sampwidth
    block_align = nchannels * sampwidth
    fmt = struct.pack(
        "<HHIIHH",
        1,
        nchannels,
        framerate,
        byte_rate,
        block_align,
        16,
    )
    riff_payload_size = 4 + (8 + 16) + (8 + data_size)
    with open(path, "wb") as outf:
        outf.write(b"RIFF")
        outf.write(struct.pack("<I", riff_payload_size))
        outf.write(b"WAVE")
        outf.write(b"fmt ")
        outf.write(struct.pack("<I", 16))
        outf.write(fmt)
        outf.write(b"data")
        outf.write(struct.pack("<I", data_size))
        outf.write(pcm)


def main() -> int:
    p = argparse.ArgumentParser(
        description="Wrap raw S16LE PCM in a RIFF WAV container (stdlib only)."
    )
    p.add_argument("input", help="Input raw file (signed 16-bit little-endian)")
    p.add_argument(
        "-o",
        "--output",
        help="Output .wav (default: replace .raw with .wav or append .wav)",
    )
    p.add_argument(
        "-r",
        "--rate",
        type=int,
        default=16000,
        help="Sample rate of the raw input in Hz (default: 16000, matches dmic_dump_buffer)",
    )
    p.add_argument(
        "-c",
        "--channels",
        type=int,
        default=1,
        choices=(1, 2),
        help="1=mono, 2=stereo interleaved (default: 1)",
    )
    p.add_argument(
        "--rate-out",
        type=int,
        default=None,
        metavar="HZ",
        help="Output WAV sample rate (e.g. 48000). For 16000 input, uses ×3 upsample to 48000.",
    )
    p.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Print duration/RMS/peak (helps verify the capture is real PCM).",
    )
    args = p.parse_args()

    if args.output:
        outp = args.output
    elif args.input.lower().endswith(".raw"):
        outp = args.input[:-4] + ".wav"
    else:
        outp = args.input + ".wav"

    try:
        with open(args.input, "rb") as inf:
            pcm = inf.read()
    except OSError as e:
        print(e, file=sys.stderr)
        return 1

    if _looks_like_non_pcm(pcm):
        print(
            "This file looks like source code or text, not raw S16LE PCM. "
            "Pass the file produced by capture_serial_to_file.py (e.g. dmic_capture.raw).",
            file=sys.stderr,
        )
        return 1

    min_mono_100ms = 2 * args.rate // 10
    if len(pcm) < min_mono_100ms:
        print(
            f"Warning: input is only {len(pcm)} bytes (~{len(pcm) / (2 * args.rate):.3f} s at "
            f"{args.rate} Hz mono). Check serial port, baud, and that firmware console UART "
            f"matches the port you capture.",
            file=sys.stderr,
        )

    frame_bytes = 2 * args.channels
    remainder = len(pcm) % frame_bytes
    if remainder:
        pcm = pcm[:-remainder]
        print(
            f"Trimmed {remainder} trailing byte(s) (incomplete frame at end of file).",
            file=sys.stderr,
        )

    if not pcm:
        print("No PCM frames after trim; nothing to write.", file=sys.stderr)
        return 1

    out_rate = args.rate_out if args.rate_out is not None else args.rate
    pcm_out, out_rate = _resample_s16_triple(pcm, args.channels, args.rate, out_rate)

    if args.verbose:
        nframes, dur, rms, peak = _pcm_stats(pcm_out, args.channels, out_rate)
        print(
            f"Output PCM: {nframes} frames  ~{dur:.3f} s  "
            f"RMS={rms:.1f}  peak={peak}  @ {out_rate} Hz",
            flush=True,
        )

    try:
        _write_wav_pcm16le(outp, pcm_out, args.channels, out_rate)
    except OSError as e:
        print(e, file=sys.stderr)
        return 1

    print(f"Wrote {outp}  ({out_rate} Hz, {args.channels} ch, 16-bit LE)", flush=True)
    if out_rate < 32000:
        print(
            "If playback shows 0:00 or silence, try:  "
            f"python3 raw_to_wav.py -r {args.rate} --rate-out 48000 -o out48.wav {args.input}",
            flush=True,
        )
    print(
        'Try:  aplay -t wav "'
        + outp.replace('"', '\\"')
        + '"   or   pw-play "'
        + outp.replace('"', '\\"')
        + '"',
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
