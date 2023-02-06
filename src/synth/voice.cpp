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

#ifndef JS80P__SYNTH__VOICE_CPP
#define JS80P__SYNTH__VOICE_CPP

#include "synth/voice.hpp"

#include "synth/math.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Params::Params(std::string const name)
    : waveform(name + "WAV"),
    amplitude(name + "AMP", 0.0, 1.0, 1.0),
    velocity_sensitivity(name + "VS", 0.0, 2.0, 1.0),
    folding(name + "FLD", Constants::FOLD_MIN, Constants::FOLD_MAX, 0.0),
    portamento_length(name + "PRT", 0.0, 3.0, 0.0),
    portamento_depth(name + "PRD", -2400.0, 2400.0, 0.0),
    detune(name + "DTN", Constants::DETUNE_MIN, Constants::DETUNE_MAX, 0.0, 100.0),
    fine_detune(
        name + "FIN", Constants::FINE_DETUNE_MIN, Constants::FINE_DETUNE_MAX, 0.0
    ),
    width(name + "WID", -1.0, 1.0, 0.2),
    panning(name + "PAN", -1.0, 1.0, 0.0),
    volume(name + "VOL", 0.0, 1.0, 0.5),

    harmonic_0(name + "C1", -1.0, 1.0, 0.333),
    harmonic_1(name + "C2", -1.0, 1.0, 0.333),
    harmonic_2(name + "C3", -1.0, 1.0, 0.333),
    harmonic_3(name + "C4", -1.0, 1.0, 0.0),
    harmonic_4(name + "C5", -1.0, 1.0, 0.0),
    harmonic_5(name + "C6", -1.0, 1.0, 0.0),
    harmonic_6(name + "C7", -1.0, 1.0, 0.0),
    harmonic_7(name + "C8", -1.0, 1.0, 0.0),
    harmonic_8(name + "C9", -1.0, 1.0, 0.0),
    harmonic_9(name + "C10", -1.0, 1.0, 0.0),

    filter_1_type(name + "F1TYP"),
    filter_1_frequency(
        name + "F1FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    ),
    filter_1_q(
        name + "F1Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        1.0
    ),
    filter_1_gain(
        name + "F1G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        0.0
    ),

    filter_2_type(name + "F2TYP"),
    filter_2_frequency(
        name + "F2FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    ),
    filter_2_q(
        name + "F2Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        1.0
    ),
    filter_2_gain(
        name + "F2G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        0.0
    )
{
}


template<class ModulatorSignalProducerClass>
Voice<ModulatorSignalProducerClass>::Voice(
        Frequency const* frequencies,
        Midi::Note const notes,
        Params& param_leaders,
        ModulatorSignalProducerClass* modulator,
        FloatParam& amplitude_modulation_level_leader,
        FloatParam& frequency_modulation_level_leader
) : SignalProducer(CHANNELS, 14),
    notes(notes),
    oscillator(
        param_leaders.waveform,
        param_leaders.amplitude,
        param_leaders.detune,
        param_leaders.fine_detune,
        param_leaders.harmonic_0,
        param_leaders.harmonic_1,
        param_leaders.harmonic_2,
        param_leaders.harmonic_3,
        param_leaders.harmonic_4,
        param_leaders.harmonic_5,
        param_leaders.harmonic_6,
        param_leaders.harmonic_7,
        param_leaders.harmonic_8,
        param_leaders.harmonic_9,
        modulator,
        amplitude_modulation_level_leader,
        frequency_modulation_level_leader
    ),
    filter_1(
        oscillator,
        param_leaders.filter_1_type,
        param_leaders.filter_1_frequency,
        param_leaders.filter_1_q,
        param_leaders.filter_1_gain
    ),
    wavefolder(filter_1, param_leaders.folding),
    filter_2(
        wavefolder,
        param_leaders.filter_2_type,
        param_leaders.filter_2_frequency,
        param_leaders.filter_2_q,
        param_leaders.filter_2_gain
    ),
    velocity_sensitivity(param_leaders.velocity_sensitivity),
    portamento_length(param_leaders.portamento_length),
    portamento_depth(param_leaders.portamento_depth),
    width(param_leaders.width),
    panning(param_leaders.panning),
    volume(param_leaders.volume),
    frequencies(frequencies),
    off_after(0.0),
    state(OFF),
    modulation_out((ModulationOut&)filter_2)
{
    register_child(velocity_sensitivity);
    register_child(portamento_length);
    register_child(portamento_depth);
    register_child(width);
    register_child(panning);
    register_child(volume);

    register_child(oscillator);
    register_child(filter_1);
    register_child(wavefolder);
    register_child(filter_2);
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::is_on() const
{
    return !is_off_after(current_time);
}


template<class ModulatorSignalProducerClass>
bool Voice<ModulatorSignalProducerClass>::is_off_after(
        Seconds const time_offset
) const {
    return state == OFF && !oscillator.has_events_after(time_offset);
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::note_on(
        Seconds const time_offset,
        Midi::Note const note,
        Number const velocity,
        Midi::Note const previous_note
) {
    if (state == ON || note >= notes) {
        return;
    }

    state = ON;

    this->note = note;
    this->velocity = calculate_velocity(velocity);

    oscillator.cancel_events(time_offset);

    wavefolder.folding.start_envelope(time_offset);

    panning.start_envelope(time_offset);
    volume.start_envelope(time_offset);

    set_up_oscillator_frequency(time_offset, note, previous_note);

    oscillator.modulated_amplitude.start_envelope(time_offset);
    oscillator.amplitude.start_envelope(time_offset);
    oscillator.frequency.start_envelope(time_offset);
    oscillator.fine_detune.start_envelope(time_offset);

    filter_1.frequency.start_envelope(time_offset);
    filter_1.q.start_envelope(time_offset);
    filter_1.gain.start_envelope(time_offset);

    filter_2.frequency.start_envelope(time_offset);
    filter_2.q.start_envelope(time_offset);
    filter_2.gain.start_envelope(time_offset);

    oscillator.start(time_offset);
}


template<class ModulatorSignalProducerClass>
Number Voice<ModulatorSignalProducerClass>::calculate_velocity(
        Number const raw_velocity
) const {
    Number const sensitivity = velocity_sensitivity.get_value();

    if (sensitivity <= 1.0) {
        return 1.0 - sensitivity + sensitivity * raw_velocity;
    }

    Number const oversensitivity = sensitivity - 1.0;
    Number const velocity_sqr = raw_velocity * raw_velocity;

    return (
        raw_velocity
        + oversensitivity * (velocity_sqr * velocity_sqr - raw_velocity)
    );
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::set_up_oscillator_frequency(
        Seconds const time_offset,
        Midi::Note const note,
        Midi::Note const previous_note
) {
    Number const portamento_length = this->portamento_length.get_value();
    Frequency const note_frequency = frequencies[note];

    oscillator.frequency.cancel_events(time_offset);

    /*
    Though we never assign an envelope to Oscillator.frequency, its modulation
    level might have one (through its leader).
    */
    oscillator.frequency.start_envelope(time_offset);

    if (portamento_length <= sampling_period) {
        oscillator.frequency.set_value((Number)note_frequency);

        return;
    }

    Number const portamento_depth = this->portamento_depth.get_value();
    Frequency const start_frequency = (
        std::fabs(portamento_depth) < 0.01
            ? frequencies[previous_note]
            : Math::detune(note_frequency, portamento_depth)
    );

    oscillator.frequency.schedule_value(
        time_offset, (Number)start_frequency
    );
    oscillator.frequency.schedule_linear_ramp(
        portamento_length, (Number)note_frequency
    );
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::note_off(
        Seconds const time_offset,
        Midi::Note const note,
        Number const velocity
) {
    off_after = time_offset + std::max(
        oscillator.amplitude.end_envelope(time_offset),
        volume.end_envelope(time_offset)
    );

    oscillator.cancel_events(off_after);
    oscillator.stop(off_after);

    state = OFF;

    wavefolder.folding.end_envelope(time_offset);

    panning.end_envelope(time_offset);

    oscillator.modulated_amplitude.end_envelope(time_offset);
    oscillator.amplitude.end_envelope(time_offset);
    oscillator.frequency.end_envelope(time_offset);
    oscillator.fine_detune.end_envelope(time_offset);

    filter_1.frequency.end_envelope(time_offset);
    filter_1.q.end_envelope(time_offset);
    filter_1.gain.end_envelope(time_offset);

    filter_2.frequency.end_envelope(time_offset);
    filter_2.q.end_envelope(time_offset);
    filter_2.gain.end_envelope(time_offset);
}


template<class ModulatorSignalProducerClass>
Sample const* const* Voice<ModulatorSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    oscillator_buffer = SignalProducer::produce<Filter2>(
        &filter_2, round, sample_count
    )[0];

    return NULL;
}


template<class ModulatorSignalProducerClass>
void Voice<ModulatorSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) {
    Sample const velocity = (Sample)this->velocity;
    Sample const* oscillator_buffer = this->oscillator_buffer;

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        buffer[0][i] = buffer[1][i] = oscillator_buffer[i] * velocity;
    }
}

}

#endif
