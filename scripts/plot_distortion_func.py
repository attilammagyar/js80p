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

import os.path
import sys

from math import exp, log, tanh

import matplotlib.pyplot as plt


def f(x, steepness):
    return tanh(steepness * x * 0.5)


def F0(x, steepness):
    return x + (2.0 / steepness) * log(exp(-steepness * x) + 1.0)


def main(argv):
    if len(argv) < 2:
        print(f"Usage: {os.path.basename(argv[0])} steepness", file=sys.stderr)

        return 1

    steepness = float(argv[1])

    if steepness <= 0.0:
        print(f"Steepness must be greater than 0, got: {steepness}", file=sys.stderr)

        return 2

    N = 20000
    max_input = 3.0
    prev = -max_input
    F0_prev = F0(prev, steepness)

    xs = []
    fs = []
    approximations = []

    for i in range(N):
        x = (2.0 * (i / N)) - 1.0
        x = max_input * x
        delta = x - prev
        F0_x = F0(x, steepness)

        xs.append(x)
        fs.append(f(x, steepness))

        if abs(delta) > 0.0:
            approximations.append((F0_x - F0_prev) / delta)
        else:
            approximations.append(f(x, steepness))

        prev = x
        F0_prev = F0_x

    plt.plot(xs, fs)
    plt.plot(xs, approximations)
    plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
