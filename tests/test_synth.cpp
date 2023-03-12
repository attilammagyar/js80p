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
    synth.modulator_add_volume.set_value(0.42);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);

    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::MWAV, inv_saw_as_ratio, 0
    );
    synth.push_message(
        Synth::MessageType::SET_PARAM, Synth::ParamId::VOL, 0.123, 0
    );
    synth.push_message(
        Synth::MessageType::REFRESH_PARAM, Synth::ParamId::ADD, 0.0, 0
    );
    synth.push_message(
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CVOL,
        0.0,
        Synth::ControllerId::ENVELOPE_3
    );

    assert_eq(1.0, synth.volume.get_value(), DOUBLE_DELTA);
    assert_eq(NULL, synth.carrier_params.volume.get_envelope());
    assert_eq(
        SimpleOscillator::SINE, synth.modulator_params.waveform.get_value()
    );
    assert_eq(
        Synth::ControllerId::NONE,
        synth.get_param_controller_id_atomic(Synth::ParamId::CVOL)
    );

    SignalProducer::produce<Synth>(&synth, 1);

    assert_eq(0.123, synth.volume.get_value(), DOUBLE_DELTA);
    assert_eq(0.123, synth.get_param_ratio_atomic(Synth::ParamId::VOL), DOUBLE_DELTA);
    assert_eq(0.42, synth.modulator_add_volume.get_value(), DOUBLE_DELTA);
    assert_eq(0.42, synth.get_param_ratio_atomic(Synth::ParamId::ADD), DOUBLE_DELTA);
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


TEST(can_look_up_param_id_by_name, {
    Synth synth;
    Integer max_collisions;
    Number avg_collisions;
    Number avg_bucket_size;

    synth.get_param_id_hash_table_statistics(
        max_collisions, avg_collisions, avg_bucket_size
    );

    assert_lte((int)max_collisions, 4);
    assert_lte(avg_bucket_size, 1.8);
    assert_lte(avg_collisions, 2.4);

    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.find_param_id(""));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.find_param_id(" \n"));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.find_param_id("NO_SUCH_PARAM"));

    assert_eq(Synth::ParamId::VOL, synth.find_param_id("VOL"));

    assert_eq(Synth::ParamId::ADD, synth.find_param_id("ADD"));
    assert_eq(Synth::ParamId::FM, synth.find_param_id("FM"));
    assert_eq(Synth::ParamId::AM, synth.find_param_id("AM"));

    assert_eq(Synth::ParamId::MAMP, synth.find_param_id("MAMP"));
    assert_eq(Synth::ParamId::MVS, synth.find_param_id("MVS"));
    assert_eq(Synth::ParamId::MFLD, synth.find_param_id("MFLD"));
    assert_eq(Synth::ParamId::MPRT, synth.find_param_id("MPRT"));
    assert_eq(Synth::ParamId::MPRD, synth.find_param_id("MPRD"));
    assert_eq(Synth::ParamId::MDTN, synth.find_param_id("MDTN"));
    assert_eq(Synth::ParamId::MFIN, synth.find_param_id("MFIN"));
    assert_eq(Synth::ParamId::MWID, synth.find_param_id("MWID"));
    assert_eq(Synth::ParamId::MPAN, synth.find_param_id("MPAN"));
    assert_eq(Synth::ParamId::MVOL, synth.find_param_id("MVOL"));

    assert_eq(Synth::ParamId::MC1, synth.find_param_id("MC1"));
    assert_eq(Synth::ParamId::MC2, synth.find_param_id("MC2"));
    assert_eq(Synth::ParamId::MC3, synth.find_param_id("MC3"));
    assert_eq(Synth::ParamId::MC4, synth.find_param_id("MC4"));
    assert_eq(Synth::ParamId::MC5, synth.find_param_id("MC5"));
    assert_eq(Synth::ParamId::MC6, synth.find_param_id("MC6"));
    assert_eq(Synth::ParamId::MC7, synth.find_param_id("MC7"));
    assert_eq(Synth::ParamId::MC8, synth.find_param_id("MC8"));
    assert_eq(Synth::ParamId::MC9, synth.find_param_id("MC9"));
    assert_eq(Synth::ParamId::MC10, synth.find_param_id("MC10"));

    assert_eq(Synth::ParamId::MF1FRQ, synth.find_param_id("MF1FRQ"));
    assert_eq(Synth::ParamId::MF1Q, synth.find_param_id("MF1Q"));
    assert_eq(Synth::ParamId::MF1G, synth.find_param_id("MF1G"));

    assert_eq(Synth::ParamId::MF2FRQ, synth.find_param_id("MF2FRQ"));
    assert_eq(Synth::ParamId::MF2Q, synth.find_param_id("MF2Q"));
    assert_eq(Synth::ParamId::MF2G, synth.find_param_id("MF2G"));

    assert_eq(Synth::ParamId::CAMP, synth.find_param_id("CAMP"));
    assert_eq(Synth::ParamId::CVS, synth.find_param_id("CVS"));
    assert_eq(Synth::ParamId::CFLD, synth.find_param_id("CFLD"));
    assert_eq(Synth::ParamId::CPRT, synth.find_param_id("CPRT"));
    assert_eq(Synth::ParamId::CPRD, synth.find_param_id("CPRD"));
    assert_eq(Synth::ParamId::CDTN, synth.find_param_id("CDTN"));
    assert_eq(Synth::ParamId::CFIN, synth.find_param_id("CFIN"));
    assert_eq(Synth::ParamId::CWID, synth.find_param_id("CWID"));
    assert_eq(Synth::ParamId::CPAN, synth.find_param_id("CPAN"));
    assert_eq(Synth::ParamId::CVOL, synth.find_param_id("CVOL"));

    assert_eq(Synth::ParamId::CC1, synth.find_param_id("CC1"));
    assert_eq(Synth::ParamId::CC2, synth.find_param_id("CC2"));
    assert_eq(Synth::ParamId::CC3, synth.find_param_id("CC3"));
    assert_eq(Synth::ParamId::CC4, synth.find_param_id("CC4"));
    assert_eq(Synth::ParamId::CC5, synth.find_param_id("CC5"));
    assert_eq(Synth::ParamId::CC6, synth.find_param_id("CC6"));
    assert_eq(Synth::ParamId::CC7, synth.find_param_id("CC7"));
    assert_eq(Synth::ParamId::CC8, synth.find_param_id("CC8"));
    assert_eq(Synth::ParamId::CC9, synth.find_param_id("CC9"));
    assert_eq(Synth::ParamId::CC10, synth.find_param_id("CC10"));

    assert_eq(Synth::ParamId::CF1FRQ, synth.find_param_id("CF1FRQ"));
    assert_eq(Synth::ParamId::CF1Q, synth.find_param_id("CF1Q"));
    assert_eq(Synth::ParamId::CF1G, synth.find_param_id("CF1G"));

    assert_eq(Synth::ParamId::CF2FRQ, synth.find_param_id("CF2FRQ"));
    assert_eq(Synth::ParamId::CF2Q, synth.find_param_id("CF2Q"));
    assert_eq(Synth::ParamId::CF2G, synth.find_param_id("CF2G"));

    assert_eq(Synth::ParamId::EOG, synth.find_param_id("EOG"));

    assert_eq(Synth::ParamId::EDG, synth.find_param_id("EDG"));

    assert_eq(Synth::ParamId::EF1FRQ, synth.find_param_id("EF1FRQ"));
    assert_eq(Synth::ParamId::EF1Q, synth.find_param_id("EF1Q"));
    assert_eq(Synth::ParamId::EF1G, synth.find_param_id("EF1G"));

    assert_eq(Synth::ParamId::EF2FRQ, synth.find_param_id("EF2FRQ"));
    assert_eq(Synth::ParamId::EF2Q, synth.find_param_id("EF2Q"));
    assert_eq(Synth::ParamId::EF2G, synth.find_param_id("EF2G"));

    // TODO: turn on these tests when echo is implemented
    // assert_eq(Synth::ParamId::EEDEL, synth.find_param_id("EEDEL"));
    // assert_eq(Synth::ParamId::EEFB, synth.find_param_id("EEFB"));
    // assert_eq(Synth::ParamId::EEDF, synth.find_param_id("EEDF"));
    // assert_eq(Synth::ParamId::EEDG, synth.find_param_id("EEDG"));
    // assert_eq(Synth::ParamId::EEWID, synth.find_param_id("EEWID"));
    // assert_eq(Synth::ParamId::EEHPF, synth.find_param_id("EEHPF"));
    // assert_eq(Synth::ParamId::EEWET, synth.find_param_id("EEWET"));
    // assert_eq(Synth::ParamId::EEDRY, synth.find_param_id("EEDRY"));

    // TODO: turn on these tests when reverb is implemented
    // assert_eq(Synth::ParamId::ERRS, synth.find_param_id("ERRS"));
    // assert_eq(Synth::ParamId::ERDF, synth.find_param_id("ERDF"));
    // assert_eq(Synth::ParamId::ERDG, synth.find_param_id("ERDG"));
    // assert_eq(Synth::ParamId::ERWID, synth.find_param_id("ERWID"));
    // assert_eq(Synth::ParamId::ERHPF, synth.find_param_id("ERHPF"));
    // assert_eq(Synth::ParamId::ERWET, synth.find_param_id("ERWET"));
    // assert_eq(Synth::ParamId::ERDRY, synth.find_param_id("ERDRY"));

    assert_eq(Synth::ParamId::F1IN, synth.find_param_id("F1IN"));
    assert_eq(Synth::ParamId::F1MIN, synth.find_param_id("F1MIN"));
    assert_eq(Synth::ParamId::F1MAX, synth.find_param_id("F1MAX"));
    assert_eq(Synth::ParamId::F1AMT, synth.find_param_id("F1AMT"));
    assert_eq(Synth::ParamId::F1DST, synth.find_param_id("F1DST"));
    assert_eq(Synth::ParamId::F1RND, synth.find_param_id("F1RND"));

    assert_eq(Synth::ParamId::F2IN, synth.find_param_id("F2IN"));
    assert_eq(Synth::ParamId::F2MIN, synth.find_param_id("F2MIN"));
    assert_eq(Synth::ParamId::F2MAX, synth.find_param_id("F2MAX"));
    assert_eq(Synth::ParamId::F2AMT, synth.find_param_id("F2AMT"));
    assert_eq(Synth::ParamId::F2DST, synth.find_param_id("F2DST"));
    assert_eq(Synth::ParamId::F2RND, synth.find_param_id("F2RND"));

    assert_eq(Synth::ParamId::F3IN, synth.find_param_id("F3IN"));
    assert_eq(Synth::ParamId::F3MIN, synth.find_param_id("F3MIN"));
    assert_eq(Synth::ParamId::F3MAX, synth.find_param_id("F3MAX"));
    assert_eq(Synth::ParamId::F3AMT, synth.find_param_id("F3AMT"));
    assert_eq(Synth::ParamId::F3DST, synth.find_param_id("F3DST"));
    assert_eq(Synth::ParamId::F3RND, synth.find_param_id("F3RND"));

    assert_eq(Synth::ParamId::F4IN, synth.find_param_id("F4IN"));
    assert_eq(Synth::ParamId::F4MIN, synth.find_param_id("F4MIN"));
    assert_eq(Synth::ParamId::F4MAX, synth.find_param_id("F4MAX"));
    assert_eq(Synth::ParamId::F4AMT, synth.find_param_id("F4AMT"));
    assert_eq(Synth::ParamId::F4DST, synth.find_param_id("F4DST"));
    assert_eq(Synth::ParamId::F4RND, synth.find_param_id("F4RND"));

    assert_eq(Synth::ParamId::F5IN, synth.find_param_id("F5IN"));
    assert_eq(Synth::ParamId::F5MIN, synth.find_param_id("F5MIN"));
    assert_eq(Synth::ParamId::F5MAX, synth.find_param_id("F5MAX"));
    assert_eq(Synth::ParamId::F5AMT, synth.find_param_id("F5AMT"));
    assert_eq(Synth::ParamId::F5DST, synth.find_param_id("F5DST"));
    assert_eq(Synth::ParamId::F5RND, synth.find_param_id("F5RND"));

    assert_eq(Synth::ParamId::F6IN, synth.find_param_id("F6IN"));
    assert_eq(Synth::ParamId::F6MIN, synth.find_param_id("F6MIN"));
    assert_eq(Synth::ParamId::F6MAX, synth.find_param_id("F6MAX"));
    assert_eq(Synth::ParamId::F6AMT, synth.find_param_id("F6AMT"));
    assert_eq(Synth::ParamId::F6DST, synth.find_param_id("F6DST"));
    assert_eq(Synth::ParamId::F6RND, synth.find_param_id("F6RND"));

    assert_eq(Synth::ParamId::F7IN, synth.find_param_id("F7IN"));
    assert_eq(Synth::ParamId::F7MIN, synth.find_param_id("F7MIN"));
    assert_eq(Synth::ParamId::F7MAX, synth.find_param_id("F7MAX"));
    assert_eq(Synth::ParamId::F7AMT, synth.find_param_id("F7AMT"));
    assert_eq(Synth::ParamId::F7DST, synth.find_param_id("F7DST"));
    assert_eq(Synth::ParamId::F7RND, synth.find_param_id("F7RND"));

    assert_eq(Synth::ParamId::F8IN, synth.find_param_id("F8IN"));
    assert_eq(Synth::ParamId::F8MIN, synth.find_param_id("F8MIN"));
    assert_eq(Synth::ParamId::F8MAX, synth.find_param_id("F8MAX"));
    assert_eq(Synth::ParamId::F8AMT, synth.find_param_id("F8AMT"));
    assert_eq(Synth::ParamId::F8DST, synth.find_param_id("F8DST"));
    assert_eq(Synth::ParamId::F8RND, synth.find_param_id("F8RND"));

    assert_eq(Synth::ParamId::F9IN, synth.find_param_id("F9IN"));
    assert_eq(Synth::ParamId::F9MIN, synth.find_param_id("F9MIN"));
    assert_eq(Synth::ParamId::F9MAX, synth.find_param_id("F9MAX"));
    assert_eq(Synth::ParamId::F9AMT, synth.find_param_id("F9AMT"));
    assert_eq(Synth::ParamId::F9DST, synth.find_param_id("F9DST"));
    assert_eq(Synth::ParamId::F9RND, synth.find_param_id("F9RND"));

    assert_eq(Synth::ParamId::F10IN, synth.find_param_id("F10IN"));
    assert_eq(Synth::ParamId::F10MIN, synth.find_param_id("F10MIN"));
    assert_eq(Synth::ParamId::F10MAX, synth.find_param_id("F10MAX"));
    assert_eq(Synth::ParamId::F10AMT, synth.find_param_id("F10AMT"));
    assert_eq(Synth::ParamId::F10DST, synth.find_param_id("F10DST"));
    assert_eq(Synth::ParamId::F10RND, synth.find_param_id("F10RND"));

    assert_eq(Synth::ParamId::N1AMT, synth.find_param_id("N1AMT"));
    assert_eq(Synth::ParamId::N1INI, synth.find_param_id("N1INI"));
    assert_eq(Synth::ParamId::N1DEL, synth.find_param_id("N1DEL"));
    assert_eq(Synth::ParamId::N1ATK, synth.find_param_id("N1ATK"));
    assert_eq(Synth::ParamId::N1PK, synth.find_param_id("N1PK"));
    assert_eq(Synth::ParamId::N1HLD, synth.find_param_id("N1HLD"));
    assert_eq(Synth::ParamId::N1DEC, synth.find_param_id("N1DEC"));
    assert_eq(Synth::ParamId::N1SUS, synth.find_param_id("N1SUS"));
    assert_eq(Synth::ParamId::N1REL, synth.find_param_id("N1REL"));
    assert_eq(Synth::ParamId::N1FIN, synth.find_param_id("N1FIN"));

    assert_eq(Synth::ParamId::N2AMT, synth.find_param_id("N2AMT"));
    assert_eq(Synth::ParamId::N2INI, synth.find_param_id("N2INI"));
    assert_eq(Synth::ParamId::N2DEL, synth.find_param_id("N2DEL"));
    assert_eq(Synth::ParamId::N2ATK, synth.find_param_id("N2ATK"));
    assert_eq(Synth::ParamId::N2PK, synth.find_param_id("N2PK"));
    assert_eq(Synth::ParamId::N2HLD, synth.find_param_id("N2HLD"));
    assert_eq(Synth::ParamId::N2DEC, synth.find_param_id("N2DEC"));
    assert_eq(Synth::ParamId::N2SUS, synth.find_param_id("N2SUS"));
    assert_eq(Synth::ParamId::N2REL, synth.find_param_id("N2REL"));
    assert_eq(Synth::ParamId::N2FIN, synth.find_param_id("N2FIN"));

    assert_eq(Synth::ParamId::N3AMT, synth.find_param_id("N3AMT"));
    assert_eq(Synth::ParamId::N3INI, synth.find_param_id("N3INI"));
    assert_eq(Synth::ParamId::N3DEL, synth.find_param_id("N3DEL"));
    assert_eq(Synth::ParamId::N3ATK, synth.find_param_id("N3ATK"));
    assert_eq(Synth::ParamId::N3PK, synth.find_param_id("N3PK"));
    assert_eq(Synth::ParamId::N3HLD, synth.find_param_id("N3HLD"));
    assert_eq(Synth::ParamId::N3DEC, synth.find_param_id("N3DEC"));
    assert_eq(Synth::ParamId::N3SUS, synth.find_param_id("N3SUS"));
    assert_eq(Synth::ParamId::N3REL, synth.find_param_id("N3REL"));
    assert_eq(Synth::ParamId::N3FIN, synth.find_param_id("N3FIN"));

    assert_eq(Synth::ParamId::N4AMT, synth.find_param_id("N4AMT"));
    assert_eq(Synth::ParamId::N4INI, synth.find_param_id("N4INI"));
    assert_eq(Synth::ParamId::N4DEL, synth.find_param_id("N4DEL"));
    assert_eq(Synth::ParamId::N4ATK, synth.find_param_id("N4ATK"));
    assert_eq(Synth::ParamId::N4PK, synth.find_param_id("N4PK"));
    assert_eq(Synth::ParamId::N4HLD, synth.find_param_id("N4HLD"));
    assert_eq(Synth::ParamId::N4DEC, synth.find_param_id("N4DEC"));
    assert_eq(Synth::ParamId::N4SUS, synth.find_param_id("N4SUS"));
    assert_eq(Synth::ParamId::N4REL, synth.find_param_id("N4REL"));
    assert_eq(Synth::ParamId::N4FIN, synth.find_param_id("N4FIN"));

    assert_eq(Synth::ParamId::N5AMT, synth.find_param_id("N5AMT"));
    assert_eq(Synth::ParamId::N5INI, synth.find_param_id("N5INI"));
    assert_eq(Synth::ParamId::N5DEL, synth.find_param_id("N5DEL"));
    assert_eq(Synth::ParamId::N5ATK, synth.find_param_id("N5ATK"));
    assert_eq(Synth::ParamId::N5PK, synth.find_param_id("N5PK"));
    assert_eq(Synth::ParamId::N5HLD, synth.find_param_id("N5HLD"));
    assert_eq(Synth::ParamId::N5DEC, synth.find_param_id("N5DEC"));
    assert_eq(Synth::ParamId::N5SUS, synth.find_param_id("N5SUS"));
    assert_eq(Synth::ParamId::N5REL, synth.find_param_id("N5REL"));
    assert_eq(Synth::ParamId::N5FIN, synth.find_param_id("N5FIN"));

    assert_eq(Synth::ParamId::N6AMT, synth.find_param_id("N6AMT"));
    assert_eq(Synth::ParamId::N6INI, synth.find_param_id("N6INI"));
    assert_eq(Synth::ParamId::N6DEL, synth.find_param_id("N6DEL"));
    assert_eq(Synth::ParamId::N6ATK, synth.find_param_id("N6ATK"));
    assert_eq(Synth::ParamId::N6PK, synth.find_param_id("N6PK"));
    assert_eq(Synth::ParamId::N6HLD, synth.find_param_id("N6HLD"));
    assert_eq(Synth::ParamId::N6DEC, synth.find_param_id("N6DEC"));
    assert_eq(Synth::ParamId::N6SUS, synth.find_param_id("N6SUS"));
    assert_eq(Synth::ParamId::N6REL, synth.find_param_id("N6REL"));
    assert_eq(Synth::ParamId::N6FIN, synth.find_param_id("N6FIN"));

    // TODO: turn on these tests when LFOs are implemented
    // assert_eq(Synth::ParamId::L1FRQ, synth.find_param_id("L1FRQ"));
    // assert_eq(Synth::ParamId::L1AMT, synth.find_param_id("L1AMT"));
    // assert_eq(Synth::ParamId::L1MIN, synth.find_param_id("L1MIN"));
    // assert_eq(Synth::ParamId::L1MAX, synth.find_param_id("L1MAX"));
    // assert_eq(Synth::ParamId::L1DST, synth.find_param_id("L1DST"));
    // assert_eq(Synth::ParamId::L1RND, synth.find_param_id("L1RND"));

    // assert_eq(Synth::ParamId::L2FRQ, synth.find_param_id("L2FRQ"));
    // assert_eq(Synth::ParamId::L2AMT, synth.find_param_id("L2AMT"));
    // assert_eq(Synth::ParamId::L2MIN, synth.find_param_id("L2MIN"));
    // assert_eq(Synth::ParamId::L2MAX, synth.find_param_id("L2MAX"));
    // assert_eq(Synth::ParamId::L2DST, synth.find_param_id("L2DST"));
    // assert_eq(Synth::ParamId::L2RND, synth.find_param_id("L2RND"));

    // assert_eq(Synth::ParamId::L3FRQ, synth.find_param_id("L3FRQ"));
    // assert_eq(Synth::ParamId::L3AMT, synth.find_param_id("L3AMT"));
    // assert_eq(Synth::ParamId::L3MIN, synth.find_param_id("L3MIN"));
    // assert_eq(Synth::ParamId::L3MAX, synth.find_param_id("L3MAX"));
    // assert_eq(Synth::ParamId::L3DST, synth.find_param_id("L3DST"));
    // assert_eq(Synth::ParamId::L3RND, synth.find_param_id("L3RND"));

    // assert_eq(Synth::ParamId::L4FRQ, synth.find_param_id("L4FRQ"));
    // assert_eq(Synth::ParamId::L4AMT, synth.find_param_id("L4AMT"));
    // assert_eq(Synth::ParamId::L4MIN, synth.find_param_id("L4MIN"));
    // assert_eq(Synth::ParamId::L4MAX, synth.find_param_id("L4MAX"));
    // assert_eq(Synth::ParamId::L4DST, synth.find_param_id("L4DST"));
    // assert_eq(Synth::ParamId::L4RND, synth.find_param_id("L4RND"));

    // assert_eq(Synth::ParamId::L5FRQ, synth.find_param_id("L5FRQ"));
    // assert_eq(Synth::ParamId::L5AMT, synth.find_param_id("L5AMT"));
    // assert_eq(Synth::ParamId::L5MIN, synth.find_param_id("L5MIN"));
    // assert_eq(Synth::ParamId::L5MAX, synth.find_param_id("L5MAX"));
    // assert_eq(Synth::ParamId::L5DST, synth.find_param_id("L5DST"));
    // assert_eq(Synth::ParamId::L5RND, synth.find_param_id("L5RND"));

    // assert_eq(Synth::ParamId::L6FRQ, synth.find_param_id("L6FRQ"));
    // assert_eq(Synth::ParamId::L6AMT, synth.find_param_id("L6AMT"));
    // assert_eq(Synth::ParamId::L6MIN, synth.find_param_id("L6MIN"));
    // assert_eq(Synth::ParamId::L6MAX, synth.find_param_id("L6MAX"));
    // assert_eq(Synth::ParamId::L6DST, synth.find_param_id("L6DST"));
    // assert_eq(Synth::ParamId::L6RND, synth.find_param_id("L6RND"));

    // assert_eq(Synth::ParamId::L7FRQ, synth.find_param_id("L7FRQ"));
    // assert_eq(Synth::ParamId::L7AMT, synth.find_param_id("L7AMT"));
    // assert_eq(Synth::ParamId::L7MIN, synth.find_param_id("L7MIN"));
    // assert_eq(Synth::ParamId::L7MAX, synth.find_param_id("L7MAX"));
    // assert_eq(Synth::ParamId::L7DST, synth.find_param_id("L7DST"));
    // assert_eq(Synth::ParamId::L7RND, synth.find_param_id("L7RND"));

    // assert_eq(Synth::ParamId::L8FRQ, synth.find_param_id("L8FRQ"));
    // assert_eq(Synth::ParamId::L8AMT, synth.find_param_id("L8AMT"));
    // assert_eq(Synth::ParamId::L8MIN, synth.find_param_id("L8MIN"));
    // assert_eq(Synth::ParamId::L8MAX, synth.find_param_id("L8MAX"));
    // assert_eq(Synth::ParamId::L8DST, synth.find_param_id("L8DST"));
    // assert_eq(Synth::ParamId::L8RND, synth.find_param_id("L8RND"));

    // TODO: turn on this test when MODE is implemented
    // assert_eq(Synth::ParamId::MODE, synth.find_param_id("MODE"));

    assert_eq(Synth::ParamId::MWAV, synth.find_param_id("MWAV"));
    assert_eq(Synth::ParamId::CWAV, synth.find_param_id("CWAV"));

    assert_eq(Synth::ParamId::MF1TYP, synth.find_param_id("MF1TYP"));
    assert_eq(Synth::ParamId::MF2TYP, synth.find_param_id("MF2TYP"));

    assert_eq(Synth::ParamId::CF1TYP, synth.find_param_id("CF1TYP"));
    assert_eq(Synth::ParamId::CF2TYP, synth.find_param_id("CF2TYP"));

    assert_eq(Synth::ParamId::EF1TYP, synth.find_param_id("EF1TYP"));
    assert_eq(Synth::ParamId::EF2TYP, synth.find_param_id("EF2TYP"));

    // TODO: turn on these tests when LFOs are implemented
    // assert_eq(Synth::ParamId::L1WAV, synth.find_param_id("L1WAV"));
    // assert_eq(Synth::ParamId::L2WAV, synth.find_param_id("L2WAV"));
    // assert_eq(Synth::ParamId::L3WAV, synth.find_param_id("L3WAV"));
    // assert_eq(Synth::ParamId::L4WAV, synth.find_param_id("L4WAV"));
    // assert_eq(Synth::ParamId::L5WAV, synth.find_param_id("L5WAV"));
    // assert_eq(Synth::ParamId::L6WAV, synth.find_param_id("L6WAV"));
    // assert_eq(Synth::ParamId::L7WAV, synth.find_param_id("L7WAV"));
    // assert_eq(Synth::ParamId::L8WAV, synth.find_param_id("L8WAV"));
})
