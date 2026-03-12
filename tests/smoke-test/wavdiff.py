###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2026  Attila M. Magyar
#
# JS80P is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# JS80P is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################

import struct
import sys
import wave


def main(argv):
    if len(argv) < 3:
        print(f"Usage: python3 {argv[0]} a.wav b.wav", file=sys.stderr)

        return 1

    with wave.open(argv[1], "rb") as wav_a, wave.open(argv[2], "rb") as wav_b:
        assert_eq(wav_a.getnchannels(), wav_b.getnchannels(), "channels")
        assert_eq(wav_a.getframerate(), wav_b.getframerate(), "sample rate")
        assert_eq(wav_a.getnframes(), wav_b.getnframes(), "number of frames")

        remaining_frames = wav_a.getnframes()
        max_diff = 0

        while remaining_frames > 0:
            chunk_len = min(8192, remaining_frames)
            remaining_frames -= chunk_len

            chunk_a = read_chunk(wav_a, chunk_len, argv[1])
            chunk_b = read_chunk(wav_b, chunk_len, argv[2])

            assert_eq(len(chunk_a), len(chunk_b), "chunk length")

            for i in range(len(chunk_a)):
                diff = abs(chunk_a[i] - chunk_b[i])

                if diff > max_diff:
                    max_diff = diff

        print(f"{max_diff:.6f}")

    return 0


def assert_eq(a, b, msg):
    assert a == b, f"Failed to assert that {a} = {b} ({msg})"


def read_chunk(wav_file, n_frames, file_name):
    if n_frames < 1:
        return []

    sample_width = wav_file.getsampwidth()
    n_channels = wav_file.getnchannels()
    raw_bytes = wav_file.readframes(n_frames)

    if sample_width == 2:
        count = len(raw_bytes) // 2

        return [s / 32768.0 for s in struct.unpack(f"<{count}h", raw_bytes)]

    if sample_width == 3:
        samples = []

        for i in range(0, len(raw_bytes), 3):
            triplet = raw_bytes[i:i + 3]
            value = triplet[0] | (triplet[1] << 8) | (triplet[2] << 16)
            value -= ((value & 0x800000) >> 23) * 0x1000000
            samples.append(value / 8388608.0)

        return samples

    raise ValueError(f"Unexpected sample width in {file_name!r}: {sample_width!r}")


if __name__ == "__main__":
    sys.exit(main(sys.argv))
