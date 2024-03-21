###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
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

import sys


INDENTATION = " " * 4 * 3


def main(argv):
    param_id = 0
    param_objs = []

    param_id = print_main_params(param_id, param_objs)
    param_id = print_oscillator_params(param_id, param_objs, "Modulator", "M")
    param_id = print_oscillator_params(param_id, param_objs, "Carrier", "C")
    param_id = print_effect_params(param_id, param_objs)
    param_id = print_macros(param_id, param_objs)
    param_id = print_envelopes_params(param_id, param_objs)
    param_id = print_lfo_params(param_id, param_objs)
    param_id = print_discrete_params(param_id, param_objs)

    print(f"{INDENTATION}PARAM_ID_COUNT = {param_id},")
    print(f"{INDENTATION}INVALID_PARAM_ID = PARAM_ID_COUNT,")

    print("")

    print_cases(param_objs, "return $.get_default_ratio();");
    print_cases(param_objs, "return $.get_max_value();");
    print_cases(param_objs, "return $.ratio_to_value(ratio);");
    print_cases(param_objs, "$.set_ratio(ratio); break;");
    print_cases(param_objs, "return $.get_ratio();");

    return 0


def print_main_params(param_id: int, param_objs: list) -> int:
    params = [
        ("MIX", "   ///< Modulator Additive Volume", "modulator_add_volume"),
        ("PM", "    ///< Phase Modulation", "phase_modulation_level"),
        ("FM", "    ///< Frequency Modulation", "frequency_modulation_level"),
        ("AM", "    ///< Amplitude Modulation", "amplitude_modulation_level"),
    ]

    return print_params(param_id, param_objs, "", "", 1, params)


def print_oscillator_params(param_id: int, param_objs: list, group: str, prefix: str) -> int:
    osc_name = "modulator" if prefix == "M" else "carrier"

    params = [
        ("$AMP", "  ///< $ Amplitude", osc_name + "_params.amplitude"),
        ("$VS", "   ///< $ Velocity Sensitivity", osc_name + "_params.velocity_sensitivity"),
        ("$FLD", "  ///< $ Folding", osc_name + "_params.folding"),
        ("$PRT", "  ///< $ Portamento Length", osc_name + "_params.portamento_length"),
        ("$PRD", "  ///< $ Portamento Depth", osc_name + "_params.portamento_depth"),
        ("$DTN", "  ///< $ Detune", osc_name + "_params.detune"),
        ("$FIN", "  ///< $ Fine Detune", osc_name + "_params.fine_detune"),
        ("$WID", "  ///< $ Width", osc_name + "_params.width"),
        ("$PAN", "  ///< $ Pan", osc_name + "_params.panning"),
        ("$VOL", "  ///< $ Volume", osc_name + "_params.volume"),
    ]

    if prefix == "M":
        params += [
            ("$SUB", "  ///< $ Subharmonic Amplitude", osc_name + "_params.subharmonic_amplitude"),
        ]
    elif prefix == "C":
        params += [
            ("$DG", "   ///< $ Distortion Gain", osc_name + "_params.distortion"),
        ]

    params += [
        ("$C1", "   ///< $ Custom Waveform 1st Harmonic", osc_name + "_params.harmonic_0"),
        ("$C2", "   ///< $ Custom Waveform 2nd Harmonic", osc_name + "_params.harmonic_1"),
        ("$C3", "   ///< $ Custom Waveform 3rd Harmonic", osc_name + "_params.harmonic_2"),
        ("$C4", "   ///< $ Custom Waveform 4th Harmonic", osc_name + "_params.harmonic_3"),
        ("$C5", "   ///< $ Custom Waveform 5th Harmonic", osc_name + "_params.harmonic_4"),
        ("$C6", "   ///< $ Custom Waveform 6th Harmonic", osc_name + "_params.harmonic_5"),
        ("$C7", "   ///< $ Custom Waveform 7th Harmonic", osc_name + "_params.harmonic_6"),
        ("$C8", "   ///< $ Custom Waveform 8th Harmonic", osc_name + "_params.harmonic_7"),
        ("$C9", "   ///< $ Custom Waveform 9th Harmonic", osc_name + "_params.harmonic_8"),
        ("$C10", "  ///< $ Custom Waveform 10th Harmonic", osc_name + "_params.harmonic_9"),

        ("$F1FRQ", "///< $ Filter 1 Frequency", osc_name + "_params.filter_1_frequency"),
        ("$F1Q", "  ///< $ Filter 1 Q Factor", osc_name + "_params.filter_1_q"),
        ("$F1G", "  ///< $ Filter 1 Gain", osc_name + "_params.filter_1_gain"),
        ("$F1FIA", "///< $ Filter 1 Frequency Inaccuracy", osc_name + "_params.filter_1_freq_inaccuracy"),
        ("$F1QIA", "///< $ Filter 1 Q Factor Inaccuracy", osc_name + "_params.filter_1_q_inaccuracy"),

        ("$F2FRQ", "///< $ Filter 2 Frequency", osc_name + "_params.filter_2_frequency"),
        ("$F2Q", "  ///< $ Filter 2 Q Factor", osc_name + "_params.filter_2_q"),
        ("$F2G", "  ///< $ Filter 2 Gain", osc_name + "_params.filter_2_gain"),
        ("$F2FIA", "///< $ Filter 2 Frequency Inaccuracy", osc_name + "_params.filter_2_freq_inaccuracy"),
        ("$F2QIA", "///< $ Filter 2 Q Factor Inaccuracy", osc_name + "_params.filter_2_q_inaccuracy"),
    ]

    return print_params(param_id, param_objs, group, prefix, 1, params)


def print_effect_params(param_id: int, param_objs: list) -> int:
    params = [
        ("$V1V", "  ///< $ Volume 1", "effects.volume_1_gain"),

        ("$OG", "   ///< $ Overdrive Gain", "effects.overdrive.level"),

        ("$DG", "   ///< $ Distortion Gain", "effects.distortion.level"),

        ("$F1FRQ", "///< $ Filter 1 Frequency", "effects.filter_1.frequency"),
        ("$F1Q", "  ///< $ Filter 1 Q Factor", "effects.filter_1.q"),
        ("$F1G", "  ///< $ Filter 1 Gain", "effects.filter_1.gain"),

        ("$F2FRQ", "///< $ Filter 2 Frequency", "effects.filter_2.frequency"),
        ("$F2Q", "  ///< $ Filter 2 Q Factor", "effects.filter_2.q"),
        ("$F2G", "  ///< $ Filter 2 Gain", "effects.filter_2.gain"),

        ("$V2V", "  ///< $ Volume 2", "effects.volume_2_gain"),

        ("$CDEL", " ///< $ Chorus Delay", "effects.chorus.delay_time"),
        ("$CFRQ", " ///< $ Chorus LFO Frequency", "effects.chorus.frequency"),
        ("$CDPT", " ///< $ Chorus Depth", "effects.chorus.depth"),
        ("$CFB", "  ///< $ Chorus Feedback", "effects.chorus.feedback"),
        ("$CDF", "  ///< $ Chorus Dampening Frequency", "effects.chorus.damping_frequency"),
        ("$CDG", "  ///< $ Chorus Dampening Gain", "effects.chorus.damping_gain"),
        ("$CWID", " ///< $ Chorus Stereo Width", "effects.chorus.width"),
        ("$CHPF", " ///< $ Chorus Highpass Frequency", "effects.chorus.high_pass_frequency"),
        ("$CWET", " ///< $ Chorus Wet Volume", "effects.chorus.wet"),
        ("$CDRY", " ///< $ Chorus Dry Volume", "effects.chorus.dry"),

        ("$EDEL", " ///< $ Echo Delay", "effects.echo.delay_time"),
        ("$EFB", "  ///< $ Echo Feedback", "effects.echo.feedback"),
        ("$EDF", "  ///< $ Echo Dampening Frequency", "effects.echo.damping_frequency"),
        ("$EDG", "  ///< $ Echo Dampening Gain", "effects.echo.damping_gain"),
        ("$EWID", " ///< $ Echo Stereo Width", "effects.echo.width"),
        ("$EHPF", " ///< $ Echo Highpass Frequency", "effects.echo.high_pass_frequency"),
        ("$ECTH", " ///< $ Echo Side-Chain Compression Threshold", "effects.echo.side_chain_compression_threshold"),
        ("$ECAT", " ///< $ Echo Side-Chain Compression Attack Time", "effects.echo.side_chain_compression_attack_time"),
        ("$ECRL", " ///< $ Echo Side-Chain Compression Release Time", "effects.echo.side_chain_compression_release_time"),
        ("$ECR", "  ///< $ Echo Side-Chain Compression Ratio", "effects.echo.side_chain_compression_ratio"),
        ("$EWET", " ///< $ Echo Wet Volume", "effects.echo.wet"),
        ("$EDRY", " ///< $ Echo Dry Volume", "effects.echo.dry"),

        ("$RRS", "  ///< $ Reverb Room Size", "effects.reverb.room_size"),
        ("$RDF", "  ///< $ Reverb Dampening Frequency", "effects.reverb.damping_frequency"),
        ("$RDG", "  ///< $ Reverb Dampening Gain", "effects.reverb.damping_gain"),
        ("$RWID", " ///< $ Reverb Stereo Width", "effects.reverb.width"),
        ("$RHPF", " ///< $ Reverb Highpass Frequency", "effects.reverb.high_pass_frequency"),
        ("$RCTH", " ///< $ Reverb Side-Chain Compression Threshold", "effects.reverb.side_chain_compression_threshold"),
        ("$RCAT", " ///< $ Reverb Side-Chain Compression Attack Time", "effects.reverb.side_chain_compression_attack_time"),
        ("$RCRL", " ///< $ Reverb Side-Chain Compression Release Time", "effects.reverb.side_chain_compression_release_time"),
        ("$RCR", "  ///< $ Reverb Side-Chain Compression Ratio", "effects.reverb.side_chain_compression_ratio"),
        ("$RWET", " ///< $ Reverb Wet Volume", "effects.reverb.wet"),
        ("$RDRY", " ///< $ Reverb Dry Volume", "effects.reverb.dry"),

        ("$V3V", "  ///< $ Volume 3", "effects.volume_3_gain"),
    ]

    return print_params(param_id, param_objs, "Effects", "E", 1, params)


def print_macros(param_id: int, param_objs: list) -> int:
    params = [
        ("$#IN", "  ///< $ # Input", ""),
        ("$#MIN", " ///< $ # Minimum Value", ""),
        ("$#MAX", " ///< $ # Maximum Value", ""),
        ("$#AMT", " ///< $ # Amount", ""),
        ("$#DST", " ///< $ # Distortion", ""),
        ("$#RND", " ///< $ # Randomness", ""),
    ]

    return print_params(param_id, param_objs, "Macro", "M", 20, params)


def print_envelopes_params(param_id: int, param_objs: list) -> int:
    params = [
        ("$#AMT", " ///< $ # Amount", ""),
        ("$#INI", " ///< $ # Initial Level", ""),
        ("$#DEL", " ///< $ # Delay Time", ""),
        ("$#ATK", " ///< $ # Attack Time", ""),
        ("$#PK", "  ///< $ # Peak Level", ""),
        ("$#HLD", " ///< $ # Hold Time", ""),
        ("$#DEC", " ///< $ # Decay Time", ""),
        ("$#SUS", " ///< $ # Sustain Level", ""),
        ("$#REL", " ///< $ # Release Time", ""),
        ("$#FIN", " ///< $ # Final Level", ""),
        ("$#TIN", " ///< $ # Time Inaccuracy", ""),
        ("$#VIN", " ///< $ # Level Inaccuracy", ""),
    ]

    return print_params(param_id, param_objs, "Envelope", "N", 12, params)


def print_lfo_params(param_id: int, param_objs: list) -> int:
    params = [
        ("$#FRQ", " ///< $ # Frequency", ""),
        ("$#PHS", " ///< $ # Phase", ""),
        ("$#MIN", " ///< $ # Minimum Value", ""),
        ("$#MAX", " ///< $ # Maximum Value", ""),
        ("$#AMT", " ///< $ # Amount", ""),
        ("$#DST", " ///< $ # Distortion", ""),
        ("$#RND", " ///< $ # Randomness", ""),
    ]

    return print_params(param_id, param_objs, "LFO", "L", 8, params)


def print_discrete_params(param_id: int, param_objs: list) -> int:
    params = [
        ("MODE", "  ///< Mode", "mode"),
        ("MWAV", "  ///< Modulator Waveform", "modulator_params.waveform"),
        ("CWAV", "  ///< Carrier Waveform", "carrier_params.waveform"),
        ("MF1TYP", "///< Modulator Filter 1 Type", "modulator_params.filter_1_type"),
        ("MF2TYP", "///< Modulator Filter 2 Type", "modulator_params.filter_2_type"),
        ("CF1TYP", "///< Carrier Filter 1 Type", "carrier_params.filter_1_type"),
        ("CF2TYP", "///< Carrier Filter 2 Type", "carrier_params.filter_2_type"),

        ("EF1TYP", "///< Effects Filter 1 Type", "effects.filter_1_type"),
        ("EF2TYP", "///< Effects Filter 2 Type", "effects.filter_2_type"),

        ("L1WAV", " ///< LFO 1 Waveform", "lfos_rw[0]->waveform"),
        ("L2WAV", " ///< LFO 2 Waveform", "lfos_rw[1]->waveform"),
        ("L3WAV", " ///< LFO 3 Waveform", "lfos_rw[2]->waveform"),
        ("L4WAV", " ///< LFO 4 Waveform", "lfos_rw[3]->waveform"),
        ("L5WAV", " ///< LFO 5 Waveform", "lfos_rw[4]->waveform"),
        ("L6WAV", " ///< LFO 6 Waveform", "lfos_rw[5]->waveform"),
        ("L7WAV", " ///< LFO 7 Waveform", "lfos_rw[6]->waveform"),
        ("L8WAV", " ///< LFO 8 Waveform", "lfos_rw[7]->waveform"),

        ("L1CEN", " ///< LFO 1 Center", "lfos_rw[0]->center"),
        ("L2CEN", " ///< LFO 2 Center", "lfos_rw[1]->center"),
        ("L3CEN", " ///< LFO 3 Center", "lfos_rw[2]->center"),
        ("L4CEN", " ///< LFO 4 Center", "lfos_rw[3]->center"),
        ("L5CEN", " ///< LFO 5 Center", "lfos_rw[4]->center"),
        ("L6CEN", " ///< LFO 6 Center", "lfos_rw[5]->center"),
        ("L7CEN", " ///< LFO 7 Center", "lfos_rw[6]->center"),
        ("L8CEN", " ///< LFO 8 Center", "lfos_rw[7]->center"),

        ("L1SYN", " ///< LFO 1 Tempo Synchronization", "lfos_rw[0]->tempo_sync"),
        ("L2SYN", " ///< LFO 2 Tempo Synchronization", "lfos_rw[1]->tempo_sync"),
        ("L3SYN", " ///< LFO 3 Tempo Synchronization", "lfos_rw[2]->tempo_sync"),
        ("L4SYN", " ///< LFO 4 Tempo Synchronization", "lfos_rw[3]->tempo_sync"),
        ("L5SYN", " ///< LFO 5 Tempo Synchronization", "lfos_rw[4]->tempo_sync"),
        ("L6SYN", " ///< LFO 6 Tempo Synchronization", "lfos_rw[5]->tempo_sync"),
        ("L7SYN", " ///< LFO 7 Tempo Synchronization", "lfos_rw[6]->tempo_sync"),
        ("L8SYN", " ///< LFO 8 Tempo Synchronization", "lfos_rw[7]->tempo_sync"),

        ("ECSYN", " ///< Effects Chorus Tempo Synchronization", "effects.chorus.tempo_sync"),

        ("EESYN", " ///< Effects Echo Tempo Synchronization", "effects.echo.tempo_sync"),

        ("MF1LOG", "///< Modulator Filter 1 Logarithmic Frequency", "modulator_params.filter_1_freq_log_scale"),
        ("MF2LOG", "///< Modulator Filter 2 Logarithmic Frequency", "modulator_params.filter_2_freq_log_scale"),
        ("CF1LOG", "///< Carrier Filter 1 Logarithmic Frequency", "carrier_params.filter_1_freq_log_scale"),
        ("CF2LOG", "///< Carrier Filter 2 Logarithmic Frequency", "carrier_params.filter_2_freq_log_scale"),
        ("EF1LOG", "///< Effects Filter 1 Logarithmic Frequency", "effects.filter_1_freq_log_scale"),
        ("EF2LOG", "///< Effects Filter 2 Logarithmic Frequency", "effects.filter_2_freq_log_scale"),
        ("ECLOG", " ///< Effects Chorus Logarithmic Filter Frequencies", "effects.chorus.log_scale_frequencies"),
        ("EELOG", " ///< Effects Echo Logarithmic Filter Frequencies", "effects.echo.log_scale_frequencies"),
        ("ERLOG", " ///< Effects Reverb Logarithmic Filter Frequencies", "effects.reverb.log_scale_frequencies"),

        ("N1DYN", " ///< Envelope 1 Dynamic", "envelopes_rw[0]->dynamic"),
        ("N2DYN", " ///< Envelope 2 Dynamic", "envelopes_rw[1]->dynamic"),
        ("N3DYN", " ///< Envelope 3 Dynamic", "envelopes_rw[2]->dynamic"),
        ("N4DYN", " ///< Envelope 4 Dynamic", "envelopes_rw[3]->dynamic"),
        ("N5DYN", " ///< Envelope 5 Dynamic", "envelopes_rw[4]->dynamic"),
        ("N6DYN", " ///< Envelope 6 Dynamic", "envelopes_rw[5]->dynamic"),
        ("N7DYN", " ///< Envelope 7 Dynamic", "envelopes_rw[6]->dynamic"),
        ("N8DYN", " ///< Envelope 8 Dynamic", "envelopes_rw[7]->dynamic"),
        ("N9DYN", " ///< Envelope 9 Dynamic", "envelopes_rw[8]->dynamic"),
        ("N10DYN", " ///< Envelope 10 Dynamic", "envelopes_rw[9]->dynamic"),
        ("N11DYN", " ///< Envelope 11 Dynamic", "envelopes_rw[10]->dynamic"),
        ("N12DYN", " ///< Envelope 12 Dynamic", "envelopes_rw[11]->dynamic"),

        ("POLY", "  ///< Polyphonic", "polyphonic"),

        ("ERTYP", " ///< Effects Reverb Type", "effects.reverb.type"),

        ("ECTYP", " ///< Effects Chorus Type", "effects.chorus.type"),

        ("MTUN", "  ///< Modulator Tuning", "modulator_params.tuning"),
        ("CTUN", "  ///< Carrier Tuning", "carrier_params.tuning"),

        ("MOIA", "  ///< Modulator Oscillator Inaccuracy", "modulator_params.oscillator_inaccuracy"),
        ("MOIS", "  ///< Modulator Oscillator Instability", "modulator_params.oscillator_instability"),

        ("COIA", "  ///< Carrier Oscillator Inaccuracy", "carrier_params.oscillator_inaccuracy"),
        ("COIS", "  ///< Carrier Oscillator Instability", "carrier_params.oscillator_instability"),

        ("MF1QLG", "///< Modulator Filter 1 Logarithmic Q Factor", "modulator_params.filter_1_q_log_scale"),
        ("MF2QLG", "///< Modulator Filter 2 Logarithmic Q Factor", "modulator_params.filter_2_q_log_scale"),
        ("CF1QLG", "///< Carrier Filter 1 Logarithmic Q Factor", "carrier_params.filter_1_q_log_scale"),
        ("CF2QLG", "///< Carrier Filter 2 Logarithmic Q Factor", "carrier_params.filter_2_q_log_scale"),
        ("EF1QLG", "///< Effects Filter 1 Logarithmic Q Factor", "effects.filter_1_q_log_scale"),
        ("EF2QLG", "///< Effects Filter 2 Logarithmic Q Factor", "effects.filter_2_q_log_scale"),

        ("L1AEN", " ///< LFO 1 Amount Envelope", "lfos_rw[0]->amount_envelope"),
        ("L2AEN", " ///< LFO 2 Amount Envelope", "lfos_rw[1]->amount_envelope"),
        ("L3AEN", " ///< LFO 3 Amount Envelope", "lfos_rw[2]->amount_envelope"),
        ("L4AEN", " ///< LFO 4 Amount Envelope", "lfos_rw[3]->amount_envelope"),
        ("L5AEN", " ///< LFO 5 Amount Envelope", "lfos_rw[4]->amount_envelope"),
        ("L6AEN", " ///< LFO 6 Amount Envelope", "lfos_rw[5]->amount_envelope"),
        ("L7AEN", " ///< LFO 7 Amount Envelope", "lfos_rw[6]->amount_envelope"),
        ("L8AEN", " ///< LFO 8 Amount Envelope", "lfos_rw[7]->amount_envelope"),
    ]

    return print_params(param_id, param_objs, "", "", 1, params)


def print_params(
        param_id: int,
        param_objs: list,
        group: str,
        prefix: str,
        objects: int,
        object_params: list
) -> int:
    for i in range(objects):
        obj_id = str(i + 1)

        for name, comment, param_obj in object_params:
            comment = comment.replace("#", obj_id).replace("$", group)
            name = name.replace("#", obj_id).replace("$", prefix)
            spaces = "   "

            if param_obj != "":
                param_objs.append((name, param_obj))

            if param_id < 100:
                spaces += " "

            if param_id < 10:
                spaces += " "

            print(f"{INDENTATION}{name} = {param_id}, {spaces}{comment}")
            param_id += 1

        print("")

    return param_id


def print_cases(param_objs, template):
    print("")
    print("/******************************************************************************/")
    print("")

    for name, obj in param_objs:
        print(f"        case ParamId::{name}: " + template.replace("$", obj))


if __name__ == "__main__":
    sys.exit(main(sys.argv))
