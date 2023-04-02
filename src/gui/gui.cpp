/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
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

bool GUI::controllers_by_id_initialized = false;


char const* const GUI::MODES[] = {
    [Synth::MIX_AND_MOD] = "Mix&Mod",
    [Synth::SPLIT_AT_C3] = "Split C3",
    [Synth::SPLIT_AT_Db3] = "Split Db3",
    [Synth::SPLIT_AT_D3] = "Split D3",
    [Synth::SPLIT_AT_Eb3] = "Split Eb3",
    [Synth::SPLIT_AT_E3] = "Split E3",
    [Synth::SPLIT_AT_F3] = "Split F3",
    [Synth::SPLIT_AT_Gb3] = "Split Gb3",
    [Synth::SPLIT_AT_G3] = "Split G3",
    [Synth::SPLIT_AT_Ab3] = "Split Ab3",
    [Synth::SPLIT_AT_A3] = "Split A3",
    [Synth::SPLIT_AT_Bb3] = "Split Bb3",
    [Synth::SPLIT_AT_B3] = "Split B3",
    [Synth::SPLIT_AT_C4] = "Split C4",
};

int const GUI::MODES_COUNT = Synth::MODES;


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


GUI::Controller::Controller(
        int const index,
        Synth::ControllerId const id,
        char const* const long_name,
        char const* const short_name
) : long_name(long_name),
    short_name(short_name),
    index(index),
    id(id)
{
}


char const* const GUI::PARAMS[] = {
    [Synth::ParamId::MIX] = "Modulator Additive Volume (%)",
    [Synth::ParamId::PM] = "Phase Modulation (%)",
    [Synth::ParamId::FM] = "Frequency Modulation (%)",
    [Synth::ParamId::AM] = "Amplitude Modulation (%)",

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

    [Synth::ParamId::MF2FRQ] = "Modulator Filter 2 Frequency (Hz)",
    [Synth::ParamId::MF2Q] = "Modulator Filter 2 Q Factor",
    [Synth::ParamId::MF2G] = "Modulator Filter 2 Gain (dB)",

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

    [Synth::ParamId::CF2FRQ] = "Carrier Filter 2 Frequency (Hz)",
    [Synth::ParamId::CF2Q] = "Carrier Filter 2 Q Factor",
    [Synth::ParamId::CF2G] = "Carrier Filter 2 Gain (dB)",

    [Synth::ParamId::EOG] = "Effects Overdrive Gain (%)",

    [Synth::ParamId::EDG] = "Effects Distortaion Gain (%)",

    [Synth::ParamId::EF1FRQ] = "Effects Filter 1 Frequency (Hz)",
    [Synth::ParamId::EF1Q] = "Effects Filter 1 Q Factor",
    [Synth::ParamId::EF1G] = "Effects Filter 1 Gain (dB)",

    [Synth::ParamId::EF2FRQ] = "Effects Filter 2 Frequency (Hz)",
    [Synth::ParamId::EF2Q] = "Effects Filter 2 Q Factor",
    [Synth::ParamId::EF2G] = "Effects Filter 2 Gain (dB)",

    [Synth::ParamId::EEDEL] = "Effects Echo Delay (s)",
    [Synth::ParamId::EEFB] = "Effects Echo Feedback (%)",
    [Synth::ParamId::EEDF] = "Effects Echo Dampening Frequency (Hz)",
    [Synth::ParamId::EEDG] = "Effects Echo Dampening Gain (dB)",
    [Synth::ParamId::EEWID] = "Effects Echo Stereo Width (%)",
    [Synth::ParamId::EEHPF] = "Effects Echo Highpass Frequency (Hz)",
    [Synth::ParamId::EEWET] = "Effects Echo Wet Volume (%)",
    [Synth::ParamId::EEDRY] = "Effects Echo Dry Volume (%)",

    [Synth::ParamId::ERRS] = "Effects Reverb Room Size (%)",
    [Synth::ParamId::ERDF] = "Effects Reverb Dampening Frequency (Hz)",
    [Synth::ParamId::ERDG] = "Effects Reverb Dampening Gain (dB)",
    [Synth::ParamId::ERWID] = "Effects Reverb Stereo Width (%)",
    [Synth::ParamId::ERHPF] = "Effects Reverb Highpass Frequency (Hz)",
    [Synth::ParamId::ERWET] = "Effects Reverb Wet Volume (%)",
    [Synth::ParamId::ERDRY] = "Effects Reverb Dry Volume (%)",

    [Synth::ParamId::F1IN] = "Flexible Controller 1 Input (%)",
    [Synth::ParamId::F1MIN] = "Flexible Controller 1 Minimum Value (%)",
    [Synth::ParamId::F1MAX] = "Flexible Controller 1 Maximum Value (%)",
    [Synth::ParamId::F1AMT] = "Flexible Controller 1 Amount (%)",
    [Synth::ParamId::F1DST] = "Flexible Controller 1 Distortion (%)",
    [Synth::ParamId::F1RND] = "Flexible Controller 1 Randomness (%)",

    [Synth::ParamId::F2IN] = "Flexible Controller 2 Input (%)",
    [Synth::ParamId::F2MIN] = "Flexible Controller 2 Minimum Value (%)",
    [Synth::ParamId::F2MAX] = "Flexible Controller 2 Maximum Value (%)",
    [Synth::ParamId::F2AMT] = "Flexible Controller 2 Amount (%)",
    [Synth::ParamId::F2DST] = "Flexible Controller 2 Distortion (%)",
    [Synth::ParamId::F2RND] = "Flexible Controller 2 Randomness (%)",

    [Synth::ParamId::F3IN] = "Flexible Controller 3 Input (%)",
    [Synth::ParamId::F3MIN] = "Flexible Controller 3 Minimum Value (%)",
    [Synth::ParamId::F3MAX] = "Flexible Controller 3 Maximum Value (%)",
    [Synth::ParamId::F3AMT] = "Flexible Controller 3 Amount (%)",
    [Synth::ParamId::F3DST] = "Flexible Controller 3 Distortion (%)",
    [Synth::ParamId::F3RND] = "Flexible Controller 3 Randomness (%)",

    [Synth::ParamId::F4IN] = "Flexible Controller 4 Input (%)",
    [Synth::ParamId::F4MIN] = "Flexible Controller 4 Minimum Value (%)",
    [Synth::ParamId::F4MAX] = "Flexible Controller 4 Maximum Value (%)",
    [Synth::ParamId::F4AMT] = "Flexible Controller 4 Amount (%)",
    [Synth::ParamId::F4DST] = "Flexible Controller 4 Distortion (%)",
    [Synth::ParamId::F4RND] = "Flexible Controller 4 Randomness (%)",

    [Synth::ParamId::F5IN] = "Flexible Controller 5 Input (%)",
    [Synth::ParamId::F5MIN] = "Flexible Controller 5 Minimum Value (%)",
    [Synth::ParamId::F5MAX] = "Flexible Controller 5 Maximum Value (%)",
    [Synth::ParamId::F5AMT] = "Flexible Controller 5 Amount (%)",
    [Synth::ParamId::F5DST] = "Flexible Controller 5 Distortion (%)",
    [Synth::ParamId::F5RND] = "Flexible Controller 5 Randomness (%)",

    [Synth::ParamId::F6IN] = "Flexible Controller 6 Input (%)",
    [Synth::ParamId::F6MIN] = "Flexible Controller 6 Minimum Value (%)",
    [Synth::ParamId::F6MAX] = "Flexible Controller 6 Maximum Value (%)",
    [Synth::ParamId::F6AMT] = "Flexible Controller 6 Amount (%)",
    [Synth::ParamId::F6DST] = "Flexible Controller 6 Distortion (%)",
    [Synth::ParamId::F6RND] = "Flexible Controller 6 Randomness (%)",

    [Synth::ParamId::F7IN] = "Flexible Controller 7 Input (%)",
    [Synth::ParamId::F7MIN] = "Flexible Controller 7 Minimum Value (%)",
    [Synth::ParamId::F7MAX] = "Flexible Controller 7 Maximum Value (%)",
    [Synth::ParamId::F7AMT] = "Flexible Controller 7 Amount (%)",
    [Synth::ParamId::F7DST] = "Flexible Controller 7 Distortion (%)",
    [Synth::ParamId::F7RND] = "Flexible Controller 7 Randomness (%)",

    [Synth::ParamId::F8IN] = "Flexible Controller 8 Input (%)",
    [Synth::ParamId::F8MIN] = "Flexible Controller 8 Minimum Value (%)",
    [Synth::ParamId::F8MAX] = "Flexible Controller 8 Maximum Value (%)",
    [Synth::ParamId::F8AMT] = "Flexible Controller 8 Amount (%)",
    [Synth::ParamId::F8DST] = "Flexible Controller 8 Distortion (%)",
    [Synth::ParamId::F8RND] = "Flexible Controller 8 Randomness (%)",

    [Synth::ParamId::F9IN] = "Flexible Controller 9 Input (%)",
    [Synth::ParamId::F9MIN] = "Flexible Controller 9 Minimum Value (%)",
    [Synth::ParamId::F9MAX] = "Flexible Controller 9 Maximum Value (%)",
    [Synth::ParamId::F9AMT] = "Flexible Controller 9 Amount (%)",
    [Synth::ParamId::F9DST] = "Flexible Controller 9 Distortion (%)",
    [Synth::ParamId::F9RND] = "Flexible Controller 9 Randomness (%)",

    [Synth::ParamId::F10IN] = "Flexible Controller 10 Input (%)",
    [Synth::ParamId::F10MIN] = "Flexible Controller 10 Minimum Value (%)",
    [Synth::ParamId::F10MAX] = "Flexible Controller 10 Maximum Value (%)",
    [Synth::ParamId::F10AMT] = "Flexible Controller 10 Amount (%)",
    [Synth::ParamId::F10DST] = "Flexible Controller 10 Distortion (%)",
    [Synth::ParamId::F10RND] = "Flexible Controller 10 Randomness (%)",

    [Synth::ParamId::N1AMT] = "Envelope 1 Amount (%)",
    [Synth::ParamId::N1INI] = "Envelope 1 Initial Level (%)",
    [Synth::ParamId::N1DEL] = "Envelope 1 Delay Time (s)",
    [Synth::ParamId::N1ATK] = "Envelope 1 Attack Time (s)",
    [Synth::ParamId::N1PK] = "Envelope 1 Peak Level (%)",
    [Synth::ParamId::N1HLD] = "Envelope 1 Hold Time (s)",
    [Synth::ParamId::N1DEC] = "Envelope 1 Decay Time (s)",
    [Synth::ParamId::N1SUS] = "Envelope 1 Sustain Level (%)",
    [Synth::ParamId::N1REL] = "Envelope 1 Release Time (s)",
    [Synth::ParamId::N1FIN] = "Envelope 1 Final Level (%)",

    [Synth::ParamId::N2AMT] = "Envelope 2 Amount (%)",
    [Synth::ParamId::N2INI] = "Envelope 2 Initial Level (%)",
    [Synth::ParamId::N2DEL] = "Envelope 2 Delay Time (s)",
    [Synth::ParamId::N2ATK] = "Envelope 2 Attack Time (s)",
    [Synth::ParamId::N2PK] = "Envelope 2 Peak Level (%)",
    [Synth::ParamId::N2HLD] = "Envelope 2 Hold Time (s)",
    [Synth::ParamId::N2DEC] = "Envelope 2 Decay Time (s)",
    [Synth::ParamId::N2SUS] = "Envelope 2 Sustain Level (%)",
    [Synth::ParamId::N2REL] = "Envelope 2 Release Time (s)",
    [Synth::ParamId::N2FIN] = "Envelope 2 Final Level (%)",

    [Synth::ParamId::N3AMT] = "Envelope 3 Amount (%)",
    [Synth::ParamId::N3INI] = "Envelope 3 Initial Level (%)",
    [Synth::ParamId::N3DEL] = "Envelope 3 Delay Time (s)",
    [Synth::ParamId::N3ATK] = "Envelope 3 Attack Time (s)",
    [Synth::ParamId::N3PK] = "Envelope 3 Peak Level (%)",
    [Synth::ParamId::N3HLD] = "Envelope 3 Hold Time (s)",
    [Synth::ParamId::N3DEC] = "Envelope 3 Decay Time (s)",
    [Synth::ParamId::N3SUS] = "Envelope 3 Sustain Level (%)",
    [Synth::ParamId::N3REL] = "Envelope 3 Release Time (s)",
    [Synth::ParamId::N3FIN] = "Envelope 3 Final Level (%)",

    [Synth::ParamId::N4AMT] = "Envelope 4 Amount (%)",
    [Synth::ParamId::N4INI] = "Envelope 4 Initial Level (%)",
    [Synth::ParamId::N4DEL] = "Envelope 4 Delay Time (s)",
    [Synth::ParamId::N4ATK] = "Envelope 4 Attack Time (s)",
    [Synth::ParamId::N4PK] = "Envelope 4 Peak Level (%)",
    [Synth::ParamId::N4HLD] = "Envelope 4 Hold Time (s)",
    [Synth::ParamId::N4DEC] = "Envelope 4 Decay Time (s)",
    [Synth::ParamId::N4SUS] = "Envelope 4 Sustain Level (%)",
    [Synth::ParamId::N4REL] = "Envelope 4 Release Time (s)",
    [Synth::ParamId::N4FIN] = "Envelope 4 Final Level (%)",

    [Synth::ParamId::N5AMT] = "Envelope 5 Amount (%)",
    [Synth::ParamId::N5INI] = "Envelope 5 Initial Level (%)",
    [Synth::ParamId::N5DEL] = "Envelope 5 Delay Time (s)",
    [Synth::ParamId::N5ATK] = "Envelope 5 Attack Time (s)",
    [Synth::ParamId::N5PK] = "Envelope 5 Peak Level (%)",
    [Synth::ParamId::N5HLD] = "Envelope 5 Hold Time (s)",
    [Synth::ParamId::N5DEC] = "Envelope 5 Decay Time (s)",
    [Synth::ParamId::N5SUS] = "Envelope 5 Sustain Level (%)",
    [Synth::ParamId::N5REL] = "Envelope 5 Release Time (s)",
    [Synth::ParamId::N5FIN] = "Envelope 5 Final Level (%)",

    [Synth::ParamId::N6AMT] = "Envelope 6 Amount (%)",
    [Synth::ParamId::N6INI] = "Envelope 6 Initial Level (%)",
    [Synth::ParamId::N6DEL] = "Envelope 6 Delay Time (s)",
    [Synth::ParamId::N6ATK] = "Envelope 6 Attack Time (s)",
    [Synth::ParamId::N6PK] = "Envelope 6 Peak Level (%)",
    [Synth::ParamId::N6HLD] = "Envelope 6 Hold Time (s)",
    [Synth::ParamId::N6DEC] = "Envelope 6 Decay Time (s)",
    [Synth::ParamId::N6SUS] = "Envelope 6 Sustain Level (%)",
    [Synth::ParamId::N6REL] = "Envelope 6 Release Time (s)",
    [Synth::ParamId::N6FIN] = "Envelope 6 Final Level (%)",

    [Synth::ParamId::L1FRQ] = "LFO 1 Frequency (Hz)",
    [Synth::ParamId::L1PHS] = "LFO 1 Phase (degree)",
    [Synth::ParamId::L1MIN] = "LFO 1 Minimum Value (%)",
    [Synth::ParamId::L1MAX] = "LFO 1 Maximum Value (%)",
    [Synth::ParamId::L1AMT] = "LFO 1 Amount (%)",
    [Synth::ParamId::L1DST] = "LFO 1 Distortion (%)",
    [Synth::ParamId::L1RND] = "LFO 1 Randomness (%)",

    [Synth::ParamId::L2FRQ] = "LFO 2 Frequency (Hz)",
    [Synth::ParamId::L2PHS] = "LFO 2 Phase (degree)",
    [Synth::ParamId::L2MIN] = "LFO 2 Minimum Value (%)",
    [Synth::ParamId::L2MAX] = "LFO 2 Maximum Value (%)",
    [Synth::ParamId::L2AMT] = "LFO 2 Amount (%)",
    [Synth::ParamId::L2DST] = "LFO 2 Distortion (%)",
    [Synth::ParamId::L2RND] = "LFO 2 Randomness (%)",

    [Synth::ParamId::L3FRQ] = "LFO 3 Frequency (Hz)",
    [Synth::ParamId::L3PHS] = "LFO 3 Phase (degree)",
    [Synth::ParamId::L3MIN] = "LFO 3 Minimum Value (%)",
    [Synth::ParamId::L3MAX] = "LFO 3 Maximum Value (%)",
    [Synth::ParamId::L3AMT] = "LFO 3 Amount (%)",
    [Synth::ParamId::L3DST] = "LFO 3 Distortion (%)",
    [Synth::ParamId::L3RND] = "LFO 3 Randomness (%)",

    [Synth::ParamId::L4FRQ] = "LFO 4 Frequency (Hz)",
    [Synth::ParamId::L4PHS] = "LFO 4 Phase (degree)",
    [Synth::ParamId::L4MIN] = "LFO 4 Minimum Value (%)",
    [Synth::ParamId::L4MAX] = "LFO 4 Maximum Value (%)",
    [Synth::ParamId::L4AMT] = "LFO 4 Amount (%)",
    [Synth::ParamId::L4DST] = "LFO 4 Distortion (%)",
    [Synth::ParamId::L4RND] = "LFO 4 Randomness (%)",

    [Synth::ParamId::L5FRQ] = "LFO 5 Frequency (Hz)",
    [Synth::ParamId::L5PHS] = "LFO 5 Phase (degree)",
    [Synth::ParamId::L5MIN] = "LFO 5 Minimum Value (%)",
    [Synth::ParamId::L5MAX] = "LFO 5 Maximum Value (%)",
    [Synth::ParamId::L5AMT] = "LFO 5 Amount (%)",
    [Synth::ParamId::L5DST] = "LFO 5 Distortion (%)",
    [Synth::ParamId::L5RND] = "LFO 5 Randomness (%)",

    [Synth::ParamId::L6FRQ] = "LFO 6 Frequency (Hz)",
    [Synth::ParamId::L6PHS] = "LFO 6 Phase (degree)",
    [Synth::ParamId::L6MIN] = "LFO 6 Minimum Value (%)",
    [Synth::ParamId::L6MAX] = "LFO 6 Maximum Value (%)",
    [Synth::ParamId::L6AMT] = "LFO 6 Amount (%)",
    [Synth::ParamId::L6DST] = "LFO 6 Distortion (%)",
    [Synth::ParamId::L6RND] = "LFO 6 Randomness (%)",

    [Synth::ParamId::L7FRQ] = "LFO 7 Frequency (Hz)",
    [Synth::ParamId::L7PHS] = "LFO 7 Phase (degree)",
    [Synth::ParamId::L7MIN] = "LFO 7 Minimum Value (%)",
    [Synth::ParamId::L7MAX] = "LFO 7 Maximum Value (%)",
    [Synth::ParamId::L7AMT] = "LFO 7 Amount (%)",
    [Synth::ParamId::L7DST] = "LFO 7 Distortion (%)",
    [Synth::ParamId::L7RND] = "LFO 7 Randomness (%)",

    [Synth::ParamId::L8FRQ] = "LFO 8 Frequency (Hz)",
    [Synth::ParamId::L8PHS] = "LFO 8 Phase (degree)",
    [Synth::ParamId::L8MIN] = "LFO 8 Minimum Value (%)",
    [Synth::ParamId::L8MAX] = "LFO 8 Maximum Value (%)",
    [Synth::ParamId::L8AMT] = "LFO 8 Amount (%)",
    [Synth::ParamId::L8DST] = "LFO 8 Distortion (%)",
    [Synth::ParamId::L8RND] = "LFO 8 Randomness (%)",

    [Synth::ParamId::MODE] = "Operating Mode",

    [Synth::ParamId::MWAV] = "Modulator Waveform",
    [Synth::ParamId::CWAV] = "Carrier Waveform",

    [Synth::ParamId::MF1TYP] = "Modulator Filter 1 Type",
    [Synth::ParamId::MF2TYP] = "Modulator Filter 2 Type",

    [Synth::ParamId::CF1TYP] = "Carrier Filter 1 Type",
    [Synth::ParamId::CF2TYP] = "Carrier Filter 2 Type",

    [Synth::ParamId::EF1TYP] = "Effects Filter 1 Type",
    [Synth::ParamId::EF2TYP] = "Effects Filter 2 Type",

    [Synth::ParamId::L1WAV] = "LFO 1 Waveform",
    [Synth::ParamId::L2WAV] = "LFO 2 Waveform",
    [Synth::ParamId::L3WAV] = "LFO 3 Waveform",
    [Synth::ParamId::L4WAV] = "LFO 4 Waveform",
    [Synth::ParamId::L5WAV] = "LFO 5 Waveform",
    [Synth::ParamId::L6WAV] = "LFO 6 Waveform",
    [Synth::ParamId::L7WAV] = "LFO 7 Waveform",
    [Synth::ParamId::L8WAV] = "LFO 8 Waveform",
};


GUI::Controller const GUI::CONTROLLERS[] = {
    Controller(0, Synth::ControllerId::NONE, "(none)", "(none)"),

    Controller(1, Synth::ControllerId::NOTE, "Note", "Note"),
    Controller(2, Synth::ControllerId::VELOCITY, "Velocity", "Vel"),

    Controller(3, Synth::ControllerId::PITCH_WHEEL, "Pitch Wheel", "PtchWh"),

    Controller(4, Synth::ControllerId::MODULATION_WHEEL, "CC 1 (Modulation Wheel)", "ModWh"),
    Controller(5, Synth::ControllerId::BREATH, "CC 2 (Breath)", "Breath"),
    Controller(6, Synth::ControllerId::UNDEFINED_1, "CC 3", "CC 3"),
    Controller(7, Synth::ControllerId::FOOT_PEDAL, "CC 4 (Foot Pedal)", "Foot"),
    Controller(8, Synth::ControllerId::PORTAMENTO_TIME, "CC 5 (Portamento Time)", "PortT"),
    Controller(9, Synth::ControllerId::VOLUME, "CC 7 (Volume)", "Vol"),
    Controller(10, Synth::ControllerId::BALANCE, "CC 8 (Balance)", "Blnc"),
    Controller(11, Synth::ControllerId::UNDEFINED_2, "CC 9", "CC 9"),
    Controller(12, Synth::ControllerId::PAN, "CC 10 (Pan)", "Pan"),
    Controller(13, Synth::ControllerId::EXPRESSION_PEDAL, "CC 11 (Expression Pedal)", "Expr"),
    Controller(14, Synth::ControllerId::FX_CTL_1, "CC 12 (Effect Control 1)", "Fx C 1"),
    Controller(15, Synth::ControllerId::FX_CTL_2, "CC 13 (Effect Control 2)", "Fx C 2"),
    Controller(16, Synth::ControllerId::UNDEFINED_3, "CC 14", "CC 14"),
    Controller(17, Synth::ControllerId::UNDEFINED_4, "CC 15", "CC 15"),
    Controller(18, Synth::ControllerId::GENERAL_1, "CC 16 (General)", "Gen 1"),
    Controller(19, Synth::ControllerId::GENERAL_2, "CC 17 (General)", "Gen 1"),
    Controller(20, Synth::ControllerId::GENERAL_3, "CC 18 (General)", "Gen 1"),
    Controller(21, Synth::ControllerId::GENERAL_4, "CC 19 (General)", "Gen 4"),
    Controller(22, Synth::ControllerId::UNDEFINED_5, "CC 20", "CC 20"),
    Controller(23, Synth::ControllerId::UNDEFINED_6, "CC 21", "CC 21"),
    Controller(24, Synth::ControllerId::UNDEFINED_7, "CC 22", "CC 22"),
    Controller(25, Synth::ControllerId::UNDEFINED_8, "CC 23", "CC 23"),
    Controller(26, Synth::ControllerId::UNDEFINED_9, "CC 24", "CC 24"),
    Controller(27, Synth::ControllerId::UNDEFINED_10, "CC 25", "CC 25"),
    Controller(28, Synth::ControllerId::UNDEFINED_11, "CC 26", "CC 26"),
    Controller(29, Synth::ControllerId::UNDEFINED_12, "CC 27", "CC 27"),
    Controller(30, Synth::ControllerId::UNDEFINED_13, "CC 28", "CC 28"),
    Controller(31, Synth::ControllerId::UNDEFINED_14, "CC 29", "CC 29"),
    Controller(32, Synth::ControllerId::UNDEFINED_15, "CC 30", "CC 30"),
    Controller(33, Synth::ControllerId::UNDEFINED_16, "CC 31", "CC 31"),
    Controller(34, Synth::ControllerId::PORTAMENTO_AMOUNT, "CC 84 (Portamento)", "Prtmnt"),
    Controller(35, Synth::ControllerId::SOUND_1, "CC 70 (Sound 1)", "Snd 1"),
    Controller(36, Synth::ControllerId::SOUND_2, "CC 71 (Sound 2)", "Snd 2"),
    Controller(37, Synth::ControllerId::SOUND_3, "CC 72 (Sound 3)", "Snd 3"),
    Controller(38, Synth::ControllerId::SOUND_4, "CC 73 (Sound 4)", "Snd 4"),
    Controller(39, Synth::ControllerId::SOUND_5, "CC 74 (Sound 5)", "Snd 5"),
    Controller(40, Synth::ControllerId::SOUND_6, "CC 75 (Sound 6)", "Snd 6"),
    Controller(41, Synth::ControllerId::SOUND_7, "CC 76 (Sound 7)", "Snd 7"),
    Controller(42, Synth::ControllerId::SOUND_8, "CC 77 (Sound 8)", "Snd 8"),
    Controller(43, Synth::ControllerId::SOUND_9, "CC 78 (Sound 9)", "Snd 9"),
    Controller(44, Synth::ControllerId::SOUND_10, "CC 79 (Sound 10)", "Snd 10"),
    Controller(45, Synth::ControllerId::UNDEFINED_17, "CC 85", "CC 85"),
    Controller(46, Synth::ControllerId::UNDEFINED_18, "CC 86", "CC 86"),
    Controller(47, Synth::ControllerId::UNDEFINED_19, "CC 87", "CC 87"),
    Controller(48, Synth::ControllerId::UNDEFINED_20, "CC 89", "CC 89"),
    Controller(49, Synth::ControllerId::UNDEFINED_21, "CC 90", "CC 90"),
    Controller(50, Synth::ControllerId::FX_1, "CC 91 (Effect 1)", "Fx 1"),
    Controller(51, Synth::ControllerId::FX_2, "CC 92 (Effect 2)", "Fx 2"),
    Controller(52, Synth::ControllerId::FX_3, "CC 93 (Effect 3)", "Fx 3"),
    Controller(53, Synth::ControllerId::FX_4, "CC 94 (Effect 4)", "Fx 4"),
    Controller(54, Synth::ControllerId::FX_5, "CC 95 (Effect 5)", "Fx 5"),
    Controller(55, Synth::ControllerId::UNDEFINED_22, "CC 102", "CC 102"),
    Controller(56, Synth::ControllerId::UNDEFINED_23, "CC 103", "CC 103"),
    Controller(57, Synth::ControllerId::UNDEFINED_24, "CC 104", "CC 104"),
    Controller(58, Synth::ControllerId::UNDEFINED_25, "CC 105", "CC 105"),
    Controller(59, Synth::ControllerId::UNDEFINED_26, "CC 106", "CC 106"),
    Controller(60, Synth::ControllerId::UNDEFINED_27, "CC 107", "CC 107"),
    Controller(61, Synth::ControllerId::UNDEFINED_28, "CC 108", "CC 108"),
    Controller(62, Synth::ControllerId::UNDEFINED_29, "CC 109", "CC 109"),
    Controller(63, Synth::ControllerId::UNDEFINED_30, "CC 110", "CC 110"),
    Controller(64, Synth::ControllerId::UNDEFINED_31, "CC 111", "CC 111"),
    Controller(65, Synth::ControllerId::UNDEFINED_32, "CC 112", "CC 112"),
    Controller(66, Synth::ControllerId::UNDEFINED_33, "CC 113", "CC 113"),
    Controller(67, Synth::ControllerId::UNDEFINED_34, "CC 114", "CC 114"),
    Controller(68, Synth::ControllerId::UNDEFINED_35, "CC 115", "CC 115"),
    Controller(69, Synth::ControllerId::UNDEFINED_36, "CC 116", "CC 116"),
    Controller(70, Synth::ControllerId::UNDEFINED_37, "CC 117", "CC 117"),
    Controller(71, Synth::ControllerId::UNDEFINED_38, "CC 118", "CC 118"),
    Controller(72, Synth::ControllerId::UNDEFINED_39, "CC 119", "CC 119"),

    Controller(73, Synth::ControllerId::FLEXIBLE_CONTROLLER_1, "Flexible Controller 1", "FC 1"),
    Controller(74, Synth::ControllerId::FLEXIBLE_CONTROLLER_2, "Flexible Controller 2", "FC 2"),
    Controller(75, Synth::ControllerId::FLEXIBLE_CONTROLLER_3, "Flexible Controller 3", "FC 3"),
    Controller(76, Synth::ControllerId::FLEXIBLE_CONTROLLER_4, "Flexible Controller 4", "FC 4"),
    Controller(77, Synth::ControllerId::FLEXIBLE_CONTROLLER_5, "Flexible Controller 5", "FC 5"),
    Controller(78, Synth::ControllerId::FLEXIBLE_CONTROLLER_6, "Flexible Controller 6", "FC 6"),
    Controller(79, Synth::ControllerId::FLEXIBLE_CONTROLLER_7, "Flexible Controller 7", "FC 7"),
    Controller(80, Synth::ControllerId::FLEXIBLE_CONTROLLER_8, "Flexible Controller 8", "FC 8"),
    Controller(81, Synth::ControllerId::FLEXIBLE_CONTROLLER_9, "Flexible Controller 9", "FC 9"),
    Controller(82, Synth::ControllerId::FLEXIBLE_CONTROLLER_10, "Flexible Controller 10", "FC 10"),

    Controller(83, Synth::ControllerId::LFO_1, "LFO 1", "LFO 1"),
    Controller(84, Synth::ControllerId::LFO_2, "LFO 2", "LFO 2"),
    Controller(85, Synth::ControllerId::LFO_3, "LFO 3", "LFO 3"),
    Controller(86, Synth::ControllerId::LFO_4, "LFO 4", "LFO 4"),
    Controller(87, Synth::ControllerId::LFO_5, "LFO 5", "LFO 5"),
    Controller(88, Synth::ControllerId::LFO_6, "LFO 6", "LFO 6"),
    Controller(89, Synth::ControllerId::LFO_7, "LFO 7", "LFO 7"),
    Controller(90, Synth::ControllerId::LFO_8, "LFO 8", "LFO 8"),

    Controller(91, Synth::ControllerId::ENVELOPE_1, "Envelope 1", "ENV 1"),
    Controller(92, Synth::ControllerId::ENVELOPE_2, "Envelope 2", "ENV 2"),
    Controller(93, Synth::ControllerId::ENVELOPE_3, "Envelope 3", "ENV 3"),
    Controller(94, Synth::ControllerId::ENVELOPE_4, "Envelope 4", "ENV 4"),
    Controller(95, Synth::ControllerId::ENVELOPE_5, "Envelope 5", "ENV 5"),
    Controller(96, Synth::ControllerId::ENVELOPE_6, "Envelope 6", "ENV 6"),
};


GUI::Controller const* GUI::controllers_by_id[Synth::ControllerId::MAX_CONTROLLER_ID];


GUI::Controller const* GUI::get_controller(Synth::ControllerId const controller_id)
{
    initialize_controllers_by_id();

    Controller const* const ctl = controllers_by_id[controller_id];

    return ctl != NULL ? ctl : &CONTROLLERS[0];
}


void GUI::initialize_controllers_by_id()
{
    if (controllers_by_id_initialized) {
        return;
    }

    int i;

    for (i = 0; i != Synth::ControllerId::MAX_CONTROLLER_ID; ++i) {
        controllers_by_id[i] = NULL;
    }

    for (i = 0; i != ALL_CTLS; ++i) {
        controllers_by_id[CONTROLLERS[i].id] = &CONTROLLERS[i];
    }

    controllers_by_id_initialized = true;
}


constexpr GUI::Color GUI::rgb(
        ColorComponent const red,
        ColorComponent const green,
        ColorComponent const blue
) {
    return (Color)(
        (unsigned int)red << 16 | (unsigned int)green << 8 | (unsigned int)blue
    );
}


const GUI::Color GUI::TEXT_COLOR = GUI::rgb(181, 181, 189);
const GUI::Color GUI::TEXT_BACKGROUND = GUI::rgb(0, 0, 0);
const GUI::Color GUI::TEXT_HIGHLIGHT_COLOR = GUI::rgb(225, 225, 235);
const GUI::Color GUI::TEXT_HIGHLIGHT_BACKGROUND = GUI::rgb(63, 63, 66);


constexpr GUI::ColorComponent GUI::red(Color const color)
{
    return color >> 16;
}


constexpr GUI::ColorComponent GUI::green(Color const color)
{
    return color >> 8;
}


constexpr GUI::ColorComponent GUI::blue(Color const color)
{
    return color;
}


void GUI::refresh_param_editors(ParamEditors param_editors)
{
    for (ParamEditors::iterator it = param_editors.begin(); it != param_editors.end(); ++it) {
        (*it)->refresh();
    }
}


void GUI::refresh_controlled_param_editors(ParamEditors param_editors)
{
    for (ParamEditors::iterator it = param_editors.begin(); it != param_editors.end(); ++it) {
        ParamEditor* editor = *it;

        if (editor->has_controller()) {
            editor->refresh();
        }
    }
}


void GUI::param_ratio_to_str(
        Synth& synth,
        Synth::ParamId const param_id,
        Number const ratio,
        Number const scale,
        char const* const format,
        char const* const* const options,
        int const number_of_options,
        char* const buffer,
        size_t const buffer_size
) {
    if (format != NULL) {
        param_ratio_to_str_float(
            synth, param_id, ratio, scale, format, buffer, buffer_size
        );
    } else if (options != NULL) {
        param_ratio_to_str_int(
            synth, param_id, ratio, options, number_of_options, buffer, buffer_size
        );
    }

    buffer[buffer_size - 1] = '\x00';
}


void GUI::param_ratio_to_str_float(
        Synth& synth,
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
}


void GUI::param_ratio_to_str_int(
        Synth& synth,
        Synth::ParamId const param_id,
        Number const ratio,
        char const* const* const options,
        int const number_of_options,
        char* const buffer,
        size_t const buffer_size
) {
    Byte const value = (
        synth.int_param_ratio_to_display_value(param_id, ratio)
    );

    if (((int)value >= number_of_options) || ((int)value < 0)) {
        buffer[0] = '\x00';

        return;
    }

    strncpy(buffer, options[value], buffer_size - 1);
}


Number GUI::clamp_ratio(Number const ratio)
{
    return std::min(1.0, std::max(0.0, ratio));
}


#define PE(owner, left, top, param_id, ctls, varg1, varg2)  \
    owner->own(                                             \
        new ParamEditor(                                    \
            GUI::PARAMS[param_id],                          \
            left,                                           \
            top,                                            \
            *controller_selector,                           \
            synth,                                          \
            param_id,                                       \
            ctls,                                           \
            varg1,                                          \
            varg2                                           \
        )                                                   \
    )

#define PE_W ParamEditor::WIDTH
#define PE_H ParamEditor::HEIGHT


GUI::GUI(
        JS80P::GUI::PlatformData platform_data,
        JS80P::GUI::PlatformWidget parent_window,
        Synth& synth
)
    : dummy_widget(NULL),
    background(NULL),
    about_body(NULL),
    controllers_body(NULL),
    effects_body(NULL),
    envelopes_body(NULL),
    lfos_body(NULL),
    synth_body(NULL),
    synth(synth),
    platform_data(platform_data)
{
    dummy_widget = new Widget("");

    ParamEditor::initialize_knob_states(
        dummy_widget,
        dummy_widget->load_bitmap(platform_data, "KNOBSTATES"),
        dummy_widget->load_bitmap(platform_data, "KNOBSTATESINACTIVE")
    );

    about_bitmap = dummy_widget->load_bitmap(platform_data, "ABOUT");
    controllers_bitmap = dummy_widget->load_bitmap(platform_data, "CONTROLLERS");
    effects_bitmap = dummy_widget->load_bitmap(platform_data, "EFFECTS");
    envelopes_bitmap = dummy_widget->load_bitmap(platform_data, "ENVELOPES");
    lfos_bitmap = dummy_widget->load_bitmap(platform_data, "LFOS");
    synth_bitmap = dummy_widget->load_bitmap(platform_data, "SYNTH");

    background = new Background();

    this->parent_window = new ExternallyCreatedWindow(platform_data, parent_window);
    this->parent_window->own(background);

    background->set_bitmap(synth_bitmap);

    controller_selector = new ControllerSelector(*background, synth);

    build_about_body();
    build_controllers_body();
    build_effects_body();
    build_envelopes_body();
    build_lfos_body();
    build_synth_body();

    background->own(
        new TabSelector(
            background,
            synth_bitmap,
            synth_body,
            "Synth",
            TabSelector::LEFT + TabSelector::WIDTH * 0
        )
    );
    background->own(
        new TabSelector(
            background,
            effects_bitmap,
            effects_body,
            "Effects",
            TabSelector::LEFT + TabSelector::WIDTH * 1
        )
    );
    background->own(
        new TabSelector(
            background,
            controllers_bitmap,
            controllers_body,
            "Controllers",
            TabSelector::LEFT + TabSelector::WIDTH * 2
        )
    );
    background->own(
        new TabSelector(
            background,
            envelopes_bitmap,
            envelopes_body,
            "Envelopes",
            TabSelector::LEFT + TabSelector::WIDTH * 3
        )
    );
    background->own(
        new TabSelector(
            background,
            lfos_bitmap,
            lfos_body,
            "LFOs",
            TabSelector::LEFT + TabSelector::WIDTH * 4
        )
    );
    background->own(
        new TabSelector(
            background,
            about_bitmap,
            about_body,
            "About",
            TabSelector::LEFT + TabSelector::WIDTH * 5
        )
    );

    background->replace_body(synth_body);

    background->own(controller_selector);
    controller_selector->hide();
}


void GUI::build_about_body()
{
    about_body = new TabBody("About");

    background->own(about_body);

    ((Widget*)about_body)->own(new AboutText());

    about_body->hide();
}


void GUI::build_controllers_body()
{
    controllers_body = new TabBody("Controllers");

    background->own(controllers_body);

    PE(controllers_body,  21 + PE_W * 0,  44, Synth::ParamId::F1IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 1,  44, Synth::ParamId::F1MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 2,  44, Synth::ParamId::F1MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body,  21 + PE_W * 0, 164, Synth::ParamId::F1AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 1, 164, Synth::ParamId::F1DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 2, 164, Synth::ParamId::F1RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 211 + PE_W * 0,  44, Synth::ParamId::F2IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 1,  44, Synth::ParamId::F2MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 2,  44, Synth::ParamId::F2MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 211 + PE_W * 0, 164, Synth::ParamId::F2AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 1, 164, Synth::ParamId::F2DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 2, 164, Synth::ParamId::F2RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 401 + PE_W * 0,  44, Synth::ParamId::F3IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 1,  44, Synth::ParamId::F3MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 2,  44, Synth::ParamId::F3MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 401 + PE_W * 0, 164, Synth::ParamId::F3AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 1, 164, Synth::ParamId::F3DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 2, 164, Synth::ParamId::F3RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 591 + PE_W * 0,  44, Synth::ParamId::F4IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 1,  44, Synth::ParamId::F4MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 2,  44, Synth::ParamId::F4MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 591 + PE_W * 0, 164, Synth::ParamId::F4AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 1, 164, Synth::ParamId::F4DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 2, 164, Synth::ParamId::F4RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 781 + PE_W * 0,  44, Synth::ParamId::F5IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 1,  44, Synth::ParamId::F5MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 2,  44, Synth::ParamId::F5MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 781 + PE_W * 0, 164, Synth::ParamId::F5AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 1, 164, Synth::ParamId::F5DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 2, 164, Synth::ParamId::F5RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body,  21 + PE_W * 0, 324, Synth::ParamId::F6IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 1, 324, Synth::ParamId::F6MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 2, 324, Synth::ParamId::F6MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body,  21 + PE_W * 0, 444, Synth::ParamId::F6AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 1, 444, Synth::ParamId::F6DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body,  21 + PE_W * 2, 444, Synth::ParamId::F6RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 211 + PE_W * 0, 324, Synth::ParamId::F7IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 1, 324, Synth::ParamId::F7MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 2, 324, Synth::ParamId::F7MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 211 + PE_W * 0, 444, Synth::ParamId::F7AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 1, 444, Synth::ParamId::F7DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 211 + PE_W * 2, 444, Synth::ParamId::F7RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 401 + PE_W * 0, 324, Synth::ParamId::F8IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 1, 324, Synth::ParamId::F8MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 2, 324, Synth::ParamId::F8MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 401 + PE_W * 0, 444, Synth::ParamId::F8AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 1, 444, Synth::ParamId::F8DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 401 + PE_W * 2, 444, Synth::ParamId::F8RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 591 + PE_W * 0, 324, Synth::ParamId::F9IN,     FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 1, 324, Synth::ParamId::F9MIN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 2, 324, Synth::ParamId::F9MAX,    FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 591 + PE_W * 0, 444, Synth::ParamId::F9AMT,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 1, 444, Synth::ParamId::F9DST,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 591 + PE_W * 2, 444, Synth::ParamId::F9RND,    FLEX_CTLS, "%.2f", 100.0);


    PE(controllers_body, 781 + PE_W * 0, 324, Synth::ParamId::F10IN,    FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 1, 324, Synth::ParamId::F10MIN,   FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 2, 324, Synth::ParamId::F10MAX,   FLEX_CTLS, "%.2f", 100.0);

    PE(controllers_body, 781 + PE_W * 0, 444, Synth::ParamId::F10AMT,   FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 1, 444, Synth::ParamId::F10DST,   FLEX_CTLS, "%.2f", 100.0);
    PE(controllers_body, 781 + PE_W * 2, 444, Synth::ParamId::F10RND,   FLEX_CTLS, "%.2f", 100.0);

    controllers_body->hide();
}


void GUI::build_effects_body()
{
    effects_body = new TabBody("Effects");

    background->own(effects_body);

    constexpr char const* const* ft = JS80P::GUI::BIQUAD_FILTER_TYPES;
    constexpr int ftc = JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT;

    PE(effects_body,  74 + PE_W * 0,    57, Synth::ParamId::EOG,    LFO_CTLS,   "%.2f", 100.0);

    PE(effects_body, 237 + PE_W * 0,    57, Synth::ParamId::EDG,    LFO_CTLS,   "%.2f", 100.0);

    PE(effects_body, 385 + PE_W * 0,    57, Synth::ParamId::EF1TYP, MIDI_CTLS,  ft, ftc);
    PE(effects_body, 385 + PE_W * 1,    57, Synth::ParamId::EF1FRQ, LFO_CTLS,   "%.1f", 1.0);
    PE(effects_body, 385 + PE_W * 2,    57, Synth::ParamId::EF1Q,   LFO_CTLS,   "%.2f", 1.0);
    PE(effects_body, 385 + PE_W * 3,    57, Synth::ParamId::EF1G,   LFO_CTLS,   "%.2f", 1.0);

    PE(effects_body, 690 + PE_W * 0,    57, Synth::ParamId::EF2TYP, MIDI_CTLS,  ft, ftc);
    PE(effects_body, 690 + PE_W * 1,    57, Synth::ParamId::EF2FRQ, LFO_CTLS,   "%.1f", 1.0);
    PE(effects_body, 690 + PE_W * 2,    57, Synth::ParamId::EF2Q,   LFO_CTLS,   "%.2f", 1.0);
    PE(effects_body, 690 + PE_W * 3,    57, Synth::ParamId::EF2G,   LFO_CTLS,   "%.2f", 1.0);

    PE(effects_body, 258 + PE_W * 0,   242, Synth::ParamId::EEDEL,  LFO_CTLS,   "%.3f", 1.0);
    PE(effects_body, 258 + PE_W * 1,   242, Synth::ParamId::EEFB,   LFO_CTLS,   "%.2f", 100.0);
    PE(effects_body, 258 + PE_W * 2,   242, Synth::ParamId::EEDF,   LFO_CTLS,   "%.1f", 1.0);
    PE(effects_body, 258 + PE_W * 3,   242, Synth::ParamId::EEDG,   LFO_CTLS,   "%.2f", 1.0);
    PE(effects_body, 258 + PE_W * 4,   242, Synth::ParamId::EEWID,  LFO_CTLS,   "%.2f", 100.0);
    PE(effects_body, 258 + PE_W * 5,   242, Synth::ParamId::EEHPF,  LFO_CTLS,   "%.1f", 1.0);
    PE(effects_body, 258 + PE_W * 6,   242, Synth::ParamId::EEWET,  LFO_CTLS,   "%.2f", 100.0);
    PE(effects_body, 258 + PE_W * 7,   242, Synth::ParamId::EEDRY,  LFO_CTLS,   "%.2f", 100.0);

    PE(effects_body, 287 + PE_W * 0,   428, Synth::ParamId::ERRS,   LFO_CTLS,   "%.2f", 100.0);
    PE(effects_body, 287 + PE_W * 1,   428, Synth::ParamId::ERDF,   LFO_CTLS,   "%.1f", 1.0);
    PE(effects_body, 287 + PE_W * 2,   428, Synth::ParamId::ERDG,   LFO_CTLS,   "%.2f", 1.0);
    PE(effects_body, 287 + PE_W * 3,   428, Synth::ParamId::ERWID,  LFO_CTLS,   "%.2f", 100.0);
    PE(effects_body, 287 + PE_W * 4,   428, Synth::ParamId::ERHPF,  LFO_CTLS,   "%.1f", 1.0);
    PE(effects_body, 287 + PE_W * 5,   428, Synth::ParamId::ERWET,  LFO_CTLS,   "%.2f", 100.0);
    PE(effects_body, 287 + PE_W * 6,   428, Synth::ParamId::ERDRY,  LFO_CTLS,   "%.2f", 100.0);

    effects_body->hide();
}


void GUI::build_envelopes_body()
{
    envelopes_body = new TabBody("Envelopes");

    background->own(envelopes_body);

    PE(envelopes_body,  37 + PE_W * 0,  44, Synth::ParamId::N1AMT,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 1,  44, Synth::ParamId::N1INI,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 2,  44, Synth::ParamId::N1PK,   FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 3,  44, Synth::ParamId::N1SUS,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 4,  44, Synth::ParamId::N1FIN,  FLEX_CTLS, "%.2f", 100.0);

    PE(envelopes_body,  37 + PE_W * 0, 164, Synth::ParamId::N1DEL,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 1, 164, Synth::ParamId::N1ATK,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 2, 164, Synth::ParamId::N1HLD,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 3, 164, Synth::ParamId::N1DEC,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 4, 164, Synth::ParamId::N1REL,  FLEX_CTLS, "%.3f", 1.0);


    PE(envelopes_body, 343 + PE_W * 0,  44, Synth::ParamId::N2AMT,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 1,  44, Synth::ParamId::N2INI,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 2,  44, Synth::ParamId::N2PK,   FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 3,  44, Synth::ParamId::N2SUS,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 4,  44, Synth::ParamId::N2FIN,  FLEX_CTLS, "%.2f", 100.0);

    PE(envelopes_body, 343 + PE_W * 0, 164, Synth::ParamId::N2DEL,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 1, 164, Synth::ParamId::N2ATK,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 2, 164, Synth::ParamId::N2HLD,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 3, 164, Synth::ParamId::N2DEC,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 4, 164, Synth::ParamId::N2REL,  FLEX_CTLS, "%.3f", 1.0);


    PE(envelopes_body, 649 + PE_W * 0,  44, Synth::ParamId::N3AMT,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 1,  44, Synth::ParamId::N3INI,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 2,  44, Synth::ParamId::N3PK,   FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 3,  44, Synth::ParamId::N3SUS,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 4,  44, Synth::ParamId::N3FIN,  FLEX_CTLS, "%.2f", 100.0);

    PE(envelopes_body, 649 + PE_W * 0, 164, Synth::ParamId::N3DEL,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 1, 164, Synth::ParamId::N3ATK,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 2, 164, Synth::ParamId::N3HLD,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 3, 164, Synth::ParamId::N3DEC,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 4, 164, Synth::ParamId::N3REL,  FLEX_CTLS, "%.3f", 1.0);


    PE(envelopes_body,  37 + PE_W * 0, 324, Synth::ParamId::N4AMT,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 1, 324, Synth::ParamId::N4INI,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 2, 324, Synth::ParamId::N4PK,   FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 3, 324, Synth::ParamId::N4SUS,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body,  37 + PE_W * 4, 324, Synth::ParamId::N4FIN,  FLEX_CTLS, "%.2f", 100.0);

    PE(envelopes_body,  37 + PE_W * 0, 444, Synth::ParamId::N4DEL,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 1, 444, Synth::ParamId::N4ATK,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 2, 444, Synth::ParamId::N4HLD,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 3, 444, Synth::ParamId::N4DEC,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body,  37 + PE_W * 4, 444, Synth::ParamId::N4REL,  FLEX_CTLS, "%.3f", 1.0);


    PE(envelopes_body, 343 + PE_W * 0, 324, Synth::ParamId::N5AMT,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 1, 324, Synth::ParamId::N5INI,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 2, 324, Synth::ParamId::N5PK,   FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 3, 324, Synth::ParamId::N5SUS,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 343 + PE_W * 4, 324, Synth::ParamId::N5FIN,  FLEX_CTLS, "%.2f", 100.0);

    PE(envelopes_body, 343 + PE_W * 0, 444, Synth::ParamId::N5DEL,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 1, 444, Synth::ParamId::N5ATK,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 2, 444, Synth::ParamId::N5HLD,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 3, 444, Synth::ParamId::N5DEC,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 343 + PE_W * 4, 444, Synth::ParamId::N5REL,  FLEX_CTLS, "%.3f", 1.0);


    PE(envelopes_body, 649 + PE_W * 0, 324, Synth::ParamId::N6AMT,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 1, 324, Synth::ParamId::N6INI,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 2, 324, Synth::ParamId::N6PK,   FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 3, 324, Synth::ParamId::N6SUS,  FLEX_CTLS, "%.2f", 100.0);
    PE(envelopes_body, 649 + PE_W * 4, 324, Synth::ParamId::N6FIN,  FLEX_CTLS, "%.2f", 100.0);

    PE(envelopes_body, 649 + PE_W * 0, 444, Synth::ParamId::N6DEL,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 1, 444, Synth::ParamId::N6ATK,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 2, 444, Synth::ParamId::N6HLD,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 3, 444, Synth::ParamId::N6DEC,  FLEX_CTLS, "%.3f", 1.0);
    PE(envelopes_body, 649 + PE_W * 4, 444, Synth::ParamId::N6REL,  FLEX_CTLS, "%.3f", 1.0);

    envelopes_body->hide();
}


void GUI::build_lfos_body()
{
    lfos_body = new TabBody("LFOs");

    background->own(lfos_body);

    constexpr char const* const* wf = JS80P::GUI::WAVEFORMS;
    constexpr int wfc = JS80P::GUI::WAVEFORMS_COUNT;

    PE(lfos_body,  16 + PE_W * 0,  32, Synth::ParamId::L1WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body,  16 + PE_W * 1,  32, Synth::ParamId::L1FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body,  16 + PE_W * 2,  32, Synth::ParamId::L1PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body,  16 + PE_W * 3,  32, Synth::ParamId::L1MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 4,  32, Synth::ParamId::L1MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 5,  32, Synth::ParamId::L1AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body,  16 + PE_W * 6,  32, Synth::ParamId::L1DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 7,  32, Synth::ParamId::L1RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body, 496 + PE_W * 0,  32, Synth::ParamId::L2WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body, 496 + PE_W * 1,  32, Synth::ParamId::L2FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body, 496 + PE_W * 2,  32, Synth::ParamId::L2PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body, 496 + PE_W * 3,  32, Synth::ParamId::L2MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 4,  32, Synth::ParamId::L2MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 5,  32, Synth::ParamId::L2AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body, 496 + PE_W * 6,  32, Synth::ParamId::L2DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 7,  32, Synth::ParamId::L2RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body,  16 + PE_W * 0, 172, Synth::ParamId::L3WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body,  16 + PE_W * 1, 172, Synth::ParamId::L3FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body,  16 + PE_W * 2, 172, Synth::ParamId::L3PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body,  16 + PE_W * 3, 172, Synth::ParamId::L3MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 4, 172, Synth::ParamId::L3MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 5, 172, Synth::ParamId::L3AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body,  16 + PE_W * 6, 172, Synth::ParamId::L3DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 7, 172, Synth::ParamId::L3RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body, 496 + PE_W * 0, 172, Synth::ParamId::L4WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body, 496 + PE_W * 1, 172, Synth::ParamId::L4FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body, 496 + PE_W * 2, 172, Synth::ParamId::L4PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body, 496 + PE_W * 3, 172, Synth::ParamId::L4MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 4, 172, Synth::ParamId::L4MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 5, 172, Synth::ParamId::L4AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body, 496 + PE_W * 6, 172, Synth::ParamId::L4DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 7, 172, Synth::ParamId::L4RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body,  16 + PE_W * 0, 312, Synth::ParamId::L5WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body,  16 + PE_W * 1, 312, Synth::ParamId::L5FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body,  16 + PE_W * 2, 312, Synth::ParamId::L5PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body,  16 + PE_W * 3, 312, Synth::ParamId::L5MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 4, 312, Synth::ParamId::L5MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 5, 312, Synth::ParamId::L5AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body,  16 + PE_W * 6, 312, Synth::ParamId::L5DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 7, 312, Synth::ParamId::L5RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body, 496 + PE_W * 0, 312, Synth::ParamId::L6WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body, 496 + PE_W * 1, 312, Synth::ParamId::L6FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body, 496 + PE_W * 2, 312, Synth::ParamId::L6PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body, 496 + PE_W * 3, 312, Synth::ParamId::L6MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 4, 312, Synth::ParamId::L6MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 5, 312, Synth::ParamId::L6AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body, 496 + PE_W * 6, 312, Synth::ParamId::L6DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 7, 312, Synth::ParamId::L6RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body,  16 + PE_W * 0, 452, Synth::ParamId::L7WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body,  16 + PE_W * 1, 452, Synth::ParamId::L7FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body,  16 + PE_W * 2, 452, Synth::ParamId::L7PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body,  16 + PE_W * 3, 452, Synth::ParamId::L7MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 4, 452, Synth::ParamId::L7MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 5, 452, Synth::ParamId::L7AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body,  16 + PE_W * 6, 452, Synth::ParamId::L7DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body,  16 + PE_W * 7, 452, Synth::ParamId::L7RND,  LFO_CTLS, "%.2f", 100.0);

    PE(lfos_body, 496 + PE_W * 0, 452, Synth::ParamId::L8WAV,  MIDI_CTLS, wf, wfc);
    PE(lfos_body, 496 + PE_W * 1, 452, Synth::ParamId::L8FRQ,  LFO_CTLS, "%.2f", 1.0);
    PE(lfos_body, 496 + PE_W * 2, 452, Synth::ParamId::L8PHS,  LFO_CTLS, "%.1f", 360.0);
    PE(lfos_body, 496 + PE_W * 3, 452, Synth::ParamId::L8MIN,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 4, 452, Synth::ParamId::L8MAX,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 5, 452, Synth::ParamId::L8AMT,  LFO_CTLS, "%.2f", 200.0);
    PE(lfos_body, 496 + PE_W * 6, 452, Synth::ParamId::L8DST,  LFO_CTLS, "%.2f", 100.0);
    PE(lfos_body, 496 + PE_W * 7, 452, Synth::ParamId::L8RND,  LFO_CTLS, "%.2f", 100.0);

    lfos_body->hide();
}


void GUI::build_synth_body()
{
    synth_body = new TabBody("Synth");

    background->own(synth_body);

    constexpr char const* const* md = JS80P::GUI::MODES;
    constexpr int mdc = JS80P::GUI::MODES_COUNT;
    constexpr char const* const* wf = JS80P::GUI::WAVEFORMS;
    constexpr int wfc = JS80P::GUI::WAVEFORMS_COUNT;
    constexpr char const* const* ft = JS80P::GUI::BIQUAD_FILTER_TYPES;
    constexpr int ftc = JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT;

    ((Widget*)synth_body)->own(
        new ImportPatchButton(7, 2, 32, 32, synth, synth_body)
    );
    ((Widget*)synth_body)->own(new ExportPatchButton(45, 2, 32, 32, synth));

    PE(synth_body, 14, 34 + (PE_H + 6) * 0, Synth::ParamId::MODE,   MIDI_CTLS,  md, mdc);
    PE(synth_body, 14, 34 + (PE_H + 6) * 1, Synth::ParamId::MIX,    LFO_CTLS,   "%.2f", 100.0);
    PE(synth_body, 14, 34 + (PE_H + 6) * 2, Synth::ParamId::PM,     ALL_CTLS,   "%.2f", 100.0 / Constants::PM_MAX);
    PE(synth_body, 14, 34 + (PE_H + 6) * 3, Synth::ParamId::FM,     ALL_CTLS,   "%.2f", 100.0 / Constants::FM_MAX);
    PE(synth_body, 14, 34 + (PE_H + 6) * 4, Synth::ParamId::AM,     ALL_CTLS,   "%.2f", 100.0 / Constants::AM_MAX);

    PE(synth_body,  87 + PE_W * 0,      36, Synth::ParamId::MWAV,   MIDI_CTLS,  wf, wfc);
    PE(synth_body,  87 + PE_W * 1,      36, Synth::ParamId::MPRT,   FLEX_CTLS,  "%.3f", 1.0);
    PE(synth_body,  87 + PE_W * 2,      36, Synth::ParamId::MPRD,   FLEX_CTLS,  "%.2f", 1.0);
    PE(synth_body,  87 + PE_W * 3,      36, Synth::ParamId::MDTN,   FLEX_CTLS,  "%.f", Constants::DETUNE_SCALE);
    PE(synth_body,  87 + PE_W * 4,      36, Synth::ParamId::MFIN,   ALL_CTLS,   "%.2f", 1.0);
    PE(synth_body,  87 + PE_W * 5,      36, Synth::ParamId::MAMP,   ALL_CTLS,   "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 6,      36, Synth::ParamId::MFLD,   ALL_CTLS,   "%.2f", 100.0 / Constants::FOLD_MAX);
    PE(synth_body,  87 + PE_W * 7,      36, Synth::ParamId::MVS,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 8,      36, Synth::ParamId::MVOL,   ALL_CTLS,   "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 9,      36, Synth::ParamId::MWID,   FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 10,     36, Synth::ParamId::MPAN,   ALL_CTLS,   "%.2f", 100.0);

    PE(synth_body, 735 + PE_W * 0,      36, Synth::ParamId::MF1TYP, MIDI_CTLS,  ft, ftc);
    PE(synth_body, 735 + PE_W * 1,      36, Synth::ParamId::MF1FRQ, ALL_CTLS,   "%.1f", 1.0);
    PE(synth_body, 735 + PE_W * 2,      36, Synth::ParamId::MF1Q,   ALL_CTLS,   "%.2f", 1.0);
    PE(synth_body, 735 + PE_W * 3,      36, Synth::ParamId::MF1G,   ALL_CTLS,   "%.2f", 1.0);

    PE(synth_body, 116 + PE_W * 0,     168, Synth::ParamId::MC1,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 1,     168, Synth::ParamId::MC2,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 2,     168, Synth::ParamId::MC3,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 3,     168, Synth::ParamId::MC4,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 4,     168, Synth::ParamId::MC5,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 5,     168, Synth::ParamId::MC6,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 6,     168, Synth::ParamId::MC7,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 7,     168, Synth::ParamId::MC8,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 8,     168, Synth::ParamId::MC9,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 9,     168, Synth::ParamId::MC10,   FLEX_CTLS,  "%.2f", 100.0);

    PE(synth_body, 735 + PE_W * 0,     168, Synth::ParamId::MF2TYP, MIDI_CTLS,  ft, ftc);
    PE(synth_body, 735 + PE_W * 1,     168, Synth::ParamId::MF2FRQ, ALL_CTLS,   "%.1f", 1.0);
    PE(synth_body, 735 + PE_W * 2,     168, Synth::ParamId::MF2Q,   ALL_CTLS,   "%.2f", 1.0);
    PE(synth_body, 735 + PE_W * 3,     168, Synth::ParamId::MF2G,   ALL_CTLS,   "%.2f", 1.0);

    PE(synth_body,  87 + PE_W * 0,     316, Synth::ParamId::CWAV,   MIDI_CTLS,  wf, wfc);
    PE(synth_body,  87 + PE_W * 1,     316, Synth::ParamId::CPRT,   FLEX_CTLS,  "%.3f", 1.0);
    PE(synth_body,  87 + PE_W * 2,     316, Synth::ParamId::CPRD,   FLEX_CTLS,  "%.2f", 1.0);
    PE(synth_body,  87 + PE_W * 3,     316, Synth::ParamId::CDTN,   FLEX_CTLS,  "%.f", 0.01);
    PE(synth_body,  87 + PE_W * 4,     316, Synth::ParamId::CFIN,   ALL_CTLS,   "%.2f", 1.0);
    PE(synth_body,  87 + PE_W * 5,     316, Synth::ParamId::CAMP,   ALL_CTLS,   "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 6,     316, Synth::ParamId::CFLD,   ALL_CTLS,   "%.2f", 100.0 / Constants::FOLD_MAX);
    PE(synth_body,  87 + PE_W * 7,     316, Synth::ParamId::CVS,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 8,     316, Synth::ParamId::CVOL,   ALL_CTLS,   "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 9,     316, Synth::ParamId::CWID,   FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body,  87 + PE_W * 10,    316, Synth::ParamId::CPAN,   ALL_CTLS,   "%.2f", 100.0);

    PE(synth_body, 735 + PE_W * 0,     316, Synth::ParamId::CF1TYP, MIDI_CTLS,  ft, ftc);
    PE(synth_body, 735 + PE_W * 1,     316, Synth::ParamId::CF1FRQ, ALL_CTLS,   "%.1f", 1.0);
    PE(synth_body, 735 + PE_W * 2,     316, Synth::ParamId::CF1Q,   ALL_CTLS,   "%.2f", 1.0);
    PE(synth_body, 735 + PE_W * 3,     316, Synth::ParamId::CF1G,   ALL_CTLS,   "%.2f", 1.0);

    PE(synth_body, 116 + PE_W * 0,     448, Synth::ParamId::CC1,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 1,     448, Synth::ParamId::CC2,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 2,     448, Synth::ParamId::CC3,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 3,     448, Synth::ParamId::CC4,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 4,     448, Synth::ParamId::CC5,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 5,     448, Synth::ParamId::CC6,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 6,     448, Synth::ParamId::CC7,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 7,     448, Synth::ParamId::CC8,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 8,     448, Synth::ParamId::CC9,    FLEX_CTLS,  "%.2f", 100.0);
    PE(synth_body, 116 + PE_W * 9,     448, Synth::ParamId::CC10,   FLEX_CTLS,  "%.2f", 100.0);

    PE(synth_body, 735 + PE_W * 0,     448, Synth::ParamId::CF2TYP, MIDI_CTLS,  ft, ftc);
    PE(synth_body, 735 + PE_W * 1,     448, Synth::ParamId::CF2FRQ, ALL_CTLS,   "%.1f", 1.0);
    PE(synth_body, 735 + PE_W * 2,     448, Synth::ParamId::CF2Q,   ALL_CTLS,   "%.2f", 1.0);
    PE(synth_body, 735 + PE_W * 3,     448, Synth::ParamId::CF2G,   ALL_CTLS,   "%.2f", 1.0);

    synth_body->show();
}


GUI::~GUI()
{
    delete parent_window;

    ParamEditor::free_knob_states(dummy_widget);

    dummy_widget->delete_bitmap(about_bitmap);
    dummy_widget->delete_bitmap(controllers_bitmap);
    dummy_widget->delete_bitmap(effects_bitmap);
    dummy_widget->delete_bitmap(envelopes_bitmap);
    dummy_widget->delete_bitmap(lfos_bitmap);
    dummy_widget->delete_bitmap(synth_bitmap);

    delete dummy_widget;

    dummy_widget = NULL;
}


void GUI::show()
{
    background->show();
}


WidgetBase::WidgetBase(char const* const text)
    : type(Type::BACKGROUND),
    text(text),
    platform_widget(NULL),
    platform_data(NULL),
    bitmap(NULL),
    parent(NULL),
    left(0),
    top(0),
    width(0),
    height(0),
    is_clicking(false)
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
    text(text),
    platform_widget(NULL),
    platform_data(NULL),
    bitmap(NULL),
    parent(NULL),
    left(left),
    top(top),
    width(width),
    height(height),
    is_clicking(false)
{
}


WidgetBase::WidgetBase(
        GUI::PlatformData platform_data,
        GUI::PlatformWidget platform_widget,
        Type const type
) : type(type),
    text(""),
    platform_widget(platform_widget),
    platform_data(platform_data),
    bitmap(NULL),
    parent(NULL),
    left(0),
    top(0),
    width(0),
    height(0),
    is_clicking(false)
{
}


WidgetBase::~WidgetBase()
{
}


void WidgetBase::destroy_children()
{
    for (GUI::Widgets::iterator it = children.begin(); it != children.end(); ++it) {
        delete *it;
    }
}


GUI::Bitmap WidgetBase::load_bitmap(
    GUI::PlatformData platform_data,
    char const* name
) {
    return NULL;
}


void WidgetBase::delete_bitmap(GUI::Bitmap bitmap)
{
}


void WidgetBase::show()
{
}


void WidgetBase::hide()
{
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


GUI::Bitmap WidgetBase::set_bitmap(GUI::Bitmap bitmap)
{
    GUI::Bitmap old = this->bitmap;
    this->bitmap = bitmap;
    redraw();

    return old;
}


GUI::PlatformWidget WidgetBase::get_platform_widget()
{
    return platform_widget;
}


void WidgetBase::click()
{
}


void WidgetBase::set_up(GUI::PlatformData platform_data, WidgetBase* parent)
{
    this->platform_data = platform_data;
    this->parent = parent;
}


bool WidgetBase::timer_tick()
{
    return false;
}


bool WidgetBase::paint()
{
    if (bitmap == NULL) {
        return false;
    }

    draw_bitmap(bitmap, 0, 0, width, height);

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


void WidgetBase::start_timer(Frequency const frequency)
{
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


void WidgetBase::draw_bitmap(
        GUI::Bitmap bitmap,
        int const left,
        int const top,
        int const width,
        int const height
) {
}


GUI::Bitmap WidgetBase::copy_bitmap_region(
    GUI::Bitmap source,
    int const left,
    int const top,
    int const width,
    int const height
) {
    return NULL;
}

}

#endif
