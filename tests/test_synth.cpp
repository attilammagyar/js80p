/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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
#include <vector>

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
constexpr Synth::MessageType CLEAR_DIRTY_FLAG = Synth::MessageType::CLEAR_DIRTY_FLAG;


constexpr Integer PEAK_CTL_TEST_BLOCK_SIZE = 8192;
constexpr Number PEAK_CTL_TEST_OSC_1_VOL = 0.9;
constexpr Number PEAK_CTL_TEST_OSC_2_VOL = 0.6;
constexpr Number PEAK_CTL_TEST_FILTER_1_GAIN = -6.0;
constexpr Number PEAK_CTL_TEST_REVERB_DRY = 0.1;


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

        Frequency get_frequency(Byte const tuning, Midi::Note const note) const
        {
            return frequencies[tuning][note];
        }
};


TEST(twelve_tone_equal_temperament_440_hz, {
    FrequenciesTestSynth synth;

    assert_eq(880.0, synth.get_frequency(Modulator::TUNING_440HZ_12TET, Midi::NOTE_A_5), DOUBLE_DELTA);
    assert_eq(440.0, synth.get_frequency(Modulator::TUNING_440HZ_12TET, Midi::NOTE_A_4), DOUBLE_DELTA);
    assert_eq(220.0, synth.get_frequency(Modulator::TUNING_440HZ_12TET, Midi::NOTE_A_3), DOUBLE_DELTA);

    assert_eq(864.0, synth.get_frequency(Modulator::TUNING_432HZ_12TET, Midi::NOTE_A_5), DOUBLE_DELTA);
    assert_eq(432.0, synth.get_frequency(Modulator::TUNING_432HZ_12TET, Midi::NOTE_A_4), DOUBLE_DELTA);
    assert_eq(216.0, synth.get_frequency(Modulator::TUNING_432HZ_12TET, Midi::NOTE_A_3), DOUBLE_DELTA);

    assert_eq(12543.85, synth.get_frequency(Modulator::TUNING_440HZ_12TET, Midi::NOTE_G_9), 0.01);
    assert_eq(261.63, synth.get_frequency(Modulator::TUNING_440HZ_12TET, Midi::NOTE_C_4), 0.01);
    assert_eq(8.18, synth.get_frequency(Modulator::TUNING_440HZ_12TET, Midi::NOTE_0), 0.01);
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
        synth, Synth::ParamId::MAMP, Synth::ControllerId::TRIGGERED_VELOCITY
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

    assert_lte((int)max_collisions, 7);
    assert_lte(avg_bucket_size, 3.0);
    assert_lte(avg_collisions, 3.5);

    assert_eq(Synth::ParamId::INVALID_PARAM_ID, synth.get_param_id(""));
    assert_eq(Synth::ParamId::INVALID_PARAM_ID, synth.get_param_id(" \n"));
    assert_eq(Synth::ParamId::INVALID_PARAM_ID, synth.get_param_id("NO_SUCH_PARAM"));

    for (int i = 0; i != Synth::ParamId::PARAM_ID_COUNT; ++i) {
        std::string const name = synth.get_param_name((Synth::ParamId)i);
        Synth::ParamId const param_id = synth.get_param_id(name);
        assert_eq((Synth::ParamId)i, param_id, "i=%d, name=\"%s\"", i, name);
    }
})


void test_operating_mode(
        Number const expected_vol_a3,
        Number const expected_vol_a5,
        Byte const mode
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
        OUT_VOLUME_PER_CHANNEL, OUT_VOLUME_PER_CHANNEL, Synth::MODE_MIX_AND_MOD
    );
    test_operating_mode(OUT_VOLUME_PER_CHANNEL, 0, Synth::MODE_SPLIT_AT_C4);
    test_operating_mode(0, OUT_VOLUME_PER_CHANNEL, Synth::MODE_SPLIT_AT_C3);
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
    Sample* const expected_samples = new Sample[block_size];

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


TEST(all_notes_off_message_clears_note_stack, {
    constexpr Integer block_size = 4096;
    constexpr Frequency sample_rate = 4096.0;

    Synth synth;
    Sample const* const* rendered_samples;
    Sample* const expected_samples = new Sample[block_size];

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    synth.resume();

    synth.note_handling.set_value(Synth::NOTE_HANDLING_MONOPHONIC);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);

    synth.carrier_params.amplitude.set_value(0.0);
    synth.carrier_params.volume.set_value(0.0);

    synth.note_on(0.0, 2, Midi::NOTE_A_2, 127);
    synth.note_on(0.0, 3, Midi::NOTE_A_3, 127);
    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_off(0.0, 3, Midi::NOTE_A_3, 127);
    synth.all_notes_off(0.0, 1);
    synth.control_change(0.1, 1, Midi::SUSTAIN_PEDAL, 0);

    std::fill_n(expected_samples, block_size, 0.0);
    rendered_samples = SignalProducer::produce<Synth>(synth, 1);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(expected_samples, rendered_samples[1], block_size, DOUBLE_DELTA);

    delete[] expected_samples;
})


TEST(when_a_param_has_the_learn_controller_assigned_then_the_controller_gets_replaced_by_the_first_supported_changing_midi_controller, {
    Synth synth;

    assign_controller(synth, Synth::ParamId::CC1, Synth::ControllerId::MIDI_LEARN);
    assign_controller(synth, Synth::ParamId::MC1, Synth::ControllerId::MIDI_LEARN);
    assign_controller(synth, Synth::ParamId::MWAV, Synth::ControllerId::MIDI_LEARN);

    synth.process_messages();

    assert_eq(
        Synth::ControllerId::MIDI_LEARN,
        synth.get_param_controller_id_atomic(Synth::ParamId::CC1)
    );
    assert_eq(
        Synth::ControllerId::MIDI_LEARN,
        synth.get_param_controller_id_atomic(Synth::ParamId::MC1)
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
        synth.get_param_controller_id_atomic(Synth::ParamId::CC1)
    );
    assert_eq(
        Synth::ControllerId::GENERAL_1,
        synth.get_param_controller_id_atomic(Synth::ParamId::MC1)
    );
    assert_eq(
        Synth::ControllerId::GENERAL_1,
        synth.get_param_controller_id_atomic(Synth::ParamId::MWAV)
    );
    assert_neq(NULL, synth.modulator_params.harmonic_0.get_midi_controller());
    assert_neq(NULL, synth.carrier_params.harmonic_0.get_midi_controller());
    assert_eq(25.0 / 127.0, synth.modulator_params.harmonic_0.get_ratio(), DOUBLE_DELTA);
    assert_eq(25.0 / 127.0, synth.carrier_params.harmonic_0.get_ratio(), DOUBLE_DELTA);
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
    SignalProducer::produce<Synth>(synth, 1, 1);
    assert_true(synth.lfos[0]->is_on());

    synth.push_message(CLEAR, Synth::ParamId::INVALID_PARAM_ID, 0.0, 0);
    assign_controller(synth, Synth::ParamId::EEDRY, Synth::ControllerId::LFO_1);
    SignalProducer::produce<Synth>(synth, 2, 1);

    assert_true(synth.lfos[0]->is_on());
})


TEST(when_synth_state_is_cleared_then_tuning_is_restored, {
    Synth synth;

    synth.resume();

    synth.modulator_params.tuning.set_value(Modulator::TUNING_432HZ_12TET);
    synth.carrier_params.tuning.set_value(Carrier::TUNING_432HZ_12TET);

    synth.push_message(CLEAR, Synth::ParamId::INVALID_PARAM_ID, 0.0, 0);

    SignalProducer::produce<Synth>(synth, 1, 1);

    assert_eq((int)Modulator::TUNING_432HZ_12TET, (int)synth.modulator_params.tuning.get_value());
    assert_eq((int)Carrier::TUNING_432HZ_12TET, (int)synth.carrier_params.tuning.get_value());
})


TEST(effects_simple, {
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
    set_param(synth, Synth::ParamId::ED1L, 0.2);
    set_param(synth, Synth::ParamId::ED2L, 0.2);
    set_param(synth, Synth::ParamId::EF1FRQ, 0.75);
    set_param(synth, Synth::ParamId::EF2FRQ, 0.75);
    set_param(synth, Synth::ParamId::ETHSS, 0.2);
    set_param(synth, Synth::ParamId::ECWET, 0.5);
    set_param(synth, Synth::ParamId::ECDRY, 0.5);
    set_param(synth, Synth::ParamId::EEWET, 0.5);
    set_param(synth, Synth::ParamId::EEDRY, 0.5);
    set_param(synth, Synth::ParamId::ERWET, 0.5);
    set_param(synth, Synth::ParamId::ERDRY, 0.5);

    synth.note_on(0.0, 1, Midi::NOTE_A_4, 114);

    synth.process_messages();

    render_rounds<Synth>(synth, buffer, rounds, block_size);
})


TEST(effects_complex, {
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

    set_param(synth, Synth::ParamId::EV1V, 0.6);

    set_param(synth, Synth::ParamId::ED1L, 0.2);
    set_param(synth, Synth::ParamId::ED2L, 0.2);
    set_param(synth, Synth::ParamId::EF1FRQ, 0.75);
    set_param(synth, Synth::ParamId::EF2LOG, 1.0);
    set_param(synth, Synth::ParamId::EF2FRQ, 0.9);

    set_param(synth, Synth::ParamId::EV2V, 0.8);

    set_param(synth, Synth::ParamId::ETSAT, 0.5);
    set_param(synth, Synth::ParamId::ETWFA, 0.3);
    set_param(synth, Synth::ParamId::ETWFS, 0.2);
    set_param(synth, Synth::ParamId::ETCLR, 0.6);
    set_param(synth, Synth::ParamId::ETHSS, 0.2);
    set_param(synth, Synth::ParamId::ETSTR, 0.1);

    set_param(synth, Synth::ParamId::ECDPT, 1.0);
    set_param(synth, Synth::ParamId::ECFB, 0.1);
    set_param(synth, Synth::ParamId::ECDF, 0.8);
    set_param(synth, Synth::ParamId::ECWID, 0.9);
    set_param(synth, Synth::ParamId::ECHPF, 0.1);
    set_param(synth, Synth::ParamId::ECDRY, 0.5);
    set_param(synth, Synth::ParamId::ECWET, 0.5);
    set_param(synth, Synth::ParamId::ECDRY, 0.5);

    set_param(synth, Synth::ParamId::EELOG, 1.0);
    set_param(synth, Synth::ParamId::EEINV, 0.6);
    set_param(synth, Synth::ParamId::EEFB, 0.2);
    set_param(synth, Synth::ParamId::EEDST, 0.5);
    set_param(synth, Synth::ParamId::EEDF, 0.8);
    set_param(synth, Synth::ParamId::EEWID, 0.8);
    set_param(synth, Synth::ParamId::EEHPF, 0.2);
    set_param(synth, Synth::ParamId::EECR, 0.1);
    set_param(synth, Synth::ParamId::EEWET, 0.5);
    set_param(synth, Synth::ParamId::EEDRY, 0.5);

    set_param(synth, Synth::ParamId::ERLOG, 1.0);
    set_param(synth, Synth::ParamId::ERRS, 0.5);
    set_param(synth, Synth::ParamId::ERDST, 0.1);
    set_param(synth, Synth::ParamId::ERDF, 0.8);
    set_param(synth, Synth::ParamId::ERWET, 0.3);
    set_param(synth, Synth::ParamId::ERHPF, 0.2);
    set_param(synth, Synth::ParamId::ERCR, 0.2);
    set_param(synth, Synth::ParamId::ERWET, 0.5);
    set_param(synth, Synth::ParamId::ERDRY, 0.5);

    set_param(synth, Synth::ParamId::EV3V, 0.8);

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


TEST(hold_note, {
    constexpr Frequency sample_rate = 3000.0;
    constexpr Seconds note_on = 0.0;
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

    set_param(
        synth_2,
        Synth::ParamId::NH,
        synth_2.note_handling.value_to_ratio(Synth::NOTE_HANDLING_POLYPHONIC_HOLD)
    );
    synth_2.process_messages();

    synth_2.note_on(note_on, 1, Midi::NOTE_A_3, 114);
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

    set_param(synth, Synth::ParamId::N1UPD, 0.0);
    set_param(synth, Synth::ParamId::N1SCL, 1.0);
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
    set_param(synth, Synth::ParamId::N1UPD, 0.0);
    set_param(synth, Synth::ParamId::N1SCL, 1.0);
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


void test_gc_decayed_vs_new_note_while_sustaining(
        void(*sustain_on)(Synth& synth),
        void(*sustain_off)(Synth& synth),
        char const* const sustain_type
) {
    constexpr Integer block_size = 2048;

    Synth synth(0);
    Sample const* const* rendered_samples = SignalProducer::produce<Synth>(synth, 3);
    Sample* const expected_samples = new Sample[block_size];

    synth.set_block_size(block_size);
    synth.set_sample_rate(22050.0);

    set_param(synth, Synth::ParamId::MAMP, 0.5);
    set_param(synth, Synth::ParamId::CAMP, 0.5);

    set_up_quickly_decaying_envelope(synth);

    synth.process_messages();

    sustain_on(synth);
    synth.note_on(0.001, 1, Midi::NOTE_A_3, 100);
    SignalProducer::produce<Synth>(synth, 1); /* note starts then decays */

    set_param(synth, Synth::ParamId::N1DEC, 0.03);
    synth.process_messages();

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 100); /* note off is deferred due to sustain pedal */
    synth.note_on(0.001, 1, Midi::NOTE_A_3, 100); /* second voice assigned to the same note */
    SignalProducer::produce<Synth>(synth, 2); /* first voice gets garbage collected */

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 100); /* also deferred */
    sustain_off(synth); /* second voice should be released */

    rendered_samples = SignalProducer::produce<Synth>(synth, 3);

    std::fill_n(expected_samples, block_size, 0.0);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA, sustain_type);
    assert_eq(expected_samples, rendered_samples[1], block_size, DOUBLE_DELTA, sustain_type);

    delete[] expected_samples;
}


void sustain_pedal_on(Synth& synth)
{
    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
}


void sustain_pedal_off(Synth& synth)
{
    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 0);
}


void hold_mode_on(Synth& synth)
{
    set_param(
        synth,
        Synth::ParamId::NH,
        synth.note_handling.value_to_ratio(Synth::NOTE_HANDLING_POLYPHONIC_HOLD)
    );
}


void hold_mode_off(Synth& synth)
{
    set_param(
        synth,
        Synth::ParamId::NH,
        synth.note_handling.value_to_ratio(Synth::NOTE_HANDLING_POLYPHONIC)
    );
}


TEST(garbage_collector_does_not_deallocate_newly_triggered_note_instead_of_decayed_clone_while_sustaining, {
    test_gc_decayed_vs_new_note_while_sustaining(&sustain_pedal_on, &sustain_pedal_off, "pedal");
    test_gc_decayed_vs_new_note_while_sustaining(&hold_mode_on, &hold_mode_off, "hold");
})


void test_gc_reallocated_notes_while_sustaining(
        void(*sustain_on)(Synth& synth),
        void(*sustain_off)(Synth& synth),
        char const* const sustain_type
) {
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

    sustain_on(synth);
    synth.note_on(0.000001, 1, Midi::NOTE_A_3, 127);
    SignalProducer::produce<Synth>(synth, 1); /* note starts then decays */

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 127); /* note off is deferred due to sustain pedal */
    SignalProducer::produce<Synth>(synth, 2); /* voice gets garbage collected */

    set_param(synth, Synth::ParamId::N1HLD, 1.0);
    synth.process_messages();

    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);
    sustain_off(synth); /* second voice should keep ringing */

    rendered_samples = SignalProducer::produce<Synth>(synth, 3);
    expected_samples = SignalProducer::produce<SumOfSines>(expected, 3);

    assert_eq(expected_samples[0], rendered_samples[0], block_size, 0.001, sustain_type);
    assert_eq(expected_samples[1], rendered_samples[1], block_size, 0.001, sustain_type);
}


TEST(garbage_collected_and_deferred_stopped_reallocated_notes_are_not_released_again_when_sustain_pedal_is_lifted, {
    test_gc_reallocated_notes_while_sustaining(&sustain_pedal_on, &sustain_pedal_off, "pedal");
    test_gc_reallocated_notes_while_sustaining(&hold_mode_on, &hold_mode_off, "hold");
})


void test_note_off_notes_that_are_triggered_multiple_times_during_sustaining(
        void(*sustain_on)(Synth& synth),
        void(*sustain_off)(Synth& synth),
        char const* const sustain_type
) {
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

    sustain_on(synth);
    synth.note_on(0.02, 1, Midi::NOTE_A_3, 127);
    synth.note_off(0.03, 1, Midi::NOTE_A_3, 127);
    synth.note_on(0.04, 1, Midi::NOTE_A_3, 127);
    synth.note_off(0.05, 1, Midi::NOTE_A_3, 127);
    synth.note_on(0.06, 1, Midi::NOTE_A_3, 127);
    sustain_off(synth);
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
}


TEST(note_off_stops_notes_that_are_triggered_multiple_times_during_sustaining, {
    test_note_off_notes_that_are_triggered_multiple_times_during_sustaining(
        &sustain_pedal_on, &sustain_pedal_off, "pedal"
    );
    test_note_off_notes_that_are_triggered_multiple_times_during_sustaining(
        &hold_mode_on, &hold_mode_off, "hold"
    );
})


void test_sustain_off_while_key_is_still_held(
        void(*sustain_on)(Synth& synth),
        void(*sustain_off)(Synth& synth),
        char const* const sustain_type
) {
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

    sustain_on(synth);
    synth.note_on(0.000001, 1, Midi::NOTE_A_3, 127);
    SignalProducer::produce<Synth>(synth, 1); /* note starts then decays */
    SignalProducer::produce<Synth>(synth, 2); /* voice gets garbage collected */

    set_param(synth, Synth::ParamId::N1HLD, 1.0);
    synth.process_messages();

    synth.note_off(0.0, 1, Midi::NOTE_A_3, 127); /* note off is deferred due to sustain pedal */
    synth.note_on(0.0000001, 1, Midi::NOTE_A_3, 127); /* new voice allocated to the note */
    sustain_off(synth); /* second voice should keep ringing */

    rendered_samples = SignalProducer::produce<Synth>(synth, 3);
    expected_samples = SignalProducer::produce<SumOfSines>(expected, 3);

    assert_eq(expected_samples[0], rendered_samples[0], block_size, 0.001);
    assert_eq(expected_samples[1], rendered_samples[1], block_size, 0.001);
}


TEST(sustain_off_leaves_garbage_collected_and_deferred_stopped_and_reallocated_note_ringing_if_key_is_still_held_down, {
    test_sustain_off_while_key_is_still_held(
        &sustain_pedal_on,
        [](Synth& synth) -> void {
            synth.control_change(0.0000002, 1, Midi::SUSTAIN_PEDAL, 0);
        },
        "pedal"
    );
    test_sustain_off_while_key_is_still_held(&hold_mode_on, &hold_mode_off, "hold");
})


void assert_message_dirtiness(
        Synth& synth,
        Synth::MessageType const message_type,
        bool const expected_dirtiness
) {
    assert_false(
        synth.is_dirty(),
        "Expected synth not to be dirty before sending message; message=%d",
        (int)message_type
    );

    synth.push_message(
        message_type, Synth::ParamId::MVOL, 0.123, Synth::ControllerId::MACRO_1
    );
    assert_false(
        synth.is_dirty(),
        "Expected synth not to become dirty before processing message; message=%d",
        (int)message_type
    );

    synth.process_messages();

    if (expected_dirtiness) {
        assert_true(
            synth.is_dirty(),
            "Expected synth to become dirty after processing message; message=%d",
            (int)message_type
        );
    } else {
        assert_false(
            synth.is_dirty(),
            "Expected synth not to become dirty after processing message; message=%d",
            (int)message_type
        );
    }

    synth.clear_dirty_flag();
    assert_false(
        synth.is_dirty(),
        "Expected synth not to remain dirty after clearing the flag; message=%d",
        (int)message_type
    );
}


TEST(when_synth_config_changes_then_synth_becomes_dirty, {
    Synth synth;

    assert_message_dirtiness(synth, SET_PARAM, true);
    assert_message_dirtiness(synth, ASSIGN_CONTROLLER, true);
    assert_message_dirtiness(synth, REFRESH_PARAM, false);
    assert_message_dirtiness(synth, CLEAR, true);

    synth.push_message(
        SET_PARAM, Synth::ParamId::CVOL, 0.123, Synth::ControllerId::MACRO_1
    );
    assert_message_dirtiness(synth, CLEAR_DIRTY_FLAG, false);
})


TEST(can_process_messages_synchronously, {
    Synth synth;
    Synth::Message message(SET_PARAM, Synth::ParamId::PM, 0.123, 0);

    assert_false(synth.is_dirty());

    synth.process_message(message);
    assert_true(synth.is_dirty());

    synth.clear_dirty_flag();
    assert_false(synth.is_dirty());

    synth.process_message(
        ASSIGN_CONTROLLER, Synth::ParamId::FM, 0.0, Synth::ControllerId::MACRO_1
    );
    assert_true(synth.is_dirty());

    assert_eq(
        0.123, synth.get_param_ratio_atomic(Synth::ParamId::PM), DOUBLE_DELTA
    );
    assert_eq(
        Synth::ControllerId::MACRO_1,
        synth.get_param_controller_id_atomic(Synth::ParamId::FM)
    );
})


void set_up_peak_controller_test(Synth& synth)
{
    synth.set_block_size(PEAK_CTL_TEST_BLOCK_SIZE);
    synth.set_sample_rate(44100.0);
    synth.note_on(0.0, 1, Midi::NOTE_A_4, 127);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.panning.set_value(1.0);
    synth.modulator_params.volume.set_value(PEAK_CTL_TEST_OSC_1_VOL);

    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.panning.set_value(1.0);
    synth.carrier_params.volume.set_value(PEAK_CTL_TEST_OSC_2_VOL);

    synth.effects.filter_1_type.set_value(BiquadFilter<SignalProducer>::HIGH_SHELF);
    synth.effects.filter_1.frequency.set_value(1.0);
    synth.effects.filter_1.gain.set_value(PEAK_CTL_TEST_FILTER_1_GAIN);

    synth.effects.reverb.dry.set_value(PEAK_CTL_TEST_REVERB_DRY);
}


TEST(peak_controllers_are_not_updated_when_they_are_not_assigned_to_any_parameter, {
    Synth synth;

    set_up_peak_controller_test(synth);

    synth.generate_samples(1, PEAK_CTL_TEST_BLOCK_SIZE);

    assert_eq(0.0, synth.osc_1_peak.get_value());
    assert_eq(0.0, synth.osc_2_peak.get_value());
    assert_eq(0.0, synth.vol_1_peak.get_value());
    assert_eq(0.0, synth.vol_2_peak.get_value());
    assert_eq(0.0, synth.vol_3_peak.get_value());
})


void test_peak_controller(
        Synth::ControllerId const controller_id,
        Number const expected_value
) {
    Synth synth;
    MidiController const* controller = NULL;

    set_up_peak_controller_test(synth);
    assign_controller(synth, Synth::ParamId::M1IN, controller_id);

    synth.generate_samples(1, PEAK_CTL_TEST_BLOCK_SIZE);

    switch (controller_id) {
        case Synth::ControllerId::OSC_1_PEAK: controller = &synth.osc_1_peak; break;
        case Synth::ControllerId::OSC_2_PEAK: controller = &synth.osc_2_peak; break;
        case Synth::ControllerId::VOL_1_PEAK: controller = &synth.vol_1_peak; break;
        case Synth::ControllerId::VOL_2_PEAK: controller = &synth.vol_2_peak; break;
        case Synth::ControllerId::VOL_3_PEAK: controller = &synth.vol_3_peak; break;
        default: break;
    }

    assert_eq(expected_value, controller->get_value(), 0.006);
}


TEST(peak_controllers_are_updated_when_in_use, {
    Number const osc_1_expected = PEAK_CTL_TEST_OSC_1_VOL;
    Number const osc_2_expected = PEAK_CTL_TEST_OSC_2_VOL;
    Number const vol_1_expected = 1.0; /* osc_1_expected + osc_2_expected > 1.0 */
    Number const vol_2_expected = (
        Math::db_to_linear(PEAK_CTL_TEST_FILTER_1_GAIN)
        * (osc_1_expected + osc_2_expected)
    );
    Number const vol_3_expected = PEAK_CTL_TEST_REVERB_DRY * vol_2_expected;

    test_peak_controller(Synth::ControllerId::OSC_1_PEAK, osc_1_expected);
    test_peak_controller(Synth::ControllerId::OSC_2_PEAK, osc_2_expected);
    test_peak_controller(Synth::ControllerId::VOL_1_PEAK, vol_1_expected);
    test_peak_controller(Synth::ControllerId::VOL_2_PEAK, vol_2_expected);
    test_peak_controller(Synth::ControllerId::VOL_3_PEAK, vol_3_expected);
})


void play_notes(Synth& synth)
{
    synth.note_on(0.00, 1, Midi::NOTE_A_2, 100);
    synth.note_off(0.21, 1, Midi::NOTE_A_2, 100);

    synth.note_on(0.20, 1, Midi::NOTE_A_3, 100);
    synth.note_off(0.41, 1, Midi::NOTE_A_3, 100);

    synth.note_on(0.40, 1, Midi::NOTE_A_4, 100);
    synth.note_off(0.61, 1, Midi::NOTE_A_4, 100);

    synth.note_on(0.60, 1, Midi::NOTE_A_5, 100);
    synth.note_off(0.81, 1, Midi::NOTE_A_5, 100);

    synth.note_on(0.80, 1, Midi::NOTE_A_6, 100);
    synth.note_off(0.99, 1, Midi::NOTE_A_6, 100);
}


TEST(voice_inaccuracy_is_deterministic_random, {
    constexpr Frequency sample_rate = 10000.0;
    constexpr Integer block_size = 10000;
    constexpr Integer rounds = 1;
    constexpr Integer buffer_size = rounds * block_size;

    Synth synth;
    Buffer buffer_precise(buffer_size, synth.get_channels());
    Buffer buffer_inaccurate_a(buffer_size, synth.get_channels());
    Buffer buffer_inaccurate_b(buffer_size, synth.get_channels());
    std::vector<Number> diff(buffer_size);
    Math::Statistics statistics;

    Number const tuning = (
        synth.modulator_params.tuning.value_to_ratio(Modulator::TUNING_432HZ_12TET)
    );

    synth.set_sample_rate(sample_rate);
    synth.set_block_size(block_size);
    synth.resume();

    set_param(synth, Synth::ParamId::MVOL, 1.0);
    set_param(synth, Synth::ParamId::CVOL, 0.0);
    synth.process_messages();

    play_notes(synth);
    render_rounds<Synth>(synth, buffer_precise, rounds, block_size);

    set_param(synth, Synth::ParamId::MTUN, tuning);
    set_param(synth, Synth::ParamId::MOIA, 1.0);
    set_param(synth, Synth::ParamId::CTUN, tuning);
    set_param(synth, Synth::ParamId::COIA, 1.0);
    synth.process_messages();

    synth.reset();
    play_notes(synth);
    render_rounds<Synth>(synth, buffer_inaccurate_a, rounds, block_size);

    synth.reset();
    play_notes(synth);
    render_rounds<Synth>(synth, buffer_inaccurate_b, rounds, block_size);

    for (Integer i = 0; i != buffer_size; ++i) {
        diff[i] = std::fabs(buffer_precise.samples[0][i] - buffer_inaccurate_a.samples[0][i]);
    }

    Math::compute_statistics(diff, statistics);
    assert_true(statistics.is_valid);
    assert_lt(statistics.min, 0.01);
    assert_gt(statistics.max, 0.70);
    assert_gt(statistics.standard_deviation, 0.2);

    assert_eq(buffer_inaccurate_a.samples[0], buffer_inaccurate_b.samples[0], buffer_size, DOUBLE_DELTA);
    assert_eq(buffer_inaccurate_a.samples[1], buffer_inaccurate_b.samples[1], buffer_size, DOUBLE_DELTA);
})


TEST(can_collect_notes_which_are_on_and_not_released, {
    Synth synth(0);

    synth.set_sample_rate(44100.0);
    synth.set_block_size(4096);

    set_param(synth, Synth::ParamId::MAMP, 0.5);
    set_param(synth, Synth::ParamId::CAMP, 0.5);

    set_param(synth, Synth::ParamId::N1REL, 1.0);
    assign_controller(synth, Synth::ParamId::MVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_1);
    synth.process_messages();

    synth.note_on(0.000001, 1, Midi::NOTE_A_2, 100);
    synth.note_on(0.000002, 1, Midi::NOTE_A_3, 100);
    synth.note_on(0.000003, 1, Midi::NOTE_A_4, 100);
    synth.note_on(0.000004, 1, Midi::NOTE_A_5, 100);

    synth.note_off(0.000005, 1, Midi::NOTE_A_3, 100);
    synth.note_off(0.000006, 1, Midi::NOTE_A_4, 100);
    synth.note_off(0.000007, 1, Midi::NOTE_A_5, 100);

    SignalProducer::produce<Synth>(synth, 1);

    Integer active_notes_count = 0;
    Synth::NoteTunings const& active_notes = synth.collect_active_notes(active_notes_count);

    assert_eq(1, (int)active_notes_count);
    assert_true(active_notes[0].is_valid());
    assert_eq((int)Midi::NOTE_A_2, (int)active_notes[0].note);
    assert_eq(1, (int)active_notes[0].channel);
})


TEST(tuning_can_be_updated_on_the_fly, {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;
    constexpr Frequency a2_freq = 110.0;
    constexpr Frequency a3_freq_original = 220.0;
    constexpr Frequency a3_freq_modified = 880.0;
    constexpr Midi::Channel channel = 12;

    Synth synth;
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL, a2_freq,
        OUT_VOLUME_PER_CHANNEL * 0.5, a3_freq_original,
        OUT_VOLUME_PER_CHANNEL * 0.5, a3_freq_modified,
        synth.get_channels()
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;
    Synth::NoteTunings note_tunings = {
        Synth::NoteTuning(channel, Midi::NOTE_A_2, -1.0),
        Synth::NoteTuning(channel, Midi::NOTE_A_3, a3_freq_modified),
    };

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(0.5);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);
    synth.modulator_params.tuning.set_value(Modulator::TUNING_440HZ_12TET);

    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.volume.set_value(0.5);
    synth.carrier_params.waveform.set_value(SimpleOscillator::SINE);
    synth.carrier_params.width.set_value(0.0);
    synth.carrier_params.tuning.set_value(Carrier::TUNING_440HZ_12TET);

    assert_false(synth.has_mts_esp_tuning());
    assert_false(synth.has_continuous_mts_esp_tuning());

    synth.modulator_params.tuning.set_value(Modulator::TUNING_MTS_ESP_NOTE_ON);
    assert_true(synth.has_mts_esp_tuning());
    assert_false(synth.has_continuous_mts_esp_tuning());

    synth.modulator_params.tuning.set_value(Carrier::TUNING_MTS_ESP_CONTINUOUS);
    assert_true(synth.has_mts_esp_tuning());
    assert_true(synth.has_continuous_mts_esp_tuning());

    synth.note_on(0.0, channel, Midi::NOTE_A_2, 127);
    synth.note_on(0.0, channel, Midi::NOTE_A_3, 127);

    synth.update_note_tuning(Synth::NoteTuning(Midi::CHANNELS, Midi::NOTES, 1760.0));
    synth.update_note_tunings(note_tunings, 2);

    SignalProducer::produce<SumOfSines>(expected, 1, 1);
    SignalProducer::produce<Synth>(synth, 1, 1);

    expected_samples = SignalProducer::produce<SumOfSines>(expected, 2);
    rendered_samples = SignalProducer::produce<Synth>(synth, 2);

    assert_close(expected_samples[0], rendered_samples[0], block_size, 0.06, "channel=0");
    assert_close(expected_samples[1], rendered_samples[1], block_size, 0.06, "channel=1");
})


TEST(stores_mts_esp_connection_flag, {
    Synth synth;

    assert_false(synth.is_mts_esp_connected());

    synth.mts_esp_connected();
    assert_true(synth.is_mts_esp_connected());

    synth.mts_esp_disconnected();
    assert_false(synth.is_mts_esp_connected());
})


TEST(updating_voice_inaccuracy_many_times_yields_uniform_distribution, {
    constexpr Integer probes = 100000;

    for (Integer i = 0; i != Synth::POLYPHONY; ++i) {
        std::vector<Number> inaccuracies(probes);
        Math::Statistics statistics;
        Number inaccuracy = Synth::calculate_inaccuracy_seed(i);

        for (Integer j = 0; j != probes; ++j) {
            inaccuracy = OscillatorInaccuracy::calculate_new_inaccuracy(inaccuracy);
            inaccuracies[j] = inaccuracy;
        }

        Math::compute_statistics(inaccuracies, statistics);

        Number const mean = (
            (OscillatorInaccuracy::MIN + OscillatorInaccuracy::MAX) / 2.0
        );

        assert_statistics(
            true,
            OscillatorInaccuracy::MIN,
            mean,
            OscillatorInaccuracy::MAX,
            mean,
            (OscillatorInaccuracy::MAX - mean) / 2.0,
            statistics,
            0.02
        );
    }
})


TEST(can_switch_between_polyphonic_and_monophonic_modes, {
    Synth synth;

    synth.mono_mode_on(0.0, 0);
    assert_eq(Synth::NOTE_HANDLING_MONOPHONIC, synth.note_handling.get_value());
    assert_false(synth.is_polyphonic());
    assert_true(synth.is_monophonic());
    assert_false(synth.is_holding());
    assert_false(synth.is_ignoring_sustain_pedal());

    synth.mono_mode_off(0.0, 0);
    assert_eq(Synth::NOTE_HANDLING_POLYPHONIC, synth.note_handling.get_value());
    assert_true(synth.is_polyphonic());
    assert_false(synth.is_monophonic());
    assert_false(synth.is_holding());
    assert_false(synth.is_ignoring_sustain_pedal());

    synth.note_handling.set_value(Synth::NOTE_HANDLING_POLYPHONIC_HOLD);
    synth.mono_mode_on(0.0, 0);
    assert_eq(
        Synth::NOTE_HANDLING_MONOPHONIC_HOLD,
        synth.note_handling.get_value()
    );
    assert_false(synth.is_polyphonic());
    assert_true(synth.is_monophonic());
    assert_true(synth.is_holding());
    assert_false(synth.is_ignoring_sustain_pedal());

    synth.mono_mode_off(0.0, 0);
    assert_eq(
        Synth::NOTE_HANDLING_POLYPHONIC_HOLD,
        synth.note_handling.get_value()
    );
    assert_true(synth.is_polyphonic());
    assert_false(synth.is_monophonic());
    assert_true(synth.is_holding());
    assert_false(synth.is_ignoring_sustain_pedal());

    synth.note_handling.set_value(Synth::NOTE_HANDLING_POLYPHONIC_RETRIGGER);
    synth.mono_mode_on(0.0, 0);
    assert_eq(Synth::NOTE_HANDLING_MONOPHONIC, synth.note_handling.get_value());

    synth.note_handling.set_value(Synth::NOTE_HANDLING_POLYPHONIC_HOLD_IGSUS);
    synth.mono_mode_on(0.0, 0);
    assert_eq(
        Synth::NOTE_HANDLING_MONOPHONIC_HOLD_IGSUS,
        synth.note_handling.get_value()
    );
    assert_false(synth.is_polyphonic());
    assert_true(synth.is_monophonic());
    assert_true(synth.is_holding());
    assert_true(synth.is_ignoring_sustain_pedal());
})


TEST(triggered_and_released_note_and_velocity_controllers_are_maintained, {
    constexpr Number A_2 = (Number)Midi::NOTE_A_2 / 127.0;
    constexpr Number A_5 = (Number)Midi::NOTE_A_5 / 127.0;

    constexpr Number tolerance = 1.0 / 512.0;

    Synth synth;

    synth.triggered_note.change(0.0, 0.0);
    synth.triggered_velocity.change(0.0, 0.0);
    synth.released_note.change(0.0, 0.0);
    synth.released_velocity.change(0.0, 0.0);

    synth.note_on(0.0, 1, Midi::NOTE_A_5, 96);

    assert_eq(A_5, synth.triggered_note.get_value(), tolerance);
    assert_eq(96.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(0.0, synth.released_note.get_value(), tolerance);
    assert_eq(0.0, synth.released_velocity.get_value(), tolerance);

    synth.note_off(0.0, 1, Midi::NOTE_A_5, 108);
    synth.note_on(0.0, 1, Midi::NOTE_A_2, 36);

    assert_eq(A_2, synth.triggered_note.get_value(), tolerance);
    assert_eq(36.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(A_5, synth.released_note.get_value(), tolerance);
    assert_eq(108.0 / 127.0, synth.released_velocity.get_value(), tolerance);

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_off(0.0, 1, Midi::NOTE_A_2, 48);

    assert_eq(A_2, synth.triggered_note.get_value(), tolerance);
    assert_eq(36.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(A_5, synth.released_note.get_value(), tolerance);
    assert_eq(108.0 / 127.0, synth.released_velocity.get_value(), tolerance);

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 0);

    assert_eq(A_2, synth.triggered_note.get_value(), tolerance);
    assert_eq(36.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(A_2, synth.released_note.get_value(), tolerance);
    assert_eq(48.0 / 127.0, synth.released_velocity.get_value(), tolerance);

    synth.note_handling.set_value(Synth::NOTE_HANDLING_MONOPHONIC);

    synth.note_on(0.0, 1, Midi::NOTE_A_2, 36);
    synth.note_on(0.0, 1, Midi::NOTE_A_5, 96);

    assert_eq(A_5, synth.triggered_note.get_value(), tolerance);
    assert_eq(96.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(A_2, synth.released_note.get_value(), tolerance);
    assert_eq(48.0 / 127.0, synth.released_velocity.get_value(), tolerance);

    synth.note_off(0.0, 1, Midi::NOTE_A_5, 108);
    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_off(0.0, 1, Midi::NOTE_A_2, 48);

    assert_eq(A_2, synth.triggered_note.get_value(), tolerance);
    assert_eq(36.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(A_5, synth.released_note.get_value(), tolerance);
    assert_eq(108.0 / 127.0, synth.released_velocity.get_value(), tolerance);

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 0);

    assert_eq(A_2, synth.triggered_note.get_value(), tolerance);
    assert_eq(36.0 / 127.0, synth.triggered_velocity.get_value(), tolerance);
    assert_eq(A_2, synth.released_note.get_value(), tolerance);
    assert_eq(48.0 / 127.0, synth.released_velocity.get_value(), tolerance);
})


TEST(when_sustained_notes_are_to_be_retriggered_then_note_on_retriggers_already_sounding_note, {
    constexpr Frequency sample_rate = 3000.0;
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

    assign_controller(synth_1, Synth::ParamId::MVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth_1, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_1);

    assign_controller(synth_2, Synth::ParamId::MVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth_2, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_1);

    synth_1.process_messages();
    synth_2.process_messages();

    synth_1.envelopes[0]->release_time.set_value(Modulator::ENVELOPE_CANCEL_DURATION);
    synth_2.envelopes[0]->release_time.set_value(Modulator::ENVELOPE_CANCEL_DURATION);

    synth_1.note_on(0.0, 1, Midi::NOTE_A_0, 100);
    synth_1.note_off(0.5, 1, Midi::NOTE_A_0, 100);
    synth_1.note_on(
        0.5 + Modulator::ENVELOPE_CANCEL_DURATION, 1, Midi::NOTE_A_0, 100
    );
    synth_1.note_off(1.0, 1, Midi::NOTE_A_0, 100);

    synth_2.note_handling.set_value(Synth::NOTE_HANDLING_POLYPHONIC_RETRIGGER_HOLD);
    synth_2.note_on(0.0, 1, Midi::NOTE_A_0, 100);
    synth_2.note_off(0.1, 1, Midi::NOTE_A_0, 100);
    synth_2.note_on(0.5, 1, Midi::NOTE_A_0, 100);
    synth_2.note_off(0.6, 1, Midi::NOTE_A_0, 100);
    synth_2.control_change(1.0, 1, Midi::SUSTAIN_PEDAL, 0);

    synth_1_samples = SignalProducer::produce<Synth>(synth_1, 1);
    synth_2_samples = SignalProducer::produce<Synth>(synth_2, 1);

    for (Integer c = 0; c != synth_1.get_channels(); ++c) {
        assert_close(
            synth_1_samples[c],
            synth_2_samples[c],
            block_size,
            0.03,
            "channel=%d",
            (int)c
        );
    }

    assert_false(synth_1.is_retriggering());
    assert_true(synth_2.is_retriggering());
})


void test_ignore_sustain_pedal(
        Byte const note_handling,
        Byte const ref_note_handling
) {
    constexpr Frequency sample_rate = 3000.0;
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

    synth_1.process_messages();
    synth_2.process_messages();

    synth_1.note_handling.set_value(ref_note_handling);
    synth_1.note_on(0.0, 1, Midi::NOTE_A_0, 100);
    synth_1.note_off(0.5, 1, Midi::NOTE_A_0, 100);

    synth_2.note_handling.set_value(note_handling);
    synth_2.note_on(0.0, 1, Midi::NOTE_A_0, 100);
    synth_2.control_change(0.2, 1, Midi::SUSTAIN_PEDAL, 127);
    assert_eq(1.0, synth_2.midi_controllers[Midi::SUSTAIN_PEDAL]->get_value());

    synth_2.note_off(0.5, 1, Midi::NOTE_A_0, 100);
    synth_2.control_change(0.7, 1, Midi::SUSTAIN_PEDAL, 0);
    assert_eq(0.0, synth_2.midi_controllers[Midi::SUSTAIN_PEDAL]->get_value());

    synth_1_samples = SignalProducer::produce<Synth>(synth_1, 1);
    synth_2_samples = SignalProducer::produce<Synth>(synth_2, 1);

    for (Integer c = 0; c != synth_1.get_channels(); ++c) {
        assert_close(
            synth_1_samples[c],
            synth_2_samples[c],
            block_size,
            0.03,
            "channel=%d",
            (int)c
        );
    }

    assert_false(synth_1.is_ignoring_sustain_pedal());
    assert_true(synth_2.is_ignoring_sustain_pedal());
}


TEST(when_sustain_pedal_is_ignored_in_non_hold_modes_then_it_only_updates_its_midi_controller, {
    test_ignore_sustain_pedal(
        Synth::NOTE_HANDLING_MONOPHONIC_IGSUS, Synth::NOTE_HANDLING_MONOPHONIC
    );
    test_ignore_sustain_pedal(
        Synth::NOTE_HANDLING_POLYPHONIC_IGSUS, Synth::NOTE_HANDLING_POLYPHONIC
    );

    test_ignore_sustain_pedal(
        Synth::NOTE_HANDLING_MONOPHONIC_HOLD_IGSUS,
        Synth::NOTE_HANDLING_MONOPHONIC_HOLD
    );
    test_ignore_sustain_pedal(
        Synth::NOTE_HANDLING_POLYPHONIC_HOLD_IGSUS,
        Synth::NOTE_HANDLING_POLYPHONIC_HOLD
    );
    test_ignore_sustain_pedal(
        Synth::NOTE_HANDLING_POLYPHONIC_RETRIGGER_HOLD_IGSUS,
        Synth::NOTE_HANDLING_POLYPHONIC_RETRIGGER_HOLD
    );
})


TEST(input_is_mixed_into_the_effects_chain, {
    constexpr Frequency sample_rate = 11025.0;
    constexpr Integer block_size = 4096;

    Synth synth;
    SumOfSines input(
        1.0, 110.0,
        0.5, 5000.0,
        0.0, 0.0,
        synth.get_channels()
    );
    SumOfSines expected(
        0.5, 110.0,
        1.0, 220.0,
        0.0, 0.0,
        synth.get_channels()
    );
    Sample const* const* in_samples;
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.suspend();

    synth.set_block_size(block_size);
    input.set_block_size(block_size);
    expected.set_block_size(block_size);

    synth.set_sample_rate(sample_rate);
    input.set_sample_rate(sample_rate);
    expected.set_sample_rate(sample_rate);

    synth.resume();

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.panning.set_value(1.0);
    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.volume.set_value(1.0);
    synth.carrier_params.panning.set_value(-1.0);
    synth.input_volume.schedule_value(0.0, 0.5);
    synth.effects.filter_1.frequency.set_value(3000.0);
    synth.effects.filter_1.q.set_value(0.0);
    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);

    for (Integer round = 1; round != 3; ++round) {
        in_samples = SignalProducer::produce<SumOfSines>(input, round);
        expected_samples = SignalProducer::produce<SumOfSines>(expected, round);
        rendered_samples = synth.generate_samples(round, block_size, in_samples);

        for (Integer c = 0; c != synth.get_channels(); ++c) {
            assert_close(
                expected_samples[c],
                rendered_samples[c],
                block_size,
                0.05,
                "round=%d, channel=%d",
                (int)round,
                (int)c
            );
        }
    }
})


void test_semi_polyphonic_aftertouch(
        Byte const envelope_update_mode,
        Frequency const expected_note_frequency
) {
    constexpr Frequency sample_rate = 11025.0;
    constexpr Integer block_size = 4096;

    Synth synth;
    SumOfSines expected(
        0.5, expected_note_frequency,
        0.0, 0.0,
        0.0, 0.0,
        synth.get_channels()
    );

    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.suspend();

    synth.set_block_size(block_size);
    expected.set_block_size(block_size);

    synth.set_sample_rate(sample_rate);
    expected.set_sample_rate(sample_rate);

    synth.resume();

    assign_controller(synth, Synth::ParamId::MVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth, Synth::ParamId::CVOL, Synth::ControllerId::ENVELOPE_1);
    assign_controller(synth, Synth::ParamId::N1SCL, Synth::ControllerId::CHANNEL_PRESSURE);
    synth.process_messages();

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.panning.set_value(-1.0);

    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.panning.set_value(1.0);

    synth.envelopes[0]->update_mode.set_value(envelope_update_mode);
    synth.envelopes[0]->initial_value.set_value(1.0);
    synth.envelopes[0]->peak_value.set_value(1.0);
    synth.envelopes[0]->sustain_value.set_value(1.0);
    synth.envelopes[0]->final_value.set_value(1.0);

    synth.note_on(0.0, 1, Midi::NOTE_A_2, 127);
    synth.note_on(0.0, 1, Midi::NOTE_A_1, 127);
    synth.note_on(0.0, 1, Midi::NOTE_A_4, 127);
    synth.note_on(0.0, 1, Midi::NOTE_A_5, 127);
    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);

    synth.channel_pressure(0.0, 1, 63);

    expected_samples = SignalProducer::produce<SumOfSines>(expected, 1);
    rendered_samples = synth.generate_samples(1, block_size);

    for (Integer c = 0; c != synth.get_channels(); ++c) {
        assert_close(
            expected_samples[c],
            rendered_samples[c],
            block_size,
            0.01,
            "channel=%d, envelope_update_mode=%hhx, expected_note_frequency=%f",
            (int)c,
            envelope_update_mode,
            expected_note_frequency
        );
    }
}


TEST(semi_polyphonic_aftertouch, {
    test_semi_polyphonic_aftertouch(Envelope::UPDATE_MODE_DYNAMIC_LAST, 220.0);
    test_semi_polyphonic_aftertouch(Envelope::UPDATE_MODE_DYNAMIC_OLDEST, 110.0);
    test_semi_polyphonic_aftertouch(Envelope::UPDATE_MODE_DYNAMIC_LOWEST, 55.0);
    test_semi_polyphonic_aftertouch(Envelope::UPDATE_MODE_DYNAMIC_HIGHEST, 880.0);
})


TEST(keeps_track_of_number_of_active_voices, {
    constexpr Frequency sample_rate = 1000.0;
    constexpr Integer block_size = 1024;
    Synth synth;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    synth.resume();

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 127);
    synth.note_on(0.0, 1, Midi::NOTE_A_2, 127);
    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);
    synth.note_off(0.5, 1, Midi::NOTE_A_3, 127);

    assert_eq(0, (int)synth.get_active_voices_count());

    SignalProducer::produce<Synth>(synth, 0);
    assert_eq(2, (int)synth.get_active_voices_count());

    synth.control_change(0.0, 1, Midi::SUSTAIN_PEDAL, 0);
    SignalProducer::produce<Synth>(synth, 1);
    assert_eq(1, (int)synth.get_active_voices_count());

    synth.note_off(0.0, 1, Midi::NOTE_A_2, 127);
    SignalProducer::produce<Synth>(synth, 2);
    assert_eq(0, (int)synth.get_active_voices_count());
})


TEST(keeps_track_of_tape_state, {
    Synth synth;

    synth.set_block_size(1024);
    synth.set_sample_rate(1000.0);
    synth.resume();

    assert_eq(
        (int)TapeParams::State::TAPE_STATE_INIT,
        (int)synth.get_tape_state()
    );

    SignalProducer::produce<Synth>(synth, 0);

    assert_eq(
        (int)TapeParams::State::TAPE_STATE_NORMAL,
        (int)synth.get_tape_state()
    );
})


TEST(modulator_additive_volume_can_be_monophonic, {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;

    Synth synth;
    Integer const channels = synth.get_channels();
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL * 0.7, 220.0,
        OUT_VOLUME_PER_CHANNEL * 0.3, 880.0,
        0.0, 0.0,
        channels
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);

    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.detune.set_value(2400.0);
    synth.carrier_params.volume.set_value(0.3);
    synth.carrier_params.waveform.set_value(SimpleOscillator::SINE);
    synth.carrier_params.width.set_value(0.0);

    set_param(synth, Synth::ParamId::MIX, 0.7);
    synth.process_messages();

    synth.note_on(0.0, 0, Midi::NOTE_A_3, 127);

    expected_samples = SignalProducer::produce<SumOfSines>(expected, 1);
    rendered_samples = SignalProducer::produce<Synth>(synth, 1);

    for (Integer i = 0; i != channels; ++i) {
        assert_close(
            expected_samples[i],
            rendered_samples[i],
            block_size,
            0.001,
            "(MIX is const) channel=%d",
            (int)i
        );
    }

    synth.suspend();
    synth.resume();
    expected.reset();

    set_param(synth, Synth::ParamId::MIX, 1.0);
    set_param(synth, Synth::ParamId::L1MIN, 0.699);
    set_param(synth, Synth::ParamId::L1MAX, 0.701);

    assign_controller(synth, Synth::ParamId::MIX, Synth::ControllerId::LFO_1);
    synth.process_messages();

    synth.note_on(0.0, 0, Midi::NOTE_A_3, 127);

    expected_samples = SignalProducer::produce<SumOfSines>(expected, 2);
    rendered_samples = SignalProducer::produce<Synth>(synth, 2);

    for (Integer i = 0; i != channels; ++i) {
        assert_close(
            expected_samples[i],
            rendered_samples[i],
            block_size,
            0.001,
            "(MIX controlled by LFO) channel=%d",
            (int)i
        );
    }
})


TEST(modulator_additive_volume_can_be_polyphonic, {
    constexpr Integer block_size = 2048;
    constexpr Frequency sample_rate = 22050.0;

    Synth synth;
    Integer const channels = synth.get_channels();
    SumOfSines expected(
        OUT_VOLUME_PER_CHANNEL * 0.5, 220.0,
        OUT_VOLUME_PER_CHANNEL * 1.0, 880.0,
        0.0, 0.0,
        channels
    );
    Sample const* const* rendered_samples;
    Sample const* const* expected_samples;

    synth.set_block_size(block_size);
    synth.set_sample_rate(sample_rate);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);

    synth.carrier_params.amplitude.set_value(1.0);
    synth.carrier_params.detune.set_value(2400.0);
    synth.carrier_params.volume.set_value(1.0);
    synth.carrier_params.waveform.set_value(SimpleOscillator::SINE);
    synth.carrier_params.width.set_value(0.0);

    set_param(synth, Synth::ParamId::N1SCL, 0.5);
    set_param(synth, Synth::ParamId::N1INI, 1.0);
    set_param(synth, Synth::ParamId::N1PK, 1.0);
    set_param(synth, Synth::ParamId::N1SUS, 1.0);
    set_param(synth, Synth::ParamId::N1FIN, 1.0);

    assign_controller(synth, Synth::ParamId::MIX, Synth::ControllerId::ENVELOPE_1);
    synth.process_messages();

    synth.note_on(0.0, 0, Midi::NOTE_A_3, 127);

    expected_samples = SignalProducer::produce<SumOfSines>(expected, 1);
    rendered_samples = SignalProducer::produce<Synth>(synth, 1);

    for (Integer i = 0; i != channels; ++i) {
        assert_close(
            expected_samples[i],
            rendered_samples[i],
            block_size,
            0.001,
            "channel=%d",
            (int)i
        );
    }
})
