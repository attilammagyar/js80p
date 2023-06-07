#!/usr/bin/python3

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


def main(argv):
    if len(argv) < 3:
        print(f"Usage: {os.path.basename(argv[0])} source.bmp destination.png", file=sys.stderr)
        return 1

    src = Image.open(argv[1])
    src.save(argv[2])

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
