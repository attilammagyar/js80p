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

#include <algorithm>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"
#include "midi.hpp"

#include "synth.cpp"


using namespace JS80P;


SimpleOscillator::WaveformParam wavetable_cache_waveform("WAV");
SimpleOscillator wavetable_cache(wavetable_cache_waveform);


TEST(communication_with_the_gui_is_lock_free, {
    Synth synth;

    assert_true(synth.is_lock_free());
});


class FrequenciesTestSynth : public Synth
{
    public:
        FrequenciesTestSynth() : Synth()
        {
        }

        Frequency get_frequency(Midi::Note const note) const
        {
            return frequencies[note];
        }
};


TEST(twelve_tone_equal_temperament_440_hz, {
    FrequenciesTestSynth synth;

    assert_eq(880.0, synth.get_frequency(Midi::NOTE_A_5), DOUBLE_DELTA);
    assert_eq(440.0, synth.get_frequency(Midi::NOTE_A_4), DOUBLE_DELTA);
    assert_eq(220.0, synth.get_frequency(Midi::NOTE_A_3), DOUBLE_DELTA);

    assert_eq(12543.85, synth.get_frequency(Midi::NOTE_G_9), 0.01);
    assert_eq(261.63, synth.get_frequency(Midi::NOTE_C_4), 0.01);
    assert_eq(8.18, synth.get_frequency(Midi::NOTE_0), 0.01);
})


void set_up_chunk_size_independent_test(Synth& synth, Frequency const sample_rate)
{
    synth.set_sample_rate(sample_rate);
    synth.note_on(0.05, 1, 69, 0.9);
    synth.note_on(0.25, 1, 127, 0.9);
    synth.note_off(0.05 + 3.0, 1, 69, 0.9);
    synth.note_off(0.25 + 2.9, 1, 127, 0.9);
}


TEST(synth_rendering_is_independent_of_chunk_size, {
    constexpr Frequency sample_rate = 44100.0;
    Synth synth_1;
    Synth synth_2;

    set_up_chunk_size_independent_test(synth_1, sample_rate);
    set_up_chunk_size_independent_test(synth_2, sample_rate);

    assert_rendering_is_independent_from_chunk_size<Synth>(synth_1, synth_2);
})


TEST(messages_get_processed_during_rendering, {
    Synth synth;
    Number const inv_saw_as_ratio = (
        synth.modulator_params.waveform.value_to_ratio(
            SimpleOscillator::INVERSE_SAWTOOTH
        )
    );

    synth.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::MWAV, inv_saw_as_ratio, 0
    );
    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::VOL, 0.123, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::ENVELOPE_3
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::VOL, 0.0, 0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::MWAV, 0.0, 0
    );

    assert_eq(1.0, synth.volume.get_value(), DOUBLE_DELTA);
    assert_eq(NULL, synth.carrier_params.volume.get_envelope());
    assert_eq(
        SimpleOscillator::SINE, synth.modulator_params.waveform.get_value()
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id(Synth::ParamId::CVOL)
    );
    SignalProducer::produce<Synth>(&synth, 1);

    assert_eq(0.123, synth.volume.get_value(), DOUBLE_DELTA);
    assert_eq(0.123, synth.get_param_ratio_atomic(Synth::ParamId::VOL), DOUBLE_DELTA);
    assert_eq(
        inv_saw_as_ratio,
        synth.get_param_ratio_atomic(Synth::ParamId::MWAV),
        DOUBLE_DELTA
    );
    assert_eq(
        SimpleOscillator::INVERSE_SAWTOOTH,
        synth.modulator_params.waveform.get_value()
    );
    assert_eq(
        (void*)synth.envelopes[2],
        (void*)synth.carrier_params.volume.get_envelope()
    );
    assert_eq(
        Synth::ControllerId::ENVELOPE_3,
        synth.get_param_controller_id(Synth::ParamId::CVOL)
    );
});


TEST(midi_controller_changes_can_affect_parameters, {
    Synth synth;
    Midi::Controller invalid = 127;
    Midi::Controller unused = 0;

    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::VOL,
        0.0,
        Synth::ControllerId::VOLUME
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::MFIN,
        0.0,
        Synth::ControllerId::PITCH_WHEEL
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::MAMP,
        0.0,
        Synth::ControllerId::VELOCITY
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::VOL, 0.0, 0.0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::MFIN, 0.0, 0.0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::MAMP, 0.0, 0.0
    );
    synth.control_change(0.0, 1, Midi::VOLUME, 0.42);
    synth.control_change(0.0, 1, invalid, 0.123);
    synth.control_change(0.0, 1, unused, 0.123);
    synth.pitch_wheel_change(0.0, 1, 0.75);
    synth.note_on(0.0, 1, 69, 0.9);

    SignalProducer::produce<Synth>(&synth, 1, 1);

    assert_eq(0.42, synth.volume.get_value(), DOUBLE_DELTA);
    assert_eq(0.9, synth.modulator_params.amplitude.get_value(), DOUBLE_DELTA);
    assert_eq(
        600.0, synth.modulator_params.fine_detune.get_value(), DOUBLE_DELTA
    );

    assert_true(synth.volume.is_constant_in_next_round(2, 1));
    assert_true(
        synth.modulator_params.amplitude.is_constant_in_next_round(2, 1)
    );
    assert_true(
        synth.modulator_params.fine_detune.is_constant_in_next_round(2, 1)
    );
})
