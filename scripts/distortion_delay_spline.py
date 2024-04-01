###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2024  Attila M. Magyar
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

import numpy as np
import scipy.linalg as la

from math import sqrt

import matplotlib.pyplot as plt


# See Distortion::Tables::initialize_delay_feedback_tables() for a detailed
# explanation

# The spline will be linear on the [0, gamma] interval.
gamma = 1.0 / 16.0

# Parameter: f'(gamma) := beta
beta = 11.0 / 16.0

# Parameter: f(1) = alpha
alpha = 445.0 / 512.0


alpha_m_1_o_4 = (alpha - 1.0) / 4.0


# These will be set by solve_spline()
A = B = C = D = cf = ch = 0.0

cg = -9.0 * alpha_m_1_o_4


h = lambda x: beta * x
H = lambda x: (1.0 / 2.0) * beta * x ** 2.0 + ch


f = lambda x: ((A * x + B) * x + C) * x + D
F = lambda x: ((((A / 4.0) * x + (B / 3.0)) * x + (C / 2.0)) * x + D) * x + cf


g = lambda x: (
    (alpha_m_1_o_4 * x - 6.0 * alpha_m_1_o_4) * x + 9.0 * alpha_m_1_o_4 + 1.0
)
G = lambda x: (
    (
        ((alpha_m_1_o_4 / 3.0) * x - 3.0 * alpha_m_1_o_4) * x
        + 9.0 * alpha_m_1_o_4 + 1.0
    ) * x + cg
)


INPUT_MAX = 3.0


def solve_spline():
    global A, B, C, D, cf, ch

    A1 = 3.0 * gamma ** 4.0
    B1 = 4.0 * gamma ** 3.0
    C1 = 6.0 * gamma ** 2.0
    D1 = 12.0 * gamma

    A5 = 3.0 * gamma ** 2.0
    B5 = 2.0 * gamma

    A6 = gamma ** 3.0 - 1.0
    B6 = gamma ** 2.0 - 1.0
    C6 = gamma - 1.0

    eq_mtx = np.array(
        [
            [-12.0,  12.0,    D1,    A1,    B1,    C1],
            [  0.0,  12.0,  12.0,   3.0,   4.0,   6.0],
            [  0.0,   0.0,   1.0,   1.0,   1.0,   1.0],
            [  0.0,   0.0,   0.0,   3.0,   2.0,   1.0],
            [  0.0,   0.0,   0.0,    A5,    B5,   1.0],
            [  0.0,   0.0,   0.0,    A6,    B6,    C6],
        ]
    )
    eq_v = np.array(
        [
            6.0 * beta * gamma ** 2.0,
            20.0 - 8.0 * alpha,
            alpha,
            1.0 - alpha,
            beta,
            beta * gamma - alpha,
        ]
    )
    ch, cf, D, A, B, C = la.inv(eq_mtx).dot(eq_v)


def spline(x):
    sgn = 1.0

    if x < 0.0:
        sgn = -1.0
        x = -x

    if x >= INPUT_MAX:
        return sgn

    if x >= 1.0:
        return sgn * g(x)

    if x >= gamma:
        return sgn * f(x)

    return sgn * h(x)


def spline_int(x):
    if x < 0.0:
        x = -x

    if x >= INPUT_MAX:
        return x

    if x >= 1.0:
        return G(x)

    if x >= gamma:
        return F(x)

    return H(x)


def main(argv):
    N = 20000
    width = 5.0

    solve_spline()

    funcs = (
        (lambda x: 0, (([], 1),)),
        (lambda x: x, (([], 1),)),
        (lambda x: spline_int(x), (([], 1),)),
        (
            lambda x: spline(x),
            (([], 1), ([], 5), ([], 25), ([], 35), ([], 10), ([], 5), ([], 3)),
        ),
    )

    xs = []

    min_d = 999.0

    for i in range(N):
        x = (2.0 * (i / N)) - 1.0
        x = width * x

        if x > 0.0:
            s = spline(x)
            d = x - s

            if d < min_d:
                min_d = d

        xs.append(x)

        for f, iters in funcs:
            fx = x

            for values, n in iters:
                for i in range(n):
                    fx = f(fx)

                values.append(fx)

    values = [v for f, iters in funcs for v, n in iters]

    print(f"""
min_d = {min_d}

alpha = {alpha}
beta = {beta}
gamma = {gamma}

cg = {cg}

constexpr Number A = {A};
constexpr Number B = {B};
constexpr Number C = {C};
constexpr Number D = {D};
constexpr Number cf = {cf};
constexpr Number ch = {ch};

""")

    for v in values:
        plt.plot(xs, v)

    plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
