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


# Parameter: f'(0) := beta
# Increasing beta reduces the distortion.
beta = 0.0

# Parameter: f(1) := alpha
# This formula picks a large alpha for the given beta.
alpha = (beta + 5.0 + sqrt(beta ** 2.0 - 6.0 * beta + 5.0)) / 8.0 - 0.001

alpha_m_1_o_4 = (alpha - 1.0) / 4.0


A = 1.0 - 3.0 * alpha + beta
B = 4.0 * alpha - 2.0 * beta - 1.0
C = beta
cf = (-5.0 * alpha + 7.0 - beta / 3.0) / 4.0

cg = -9.0 * alpha_m_1_o_4


f = lambda x: ((A * x + B) * x + C) * x
F = lambda x: ((((A / 4.0) * x + (B / 3.0)) * x + (C / 2.0)) * x) * x + cf


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


def check_spline_params():
    F_eq_mtx = np.array(
        [
            [1.0, 1.0, 0.0],
            [3.0, 2.0, 0.0],
            [3.0, 4.0, 12.0],
        ]
    )
    F_eq = np.array(
        [
            alpha - beta,
            1.0 - alpha - beta,
            19.0 * alpha - 7.0 + 12.0 * cg - 6.0 * C
        ]
    )
    A_cmp, B_cmp, cf_cmp = la.inv(F_eq_mtx).dot(F_eq)

    checks = (
        ("A", A, A_cmp),
        ("B", B, B_cmp),
        ("cf", cf, cf_cmp),
    )

    for name, value, cmp in checks:
        if abs(value - cmp) > 0.000001:
            raise ValueError(
                f"Calculation error in {name}:\n"
                f"      manual: {value}\n"
                f"    computed: {cmp}\n"
                f"        diff: {abs(value - cmp)}"
            )


def spline(x):
    sgn = 1.0

    if x < 0.0:
        sgn = -1.0
        x = -x

    if x >= INPUT_MAX:
        return sgn

    if x >= 1.0:
        return sgn * g(x)

    return sgn * f(x)


def spline_int(x):
    if x < 0.0:
        x = -x

    if x >= INPUT_MAX:
        return x

    if x >= 1.0:
        return G(x)

    return F(x)


def spline_n_times(x, n):
    for i in range(n):
        x = spline(x)

    return x


def main(argv):
    N = 20000
    width = 5.0

    check_spline_params()

    funcs = (
        ([], lambda x: 0),
        ([], lambda x: x),
        ([], lambda x: spline(x)),
        ([], lambda x: spline_int(x)),
        ([], lambda x: spline_n_times(x, 5)),
        ([], lambda x: spline_n_times(x, 10)),
        ([], lambda x: spline_n_times(x, 25)),
    )
    values = tuple([] for f in funcs)

    xs = []

    for i in range(N):
        x = (2.0 * (i / N)) - 1.0
        x = width * x

        xs.append(x)

        for values, f in funcs:
            values.append(f(x))

    print(f"""
alpha = {alpha}
beta = {beta}

A = {A}
B = {B}
C = {C}
cf = {cf}

cg = {cg}

""")

    for values, _ in funcs:
        plt.plot(xs, values)

    plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
