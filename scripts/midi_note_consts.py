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
