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

from math import exp, log1p, tanh, sin, cos, pi, sqrt

import numpy as np
import scipy.linalg as la

import matplotlib.pyplot as plt


INPUT_MAX = 3.0
INPUT_MIN = - INPUT_MAX

PI2 = pi * 2.0


def tanh_f(x, steepness):
    return tanh(steepness * x * 0.5)


def tanh_F0(x, steepness):
    return x + (2.0 / steepness) * log1p(exp(-steepness * x))


def join(alpha, gamma, h, dh, H):
    def combine_branches(funcs):
        return lambda x: (
            funcs[0](x) if x < gamma
                else (funcs[1](x) if x < 1.0 else funcs[2](x))
        )

    E = (alpha - 1.0) / 4.0
    cg = -9.0 * E

    g = lambda x: E * (x - 3.0) ** 2.0 + 1.0
    dg = lambda x: 2.0 * E * x - 6.0 * E
    G = lambda x: (((E / 3.0) * x - 3.0 * E) * x + 9.0 * E + 1.0) * x + cg

    L1 = gamma
    L2 = gamma ** 2.0
    L3 = gamma ** 3.0
    L4 = gamma ** 4.0

    eq_mtx = np.array(
        [
            [   3.0 * L2,   2.0 * L1,        1.0,         0.0,   0.0,     0.0],
            [         L3,         L2,         L1,         1.0,   0.0,     0.0],
            [   3.0 * L4,   4.0 * L3,   6.0 * L2,   12.0 * L1,  12.0,   -12.0],
            [        3.0,        2.0,        1.0,         0.0,   0.0,     0.0],
            [        1.0,        1.0,        1.0,         1.0,   0.0,     0.0],
            [        3.0,        4.0,        6.0,        12.0,  12.0,     0.0],
        ]
    )
    eq_v = np.array(
        [
                  dh(gamma),
                   h(gamma),
            12.0 * H(gamma),
                1.0 - alpha,
                      alpha,
                12.0 * G(1),
        ]
    )
    A, B, C, D, cf, ch = la.inv(eq_mtx).dot(eq_v)

    H_p_ch = lambda x: H(x) + ch

    f = lambda x: ((A * x + B) * x + C) * x + D
    df = lambda x: (3.0 * A * x + 2.0 * B) * x + C
    F = lambda x: ((((A / 4.0) * x + B / 3.0) * x + C / 2.0) * x + D) * x + cf

    return (
        (combine_branches((h, f, g)), combine_branches((H_p_ch, F, G))),
        (A, B, C, D, cf, ch)
    )


def bit_crush(type_const, k, alpha, gamma):
    PI2k = PI2 * k
    W = 1.0 / (1.7 * pi * k)

    h_k = lambda x: x - W * sin(PI2k * x)
    dh_k = lambda x: 1 - W * cos(PI2k * x)
    H_k = lambda x: x ** 2.0 / 2.0 + W * cos(PI2k * x) / PI2k

    funcs, (A, B, C, D, cf, ch) = join(alpha, gamma, h_k, dh_k, H_k)

    print(f"""\

    initialize_bit_crush_tables(
        {type_const},
        {k},
        {alpha},
        {gamma},
        {A},
        {B},
        {C},
        {D},
        {cf},
        {ch}
    );""")

    return alpha, gamma, funcs


def harmonics(type_const, alpha, gamma, w1, w3, w5, norm=1.0):
    w1 *= norm
    w3 *= norm
    w5 *= norm

    t1 = lambda x: x
    t3 = lambda x: 4.0 * x ** 3.0 - 3.0 * x
    t5 = lambda x: 16.0 * x ** 5.0 - 20.0 * x ** 3.0 + 5.0 * x

    dt1 = lambda x: 1.0
    dt3 = lambda x: 12.0 * x ** 2.0 - 3.0
    dt5 = lambda x: 80.0 * x ** 4.0 - 60.0 * x ** 2.0 + 5.0

    T1 = lambda x: (1.0 / 2.0) * x ** 2.0
    T3 = lambda x: x ** 4.0 - (3.0 / 2.0) * x ** 2.0
    T5 = lambda x: (16.0 / 6.0) * x ** 6.0 - 5.0 * x ** 4.0 + (5.0 / 2.0) * x ** 2.0

    h = lambda x: w1 * t1(x) + w3 * t3(x) + w5 * t5(x)
    dh = lambda x: w1 * dt1(x) + w3 * dt3(x) + w5 * dt5(x)
    H = lambda x: w1 * T1(x) + w3 * T3(x) + w5 * T5(x)

    funcs, (A, B, C, D, cf, ch) = join(alpha, gamma, h, dh, H)

    print(f"""\

    initialize_harmonic_tables(
        {type_const},
        {w1},
        {w3},
        {w5},
        {alpha},
        {gamma},
        {A},
        {B},
        {C},
        {D},
        {cf},
        {ch}
    );""")

    return alpha, gamma, funcs


SQR = (4.0 / pi, 4.0 / (3.0 * pi), 4.0 / (5.0 * pi))
TRI = (8.0 / (1.0 * pi) ** 2.0, -8.0 / (3.0 * pi) ** 2.0, 8.0 / (5.0 * pi) ** 2.0)


DISTORTIONS = (
    (1.0, 1.0, (lambda x: tanh_f(x, 3.0), lambda x: tanh_F0(x, 3.0))),
    (1.0, 1.0, (lambda x: tanh_f(x, 5.0), lambda x: tanh_F0(x, 5.0))),
    (1.0, 1.0, (lambda x: tanh_f(x, 10.0), lambda x: tanh_F0(x, 10.0))),

    harmonics("TYPE_HARMONIC_13",    0.90, 0.80, 1.0, 1.0, 0.0, norm=1.10),
    harmonics("TYPE_HARMONIC_15",    0.90, 0.80, 1.0, 0.0, 1.0, norm=0.67),
    harmonics("TYPE_HARMONIC_135",   0.90, 0.80, 1.0, 1.0, 1.0, norm=1.20),
    harmonics("TYPE_HARMONIC_SQR",   0.90, 0.80, *SQR, norm=1.10),
    harmonics("TYPE_HARMONIC_TRI",   0.90, 0.80, *TRI, norm=1.20),

    bit_crush("TYPE_BIT_CRUSH_1",     1, 0.980, 0.720),
    bit_crush("TYPE_BIT_CRUSH_2",     2, 0.980, 0.630),
    bit_crush("TYPE_BIT_CRUSH_3",     4, 0.950, 0.810),
    bit_crush("TYPE_BIT_CRUSH_4",     8, 0.970, 0.903),
    bit_crush("TYPE_BIT_CRUSH_4_6",  12, 0.975, 0.936),
    bit_crush("TYPE_BIT_CRUSH_5",    16, 0.980, 0.953),
    bit_crush("TYPE_BIT_CRUSH_5_6",  24, 0.986, 0.968),
    bit_crush("TYPE_BIT_CRUSH_6",    32, 0.990, 0.976),
    bit_crush("TYPE_BIT_CRUSH_6_6",  48, 0.990, 0.984),
    bit_crush("TYPE_BIT_CRUSH_7",    64, 0.993, 0.988),
    bit_crush("TYPE_BIT_CRUSH_7_6",  96, 0.997, 0.992),
    bit_crush("TYPE_BIT_CRUSH_8",   128, 0.999, 0.994),
    bit_crush("TYPE_BIT_CRUSH_8_6", 192, 0.999, 0.993),
    bit_crush("TYPE_BIT_CRUSH_9",   256, 0.999, 0.995),
)


def main(argv):
    if len(argv) < 2:
        print(
            f"Usage: {os.path.basename(argv[0])} distortion_type",
            file=sys.stderr
        )

        return 1

    dist_type = int(argv[1])

    if not (0 <= dist_type < len(DISTORTIONS)):
        print(
            f"Distortion type must be between 0 and {len(DISTORTIONS) - 1}, got: {dist_type}",
            file=sys.stderr
        )

        return 2

    alpha, gamma, (f_pos, F0_pos) = DISTORTIONS[dist_type]

    f = lambda x: f_pos(x) if x >= 0.0 else -f_pos(-x)
    F0 = lambda x: F0_pos(x) if x >= 0.0 else F0_pos(-x)

    N = 70000
    width = 5.0
    prev = -width
    F0_prev = F0(prev)

    xs = []
    fs = []
    F0s = []
    guide = []
    approximations = []
    F0_x = 0.0
    f_x = 0.0
    M = 0.0

    for i in range(N):
        x = (2.0 * (i / N)) - 1.0
        x = width * x
        delta = x - prev

        if x < INPUT_MIN:
            F0_x = -x
            f_x = -1.0
        elif x >= INPUT_MAX:
            F0_x = x
            f_x = 1.0
        else:
            F0_x = F0(x)
            f_x = f(x)

        if x < -1.0:
            guide.append(-1.0)
        elif x > 1.0:
            guide.append(1.0)
        else:
            if 0 <= x <= gamma and abs(f_x) > M:
                M = abs(f_x)

            guide.append(x)

        xs.append(x)
        fs.append(f_x)
        F0s.append(F0_x)

        if abs(x - INPUT_MIN) < 0.0000001:
            approximations.append(-1.0)
        elif abs(x - INPUT_MAX) < 0.0000001:
            approximations.append(1.0)
        elif abs(delta) > 0.00000001:
            approximations.append((F0_x - F0_prev) / delta)
        else:
            approximations.append(f_x)

        prev = x
        F0_prev = F0_x

    print(f"max={M}", file=sys.stderr)
    print(f"norm={1.0 / M}", file=sys.stderr)
    print(f"norm_alpha={alpha / M}", file=sys.stderr)

    plt.plot(xs, guide)
    plt.plot(xs, fs)
    plt.plot(xs, approximations)
    plt.plot(xs, F0s)
    plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
