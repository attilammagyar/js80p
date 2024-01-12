###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
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

import sys

from math import pi, sqrt, sin, cos, ceil

import matplotlib.pyplot as plt


FOLD_MIN = 0.0
FOLD_TRANSITION = 0.5
FOLD_MAX = 5.0 + FOLD_TRANSITION

PI = pi
PI_DOUBLE = PI * 2.0
PI_HALF = PI / 2.0
PI_SQR = PI * PI

TRIANGLE_SCALE = 8.0 / PI_SQR

S0 = TRIANGLE_SCALE
S1 = PI_HALF
S2 = TRIANGLE_SCALE / 9.0
S3 = PI_HALF * 3.0
S4 = TRIANGLE_SCALE / 25.0
S5 = PI_HALF * 5.0
S6 = TRIANGLE_SCALE * 2.0 / PI
S7 = TRIANGLE_SCALE / (27.0 * PI)
S8 = TRIANGLE_SCALE / (125.0 * PI)

TRIG_OFFSET = PI_DOUBLE * ceil(FOLD_MAX * S5)


def f(x):
    return (
        S0 * sin(S1 * x + TRIG_OFFSET)
        - S2 * sin(S3 * x + TRIG_OFFSET)
        + S4 * sin(S5 * x + TRIG_OFFSET)
    )


def F0(x):
    return (
        -S6 * cos(S1 * x + TRIG_OFFSET)
        + S7 * cos(S3 * x + TRIG_OFFSET)
        - S8 * cos(S5 * x + TRIG_OFFSET)
    )


def main(argv):
    N = 20000
    folding = 5.0
    prev = -folding
    F0_prev = F0(prev)
    approximation = (F0(-folding + 1.0 / N) - F0(-folding)) * N

    xs = []
    fs = []
    approximations = []

    for i in range(N):
        x = (2.0 * (i / N)) - 1.0
        x = folding * x
        delta = x - prev

        xs.append(x)
        fs.append(f(x))

        if abs(delta) > 0.0:
            F0_x = F0(x)
            approximation = (F0_x - F0_prev) / delta
            approximations.append(approximation)
            F0_prev = F0_x
        else:
            approximations.append(approximation)

        prev = x

    plt.plot(xs, fs)
    plt.plot(xs, approximations)
    plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
