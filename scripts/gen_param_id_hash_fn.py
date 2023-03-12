import random
import sys
import time


# Inspiration from https://orlp.net/blog/worlds-smallest-hash-table/


LETTER_OFFSET = ord("A") - 10   # 0x41 - 10
DIGIT_OFFSET = ord("0")         # 0x30

params = [
    ("VOL", 0),
    ("ADD", 1),
    ("FM", 2),
    ("AM", 3),
    ("MAMP", 4),
    ("MVS", 5),
    ("MFLD", 6),
    ("MPRT", 7),
    ("MPRD", 8),
    ("MDTN", 9),
    ("MFIN", 10),
    ("MWID", 11),
    ("MPAN", 12),
    ("MVOL", 13),
    ("MC1", 14),
    ("MC2", 15),
    ("MC3", 16),
    ("MC4", 17),
    ("MC5", 18),
    ("MC6", 19),
    ("MC7", 20),
    ("MC8", 21),
    ("MC9", 22),
    ("MC10", 23),
    ("MF1FRQ", 24),
    ("MF1Q", 25),
    ("MF1G", 26),
    ("MF2FRQ", 27),
    ("MF2Q", 28),
    ("MF2G", 29),
    ("CAMP", 30),
    ("CVS", 31),
    ("CFLD", 32),
    ("CPRT", 33),
    ("CPRD", 34),
    ("CDTN", 35),
    ("CFIN", 36),
    ("CWID", 37),
    ("CPAN", 38),
    ("CVOL", 39),
    ("CC1", 40),
    ("CC2", 41),
    ("CC3", 42),
    ("CC4", 43),
    ("CC5", 44),
    ("CC6", 45),
    ("CC7", 46),
    ("CC8", 47),
    ("CC9", 48),
    ("CC10", 49),
    ("CF1FRQ", 50),
    ("CF1Q", 51),
    ("CF1G", 52),
    ("CF2FRQ", 53),
    ("CF2Q", 54),
    ("CF2G", 55),
    ("EOG", 56),
    ("EDG", 57),
    ("EF1FRQ", 58),
    ("EF1Q", 59),
    ("EF1G", 60),
    ("EF2FRQ", 61),
    ("EF2Q", 62),
    ("EF2G", 63),
    ("EEDEL", 64),
    ("EEFB", 65),
    ("EEDF", 66),
    ("EEDG", 67),
    ("EEWID", 68),
    ("EEHPF", 69),
    ("EEWET", 70),
    ("EEDRY", 71),
    ("ERRS", 72),
    ("ERDF", 73),
    ("ERDG", 74),
    ("ERWID", 75),
    ("ERHPF", 76),
    ("ERWET", 77),
    ("ERDRY", 78),
    ("F1IN", 79),
    ("F1MIN", 80),
    ("F1MAX", 81),
    ("F1AMT", 82),
    ("F1DST", 83),
    ("F1RND", 84),
    ("F2IN", 85),
    ("F2MIN", 86),
    ("F2MAX", 87),
    ("F2AMT", 88),
    ("F2DST", 89),
    ("F2RND", 90),
    ("F3IN", 91),
    ("F3MIN", 92),
    ("F3MAX", 93),
    ("F3AMT", 94),
    ("F3DST", 95),
    ("F3RND", 96),
    ("F4IN", 97),
    ("F4MIN", 98),
    ("F4MAX", 99),
    ("F4AMT", 100),
    ("F4DST", 101),
    ("F4RND", 102),
    ("F5IN", 103),
    ("F5MIN", 104),
    ("F5MAX", 105),
    ("F5AMT", 106),
    ("F5DST", 107),
    ("F5RND", 108),
    ("F6IN", 109),
    ("F6MIN", 110),
    ("F6MAX", 111),
    ("F6AMT", 112),
    ("F6DST", 113),
    ("F6RND", 114),
    ("F7IN", 115),
    ("F7MIN", 116),
    ("F7MAX", 117),
    ("F7AMT", 118),
    ("F7DST", 119),
    ("F7RND", 120),
    ("F8IN", 121),
    ("F8MIN", 122),
    ("F8MAX", 123),
    ("F8AMT", 124),
    ("F8DST", 125),
    ("F8RND", 126),
    ("F9IN", 127),
    ("F9MIN", 128),
    ("F9MAX", 129),
    ("F9AMT", 130),
    ("F9DST", 131),
    ("F9RND", 132),
    ("F10IN", 133),
    ("F10MIN", 134),
    ("F10MAX", 135),
    ("F10AMT", 136),
    ("F10DST", 137),
    ("F10RND", 138),
    ("N1AMT", 139),
    ("N1INI", 140),
    ("N1DEL", 141),
    ("N1ATK", 142),
    ("N1PK", 143),
    ("N1HLD", 144),
    ("N1DEC", 145),
    ("N1SUS", 146),
    ("N1REL", 147),
    ("N1FIN", 148),
    ("N2AMT", 149),
    ("N2INI", 150),
    ("N2DEL", 151),
    ("N2ATK", 152),
    ("N2PK", 153),
    ("N2HLD", 154),
    ("N2DEC", 155),
    ("N2SUS", 156),
    ("N2REL", 157),
    ("N2FIN", 158),
    ("N3AMT", 159),
    ("N3INI", 160),
    ("N3DEL", 161),
    ("N3ATK", 162),
    ("N3PK", 163),
    ("N3HLD", 164),
    ("N3DEC", 165),
    ("N3SUS", 166),
    ("N3REL", 167),
    ("N3FIN", 168),
    ("N4AMT", 169),
    ("N4INI", 170),
    ("N4DEL", 171),
    ("N4ATK", 172),
    ("N4PK", 173),
    ("N4HLD", 174),
    ("N4DEC", 175),
    ("N4SUS", 176),
    ("N4REL", 177),
    ("N4FIN", 178),
    ("N5AMT", 179),
    ("N5INI", 180),
    ("N5DEL", 181),
    ("N5ATK", 182),
    ("N5PK", 183),
    ("N5HLD", 184),
    ("N5DEC", 185),
    ("N5SUS", 186),
    ("N5REL", 187),
    ("N5FIN", 188),
    ("N6AMT", 189),
    ("N6INI", 190),
    ("N6DEL", 191),
    ("N6ATK", 192),
    ("N6PK", 193),
    ("N6HLD", 194),
    ("N6DEC", 195),
    ("N6SUS", 196),
    ("N6REL", 197),
    ("N6FIN", 198),
    ("L1FRQ", 199),
    ("L1AMT", 200),
    ("L1MIN", 201),
    ("L1MAX", 202),
    ("L1DST", 203),
    ("L1RND", 204),
    ("L2FRQ", 205),
    ("L2AMT", 206),
    ("L2MIN", 207),
    ("L2MAX", 208),
    ("L2DST", 209),
    ("L2RND", 210),
    ("L3FRQ", 211),
    ("L3AMT", 212),
    ("L3MIN", 213),
    ("L3MAX", 214),
    ("L3DST", 215),
    ("L3RND", 216),
    ("L4FRQ", 217),
    ("L4AMT", 218),
    ("L4MIN", 219),
    ("L4MAX", 220),
    ("L4DST", 221),
    ("L4RND", 222),
    ("L5FRQ", 223),
    ("L5AMT", 224),
    ("L5MIN", 225),
    ("L5MAX", 226),
    ("L5DST", 227),
    ("L5RND", 228),
    ("L6FRQ", 229),
    ("L6AMT", 230),
    ("L6MIN", 231),
    ("L6MAX", 232),
    ("L6DST", 233),
    ("L6RND", 234),
    ("L7FRQ", 235),
    ("L7AMT", 236),
    ("L7MIN", 237),
    ("L7MAX", 238),
    ("L7DST", 239),
    ("L7RND", 240),
    ("L8FRQ", 241),
    ("L8AMT", 242),
    ("L8MIN", 243),
    ("L8MAX", 244),
    ("L8DST", 245),
    ("L8RND", 246),
    ("MODE", 247),
    ("MWAV", 248),
    ("CWAV", 249),
    ("MF1TYP", 250),
    ("MF2TYP", 251),
    ("CF1TYP", 252),
    ("CF2TYP", 253),
    ("EF1TYP", 254),
    ("EF2TYP", 255),
    ("L1WAV", 256),
    ("L2WAV", 257),
    ("L3WAV", 258),
    ("L4WAV", 259),
    ("L5WAV", 260),
    ("L6WAV", 261),
    ("L7WAV", 262),
    ("L8WAV", 263),
]


def main(argv):
    # for name, _ in params:
        # h = compute_hash(name, 23781, 9, 128)
        # print(
            # f"assert_eq(Synth::ParamId::{name}, synth.find_param_id(\"{name}\"));"
        # )

    # return 0

    best_utilized = 0
    best_mod = 2 ** 31
    best_max_coll = 2 ** 31
    best_avg_coll = 2 ** 31
    start = time.time()
    delta = 0

    for i in range(1, 100000):
        # multiplier = random.randrange(2 ** 15)
        multiplier = i
        multiplier = multiplier * 2 + 1

        for shift in range(23):
            # for mod in range(220, 290):
            for mod in [128]:
                hashes = {}

                for name, param_id in params:
                    h = compute_hash(name, multiplier, shift, mod)
                    hashes.setdefault(h, []).append(name)

                max_coll = max(len(n) for n in hashes.values())
                avg_coll = [len(n) for n in hashes.values() if len(n) > 1]
                avg_coll = sum(avg_coll) / len(avg_coll)
                utilized = len(hashes.keys())

                if (
                    (max_coll < best_max_coll)
                    or (max_coll == best_max_coll and avg_coll < best_avg_coll)
                ):
                    delta_t = time.time() - start
                    best_utilized = utilized
                    best_max_coll = max_coll
                    best_avg_coll = avg_coll
                    best_mod = mod
                    print(
                        "\t".join([
                            f"time={delta_t}",
                            f"multiplier={multiplier}",
                            f"shift={shift}",
                            f"max_coll={max_coll}",
                            f"avg_coll={avg_coll}",
                            f"mod={mod}",
                            f"utilized={len(hashes)}"
                        ])
                    )


def compute_hash(text: str, multiplier: int, shift: int, mod: int) -> int:
    h = 0

    for i, c in enumerate(text):
        c = ord(c)

        if c >= LETTER_OFFSET:
            c -= LETTER_OFFSET
        else:
            c -= DIGIT_OFFSET

        h *= 36
        h += c

        if i == 4:
            break

    h <<= 3
    h += i
    h = (h * multiplier) >> shift
    h = h % mod

    return h


if __name__ == "__main__":
    sys.exit(main(sys.argv))
