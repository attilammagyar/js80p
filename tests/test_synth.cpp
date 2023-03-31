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


// TODO: remove this array once all params are implemented, and use Synth::PARAM_NAMES instead
Synth::ParamId IMPLEMENTED_PARAMS[] = {
    Synth::ParamId::MIX,
    Synth::ParamId::PM,
    Synth::ParamId::FM,
    Synth::ParamId::AM,

    Synth::ParamId::MAMP,
    Synth::ParamId::MVS,
    Synth::ParamId::MFLD,
    Synth::ParamId::MPRT,
    Synth::ParamId::MPRD,
    Synth::ParamId::MDTN,
    Synth::ParamId::MFIN,
    Synth::ParamId::MWID,
    Synth::ParamId::MPAN,
    Synth::ParamId::MVOL,

    Synth::ParamId::MC1,
    Synth::ParamId::MC2,
    Synth::ParamId::MC3,
    Synth::ParamId::MC4,
    Synth::ParamId::MC5,
    Synth::ParamId::MC6,
    Synth::ParamId::MC7,
    Synth::ParamId::MC8,
    Synth::ParamId::MC9,
    Synth::ParamId::MC10,

    Synth::ParamId::MF1FRQ,
    Synth::ParamId::MF1Q,
    Synth::ParamId::MF1G,

    Synth::ParamId::MF2FRQ,
    Synth::ParamId::MF2Q,
    Synth::ParamId::MF2G,

    Synth::ParamId::CAMP,
    Synth::ParamId::CVS,
    Synth::ParamId::CFLD,
    Synth::ParamId::CPRT,
    Synth::ParamId::CPRD,
    Synth::ParamId::CDTN,
    Synth::ParamId::CFIN,
    Synth::ParamId::CWID,
    Synth::ParamId::CPAN,
    Synth::ParamId::CVOL,

    Synth::ParamId::CC1,
    Synth::ParamId::CC2,
    Synth::ParamId::CC3,
    Synth::ParamId::CC4,
    Synth::ParamId::CC5,
    Synth::ParamId::CC6,
    Synth::ParamId::CC7,
    Synth::ParamId::CC8,
    Synth::ParamId::CC9,
    Synth::ParamId::CC10,

    Synth::ParamId::CF1FRQ,
    Synth::ParamId::CF1Q,
    Synth::ParamId::CF1G,

    Synth::ParamId::CF2FRQ,
    Synth::ParamId::CF2Q,
    Synth::ParamId::CF2G,

    Synth::ParamId::EOG,

    Synth::ParamId::EDG,

    Synth::ParamId::EF1FRQ,
    Synth::ParamId::EF1Q,
    Synth::ParamId::EF1G,

    Synth::ParamId::EF2FRQ,
    Synth::ParamId::EF2Q,
    Synth::ParamId::EF2G,

    Synth::ParamId::EEDEL,
    Synth::ParamId::EEFB,
    Synth::ParamId::EEDF,
    Synth::ParamId::EEDG,
    Synth::ParamId::EEWID,
    Synth::ParamId::EEHPF,
    Synth::ParamId::EEWET,
    Synth::ParamId::EEDRY,

    // TODO: turn on these when Reverb is implemented
    // Synth::ParamId::ERRS,
    // Synth::ParamId::ERDF,
    // Synth::ParamId::ERDG,
    // Synth::ParamId::ERWID,
    // Synth::ParamId::ERHPF,
    // Synth::ParamId::ERWET,
    // Synth::ParamId::ERDRY,

    Synth::ParamId::F1IN,
    Synth::ParamId::F1MIN,
    Synth::ParamId::F1MAX,
    Synth::ParamId::F1AMT,
    Synth::ParamId::F1DST,
    Synth::ParamId::F1RND,

    Synth::ParamId::F2IN,
    Synth::ParamId::F2MIN,
    Synth::ParamId::F2MAX,
    Synth::ParamId::F2AMT,
    Synth::ParamId::F2DST,
    Synth::ParamId::F2RND,

    Synth::ParamId::F3IN,
    Synth::ParamId::F3MIN,
    Synth::ParamId::F3MAX,
    Synth::ParamId::F3AMT,
    Synth::ParamId::F3DST,
    Synth::ParamId::F3RND,

    Synth::ParamId::F4IN,
    Synth::ParamId::F4MIN,
    Synth::ParamId::F4MAX,
    Synth::ParamId::F4AMT,
    Synth::ParamId::F4DST,
    Synth::ParamId::F4RND,

    Synth::ParamId::F5IN,
    Synth::ParamId::F5MIN,
    Synth::ParamId::F5MAX,
    Synth::ParamId::F5AMT,
    Synth::ParamId::F5DST,
    Synth::ParamId::F5RND,

    Synth::ParamId::F6IN,
    Synth::ParamId::F6MIN,
    Synth::ParamId::F6MAX,
    Synth::ParamId::F6AMT,
    Synth::ParamId::F6DST,
    Synth::ParamId::F6RND,

    Synth::ParamId::F7IN,
    Synth::ParamId::F7MIN,
    Synth::ParamId::F7MAX,
    Synth::ParamId::F7AMT,
    Synth::ParamId::F7DST,
    Synth::ParamId::F7RND,

    Synth::ParamId::F8IN,
    Synth::ParamId::F8MIN,
    Synth::ParamId::F8MAX,
    Synth::ParamId::F8AMT,
    Synth::ParamId::F8DST,
    Synth::ParamId::F8RND,

    Synth::ParamId::F9IN,
    Synth::ParamId::F9MIN,
    Synth::ParamId::F9MAX,
    Synth::ParamId::F9AMT,
    Synth::ParamId::F9DST,
    Synth::ParamId::F9RND,

    Synth::ParamId::F10IN,
    Synth::ParamId::F10MIN,
    Synth::ParamId::F10MAX,
    Synth::ParamId::F10AMT,
    Synth::ParamId::F10DST,
    Synth::ParamId::F10RND,

    Synth::ParamId::N1AMT,
    Synth::ParamId::N1INI,
    Synth::ParamId::N1DEL,
    Synth::ParamId::N1ATK,
    Synth::ParamId::N1PK,
    Synth::ParamId::N1HLD,
    Synth::ParamId::N1DEC,
    Synth::ParamId::N1SUS,
    Synth::ParamId::N1REL,
    Synth::ParamId::N1FIN,

    Synth::ParamId::N2AMT,
    Synth::ParamId::N2INI,
    Synth::ParamId::N2DEL,
    Synth::ParamId::N2ATK,
    Synth::ParamId::N2PK,
    Synth::ParamId::N2HLD,
    Synth::ParamId::N2DEC,
    Synth::ParamId::N2SUS,
    Synth::ParamId::N2REL,
    Synth::ParamId::N2FIN,

    Synth::ParamId::N3AMT,
    Synth::ParamId::N3INI,
    Synth::ParamId::N3DEL,
    Synth::ParamId::N3ATK,
    Synth::ParamId::N3PK,
    Synth::ParamId::N3HLD,
    Synth::ParamId::N3DEC,
    Synth::ParamId::N3SUS,
    Synth::ParamId::N3REL,
    Synth::ParamId::N3FIN,

    Synth::ParamId::N4AMT,
    Synth::ParamId::N4INI,
    Synth::ParamId::N4DEL,
    Synth::ParamId::N4ATK,
    Synth::ParamId::N4PK,
    Synth::ParamId::N4HLD,
    Synth::ParamId::N4DEC,
    Synth::ParamId::N4SUS,
    Synth::ParamId::N4REL,
    Synth::ParamId::N4FIN,

    Synth::ParamId::N5AMT,
    Synth::ParamId::N5INI,
    Synth::ParamId::N5DEL,
    Synth::ParamId::N5ATK,
    Synth::ParamId::N5PK,
    Synth::ParamId::N5HLD,
    Synth::ParamId::N5DEC,
    Synth::ParamId::N5SUS,
    Synth::ParamId::N5REL,
    Synth::ParamId::N5FIN,

    Synth::ParamId::N6AMT,
    Synth::ParamId::N6INI,
    Synth::ParamId::N6DEL,
    Synth::ParamId::N6ATK,
    Synth::ParamId::N6PK,
    Synth::ParamId::N6HLD,
    Synth::ParamId::N6DEC,
    Synth::ParamId::N6SUS,
    Synth::ParamId::N6REL,
    Synth::ParamId::N6FIN,

    Synth::ParamId::L1FRQ,
    Synth::ParamId::L1PHS,
    Synth::ParamId::L1MIN,
    Synth::ParamId::L1MAX,
    Synth::ParamId::L1AMT,
    Synth::ParamId::L1DST,
    Synth::ParamId::L1RND,

    Synth::ParamId::L2FRQ,
    Synth::ParamId::L2PHS,
    Synth::ParamId::L2MIN,
    Synth::ParamId::L2MAX,
    Synth::ParamId::L2AMT,
    Synth::ParamId::L2DST,
    Synth::ParamId::L2RND,

    Synth::ParamId::L3FRQ,
    Synth::ParamId::L3PHS,
    Synth::ParamId::L3MIN,
    Synth::ParamId::L3MAX,
    Synth::ParamId::L3AMT,
    Synth::ParamId::L3DST,
    Synth::ParamId::L3RND,

    Synth::ParamId::L4FRQ,
    Synth::ParamId::L4PHS,
    Synth::ParamId::L4MIN,
    Synth::ParamId::L4MAX,
    Synth::ParamId::L4AMT,
    Synth::ParamId::L4DST,
    Synth::ParamId::L4RND,

    Synth::ParamId::L5FRQ,
    Synth::ParamId::L5PHS,
    Synth::ParamId::L5MIN,
    Synth::ParamId::L5MAX,
    Synth::ParamId::L5AMT,
    Synth::ParamId::L5DST,
    Synth::ParamId::L5RND,

    Synth::ParamId::L6FRQ,
    Synth::ParamId::L6PHS,
    Synth::ParamId::L6MIN,
    Synth::ParamId::L6MAX,
    Synth::ParamId::L6AMT,
    Synth::ParamId::L6DST,
    Synth::ParamId::L6RND,

    Synth::ParamId::L7FRQ,
    Synth::ParamId::L7PHS,
    Synth::ParamId::L7AMT,
    Synth::ParamId::L7MIN,
    Synth::ParamId::L7MAX,
    Synth::ParamId::L7DST,
    Synth::ParamId::L7RND,

    Synth::ParamId::L8FRQ,
    Synth::ParamId::L8PHS,
    Synth::ParamId::L8MIN,
    Synth::ParamId::L8MAX,
    Synth::ParamId::L8AMT,
    Synth::ParamId::L8DST,
    Synth::ParamId::L8RND,

    Synth::ParamId::MODE,

    Synth::ParamId::MWAV,
    Synth::ParamId::CWAV,

    Synth::ParamId::MF1TYP,
    Synth::ParamId::MF2TYP,

    Synth::ParamId::CF1TYP,
    Synth::ParamId::CF2TYP,

    Synth::ParamId::EF1TYP,
    Synth::ParamId::EF2TYP,

    Synth::ParamId::L1WAV,
    Synth::ParamId::L2WAV,
    Synth::ParamId::L3WAV,
    Synth::ParamId::L4WAV,
    Synth::ParamId::L5WAV,
    Synth::ParamId::L6WAV,
    Synth::ParamId::L7WAV,
    Synth::ParamId::L8WAV,

    Synth::ParamId::MAX_PARAM_ID
};


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

    synth.phase_modulation_level.set_value(1.0);
    synth.modulator_add_volume.set_value(0.42);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::MWAV, inv_saw_as_ratio, 0
    );
    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::PM, 0.123, 0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::MIX, 0.0, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::ENVELOPE_3
    );

    assert_eq(1.0, synth.phase_modulation_level.get_value(), DOUBLE_DELTA);
    assert_eq(NULL, synth.carrier_params.volume.get_envelope());
    assert_eq(
        SimpleOscillator::SINE, synth.modulator_params.waveform.get_value()
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );

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
    Synth synth;
    Midi::Controller invalid = 127;
    Midi::Controller unused = 0;

    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::PM,
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
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWAV,
        0.0,
        Synth::ControllerId::MODULATION_WHEEL
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::PM, 0.0, 0.0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::MFIN, 0.0, 0.0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::MAMP, 0.0, 0.0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::CWAV, 0.0, 0.0
    );
    synth.control_change(0.0, 1, Midi::VOLUME, 0.42);
    synth.control_change(0.0, 1, Midi::MODULATION_WHEEL, 1.0);
    synth.control_change(0.0, 1, invalid, 0.123);
    synth.control_change(0.0, 1, unused, 0.123);
    synth.pitch_wheel_change(0.0, 1, 0.75);
    synth.note_on(0.0, 1, 69, 0.9);

    SignalProducer::produce<Synth>(&synth, 1, 1);

    assert_eq(0.42, synth.phase_modulation_level.get_ratio(), DOUBLE_DELTA);
    assert_eq(0.9, synth.modulator_params.amplitude.get_ratio(), DOUBLE_DELTA);
    assert_eq(0.75, synth.modulator_params.fine_detune.get_ratio(), DOUBLE_DELTA);
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

    assert_lte((int)max_collisions, 5);
    assert_lte(avg_bucket_size, 2.22);
    assert_lte(avg_collisions, 2.8);

    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id(""));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id(" \n"));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id("NO_SUCH_PARAM"));

    for (int i = 0; IMPLEMENTED_PARAMS[i] != Synth::ParamId::MAX_PARAM_ID; ++i) {
        std::string const name = synth.get_param_name(IMPLEMENTED_PARAMS[i]);
        Synth::ParamId const param_id = synth.get_param_id(name);
        assert_eq(IMPLEMENTED_PARAMS[i], param_id, "i=%d, name=\"%s\"", i, name);
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
    Sample const* const* samples;
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

    synth.note_on(0.0, 0, Midi::NOTE_A_3, 1.0);

    expected_samples = SignalProducer::produce<SumOfSines>(&expected, 1);
    samples = SignalProducer::produce<Synth>(&synth, 1);

    assert_close(
        expected_samples[0],
        samples[0],
        block_size,
        0.001,
        "channel=0, mode=%d",
        (int)mode
    );
    assert_close(
        expected_samples[1],
        samples[1],
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
