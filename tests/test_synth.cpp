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
#include <cmath>
#include <string>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"
#include "midi.hpp"

#include "synth.cpp"


using namespace JS80P;


SimpleOscillator::WaveformParam wavetable_cache_waveform("WAV");
SimpleOscillator wavetable_cache(wavetable_cache_waveform);


constexpr Number OUT_VOLUME_PER_CHANNEL = std::sin(Math::PI / 4.0);


/*
MIDI reserves CC 32-63 for the lowest 7 bits of CC 0-31 messages
respectively. Even though JS80P doesn't read 14 bit CC messages yet, we
don't want to allow assigning these CC numbers separately, because that
would complicate implementing 14 bit messages.
*/
constexpr Midi::Controller UNSUPPORTED_CC = 33;


constexpr Synth::MessageType SET_PARAM = Synth::MessageType::SET_PARAM;
constexpr Synth::MessageType REFRESH_PARAM = Synth::MessageType::REFRESH_PARAM;
constexpr Synth::MessageType ASSIGN_CONTROLLER = Synth::MessageType::ASSIGN_CONTROLLER;
constexpr Synth::MessageType CLEAR = Synth::MessageType::CLEAR;


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
    synth.resume();
    synth.note_on(0.05, 1, Midi::NOTE_A_4, 114);
    synth.note_on(0.25, 1, Midi::NOTE_G_9, 114);
    synth.note_off(0.05 + 3.0, 1, Midi::NOTE_A_4, 114);
    synth.note_off(0.25 + 2.9, 1, Midi::NOTE_G_9, 114);
}


TEST(synth_rendering_is_independent_of_chunk_size, {
    constexpr Frequency sample_rate = 44100.0;
    Synth synth_1;
    Synth synth_2;

    set_up_chunk_size_independent_test(synth_1, sample_rate);
    set_up_chunk_size_independent_test(synth_2, sample_rate);

    assert_rendering_is_independent_from_chunk_size<Synth>(synth_1, synth_2);
})


void set_param(Synth& synth, Synth::ParamId const param_id, Number const ratio)
{
    synth.push_message(SET_PARAM, param_id, ratio, 0);
}


void assign_controller(
        Synth& synth,
        Synth::ParamId const param_id,
        Byte const controller_id
) {
    synth.push_message(ASSIGN_CONTROLLER, param_id, 0.0, controller_id);
}


TEST(messages_get_processed_during_rendering, {
    Synth synth;
    Synth::Message message(SET_PARAM, Synth::ParamId::PM, 0.123, 0);
    Number const inv_saw_as_ratio = (
        synth.modulator_params.waveform.value_to_ratio(
            SimpleOscillator::INVERSE_SAWTOOTH
        )
    );

    synth.phase_modulation_level.set_value(1.0);
    synth.modulator_add_volume.set_value(0.42);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);

    synth.push_message(SET_PARAM, Synth::ParamId::MWAV, inv_saw_as_ratio, 0);
    synth.push_message(message);
    synth.push_message(REFRESH_PARAM, Synth::ParamId::MIX, 0.0, 0);
    assign_controller(synth, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_3);

    assert_eq(1.0, synth.phase_modulation_level.get_value(), DOUBLE_DELTA);
    assert_eq(NULL, synth.carrier_params.volume.get_envelope());
    assert_eq(
        SimpleOscillator::SINE, synth.modulator_params.waveform.get_value()
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );

    message.number_param = 0.321;
    synth.process_messages();

    assert_eq(0.123, synth.phase_modulation_level.get_ratio(), DOUBLE_DELTA);
    assert_eq(0.123, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA);
    assert_eq(0.42, synth.modulator_add_volume.get_ratio(), DOUBLE_DELTA);
    assert_eq(0.42, synth.get_param_ratio_atomic(Synth::ParamId::MIX), DOUBLE_DELTA);
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
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
});


TEST(midi_controller_changes_can_affect_parameters, {
    constexpr Integer block_size = 2048;

    Synth synth;
    Midi::Controller invalid = 127;
    Midi::Controller unused = 0;

    synth.resume();

    synth.set_block_size(block_size);

    assign_controller(synth, Synth::ParamId::PM, Synth::ControllerId::VOLUME);
    assign_controller(
        synth, Synth::ParamId::MFIN, Synth::ControllerId::PITCH_WHEEL
    );
    assign_controller(
        synth, Synth::ParamId::MAMP, Synth::ControllerId::VELOCITY
    );
    assign_controller(
        synth, Synth::ParamId::CWAV, Synth::ControllerId::MODULATION_WHEEL
    );

    synth.push_message(REFRESH_PARAM, Synth::ParamId::PM, 0.0, 0.0);
    synth.push_message(REFRESH_PARAM, Synth::ParamId::MFIN, 0.0, 0.0);
    synth.push_message(REFRESH_PARAM, Synth::ParamId::MAMP, 0.0, 0.0);
    synth.push_message(REFRESH_PARAM, Synth::ParamId::CWAV, 0.0, 0.0);

    synth.control_change(0.0, 1, Midi::VOLUME, 53);
    synth.control_change(0.0, 1, Midi::MODULATION_WHEEL, 127);
    synth.control_change(0.0, 1, invalid, 16);
    synth.control_change(0.0, 1, unused, 16);
    synth.pitch_wheel_change(0.0, 1, 12288);
    synth.note_on(0.0, 1, Midi::NOTE_A_4, 114);

    SignalProducer::produce<Synth>(synth, 1);

    assert_eq(53.0 / 127.0, synth.phase_modulation_level.get_ratio(), DOUBLE_DELTA);
    assert_eq(114.0 / 127.0, synth.modulator_params.amplitude.get_ratio(), DOUBLE_DELTA);
    assert_eq(12288.0 / 16384, synth.modulator_params.fine_detune.get_ratio(), DOUBLE_DELTA);
    assert_eq(SimpleOscillator::CUSTOM, synth.carrier_params.waveform.get_value());

    assert_true(synth.phase_modulation_level.is_constant_in_next_round(2, 1));
    assert_true(
        synth.modulator_params.amplitude.is_constant_in_next_round(2, 1)
    );
    assert_true(
        synth.modulator_params.fine_detune.is_constant_in_next_round(2, 1)
    );
})


TEST(can_look_up_param_id_by_name, {
    Synth synth;
    Integer max_collisions;
    Number avg_collisions;
    Number avg_bucket_size;

    synth.get_param_id_hash_table_statistics(
        max_collisions, avg_collisions, avg_bucket_size
    );

    assert_lte((int)max_collisions, 6);
    assert_lte(avg_bucket_size, 3.0);
    assert_lte(avg_collisions, 3.5);

    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id(""));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id(" \n"));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id("NO_SUCH_PARAM"));

    for (int i = 0; i != Synth::ParamId::MAX_PARAM_ID; ++i) {
        std::string const name = synth.get_param_name((Synth::ParamId)i);
        Synth::ParamId const param_id = synth.get_param_id(name);
        assert_eq((Synth::ParamId)i, param_id, "i=%d, name=\"%s\"", i, name);
    }
})


void test_operating_mode(
        Number const expected_vol_a3,
        Number const expected_vol_a5,
        Synth::Mode const mode
) {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;

    Synth synth;
    SumOfSines expected(
        expected_vol_a3, 220.0,
        expected_vol_a5, 880.0,
        0.0, 0.0,
        synth.get_channels()
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    synth.mode.set_value(mode);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);

    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.detune.set_value(2400.0);
    synth.carrier_params.volume.set_value(1.0);
    synth.carrier_params.waveform.set_value(SimpleOscillator::SINE);
    synth.carrier_params.width.set_value(0.0);

    synth.note_on(0.0, 0, Midi::NOTE_A_3, 127);

    expected_samples = SignalProducer::produce<SumOfSines>(expected, 1);
    rendered_samples = SignalProducer::produce<Synth>(synth, 1);

    assert_close(
        expected_samples[0],
        rendered_samples[0],
        block_size,
        0.001,
        "channel=0, mode=%d",
        (int)mode
    );
    assert_close(
        expected_samples[1],
        rendered_samples[1],
        block_size,
        0.001,
        "channel=1, mode=%d",
        (int)mode
    );
}


TEST(operating_mode, {
    test_operating_mode(
        OUT_VOLUME_PER_CHANNEL, OUT_VOLUME_PER_CHANNEL, Synth::MIX_AND_MOD
    );
    test_operating_mode(OUT_VOLUME_PER_CHANNEL, 0, Synth::SPLIT_AT_C4);
    test_operating_mode(0, OUT_VOLUME_PER_CHANNEL, Synth::SPLIT_AT_C3);
})


TEST(all_sound_off_message_turns_off_all_sounds_immediately, {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;

    Synth synth;
    Constant expected(0.0, synth.get_channels());
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    synth.resume();

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    synth.note_on(0.0, 0, Midi::NOTE_A_5, 127);
    synth.all_sound_off(1.0 / sample_rate, 1);

    expected_samples = SignalProducer::produce<Constant>(expected, 1);
    rendered_samples = SignalProducer::produce<Synth>(synth, 1);

    assert_eq(expected_samples[0], rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(expected_samples[1], rendered_samples[1], block_size, DOUBLE_DELTA);
})


TEST(all_notes_off_message_turns_off_all_notes_at_the_specified_time, {
    constexpr Integer block_size = 4096;
    constexpr Integer half_a_second = block_size / 2;
    constexpr Frequency sample_rate = 4096.0;

    Synth synth;
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL, 110.0,
        OUT_VOLUME_PER_CHANNEL, 220.0,
        0.0, 0.0,
        synth.get_channels()
    );
    Sample const* const* rendered_samples;
    Sample const* const* sines;
    Sample* expected_samples = new Sample[block_size];

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    synth.resume();

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);

    synth.carrier_params.amplitude.set_value(0.0);
    synth.carrier_params.volume.set_value(0.0);

    synth.note_on(0.0, 2, Midi::NOTE_A_2, 127);
    synth.note_on(0.0, 3, Midi::NOTE_A_3, 127);
    synth.all_notes_off(0.5, 1);

    sines = SignalProducer::produce<SumOfSines>(expected, 1);
    rendered_samples = SignalProducer::produce<Synth>(synth, 1);

    for (Integer i = 0; i != block_size; ++i) {
        expected_samples[i] = i < half_a_second ? sines[0][i] : 0.0;
    }

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(expected_samples, rendered_samples[1], block_size, DOUBLE_DELTA);

    delete[] expected_samples;
})


TEST(when_a_param_has_the_learn_controller_assigned_then_the_controller_gets_replaced_by_the_first_supported_changing_midi_controller, {
    Synth synth;

    assign_controller(synth, Synth::ParamId::CVOL, Synth::ControllerId::MIDI_LEARN);
    assign_controller(synth, Synth::ParamId::MVOL, Synth::ControllerId::MIDI_LEARN);
    assign_controller(synth, Synth::ParamId::MWAV, Synth::ControllerId::MIDI_LEARN);

    synth.process_messages();

    assert_eq(
        Synth::ControllerId::MIDI_LEARN,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
    assert_eq(
        Synth::ControllerId::MIDI_LEARN,
        synth.get_param_controller_id_atomic(Synth::ParamId::MVOL)
    );
    assert_eq(
        Synth::ControllerId::MIDI_LEARN,
        synth.get_param_controller_id_atomic(Synth::ParamId::MWAV)
    );

    synth.control_change(0.000001, 1, UNSUPPORTED_CC, 12);
    synth.control_change(0.000002, 1, Midi::GENERAL_1, 25);
    synth.control_change(0.000003, 1, Midi::GENERAL_2, 38);

    assert_eq(
        Synth::ControllerId::GENERAL_1,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
    assert_eq(
        Synth::ControllerId::GENERAL_1,
        synth.get_param_controller_id_atomic(Synth::ParamId::MVOL)
    );
    assert_eq(
        Synth::ControllerId::GENERAL_1,
        synth.get_param_controller_id_atomic(Synth::ParamId::MWAV)
    );
    assert_neq(NULL, synth.modulator_params.volume.get_midi_controller());
    assert_neq(NULL, synth.carrier_params.volume.get_midi_controller());
    assert_eq(25.0 / 127.0, synth.modulator_params.volume.get_value(), DOUBLE_DELTA);
    assert_eq(25.0 / 127.0, synth.carrier_params.volume.get_value(), DOUBLE_DELTA);
});


TEST(unsupported_controllers_cannot_be_assigned, {
    Synth synth;

    assign_controller(
        synth, Synth::ParamId::MWAV, Synth::ControllerId::MODULATION_WHEEL
    );
    assign_controller(
        synth, Synth::ParamId::MVOL, Synth::ControllerId::MODULATION_WHEEL
    );
    assign_controller(synth, Synth::ParamId::MWAV, UNSUPPORTED_CC);
    assign_controller(synth, Synth::ParamId::MVOL, UNSUPPORTED_CC);
    assign_controller(synth, Synth::ParamId::CWAV, UNSUPPORTED_CC);
    assign_controller(synth, Synth::ParamId::CVOL, UNSUPPORTED_CC);

    synth.process_messages();

    assert_eq(
        Synth::ControllerId::MODULATION_WHEEL,
        synth.get_param_controller_id_atomic(Synth::ParamId::MWAV)
    );
    assert_eq(
        Synth::ControllerId::MODULATION_WHEEL,
        synth.get_param_controller_id_atomic(Synth::ParamId::MVOL)
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CWAV)
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );
})


TEST(toggles_cannot_have_controllers_assigned_to_them, {
    Synth synth;

    assign_controller(
        synth, Synth::ParamId::MF1LOG, Synth::ControllerId::MODULATION_WHEEL
    );
    assign_controller(
        synth, Synth::ParamId::L1SYN, Synth::ControllerId::MIDI_LEARN
    );

    synth.process_messages();

    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::MF1LOG)
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::L1SYN)
    );
})


TEST(when_the_same_controller_message_is_received_over_multiple_channels_then_only_one_is_processed, {
    Synth synth;

    synth.control_change(0.1, 1, Midi::VOLUME, 53);
    synth.control_change(0.1, 2, Midi::VOLUME, 53);
    synth.pitch_wheel_change(0.1, 1, 10000);
    synth.control_change(0.1, 3, Midi::VOLUME, 53);
    synth.pitch_wheel_change(0.1, 2, 10000);
    synth.channel_pressure(0.1, 1, 100);
    synth.pitch_wheel_change(0.1, 3, 10000);
    synth.channel_pressure(0.1, 2, 100);
    synth.channel_pressure(0.1, 3, 100);

    assert_eq(1, synth.midi_controllers[Midi::VOLUME]->events.length());
    assert_eq(1, synth.pitch_wheel.events.length());
    assert_eq(1, synth.channel_pressure_ctl.events.length());
})


TEST(when_synth_state_is_cleared_then_lfos_are_started_again, {
    Synth synth;

    synth.resume();

    assign_controller(synth, Synth::ParamId::EEDRY, Synth::ControllerId::LFO_1);
    SignalProducer::produce<Synth>(synth, 1);
    assert_true(synth.lfos[0]->is_on());

    synth.push_message(CLEAR, Synth::ParamId::MAX_PARAM_ID, 0.0, 0);
    assign_controller(synth, Synth::ParamId::EEDRY, Synth::ControllerId::LFO_1);
    SignalProducer::produce<Synth>(synth, 2);

    assert_true(synth.lfos[0]->is_on());
})


TEST(effects, {
    constexpr Frequency sample_rate = 22050.0;
    constexpr Integer block_size = 2048;
    constexpr Integer rounds = 10;
    constexpr Integer buffer_size = rounds * block_size;

    Synth synth;
    Buffer buffer(buffer_size, synth.get_channels());

    Number const inv_saw_as_ratio = (
        synth.modulator_params.waveform.value_to_ratio(
            SimpleOscillator::INVERSE_SAWTOOTH
        )
    );

    synth.set_sample_rate(sample_rate);
    synth.set_block_size(block_size);

    synth.resume();

    set_param(synth, Synth::ParamId::MWAV, inv_saw_as_ratio);
    set_param(synth, Synth::ParamId::CWAV, inv_saw_as_ratio);
    set_param(synth, Synth::ParamId::EOG, 0.2);
    set_param(synth, Synth::ParamId::EDG, 0.2);
    set_param(synth, Synth::ParamId::EF1FRQ, 0.75);
    set_param(synth, Synth::ParamId::EF2FRQ, 0.75);
    set_param(synth, Synth::ParamId::ECDPT, 1.0);
    set_param(synth, Synth::ParamId::ECWET, 0.5);
    set_param(synth, Synth::ParamId::ECDRY, 0.5);
    set_param(synth, Synth::ParamId::EEWET, 0.5);
    set_param(synth, Synth::ParamId::EEDRY, 0.5);
    set_param(synth, Synth::ParamId::ERWET, 0.5);
    set_param(synth, Synth::ParamId::ERDRY, 0.5);
    set_param(synth, Synth::ParamId::L1CEN, 1.0);

    assign_controller(synth, Synth::ParamId::EF1Q, Synth::ControllerId::LFO_1);

    synth.note_on(0.0, 1, Midi::NOTE_A_4, 114);

    synth.process_messages();

    render_rounds<Synth>(synth, buffer, rounds, block_size);
})


TEST(sustain_pedal, {
    constexpr Frequency sample_rate = 3000.0;
    constexpr Seconds note_on = 0.0;
    constexpr Seconds sustain_on = 0.1;
    constexpr Seconds note_off = 0.2;
    constexpr Seconds sustain_off = 1.0;
    constexpr Integer block_size = 4196;
    Synth synth_1;
    Synth synth_2;
    Sample const* const* synth_1_samples;
    Sample const* const* synth_2_samples;

    synth_1.set_sample_rate(sample_rate);
    synth_2.set_sample_rate(sample_rate);

    synth_1.set_block_size(block_size);
    synth_2.set_block_size(block_size);

    synth_1.resume();
    synth_2.resume();

    synth_1.note_on(note_on, 1, Midi::NOTE_A_3, 114);
    synth_1.note_off(sustain_off, 1, Midi::NOTE_A_3, 114);

    synth_2.note_on(note_on, 1, Midi::NOTE_A_3, 114);
    synth_2.control_change(sustain_on, 1, Midi::SUSTAIN_PEDAL, 127);
    synth_2.note_off(note_off, 1, Midi::NOTE_A_3, 114);
    synth_2.control_change(sustain_off, 1, Midi::SUSTAIN_PEDAL, 0);

    synth_1_samples = SignalProducer::produce<Synth>(synth_1, 1);
    synth_2_samples = SignalProducer::produce<Synth>(synth_2, 1);

    for (Integer c = 0; c != synth_1.get_channels(); ++c) {
        assert_eq(
            synth_1_samples[c],
            synth_2_samples[c],
            block_size,
            "channel=%d",
            (int)c
        );
    }
})


TEST(decaying_voices_are_garbage_collected, {
    constexpr Seconds note_start = 0.002;
    constexpr Seconds decay_time = 0.001;
    constexpr Seconds hold_time = 1.0;
    constexpr Seconds sustain_start = note_start + hold_time + decay_time;

    Synth synth(0);
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL, 220.0,
        0.0, 0.0,
        0.0, 0.0,
        synth.get_channels()
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;
    Integer number_of_rendered = 0;
    Integer round = 0;

    Integer const sustain_start_samples = (
        (Integer)std::ceil(sustain_start * synth.get_sample_rate())
    );

    set_param(synth, Synth::ParamId::MAMP, 0.5);
    set_param(synth, Synth::ParamId::CAMP, 0.5);

    set_param(synth, Synth::ParamId::N1DYN, 0.0);
    set_param(synth, Synth::ParamId::N1AMT, 1.0);
    set_param(synth, Synth::ParamId::N1INI, 0.0);
    set_param(synth, Synth::ParamId::N1DEL, 0.0);
    set_param(synth, Synth::ParamId::N1ATK, 0.0);
    set_param(synth, Synth::ParamId::N1PK, 1.0);
    set_param(synth, Synth::ParamId::N1HLD, synth.envelopes[0]->hold_time.value_to_ratio(hold_time));
    set_param(synth, Synth::ParamId::N1DEC, synth.envelopes[0]->decay_time.value_to_ratio(decay_time));
    set_param(synth, Synth::ParamId::N1SUS, 0.0);
    set_param(synth, Synth::ParamId::N1REL, 1.0);
    set_param(synth, Synth::ParamId::N1FIN, 0.0);

    assign_controller(synth, Synth::ParamId::MVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_1);

    synth.process_messages();

    synth.control_change(note_start, 1, Midi::SUSTAIN_PEDAL, 127);

    for (Integer i = 0; i != Synth::POLYPHONY; ++i) {
        synth.note_on(note_start, 1, Midi::NOTE_A_5, 100);
    }

    while (number_of_rendered < sustain_start_samples) {
        SignalProducer::produce<Synth>(synth, round);
        number_of_rendered += synth.get_block_size();
        ++round;
    }

    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);

    rendered_samples = SignalProducer::produce<Synth>(synth, round);
    expected_samples = SignalProducer::produce<SumOfSines>(expected, round);

    assert_eq(expected_samples[0], rendered_samples[0], synth.get_block_size(), DOUBLE_DELTA);
    assert_eq(expected_samples[0], rendered_samples[1], synth.get_block_size(), DOUBLE_DELTA);
})


void set_up_quickly_decaying_envelope(Synth& synth)
{
    set_param(synth, Synth::ParamId::N1DYN, 0.0);
    set_param(synth, Synth::ParamId::N1AMT, 1.0);
    set_param(synth, Synth::ParamId::N1INI, 0.0);
    set_param(synth, Synth::ParamId::N1DEL, 0.0);
    set_param(synth, Synth::ParamId::N1ATK, 0.0);
    set_param(synth, Synth::ParamId::N1PK, 1.0);
    set_param(synth, Synth::ParamId::N1HLD, 0.0);
    set_param(synth, Synth::ParamId::N1DEC, 0.0001);
    set_param(synth, Synth::ParamId::N1SUS, 0.0);
    set_param(synth, Synth::ParamId::N1REL, 0.0);
    set_param(synth, Synth::ParamId::N1FIN, 0.0);

    assign_controller(synth, Synth::ParamId::MVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_1);
}


TEST(garbage_collector_does_not_deallocate_newly_triggered_note_instead_of_decayed_clone_while_sustaining, {
    constexpr Integer block_size = 2048;

    Synth synth(0);
    Sample const* const* rendered_samples = SignalProducer::produce<Synth>(synth, 3);
    Sample* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(22050.0);

    set_param(synth, Synth::ParamId::MAMP, 0.5);
    set_param(synth, Synth::ParamId::CAMP, 0.5);

    set_up_quickly_decaying_envelope(synth);

    synth.process_messages();

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_on(0.001, 1, Midi::NOTE_A_3, 100);
    SignalProducer::produce<Synth>(synth, 1); /* note starts then decays */

    set_param(synth, Synth::ParamId::N1DEC, 0.03);
    synth.process_messages();

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 100); /* note off is deferred due to sustain pedal */
    synth.note_on(0.001, 1, Midi::NOTE_A_3, 100); /* second voice assigned to the same note */
    SignalProducer::produce<Synth>(synth, 2); /* first voice gets garbage collected */

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 100); /* also deferred */
    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 0); /* second voice should be released */

    rendered_samples = SignalProducer::produce<Synth>(synth, 3);

    expected_samples = new Sample[block_size];
    std::fill_n(expected_samples, block_size, 0.0);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(expected_samples, rendered_samples[1], block_size, DOUBLE_DELTA);

    delete[] expected_samples;
})


TEST(garbage_collected_and_deferred_stopped_reallocated_notes_are_not_released_again_when_sustain_pedal_is_lifted, {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;

    Synth synth(0);
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL, 220.0,
        0.0, 0.0,
        0.0, 0.0,
        synth.get_channels()
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    set_param(synth, Synth::ParamId::MAMP, 0.5);
    set_param(synth, Synth::ParamId::CAMP, 0.5);

    set_up_quickly_decaying_envelope(synth);

    synth.process_messages();

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_on(0.000001, 1, Midi::NOTE_A_3, 127);
    SignalProducer::produce<Synth>(synth, 1); /* note starts then decays */

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 127); /* note off is deferred due to sustain pedal */
    SignalProducer::produce<Synth>(synth, 2); /* voice gets garbage collected */

    set_param(synth, Synth::ParamId::N1HLD, 1.0);
    synth.process_messages();

    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);
    synth.control_change(0.000001, 1, Midi::SUSTAIN_PEDAL, 0); /* second voice should keep ringing */

    rendered_samples = SignalProducer::produce<Synth>(synth, 3);
    expected_samples = SignalProducer::produce<SumOfSines>(expected, 3);

    assert_eq(expected_samples[0], rendered_samples[0], block_size, 0.001);
    assert_eq(expected_samples[1], rendered_samples[1], block_size, 0.001);
})


TEST(note_off_stops_notes_that_are_triggered_multiple_times_during_sustaining, {
    constexpr Frequency sample_rate = 3000.0;
    constexpr Integer block_size = 3000;
    Synth synth;
    Constant expected(0.0, synth.get_channels());
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_sample_rate(sample_rate);
    synth.set_block_size(block_size);

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    synth.resume();

    synth.control_change(0.01, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_on(0.02, 1, Midi::NOTE_A_3, 127);
    synth.note_off(0.03, 1, Midi::NOTE_A_3, 127);
    synth.note_on(0.04, 1, Midi::NOTE_A_3, 127);
    synth.note_off(0.05, 1, Midi::NOTE_A_3, 127);
    synth.note_on(0.06, 1, Midi::NOTE_A_3, 127);
    synth.control_change(0.07, 1, Midi::SUSTAIN_PEDAL, 0);
    synth.note_off(0.08, 1, Midi::NOTE_A_3, 127);

    SignalProducer::produce<Synth>(synth, 1, block_size / 10);

    rendered_samples = SignalProducer::produce<Synth>(synth, 2);
    expected_samples = SignalProducer::produce<Constant>(expected, 2);

    for (Integer c = 0; c != synth.get_channels(); ++c) {
        assert_eq(
            expected_samples[c],
            rendered_samples[c],
            block_size,
            "channel=%d",
            (int)c
        );
    }
})


TEST(sustain_off_leaves_garbage_collected_and_deferred_stopped_and_reallocated_note_ringing_if_key_is_still_held_down, {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;

    Synth synth(0);
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL, 220.0,
        0.0, 0.0,
        0.0, 0.0,
        synth.get_channels()
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    set_param(synth, Synth::ParamId::MAMP, 0.5);
    set_param(synth, Synth::ParamId::CAMP, 0.5);

    set_up_quickly_decaying_envelope(synth);

    synth.process_messages();

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_on(0.000001, 1, Midi::NOTE_A_3, 127);
    SignalProducer::produce<Synth>(synth, 1); /* note starts then decays */
    SignalProducer::produce<Synth>(synth, 2); /* voice gets garbage collected */

    set_param(synth, Synth::ParamId::N1HLD, 1.0);
    synth.process_messages();

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 127); /* note off is deferred due to sustain pedal */
    synth.note_on(0.0000001, 1, Midi::NOTE_A_3, 127); /* new voice allocated to the note */
    synth.control_change(0.0000002, 1, Midi::SUSTAIN_PEDAL, 0); /* second voice should keep ringing */

    rendered_samples = SignalProducer::produce<Synth>(synth, 3);
    expected_samples = SignalProducer::produce<SumOfSines>(expected, 3);

    assert_eq(expected_samples[0], rendered_samples[0], block_size, 0.001);
    assert_eq(expected_samples[1], rendered_samples[1], block_size, 0.001);
})
