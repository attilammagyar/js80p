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

