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

import fileinput
import os.path
import sys

import matplotlib.pyplot as plt


def main(argv):
    tsv = []
    cols = None

    for line_idx, line in enumerate(fileinput.input()):
        line = line.strip(" \r\n")

        if not line:
            continue

        row = line.split("\t")

        if cols is None:
            cols = len(row)
            tsv = [[] for i in range(cols)]
        elif len(row) != cols:
            raise Exception(
                f"Inconsistent number of columns in line {line_idx}, expected {cols} columns:\n{line!r}"
            )

        try:
            for i, col in enumerate(row):
                tsv[i].append(float(col))

        except Exception as e:
            print(
                f"NOTE: Ignoring line {line_idx}: {type(e)}: {e}\n      {line!r}",
                file=sys.stderr
            )
            continue

    if cols is None or cols < 2:
        raise Exception(f"Nothing to plot")

    for col in tsv[1:]:
        plt.plot(tsv[0], col)

    plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

