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

from math import log, tanh


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
GREEN = (100, 255, 140)
CYAN_1 = (120, 200, 230)
CYAN_2 = (140, 210, 240)
CYAN_3 = (160, 220, 250)
PURPLE_1 = (170, 100, 220)
PURPLE_2 = (200, 120, 240)
PURPLE_3 = (230, 140, 250)
YELLOW_1 = (255, 175, 120)
YELLOW_2 = (255, 215, 140)
YELLOW_3 = (255, 255, 160)
RED_1 = (210, 100, 100)
RED_2 = (230, 140, 140)
RED_3 = (250, 180, 180)

SUBPIXELS = 5

WIDTH = 21
HEIGHT = 21
SEGMENTS = 20
STROKE_WIDTH = int(SUBPIXELS * 3.5 + 0.5)


ENV_SHAPES = (
    # See Math::apply_envelope_shape().
    (CYAN_1, lambda x: ((-2.0 * x + 3.0) * x) * x),
    (CYAN_2, lambda x: ((((6.0 * x - 15.0) * x + 10.0) * x) * x) * x),
    (CYAN_3, lambda x: ((((((((((-252.0 * x + 1386.0) * x - 3080.0) * x + 3465.0) * x - 1980.0) * x + 462.0) * x) * x) * x) * x) * x) * x),

    (PURPLE_1, lambda x: x ** 2.0),
    (PURPLE_2, lambda x: x ** 3.0),
    (PURPLE_3, lambda x: x ** 5.0),

    (YELLOW_1, lambda x: x * (1.0 - log(x + 0.001)) / (1.0 - log(1.001))),
    (YELLOW_2, lambda x: (x * (1.0 - log(x + 0.001)) / (1.0 - log(1.001))) ** (2.0 / 3.0)),
    (YELLOW_3, lambda x: (x * (1.0 - log(x + 0.001)) / (1.0 - log(1.001))) ** (1.0 / 3.0)),

    (RED_1, lambda x: ((4.0 * x - 6.0) * x + 3.0) * x),
    (RED_2, lambda x: ((((16.0 * x - 40.0) * x + 40.0) * x - 20.0) * x + 5.0) * x),
    (RED_3, lambda x: ((((((((((1024.0 * x - 5632.0) * x + 14080.0) * x - 21120.0) * x + 21120.0) * x - 14784.0) * x + 7392.0) * x - 2640.0) * x + 660.0) * x - 110.0) * x + 11.0) * x),

    (LIGHT_GREY, lambda x: x),
)


MACRO_DISTORTIONS = (
    (CYAN_2, lambda x: tanh(8.0 * (x * 2.0 - 1.0)) * 0.5 + 0.5),
    (PURPLE_2, ENV_SHAPES[5][1]),
    (YELLOW_2, ENV_SHAPES[8][1]),
    (RED_2, ENV_SHAPES[11][1]),
)


def main(argv):
    out_dir = os.path.join(os.path.dirname(argv[0]), "../gui/img/")
    generate_image(
        os.path.join(out_dir, "env_shapes-01.png"), 0.0, 1.0, ENV_SHAPES
    )
    generate_image(
        os.path.join(out_dir, "env_shapes-10.png"), 1.0, 0.0, ENV_SHAPES
    )
    generate_image(
        os.path.join(out_dir, "macro_distortions.png"), 0.0, 1.0, MACRO_DISTORTIONS
    )
    generate_image(
        os.path.join(out_dir, "macro_midpoint_states.png"),
        0.0,
        1.0,
        tuple(
            make_midpoint_shift_function(i / 127.0)
            for i in range(128)
        )
    )


def generate_image(out_file, start, end, funcs):
    image = Image.new("RGB", (WIDTH, HEIGHT * len(funcs)), (0, 0, 0))

    for i, (color, f) in enumerate(funcs):
        plot = draw_plot(f, start, end, color)
        image.paste(plot, (0, HEIGHT * i, WIDTH, HEIGHT * (i + 1)))

    image.save(out_file)


def draw_plot(f, start, end, color):
    w = WIDTH * SUBPIXELS
    h = HEIGHT * SUBPIXELS
    plot_image = Image.new("RGB", (w, h), (0, 0, 0))
    plot_canvas = ImageDraw.Draw(plot_image)

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

    return plot_image.resize((WIDTH, HEIGHT), Image.BICUBIC)


def make_midpoint_shift_function(midpoint):
    func = lambda x: shift_midpoint(midpoint, x)
    color = None

    if midpoint <= 0.5:
        s = 2.0 * midpoint
        color = vint(vsum(vscale(1.0 - s, GREEN), vscale(s, LIGHT_GREY)))
    elif midpoint <= 0.75:
        s = 4.0 * (midpoint - 0.5)
        color = vint(vsum(vscale(1.0 - s, LIGHT_GREY), vscale(s, YELLOW_2)))
    else:
        s = 4.0 * (midpoint - 0.75)
        color = vint(vsum(vscale(1.0 - s, YELLOW_2), vscale(s, RED_1)))

    return (color, func)


def shift_midpoint(midpoint, x):
    x2 = 2.0 * x

    if x2 < 1.0:
        return x2 * midpoint

    return midpoint + (x2 - 1.0) * (1.0 - midpoint)


def vint(x):
    return tuple(int(xi) for xi in x)


def vsum(a, b):
    return tuple(ai + bi for ai, bi in zip(a, b))


def vscale(scalar, vector):
    return tuple(scalar * vi for vi in vector)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
