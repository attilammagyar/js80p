import sys


INDENTATION = " " * 4 * 3


def main(argv):
    param_id = 96

    param_id = print_flexible_controllers(param_id)
    param_id = print_envelopes(param_id)


def print_flexible_controllers(param_id):
    FLEX_CTLS = 10
    FLEX_CTL_PARAMS = {
        "IN": " // Flexible Controller # Input",
        "MIN": "// Flexible Controller # Minimum Value",
        "MAX": "// Flexible Controller # Maximum Value",
        "AMT": "// Flexible Controller # Amount",
        "DST": "// Flexible Controller # Distortion",
        "RND": "// Flexible Controller # Randomness",
    }

    return print_params(param_id, "C", FLEX_CTLS, FLEX_CTL_PARAMS)


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

    return print_params(param_id, "E", ENVELOPES, ENVELOPE_PARAMS)


def print_params(param_id: int, prefix: str, objects: int, object_params: dict):
    for i in range(objects):
        obj_id = str(i + 1)

        for name, comment in object_params.items():
            comment = comment.replace("#", obj_id)
            spaces = "    " if param_id < 100 else "   "
            print(f"{INDENTATION}{prefix}{obj_id}{name} = {param_id}, {spaces}{comment}")
            param_id += 1

        print("")

    return param_id


if __name__ == "__main__":
    sys.exit(main(sys.argv))
