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

        ("$CDEL", " ///< $ Chorus Delay Time", "effects.chorus.delay_time"),
        ("$CFRQ", " ///< $ Chorus LFO Frequency", "effects.chorus.frequency"),
        ("$CDPT", " ///< $ Chorus Depth", "effects.chorus.depth"),
        ("$CFB", "  ///< $ Chorus Feedback", "effects.chorus.feedback"),
        ("$CDF", "  ///< $ Chorus Dampening Frequency", "effects.chorus.damping_frequency"),
        ("$CDG", "  ///< $ Chorus Dampening Gain", "effects.chorus.damping_gain"),
        ("$CWID", " ///< $ Chorus Stereo Width", "effects.chorus.width"),
        ("$CHPF", " ///< $ Chorus High-pass Frequency", "effects.chorus.high_pass_frequency"),
        ("$CHPQ", " ///< $ Chorus High-pass Q Factor", "effects.chorus.high_pass_q"),
        ("$CWET", " ///< $ Chorus Wet Volume", "effects.chorus.wet"),
        ("$CDRY", " ///< $ Chorus Dry Volume", "effects.chorus.dry"),

        ("$EDEL", " ///< $ Echo Delay Time", "effects.echo.delay_time"),
        ("$EINV", " ///< $ Echo Input Volume", "effects.echo.input_volume"),
        ("$EFB", "  ///< $ Echo Feedback", "effects.echo.feedback"),
        ("$EDST", " ///< $ Echo Distortion", "effects.echo.distortion_level"),
        ("$EDF", "  ///< $ Echo Dampening Frequency", "effects.echo.damping_frequency"),
        ("$EDG", "  ///< $ Echo Dampening Gain", "effects.echo.damping_gain"),
        ("$EWID", " ///< $ Echo Stereo Width", "effects.echo.width"),
        ("$EHPF", " ///< $ Echo High-pass Frequency", "effects.echo.high_pass_frequency"),
        ("$EHPQ", " ///< $ Echo High-pass Q Factor", "effects.echo.high_pass_q"),
        ("$ECTH", " ///< $ Echo Side-Chain Compression Threshold", "effects.echo.side_chain_compression_threshold"),
        ("$ECAT", " ///< $ Echo Side-Chain Compression Attack Time", "effects.echo.side_chain_compression_attack_time"),
        ("$ECRL", " ///< $ Echo Side-Chain Compression Release Time", "effects.echo.side_chain_compression_release_time"),
        ("$ECR", "  ///< $ Echo Side-Chain Compression Ratio", "effects.echo.side_chain_compression_ratio"),
        ("$EWET", " ///< $ Echo Wet Volume", "effects.echo.wet"),
        ("$EDRY", " ///< $ Echo Dry Volume", "effects.echo.dry"),

        ("$RRR", "  ///< $ Reverb Room Reflectivity", "effects.reverb.room_reflectivity"),
        ("$RDST", " ///< $ Reverb Distortion", "effects.reverb.distortion_level"),
        ("$RDF", "  ///< $ Reverb Dampening Frequency", "effects.reverb.damping_frequency"),
        ("$RDG", "  ///< $ Reverb Dampening Gain", "effects.reverb.damping_gain"),
        ("$RWID", " ///< $ Reverb Stereo Width", "effects.reverb.width"),
        ("$RHPF", " ///< $ Reverb High-pass Frequency", "effects.reverb.high_pass_frequency"),
        ("$RHPQ", " ///< $ Reverb High-pass Q Factor", "effects.reverb.high_pass_q"),
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
        ("$#MID", " ///< $ # Midpoint", ""),
        ("$#IN", "  ///< $ # Input", ""),
        ("$#MIN", " ///< $ # Minimum Value", ""),
        ("$#MAX", " ///< $ # Maximum Value", ""),
        ("$#AMT", " ///< $ # Amount", ""),
        ("$#DST", " ///< $ # Distortion", ""),
        ("$#RND", " ///< $ # Randomness", ""),
    ]

    return print_params(param_id, param_objs, "Macro", "M", 30, params)


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

        ("L1LOG", " ///< LFO 1 Logarithmic Frequency", "lfos_rw[0]->freq_log_scale"),
        ("L2LOG", " ///< LFO 2 Logarithmic Frequency", "lfos_rw[1]->freq_log_scale"),
        ("L3LOG", " ///< LFO 3 Logarithmic Frequency", "lfos_rw[2]->freq_log_scale"),
        ("L4LOG", " ///< LFO 4 Logarithmic Frequency", "lfos_rw[3]->freq_log_scale"),
        ("L5LOG", " ///< LFO 5 Logarithmic Frequency", "lfos_rw[4]->freq_log_scale"),
        ("L6LOG", " ///< LFO 6 Logarithmic Frequency", "lfos_rw[5]->freq_log_scale"),
        ("L7LOG", " ///< LFO 7 Logarithmic Frequency", "lfos_rw[6]->freq_log_scale"),
        ("L8LOG", " ///< LFO 8 Logarithmic Frequency", "lfos_rw[7]->freq_log_scale"),

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
        ("ECLOG", " ///< Effects Chorus Logarithmic Filter Frequencies", "effects.chorus.log_scale_filter_frequencies"),
        ("ECLHQ", " ///< Effects Chorus Logarithmic High-pass Filter Q Factor", "effects.chorus.log_scale_high_pass_q"),
        ("ECLLG", " ///< Effects Chorus Logarithmic LFO Frequency", "effects.chorus.log_scale_lfo_frequency"),
        ("EELOG", " ///< Effects Echo Logarithmic Filter Frequencies", "effects.echo.log_scale_frequencies"),
        ("EELHQ", " ///< Effects Echo Logarithmic High-pass Filter Q Factor", "effects.echo.log_scale_high_pass_q"),
        ("ERLOG", " ///< Effects Reverb Logarithmic Filter Frequencies", "effects.reverb.log_scale_frequencies"),
        ("ERLHQ", " ///< Effects Reverb Logarithmic High-pass Filter Q Factor", "effects.reverb.log_scale_high_pass_q"),

        ("N1UPD", " ///< Envelope 1 Update Mode", "envelopes_rw[0]->update_mode"),
        ("N2UPD", " ///< Envelope 2 Update Mode", "envelopes_rw[1]->update_mode"),
        ("N3UPD", " ///< Envelope 3 Update Mode", "envelopes_rw[2]->update_mode"),
        ("N4UPD", " ///< Envelope 4 Update Mode", "envelopes_rw[3]->update_mode"),
        ("N5UPD", " ///< Envelope 5 Update Mode", "envelopes_rw[4]->update_mode"),
        ("N6UPD", " ///< Envelope 6 Update Mode", "envelopes_rw[5]->update_mode"),
        ("N7UPD", " ///< Envelope 7 Update Mode", "envelopes_rw[6]->update_mode"),
        ("N8UPD", " ///< Envelope 8 Update Mode", "envelopes_rw[7]->update_mode"),
        ("N9UPD", " ///< Envelope 9 Update Mode", "envelopes_rw[8]->update_mode"),
        ("N10UPD", "///< Envelope 10 Update Mode", "envelopes_rw[9]->update_mode"),
        ("N11UPD", "///< Envelope 11 Update Mode", "envelopes_rw[10]->update_mode"),
        ("N12UPD", "///< Envelope 12 Update Mode", "envelopes_rw[11]->update_mode"),

        ("NH", "    ///< Note Handling", "note_handling"),

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

        ("N1SYN", " ///< Envelope 1 Tempo Synchronization", "envelopes_rw[0]->tempo_sync"),
        ("N2SYN", " ///< Envelope 2 Tempo Synchronization", "envelopes_rw[1]->tempo_sync"),
        ("N3SYN", " ///< Envelope 3 Tempo Synchronization", "envelopes_rw[2]->tempo_sync"),
        ("N4SYN", " ///< Envelope 4 Tempo Synchronization", "envelopes_rw[3]->tempo_sync"),
        ("N5SYN", " ///< Envelope 5 Tempo Synchronization", "envelopes_rw[4]->tempo_sync"),
        ("N6SYN", " ///< Envelope 6 Tempo Synchronization", "envelopes_rw[5]->tempo_sync"),
        ("N7SYN", " ///< Envelope 7 Tempo Synchronization", "envelopes_rw[6]->tempo_sync"),
        ("N8SYN", " ///< Envelope 8 Tempo Synchronization", "envelopes_rw[7]->tempo_sync"),
        ("N9SYN", " ///< Envelope 9 Tempo Synchronization", "envelopes_rw[8]->tempo_sync"),
        ("N10SYN", "///< Envelope 10 Tempo Synchronization", "envelopes_rw[9]->tempo_sync"),
        ("N11SYN", "///< Envelope 11 Tempo Synchronization", "envelopes_rw[10]->tempo_sync"),
        ("N12SYN", "///< Envelope 12 Tempo Synchronization", "envelopes_rw[11]->tempo_sync"),

        ("N1ASH", " ///< Envelope 1 Attack Shape", "envelopes_rw[0]->attack_shape"),
        ("N2ASH", " ///< Envelope 2 Attack Shape", "envelopes_rw[1]->attack_shape"),
        ("N3ASH", " ///< Envelope 3 Attack Shape", "envelopes_rw[2]->attack_shape"),
        ("N4ASH", " ///< Envelope 4 Attack Shape", "envelopes_rw[3]->attack_shape"),
        ("N5ASH", " ///< Envelope 5 Attack Shape", "envelopes_rw[4]->attack_shape"),
        ("N6ASH", " ///< Envelope 6 Attack Shape", "envelopes_rw[5]->attack_shape"),
        ("N7ASH", " ///< Envelope 7 Attack Shape", "envelopes_rw[6]->attack_shape"),
        ("N8ASH", " ///< Envelope 8 Attack Shape", "envelopes_rw[7]->attack_shape"),
        ("N9ASH", " ///< Envelope 9 Attack Shape", "envelopes_rw[8]->attack_shape"),
        ("N10ASH", "///< Envelope 10 Attack Shape", "envelopes_rw[9]->attack_shape"),
        ("N11ASH", "///< Envelope 11 Attack Shape", "envelopes_rw[10]->attack_shape"),
        ("N12ASH", "///< Envelope 12 Attack Shape", "envelopes_rw[11]->attack_shape"),

        ("N1DSH", " ///< Envelope 1 Decay Shape", "envelopes_rw[0]->decay_shape"),
        ("N2DSH", " ///< Envelope 2 Decay Shape", "envelopes_rw[1]->decay_shape"),
        ("N3DSH", " ///< Envelope 3 Decay Shape", "envelopes_rw[2]->decay_shape"),
        ("N4DSH", " ///< Envelope 4 Decay Shape", "envelopes_rw[3]->decay_shape"),
        ("N5DSH", " ///< Envelope 5 Decay Shape", "envelopes_rw[4]->decay_shape"),
        ("N6DSH", " ///< Envelope 6 Decay Shape", "envelopes_rw[5]->decay_shape"),
        ("N7DSH", " ///< Envelope 7 Decay Shape", "envelopes_rw[6]->decay_shape"),
        ("N8DSH", " ///< Envelope 8 Decay Shape", "envelopes_rw[7]->decay_shape"),
        ("N9DSH", " ///< Envelope 9 Decay Shape", "envelopes_rw[8]->decay_shape"),
        ("N10DSH", "///< Envelope 10 Decay Shape", "envelopes_rw[9]->decay_shape"),
        ("N11DSH", "///< Envelope 11 Decay Shape", "envelopes_rw[10]->decay_shape"),
        ("N12DSH", "///< Envelope 12 Decay Shape", "envelopes_rw[11]->decay_shape"),

        ("N1RSH", " ///< Envelope 1 Release Shape", "envelopes_rw[0]->release_shape"),
        ("N2RSH", " ///< Envelope 2 Release Shape", "envelopes_rw[1]->release_shape"),
        ("N3RSH", " ///< Envelope 3 Release Shape", "envelopes_rw[2]->release_shape"),
        ("N4RSH", " ///< Envelope 4 Release Shape", "envelopes_rw[3]->release_shape"),
        ("N5RSH", " ///< Envelope 5 Release Shape", "envelopes_rw[4]->release_shape"),
        ("N6RSH", " ///< Envelope 6 Release Shape", "envelopes_rw[5]->release_shape"),
        ("N7RSH", " ///< Envelope 7 Release Shape", "envelopes_rw[6]->release_shape"),
        ("N8RSH", " ///< Envelope 8 Release Shape", "envelopes_rw[7]->release_shape"),
        ("N9RSH", " ///< Envelope 9 Release Shape", "envelopes_rw[8]->release_shape"),
        ("N10RSH", "///< Envelope 10 Release Shape", "envelopes_rw[9]->release_shape"),
        ("N11RSH", "///< Envelope 11 Release Shape", "envelopes_rw[10]->release_shape"),
        ("N12RSH", "///< Envelope 12 Release Shape", "envelopes_rw[11]->release_shape"),

        ("M1DSH", " ///< Macro 1 Distortion Shape", "macros_rw[0]->distortion_shape"),
        ("M2DSH", " ///< Macro 2 Distortion Shape", "macros_rw[1]->distortion_shape"),
        ("M3DSH", " ///< Macro 3 Distortion Shape", "macros_rw[2]->distortion_shape"),
        ("M4DSH", " ///< Macro 4 Distortion Shape", "macros_rw[3]->distortion_shape"),
        ("M5DSH", " ///< Macro 5 Distortion Shape", "macros_rw[4]->distortion_shape"),
        ("M6DSH", " ///< Macro 6 Distortion Shape", "macros_rw[5]->distortion_shape"),
        ("M7DSH", " ///< Macro 7 Distortion Shape", "macros_rw[6]->distortion_shape"),
        ("M8DSH", " ///< Macro 8 Distortion Shape", "macros_rw[7]->distortion_shape"),
        ("M9DSH", " ///< Macro 9 Distortion Shape", "macros_rw[8]->distortion_shape"),
        ("M10DSH", "///< Macro 10 Distortion Shape", "macros_rw[9]->distortion_shape"),
        ("M11DSH", "///< Macro 11 Distortion Shape", "macros_rw[10]->distortion_shape"),
        ("M12DSH", "///< Macro 12 Distortion Shape", "macros_rw[11]->distortion_shape"),
        ("M13DSH", "///< Macro 13 Distortion Shape", "macros_rw[12]->distortion_shape"),
        ("M14DSH", "///< Macro 14 Distortion Shape", "macros_rw[13]->distortion_shape"),
        ("M15DSH", "///< Macro 15 Distortion Shape", "macros_rw[14]->distortion_shape"),
        ("M16DSH", "///< Macro 16 Distortion Shape", "macros_rw[15]->distortion_shape"),
        ("M17DSH", "///< Macro 17 Distortion Shape", "macros_rw[16]->distortion_shape"),
        ("M18DSH", "///< Macro 18 Distortion Shape", "macros_rw[17]->distortion_shape"),
        ("M19DSH", "///< Macro 19 Distortion Shape", "macros_rw[18]->distortion_shape"),
        ("M20DSH", "///< Macro 20 Distortion Shape", "macros_rw[19]->distortion_shape"),
        ("M21DSH", "///< Macro 21 Distortion Shape", "macros_rw[20]->distortion_shape"),
        ("M22DSH", "///< Macro 22 Distortion Shape", "macros_rw[21]->distortion_shape"),
        ("M23DSH", "///< Macro 23 Distortion Shape", "macros_rw[22]->distortion_shape"),
        ("M24DSH", "///< Macro 24 Distortion Shape", "macros_rw[23]->distortion_shape"),
        ("M25DSH", "///< Macro 25 Distortion Shape", "macros_rw[24]->distortion_shape"),
        ("M26DSH", "///< Macro 26 Distortion Shape", "macros_rw[25]->distortion_shape"),
        ("M27DSH", "///< Macro 27 Distortion Shape", "macros_rw[26]->distortion_shape"),
        ("M28DSH", "///< Macro 28 Distortion Shape", "macros_rw[27]->distortion_shape"),
        ("M29DSH", "///< Macro 29 Distortion Shape", "macros_rw[28]->distortion_shape"),
        ("M30DSH", "///< Macro 30 Distortion Shape", "macros_rw[29]->distortion_shape"),

        ("MFX4", "  ///< Modulator Fine Detune x4", "modulator_params.fine_detune_x4"),
        ("CFX4", "  ///< Carrier Fine Detune x4", "carrier_params.fine_detune_x4"),
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


if __name__ == "__main__":
    sys.exit(main(sys.argv))
