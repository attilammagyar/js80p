###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2025, 2026  Attila M. Magyar
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

import re
import sys


def main(argv):
    last_num_re = re.compile(r"([+-]?[0-9]+(\.[0-9]+)?)$")

    with open("gui/mockup.html", "r") as f:
        in_html = f.read()

    out_html = []

    for chunk in in_html.split("px"):
        if last_num := last_num_re.search(chunk):
            double = 2 * float(last_num[1])
            double = int(double) if double.is_integer() else double
            out_html.append(chunk[:-len(last_num[1])] + str(double))
        else:
            out_html.append(chunk)

    print("px".join(out_html))

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
