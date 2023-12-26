###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023  Attila M. Magyar
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


def main(argv):
    names = [
        ["A"],
        ["A_SHARP", "B_FLAT"],
        ["B"],
        ["C"],
        ["C_SHARP", "D_FLAT"],
        ["D"],
        ["D_SHARP", "E_FLAT"],
        ["E"],
        ["F"],
        ["F_SHARP", "G_FLAT"],
        ["G"],
        ["G_SHARP", "A_FLAT"],
    ]

    for i in range(107):
        n = 127 - i
        name = names[(n + 3) % 12]
        octave = int(n / 12) - 1

        for option in name:
            c = f"{option}_{octave}"
            print(f"Note const NOTE_{c:<31} = {n};")

    print("")

    for i in range(128):
        n = 127 - i
        print(f"Note const NOTE_{n:<31} = {n};")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
