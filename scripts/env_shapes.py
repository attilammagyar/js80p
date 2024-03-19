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

from math import log


try:
    from PIL import Image, ImageDraw, ImageFilter, ImageEnhance, ImageChops

except ImportError as error:
    print(
        f"Unable to import from PIL, please install Pillow 7.0.0+ (python3-pillow) - error: {error}",
        file=sys.stderr
    )
    sys.exit(1)


BLACK = (0, 0, 0)
LIGHT_GREY = (200, 200, 200)
CYAN_1 = (120, 200, 230)
CYAN_2 = (140, 210, 240)
CYAN_3 = (160, 220, 250)
PURPLE_1 = (170, 100, 220)
PURPLE_2 = (200, 120, 240)
PURPLE_3 = (230, 140, 250)
YELLOW_1 = (255, 175, 120)
YELLOW_2 = (255, 215, 140)
YELLOW_3 = (255, 255, 160)

SUBPIXELS = 5

WIDTH = 21
HEIGHT = 21
SEGMENTS = 20
STROKE_WIDTH = int(SUBPIXELS * 3.5 + 0.5)


FUNCTIONS = (
    # Antiderivative of 6 * (x - x ^ 2).
    # Construction: the idea is to map [0, 1] to itself with a smooth function f
    # for which all of the following properties hold:
    #  1. f(0) = 0.
    #  2. f(1) = 1.
    #  3. f'(0) = f'(1) = 0 (ie. connect smoothly to the constant 0 and 1
    #     functions on the respective ends).
    #  4. f'(x) > 0 for all x where 0 < x < 1.
    #  5. f'(x) = f'(1 - x) for all x where 0 < x < 1.
    #  6. f''(1 / 2) = 0.
    #  7. f''(x) > 0 for all x where 0 <= x < 1 / 2.
    #  8. f''(x) < 0 for all x where 1 / 2 < x <= 1.
    # The 1 / 4 - (x - 1 / 2) ^ 2 function is almost perfect for the role of
    # f', but it needs to be scaled by 6 in order to make f fit the bill.
    # Expansion and simplification gives 6 * (x - x ^ 2).
    (CYAN_1, lambda x: ((-2.0 * x + 3.0) * x) * x),

    # Antiderivative of 30 * ((x - x ^ 2) ^ 2). Same idea as above but steeper.
    (CYAN_2, lambda x: ((((6.0 * x - 15.0) * x + 10.0) * x) * x) * x),

    # Antiderivative of 2772 * ((x - x ^ 2) ^ 5). Same idea as above but even
    # more steep.
    (CYAN_3, lambda x: ((((((((((-252.0 * x + 1386.0) * x - 3080.0) * x + 3465.0) * x - 1980.0) * x + 462.0) * x) * x) * x) * x) * x) * x),

    (PURPLE_1, lambda x: x ** 2.0),
    (PURPLE_2, lambda x: x ** 3.0),
    (PURPLE_3, lambda x: x ** 5.0),

    (YELLOW_1, lambda x: x * (1.0 - log(x + 0.001)) / (1.0 - log(1.001))),
    (YELLOW_2, lambda x: (x * (1.0 - log(x + 0.001)) / (1.0 - log(1.001))) ** (2.0 / 3.0)),
    (YELLOW_3, lambda x: (x * (1.0 - log(x + 0.001)) / (1.0 - log(1.001))) ** (1.0 / 3.0)),

    (LIGHT_GREY, lambda x: x),
)


def main(argv):
    out_dir = os.path.join(os.path.dirname(argv[0]), "../gui/img/")
    generate_env_shapes(os.path.join(out_dir, "env_shapes-01.png"), 0.0, 1.0)
    generate_env_shapes(os.path.join(out_dir, "env_shapes-10.png"), 1.0, 0.0)


def generate_env_shapes(out_file, start, end):
    env_shapes = Image.new("RGB", (WIDTH, HEIGHT * len(FUNCTIONS)), (0, 0, 0))
    env_shapes_canvas = ImageDraw.Draw(env_shapes)

    for i, (color, f) in enumerate(FUNCTIONS):
        plot = draw_plot(f, start, end, color)
        env_shapes.paste(plot, (0, HEIGHT * i, WIDTH, HEIGHT * (i + 1)))

    env_shapes.save(out_file)


def draw_plot(f, start, end, color):
    w = WIDTH * SUBPIXELS
    h = HEIGHT * SUBPIXELS
    plot = Image.new("RGB", (w, h), (0, 0, 0))
    plot_canvas = ImageDraw.Draw(plot)

    border = int(STROKE_WIDTH * 0.66 + 0.5)

    delta = end - start

    w -= border * 2
    h -= border * 2
    p = (border, h - int(h * start + 0.5) + border)

    segments = [(0 - border * 3, p[1]), p]

    for i in range(SEGMENTS):
        ratio = (i + 1) / SEGMENTS
        y = start + f(ratio) * delta
        p = (int(w * ratio + 0.5) + border, h - int(h * y + 0.5) + border)
        segments.append(p)

    segments.append((w + border * 3, p[1]))

    plot_canvas.line(segments, fill=color, width=STROKE_WIDTH, joint="curve")

    return plot.resize((WIDTH, HEIGHT), Image.BICUBIC)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
