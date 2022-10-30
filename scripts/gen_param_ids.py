import sys


INDENTATION = " " * 4 * 3


def main(argv):
    param_id = 96

    param_id = print_envelopes(param_id)


def print_envelopes(param_id):
    ENVELOPES = 6
    ENVELOPE_PARAMS = {
        "AMT": "// Envelope # Amount",
        "INI": "// Envelope # Initial Level",
        "DEL": "// Envelope # Delay Time",
        "ATK": "// Envelope # Attack Time",
        "PK": " // Envelope # Peak Level",
        "HLD": "// Envelope # Hold Time",
        "DEC": "// Envelope # Decay Time",
        "SUS": "// Envelope # Sustain Level",
        "REL": "// Envelope # Release Time",
        "FIN": "// Envelope # Final Level",
    }


    for i in range(ENVELOPES):
        for name, comment in ENVELOPE_PARAMS.items():
            envelope = str(i + 1)
            comment = comment.replace("#", envelope)
            spaces = "    " if param_id < 100 else "   "
            print(f"{INDENTATION}E{envelope}{name} = {param_id}, {spaces}{comment}")
            param_id += 1

        print("")


if __name__ == "__main__":
    sys.exit(main(sys.argv))
