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
    colors = (
        ("CTL_COLOR_MIDI_CC", rgb(255, 255, 120)),
        ("CTL_COLOR_MIDI_SPECIAL", rgb(255, 220, 150)),
        ("CTL_COLOR_MIDI_LEARN", rgb(90, 120, 230)),
        ("CTL_COLOR_AFTERTOUCH", rgb(255, 160, 110)),
        ("CTL_COLOR_MACRO", rgb(110, 190, 255)),
        ("CTL_COLOR_LFO", rgb(230, 100, 255)),
        ("CTL_COLOR_ENVELOPE", rgb(110, 255, 150)),
    )

    for name, text_color in colors:
        tr, tg, tb = text_color
        br, bg, bb = darken(text_color)
        print(f"""\
const GUI::Color GUI::{name}_TEXT = GUI::rgb({tr}, {tg}, {tb});
const GUI::Color GUI::{name}_BG = GUI::rgb({br}, {bg}, {bb});
""")

    return 0


def rgb(r, g, b):
    return (r, g, b)


def darken(color):
    r, g, b = color
    s = 0.57

    return rgb(
        int(r * s + 0.5),
        int(g * s + 0.5),
        int(b * s + 0.5),
    )


if __name__ == "__main__":
    sys.exit(main(sys.argv))
