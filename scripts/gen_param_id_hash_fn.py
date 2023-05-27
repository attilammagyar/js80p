import random
import sys
import time


# Inspiration from https://orlp.net/blog/worlds-smallest-hash-table/


LETTER_OFFSET = ord("A") - 10   # 0x41 - 10
DIGIT_OFFSET = ord("0")         # 0x30

params = [
    "MIX",
    "PM",
    "FM",
    "AM",
    "MAMP",
    "MVS",
    "MFLD",
    "MPRT",
    "MPRD",
    "MDTN",
    "MFIN",
    "MWID",
    "MPAN",
    "MVOL",
    "MC1",
    "MC2",
    "MC3",
    "MC4",
    "MC5",
    "MC6",
    "MC7",
    "MC8",
    "MC9",
    "MC10",
    "MF1FRQ",
    "MF1Q",
    "MF1G",
    "MF2FRQ",
    "MF2Q",
    "MF2G",
    "CAMP",
    "CVS",
    "CFLD",
    "CPRT",
    "CPRD",
    "CDTN",
    "CFIN",
    "CWID",
    "CPAN",
    "CVOL",
    "CC1",
    "CC2",
    "CC3",
    "CC4",
    "CC5",
    "CC6",
    "CC7",
    "CC8",
    "CC9",
    "CC10",
    "CF1FRQ",
    "CF1Q",
    "CF1G",
    "CF2FRQ",
    "CF2Q",
    "CF2G",
    "EOG",
    "EDG",
    "EF1FRQ",
    "EF1Q",
    "EF1G",
    "EF2FRQ",
    "EF2Q",
    "EF2G",
    "EEDEL",
    "EEFB",
    "EEDF",
    "EEDG",
    "EEWID",
    "EEHPF",
    "EEWET",
    "EEDRY",
    "ERRS",
    "ERDF",
    "ERDG",
    "ERWID",
    "ERHPF",
    "ERWET",
    "ERDRY",
    "F1IN",
    "F1MIN",
    "F1MAX",
    "F1AMT",
    "F1DST",
    "F1RND",
    "F2IN",
    "F2MIN",
    "F2MAX",
    "F2AMT",
    "F2DST",
    "F2RND",
    "F3IN",
    "F3MIN",
    "F3MAX",
    "F3AMT",
    "F3DST",
    "F3RND",
    "F4IN",
    "F4MIN",
    "F4MAX",
    "F4AMT",
    "F4DST",
    "F4RND",
    "F5IN",
    "F5MIN",
    "F5MAX",
    "F5AMT",
    "F5DST",
    "F5RND",
    "F6IN",
    "F6MIN",
    "F6MAX",
    "F6AMT",
    "F6DST",
    "F6RND",
    "F7IN",
    "F7MIN",
    "F7MAX",
    "F7AMT",
    "F7DST",
    "F7RND",
    "F8IN",
    "F8MIN",
    "F8MAX",
    "F8AMT",
    "F8DST",
    "F8RND",
    "F9IN",
    "F9MIN",
    "F9MAX",
    "F9AMT",
    "F9DST",
    "F9RND",
    "F10IN",
    "F10MIN",
    "F10MAX",
    "F10AMT",
    "F10DST",
    "F10RND",
    "N1AMT",
    "N1INI",
    "N1DEL",
    "N1ATK",
    "N1PK",
    "N1HLD",
    "N1DEC",
    "N1SUS",
    "N1REL",
    "N1FIN",
    "N2AMT",
    "N2INI",
    "N2DEL",
    "N2ATK",
    "N2PK",
    "N2HLD",
    "N2DEC",
    "N2SUS",
    "N2REL",
    "N2FIN",
    "N3AMT",
    "N3INI",
    "N3DEL",
    "N3ATK",
    "N3PK",
    "N3HLD",
    "N3DEC",
    "N3SUS",
    "N3REL",
    "N3FIN",
    "N4AMT",
    "N4INI",
    "N4DEL",
    "N4ATK",
    "N4PK",
    "N4HLD",
    "N4DEC",
    "N4SUS",
    "N4REL",
    "N4FIN",
    "N5AMT",
    "N5INI",
    "N5DEL",
    "N5ATK",
    "N5PK",
    "N5HLD",
    "N5DEC",
    "N5SUS",
    "N5REL",
    "N5FIN",
    "N6AMT",
    "N6INI",
    "N6DEL",
    "N6ATK",
    "N6PK",
    "N6HLD",
    "N6DEC",
    "N6SUS",
    "N6REL",
    "N6FIN",
    "L1FRQ",
    "L1PHS",
    "L1MIN",
    "L1MAX",
    "L1AMT",
    "L1DST",
    "L1RND",
    "L2FRQ",
    "L2PHS",
    "L2MIN",
    "L2MAX",
    "L2AMT",
    "L2DST",
    "L2RND",
    "L3FRQ",
    "L3PHS",
    "L3MIN",
    "L3MAX",
    "L3AMT",
    "L3DST",
    "L3RND",
    "L4FRQ",
    "L4PHS",
    "L4MIN",
    "L4MAX",
    "L4AMT",
    "L4DST",
    "L4RND",
    "L5FRQ",
    "L5PHS",
    "L5MIN",
    "L5MAX",
    "L5AMT",
    "L5DST",
    "L5RND",
    "L6FRQ",
    "L6PHS",
    "L6MIN",
    "L6MAX",
    "L6AMT",
    "L6DST",
    "L6RND",
    "L7FRQ",
    "L7PHS",
    "L7MIN",
    "L7MAX",
    "L7AMT",
    "L7DST",
    "L7RND",
    "L8FRQ",
    "L8PHS",
    "L8MIN",
    "L8MAX",
    "L8AMT",
    "L8DST",
    "L8RND",
    "MODE",
    "MWAV",
    "CWAV",
    "MF1TYP",
    "MF2TYP",
    "CF1TYP",
    "CF2TYP",
    "EF1TYP",
    "EF2TYP",
    "L1WAV",
    "L2WAV",
    "L3WAV",
    "L4WAV",
    "L5WAV",
    "L6WAV",
    "L7WAV",
    "L8WAV",
    "L1SYN",
    "L2SYN",
    "L3SYN",
    "L4SYN",
    "L5SYN",
    "L6SYN",
    "L7SYN",
    "L8SYN",
    "EESYN",
    "MF1LOG",
    "MF2LOG",
    "CF1LOG",
    "CF2LOG",
    "EF1LOG",
    "EF2LOG",
]


def main(argv):
    # for name in params:
        # h = compute_hash(name, 23781, 9, 128)
        # print(
            # f"assert_eq(Synth::ParamId::{name}, synth.find_param_id(\"{name}\"));"
        # )

    # return 0

    best_utilized = 0
    best_mod = 2 ** 31
    best_max_coll = 2 ** 31
    best_avg_len = 2 ** 31
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

                for name in params:
                    h = compute_hash(name, multiplier, shift, mod)
                    hashes.setdefault(h, []).append(name)

                max_coll = max(len(n) for n in hashes.values())
                avg_len = [len(n) for n in hashes.values()]
                avg_len = sum(avg_len) / len(avg_len)
                utilized = len(hashes.keys())

                if (
                    (max_coll < best_max_coll)
                    or (max_coll == best_max_coll and avg_len < best_avg_len)
                ):
                    delta_t = time.time() - start
                    best_utilized = utilized
                    best_max_coll = max_coll
                    best_avg_len = avg_len
                    best_mod = mod
                    print(
                        "\t".join([
                            f"time={delta_t}",
                            f"multiplier={multiplier}",
                            f"shift={shift}",
                            f"max_coll={max_coll}",
                            f"avg_len={avg_len}",
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
