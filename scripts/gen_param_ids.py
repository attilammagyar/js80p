import sys


INDENTATION = " " * 4 * 3


def main(argv):
    param_id = 0

    param_id = print_main_params(param_id)
    param_id = print_oscillator_params(param_id, "Modulator", "M")
    param_id = print_oscillator_params(param_id, "Carrier", "C")
    param_id = print_effect_params(param_id)
    param_id = print_flexible_controllers_params(param_id)
    param_id = print_envelopes_params(param_id)
    param_id = print_lfo_params(param_id)
    param_id = print_special_params(param_id)

    print(f"{INDENTATION}MAX_PARAM_ID = {param_id}")

    return 0


def print_main_params(param_id: int) -> int:
    params = [
        ("MIX", "   ///< Modulator Additive Volume"),
        ("PM", "    ///< Phase Modulation"),
        ("FM", "    ///< Frequency Modulation"),
        ("AM", "    ///< Amplitude Modulation"),
    ]

    return print_params(param_id, "", "", 1, params)


def print_oscillator_params(param_id: int, group: str, prefix: str) -> int:
    params = [
        ("$AMP", "  ///< $ Amplitude"),
        ("$VS", "   ///< $ Velocity Sensitivity"),
        ("$FLD", "  ///< $ Folding"),
        ("$PRT", "  ///< $ Portamento Length"),
        ("$PRD", "  ///< $ Portamento Depth"),
        ("$DTN", "  ///< $ Detune"),
        ("$FIN", "  ///< $ Fine Detune"),
        ("$WID", "  ///< $ Width"),
        ("$PAN", "  ///< $ Pan"),
        ("$VOL", "  ///< $ Volume"),

        ("$C1", "   ///< $ Custom Waveform 1st Harmonic"),
        ("$C2", "   ///< $ Custom Waveform 2nd Harmonic"),
        ("$C3", "   ///< $ Custom Waveform 3rd Harmonic"),
        ("$C4", "   ///< $ Custom Waveform 4th Harmonic"),
        ("$C5", "   ///< $ Custom Waveform 5th Harmonic"),
        ("$C6", "   ///< $ Custom Waveform 6th Harmonic"),
        ("$C7", "   ///< $ Custom Waveform 7th Harmonic"),
        ("$C8", "   ///< $ Custom Waveform 8th Harmonic"),
        ("$C9", "   ///< $ Custom Waveform 9th Harmonic"),
        ("$C10", "  ///< $ Custom Waveform 10th Harmonic"),

        ("$F1FRQ", "///< $ Filter 1 Frequency"),
        ("$F1Q", "  ///< $ Filter 1 Q Factor"),
        ("$F1G", "  ///< $ Filter 1 Gain"),

        ("$F2FRQ", "///< $ Filter 2 Frequency"),
        ("$F2Q", "  ///< $ Filter 2 Q Factor"),
        ("$F2G", "  ///< $ Filter 2 Gain"),
    ]

    return print_params(param_id, group, prefix, 1, params)


def print_effect_params(param_id: int) -> int:
    params = [
        ("$OG", "   ///< $ Overdrive Gain"),

        ("$DG", "   ///< $ Distortion Gain"),

        ("$F1FRQ", "///< $ Filter 1 Frequency"),
        ("$F1Q", "  ///< $ Filter 1 Q Factor"),
        ("$F1G", "  ///< $ Filter 1 Gain"),

        ("$F2FRQ", "///< $ Filter 2 Frequency"),
        ("$F2Q", "  ///< $ Filter 2 Q Factor"),
        ("$F2G", "  ///< $ Filter 2 Gain"),

        ("$EDEL", " ///< $ Echo Delay"),
        ("$EFB", "  ///< $ Echo Feedback"),
        ("$EDF", "  ///< $ Echo Dampening Frequency"),
        ("$EDG", "  ///< $ Echo Dampening Gain"),
        ("$EWID", " ///< $ Echo Stereo Width"),
        ("$EHPF", " ///< $ Echo Highpass Frequency"),
        ("$EWET", " ///< $ Echo Wet Volume"),
        ("$EDRY", " ///< $ Echo Dry Volume"),

        ("$RRS", "  ///< $ Reverb Room Size"),
        ("$RDF", "  ///< $ Reverb Dampening Frequency"),
        ("$RDG", "  ///< $ Reverb Dampening Gain"),
        ("$RWID", " ///< $ Reverb Stereo Width"),
        ("$RHPF", " ///< $ Reverb Highpass Frequency"),
        ("$RWET", " ///< $ Reverb Wet Volume"),
        ("$RDRY", " ///< $ Reverb Dry Volume"),
    ]

    return print_params(param_id, "Effects", "E", 1, params)


def print_flexible_controllers_params(param_id: int) -> int:
    params = [
        ("$#IN", "  ///< $ # Input"),
        ("$#MIN", " ///< $ # Minimum Value"),
        ("$#MAX", " ///< $ # Maximum Value"),
        ("$#AMT", " ///< $ # Amount"),
        ("$#DST", " ///< $ # Distortion"),
        ("$#RND", " ///< $ # Randomness"),
    ]

    return print_params(param_id, "Flexible Controller", "F", 10, params)


def print_envelopes_params(param_id: int) -> int:
    params = [
        ("$#AMT", " ///< $ # Amount"),
        ("$#INI", " ///< $ # Initial Level"),
        ("$#DEL", " ///< $ # Delay Time"),
        ("$#ATK", " ///< $ # Attack Time"),
        ("$#PK", "  ///< $ # Peak Level"),
        ("$#HLD", " ///< $ # Hold Time"),
        ("$#DEC", " ///< $ # Decay Time"),
        ("$#SUS", " ///< $ # Sustain Level"),
        ("$#REL", " ///< $ # Release Time"),
        ("$#FIN", " ///< $ # Final Level"),
    ]

    return print_params(param_id, "Envelope", "N", 6, params)


def print_lfo_params(param_id: int) -> int:
    params = [
        ("$#FRQ", " ///< $ # Frequency"),
        ("$#PHS", " ///< $ # Phase"),
        ("$#MIN", " ///< $ # Minimum Value"),
        ("$#MAX", " ///< $ # Maximum Value"),
        ("$#AMT", " ///< $ # Amount"),
        ("$#DST", " ///< $ # Distortion"),
        ("$#RND", " ///< $ # Randomness"),
    ]

    return print_params(param_id, "LFO", "L", 8, params)


def print_special_params(param_id: int) -> int:
    params = [
        ("MODE", "  ///< Mode"),
        ("MWAV", "  ///< Modulator Waveform"),
        ("CWAV", "  ///< Carrier Waveform"),
        ("MF1TYP", "///< Modulator Filter 1 Type"),
        ("MF2TYP", "///< Modulator Filter 2 Type"),
        ("CF1TYP", "///< Carrier Filter 1 Type"),
        ("CF2TYP", "///< Carrier Filter 2 Type"),

        ("EF1TYP", "///< Effects Filter 1 Type"),
        ("EF2TYP", "///< Effects Filter 2 Type"),

        ("L1WAV", " ///< LFO 1 Waveform"),
        ("L2WAV", " ///< LFO 2 Waveform"),
        ("L3WAV", " ///< LFO 3 Waveform"),
        ("L4WAV", " ///< LFO 4 Waveform"),
        ("L5WAV", " ///< LFO 5 Waveform"),
        ("L6WAV", " ///< LFO 6 Waveform"),
        ("L7WAV", " ///< LFO 7 Waveform"),
        ("L8WAV", " ///< LFO 8 Waveform"),

        ("L1SYN", " ///< LFO 1 Tempo Synchronization"),
        ("L2SYN", " ///< LFO 2 Tempo Synchronization"),
        ("L3SYN", " ///< LFO 3 Tempo Synchronization"),
        ("L4SYN", " ///< LFO 4 Tempo Synchronization"),
        ("L5SYN", " ///< LFO 5 Tempo Synchronization"),
        ("L6SYN", " ///< LFO 6 Tempo Synchronization"),
        ("L7SYN", " ///< LFO 7 Tempo Synchronization"),
        ("L8SYN", " ///< LFO 8 Tempo Synchronization"),

        ("EESYN", " ///< Effects Echo Tempo Synchronization"),

        ("MF1LOG", "///< Modulator Filter 1 Logarithmic Frequency"),
        ("MF2LOG", "///< Modulator Filter 2 Logarithmic Frequency"),
        ("CF1LOG", "///< Carrier Filter 1 Logarithmic Frequency"),
        ("CF2LOG", "///< Carrier Filter 2 Logarithmic Frequency"),

        ("EF1LOG", "///< Effects Filter 1 Logarithmic Frequency"),
        ("EF2LOG", "///< Effects Filter 2 Logarithmic Frequency"),
    ]

    return print_params(param_id, "", "", 1, params)


def print_params(
        param_id: int,
        group: str,
        prefix: str,
        objects: int,
        object_params: list
) -> int:
    for i in range(objects):
        obj_id = str(i + 1)

        for name, comment in object_params:
            comment = comment.replace("#", obj_id).replace("$", group)
            name = name.replace("#", obj_id).replace("$", prefix)
            spaces = "   "

            if param_id < 100:
                spaces += " "

            if param_id < 10:
                spaces += " "

            print(f"{INDENTATION}{name} = {param_id}, {spaces}{comment}")
            param_id += 1

        print("")

    return param_id


if __name__ == "__main__":
    sys.exit(main(sys.argv))
