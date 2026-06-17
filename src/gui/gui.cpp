/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JS80P__GUI__GUI_CPP
#define JS80P__GUI__GUI_CPP

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "gui/gui.hpp"


namespace JS80P
{

void GUI::EventHandler::handle_resize_request(
        int const new_width,
        int const new_height
) {
}


bool GUI::controllers_by_id_initialized = false;


char const* const GUI::MODES[] = {
    [Synth::MODE_MIX_AND_MOD] = "Mix&Mod",
    [Synth::MODE_SPLIT_AT_C3] = "Split C3",
    [Synth::MODE_SPLIT_AT_Db3] = "Split Db3",
    [Synth::MODE_SPLIT_AT_D3] = "Split D3",
    [Synth::MODE_SPLIT_AT_Eb3] = "Split Eb3",
    [Synth::MODE_SPLIT_AT_E3] = "Split E3",
    [Synth::MODE_SPLIT_AT_F3] = "Split F3",
    [Synth::MODE_SPLIT_AT_Gb3] = "Split Gb3",
    [Synth::MODE_SPLIT_AT_G3] = "Split G3",
    [Synth::MODE_SPLIT_AT_Ab3] = "Split Ab3",
    [Synth::MODE_SPLIT_AT_A3] = "Split A3",
    [Synth::MODE_SPLIT_AT_Bb3] = "Split Bb3",
    [Synth::MODE_SPLIT_AT_B3] = "Split B3",
    [Synth::MODE_SPLIT_AT_C4] = "Split C4",
};

int const GUI::MODES_COUNT = Synth::MODES;


char const* const GUI::TUNINGS[] = {
    [Modulator::TUNING_440HZ_12TET] = "440 12TET",
    [Modulator::TUNING_432HZ_12TET] = "432 12TET",
    [Modulator::TUNING_MTS_ESP_CONTINUOUS] = "C MTS-ESP",
    [Modulator::TUNING_MTS_ESP_NOTE_ON] = "N MTS-ESP",
};

int const GUI::TUNINGS_COUNT = VOICE_TUNINGS;


char const* const GUI::OSCILLATOR_INACCURACY_LEVELS[] = {
    "0.00%",
    "0.01%",
    "0.05%",
    "0.12%",
    "0.23%",
    "0.37%",
    "0.56%",
    "0.80%",
    "1.07%",
    "1.40%",
    "1.77%",
    "2.20%",
    "2.67%",
    "3.20%",
    "3.78%",
    "4.42%",
    "5.11%",
    "5.86%",
    "6.66%",
    "7.52%",
    "8.44%",
    "9.42%",
    "10.46%",
    "11.56%",
    "12.72%",
    "13.95%",
    "15.24%",
    "16.59%",
    "18.00%",
    "19.48%",
    "21.02%",
    "22.63%",
    "24.31%",
    "26.05%",
    "27.86%",
    "29.74%",
    "31.68%",
    "33.70%",
    "35.78%",
    "37.94%",
    "40.16%",
    "42.45%",
    "44.82%",
    "47.26%",
    "49.77%",
    "52.35%",
    "55.00%",
    "57.73%",
    "60.53%",
    "63.40%",
    "66.35%",
    "69.37%",
    "72.47%",
    "75.65%",
    "78.89%",
    "82.22%",
    "85.62%",
    "89.10%",
    "92.66%",
    "96.29%",
    "100.00%",
};

int const GUI::OSCILLATOR_INACCURACY_LEVELS_COUNT = (
    OscillatorInaccuracy::MAX_LEVEL + 1
);


char const* const GUI::WAVEFORMS[] = {
    [SimpleOscillator::SINE] = "Sine",
    [SimpleOscillator::SAWTOOTH] = "Saw",
    [SimpleOscillator::SOFT_SAWTOOTH] = "Soft Sw",
    [SimpleOscillator::INVERSE_SAWTOOTH] = "Inv Saw",
    [SimpleOscillator::SOFT_INVERSE_SAWTOOTH] = "Soft I S",
    [SimpleOscillator::TRIANGLE] = "Triangle",
    [SimpleOscillator::SOFT_TRIANGLE] = "Soft Tri",
    [SimpleOscillator::SQUARE] = "Square",
    [SimpleOscillator::SOFT_SQUARE] = "Soft Sqr",
    [SimpleOscillator::PULSE] = "Pulse",
    [SimpleOscillator::SOFT_PULSE] = "Soft Pls",
    [SimpleOscillator::BIPOLAR_PULSE] = "BiPoPu",
    [SimpleOscillator::SOFT_BIPOLAR_PULSE] = "Soft BPP",
    [SimpleOscillator::CUSTOM] = "Custom",
};

int const GUI::WAVEFORMS_COUNT = SimpleOscillator::WAVEFORMS;


char const* const GUI::BIQUAD_FILTER_TYPES[] = {
    [SimpleBiquadFilter::LOW_PASS] = "LP",
    [SimpleBiquadFilter::HIGH_PASS] = "HP",
    [SimpleBiquadFilter::BAND_PASS] = "BP",
    [SimpleBiquadFilter::NOTCH] = "Notch",
    [SimpleBiquadFilter::PEAKING] = "Bell",
    [SimpleBiquadFilter::LOW_SHELF] = "LS",
    [SimpleBiquadFilter::HIGH_SHELF] = "HS",
};

int const GUI::BIQUAD_FILTER_TYPES_COUNT = 7;


char const* const GUI::CHORUS_TYPES[] = {
    [Chorus<SignalProducer>::CHORUS_1] = "1",
    [Chorus<SignalProducer>::CHORUS_2] = "2",
    [Chorus<SignalProducer>::CHORUS_3] = "3",
    [Chorus<SignalProducer>::CHORUS_4] = "4",
    [Chorus<SignalProducer>::CHORUS_5] = "5",
    [Chorus<SignalProducer>::CHORUS_6] = "6",
    [Chorus<SignalProducer>::CHORUS_7] = "7",
    [Chorus<SignalProducer>::CHORUS_8] = "8",
    [Chorus<SignalProducer>::CHORUS_9] = "9",
    [Chorus<SignalProducer>::CHORUS_10] = "10",
    [Chorus<SignalProducer>::CHORUS_11] = "11",
    [Chorus<SignalProducer>::CHORUS_12] = "12",
    [Chorus<SignalProducer>::CHORUS_13] = "13",
    [Chorus<SignalProducer>::CHORUS_14] = "14",
    [Chorus<SignalProducer>::CHORUS_15] = "15",
};

int const GUI::CHORUS_TYPES_COUNT = 15;


char const* const GUI::REVERB_TYPES[] = {
    [Reverb<SignalProducer>::REVERB_1] = "1",
    [Reverb<SignalProducer>::REVERB_2] = "2",
    [Reverb<SignalProducer>::REVERB_3] = "3",
    [Reverb<SignalProducer>::REVERB_4] = "4",
    [Reverb<SignalProducer>::REVERB_5] = "5",
    [Reverb<SignalProducer>::REVERB_6] = "6",
    [Reverb<SignalProducer>::REVERB_7] = "7",
    [Reverb<SignalProducer>::REVERB_8] = "8",
    [Reverb<SignalProducer>::REVERB_9] = "9",
    [Reverb<SignalProducer>::REVERB_10] = "10",
};

int const GUI::REVERB_TYPES_COUNT = 10;


char const* const GUI::LFO_AMPLITUDE_ENVELOPES[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "",
};

int const GUI::LFO_AMPLITUDE_ENVELOPES_COUNT = 13;


char const* const GUI::ENVELOPE_UPDATE_TYPES[] = {
    [Envelope::UPDATE_MODE_DYNAMIC_LAST] = "LST",
    [Envelope::UPDATE_MODE_DYNAMIC_OLDEST] = "OLD",
    [Envelope::UPDATE_MODE_DYNAMIC_LOWEST] = "LOW",
    [Envelope::UPDATE_MODE_DYNAMIC_HIGHEST] = "HI",
    [Envelope::UPDATE_MODE_STATIC] = "STA",
    [Envelope::UPDATE_MODE_END] = "END",
    [Envelope::UPDATE_MODE_DYNAMIC] = "DYN",
};

int const GUI::ENVELOPE_UPDATE_TYPES_COUNT = 7;


char const* const GUI::NOTE_HANDLING_MODES[] = {
    [Synth::NOTE_HANDLING_MONO] = "MONO",
    [Synth::NOTE_HANDLING_MONO_HOLD] = "M HOLD",
    [Synth::NOTE_HANDLING_MONO_IGSUS] = "M IS",
    [Synth::NOTE_HANDLING_MONO_HOLD_IGSUS] = "M H IS",
    [Synth::NOTE_HANDLING_POLY] = "POLY",
    [Synth::NOTE_HANDLING_POLY_HOLD] = "P HOLD",
    [Synth::NOTE_HANDLING_POLY_IGSUS] = "P IS",
    [Synth::NOTE_HANDLING_POLY_HOLD_IGSUS] = "P H IS",
    [Synth::NOTE_HANDLING_POLY_RETRIGGER] = "P RET",
    [Synth::NOTE_HANDLING_POLY_RETRIGGER_HOLD] = "P H RET",
    [Synth::NOTE_HANDLING_POLY_RETRIGGER_HOLD_IGSUS] = "P H R IS",
};

int const GUI::NOTE_HANDLING_MODES_COUNT = 11;


char const* const GUI::DISTORTION_TYPES[] = {
    [Distortion::TYPE_TANH_3] = "tanh 3x",
    [Distortion::TYPE_TANH_5] = "tanh 5x",
    [Distortion::TYPE_TANH_10] = "tanh 10x",
    [Distortion::TYPE_SIN] = "sin x",
    [Distortion::TYPE_SQR] = "x^2",
    [Distortion::TYPE_SQRT] = "sqrt x",
    [Distortion::TYPE_CUBE] = "x^3",
    [Distortion::TYPE_CBRT] = "cbrt x",
    [Distortion::TYPE_HARMONIC_13] = "1+3",
    [Distortion::TYPE_HARMONIC_15] = "1+5",
    [Distortion::TYPE_HARMONIC_135] = "1+3+5",
    [Distortion::TYPE_HARMONIC_SQR] = "square",
    [Distortion::TYPE_HARMONIC_TRI] = "triangle",
    [Distortion::TYPE_BIT_CRUSH_1] = "bit 1",
    [Distortion::TYPE_BIT_CRUSH_2] = "bit 2",
    [Distortion::TYPE_BIT_CRUSH_3] = "bit 3",
    [Distortion::TYPE_BIT_CRUSH_4] = "bit 4",
    [Distortion::TYPE_BIT_CRUSH_4_6] = "bit 4.6",
    [Distortion::TYPE_BIT_CRUSH_5] = "bit 5",
    [Distortion::TYPE_BIT_CRUSH_5_6] = "bit 5.6",
    [Distortion::TYPE_BIT_CRUSH_6] = "bit 6",
    [Distortion::TYPE_BIT_CRUSH_6_6] = "bit 6.6",
    [Distortion::TYPE_BIT_CRUSH_7] = "bit 7",
    [Distortion::TYPE_BIT_CRUSH_7_6] = "bit 7.6",
    [Distortion::TYPE_BIT_CRUSH_8] = "bit 8",
    [Distortion::TYPE_BIT_CRUSH_8_6] = "bit 8.6",
    [Distortion::TYPE_BIT_CRUSH_9] = "bit 9",
    [Distortion::TYPE_DELAY_FEEDBACK  ] = "reduce",
};

int const GUI::DISTORTION_TYPES_COUNT = Distortion::TYPES;


char const* const GUI::TAPE_STATES[] = {
    [TapeParams::State::TAPE_STATE_INIT] = "waiting for Stop / Start zero",
    [TapeParams::State::TAPE_STATE_NORMAL] = NULL,
    [TapeParams::State::TAPE_STATE_STOPPING] = "stopping",
    [TapeParams::State::TAPE_STATE_STOPPED] = "STOPPED",
    [TapeParams::State::TAPE_STATE_STARTABLE] = "STOPPED",
    [TapeParams::State::TAPE_STATE_STARTING] = "starting",
    [TapeParams::State::TAPE_STATE_STARTED] = "waiting for Stop / Start zero",
    [TapeParams::State::TAPE_STATE_FF_STARTABLE] = "STOPPED",
    [TapeParams::State::TAPE_STATE_FF_STARTING] = "fast-forwarding",
    [TapeParams::State::TAPE_STATE_FF_STARTED] =
        "waiting for Stop / Start zero",
};

int const GUI::TAPE_STATES_COUNT = TapeParams::State::TAPE_STATES;


char const* const GUI::COMPRESSION_MODES[] = {
    [CompressionMode::COMPRESSION_MODE_COMPRESSOR] = "COMP",
    [CompressionMode::COMPRESSION_MODE_EXPANDER] = "EXPD",
};

int const GUI::COMPRESSION_MODES_COUNT = CompressionMode::COMPRESSION_MODES;


char const* const GUI::MPE_SETTINGS[] = {
    [Synth::MPE_OFF] = "OFF",
    [Synth::MPE_L15] = "Lo 15",
    [Synth::MPE_L14] = "Lo 14",
    [Synth::MPE_L13] = "Lo 13",
    [Synth::MPE_L12] = "Lo 12",
    [Synth::MPE_L11] = "Lo 11",
    [Synth::MPE_L10] = "Lo 10",
    [Synth::MPE_L09] = "Lo 9",
    [Synth::MPE_L08] = "Lo 8",
    [Synth::MPE_L07] = "Lo 7",
    [Synth::MPE_L06] = "Lo 6",
    [Synth::MPE_L05] = "Lo 5",
    [Synth::MPE_L04] = "Lo 4",
    [Synth::MPE_L03] = "Lo 3",
    [Synth::MPE_L02] = "Lo 2",
    [Synth::MPE_L01] = "Lo 1",
    [Synth::MPE_U15] = "Up 15",
    [Synth::MPE_U14] = "Up 14",
    [Synth::MPE_U13] = "Up 13",
    [Synth::MPE_U12] = "Up 12",
    [Synth::MPE_U11] = "Up 11",
    [Synth::MPE_U10] = "Up 10",
    [Synth::MPE_U09] = "Up 9",
    [Synth::MPE_U08] = "Up 8",
    [Synth::MPE_U07] = "Up 7",
    [Synth::MPE_U06] = "Up 6",
    [Synth::MPE_U05] = "Up 5",
    [Synth::MPE_U04] = "Up 4",
    [Synth::MPE_U03] = "Up 3",
    [Synth::MPE_U02] = "Up 2",
    [Synth::MPE_U01] = "Up 1",
};

int const GUI::MPE_SETTINGS_COUNT = 31;


GUI::Controller::Controller(
        int const index,
        ControllerCapability const required_capability,
        Synth::ControllerId const id,
        char const* const long_name,
        char const* const name_8,
        char const* const name_5
) : long_name(long_name),
    name_8(name_8),
    name_5(name_5),
    required_capability(required_capability),
    index(index),
    id(id)
{
}


char const* const GUI::PARAMS[Synth::ParamId::PARAM_ID_COUNT] = {
    [Synth::ParamId::MIX] = "Modulator Additive Volume (%)",
    [Synth::ParamId::PM] = "Phase Modulation (%)",
    [Synth::ParamId::FM] = "Frequency Modulation (%)",
    [Synth::ParamId::AM] = "Amplitude Modulation (%)",
    [Synth::ParamId::INVOL] = "Auxiliary Input Volume (%)",

    [Synth::ParamId::MN] = "Modulator Noise Level (%)",

    [Synth::ParamId::MPW] = "Modulator Pulse Width (%)",
    [Synth::ParamId::MAMP] = "Modulator Amplitude (%)",
    [Synth::ParamId::MVS] = "Modulator Velocity Sensitivity (%)",
    [Synth::ParamId::MFLD] = "Modulator Folding (%)",
    [Synth::ParamId::MPRT] = "Modulator Portamento Length (s)",
    [Synth::ParamId::MPRD] = "Modulator Portamento Depth (cents)",
    [Synth::ParamId::MDTN] = "Modulator Detune (semitones)",
    [Synth::ParamId::MFIN] = "Modulator Fine Detune (cents)",
    [Synth::ParamId::MWID] = "Modulator Width (%)",
    [Synth::ParamId::MPAN] = "Modulator Pan (%)",
    [Synth::ParamId::MVOL] = "Modulator Volume (%)",
    [Synth::ParamId::MSUB] = "Modulator Subharmonic Amplitude (%)",

    [Synth::ParamId::MC1] = "Modulator Custom Waveform 1st Harmonic (%)",
    [Synth::ParamId::MC2] = "Modulator Custom Waveform 2nd Harmonic (%)",
    [Synth::ParamId::MC3] = "Modulator Custom Waveform 3rd Harmonic (%)",
    [Synth::ParamId::MC4] = "Modulator Custom Waveform 4th Harmonic (%)",
    [Synth::ParamId::MC5] = "Modulator Custom Waveform 5th Harmonic (%)",
    [Synth::ParamId::MC6] = "Modulator Custom Waveform 6th Harmonic (%)",
    [Synth::ParamId::MC7] = "Modulator Custom Waveform 7th Harmonic (%)",
    [Synth::ParamId::MC8] = "Modulator Custom Waveform 8th Harmonic (%)",
    [Synth::ParamId::MC9] = "Modulator Custom Waveform 9th Harmonic (%)",
    [Synth::ParamId::MC10] = "Modulator Custom Waveform 10th Harmonic (%)",

    [Synth::ParamId::MF1FRQ] = "Modulator Filter 1 Frequency (Hz)",
    [Synth::ParamId::MF1Q] = "Modulator Filter 1 Q Factor",
    [Synth::ParamId::MF1G] = "Modulator Filter 1 Gain (dB)",
    [Synth::ParamId::MF1FIA] = "Modulator Filter 1 Freq. Inaccuracy",
    [Synth::ParamId::MF1QIA] = "Modulator Filter 1 Q Inaccuracy",

    [Synth::ParamId::MF2FRQ] = "Modulator Filter 2 Frequency (Hz)",
    [Synth::ParamId::MF2Q] = "Modulator Filter 2 Q Factor",
    [Synth::ParamId::MF2G] = "Modulator Filter 2 Gain (dB)",
    [Synth::ParamId::MF2FIA] = "Modulator Filter 2 Freq. Inaccuracy",
    [Synth::ParamId::MF2QIA] = "Modulator Filter 2 Q Inaccuracy",

    [Synth::ParamId::CN] = "Carrier Noise Level (%)",

    [Synth::ParamId::CPW] = "Carrier Pulse Width (%)",
    [Synth::ParamId::CAMP] = "Carrier Amplitude (%)",
    [Synth::ParamId::CVS] = "Carrier Velocity Sensitivity (%)",
    [Synth::ParamId::CFLD] = "Carrier Folding (%)",
    [Synth::ParamId::CPRT] = "Carrier Portamento Length (s)",
    [Synth::ParamId::CPRD] = "Carrier Portamento Depth (cents)",
    [Synth::ParamId::CDTN] = "Carrier Detune (semitones)",
    [Synth::ParamId::CFIN] = "Carrier Fine Detune (cents)",
    [Synth::ParamId::CWID] = "Carrier Width (%)",
    [Synth::ParamId::CPAN] = "Carrier Pan (%)",
    [Synth::ParamId::CVOL] = "Carrier Volume (%)",
    [Synth::ParamId::CDL] = "Carrier Distortion Level (%)",

    [Synth::ParamId::CC1] = "Carrier Custom Waveform 1st Harmonic (%)",
    [Synth::ParamId::CC2] = "Carrier Custom Waveform 2nd Harmonic (%)",
    [Synth::ParamId::CC3] = "Carrier Custom Waveform 3rd Harmonic (%)",
    [Synth::ParamId::CC4] = "Carrier Custom Waveform 4th Harmonic (%)",
    [Synth::ParamId::CC5] = "Carrier Custom Waveform 5th Harmonic (%)",
    [Synth::ParamId::CC6] = "Carrier Custom Waveform 6th Harmonic (%)",
    [Synth::ParamId::CC7] = "Carrier Custom Waveform 7th Harmonic (%)",
    [Synth::ParamId::CC8] = "Carrier Custom Waveform 8th Harmonic (%)",
    [Synth::ParamId::CC9] = "Carrier Custom Waveform 9th Harmonic (%)",
    [Synth::ParamId::CC10] = "Carrier Custom Waveform 10th Harmonic (%)",

    [Synth::ParamId::CF1FRQ] = "Carrier Filter 1 Frequency (Hz)",
    [Synth::ParamId::CF1Q] = "Carrier Filter 1 Q Factor",
    [Synth::ParamId::CF1G] = "Carrier Filter 1 Gain (dB)",
    [Synth::ParamId::CF1FIA] = "Carrier Filter 1 Frequency Inaccuracy",
    [Synth::ParamId::CF1QIA] = "Carrier Filter 1 Q Factor Inaccuracy",

    [Synth::ParamId::CF2FRQ] = "Carrier Filter 2 Frequency (Hz)",
    [Synth::ParamId::CF2Q] = "Carrier Filter 2 Q Factor",
    [Synth::ParamId::CF2G] = "Carrier Filter 2 Gain (dB)",
    [Synth::ParamId::CF2FIA] = "Carrier Filter 2 Frequency Inaccuracy",
    [Synth::ParamId::CF2QIA] = "Carrier Filter 2 Q Factor Inaccuracy",

    [Synth::ParamId::EV1V] = "Volume 1 (%)",

    [Synth::ParamId::ED1L] = "Distortion 1 Level (%)",

    [Synth::ParamId::ED2L] = "Distortion 2 Level (%)",

    [Synth::ParamId::EF1FRQ] = "Filter 1 Frequency (Hz)",
    [Synth::ParamId::EF1Q] = "Filter 1 Q Factor",
    [Synth::ParamId::EF1G] = "Filter 1 Gain (dB)",

    [Synth::ParamId::EF2FRQ] = "Filter 2 Frequency (Hz)",
    [Synth::ParamId::EF2Q] = "Filter 2 Q Factor",
    [Synth::ParamId::EF2G] = "Filter 2 Gain (dB)",

    [Synth::ParamId::EV2V] = "Volume 2 (%)",

    [Synth::ParamId::ETSTP] = "Tape Stop / Start (s)",
    [Synth::ParamId::ETWFA] = "Tape Wow and Flutter Amplitude (%)",
    [Synth::ParamId::ETWFS] = "Tape Wow and Flutter Speed (%)",
    [Synth::ParamId::ETSAT] = "Tape Saturation (%)",
    [Synth::ParamId::ETCLR] = "Tape Color (%)",
    [Synth::ParamId::ETHSS] = "Tape Hiss Level (%)",
    [Synth::ParamId::ETSTR] = "Tape Stereo Wow and Flutter (%)",

    [Synth::ParamId::ECDEL] = "Chorus Delay Time (s)",
    [Synth::ParamId::ECFRQ] = "Chorus LFO Frequency (Hz)",
    [Synth::ParamId::ECDPT] = "Chorus Depth (%)",
    [Synth::ParamId::ECFB] = "Chorus Feedback (%)",
    [Synth::ParamId::ECDF] = "Chorus Dampening Frequency (Hz)",
    [Synth::ParamId::ECDG] = "Chorus Dampening Gain (dB)",
    [Synth::ParamId::ECWID] = "Chorus Stereo Width (%)",
    [Synth::ParamId::ECHPF] = "Chorus High-pass Frequency (Hz)",
    [Synth::ParamId::ECHPQ] = "Chorus High-pass Q Factor",
    [Synth::ParamId::ECWET] = "Chorus Wet Volume (%)",
    [Synth::ParamId::ECDRY] = "Chorus Dry Volume (%)",

    [Synth::ParamId::EEDEL] = "Echo Delay Time (s)",
    [Synth::ParamId::EEINV] = "Echo Input Volume (%)",
    [Synth::ParamId::EEFB] = "Echo Feedback (%)",
    [Synth::ParamId::EEDST] = "Echo Distortion (%)",
    [Synth::ParamId::EEDF] = "Echo Dampening Frequency (Hz)",
    [Synth::ParamId::EEDG] = "Echo Dampening Gain (dB)",
    [Synth::ParamId::EEWID] = "Echo Stereo Width (%)",
    [Synth::ParamId::EEHPF] = "Echo High-pass Frequency (Hz)",
    [Synth::ParamId::EEHPQ] = "Echo High-pass Q Factor",
    [Synth::ParamId::EECTH] = "Echo SC. Compr. Threshold (dB)",
    [Synth::ParamId::EECAT] = "Echo SC. Compr. Attack Time (s)",
    [Synth::ParamId::EECRL] = "Echo SC. Compr. Release Time (s)",
    [Synth::ParamId::EECR] = "Echo SC. Compr. Ratio (1:x)",
    [Synth::ParamId::EEWET] = "Echo Wet Volume (%)",
    [Synth::ParamId::EEDRY] = "Echo Dry Volume (%)",

    [Synth::ParamId::ERRS] = "Reverb Room Size (%)",
    [Synth::ParamId::ERRR] = "Reverb Room Reflectivity (%)",
    [Synth::ParamId::ERDST] = "Reverb Distortion (%)",
    [Synth::ParamId::ERDF] = "Reverb Dampening Frequency (Hz)",
    [Synth::ParamId::ERDG] = "Reverb Dampening Gain (dB)",
    [Synth::ParamId::ERWID] = "Reverb Stereo Width (%)",
    [Synth::ParamId::ERHPF] = "Reverb High-pass Frequency (Hz)",
    [Synth::ParamId::ERHPQ] = "Reverb High-pass Q Factor",
    [Synth::ParamId::ERCTH] = "Reverb SC. Compr. Threshold (dB)",
    [Synth::ParamId::ERCAT] = "Reverb SC. Compr. Attack Time (s)",
    [Synth::ParamId::ERCRL] = "Reverb SC. Compr. Release Time (s)",
    [Synth::ParamId::ERCR] = "Reverb SC. Compr. Ratio (1:x)",
    [Synth::ParamId::ERWET] = "Reverb Wet Volume (%)",
    [Synth::ParamId::ERDRY] = "Reverb Dry Volume (%)",

    [Synth::ParamId::EV3V] = "Volume 3 (%)",

    [Synth::ParamId::M1MID] = "Macro 1 Midpoint",
    [Synth::ParamId::M1IN] = "Macro 1 Input (%)",
    [Synth::ParamId::M1MIN] = "Macro 1 Minimum Value (%)",
    [Synth::ParamId::M1MAX] = "Macro 1 Maximum Value (%)",
    [Synth::ParamId::M1SCL] = "Macro 1 Scale (%)",
    [Synth::ParamId::M1DST] = "Macro 1 Distortion (%)",
    [Synth::ParamId::M1RND] = "Macro 1 Randomness (%)",

    [Synth::ParamId::M2MID] = "Macro 2 Midpoint",
    [Synth::ParamId::M2IN] = "Macro 2 Input (%)",
    [Synth::ParamId::M2MIN] = "Macro 2 Minimum Value (%)",
    [Synth::ParamId::M2MAX] = "Macro 2 Maximum Value (%)",
    [Synth::ParamId::M2SCL] = "Macro 2 Scale (%)",
    [Synth::ParamId::M2DST] = "Macro 2 Distortion (%)",
    [Synth::ParamId::M2RND] = "Macro 2 Randomness (%)",

    [Synth::ParamId::M3MID] = "Macro 3 Midpoint",
    [Synth::ParamId::M3IN] = "Macro 3 Input (%)",
    [Synth::ParamId::M3MIN] = "Macro 3 Minimum Value (%)",
    [Synth::ParamId::M3MAX] = "Macro 3 Maximum Value (%)",
    [Synth::ParamId::M3SCL] = "Macro 3 Scale (%)",
    [Synth::ParamId::M3DST] = "Macro 3 Distortion (%)",
    [Synth::ParamId::M3RND] = "Macro 3 Randomness (%)",

    [Synth::ParamId::M4MID] = "Macro 4 Midpoint",
    [Synth::ParamId::M4IN] = "Macro 4 Input (%)",
    [Synth::ParamId::M4MIN] = "Macro 4 Minimum Value (%)",
    [Synth::ParamId::M4MAX] = "Macro 4 Maximum Value (%)",
    [Synth::ParamId::M4SCL] = "Macro 4 Scale (%)",
    [Synth::ParamId::M4DST] = "Macro 4 Distortion (%)",
    [Synth::ParamId::M4RND] = "Macro 4 Randomness (%)",

    [Synth::ParamId::M5MID] = "Macro 5 Midpoint",
    [Synth::ParamId::M5IN] = "Macro 5 Input (%)",
    [Synth::ParamId::M5MIN] = "Macro 5 Minimum Value (%)",
    [Synth::ParamId::M5MAX] = "Macro 5 Maximum Value (%)",
    [Synth::ParamId::M5SCL] = "Macro 5 Scale (%)",
    [Synth::ParamId::M5DST] = "Macro 5 Distortion (%)",
    [Synth::ParamId::M5RND] = "Macro 5 Randomness (%)",

    [Synth::ParamId::M6MID] = "Macro 6 Midpoint",
    [Synth::ParamId::M6IN] = "Macro 6 Input (%)",
    [Synth::ParamId::M6MIN] = "Macro 6 Minimum Value (%)",
    [Synth::ParamId::M6MAX] = "Macro 6 Maximum Value (%)",
    [Synth::ParamId::M6SCL] = "Macro 6 Scale (%)",
    [Synth::ParamId::M6DST] = "Macro 6 Distortion (%)",
    [Synth::ParamId::M6RND] = "Macro 6 Randomness (%)",

    [Synth::ParamId::M7MID] = "Macro 7 Midpoint",
    [Synth::ParamId::M7IN] = "Macro 7 Input (%)",
    [Synth::ParamId::M7MIN] = "Macro 7 Minimum Value (%)",
    [Synth::ParamId::M7MAX] = "Macro 7 Maximum Value (%)",
    [Synth::ParamId::M7SCL] = "Macro 7 Scale (%)",
    [Synth::ParamId::M7DST] = "Macro 7 Distortion (%)",
    [Synth::ParamId::M7RND] = "Macro 7 Randomness (%)",

    [Synth::ParamId::M8MID] = "Macro 8 Midpoint",
    [Synth::ParamId::M8IN] = "Macro 8 Input (%)",
    [Synth::ParamId::M8MIN] = "Macro 8 Minimum Value (%)",
    [Synth::ParamId::M8MAX] = "Macro 8 Maximum Value (%)",
    [Synth::ParamId::M8SCL] = "Macro 8 Scale (%)",
    [Synth::ParamId::M8DST] = "Macro 8 Distortion (%)",
    [Synth::ParamId::M8RND] = "Macro 8 Randomness (%)",

    [Synth::ParamId::M9MID] = "Macro 9 Midpoint",
    [Synth::ParamId::M9IN] = "Macro 9 Input (%)",
    [Synth::ParamId::M9MIN] = "Macro 9 Minimum Value (%)",
    [Synth::ParamId::M9MAX] = "Macro 9 Maximum Value (%)",
    [Synth::ParamId::M9SCL] = "Macro 9 Scale (%)",
    [Synth::ParamId::M9DST] = "Macro 9 Distortion (%)",
    [Synth::ParamId::M9RND] = "Macro 9 Randomness (%)",

    [Synth::ParamId::M10MID] = "Macro 10 Midpoint",
    [Synth::ParamId::M10IN] = "Macro 10 Input (%)",
    [Synth::ParamId::M10MIN] = "Macro 10 Minimum Value (%)",
    [Synth::ParamId::M10MAX] = "Macro 10 Maximum Value (%)",
    [Synth::ParamId::M10SCL] = "Macro 10 Scale (%)",
    [Synth::ParamId::M10DST] = "Macro 10 Distortion (%)",
    [Synth::ParamId::M10RND] = "Macro 10 Randomness (%)",

    [Synth::ParamId::M11MID] = "Macro 11 Midpoint",
    [Synth::ParamId::M11IN] = "Macro 11 Input (%)",
    [Synth::ParamId::M11MIN] = "Macro 11 Minimum Value (%)",
    [Synth::ParamId::M11MAX] = "Macro 11 Maximum Value (%)",
    [Synth::ParamId::M11SCL] = "Macro 11 Scale (%)",
    [Synth::ParamId::M11DST] = "Macro 11 Distortion (%)",
    [Synth::ParamId::M11RND] = "Macro 11 Randomness (%)",

    [Synth::ParamId::M12MID] = "Macro 12 Midpoint",
    [Synth::ParamId::M12IN] = "Macro 12 Input (%)",
    [Synth::ParamId::M12MIN] = "Macro 12 Minimum Value (%)",
    [Synth::ParamId::M12MAX] = "Macro 12 Maximum Value (%)",
    [Synth::ParamId::M12SCL] = "Macro 12 Scale (%)",
    [Synth::ParamId::M12DST] = "Macro 12 Distortion (%)",
    [Synth::ParamId::M12RND] = "Macro 12 Randomness (%)",

    [Synth::ParamId::M13MID] = "Macro 13 Midpoint",
    [Synth::ParamId::M13IN] = "Macro 13 Input (%)",
    [Synth::ParamId::M13MIN] = "Macro 13 Minimum Value (%)",
    [Synth::ParamId::M13MAX] = "Macro 13 Maximum Value (%)",
    [Synth::ParamId::M13SCL] = "Macro 13 Scale (%)",
    [Synth::ParamId::M13DST] = "Macro 13 Distortion (%)",
    [Synth::ParamId::M13RND] = "Macro 13 Randomness (%)",

    [Synth::ParamId::M14MID] = "Macro 14 Midpoint",
    [Synth::ParamId::M14IN] = "Macro 14 Input (%)",
    [Synth::ParamId::M14MIN] = "Macro 14 Minimum Value (%)",
    [Synth::ParamId::M14MAX] = "Macro 14 Maximum Value (%)",
    [Synth::ParamId::M14SCL] = "Macro 14 Scale (%)",
    [Synth::ParamId::M14DST] = "Macro 14 Distortion (%)",
    [Synth::ParamId::M14RND] = "Macro 14 Randomness (%)",

    [Synth::ParamId::M15MID] = "Macro 15 Midpoint",
    [Synth::ParamId::M15IN] = "Macro 15 Input (%)",
    [Synth::ParamId::M15MIN] = "Macro 15 Minimum Value (%)",
    [Synth::ParamId::M15MAX] = "Macro 15 Maximum Value (%)",
    [Synth::ParamId::M15SCL] = "Macro 15 Scale (%)",
    [Synth::ParamId::M15DST] = "Macro 15 Distortion (%)",
    [Synth::ParamId::M15RND] = "Macro 15 Randomness (%)",

    [Synth::ParamId::M16MID] = "Macro 16 Midpoint",
    [Synth::ParamId::M16IN] = "Macro 16 Input (%)",
    [Synth::ParamId::M16MIN] = "Macro 16 Minimum Value (%)",
    [Synth::ParamId::M16MAX] = "Macro 16 Maximum Value (%)",
    [Synth::ParamId::M16SCL] = "Macro 16 Scale (%)",
    [Synth::ParamId::M16DST] = "Macro 16 Distortion (%)",
    [Synth::ParamId::M16RND] = "Macro 16 Randomness (%)",

    [Synth::ParamId::M17MID] = "Macro 17 Midpoint",
    [Synth::ParamId::M17IN] = "Macro 17 Input (%)",
    [Synth::ParamId::M17MIN] = "Macro 17 Minimum Value (%)",
    [Synth::ParamId::M17MAX] = "Macro 17 Maximum Value (%)",
    [Synth::ParamId::M17SCL] = "Macro 17 Scale (%)",
    [Synth::ParamId::M17DST] = "Macro 17 Distortion (%)",
    [Synth::ParamId::M17RND] = "Macro 17 Randomness (%)",

    [Synth::ParamId::M18MID] = "Macro 18 Midpoint",
    [Synth::ParamId::M18IN] = "Macro 18 Input (%)",
    [Synth::ParamId::M18MIN] = "Macro 18 Minimum Value (%)",
    [Synth::ParamId::M18MAX] = "Macro 18 Maximum Value (%)",
    [Synth::ParamId::M18SCL] = "Macro 18 Scale (%)",
    [Synth::ParamId::M18DST] = "Macro 18 Distortion (%)",
    [Synth::ParamId::M18RND] = "Macro 18 Randomness (%)",

    [Synth::ParamId::M19MID] = "Macro 19 Midpoint",
    [Synth::ParamId::M19IN] = "Macro 19 Input (%)",
    [Synth::ParamId::M19MIN] = "Macro 19 Minimum Value (%)",
    [Synth::ParamId::M19MAX] = "Macro 19 Maximum Value (%)",
    [Synth::ParamId::M19SCL] = "Macro 19 Scale (%)",
    [Synth::ParamId::M19DST] = "Macro 19 Distortion (%)",
    [Synth::ParamId::M19RND] = "Macro 19 Randomness (%)",

    [Synth::ParamId::M20MID] = "Macro 20 Midpoint",
    [Synth::ParamId::M20IN] = "Macro 20 Input (%)",
    [Synth::ParamId::M20MIN] = "Macro 20 Minimum Value (%)",
    [Synth::ParamId::M20MAX] = "Macro 20 Maximum Value (%)",
    [Synth::ParamId::M20SCL] = "Macro 20 Scale (%)",
    [Synth::ParamId::M20DST] = "Macro 20 Distortion (%)",
    [Synth::ParamId::M20RND] = "Macro 20 Randomness (%)",

    [Synth::ParamId::M21MID] = "Macro 21 Midpoint",
    [Synth::ParamId::M21IN] = "Macro 21 Input (%)",
    [Synth::ParamId::M21MIN] = "Macro 21 Minimum Value (%)",
    [Synth::ParamId::M21MAX] = "Macro 21 Maximum Value (%)",
    [Synth::ParamId::M21SCL] = "Macro 21 Scale (%)",
    [Synth::ParamId::M21DST] = "Macro 21 Distortion (%)",
    [Synth::ParamId::M21RND] = "Macro 21 Randomness (%)",

    [Synth::ParamId::M22MID] = "Macro 22 Midpoint",
    [Synth::ParamId::M22IN] = "Macro 22 Input (%)",
    [Synth::ParamId::M22MIN] = "Macro 22 Minimum Value (%)",
    [Synth::ParamId::M22MAX] = "Macro 22 Maximum Value (%)",
    [Synth::ParamId::M22SCL] = "Macro 22 Scale (%)",
    [Synth::ParamId::M22DST] = "Macro 22 Distortion (%)",
    [Synth::ParamId::M22RND] = "Macro 22 Randomness (%)",

    [Synth::ParamId::M23MID] = "Macro 23 Midpoint",
    [Synth::ParamId::M23IN] = "Macro 23 Input (%)",
    [Synth::ParamId::M23MIN] = "Macro 23 Minimum Value (%)",
    [Synth::ParamId::M23MAX] = "Macro 23 Maximum Value (%)",
    [Synth::ParamId::M23SCL] = "Macro 23 Scale (%)",
    [Synth::ParamId::M23DST] = "Macro 23 Distortion (%)",
    [Synth::ParamId::M23RND] = "Macro 23 Randomness (%)",

    [Synth::ParamId::M24MID] = "Macro 24 Midpoint",
    [Synth::ParamId::M24IN] = "Macro 24 Input (%)",
    [Synth::ParamId::M24MIN] = "Macro 24 Minimum Value (%)",
    [Synth::ParamId::M24MAX] = "Macro 24 Maximum Value (%)",
    [Synth::ParamId::M24SCL] = "Macro 24 Scale (%)",
    [Synth::ParamId::M24DST] = "Macro 24 Distortion (%)",
    [Synth::ParamId::M24RND] = "Macro 24 Randomness (%)",

    [Synth::ParamId::M25MID] = "Macro 25 Midpoint",
    [Synth::ParamId::M25IN] = "Macro 25 Input (%)",
    [Synth::ParamId::M25MIN] = "Macro 25 Minimum Value (%)",
    [Synth::ParamId::M25MAX] = "Macro 25 Maximum Value (%)",
    [Synth::ParamId::M25SCL] = "Macro 25 Scale (%)",
    [Synth::ParamId::M25DST] = "Macro 25 Distortion (%)",
    [Synth::ParamId::M25RND] = "Macro 25 Randomness (%)",

    [Synth::ParamId::M26MID] = "Macro 26 Midpoint",
    [Synth::ParamId::M26IN] = "Macro 26 Input (%)",
    [Synth::ParamId::M26MIN] = "Macro 26 Minimum Value (%)",
    [Synth::ParamId::M26MAX] = "Macro 26 Maximum Value (%)",
    [Synth::ParamId::M26SCL] = "Macro 26 Scale (%)",
    [Synth::ParamId::M26DST] = "Macro 26 Distortion (%)",
    [Synth::ParamId::M26RND] = "Macro 26 Randomness (%)",

    [Synth::ParamId::M27MID] = "Macro 27 Midpoint",
    [Synth::ParamId::M27IN] = "Macro 27 Input (%)",
    [Synth::ParamId::M27MIN] = "Macro 27 Minimum Value (%)",
    [Synth::ParamId::M27MAX] = "Macro 27 Maximum Value (%)",
    [Synth::ParamId::M27SCL] = "Macro 27 Scale (%)",
    [Synth::ParamId::M27DST] = "Macro 27 Distortion (%)",
    [Synth::ParamId::M27RND] = "Macro 27 Randomness (%)",

    [Synth::ParamId::M28MID] = "Macro 28 Midpoint",
    [Synth::ParamId::M28IN] = "Macro 28 Input (%)",
    [Synth::ParamId::M28MIN] = "Macro 28 Minimum Value (%)",
    [Synth::ParamId::M28MAX] = "Macro 28 Maximum Value (%)",
    [Synth::ParamId::M28SCL] = "Macro 28 Scale (%)",
    [Synth::ParamId::M28DST] = "Macro 28 Distortion (%)",
    [Synth::ParamId::M28RND] = "Macro 28 Randomness (%)",

    [Synth::ParamId::M29MID] = "Macro 29 Midpoint",
    [Synth::ParamId::M29IN] = "Macro 29 Input (%)",
    [Synth::ParamId::M29MIN] = "Macro 29 Minimum Value (%)",
    [Synth::ParamId::M29MAX] = "Macro 29 Maximum Value (%)",
    [Synth::ParamId::M29SCL] = "Macro 29 Scale (%)",
    [Synth::ParamId::M29DST] = "Macro 29 Distortion (%)",
    [Synth::ParamId::M29RND] = "Macro 29 Randomness (%)",

    [Synth::ParamId::M30MID] = "Macro 30Midpoint",
    [Synth::ParamId::M30IN] = "Macro 30 Input (%)",
    [Synth::ParamId::M30MIN] = "Macro 30 Minimum Value (%)",
    [Synth::ParamId::M30MAX] = "Macro 30 Maximum Value (%)",
    [Synth::ParamId::M30SCL] = "Macro 30 Scale (%)",
    [Synth::ParamId::M30DST] = "Macro 30 Distortion (%)",
    [Synth::ParamId::M30RND] = "Macro 30 Randomness (%)",

    [Synth::ParamId::N1SCL] = "Envelope 1 Scale (%)",
    [Synth::ParamId::N1INI] = "Envelope 1 Initial Level (%)",
    [Synth::ParamId::N1DEL] = "Envelope 1 Delay Time (s)",
    [Synth::ParamId::N1ATK] = "Envelope 1 Attack Time (s)",
    [Synth::ParamId::N1PK] = "Envelope 1 Peak Level (%)",
    [Synth::ParamId::N1HLD] = "Envelope 1 Hold Time (s)",
    [Synth::ParamId::N1DEC] = "Envelope 1 Decay Time (s)",
    [Synth::ParamId::N1SUS] = "Envelope 1 Sustain Level (%)",
    [Synth::ParamId::N1REL] = "Envelope 1 Release Time (s)",
    [Synth::ParamId::N1FIN] = "Envelope 1 Final Level (%)",
    [Synth::ParamId::N1TIN] = "Envelope 1 Time Inaccuracy",
    [Synth::ParamId::N1VIN] = "Envelope 1 Level Inaccuracy",

    [Synth::ParamId::N2SCL] = "Envelope 2 Scale (%)",
    [Synth::ParamId::N2INI] = "Envelope 2 Initial Level (%)",
    [Synth::ParamId::N2DEL] = "Envelope 2 Delay Time (s)",
    [Synth::ParamId::N2ATK] = "Envelope 2 Attack Time (s)",
    [Synth::ParamId::N2PK] = "Envelope 2 Peak Level (%)",
    [Synth::ParamId::N2HLD] = "Envelope 2 Hold Time (s)",
    [Synth::ParamId::N2DEC] = "Envelope 2 Decay Time (s)",
    [Synth::ParamId::N2SUS] = "Envelope 2 Sustain Level (%)",
    [Synth::ParamId::N2REL] = "Envelope 2 Release Time (s)",
    [Synth::ParamId::N2FIN] = "Envelope 2 Final Level (%)",
    [Synth::ParamId::N2TIN] = "Envelope 2 Time Inaccuracy",
    [Synth::ParamId::N2VIN] = "Envelope 2 Level Inaccuracy",

    [Synth::ParamId::N3SCL] = "Envelope 3 Scale (%)",
    [Synth::ParamId::N3INI] = "Envelope 3 Initial Level (%)",
    [Synth::ParamId::N3DEL] = "Envelope 3 Delay Time (s)",
    [Synth::ParamId::N3ATK] = "Envelope 3 Attack Time (s)",
    [Synth::ParamId::N3PK] = "Envelope 3 Peak Level (%)",
    [Synth::ParamId::N3HLD] = "Envelope 3 Hold Time (s)",
    [Synth::ParamId::N3DEC] = "Envelope 3 Decay Time (s)",
    [Synth::ParamId::N3SUS] = "Envelope 3 Sustain Level (%)",
    [Synth::ParamId::N3REL] = "Envelope 3 Release Time (s)",
    [Synth::ParamId::N3FIN] = "Envelope 3 Final Level (%)",
    [Synth::ParamId::N3TIN] = "Envelope 3 Time Inaccuracy",
    [Synth::ParamId::N3VIN] = "Envelope 3 Level Inaccuracy",

    [Synth::ParamId::N4SCL] = "Envelope 4 Scale (%)",
    [Synth::ParamId::N4INI] = "Envelope 4 Initial Level (%)",
    [Synth::ParamId::N4DEL] = "Envelope 4 Delay Time (s)",
    [Synth::ParamId::N4ATK] = "Envelope 4 Attack Time (s)",
    [Synth::ParamId::N4PK] = "Envelope 4 Peak Level (%)",
    [Synth::ParamId::N4HLD] = "Envelope 4 Hold Time (s)",
    [Synth::ParamId::N4DEC] = "Envelope 4 Decay Time (s)",
    [Synth::ParamId::N4SUS] = "Envelope 4 Sustain Level (%)",
    [Synth::ParamId::N4REL] = "Envelope 4 Release Time (s)",
    [Synth::ParamId::N4FIN] = "Envelope 4 Final Level (%)",
    [Synth::ParamId::N4TIN] = "Envelope 4 Time Inaccuracy",
    [Synth::ParamId::N4VIN] = "Envelope 4 Level Inaccuracy",

    [Synth::ParamId::N5SCL] = "Envelope 5 Scale (%)",
    [Synth::ParamId::N5INI] = "Envelope 5 Initial Level (%)",
    [Synth::ParamId::N5DEL] = "Envelope 5 Delay Time (s)",
    [Synth::ParamId::N5ATK] = "Envelope 5 Attack Time (s)",
    [Synth::ParamId::N5PK] = "Envelope 5 Peak Level (%)",
    [Synth::ParamId::N5HLD] = "Envelope 5 Hold Time (s)",
    [Synth::ParamId::N5DEC] = "Envelope 5 Decay Time (s)",
    [Synth::ParamId::N5SUS] = "Envelope 5 Sustain Level (%)",
    [Synth::ParamId::N5REL] = "Envelope 5 Release Time (s)",
    [Synth::ParamId::N5FIN] = "Envelope 5 Final Level (%)",
    [Synth::ParamId::N5TIN] = "Envelope 5 Time Inaccuracy",
    [Synth::ParamId::N5VIN] = "Envelope 5 Level Inaccuracy",

    [Synth::ParamId::N6SCL] = "Envelope 6 Scale (%)",
    [Synth::ParamId::N6INI] = "Envelope 6 Initial Level (%)",
    [Synth::ParamId::N6DEL] = "Envelope 6 Delay Time (s)",
    [Synth::ParamId::N6ATK] = "Envelope 6 Attack Time (s)",
    [Synth::ParamId::N6PK] = "Envelope 6 Peak Level (%)",
    [Synth::ParamId::N6HLD] = "Envelope 6 Hold Time (s)",
    [Synth::ParamId::N6DEC] = "Envelope 6 Decay Time (s)",
    [Synth::ParamId::N6SUS] = "Envelope 6 Sustain Level (%)",
    [Synth::ParamId::N6REL] = "Envelope 6 Release Time (s)",
    [Synth::ParamId::N6FIN] = "Envelope 6 Final Level (%)",
    [Synth::ParamId::N6TIN] = "Envelope 6 Time Inaccuracy",
    [Synth::ParamId::N6VIN] = "Envelope 6 Level Inaccuracy",

    [Synth::ParamId::N7SCL] = "Envelope 7 Scale (%)",
    [Synth::ParamId::N7INI] = "Envelope 7 Initial Level (%)",
    [Synth::ParamId::N7DEL] = "Envelope 7 Delay Time (s)",
    [Synth::ParamId::N7ATK] = "Envelope 7 Attack Time (s)",
    [Synth::ParamId::N7PK] = "Envelope 7 Peak Level (%)",
    [Synth::ParamId::N7HLD] = "Envelope 7 Hold Time (s)",
    [Synth::ParamId::N7DEC] = "Envelope 7 Decay Time (s)",
    [Synth::ParamId::N7SUS] = "Envelope 7 Sustain Level (%)",
    [Synth::ParamId::N7REL] = "Envelope 7 Release Time (s)",
    [Synth::ParamId::N7FIN] = "Envelope 7 Final Level (%)",
    [Synth::ParamId::N7TIN] = "Envelope 7 Time Inaccuracy",
    [Synth::ParamId::N7VIN] = "Envelope 7 Level Inaccuracy",

    [Synth::ParamId::N8SCL] = "Envelope 8 Scale (%)",
    [Synth::ParamId::N8INI] = "Envelope 8 Initial Level (%)",
    [Synth::ParamId::N8DEL] = "Envelope 8 Delay Time (s)",
    [Synth::ParamId::N8ATK] = "Envelope 8 Attack Time (s)",
    [Synth::ParamId::N8PK] = "Envelope 8 Peak Level (%)",
    [Synth::ParamId::N8HLD] = "Envelope 8 Hold Time (s)",
    [Synth::ParamId::N8DEC] = "Envelope 8 Decay Time (s)",
    [Synth::ParamId::N8SUS] = "Envelope 8 Sustain Level (%)",
    [Synth::ParamId::N8REL] = "Envelope 8 Release Time (s)",
    [Synth::ParamId::N8FIN] = "Envelope 8 Final Level (%)",
    [Synth::ParamId::N8TIN] = "Envelope 8 Time Inaccuracy",
    [Synth::ParamId::N8VIN] = "Envelope 8 Level Inaccuracy",

    [Synth::ParamId::N9SCL] = "Envelope 9 Scale (%)",
    [Synth::ParamId::N9INI] = "Envelope 9 Initial Level (%)",
    [Synth::ParamId::N9DEL] = "Envelope 9 Delay Time (s)",
    [Synth::ParamId::N9ATK] = "Envelope 9 Attack Time (s)",
    [Synth::ParamId::N9PK] = "Envelope 9 Peak Level (%)",
    [Synth::ParamId::N9HLD] = "Envelope 9 Hold Time (s)",
    [Synth::ParamId::N9DEC] = "Envelope 9 Decay Time (s)",
    [Synth::ParamId::N9SUS] = "Envelope 9 Sustain Level (%)",
    [Synth::ParamId::N9REL] = "Envelope 9 Release Time (s)",
    [Synth::ParamId::N9FIN] = "Envelope 9 Final Level (%)",
    [Synth::ParamId::N9TIN] = "Envelope 9 Time Inaccuracy",
    [Synth::ParamId::N9VIN] = "Envelope 9 Level Inaccuracy",

    [Synth::ParamId::N10SCL] = "Envelope 10 Scale (%)",
    [Synth::ParamId::N10INI] = "Envelope 10 Initial Level (%)",
    [Synth::ParamId::N10DEL] = "Envelope 10 Delay Time (s)",
    [Synth::ParamId::N10ATK] = "Envelope 10 Attack Time (s)",
    [Synth::ParamId::N10PK] = "Envelope 10 Peak Level (%)",
    [Synth::ParamId::N10HLD] = "Envelope 10 Hold Time (s)",
    [Synth::ParamId::N10DEC] = "Envelope 10 Decay Time (s)",
    [Synth::ParamId::N10SUS] = "Envelope 10 Sustain Level (%)",
    [Synth::ParamId::N10REL] = "Envelope 10 Release Time (s)",
    [Synth::ParamId::N10FIN] = "Envelope 10 Final Level (%)",
    [Synth::ParamId::N10TIN] = "Envelope 10 Time Inaccuracy",
    [Synth::ParamId::N10VIN] = "Envelope 10 Level Inaccuracy",

    [Synth::ParamId::N11SCL] = "Envelope 11 Scale (%)",
    [Synth::ParamId::N11INI] = "Envelope 11 Initial Level (%)",
    [Synth::ParamId::N11DEL] = "Envelope 11 Delay Time (s)",
    [Synth::ParamId::N11ATK] = "Envelope 11 Attack Time (s)",
    [Synth::ParamId::N11PK] = "Envelope 11 Peak Level (%)",
    [Synth::ParamId::N11HLD] = "Envelope 11 Hold Time (s)",
    [Synth::ParamId::N11DEC] = "Envelope 11 Decay Time (s)",
    [Synth::ParamId::N11SUS] = "Envelope 11 Sustain Level (%)",
    [Synth::ParamId::N11REL] = "Envelope 11 Release Time (s)",
    [Synth::ParamId::N11FIN] = "Envelope 11 Final Level (%)",
    [Synth::ParamId::N11TIN] = "Envelope 11 Time Inaccuracy",
    [Synth::ParamId::N11VIN] = "Envelope 11 Level Inaccuracy",

    [Synth::ParamId::N12SCL] = "Envelope 12 Scale (%)",
    [Synth::ParamId::N12INI] = "Envelope 12 Initial Level (%)",
    [Synth::ParamId::N12DEL] = "Envelope 12 Delay Time (s)",
    [Synth::ParamId::N12ATK] = "Envelope 12 Attack Time (s)",
    [Synth::ParamId::N12PK] = "Envelope 12 Peak Level (%)",
    [Synth::ParamId::N12HLD] = "Envelope 12 Hold Time (s)",
    [Synth::ParamId::N12DEC] = "Envelope 12 Decay Time (s)",
    [Synth::ParamId::N12SUS] = "Envelope 12 Sustain Level (%)",
    [Synth::ParamId::N12REL] = "Envelope 12 Release Time (s)",
    [Synth::ParamId::N12FIN] = "Envelope 12 Final Level (%)",
    [Synth::ParamId::N12TIN] = "Envelope 12 Time Inaccuracy",
    [Synth::ParamId::N12VIN] = "Envelope 12 Level Inaccuracy",

    [Synth::ParamId::L1PW] = "LFO 1 Pulse Width (%)",
    [Synth::ParamId::L1FRQ] = "LFO 1 Frequency (Hz)",
    [Synth::ParamId::L1PHS] = "LFO 1 Phase (degree)",
    [Synth::ParamId::L1MIN] = "LFO 1 Minimum Value (%)",
    [Synth::ParamId::L1MAX] = "LFO 1 Maximum Value (%)",
    [Synth::ParamId::L1AMP] = "LFO 1 Amplitude (%)",
    [Synth::ParamId::L1DST] = "LFO 1 Distortion (%)",
    [Synth::ParamId::L1RND] = "LFO 1 Randomness (%)",

    [Synth::ParamId::L2FRQ] = "LFO 2 Frequency (Hz)",
    [Synth::ParamId::L2PHS] = "LFO 2 Phase (degree)",
    [Synth::ParamId::L2MIN] = "LFO 2 Minimum Value (%)",
    [Synth::ParamId::L2MAX] = "LFO 2 Maximum Value (%)",
    [Synth::ParamId::L2AMP] = "LFO 2 Amplitude (%)",
    [Synth::ParamId::L2DST] = "LFO 2 Distortion (%)",
    [Synth::ParamId::L2RND] = "LFO 2 Randomness (%)",

    [Synth::ParamId::L3PW] = "LFO 3 Pulse Width (%)",
    [Synth::ParamId::L3FRQ] = "LFO 3 Frequency (Hz)",
    [Synth::ParamId::L3PHS] = "LFO 3 Phase (degree)",
    [Synth::ParamId::L3MIN] = "LFO 3 Minimum Value (%)",
    [Synth::ParamId::L3MAX] = "LFO 3 Maximum Value (%)",
    [Synth::ParamId::L3AMP] = "LFO 3 Amplitude (%)",
    [Synth::ParamId::L3DST] = "LFO 3 Distortion (%)",
    [Synth::ParamId::L3RND] = "LFO 3 Randomness (%)",

    [Synth::ParamId::L4FRQ] = "LFO 4 Frequency (Hz)",
    [Synth::ParamId::L4PHS] = "LFO 4 Phase (degree)",
    [Synth::ParamId::L4MIN] = "LFO 4 Minimum Value (%)",
    [Synth::ParamId::L4MAX] = "LFO 4 Maximum Value (%)",
    [Synth::ParamId::L4AMP] = "LFO 4 Amplitude (%)",
    [Synth::ParamId::L4DST] = "LFO 4 Distortion (%)",
    [Synth::ParamId::L4RND] = "LFO 4 Randomness (%)",

    [Synth::ParamId::L5PW] = "LFO 5 Pulse Width (%)",
    [Synth::ParamId::L5FRQ] = "LFO 5 Frequency (Hz)",
    [Synth::ParamId::L5PHS] = "LFO 5 Phase (degree)",
    [Synth::ParamId::L5MIN] = "LFO 5 Minimum Value (%)",
    [Synth::ParamId::L5MAX] = "LFO 5 Maximum Value (%)",
    [Synth::ParamId::L5AMP] = "LFO 5 Amplitude (%)",
    [Synth::ParamId::L5DST] = "LFO 5 Distortion (%)",
    [Synth::ParamId::L5RND] = "LFO 5 Randomness (%)",

    [Synth::ParamId::L6FRQ] = "LFO 6 Frequency (Hz)",
    [Synth::ParamId::L6PHS] = "LFO 6 Phase (degree)",
    [Synth::ParamId::L6MIN] = "LFO 6 Minimum Value (%)",
    [Synth::ParamId::L6MAX] = "LFO 6 Maximum Value (%)",
    [Synth::ParamId::L6AMP] = "LFO 6 Amplitude (%)",
    [Synth::ParamId::L6DST] = "LFO 6 Distortion (%)",
    [Synth::ParamId::L6RND] = "LFO 6 Randomness (%)",

    [Synth::ParamId::L7PW] = "LFO 7 Pulse Width (%)",
    [Synth::ParamId::L7FRQ] = "LFO 7 Frequency (Hz)",
    [Synth::ParamId::L7PHS] = "LFO 7 Phase (degree)",
    [Synth::ParamId::L7MIN] = "LFO 7 Minimum Value (%)",
    [Synth::ParamId::L7MAX] = "LFO 7 Maximum Value (%)",
    [Synth::ParamId::L7AMP] = "LFO 7 Amplitude (%)",
    [Synth::ParamId::L7DST] = "LFO 7 Distortion (%)",
    [Synth::ParamId::L7RND] = "LFO 7 Randomness (%)",

    [Synth::ParamId::L8FRQ] = "LFO 8 Frequency (Hz)",
    [Synth::ParamId::L8PHS] = "LFO 8 Phase (degree)",
    [Synth::ParamId::L8MIN] = "LFO 8 Minimum Value (%)",
    [Synth::ParamId::L8MAX] = "LFO 8 Maximum Value (%)",
    [Synth::ParamId::L8AMP] = "LFO 8 Amplitude (%)",
    [Synth::ParamId::L8DST] = "LFO 8 Distortion (%)",
    [Synth::ParamId::L8RND] = "LFO 8 Randomness (%)",

    [Synth::ParamId::MODE] = "Operating Mode",

    [Synth::ParamId::MWFM] = "Modulator Waveform",
    [Synth::ParamId::CWFM] = "Carrier Waveform",

    [Synth::ParamId::MF1TYP] = "Modulator Filter 1 Type",
    [Synth::ParamId::MF2TYP] = "Modulator Filter 2 Type",

    [Synth::ParamId::CF1TYP] = "Carrier Filter 1 Type",
    [Synth::ParamId::CDTYP] = "Carrier Distortion Type",
    [Synth::ParamId::CF2TYP] = "Carrier Filter 2 Type",

    [Synth::ParamId::ED1TYP] = "Distortion 1 Type",
    [Synth::ParamId::ED2TYP] = "Distortion 2 Type",

    [Synth::ParamId::EF1TYP] = "Filter 1 Type",
    [Synth::ParamId::EF2TYP] = "Filter 2 Type",

    [Synth::ParamId::L1WAV] = "LFO 1 Waveform",
    [Synth::ParamId::L2WAV] = "LFO 2 Waveform",
    [Synth::ParamId::L3WAV] = "LFO 3 Waveform",
    [Synth::ParamId::L4WAV] = "LFO 4 Waveform",
    [Synth::ParamId::L5WAV] = "LFO 5 Waveform",
    [Synth::ParamId::L6WAV] = "LFO 6 Waveform",
    [Synth::ParamId::L7WAV] = "LFO 7 Waveform",
    [Synth::ParamId::L8WAV] = "LFO 8 Waveform",

    [Synth::ParamId::L1LOG] = "LFO 1 Logarithmic Frequency",
    [Synth::ParamId::L2LOG] = "LFO 2 Logarithmic Frequency",
    [Synth::ParamId::L3LOG] = "LFO 3 Logarithmic Frequency",
    [Synth::ParamId::L4LOG] = "LFO 4 Logarithmic Frequency",
    [Synth::ParamId::L5LOG] = "LFO 5 Logarithmic Frequency",
    [Synth::ParamId::L6LOG] = "LFO 6 Logarithmic Frequency",
    [Synth::ParamId::L7LOG] = "LFO 7 Logarithmic Frequency",
    [Synth::ParamId::L8LOG] = "LFO 8 Logarithmic Frequency",

    [Synth::ParamId::L1CEN] = "LFO 1 Center",
    [Synth::ParamId::L2CEN] = "LFO 2 Center",
    [Synth::ParamId::L3CEN] = "LFO 3 Center",
    [Synth::ParamId::L4CEN] = "LFO 4 Center",
    [Synth::ParamId::L5CEN] = "LFO 5 Center",
    [Synth::ParamId::L6CEN] = "LFO 6 Center",
    [Synth::ParamId::L7CEN] = "LFO 7 Center",
    [Synth::ParamId::L8CEN] = "LFO 8 Center",

    [Synth::ParamId::L1SYN] = "LFO 1 Tempo Synchronization",
    [Synth::ParamId::L2SYN] = "LFO 2 Tempo Synchronization",
    [Synth::ParamId::L3SYN] = "LFO 3 Tempo Synchronization",
    [Synth::ParamId::L4SYN] = "LFO 4 Tempo Synchronization",
    [Synth::ParamId::L5SYN] = "LFO 5 Tempo Synchronization",
    [Synth::ParamId::L6SYN] = "LFO 6 Tempo Synchronization",
    [Synth::ParamId::L7SYN] = "LFO 7 Tempo Synchronization",
    [Synth::ParamId::L8SYN] = "LFO 8 Tempo Synchronization",

    [Synth::ParamId::ECSYN] = "Chorus Tempo Sync",

    [Synth::ParamId::EESYN] = "Echo Tempo Sync",

    [Synth::ParamId::MF1LOG] = "Modulator Filter 1 Logarithmic Frequency",
    [Synth::ParamId::MF2LOG] = "Modulator Filter 2 Logarithmic Frequency",
    [Synth::ParamId::CF1LOG] = "Carrier Filter 1 Logarithmic Frequency",
    [Synth::ParamId::CF2LOG] = "Carrier Filter 2 Logarithmic Frequency",
    [Synth::ParamId::EF1LOG] = "Filter 1 Logarithmic Frequency",
    [Synth::ParamId::EF2LOG] = "Filter 2 Logarithmic Frequency",
    [Synth::ParamId::ECLOG] = "Chorus Logarithmic Filter Frequencies",
    [Synth::ParamId::ECLHQ] = "Chorus Logarithmic High-pass Q Factor",
    [Synth::ParamId::ECLLG] = "Chorus Logarithmic LFO Frequency",
    [Synth::ParamId::EELOG] = "Echo Logarithmic Filter Frequencies",
    [Synth::ParamId::EELHQ] = "Echo Logarithmic High-pass Q Factor",
    [Synth::ParamId::ERLOG] = "Reverb Logarithmic Filter Frequencies",
    [Synth::ParamId::ERLHQ] = "Reverb Logarithmic High-pass Q Factor",

    [Synth::ParamId::N1UPD] = "Envelope 1 Update Mode",
    [Synth::ParamId::N2UPD] = "Envelope 2 Update Mode",
    [Synth::ParamId::N3UPD] = "Envelope 3 Update Mode",
    [Synth::ParamId::N4UPD] = "Envelope 4 Update Mode",
    [Synth::ParamId::N5UPD] = "Envelope 5 Update Mode",
    [Synth::ParamId::N6UPD] = "Envelope 6 Update Mode",
    [Synth::ParamId::N7UPD] = "Envelope 7 Update Mode",
    [Synth::ParamId::N8UPD] = "Envelope 8 Update Mode",
    [Synth::ParamId::N9UPD] = "Envelope 9 Update Mode",
    [Synth::ParamId::N10UPD] = "Envelope 10 Update Mode",
    [Synth::ParamId::N11UPD] = "Envelope 11 Update Mode",
    [Synth::ParamId::N12UPD] = "Envelope 12 Update Mode",

    [Synth::ParamId::NH] = "Note Handling",

    [Synth::ParamId::ERTYP] = "Reverb Type",
    [Synth::ParamId::ECTYP] = "Chorus Type",

    [Synth::ParamId::MTUN] = "Modulator Tuning",
    [Synth::ParamId::CTUN] = "Carrier Tuning",

    [Synth::ParamId::MOIA] = "Modulator Oscillator Inaccuracy",
    [Synth::ParamId::MOIS] = "Modulator Oscillator Instability",

    [Synth::ParamId::COIA] = "Carrier Oscillator Inaccuracy",
    [Synth::ParamId::COIS] = "Carrier Oscillator Instability",

    [Synth::ParamId::MF1QLG] = "Modulator Filter 1 Logarithmic Q Factor",
    [Synth::ParamId::MF2QLG] = "Modulator Filter 2 Logarithmic Q Factor",
    [Synth::ParamId::CF1QLG] = "Carrier Filter 1 Logarithmic Q Factor",
    [Synth::ParamId::CF2QLG] = "Carrier Filter 2 Logarithmic Q Factor",
    [Synth::ParamId::EF1QLG] = "Filter 1 Logarithmic Q Factor",
    [Synth::ParamId::EF2QLG] = "Filter 2 Logarithmic Q Factor",

    [Synth::ParamId::L1AEN] = "LFO 1 Amplitude Envelope",
    [Synth::ParamId::L2AEN] = "LFO 2 Amplitude Envelope",
    [Synth::ParamId::L3AEN] = "LFO 3 Amplitude Envelope",
    [Synth::ParamId::L4AEN] = "LFO 4 Amplitude Envelope",
    [Synth::ParamId::L5AEN] = "LFO 5 Amplitude Envelope",
    [Synth::ParamId::L6AEN] = "LFO 6 Amplitude Envelope",
    [Synth::ParamId::L7AEN] = "LFO 7 Amplitude Envelope",
    [Synth::ParamId::L8AEN] = "LFO 8 Amplitude Envelope",

    [Synth::ParamId::N1SYN] = "Envelope 1 Tempo Synchronization",
    [Synth::ParamId::N2SYN] = "Envelope 2 Tempo Synchronization",
    [Synth::ParamId::N3SYN] = "Envelope 3 Tempo Synchronization",
    [Synth::ParamId::N4SYN] = "Envelope 4 Tempo Synchronization",
    [Synth::ParamId::N5SYN] = "Envelope 5 Tempo Synchronization",
    [Synth::ParamId::N6SYN] = "Envelope 6 Tempo Synchronization",
    [Synth::ParamId::N7SYN] = "Envelope 7 Tempo Synchronization",
    [Synth::ParamId::N8SYN] = "Envelope 8 Tempo Synchronization",
    [Synth::ParamId::N9SYN] = "Envelope 9 Tempo Synchronization",
    [Synth::ParamId::N10SYN] = "Envelope 10 Tempo Synchronization",
    [Synth::ParamId::N11SYN] = "Envelope 11 Tempo Synchronization",
    [Synth::ParamId::N12SYN] = "Envelope 12 Tempo Synchronization",

    [Synth::ParamId::N1ASH] = "Envelope 1 Attack Shape",
    [Synth::ParamId::N2ASH] = "Envelope 2 Attack Shape",
    [Synth::ParamId::N3ASH] = "Envelope 3 Attack Shape",
    [Synth::ParamId::N4ASH] = "Envelope 4 Attack Shape",
    [Synth::ParamId::N5ASH] = "Envelope 5 Attack Shape",
    [Synth::ParamId::N6ASH] = "Envelope 6 Attack Shape",
    [Synth::ParamId::N7ASH] = "Envelope 7 Attack Shape",
    [Synth::ParamId::N8ASH] = "Envelope 8 Attack Shape",
    [Synth::ParamId::N9ASH] = "Envelope 9 Attack Shape",
    [Synth::ParamId::N10ASH] = "Envelope 10 Attack Shape",
    [Synth::ParamId::N11ASH] = "Envelope 11 Attack Shape",
    [Synth::ParamId::N12ASH] = "Envelope 12 Attack Shape",

    [Synth::ParamId::N1DSH] = "Envelope 1 Decay Shape",
    [Synth::ParamId::N2DSH] = "Envelope 2 Decay Shape",
    [Synth::ParamId::N3DSH] = "Envelope 3 Decay Shape",
    [Synth::ParamId::N4DSH] = "Envelope 4 Decay Shape",
    [Synth::ParamId::N5DSH] = "Envelope 5 Decay Shape",
    [Synth::ParamId::N6DSH] = "Envelope 6 Decay Shape",
    [Synth::ParamId::N7DSH] = "Envelope 7 Decay Shape",
    [Synth::ParamId::N8DSH] = "Envelope 8 Decay Shape",
    [Synth::ParamId::N9DSH] = "Envelope 9 Decay Shape",
    [Synth::ParamId::N10DSH] = "Envelope 10 Decay Shape",
    [Synth::ParamId::N11DSH] = "Envelope 11 Decay Shape",
    [Synth::ParamId::N12DSH] = "Envelope 12 Decay Shape",

    [Synth::ParamId::N1RSH] = "Envelope 1 Release Shape",
    [Synth::ParamId::N2RSH] = "Envelope 2 Release Shape",
    [Synth::ParamId::N3RSH] = "Envelope 3 Release Shape",
    [Synth::ParamId::N4RSH] = "Envelope 4 Release Shape",
    [Synth::ParamId::N5RSH] = "Envelope 5 Release Shape",
    [Synth::ParamId::N6RSH] = "Envelope 6 Release Shape",
    [Synth::ParamId::N7RSH] = "Envelope 7 Release Shape",
    [Synth::ParamId::N8RSH] = "Envelope 8 Release Shape",
    [Synth::ParamId::N9RSH] = "Envelope 9 Release Shape",
    [Synth::ParamId::N10RSH] = "Envelope 10 Release Shape",
    [Synth::ParamId::N11RSH] = "Envelope 11 Release Shape",
    [Synth::ParamId::N12RSH] = "Envelope 12 Release Shape",

    [Synth::ParamId::M1DCV] = "Macro 1 Distortion Curve",
    [Synth::ParamId::M2DCV] = "Macro 2 Distortion Curve",
    [Synth::ParamId::M3DCV] = "Macro 3 Distortion Curve",
    [Synth::ParamId::M4DCV] = "Macro 4 Distortion Curve",
    [Synth::ParamId::M5DCV] = "Macro 5 Distortion Curve",
    [Synth::ParamId::M6DCV] = "Macro 6 Distortion Curve",
    [Synth::ParamId::M7DCV] = "Macro 7 Distortion Curve",
    [Synth::ParamId::M8DCV] = "Macro 8 Distortion Curve",
    [Synth::ParamId::M9DCV] = "Macro 9 Distortion Curve",
    [Synth::ParamId::M10DCV] = "Macro 10 Distortion Curve",
    [Synth::ParamId::M11DCV] = "Macro 11 Distortion Curve",
    [Synth::ParamId::M12DCV] = "Macro 12 Distortion Curve",
    [Synth::ParamId::M13DCV] = "Macro 13 Distortion Curve",
    [Synth::ParamId::M14DCV] = "Macro 14 Distortion Curve",
    [Synth::ParamId::M15DCV] = "Macro 15 Distortion Curve",
    [Synth::ParamId::M16DCV] = "Macro 16 Distortion Curve",
    [Synth::ParamId::M17DCV] = "Macro 17 Distortion Curve",
    [Synth::ParamId::M18DCV] = "Macro 18 Distortion Curve",
    [Synth::ParamId::M19DCV] = "Macro 19 Distortion Curve",
    [Synth::ParamId::M20DCV] = "Macro 20 Distortion Curve",
    [Synth::ParamId::M21DCV] = "Macro 21 Distortion Curve",
    [Synth::ParamId::M22DCV] = "Macro 22 Distortion Curve",
    [Synth::ParamId::M23DCV] = "Macro 23 Distortion Curve",
    [Synth::ParamId::M24DCV] = "Macro 24 Distortion Curve",
    [Synth::ParamId::M25DCV] = "Macro 25 Distortion Curve",
    [Synth::ParamId::M26DCV] = "Macro 26 Distortion Curve",
    [Synth::ParamId::M27DCV] = "Macro 27 Distortion Curve",
    [Synth::ParamId::M28DCV] = "Macro 28 Distortion Curve",
    [Synth::ParamId::M29DCV] = "Macro 29 Distortion Curve",
    [Synth::ParamId::M30DCV] = "Macro 30 Distortion Curve",
    [Synth::ParamId::MFX4] = "Modulator Fine Detune x4",
    [Synth::ParamId::CFX4] = "Carrier Fine Detune x4",
    [Synth::ParamId::EER1] = "Echo Delay 1 Reversed",
    [Synth::ParamId::EER2] = "Echo Delay 2 Reversed",
    [Synth::ParamId::ETSTYP] = "Tape Saturation Type",
    [Synth::ParamId::ETEND] = "Tape Position at End of Chain",
    [Synth::ParamId::EECM] = "Echo Side-Chain Compression Mode",
    [Synth::ParamId::ERCM] = "Reverb Side-Chain Compression Mode",
    [Synth::ParamId::MPEST] = "MPE Settings",
};


#define Ctl(index, capability, id, long_name, name_8, name_5)   \
    Controller(                                                 \
        index,                                                  \
        ControllerCapability:: capability,                      \
        Synth::ControllerId:: id,                               \
        long_name,                                              \
        name_8,                                                 \
        name_5                                                  \
    )

GUI::Controller const GUI::CONTROLLERS[] = {
    Ctl(0, NONE, NONE, "(none)", "(none)", "none"),

    Ctl(1, MIDI_CONTROLLER, TRIGGERED_NOTE, "Triggered Note", "Note on", "NOn"),
    Ctl(
        2,
        MIDI_CONTROLLER,
        TRIGGERED_VELOCITY,
        "Triggered Velocity",
        "Vel on",
        "VOn"
    ),
    Ctl(3, MIDI_CONTROLLER, RELEASED_NOTE, "Released Note", "Note off", "NOff"),
    Ctl(
        4,
        MIDI_CONTROLLER,
        RELEASED_VELOCITY,
        "Released Velocity",
        "Vel off",
        "VOff"
    ),
    Ctl(5, MIDI_CONTROLLER, PITCH_WHEEL, "Pitch Wheel", "PtchWh", "Ptch"),
    Ctl(6, MIDI_CONTROLLER, OSC_1_PEAK, "Osc 1 Out Peak", "O1 Pk", "O1Pk"),
    Ctl(7, MIDI_CONTROLLER, OSC_2_PEAK, "Osc 2 Out Peak", "O2 Pk", "O2Pk"),
    Ctl(8, MIDI_CONTROLLER, VOL_1_PEAK, "Vol 1 In Peak", "V1 Pk", "V1Pk"),
    Ctl(9, MIDI_CONTROLLER, VOL_2_PEAK, "Vol 2 In Peak", "V2 Pk", "V2Pk"),
    Ctl(10, MIDI_CONTROLLER, VOL_3_PEAK, "Vol 3 In Peak", "V3 Pk", "V3Pk"),
    Ctl(
        11,
        CHANNEL_PRESSURE,
        CHANNEL_PRESSURE,
        "Channel Aftertouch",
        "Ch AT",
        "ChAT"
    ),

    Ctl(12, MIDI_CONTROLLER, MIDI_LEARN, "MIDI Learn", "Learn", "Lrn"),

    Ctl(
        13,
        MIDI_CONTROLLER,
        MODULATION_WHEEL,
        "MIDI CC 1 (Modulation Wheel)",
        "ModWh",
        "C1"
    ),
    Ctl(14, MIDI_CONTROLLER, BREATH, "MIDI CC 2 (Breath)", "Breath", "C2"),
    Ctl(15, MIDI_CONTROLLER, UNDEFINED_1, "MIDI CC 3", "CC 3", "C3"),
    Ctl(
        16, MIDI_CONTROLLER, FOOT_PEDAL, "MIDI CC 4 (Foot Pedal)", "Foot", "C4"
    ),
    Ctl(
        17,
        MIDI_CONTROLLER,
        PORTAMENTO_TIME,
        "MIDI CC 5 (Portamento Time)",
        "PortT",
        "C5"
    ),
    Ctl(
        18, MIDI_CONTROLLER, DATA_ENTRY, "MIDI CC 6 (Data Entry)", "DtEnt", "C6"
    ),
    Ctl(19, MIDI_CONTROLLER, VOLUME, "MIDI CC 7 (Volume)", "Vol", "C7"),
    Ctl(20, MIDI_CONTROLLER, BALANCE, "MIDI CC 8 (Balance)", "Blnc", "C8"),
    Ctl(21, MIDI_CONTROLLER, UNDEFINED_2, "MIDI CC 9", "CC 9", "C9"),
    Ctl(22, MIDI_CONTROLLER, PAN, "MIDI CC 10 (Pan)", "Pan", "C10"),
    Ctl(
        23,
        MIDI_CONTROLLER,
        EXPRESSION_PEDAL,
        "MIDI CC 11 (Expr. Pedal)",
        "Expr",
        "C11"
    ),
    Ctl(
        24,
        MIDI_CONTROLLER,
        FX_CTL_1,
        "MIDI CC 12 (Effect Control 1)",
        "Fx C 1",
        "C12"
    ),
    Ctl(
        25,
        MIDI_CONTROLLER,
        FX_CTL_2,
        "MIDI CC 13 (Effect Control 2)",
        "Fx C 2",
        "C13"
    ),
    Ctl(26, MIDI_CONTROLLER, UNDEFINED_3, "MIDI CC 14", "CC 14", "C14"),
    Ctl(27, MIDI_CONTROLLER, UNDEFINED_4, "MIDI CC 15", "CC 15", "C15"),
    Ctl(
        28, MIDI_CONTROLLER, GENERAL_1, "MIDI CC 16 (General 1)", "Gen 1", "C16"
    ),
    Ctl(
        29, MIDI_CONTROLLER, GENERAL_2, "MIDI CC 17 (General 2)", "Gen 2", "C17"
    ),
    Ctl(
        30, MIDI_CONTROLLER, GENERAL_3, "MIDI CC 18 (General 3)", "Gen 3", "C18"
    ),
    Ctl(
        31, MIDI_CONTROLLER, GENERAL_4, "MIDI CC 19 (General 4)", "Gen 4", "C19"
    ),
    Ctl(32, MIDI_CONTROLLER, UNDEFINED_5, "MIDI CC 20", "CC 20", "C20"),
    Ctl(33, MIDI_CONTROLLER, UNDEFINED_6, "MIDI CC 21", "CC 21", "C21"),
    Ctl(34, MIDI_CONTROLLER, UNDEFINED_7, "MIDI CC 22", "CC 22", "C22"),
    Ctl(35, MIDI_CONTROLLER, UNDEFINED_8, "MIDI CC 23", "CC 23", "C23"),
    Ctl(36, MIDI_CONTROLLER, UNDEFINED_9, "MIDI CC 24", "CC 24", "C24"),
    Ctl(37, MIDI_CONTROLLER, UNDEFINED_10, "MIDI CC 25", "CC 25", "C25"),
    Ctl(38, MIDI_CONTROLLER, UNDEFINED_11, "MIDI CC 26", "CC 26", "C26"),
    Ctl(39, MIDI_CONTROLLER, UNDEFINED_12, "MIDI CC 27", "CC 27", "C27"),
    Ctl(40, MIDI_CONTROLLER, UNDEFINED_13, "MIDI CC 28", "CC 28", "C28"),
    Ctl(41, MIDI_CONTROLLER, UNDEFINED_14, "MIDI CC 29", "CC 29", "C29"),
    Ctl(42, MIDI_CONTROLLER, UNDEFINED_15, "MIDI CC 30", "CC 30", "C30"),
    Ctl(43, MIDI_CONTROLLER, UNDEFINED_16, "MIDI CC 31", "CC 31", "C31"),
    Ctl(
        44,
        MIDI_CONTROLLER,
        SUSTAIN_PEDAL,
        "MIDI CC 64 (Sustain Pedal)",
        "Sustn",
        "C64"
    ),
    Ctl(45, MIDI_CONTROLLER, SOUND_1, "MIDI CC 70 (Sound 1)", "Snd 1", "C70"),
    Ctl(46, MIDI_CONTROLLER, SOUND_2, "MIDI CC 71 (Sound 2)", "Snd 2", "C71"),
    Ctl(47, MIDI_CONTROLLER, SOUND_3, "MIDI CC 72 (Sound 3)", "Snd 3", "C72"),
    Ctl(48, MIDI_CONTROLLER, SOUND_4, "MIDI CC 73 (Sound 4)", "Snd 4", "C73"),
    Ctl(49, MIDI_CONTROLLER, SOUND_5, "MIDI CC 74 (Sound 5)", "Snd 5", "C74"),
    Ctl(50, MIDI_CONTROLLER, SOUND_6, "MIDI CC 75 (Sound 6)", "Snd 6", "C75"),
    Ctl(51, MIDI_CONTROLLER, SOUND_7, "MIDI CC 76 (Sound 7)", "Snd 7", "C76"),
    Ctl(52, MIDI_CONTROLLER, SOUND_8, "MIDI CC 77 (Sound 8)", "Snd 8", "C77"),
    Ctl(53, MIDI_CONTROLLER, SOUND_9, "MIDI CC 78 (Sound 9)", "Snd 9", "C78"),
    Ctl(
        54, MIDI_CONTROLLER, SOUND_10, "MIDI CC 79 (Sound 10)", "Snd 10", "C79"
    ),
    Ctl(55, MIDI_CONTROLLER, UNDEFINED_17, "MIDI CC 85", "CC 85", "C85"),
    Ctl(56, MIDI_CONTROLLER, UNDEFINED_18, "MIDI CC 86", "CC 86", "C86"),
    Ctl(57, MIDI_CONTROLLER, UNDEFINED_19, "MIDI CC 87", "CC 87", "C87"),
    Ctl(58, MIDI_CONTROLLER, UNDEFINED_20, "MIDI CC 88", "CC 88", "C88"),
    Ctl(59, MIDI_CONTROLLER, UNDEFINED_21, "MIDI CC 89", "CC 89", "C89"),
    Ctl(60, MIDI_CONTROLLER, UNDEFINED_22, "MIDI CC 90", "CC 90", "C90"),
    Ctl(61, MIDI_CONTROLLER, FX_1, "MIDI CC 91 (Effect 1)", "Fx 1", "C91"),
    Ctl(62, MIDI_CONTROLLER, FX_2, "MIDI CC 92 (Effect 2)", "Fx 2", "C92"),
    Ctl(63, MIDI_CONTROLLER, FX_3, "MIDI CC 93 (Effect 3)", "Fx 3", "C93"),
    Ctl(64, MIDI_CONTROLLER, FX_4, "MIDI CC 94 (Effect 4)", "Fx 4", "C94"),
    Ctl(65, MIDI_CONTROLLER, FX_5, "MIDI CC 95 (Effect 5)", "Fx 5", "C95"),
    Ctl(66, MIDI_CONTROLLER, UNDEFINED_23, "MIDI CC 102", "CC 102", "C102"),
    Ctl(67, MIDI_CONTROLLER, UNDEFINED_24, "MIDI CC 103", "CC 103", "C103"),
    Ctl(68, MIDI_CONTROLLER, UNDEFINED_25, "MIDI CC 104", "CC 104", "C104"),
    Ctl(69, MIDI_CONTROLLER, UNDEFINED_26, "MIDI CC 105", "CC 105", "C105"),
    Ctl(70, MIDI_CONTROLLER, UNDEFINED_27, "MIDI CC 106", "CC 106", "C106"),
    Ctl(71, MIDI_CONTROLLER, UNDEFINED_28, "MIDI CC 107", "CC 107", "C107"),
    Ctl(72, MIDI_CONTROLLER, UNDEFINED_29, "MIDI CC 108", "CC 108", "C108"),
    Ctl(73, MIDI_CONTROLLER, UNDEFINED_30, "MIDI CC 109", "CC 109", "C109"),
    Ctl(74, MIDI_CONTROLLER, UNDEFINED_31, "MIDI CC 110", "CC 110", "C110"),
    Ctl(75, MIDI_CONTROLLER, UNDEFINED_32, "MIDI CC 111", "CC 111", "C111"),
    Ctl(76, MIDI_CONTROLLER, UNDEFINED_33, "MIDI CC 112", "CC 112", "C112"),
    Ctl(77, MIDI_CONTROLLER, UNDEFINED_34, "MIDI CC 113", "CC 113", "C113"),
    Ctl(78, MIDI_CONTROLLER, UNDEFINED_35, "MIDI CC 114", "CC 114", "C114"),
    Ctl(79, MIDI_CONTROLLER, UNDEFINED_36, "MIDI CC 115", "CC 115", "C115"),
    Ctl(80, MIDI_CONTROLLER, UNDEFINED_37, "MIDI CC 116", "CC 116", "C116"),
    Ctl(81, MIDI_CONTROLLER, UNDEFINED_38, "MIDI CC 117", "CC 117", "C117"),
    Ctl(82, MIDI_CONTROLLER, UNDEFINED_39, "MIDI CC 118", "CC 118", "C118"),
    Ctl(83, MIDI_CONTROLLER, UNDEFINED_40, "MIDI CC 119", "CC 119", "C119"),

    Ctl(84, MACRO, MACRO_1, "Macro 1", "MCR 1", "M1"),
    Ctl(85, MACRO, MACRO_2, "Macro 2", "MCR 2", "M2"),
    Ctl(86, MACRO, MACRO_3, "Macro 3", "MCR 3", "M3"),
    Ctl(87, MACRO, MACRO_4, "Macro 4", "MCR 4", "M4"),
    Ctl(88, MACRO, MACRO_5, "Macro 5", "MCR 5", "M5"),
    Ctl(89, MACRO, MACRO_6, "Macro 6", "MCR 6", "M6"),
    Ctl(90, MACRO, MACRO_7, "Macro 7", "MCR 7", "M7"),
    Ctl(91, MACRO, MACRO_8, "Macro 8", "MCR 8", "M8"),
    Ctl(92, MACRO, MACRO_9, "Macro 9", "MCR 9", "M9"),
    Ctl(93, MACRO, MACRO_10, "Macro 10", "MCR 10", "M10"),
    Ctl(94, MACRO, MACRO_11, "Macro 11", "MCR 11", "M11"),
    Ctl(95, MACRO, MACRO_12, "Macro 12", "MCR 12", "M12"),
    Ctl(96, MACRO, MACRO_13, "Macro 13", "MCR 13", "M13"),
    Ctl(97, MACRO, MACRO_14, "Macro 14", "MCR 14", "M14"),
    Ctl(98, MACRO, MACRO_15, "Macro 15", "MCR 15", "M15"),
    Ctl(99, MACRO, MACRO_16, "Macro 16", "MCR 16", "M16"),
    Ctl(100, MACRO, MACRO_17, "Macro 17", "MCR 17", "M17"),
    Ctl(101, MACRO, MACRO_18, "Macro 18", "MCR 18", "M18"),
    Ctl(102, MACRO, MACRO_19, "Macro 19", "MCR 19", "M19"),
    Ctl(103, MACRO, MACRO_20, "Macro 20", "MCR 20", "M20"),
    Ctl(104, MACRO, MACRO_21, "Macro 21", "MCR 21", "M21"),
    Ctl(105, MACRO, MACRO_22, "Macro 22", "MCR 22", "M22"),
    Ctl(106, MACRO, MACRO_23, "Macro 23", "MCR 23", "M23"),
    Ctl(107, MACRO, MACRO_24, "Macro 24", "MCR 24", "M24"),
    Ctl(108, MACRO, MACRO_25, "Macro 25", "MCR 25", "M25"),
    Ctl(109, MACRO, MACRO_26, "Macro 26", "MCR 26", "M26"),
    Ctl(110, MACRO, MACRO_27, "Macro 27", "MCR 27", "M27"),
    Ctl(111, MACRO, MACRO_28, "Macro 28", "MCR 28", "M28"),
    Ctl(112, MACRO, MACRO_29, "Macro 29", "MCR 29", "M29"),
    Ctl(113, MACRO, MACRO_30, "Macro 30", "MCR 30", "M30"),

    Ctl(114, LFO, LFO_1, "LFO 1", "LFO 1", "LFO1"),
    Ctl(115, LFO, LFO_2, "LFO 2", "LFO 2", "LFO2"),
    Ctl(116, LFO, LFO_3, "LFO 3", "LFO 3", "LFO3"),
    Ctl(117, LFO, LFO_4, "LFO 4", "LFO 4", "LFO4"),
    Ctl(118, LFO, LFO_5, "LFO 5", "LFO 5", "LFO5"),
    Ctl(119, LFO, LFO_6, "LFO 6", "LFO 6", "LFO6"),
    Ctl(120, LFO, LFO_7, "LFO 7", "LFO 7", "LFO7"),
    Ctl(121, LFO, LFO_8, "LFO 8", "LFO 8", "LFO8"),

    Ctl(122, ENVELOPE, ENVELOPE_1, "Envelope 1", "ENV 1", "E1"),
    Ctl(123, ENVELOPE, ENVELOPE_2, "Envelope 2", "ENV 2", "E2"),
    Ctl(124, ENVELOPE, ENVELOPE_3, "Envelope 3", "ENV 3", "E3"),
    Ctl(125, ENVELOPE, ENVELOPE_4, "Envelope 4", "ENV 4", "E4"),
    Ctl(126, ENVELOPE, ENVELOPE_5, "Envelope 5", "ENV 5", "E5"),
    Ctl(127, ENVELOPE, ENVELOPE_6, "Envelope 6", "ENV 6", "E6"),
    Ctl(128, ENVELOPE, ENVELOPE_7, "Envelope 7", "ENV 7", "E7"),
    Ctl(129, ENVELOPE, ENVELOPE_8, "Envelope 8", "ENV 8", "E8"),
    Ctl(130, ENVELOPE, ENVELOPE_9, "Envelope 9", "ENV 9", "E9"),
    Ctl(131, ENVELOPE, ENVELOPE_10, "Envelope 10", "ENV 10", "E10"),
    Ctl(132, ENVELOPE, ENVELOPE_11, "Envelope 11", "ENV 11", "E11"),
    Ctl(133, ENVELOPE, ENVELOPE_12, "Envelope 12", "ENV 12", "E12"),
};

#undef Ctl


GUI::Controller const*
    GUI::controllers_by_id[Synth::ControllerId::CONTROLLER_ID_COUNT] = { NULL };


GUI::Controller const* GUI::get_controller(
        Synth::ControllerId const controller_id
) {
    initialize_controllers_by_id();

    Controller const* const ctl = controllers_by_id[controller_id];

    return ctl != NULL ? ctl : &CONTROLLERS[0];
}


void GUI::initialize_controllers_by_id()
{
    if (JS80P_LIKELY(controllers_by_id_initialized)) {
        return;
    }

    for (int i = 0; i != CONTROLLERS_COUNT; ++i) {
        controllers_by_id[CONTROLLERS[i].id] = &CONTROLLERS[i];
    }

    controllers_by_id_initialized = true;
}


const GUI::Color GUI::TEXT_COLOR = GUI::rgb(181, 181, 189);
const GUI::Color GUI::TEXT_BACKGROUND = GUI::rgb(0, 0, 0);
const GUI::Color GUI::TEXT_HIGHLIGHT_COLOR = GUI::rgb(230, 230, 235);
const GUI::Color GUI::TEXT_HIGHLIGHT_BACKGROUND = GUI::rgb(82, 82, 86);
const GUI::Color GUI::STATUS_LINE_BACKGROUND = GUI::rgb(21, 21, 32);
const GUI::Color GUI::TOGGLE_OFF_COLOR = GUI::rgb(0, 0, 0);
const GUI::Color GUI::TOGGLE_ON_COLOR = GUI::rgb(150, 200, 230);
const GUI::Color GUI::TOGGLE_ON_BLUR_COLOR = GUI::rgb(75, 100, 115);


const GUI::Color GUI::CTL_COLOR_NONE_TEXT = TEXT_COLOR;
const GUI::Color GUI::CTL_COLOR_NONE_BG = TEXT_HIGHLIGHT_BACKGROUND;

const GUI::Color GUI::CTL_COLOR_MIDI_CC_TEXT = GUI::rgb(255, 255, 120);
const GUI::Color GUI::CTL_COLOR_MIDI_CC_BG = GUI::rgb(145, 145, 68);

const GUI::Color GUI::CTL_COLOR_MIDI_SPECIAL_TEXT = GUI::rgb(255, 220, 150);
const GUI::Color GUI::CTL_COLOR_MIDI_SPECIAL_BG = GUI::rgb(145, 125, 85);

const GUI::Color GUI::CTL_COLOR_MIDI_LEARN_TEXT = GUI::rgb(90, 120, 230);
const GUI::Color GUI::CTL_COLOR_MIDI_LEARN_BG = GUI::rgb(51, 68, 131);

const GUI::Color GUI::CTL_COLOR_AFTERTOUCH_TEXT = GUI::rgb(255, 160, 110);
const GUI::Color GUI::CTL_COLOR_AFTERTOUCH_BG = GUI::rgb(145, 91, 63);

const GUI::Color GUI::CTL_COLOR_MACRO_TEXT = GUI::rgb(110, 190, 255);
const GUI::Color GUI::CTL_COLOR_MACRO_BG = GUI::rgb(63, 108, 145);

const GUI::Color GUI::CTL_COLOR_LFO_TEXT = GUI::rgb(230, 100, 255);
const GUI::Color GUI::CTL_COLOR_LFO_BG = GUI::rgb(131, 57, 145);

const GUI::Color GUI::CTL_COLOR_ENVELOPE_TEXT = GUI::rgb(110, 255, 150);
const GUI::Color GUI::CTL_COLOR_ENVELOPE_BG = GUI::rgb(63, 145, 85);


void GUI::param_ratio_to_str(
        Synth const& synth,
        Synth::ParamId const param_id,
        Number const ratio,
        Number const scale,
        char const* const format,
        char const* const* const options,
        size_t const number_of_options,
        char* const buffer,
        size_t const buffer_size
) {
    if (format != NULL) {
        param_ratio_to_str_float(
            synth, param_id, ratio, scale, format, buffer, buffer_size
        );
    } else if (options != NULL) {
        param_ratio_to_str_options(
            synth,
            param_id,
            ratio,
            options,
            number_of_options,
            buffer,
            buffer_size
        );
    } else {
        param_ratio_to_str_int(synth, param_id, ratio, buffer, buffer_size);
    }
}


void GUI::param_ratio_to_str_float(
        Synth const& synth,
        Synth::ParamId const param_id,
        Number const ratio,
        Number const scale,
        char const* const format,
        char* const buffer,
        size_t const buffer_size
) {
    Number const value = (
        synth.float_param_ratio_to_display_value(param_id, ratio) * scale
    );

    snprintf(buffer, buffer_size, format, value);

    bool minus_zero = buffer[0] == '-';

    for (size_t i = 1; minus_zero && i != buffer_size; ++i) {
        if (buffer[i] == '\x00') {
            break;
        }

        if (buffer[i] != '0' && buffer[i] != '.') {
            minus_zero = false;
        }
    }

    if (minus_zero) {
        snprintf(buffer, buffer_size, format, 0.0);
    }

    buffer[buffer_size - 1] = '\x00';
}


void GUI::param_ratio_to_str_options(
        Synth const& synth,
        Synth::ParamId const param_id,
        Number const ratio,
        char const* const* const options,
        size_t const number_of_options,
        char* const buffer,
        size_t const buffer_size
) {
    Byte const value = (
        synth.byte_param_ratio_to_display_value(param_id, ratio)
    );

    if ((size_t)value >= number_of_options) {
        buffer[0] = '\x00';

        return;
    }

    strncpy(buffer, options[value], buffer_size - 1);
    buffer[buffer_size - 1] = '\x00';
}


void GUI::param_ratio_to_str_int(
        Synth const& synth,
        Synth::ParamId const param_id,
        Number const ratio,
        char* const buffer,
        size_t const buffer_size
) {
    Byte const value = (
        synth.byte_param_ratio_to_display_value(param_id, ratio)
    );

    snprintf(buffer, buffer_size, "%hhu", value);
    buffer[buffer_size - 1] = '\x00';
}


Number GUI::clamp_ratio(Number const ratio)
{
    return std::clamp(ratio, 0.0, 1.0);
}


GUI::Color GUI::controller_id_to_text_color(
        Synth::ControllerId const controller_id
) {
    switch (controller_id) {
        case Synth::ControllerId::NONE:
            return CTL_COLOR_NONE_TEXT;

        case Synth::ControllerId::PITCH_WHEEL:
        case Synth::ControllerId::TRIGGERED_NOTE:
        case Synth::ControllerId::RELEASED_NOTE:
        case Synth::ControllerId::TRIGGERED_VELOCITY:
        case Synth::ControllerId::RELEASED_VELOCITY:
        case Synth::ControllerId::OSC_1_PEAK:
        case Synth::ControllerId::OSC_2_PEAK:
        case Synth::ControllerId::VOL_1_PEAK:
        case Synth::ControllerId::VOL_2_PEAK:
        case Synth::ControllerId::VOL_3_PEAK:
            return CTL_COLOR_MIDI_SPECIAL_TEXT;

        case Synth::ControllerId::MACRO_1:
        case Synth::ControllerId::MACRO_2:
        case Synth::ControllerId::MACRO_3:
        case Synth::ControllerId::MACRO_4:
        case Synth::ControllerId::MACRO_5:
        case Synth::ControllerId::MACRO_6:
        case Synth::ControllerId::MACRO_7:
        case Synth::ControllerId::MACRO_8:
        case Synth::ControllerId::MACRO_9:
        case Synth::ControllerId::MACRO_10:
        case Synth::ControllerId::MACRO_11:
        case Synth::ControllerId::MACRO_12:
        case Synth::ControllerId::MACRO_13:
        case Synth::ControllerId::MACRO_14:
        case Synth::ControllerId::MACRO_15:
        case Synth::ControllerId::MACRO_16:
        case Synth::ControllerId::MACRO_17:
        case Synth::ControllerId::MACRO_18:
        case Synth::ControllerId::MACRO_19:
        case Synth::ControllerId::MACRO_20:
        case Synth::ControllerId::MACRO_21:
        case Synth::ControllerId::MACRO_22:
        case Synth::ControllerId::MACRO_23:
        case Synth::ControllerId::MACRO_24:
        case Synth::ControllerId::MACRO_25:
        case Synth::ControllerId::MACRO_26:
        case Synth::ControllerId::MACRO_27:
        case Synth::ControllerId::MACRO_28:
        case Synth::ControllerId::MACRO_29:
        case Synth::ControllerId::MACRO_30:
            return CTL_COLOR_MACRO_TEXT;

        case Synth::ControllerId::LFO_1:
        case Synth::ControllerId::LFO_2:
        case Synth::ControllerId::LFO_3:
        case Synth::ControllerId::LFO_4:
        case Synth::ControllerId::LFO_5:
        case Synth::ControllerId::LFO_6:
        case Synth::ControllerId::LFO_7:
        case Synth::ControllerId::LFO_8:
            return CTL_COLOR_LFO_TEXT;

        case Synth::ControllerId::ENVELOPE_1:
        case Synth::ControllerId::ENVELOPE_2:
        case Synth::ControllerId::ENVELOPE_3:
        case Synth::ControllerId::ENVELOPE_4:
        case Synth::ControllerId::ENVELOPE_5:
        case Synth::ControllerId::ENVELOPE_6:
        case Synth::ControllerId::ENVELOPE_7:
        case Synth::ControllerId::ENVELOPE_8:
        case Synth::ControllerId::ENVELOPE_9:
        case Synth::ControllerId::ENVELOPE_10:
        case Synth::ControllerId::ENVELOPE_11:
        case Synth::ControllerId::ENVELOPE_12:
            return CTL_COLOR_ENVELOPE_TEXT;

        case Synth::ControllerId::CHANNEL_PRESSURE:
            return CTL_COLOR_AFTERTOUCH_TEXT;

        case Synth::ControllerId::MIDI_LEARN:
            return CTL_COLOR_MIDI_LEARN_TEXT;

        default: return CTL_COLOR_MIDI_CC_TEXT;
    };
}


GUI::Color GUI::controller_id_to_bg_color(
        Synth::ControllerId const controller_id
) {
    switch (controller_id) {
        case Synth::ControllerId::NONE:
            return CTL_COLOR_NONE_BG;

        case Synth::ControllerId::PITCH_WHEEL:
        case Synth::ControllerId::TRIGGERED_NOTE:
        case Synth::ControllerId::TRIGGERED_VELOCITY:
        case Synth::ControllerId::RELEASED_NOTE:
        case Synth::ControllerId::RELEASED_VELOCITY:
            return CTL_COLOR_MIDI_SPECIAL_BG;

        case Synth::ControllerId::MACRO_1:
        case Synth::ControllerId::MACRO_2:
        case Synth::ControllerId::MACRO_3:
        case Synth::ControllerId::MACRO_4:
        case Synth::ControllerId::MACRO_5:
        case Synth::ControllerId::MACRO_6:
        case Synth::ControllerId::MACRO_7:
        case Synth::ControllerId::MACRO_8:
        case Synth::ControllerId::MACRO_9:
        case Synth::ControllerId::MACRO_10:
        case Synth::ControllerId::MACRO_11:
        case Synth::ControllerId::MACRO_12:
        case Synth::ControllerId::MACRO_13:
        case Synth::ControllerId::MACRO_14:
        case Synth::ControllerId::MACRO_15:
        case Synth::ControllerId::MACRO_16:
        case Synth::ControllerId::MACRO_17:
        case Synth::ControllerId::MACRO_18:
        case Synth::ControllerId::MACRO_19:
        case Synth::ControllerId::MACRO_20:
        case Synth::ControllerId::MACRO_21:
        case Synth::ControllerId::MACRO_22:
        case Synth::ControllerId::MACRO_23:
        case Synth::ControllerId::MACRO_24:
        case Synth::ControllerId::MACRO_25:
        case Synth::ControllerId::MACRO_26:
        case Synth::ControllerId::MACRO_27:
        case Synth::ControllerId::MACRO_28:
        case Synth::ControllerId::MACRO_29:
        case Synth::ControllerId::MACRO_30:
            return CTL_COLOR_MACRO_BG;

        case Synth::ControllerId::LFO_1:
        case Synth::ControllerId::LFO_2:
        case Synth::ControllerId::LFO_3:
        case Synth::ControllerId::LFO_4:
        case Synth::ControllerId::LFO_5:
        case Synth::ControllerId::LFO_6:
        case Synth::ControllerId::LFO_7:
        case Synth::ControllerId::LFO_8:
            return CTL_COLOR_LFO_BG;

        case Synth::ControllerId::ENVELOPE_1:
        case Synth::ControllerId::ENVELOPE_2:
        case Synth::ControllerId::ENVELOPE_3:
        case Synth::ControllerId::ENVELOPE_4:
        case Synth::ControllerId::ENVELOPE_5:
        case Synth::ControllerId::ENVELOPE_6:
        case Synth::ControllerId::ENVELOPE_7:
        case Synth::ControllerId::ENVELOPE_8:
        case Synth::ControllerId::ENVELOPE_9:
        case Synth::ControllerId::ENVELOPE_10:
        case Synth::ControllerId::ENVELOPE_11:
        case Synth::ControllerId::ENVELOPE_12:
            return CTL_COLOR_ENVELOPE_BG;

        case Synth::ControllerId::CHANNEL_PRESSURE:
            return CTL_COLOR_AFTERTOUCH_BG;

        case Synth::ControllerId::MIDI_LEARN:
            return CTL_COLOR_MIDI_LEARN_BG;

        default: return CTL_COLOR_MIDI_CC_BG;
    };
}


#define KNOB_W 116
#define KNOB_H 200
#define KNOB_TOP 32

#define SCREW_W 40
#define SCREW_H 40

constexpr int pos_rel_offset_left = 0;
constexpr int pos_rel_offset_top = 0;

#define POSITION_RELATIVE_BEGIN(left, top)                  \
    do {                                                    \
        constexpr int pos_rel_offset_left = left;           \
        constexpr int pos_rel_offset_top = top;             \

#define POSITION_RELATIVE_END()                             \
    } while (false)

#define KNOB(left, top, param_id, ctls, varg1, varg2, ks)   \
    KNOB4(                                                  \
        left,                                               \
        top,                                                \
        param_id,                                           \
        ctls,                                               \
        varg1,                                              \
        varg2,                                              \
        ks,                                                 \
        Synth::ParamId::INVALID_PARAM_ID                    \
    )


#define KNOB4(left, top, param_id, ctls, varg1, varg2, ks, x4_param_id) \
    knob_owner->own(                                                    \
        new KnobParamEditor(                                            \
            *this,                                                      \
            GUI::PARAMS[Synth::ParamId:: param_id],                     \
            pos_rel_offset_left + left,                                 \
            pos_rel_offset_top + top,                                   \
            KNOB_W,                                                     \
            KNOB_H,                                                     \
            KNOB_TOP,                                                   \
            *controller_selector,                                       \
            synth,                                                      \
            Synth::ParamId:: param_id,                                  \
            ctls,                                                       \
            varg1,                                                      \
            varg2,                                                      \
            ks,                                                         \
            x4_param_id                                                 \
        )                                                               \
    )

#define SCREW(left, top, param_id, varg1, varg2, ks)    \
    knob_owner->own(                                    \
        new KnobParamEditor(                            \
            *this,                                      \
            GUI::PARAMS[Synth::ParamId:: param_id],     \
            pos_rel_offset_left + left,                 \
            pos_rel_offset_top + top,                   \
            SCREW_W,                                    \
            SCREW_H,                                    \
            0,                                          \
            *controller_selector,                       \
            synth,                                      \
            Synth::ParamId:: param_id,                  \
            0,                                          \
            varg1,                                      \
            varg2,                                      \
            ks                                          \
        )                                               \
    )

#define TOGG(left, top, width, height, box_left, param_id)  \
    knob_owner->own(                                        \
        new ToggleSwitchParamEditor(                        \
            *this,                                          \
            GUI::PARAMS[Synth::ParamId:: param_id],         \
            pos_rel_offset_left + left,                     \
            pos_rel_offset_top + top,                       \
            width,                                          \
            height,                                         \
            box_left,                                       \
            synth,                                          \
            Synth::ParamId:: param_id                       \
        )                                                   \
    )

#define DPEI(left, top, w, h, vleft, vwidth, param_id, imgs)    \
    knob_owner->own(                                            \
        new DiscreteParamEditor(                                \
            *this,                                              \
            GUI::PARAMS[Synth::ParamId:: param_id],             \
            pos_rel_offset_left + left,                         \
            pos_rel_offset_top + top,                           \
            w,                                                  \
            h,                                                  \
            vleft,                                              \
            vwidth,                                             \
            synth,                                              \
            Synth::ParamId:: param_id,                          \
            imgs                                                \
        )                                                       \
    )

#define DPET(l, t, w, h, vl, vw, param_id, opts, nopts) \
    knob_owner->own(                                    \
        new DiscreteParamEditor(                        \
            *this,                                      \
            GUI::PARAMS[Synth::ParamId:: param_id],     \
            pos_rel_offset_left + l,                    \
            pos_rel_offset_top + t,                     \
            w,                                          \
            h,                                          \
            vl,                                         \
            vw,                                         \
            synth,                                      \
            Synth::ParamId:: param_id,                  \
            opts,                                       \
            nopts                                       \
        )                                               \
    )

#define MACMID(left, top, param_id, varg1, varg2, ks)   \
    knob_owner->own(                                    \
        new KnobParamEditor(                            \
            *this,                                      \
            GUI::PARAMS[Synth::ParamId:: param_id],     \
            pos_rel_offset_left + left,                 \
            pos_rel_offset_top + top,                   \
            42,                                         \
            42,                                         \
            0,                                          \
            *controller_selector,                       \
            synth,                                      \
            Synth::ParamId:: param_id,                  \
            0,                                          \
            varg1,                                      \
            varg2,                                      \
            ks                                          \
        )                                               \
    )

#define M____ (ControllerCapability::MIDI_CONTROLLER)

#define MM___ (                                 \
    0                                           \
    | ControllerCapability::MIDI_CONTROLLER     \
    | ControllerCapability::MACRO               \
)

#define MM__C (                                 \
    0                                           \
    | ControllerCapability::MIDI_CONTROLLER     \
    | ControllerCapability::MACRO               \
    | ControllerCapability::CHANNEL_PRESSURE    \
)

#define MML__ (                                 \
    0                                           \
    | ControllerCapability::MIDI_CONTROLLER     \
    | ControllerCapability::MACRO               \
    | ControllerCapability::LFO                 \
)

#define MML_C (                                 \
    0                                           \
    | ControllerCapability::MIDI_CONTROLLER     \
    | ControllerCapability::MACRO               \
    | ControllerCapability::LFO                 \
    | ControllerCapability::CHANNEL_PRESSURE    \
)

#define MMLEC (                                 \
    0                                           \
    | ControllerCapability::MIDI_CONTROLLER     \
    | ControllerCapability::MACRO               \
    | ControllerCapability::LFO                 \
    | ControllerCapability::ENVELOPE            \
    | ControllerCapability::CHANNEL_PRESSURE    \
)


GUI::GUI(
        char const* const sdk_version,
        PlatformData platform_data,
        PlatformWidget parent_window,
        Synth& synth,
        bool const show_vst_logo,
        EventHandler* const event_handler
)
    : show_vst_logo(show_vst_logo),
    default_event_handler(*this),
    event_handler(
        event_handler == NULL ? &default_event_handler : event_handler
    ),
    dummy_widget(NULL),
    background(NULL),
    about_body(NULL),
    macros_1_body(NULL),
    macros_2_body(NULL),
    macros_3_body(NULL),
    effects_body(NULL),
    envelopes_1_body(NULL),
    envelopes_2_body(NULL),
    lfos_body(NULL),
    synth_body(NULL),
    status_line(NULL),
    scale(1.0),
    active_voices_count(0),
    tape_state(TapeParams::State::TAPE_STATE_INIT),
    default_status_line_color(TEXT_COLOR),
    prev_resize_time_ms(0),
    width(WIDTH),
    height(HEIGHT),
    new_width(WIDTH),
    new_height(HEIGHT),
    synth(synth),
    platform_data(platform_data),
    resizing_allowed(true)
{
    default_status_line[0] = '\x00';
    update_synth_state();

    initialize();

    dummy_widget = new Widget("");

    knob_states = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "KNOBSTATESFREE"),
        dummy_widget->load_image(this->platform_data, "KNOBSTATESCONTROLLED"),
        NULL,
        dummy_widget->load_image(this->platform_data, "KNOBSTATESNONE"),
        128,
        96,
        96
    );

    knob_states_red = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "KNOBSTATESRED"),
        NULL,
        NULL,
        NULL,
        128,
        96,
        96
    );

    screw_states = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "SCREWSTATES"),
        NULL,
        dummy_widget->load_image(this->platform_data, "SCREWSTATESSYNCED"),
        NULL,
        61,
        SCREW_W,
        SCREW_H
    );

    envelope_shapes_01 = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "ENVSHAPES01"),
        NULL,
        NULL,
        NULL,
        13,
        42,
        42
    );

    envelope_shapes_10 = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "ENVSHAPES10"),
        NULL,
        NULL,
        NULL,
        13,
        42,
        42
    );

    macro_distortions = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "MACRODIST"),
        NULL,
        NULL,
        NULL,
        4,
        42,
        42
    );

    macro_midpoint_states = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "MACROMID"),
        NULL,
        NULL,
        NULL,
        128,
        42,
        42
    );

    reversed_toggle_states = new ParamStateImages(
        dummy_widget,
        dummy_widget->load_image(this->platform_data, "REVERSED"),
        NULL,
        NULL,
        NULL,
        2,
        36,
        36
    );

    vst_logo_image = dummy_widget->load_image(this->platform_data, "VSTLOGO");

    background = new Background(*this);

    this->parent_window = new ExternallyCreatedWindow(
        this->platform_data, parent_window
    );
    this->parent_window->own(background);

    status_line = new StatusLine();
    status_line->set_text("");

    controller_selector = new ControllerSelector(*background, synth);

    build_about_body(sdk_version);
    build_macros_1_body(knob_states, macro_distortions, macro_midpoint_states);
    build_macros_2_body(knob_states, macro_distortions, macro_midpoint_states);
    build_macros_3_body(knob_states, macro_distortions, macro_midpoint_states);
    build_effects_body(knob_states, knob_states_red, reversed_toggle_states);
    build_envelopes_1_body(
        knob_states, screw_states, envelope_shapes_01, envelope_shapes_10
    );
    build_envelopes_2_body(
        knob_states, screw_states, envelope_shapes_01, envelope_shapes_10
    );
    build_lfos_body(knob_states);
    build_synth_body(knob_states, screw_states);

    background->own(
        new TabSelector(
            background,
            "SYNTH",
            synth_body,
            "Synth",
            TabSelector::LEFT + TabSelector::WIDTH * 0
        )
    );
    background->own(
        new TabSelector(
            background,
            "EFFECTS",
            effects_body,
            "Effects",
            TabSelector::LEFT + TabSelector::WIDTH * 1
        )
    );
    background->own(
        new TabSelector(
            background,
            "MACROS1",
            macros_1_body,
            "Macros 1-10",
            TabSelector::LEFT + TabSelector::WIDTH * 2
        )
    );
    background->own(
        new TabSelector(
            background,
            "MACROS2",
            macros_2_body,
            "Macros 11-20",
            TabSelector::LEFT + TabSelector::WIDTH * 3
        )
    );
    background->own(
        new TabSelector(
            background,
            "MACROS3",
            macros_3_body,
            "Macros 21-30",
            TabSelector::LEFT + TabSelector::WIDTH * 4
        )
    );
    background->own(
        new TabSelector(
            background,
            "ENVELOPES1",
            envelopes_1_body,
            "Envelopes 1-6",
            TabSelector::LEFT + TabSelector::WIDTH * 5
        )
    );
    background->own(
        new TabSelector(
            background,
            "ENVELOPES2",
            envelopes_2_body,
            "Envelopes 7-12",
            TabSelector::LEFT + TabSelector::WIDTH * 6
        )
    );
    background->own(
        new TabSelector(
            background,
            "LFOS",
            lfos_body,
            "LFOs",
            TabSelector::LEFT + TabSelector::WIDTH * 7
        )
    );
    background->own(
        new TabSelector(
            background,
            "ABOUT",
            about_body,
            "About",
            TabSelector::LEFT + TabSelector::WIDTH * 8
        )
    );

    background->replace_body(synth_body);

    background->own(status_line);
    background->own(controller_selector);
    controller_selector->hide();

    resize(INIT_WIDTH, INIT_HEIGHT);
}


void GUI::build_about_body(char const* const sdk_version)
{
    about_body = new TabBody(*this, "About");

    background->own(about_body);

    about_body->own(new ResizerHandle(*this));
    about_body->own(
        new AboutText(sdk_version, show_vst_logo ? vst_logo_image : NULL)
    );

    about_body->hide();
}


void GUI::build_macros_1_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const macro_distortions,
        ParamStateImages const* const macro_midpoint_states
) {
    macros_1_body = new TabBody(*this, "Macros 1-10");

    TabBody* const knob_owner = macros_1_body;

    background->own(macros_1_body);

    macros_1_body->own(new ResizerHandle(*this));


    POSITION_RELATIVE_BEGIN(52, 28);

    KNOB(12 + KNOB_W * 0,  58, M1IN,   MM__C,  "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M1MIN,  MM__C,  "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M1MAX,  MM__C,  "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M1SCL,  MM__C,  "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M1DST,  MM__C,  "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M1RND,  MM__C,  "%.2f", 100.0, knob_states);

    MACMID(266, 6, M1MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M1DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(452, 28);

    KNOB(12 + KNOB_W * 0,  58, M2IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M2MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M2MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M2SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M2DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M2RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M2MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M2DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(852, 28);

    KNOB(12 + KNOB_W * 0,  58, M3IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M3MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M3MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M3SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M3DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M3RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M3MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M3DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1252, 28);

    KNOB(12 + KNOB_W * 0,  58, M4IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M4MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M4MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M4SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M4DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M4RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M4MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M4DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1652, 28);

    KNOB(12 + KNOB_W * 0,  58, M5IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M5MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M5MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M5SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M5DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M5RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M5MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M5DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(52, 586);

    KNOB(12 + KNOB_W * 0,  58, M6IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M6MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M6MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M6SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M6DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M6RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M6MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M6DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(452, 586);

    KNOB(12 + KNOB_W * 0,  58, M7IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M7MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M7MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M7SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M7DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M7RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M7MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M7DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(852, 586);

    KNOB(12 + KNOB_W * 0,  58, M8IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M8MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M8MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M8SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M8DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M8RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M8MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M8DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1252, 586);

    KNOB(12 + KNOB_W * 0,  58, M9IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M9MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M9MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M9SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M9DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M9RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M9MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M9DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1652, 586);

    KNOB(12 + KNOB_W * 0,  58, M10IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M10MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M10MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M10SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M10DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M10RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M10MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M10DCV, macro_distortions);

    POSITION_RELATIVE_END();


    macros_1_body->hide();
}


void GUI::build_macros_2_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const macro_distortions,
        ParamStateImages const* const macro_midpoint_states
) {
    macros_2_body = new TabBody(*this, "Macros 11-20");

    TabBody* const knob_owner = macros_2_body;

    background->own(macros_2_body);

    macros_2_body->own(new ResizerHandle(*this));


    POSITION_RELATIVE_BEGIN(52, 28);

    KNOB(12 + KNOB_W * 0,  58, M11IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M11MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M11MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M11SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M11DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M11RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M11MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M11DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(452, 28);

    KNOB(12 + KNOB_W * 0,  58, M12IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M12MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M12MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M12SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M12DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M12RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M12MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M12DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(852, 28);

    KNOB(12 + KNOB_W * 0,  58, M13IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M13MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M13MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M13SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M13DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M13RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M13MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M13DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1252, 28);

    KNOB(12 + KNOB_W * 0,  58, M14IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M14MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M14MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M14SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M14DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M14RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M14MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M14DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1652, 28);

    KNOB(12 + KNOB_W * 0,  58, M15IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M15MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M15MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M15SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M15DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M15RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M15MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M15DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(52, 586);

    KNOB(12 + KNOB_W * 0,  58, M16IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M16MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M16MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M16SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M16DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M16RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M16MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M16DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(452, 586);

    KNOB(12 + KNOB_W * 0,  58, M17IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M17MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M17MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M17SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M17DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M17RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M17MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M17DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(852, 586);

    KNOB(12 + KNOB_W * 0,  58, M18IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M18MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M18MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M18SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M18DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M18RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M18MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M18DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1252, 586);

    KNOB(12 + KNOB_W * 0,  58, M19IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M19MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M19MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M19SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M19DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M19RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M19MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M19DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1652, 586);

    KNOB(12 + KNOB_W * 0,  58, M20IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M20MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M20MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M20SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M20DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M20RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M20MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M20DCV, macro_distortions);

    POSITION_RELATIVE_END();


    macros_2_body->hide();
}


void GUI::build_macros_3_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const macro_distortions,
        ParamStateImages const* const macro_midpoint_states
) {
    macros_3_body = new TabBody(*this, "Macros 21-30");

    TabBody* const knob_owner = macros_3_body;

    background->own(macros_3_body);

    macros_3_body->own(new ResizerHandle(*this));


    POSITION_RELATIVE_BEGIN(52, 28);

    KNOB(12 + KNOB_W * 0,  58, M21IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M21MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M21MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M21SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M21DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M21RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M21MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M21DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(452, 28);

    KNOB(12 + KNOB_W * 0,  58, M22IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M22MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M22MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M22SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M22DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M22RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M22MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M22DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(852, 28);

    KNOB(12 + KNOB_W * 0,  58, M23IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M23MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M23MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M23SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M23DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M23RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M23MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M23DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1252, 28);

    KNOB(12 + KNOB_W * 0,  58, M24IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M24MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M24MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M24SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M24DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M24RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M24MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M24DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1652, 28);

    KNOB(12 + KNOB_W * 0,  58, M25IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M25MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M25MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M25SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M25DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M25RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M25MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M25DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(52, 586);

    KNOB(12 + KNOB_W * 0,  58, M26IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M26MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M26MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M26SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M26DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M26RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M26MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M26DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(452, 586);

    KNOB(12 + KNOB_W * 0,  58, M27IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M27MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M27MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M27SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M27DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M27RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M27MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M27DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(852, 586);

    KNOB(12 + KNOB_W * 0,  58, M28IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M28MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M28MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M28SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M28DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M28RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M28MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M28DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1252, 586);

    KNOB(12 + KNOB_W * 0,  58, M29IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M29MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M29MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M29SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M29DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M29RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M29MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M29DCV, macro_distortions);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1652, 586);

    KNOB(12 + KNOB_W * 0,  58, M30IN,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  58, M30MIN, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  58, M30MAX, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 298, M30SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1, 298, M30DST, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 298, M30RND, MM__C, "%.2f", 100.0, knob_states);

    MACMID(266, 6, M30MID, "%.2f%%", 100.0, macro_midpoint_states);
    DPEI(310, 6, 42, 42, 0, 42, M30DCV, macro_distortions);

    POSITION_RELATIVE_END();


    macros_3_body->hide();
}


void GUI::build_effects_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const knob_states_red,
        ParamStateImages const* const reversed_toggle_states
) {
    effects_body = new TabBody(*this, "Effects");

    TabBody* const knob_owner = effects_body;

    background->own(effects_body);

    effects_body->own(new ResizerHandle(*this));

    constexpr char const* const* ft = JS80P::GUI::BIQUAD_FILTER_TYPES;
    constexpr int ftc = JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT;

    constexpr char const* const* ct = JS80P::GUI::CHORUS_TYPES;
    constexpr int ctc = JS80P::GUI::CHORUS_TYPES_COUNT;

    constexpr char const* const* rt = JS80P::GUI::REVERB_TYPES;
    constexpr int rtc = JS80P::GUI::REVERB_TYPES_COUNT;

    constexpr char const* const* dt = JS80P::GUI::DISTORTION_TYPES;
    constexpr int dtc = JS80P::GUI::DISTORTION_TYPES_COUNT;

    constexpr char const* const* cm = JS80P::GUI::COMPRESSION_MODES;
    constexpr int cmc = JS80P::GUI::COMPRESSION_MODES_COUNT;


    POSITION_RELATIVE_BEGIN(69, 7);

    KNOB(18 + KNOB_W * 0, 61, INVOL, MML_C, "%.2f", 100.0, knob_states);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(255, 7);

    KNOB(18 + KNOB_W * 0, 61, EV1V, MML_C, "%.2f", 100.0, knob_states);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(440, 7);

    KNOB(18 + KNOB_W * 0, 61, ED1L, MML_C, "%.2f", 100.0, knob_states);
    DPET(16, 6, 120, 42, 0, 120, ED1TYP, dt, dtc);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(626, 7);

    KNOB(18 + KNOB_W * 0, 61, ED2L, MML_C, "%.2f", 100.0, knob_states);
    DPET(16, 6, 120, 42, 0, 120, ED2TYP, dt, dtc);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(812, 7);

    KNOB(12 + KNOB_W * 0, 61, EF1TYP, MM___, ft, ftc, knob_states);
    KNOB(12 + KNOB_W * 1, 61, EF1FRQ, MML_C, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 61, EF1Q,   MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 61, EF1G,   MML_C, "%.2f", 1.0, knob_states);
    TOGG(135, 9, 106, 48, 0, EF1LOG);
    TOGG(251, 9, 106, 48, 0, EF1QLG);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1333, 7);

    KNOB(12 + KNOB_W * 0, 61, EF2TYP, MM___, ft, ftc, knob_states);
    KNOB(12 + KNOB_W * 1, 61, EF2FRQ, MML_C, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 61, EF2Q,   MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 61, EF2G,   MML_C, "%.2f", 1.0, knob_states);
    TOGG(135, 9, 106, 48, 0, EF2LOG);
    TOGG(251, 9, 106, 48, 0, EF2QLG);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1855, 7);

    KNOB(18 + KNOB_W * 0, 61, EV2V, MML_C, "%.2f", 100.0, knob_states);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(64, 285);

    KNOB(17 + KNOB_W * 0, 61, ETSTP, MM__C, "%.3f", 1.0, knob_states_red);
    KNOB(17 + KNOB_W * 1, 61, ETWFA, MM__C, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 2, 61, ETSAT, MML_C, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 3, 61, ETCLR, MM__C, "%.2f", 200.0, knob_states);
    DPET(249, 6, 120, 42, 0, 120, ETSTYP, dt, dtc);
    TOGG(388, 9, 100, 48, 56, ETEND);
    SCREW(107, 7, ETWFS, "%.2f%%", 100.0, screw_states);
    SCREW(147, 7, ETSTR, "%.2f%%", 200.0, screw_states);
    SCREW(187, 7, ETHSS, "%.2f%%", 800.0, screw_states);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(596, 285);

    constexpr Number ch_fb_scl = 100.0 * Constants::CHORUS_FEEDBACK_SCALE;

    KNOB(12 + KNOB_W * 0,  61, ECHPF, MML__, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1,  61, ECHPQ, MML__, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2,  61, ECTYP, MM___, ct, ctc, knob_states);
    KNOB(12 + KNOB_W * 3,  61, ECDEL, MML__, "%.4f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4,  61, ECFRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 5,  61, ECDPT, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 6,  61, ECDF,  MML__, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 7,  61, ECDG,  MML_C, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 8,  61, ECFB,  MML_C, "%.2f", ch_fb_scl, knob_states);
    KNOB(12 + KNOB_W * 9,  61, ECWID, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 10, 61, ECWET, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 11, 61, ECDRY, MML_C, "%.2f", 100.0, knob_states);
    TOGG( 166, 9, 100, 48,   0, ECLHQ);
    TOGG( 514, 9, 228, 48,   0, ECLLG);
    TOGG( 747, 9, 272, 48,   0, ECLOG);
    TOGG(1226, 9, 180, 48, 132, ECSYN);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(156, 565);

    KNOB(12 + KNOB_W * 0,  61, EEINV, MML__, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  61, EEHPF, MML__, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2,  61, EEHPQ, MML__, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3,  61, EEDEL, MML__, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4,  61, EEDST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5,  61, EEDF,  MML__, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 6,  61, EEDG,  MML_C, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 7,  61, EEFB,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 8,  61, EEWID, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 9,  61, EECTH, MM___, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 10, 61, EECAT, MM___, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 11, 61, EECRL, MM___, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 12, 61, EECR,  MM___, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 13, 61, EEWET, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 14, 61, EEDRY, MML_C, "%.2f", 100.0, knob_states);
    TOGG(282, 11, 100, 48, 0, EELHQ);
    TOGG(631, 11, 272, 48, 0, EELOG);
    TOGG(1574, 9, 180, 48, 132, EESYN);
    DPEI(121, 14, 36, 36, 0, 36, EER1, reversed_toggle_states);
    DPEI(157, 14, 36, 36, 0, 36, EER2, reversed_toggle_states);
    DPET(1220, 6, 120, 42, 0, 120, EECM, cm, cmc);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(63, 845);

    KNOB(12 + KNOB_W * 0,  61, ERHPF, MML__, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1,  61, ERHPQ, MML__, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2,  61, ERTYP, MM___, rt, rtc, knob_states);
    KNOB(12 + KNOB_W * 3,  61, ERRS,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  61, ERRR,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5,  61, ERDST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 6,  61, ERDF,  MML__, "%.1f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 7,  61, ERDG,  MML_C, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 8,  61, ERWID, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 9,  61, ERCTH, MM___, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 10, 61, ERCAT, MM___, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 11, 61, ERCRL, MM___, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 12, 61, ERCR,  MM___, "%.2f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 13, 61, ERWET, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 14, 61, ERDRY, MML_C, "%.2f", 100.0, knob_states);
    TOGG(167, 9, 100, 48, 0, ERLHQ);
    TOGG(745, 9, 272, 48, 0, ERLOG);
    DPET(1222, 6, 120, 42, 0, 120, ERCM, cm, cmc);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1861, 845);

    KNOB(18 + KNOB_W * 0, 61, EV3V, MML_C, "%.2f", 100.0, knob_states);

    POSITION_RELATIVE_END();


    effects_body->hide();
}


void GUI::build_envelopes_1_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const screw_states,
        ParamStateImages const* const envelope_shapes_01,
        ParamStateImages const* const envelope_shapes_10
) {
    envelopes_1_body = new TabBody(*this, "Envelopes");

    TabBody* const knob_owner = envelopes_1_body;

    background->own(envelopes_1_body);

    envelopes_1_body->own(new ResizerHandle(*this));

    constexpr char const* const* ut = JS80P::GUI::ENVELOPE_UPDATE_TYPES;
    constexpr int utc = JS80P::GUI::ENVELOPE_UPDATE_TYPES_COUNT;


    POSITION_RELATIVE_BEGIN(72, 23);

    KNOB(12 + KNOB_W * 0,  66, N1SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N1INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N1PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N1SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N1FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N1DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N1ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N1HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N1DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N1REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N1TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N1VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N1UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N1SYN);

    DPEI(126, 14, 42, 42, 0, 42, N1ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N1DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N1RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(736, 23);

    KNOB(12 + KNOB_W * 0,  66, N2SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N2INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N2PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N2SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N2FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N2DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N2ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N2HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N2DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N2REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N2TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N2VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N2UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N2SYN);

    DPEI(126, 14, 42, 42, 0, 42, N2ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N2DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N2RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1400, 23);

    KNOB(12 + KNOB_W * 0,  66, N3SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N3INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N3PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N3SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N3FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N3DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N3ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N3HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N3DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N3REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N3TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N3VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N3UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N3SYN);

    DPEI(126, 14, 42, 42, 0, 42, N3ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N3DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N3RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(72, 582);

    KNOB(12 + KNOB_W * 0,  66, N4SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N4INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N4PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N4SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N4FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N4DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N4ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N4HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N4DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N4REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N4TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N4VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N4UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N4SYN);

    DPEI(126, 14, 42, 42, 0, 42, N4ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N4DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N4RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(736, 582);

    KNOB(12 + KNOB_W * 0,  66, N5SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N5INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N5PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N5SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N5FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N5DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N5ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N5HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N5DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N5REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N5TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N5VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N5UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N5SYN);

    DPEI(126, 14, 42, 42, 0, 42, N5ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N5DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N5RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1400, 582);

    KNOB(12 + KNOB_W * 0,  66, N6SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N6INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N6PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N6SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N6FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N6DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N6ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N6HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N6DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N6REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N6TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N6VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N6UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N6SYN);

    DPEI(126, 14, 42, 42, 0, 42, N6ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N6DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N6RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    envelopes_1_body->hide();
}


void GUI::build_envelopes_2_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const screw_states,
        ParamStateImages const* const envelope_shapes_01,
        ParamStateImages const* const envelope_shapes_10
) {
    envelopes_2_body = new TabBody(*this, "Envelopes");

    TabBody* const knob_owner = envelopes_2_body;

    background->own(envelopes_2_body);

    envelopes_2_body->own(new ResizerHandle(*this));

    constexpr char const* const* ut = JS80P::GUI::ENVELOPE_UPDATE_TYPES;
    constexpr int utc = JS80P::GUI::ENVELOPE_UPDATE_TYPES_COUNT;


    POSITION_RELATIVE_BEGIN(72, 23);

    KNOB(12 + KNOB_W * 0,  66, N7SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N7INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N7PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N7SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N7FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N7DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N7ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N7HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N7DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N7REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N7TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N7VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N7UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N7SYN);

    DPEI(126, 14, 42, 42, 0, 42, N7ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N7DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N7RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(736, 23);

    KNOB(12 + KNOB_W * 0,  66, N8SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N8INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N8PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N8SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N8FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N8DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N8ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N8HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N8DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N8REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N8TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N8VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N8UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N8SYN);

    DPEI(126, 14, 42, 42, 0, 42, N8ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N8DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N8RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1400, 23);

    KNOB(12 + KNOB_W * 0,  66, N9SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N9INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N9PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N9SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N9FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N9DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N9ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N9HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N9DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N9REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N9TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N9VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N9UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N9SYN);

    DPEI(126, 14, 42, 42, 0, 42, N9ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N9DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N9RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(72, 582);

    KNOB(12 + KNOB_W * 0,  66, N10SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N10INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N10PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N10SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N10FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N10DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N10ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N10HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N10DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N10REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N10TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N10VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N10UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N10SYN);

    DPEI(126, 14, 42, 42, 0, 42, N10ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N10DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N10RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(736, 582);

    KNOB(12 + KNOB_W * 0,  66, N11SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N11INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N11PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N11SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N11FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N11DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N11ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N11HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N11DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N11REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N11TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N11VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N11UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N11SYN);

    DPEI(126, 14, 42, 42, 0, 42, N11ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N11DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N11RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1400, 582);

    KNOB(12 + KNOB_W * 0,  66, N12SCL, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 1,  66, N12INI, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2,  66, N12PK,  MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 3,  66, N12SUS, MM__C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4,  66, N12FIN, MM__C, "%.2f", 100.0, knob_states);

    KNOB(12 + KNOB_W * 0, 306, N12DEL, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 1, 306, N12ATK, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 306, N12HLD, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 306, N12DEC, MM__C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 4, 306, N12REL, MM__C, "%.3f", 1.0, knob_states);

    SCREW(504, 13, N12TIN, "%.2f%%", 100.0, screw_states);
    SCREW(544, 13, N12VIN, "%.2f%%", 100.0, screw_states);

    DPET(286, 14, 96, 42, 0, 96, N12UPD, ut, utc);
    TOGG(396, 12, 92, 48, 50, N12SYN);

    DPEI(126, 14, 42, 42, 0, 42, N12ASH, envelope_shapes_01);
    DPEI(176, 14, 42, 42, 0, 42, N12DSH, envelope_shapes_10);
    DPEI(226, 14, 42, 42, 0, 42, N12RSH, envelope_shapes_10);

    POSITION_RELATIVE_END();


    envelopes_2_body->hide();
}


void GUI::build_lfos_body(ParamStateImages const* const knob_states)
{
    lfos_body = new TabBody(*this, "LFOs");

    TabBody* const knob_owner = lfos_body;

    background->own(lfos_body);

    lfos_body->own(new ResizerHandle(*this));

    constexpr char const* const* wf = JS80P::GUI::WAVEFORMS;
    constexpr int wfc = JS80P::GUI::WAVEFORMS_COUNT;

    constexpr char const* const* ae = JS80P::GUI::LFO_AMPLITUDE_ENVELOPES;
    constexpr int aec = JS80P::GUI::LFO_AMPLITUDE_ENVELOPES_COUNT;


    POSITION_RELATIVE_BEGIN(24, 8);

    KNOB(12 + KNOB_W * 0, 56, L1WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L1PW,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L1FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L1PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L1MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L1MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L1AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L1DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 8, 56, L1RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(247, 7, 152, 48, 0, L1LOG);
    TOGG(484, 7, 150, 48, 102, L1CEN);
    TOGG(878, 7, 180, 48, 132, L1SYN);
    DPET(656, 7, 180, 42, 124, 50, L1AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1100, 8);

    KNOB(12 + KNOB_W * 0, 56, L2WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L2FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L2PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L2MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L2MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L2AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L2DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L2RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(131, 7, 152, 48, 0, L2LOG);
    TOGG(368, 7, 150, 48, 102, L2CEN);
    TOGG(762, 7, 180, 48, 132, L2SYN);
    DPET(540, 7, 180, 42, 124, 50, L2AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(24, 288);

    KNOB(12 + KNOB_W * 0, 56, L3WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L3PW,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L3FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L3PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L3MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L3MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L3AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L3DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 8, 56, L3RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(247, 7, 152, 48, 0, L3LOG);
    TOGG(484, 7, 150, 48, 102, L3CEN);
    TOGG(878, 7, 180, 48, 132, L3SYN);
    DPET(656, 7, 180, 42, 124, 50, L3AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1100, 288);

    KNOB(12 + KNOB_W * 0, 56, L4WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L4FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L4PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L4MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L4MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L4AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L4DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L4RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(131, 7, 152, 48, 0, L4LOG);
    TOGG(368, 7, 150, 48, 102, L4CEN);
    TOGG(762, 7, 180, 48, 132, L4SYN);
    DPET(540, 7, 180, 42, 124, 50, L4AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(24, 568);

    KNOB(12 + KNOB_W * 0, 56, L5WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L5PW,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L5FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L5PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L5MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L5MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L5AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L5DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 8, 56, L5RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(247, 7, 152, 48, 0, L5LOG);
    TOGG(484, 7, 150, 48, 102, L5CEN);
    TOGG(878, 7, 180, 48, 132, L5SYN);
    DPET(656, 7, 180, 42, 124, 50, L5AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1100, 568);

    KNOB(12 + KNOB_W * 0, 56, L6WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L6FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L6PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L6MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L6MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L6AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L6DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L6RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(131, 7, 152, 48, 0, L6LOG);
    TOGG(368, 7, 150, 48, 102, L6CEN);
    TOGG(762, 7, 180, 48, 132, L6SYN);
    DPET(540, 7, 180, 42, 124, 50, L6AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(24, 848);

    KNOB(12 + KNOB_W * 0, 56, L7WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L7PW,  MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L7FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L7PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L7MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L7MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L7AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L7DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 8, 56, L7RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(247, 7, 152, 48, 0, L7LOG);
    TOGG(484, 7, 150, 48, 102, L7CEN);
    TOGG(878, 7, 180, 48, 132, L7SYN);
    DPET(656, 7, 180, 42, 124, 50, L7AEN, ae, aec);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(1100, 848);

    KNOB(12 + KNOB_W * 0, 56, L8WAV, MM___, wf, wfc, knob_states);
    KNOB(12 + KNOB_W * 1, 56, L8FRQ, MML_C, "%.3f", 1.0, knob_states);
    KNOB(12 + KNOB_W * 2, 56, L8PHS, MML_C, "%.1f", 360.0, knob_states);
    KNOB(12 + KNOB_W * 3, 56, L8MIN, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 4, 56, L8MAX, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 5, 56, L8AMP, MML_C, "%.2f", 200.0, knob_states);
    KNOB(12 + KNOB_W * 6, 56, L8DST, MML_C, "%.2f", 100.0, knob_states);
    KNOB(12 + KNOB_W * 7, 56, L8RND, MML_C, "%.2f", 100.0, knob_states);
    TOGG(131, 7, 152, 48, 0, L8LOG);
    TOGG(368, 7, 150, 48, 102, L8CEN);
    TOGG(762, 7, 180, 48, 132, L8SYN);
    DPET(540, 7, 180, 42, 124, 50, L8AEN, ae, aec);

    POSITION_RELATIVE_END();


    lfos_body->hide();
}


void GUI::build_synth_body(
        ParamStateImages const* const knob_states,
        ParamStateImages const* const screw_states
) {
    synth_body = new TabBody(*this, "Synth");

    TabBody* const knob_owner = synth_body;

    background->own(synth_body);

    synth_body->own(new ResizerHandle(*this));

    constexpr char const* const* nh = JS80P::GUI::NOTE_HANDLING_MODES;
    constexpr int nhc = JS80P::GUI::NOTE_HANDLING_MODES_COUNT;

    constexpr char const* const* md = JS80P::GUI::MODES;
    constexpr int mdc = JS80P::GUI::MODES_COUNT;

    constexpr char const* const* oia = JS80P::GUI::OSCILLATOR_INACCURACY_LEVELS;
    constexpr int oiac = JS80P::GUI::OSCILLATOR_INACCURACY_LEVELS_COUNT;

    constexpr char const* const* mpe = JS80P::GUI::MPE_SETTINGS;
    constexpr int mpec = JS80P::GUI::MPE_SETTINGS_COUNT;

    constexpr char const* const* dt = JS80P::GUI::DISTORTION_TYPES;
    constexpr int dtc = JS80P::GUI::DISTORTION_TYPES_COUNT;

    constexpr char const* const* ft = JS80P::GUI::BIQUAD_FILTER_TYPES;
    constexpr int ftc = JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT;

    constexpr char const* const* wf = JS80P::GUI::WAVEFORMS;
    constexpr int wfc = JS80P::GUI::WAVEFORMS_COUNT;

    constexpr Number fld_scale = 100.0 / Constants::FOLD_MAX;
    constexpr Number dtn_scale = Constants::DETUNE_SCALE;

    POSITION_RELATIVE_BEGIN(14, 4);

    synth_body->own(
        new ImportPatchButton(
            *this,
            pos_rel_offset_left + 4,
            pos_rel_offset_top + 5,
            45,
            45,
            synth,
            synth_body
        )
    );
    synth_body->own(
        new RandomizePatchButton(
            *this,
            pos_rel_offset_left + 49,
            pos_rel_offset_top + 5,
            45,
            45,
            synth,
            synth_body
        )
    );
    synth_body->own(
        new ExportPatchButton(
            *this,
            pos_rel_offset_left + 94,
            pos_rel_offset_top + 5,
            45,
            45,
            synth
        )
    );

    DPET(12, 60, 116, 38, 0, 116, NH, nh, nhc);

    constexpr Number pm_scale = 100.0 / Constants::PM_MAX;
    constexpr Number fm_scale = 100.0 / Constants::FM_MAX;
    constexpr Number am_scale = 100.0 / Constants::AM_MAX;

    KNOB(12, 98 + (KNOB_H + 2) * 0, MODE, MM___, md, mdc, knob_states);
    KNOB(12, 98 + (KNOB_H + 2) * 1, MIX,  MMLEC, "%.2f", 100.0, knob_states);
    KNOB(12, 98 + (KNOB_H + 2) * 2, PM,   MMLEC, "%.2f", pm_scale, knob_states);
    KNOB(12, 98 + (KNOB_H + 2) * 3, FM,   MMLEC, "%.2f", fm_scale, knob_states);
    KNOB(12, 98 + (KNOB_H + 2) * 4, AM,   MMLEC, "%.2f", am_scale, knob_states);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(158, 6);

    constexpr Synth::ParamId COIA = Synth::ParamId::COIA;
    constexpr Synth::ParamId COIS = Synth::ParamId::COIS;

    synth_body->own(
        new TuningSelector(
            *this,
            GUI::PARAMS[Synth::ParamId::MTUN],
            pos_rel_offset_left + 197,
            pos_rel_offset_top + 6,
            synth,
            Synth::ParamId::MTUN
        )
    );
    SCREW(423, 9, MOIA, oia, oiac, screw_states)->set_sync_param_id(COIA);
    SCREW(463, 9, MOIS, oia, oiac, screw_states)->set_sync_param_id(COIS);
    DPET(574, 6, 108, 46, 0, 108, MPEST, mpe, mpec);
    TOGG(1218, 5, 126, 48, 84, MFX4);

    TOGG(1551, 17, 106, 48, 0, MF1LOG);
    TOGG(1665, 17, 106, 48, 0, MF1QLG);
    SCREW(1787, 15, MF1FIA, "%.2f%%", 100.0, screw_states);
    SCREW(1827, 15, MF1QIA, "%.2f%%", 250.0, screw_states);

    constexpr Synth::ParamId MFX4 = Synth::ParamId::MFX4;

    KNOB(  17 + KNOB_W * 0,  61, MPRT,   MM___, "%.3f", 1.0, knob_states);
    KNOB(  17 + KNOB_W * 1,  61, MPRD,   MM___, "%.2f", 1.0, knob_states);
    KNOB(  17 + KNOB_W * 2,  61, MDTN,   MM__C, "%.f", dtn_scale, knob_states);
    KNOB4( 17 + KNOB_W * 3,  61, MFIN,   MMLEC, "%.2f", 1.0, knob_states, MFX4);
    KNOB(  17 + KNOB_W * 4,  61, MN,     MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 5,  61, MAMP,   MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 6,  61, MSUB,   MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 7,  61, MFLD,   MMLEC, "%.2f", fld_scale, knob_states);
    KNOB(  17 + KNOB_W * 8,  61, MVS,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 9,  61, MVOL,   MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 10, 61, MWID,   MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 11, 61, MPAN,   MMLEC, "%.2f", 100.0, knob_states);

    KNOB(1422 + KNOB_W * 0,  61, MF1TYP, MM___, ft, ftc, knob_states);
    KNOB(1422 + KNOB_W * 1,  61, MF1FRQ, MMLEC, "%.1f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 2,  61, MF1Q,   MMLEC, "%.3f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 3,  61, MF1G,   MMLEC, "%.2f", 1.0, knob_states);

    TOGG(1551, 280, 106, 48, 0, MF2LOG);
    TOGG(1665, 280, 106, 48, 0, MF2QLG);
    SCREW(1787, 278, MF2FIA, "%.2f%%", 100.0, screw_states);
    SCREW(1827, 278, MF2QIA, "%.2f%%", 250.0, screw_states);

    KNOB(  17 + KNOB_W * 0,  324, MWFM,   MM___, wf, wfc, knob_states);
    KNOB(  17 + KNOB_W * 1,  324, MPW,    MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 2,  324, MC1,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 3,  324, MC2,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 4,  324, MC3,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 5,  324, MC4,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 6,  324, MC5,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 7,  324, MC6,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 8,  324, MC7,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 9,  324, MC8,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 10, 324, MC9,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 11, 324, MC10,   MM___, "%.2f", 100.0, knob_states);

    KNOB(1422 + KNOB_W * 0,  324, MF2TYP, MM___, ft, ftc, knob_states);
    KNOB(1422 + KNOB_W * 1,  324, MF2FRQ, MMLEC, "%.1f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 2,  324, MF2Q,   MMLEC, "%.3f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 3,  324, MF2G,   MMLEC, "%.2f", 1.0, knob_states);

    POSITION_RELATIVE_END();


    POSITION_RELATIVE_BEGIN(158, 568);

    constexpr Synth::ParamId MOIA = Synth::ParamId::MOIA;
    constexpr Synth::ParamId MOIS = Synth::ParamId::MOIS;

    synth_body->own(
        new TuningSelector(
            *this,
            GUI::PARAMS[Synth::ParamId::CTUN],
            pos_rel_offset_left + 197,
            pos_rel_offset_top + 6,
            synth,
            Synth::ParamId::CTUN
        )
    );
    SCREW(423, 9, COIA, oia, oiac, screw_states)->set_sync_param_id(MOIA);
    SCREW(463, 9, COIS, oia, oiac, screw_states)->set_sync_param_id(MOIS);
    DPET(796, 9, 120, 42, 0, 120, CDTYP, dt, dtc);
    TOGG(1218, 5, 126, 48, 84, CFX4);

    TOGG(1551, 20, 106, 48, 0, CF1LOG);
    TOGG(1665, 20, 106, 48, 0, CF1QLG);
    SCREW(1787, 16, CF1FIA, "%.2f%%", 100.0, screw_states);
    SCREW(1827, 16, CF1QIA, "%.2f%%", 250.0, screw_states);

    constexpr Synth::ParamId CFX4 = Synth::ParamId::CFX4;

    KNOB(  17 + KNOB_W * 0,  61, CPRT,   MM___, "%.3f", 1.0, knob_states);
    KNOB(  17 + KNOB_W * 1,  61, CPRD,   MM___, "%.2f", 1.0, knob_states);
    KNOB(  17 + KNOB_W * 2,  61, CDTN,   MM__C, "%.f", 0.01, knob_states);
    KNOB4( 17 + KNOB_W * 3,  61, CFIN,   MMLEC, "%.2f", 1.0, knob_states, CFX4);
    KNOB(  17 + KNOB_W * 4,  61, CN,     MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 5,  61, CAMP,   MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 6,  61, CFLD,   MMLEC, "%.2f", fld_scale, knob_states);
    KNOB(  17 + KNOB_W * 7,  61, CDL,    MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 8,  61, CVS,    MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 9,  61, CVOL,   MMLEC, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 10, 61, CWID,   MM___, "%.2f", 100.0, knob_states);
    KNOB(  17 + KNOB_W * 11, 61, CPAN,   MMLEC, "%.2f", 100.0, knob_states);

    KNOB(1422 + KNOB_W * 0,  61, CF1TYP, MM___, ft, ftc, knob_states);
    KNOB(1422 + KNOB_W * 1,  61, CF1FRQ, MMLEC, "%.1f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 2,  61, CF1Q,   MMLEC, "%.3f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 3,  61, CF1G,   MMLEC, "%.2f", 1.0, knob_states);

    TOGG(1551, 283, 106, 48, 0, CF2LOG);
    TOGG(1665, 283, 106, 48, 0, CF2QLG);
    SCREW(1787, 279, CF2FIA, "%.2f%%", 100.0, screw_states);
    SCREW(1827, 279, CF2QIA, "%.2f%%", 250.0, screw_states);

    KNOB(17 + KNOB_W * 0,   324, CWFM,   MM___, wf, wfc, knob_states);
    KNOB(17 + KNOB_W * 1,   324, CPW,    MMLEC, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 2,   324, CC1,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 3,   324, CC2,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 4,   324, CC3,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 5,   324, CC4,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 6,   324, CC5,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 7,   324, CC6,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 8,   324, CC7,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 9,   324, CC8,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 10,  324, CC9,    MM___, "%.2f", 100.0, knob_states);
    KNOB(17 + KNOB_W * 11,  324, CC10,   MM___, "%.2f", 100.0, knob_states);

    KNOB(1422 + KNOB_W * 0, 324, CF2TYP, MM___, ft, ftc, knob_states);
    KNOB(1422 + KNOB_W * 1, 324, CF2FRQ, MMLEC, "%.1f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 2, 324, CF2Q,   MMLEC, "%.3f", 1.0, knob_states);
    KNOB(1422 + KNOB_W * 3, 324, CF2G,   MMLEC, "%.2f", 1.0, knob_states);

    POSITION_RELATIVE_END();


    synth_body->show();
}


GUI::~GUI()
{
    delete parent_window;

    delete knob_states;
    delete knob_states_red;
    delete screw_states;
    delete envelope_shapes_01;
    delete envelope_shapes_10;
    delete macro_distortions;
    delete macro_midpoint_states;
    delete reversed_toggle_states;

    dummy_widget->delete_image(vst_logo_image);

    delete dummy_widget;

    dummy_widget = NULL;

    destroy();
}


void GUI::show()
{
    background->show();
}


void GUI::resize(int const new_width, int const new_height)
{
    Number new_scale;
    int constrained_new_width = new_width;
    int constrained_new_height = new_height;

    apply_size_constraints(
        constrained_new_width, constrained_new_height, new_scale
    );

    if (constrained_new_width == width && constrained_new_height == height) {
        return;
    }

    if (JS80P_UNLIKELY(dummy_widget == NULL)) {
        return;
    }

    scale = new_scale;
    width = this->new_width = constrained_new_width;
    height = this->new_height = constrained_new_height;

    dummy_widget->set_scale(scale);

    knob_states->set_scale(scale);
    knob_states_red->set_scale(scale);
    screw_states->set_scale(scale);
    envelope_shapes_01->set_scale(scale);
    envelope_shapes_10->set_scale(scale);
    macro_distortions->set_scale(scale);
    macro_midpoint_states->set_scale(scale);
    reversed_toggle_states->set_scale(scale);

    background->set_scale(scale);
    background->redraw();

    prev_resize_time_ms = dummy_widget->monotonic_clock_ms();
}


void GUI::schedule_resize(int const new_width, int const new_height)
{
    Number new_scale;
    int constrained_new_width = new_width;
    int constrained_new_height = new_height;

    apply_size_constraints(
        constrained_new_width, constrained_new_height, new_scale
    );

    this->new_width = constrained_new_width;
    this->new_height = constrained_new_height;
}


void GUI::handle_scheduled_resize()
{
    if (JS80P_LIKELY(new_width == width && new_height == height)) {
        return;
    }

    if (JS80P_UNLIKELY(dummy_widget == NULL)) {
        return;
    }

    uint64_t now = dummy_widget->monotonic_clock_ms();

    if ((now - prev_resize_time_ms) < SCHEDULED_RESIZE_WAIT) {
        return;
    }

    event_handler->handle_resize_request(new_width, new_height);
}


void GUI::start_resizing()
{
    TabBody* const active_tab_body = background->get_body();

    if (active_tab_body != NULL) {
        active_tab_body->hide_param_editors();
    }
}


void GUI::stop_resizing()
{
    TabBody* const active_tab_body = background->get_body();

    if (active_tab_body != NULL) {
        active_tab_body->show_param_editors();
    }
}


void GUI::apply_size_constraints(
        int& new_width,
        int& new_height,
        Number& new_scale
) const {
    if (!resizing_allowed) {
        new_scale = scale;
        new_width = width;
        new_height = height;

        return;
    }

    new_width = std::clamp(new_width, MIN_WIDTH, MAX_WIDTH);
    new_height = std::clamp(new_height, MIN_HEIGHT, MAX_HEIGHT);

    Number const width_scale = (Number)new_width / WIDTH_FLOAT;
    Number const height_scale = (Number)new_height / HEIGHT_FLOAT;

    if (width_scale <= height_scale) {
        new_scale = width_scale;
        new_height = (int)std::round(HEIGHT_FLOAT * new_scale);
    } else {
        new_scale = height_scale;
        new_width = (int)std::round(WIDTH_FLOAT * new_scale);
    }
}


void GUI::ignore_resizing()
{
    resizing_allowed = false;
}


int GUI::get_width() const
{
    return width;
}


int GUI::get_height() const
{
    return height;
}


void GUI::update_synth_state()
{
    constexpr Color tape_status_color = GUI::rgb(255, 184, 96);

    Integer const old_active_voices_count = active_voices_count;
    TapeParams::State const old_tape_state = tape_state;

    active_voices_count = synth.get_active_voices_count();
    tape_state = synth.get_tape_state();

    if (
            active_voices_count == old_active_voices_count
            && old_tape_state == tape_state
    ) {
        return;
    }

    if (
            (int)tape_state < TAPE_STATES_COUNT
            && TAPE_STATES[tape_state] != NULL
    ) {
        snprintf(
            default_status_line,
            DEFAULT_STATUS_LINE_MAX_LENGTH,
            "Tape: %s",
            TAPE_STATES[tape_state]
        );
        default_status_line[DEFAULT_STATUS_LINE_MAX_LENGTH - 1] = '\x00';
        default_status_line_color = tape_status_color;
    } else if (active_voices_count != 0) {
        snprintf(
            default_status_line,
            DEFAULT_STATUS_LINE_MAX_LENGTH,
            "Voices: %d / %d",
            (int)active_voices_count,
            (int)Synth::POLYPHONY
        );
        default_status_line[DEFAULT_STATUS_LINE_MAX_LENGTH - 1] = '\x00';
        default_status_line_color = TEXT_COLOR;
    } else {
        default_status_line[0] = '\x00';
        default_status_line_color = TEXT_COLOR;
    }

    if (status_line != NULL) {
        status_line->set_text(default_status_line);
        status_line->set_text_color(default_status_line_color);
        redraw_status_line();
    }
}


void GUI::set_status_line(char const* const text)
{
    if (text[0] == '\x00') {
        status_line->set_text(default_status_line);
        status_line->set_text_color(default_status_line_color);
    } else {
        status_line->set_text(text);
        status_line->set_text_color(TEXT_COLOR);
    }

    redraw_status_line();
}


void GUI::redraw_status_line()
{
    status_line->redraw();
}


bool GUI::is_mts_esp_connected() const
{
    return synth.is_mts_esp_connected();
}


GUI::PlatformData GUI::get_platform_data() const
{
    return platform_data;
}


GUI::DefaultEventHandler::DefaultEventHandler(GUI& gui) : gui(gui)
{
}


void GUI::DefaultEventHandler::handle_resize_request(
        int const new_width,
        int const new_height
) {
    gui.resize(new_width, new_height);
}


WidgetBase::WidgetBase(char const* const text)
    : type(Type::BACKGROUND),
    platform_widget(NULL),
    platform_data(NULL),
    image(NULL),
    gui(NULL),
    parent(NULL),
    text(text),
    image_id(0),
    scale(1.0),
    left(0),
    top(0),
    width(0),
    height(0),
    is_visible(true),
    is_clicking(false),
    scale_changed(true)
{
}


WidgetBase::WidgetBase(
        char const* const text,
        int const left,
        int const top,
        int const width,
        int const height,
        Type const type
) : type(type),
    platform_widget(NULL),
    platform_data(NULL),
    image(NULL),
    gui(NULL),
    parent(NULL),
    text(text),
    image_id(0),
    scale(1.0),
    left(left),
    top(top),
    width(width),
    height(height),
    is_visible(true),
    is_clicking(false),
    scale_changed(true)
{
}


WidgetBase::WidgetBase(
        GUI::PlatformData platform_data,
        GUI::PlatformWidget platform_widget,
        Type const type
) : type(type),
    platform_widget(platform_widget),
    platform_data(platform_data),
    image(NULL),
    gui(NULL),
    parent(NULL),
    text(""),
    image_id(0),
    scale(1.0),
    left(0),
    top(0),
    width(0),
    height(0),
    is_visible(true),
    is_clicking(false),
    scale_changed(true)
{
}


WidgetBase::~WidgetBase()
{
}


void WidgetBase::destroy_children()
{
    GUI::Widgets::iterator it;

    for (it = children.begin(); it != children.end(); ++it) {
        delete *it;
    }
}


int WidgetBase::get_left() const
{
    return left;
}


int WidgetBase::get_top() const
{
    return top;
}


int WidgetBase::get_width() const
{
    return width;
}


int WidgetBase::get_height() const
{
    return height;
}


void WidgetBase::set_text(char const* const text)
{
    this->text = text;
}


char const* WidgetBase::get_text() const
{
    return text;
}


WidgetBase* WidgetBase::get_parent() const
{
    return parent;
}


void WidgetBase::set_scale(Number const new_scale)
{
    scale = new_scale;
    scale_changed = true;

    if (is_visible) {
        update_children_scale_if_changed();
    }
}


void WidgetBase::update_children_scale_if_changed()
{
    if (!scale_changed) {
        return;
    }

    scale_changed = false;

    GUI::Widgets::iterator it;

    for (it = children.begin(); it != children.end(); ++it) {
        (*it)->set_scale(scale);
    }
}


int WidgetBase::scale_value(int const value) const
{
    return (int)std::round(scale * (Number)value);
}


GUI::Image WidgetBase::load_image(
        GUI::PlatformData platform_data,
        char const* const name
) {
    return NULL;
}


void WidgetBase::delete_image(GUI::Image image)
{
}


bool WidgetBase::is_on_screen() const
{
    WidgetBase const* widget = this;

    while (widget != NULL) {
        if (!widget->is_visible) {
            return false;
        }

        widget = widget->parent;
    }

    return true;
}


void WidgetBase::show()
{
    is_visible = true;
    update_children_scale_if_changed();
}


void WidgetBase::hide()
{
    is_visible = false;
}



void WidgetBase::focus()
{
}


void WidgetBase::bring_to_top()
{
}


void WidgetBase::redraw()
{
}


WidgetBase* WidgetBase::own(WidgetBase* widget)
{
    children.push_back(widget);
    widget->set_up(platform_data, (WidgetBase*)this);

    return widget;
}


GUI::Image WidgetBase::set_image(GUI::Image new_image)
{
    GUI::Image old_image = image;
    image = new_image;
    image_id = (image_id + 1) & 0x7fffffff;
    redraw();

    return old_image;
}


GUI::Image WidgetBase::get_image() const
{
    return image;
}


Integer WidgetBase::get_image_id() const
{
    return image_id;
}


GUI::PlatformWidget WidgetBase::get_platform_widget()
{
    return platform_widget;
}


void WidgetBase::click()
{
}


void WidgetBase::set_up(
        GUI::PlatformData platform_data,
        WidgetBase* const parent
) {
    this->platform_data = platform_data;
    this->parent = parent;
}


void WidgetBase::set_gui(GUI& gui)
{
    this->gui = &gui;
}


bool WidgetBase::paint()
{
    if (image == NULL || !is_on_screen()) {
        return false;
    }

    draw_image(
        image, 0, 0, this->scale_value(width), this->scale_value(height)
    );

    return true;
}


bool WidgetBase::double_click()
{
    return false;
}


bool WidgetBase::mouse_down(int const x, int const y)
{
    return false;
}


bool WidgetBase::mouse_up(int const x, int const y)
{
    return false;
}


bool WidgetBase::mouse_move(int const x, int const y, bool const modifier)
{
    return false;
}


bool WidgetBase::mouse_leave(int const x, int const y)
{
    return false;
}


bool WidgetBase::mouse_wheel(Number const delta, bool const modifier)
{
    return false;
}


uint64_t WidgetBase::monotonic_clock_ms()
{
    return 0;
}


void WidgetBase::fill_rectangle(
        int const left,
        int const top,
        int const width,
        int const height,
        GUI::Color const color
) {
}


void WidgetBase::draw_text(
        char const* const text,
        int const font_size_px,
        int const left,
        int const top,
        int const width,
        int const height,
        GUI::Color const color,
        GUI::Color const background,
        FontWeight const font_weight,
        int const padding,
        TextAlignment const alignment
) {
}


void WidgetBase::draw_image(
        GUI::Image image,
        int const left,
        int const top,
        int const width,
        int const height
) {
}


GUI::Image WidgetBase::copy_image_region(
        GUI::Image source,
        int const left,
        int const top,
        int const width,
        int const height
) {
    return NULL;
}


GUI::Image WidgetBase::downscale_image(
        GUI::Image source,
        int const old_width,
        int const old_height,
        int const new_width,
        int const new_height
) {
    JS80P_ASSERT(new_width <= old_width && new_height <= old_height);

    return NULL;
}

}

#endif
