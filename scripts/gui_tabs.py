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


WIDTH = 980
HEIGHT = 600
LEFT = 11


def save_tab_bg(file_name, mockup, top):
    tab = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
    cropped = mockup.crop((LEFT, top, LEFT + WIDTH, top + HEIGHT))
    cropped.save(file_name + ".png")
    cropped.save(file_name + ".bmp")


def main(argv):
    if len(argv) < 2:
        print(f"Usage: python3 {os.path.basename(argv[0])} gui-mockup-screenshot.png", file=sys.stderr)

        return 1

    dir_name = os.path.join(os.path.dirname(argv[0]), "../gui/img")
    mockup = Image.open(os.path.join(argv[1]))

    tabs = (
        ("synth", 11),
        ("effects", 623),
        ("macros1", 1235),
        ("macros2", 1847),
        ("macros3", 2459),
        ("envelopes1", 3071),
        ("envelopes2", 3683),
        ("lfos", 4295),
        ("about", 4907),
    )

    for file_name, top in tabs:
        save_tab_bg(os.path.join(dir_name, file_name), mockup, top)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
