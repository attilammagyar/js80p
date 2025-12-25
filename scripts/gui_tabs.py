###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2024, 2025  Attila M. Magyar
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

import math
import os.path
import sys


try:
    from PIL import Image, ImageDraw, ImageFilter, ImageEnhance, ImageChops

except ImportError as error:
    print(
        f"Unable to import from PIL, please install Pillow 7.0.0+ (python3-pillow) - error: {error}",
        file=sys.stderr
    )
    sys.exit(1)


WIDTH = 2076
HEIGHT = 1200
LEFT = 22


def save_tab_bg(file_name, mockup, top):
    tab = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
    cropped = mockup.crop((LEFT, top, LEFT + WIDTH, top + HEIGHT))
    cropped.save(file_name + ".png")


def main(argv):
    if len(argv) < 2:
        print(f"Usage: python3 {os.path.basename(argv[0])} gui-mockup-screenshot.png", file=sys.stderr)

        return 1

    dir_name = os.path.join(os.path.dirname(argv[0]), "../gui/img")
    mockup = Image.open(os.path.join(argv[1]))

    tabs = (
        (20 + 2 + (1200 + 20 + 2 + 2) * 0, "synth"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 1, "effects"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 2, "macros1"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 3, "macros2"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 4, "macros3"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 5, "envelopes1"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 6, "envelopes2"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 7, "lfos"),
        (20 + 2 + (1200 + 20 + 2 + 2) * 8, "about"),
    )

    for top, file_name in tabs:
        save_tab_bg(os.path.join(dir_name, file_name), mockup, top)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
