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

#include "gui/gui.hpp"

#include "synth/biquad_filter.hpp"
#include "synth/oscillator.hpp"


namespace JS80P
{

bool GUI::controllers_by_id_initialized = false;


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
    [Synth::ParamId::VOL] = "Volume (%)",

    [Synth::ParamId::ADD] = "Modulator Additive Volume (%)",
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
    [Synth::ParamId::L1AMT] = "LFO 1 Amount (%)",
    [Synth::ParamId::L1MIN] = "LFO 1 Minimum Value (%)",
    [Synth::ParamId::L1MAX] = "LFO 1 Maximum Value (%)",
    [Synth::ParamId::L1DST] = "LFO 1 Distortion (%)",
    [Synth::ParamId::L1RND] = "LFO 1 Randomness (%)",

    [Synth::ParamId::L2FRQ] = "LFO 2 Frequency (Hz)",
    [Synth::ParamId::L2AMT] = "LFO 2 Amount (%)",
    [Synth::ParamId::L2MIN] = "LFO 2 Minimum Value (%)",
    [Synth::ParamId::L2MAX] = "LFO 2 Maximum Value (%)",
    [Synth::ParamId::L2DST] = "LFO 2 Distortion (%)",
    [Synth::ParamId::L2RND] = "LFO 2 Randomness (%)",

    [Synth::ParamId::L3FRQ] = "LFO 3 Frequency (Hz)",
    [Synth::ParamId::L3AMT] = "LFO 3 Amount (%)",
    [Synth::ParamId::L3MIN] = "LFO 3 Minimum Value (%)",
    [Synth::ParamId::L3MAX] = "LFO 3 Maximum Value (%)",
    [Synth::ParamId::L3DST] = "LFO 3 Distortion (%)",
    [Synth::ParamId::L3RND] = "LFO 3 Randomness (%)",

    [Synth::ParamId::L4FRQ] = "LFO 4 Frequency (Hz)",
    [Synth::ParamId::L4AMT] = "LFO 4 Amount (%)",
    [Synth::ParamId::L4MIN] = "LFO 4 Minimum Value (%)",
    [Synth::ParamId::L4MAX] = "LFO 4 Maximum Value (%)",
    [Synth::ParamId::L4DST] = "LFO 4 Distortion (%)",
    [Synth::ParamId::L4RND] = "LFO 4 Randomness (%)",

    [Synth::ParamId::L5FRQ] = "LFO 5 Frequency (Hz)",
    [Synth::ParamId::L5AMT] = "LFO 5 Amount (%)",
    [Synth::ParamId::L5MIN] = "LFO 5 Minimum Value (%)",
    [Synth::ParamId::L5MAX] = "LFO 5 Maximum Value (%)",
    [Synth::ParamId::L5DST] = "LFO 5 Distortion (%)",
    [Synth::ParamId::L5RND] = "LFO 5 Randomness (%)",

    [Synth::ParamId::L6FRQ] = "LFO 6 Frequency (Hz)",
    [Synth::ParamId::L6AMT] = "LFO 6 Amount (%)",
    [Synth::ParamId::L6MIN] = "LFO 6 Minimum Value (%)",
    [Synth::ParamId::L6MAX] = "LFO 6 Maximum Value (%)",
    [Synth::ParamId::L6DST] = "LFO 6 Distortion (%)",
    [Synth::ParamId::L6RND] = "LFO 6 Randomness (%)",

    [Synth::ParamId::L7FRQ] = "LFO 7 Frequency (Hz)",
    [Synth::ParamId::L7AMT] = "LFO 7 Amount (%)",
    [Synth::ParamId::L7MIN] = "LFO 7 Minimum Value (%)",
    [Synth::ParamId::L7MAX] = "LFO 7 Maximum Value (%)",
    [Synth::ParamId::L7DST] = "LFO 7 Distortion (%)",
    [Synth::ParamId::L7RND] = "LFO 7 Randomness (%)",

    [Synth::ParamId::L8FRQ] = "LFO 8 Frequency (Hz)",
    [Synth::ParamId::L8AMT] = "LFO 8 Amount (%)",
    [Synth::ParamId::L8MIN] = "LFO 8 Minimum Value (%)",
    [Synth::ParamId::L8MAX] = "LFO 8 Maximum Value (%)",
    [Synth::ParamId::L8DST] = "LFO 8 Distortion (%)",
    [Synth::ParamId::L8RND] = "LFO 8 Randomness (%)",

    [Synth::ParamId::MODE] = "Mode",

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


GUI::Controller const* GUI::get_controller(Synth::ControllerId controller_id)
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

}
