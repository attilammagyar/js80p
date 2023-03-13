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


class ParamIdNamePair
{
    public:
        ParamIdNamePair(Synth::ParamId const param_id, std::string const name)
            : param_id(param_id),
            name(name)
        {
        }

        Synth::ParamId const param_id;
        std::string const name;
};


// TODO: remove this vector once all params are implemented, and use Synth::PARAM_NAMES instead
std::vector<ParamIdNamePair> PARAMS({
    ParamIdNamePair(Synth::ParamId::VOL, "VOL"),

    ParamIdNamePair(Synth::ParamId::ADD, "ADD"),
    ParamIdNamePair(Synth::ParamId::FM, "FM"),
    ParamIdNamePair(Synth::ParamId::AM, "AM"),

    ParamIdNamePair(Synth::ParamId::MAMP, "MAMP"),
    ParamIdNamePair(Synth::ParamId::MVS, "MVS"),
    ParamIdNamePair(Synth::ParamId::MFLD, "MFLD"),
    ParamIdNamePair(Synth::ParamId::MPRT, "MPRT"),
    ParamIdNamePair(Synth::ParamId::MPRD, "MPRD"),
    ParamIdNamePair(Synth::ParamId::MDTN, "MDTN"),
    ParamIdNamePair(Synth::ParamId::MFIN, "MFIN"),
    ParamIdNamePair(Synth::ParamId::MWID, "MWID"),
    ParamIdNamePair(Synth::ParamId::MPAN, "MPAN"),
    ParamIdNamePair(Synth::ParamId::MVOL, "MVOL"),

    ParamIdNamePair(Synth::ParamId::MC1, "MC1"),
    ParamIdNamePair(Synth::ParamId::MC2, "MC2"),
    ParamIdNamePair(Synth::ParamId::MC3, "MC3"),
    ParamIdNamePair(Synth::ParamId::MC4, "MC4"),
    ParamIdNamePair(Synth::ParamId::MC5, "MC5"),
    ParamIdNamePair(Synth::ParamId::MC6, "MC6"),
    ParamIdNamePair(Synth::ParamId::MC7, "MC7"),
    ParamIdNamePair(Synth::ParamId::MC8, "MC8"),
    ParamIdNamePair(Synth::ParamId::MC9, "MC9"),
    ParamIdNamePair(Synth::ParamId::MC10, "MC10"),

    ParamIdNamePair(Synth::ParamId::MF1FRQ, "MF1FRQ"),
    ParamIdNamePair(Synth::ParamId::MF1Q, "MF1Q"),
    ParamIdNamePair(Synth::ParamId::MF1G, "MF1G"),

    ParamIdNamePair(Synth::ParamId::MF2FRQ, "MF2FRQ"),
    ParamIdNamePair(Synth::ParamId::MF2Q, "MF2Q"),
    ParamIdNamePair(Synth::ParamId::MF2G, "MF2G"),

    ParamIdNamePair(Synth::ParamId::CAMP, "CAMP"),
    ParamIdNamePair(Synth::ParamId::CVS, "CVS"),
    ParamIdNamePair(Synth::ParamId::CFLD, "CFLD"),
    ParamIdNamePair(Synth::ParamId::CPRT, "CPRT"),
    ParamIdNamePair(Synth::ParamId::CPRD, "CPRD"),
    ParamIdNamePair(Synth::ParamId::CDTN, "CDTN"),
    ParamIdNamePair(Synth::ParamId::CFIN, "CFIN"),
    ParamIdNamePair(Synth::ParamId::CWID, "CWID"),
    ParamIdNamePair(Synth::ParamId::CPAN, "CPAN"),
    ParamIdNamePair(Synth::ParamId::CVOL, "CVOL"),

    ParamIdNamePair(Synth::ParamId::CC1, "CC1"),
    ParamIdNamePair(Synth::ParamId::CC2, "CC2"),
    ParamIdNamePair(Synth::ParamId::CC3, "CC3"),
    ParamIdNamePair(Synth::ParamId::CC4, "CC4"),
    ParamIdNamePair(Synth::ParamId::CC5, "CC5"),
    ParamIdNamePair(Synth::ParamId::CC6, "CC6"),
    ParamIdNamePair(Synth::ParamId::CC7, "CC7"),
    ParamIdNamePair(Synth::ParamId::CC8, "CC8"),
    ParamIdNamePair(Synth::ParamId::CC9, "CC9"),
    ParamIdNamePair(Synth::ParamId::CC10, "CC10"),

    ParamIdNamePair(Synth::ParamId::CF1FRQ, "CF1FRQ"),
    ParamIdNamePair(Synth::ParamId::CF1Q, "CF1Q"),
    ParamIdNamePair(Synth::ParamId::CF1G, "CF1G"),

    ParamIdNamePair(Synth::ParamId::CF2FRQ, "CF2FRQ"),
    ParamIdNamePair(Synth::ParamId::CF2Q, "CF2Q"),
    ParamIdNamePair(Synth::ParamId::CF2G, "CF2G"),

    ParamIdNamePair(Synth::ParamId::EOG, "EOG"),

    ParamIdNamePair(Synth::ParamId::EDG, "EDG"),

    ParamIdNamePair(Synth::ParamId::EF1FRQ, "EF1FRQ"),
    ParamIdNamePair(Synth::ParamId::EF1Q, "EF1Q"),
    ParamIdNamePair(Synth::ParamId::EF1G, "EF1G"),

    ParamIdNamePair(Synth::ParamId::EF2FRQ, "EF2FRQ"),
    ParamIdNamePair(Synth::ParamId::EF2Q, "EF2Q"),
    ParamIdNamePair(Synth::ParamId::EF2G, "EF2G"),

    // TODO: turn on these when Echo is implemented
    // ParamIdNamePair(Synth::ParamId::EEDEL, "EEDEL"),
    // ParamIdNamePair(Synth::ParamId::EEFB, "EEFB"),
    // ParamIdNamePair(Synth::ParamId::EEDF, "EEDF"),
    // ParamIdNamePair(Synth::ParamId::EEDG, "EEDG"),
    // ParamIdNamePair(Synth::ParamId::EEWID, "EEWID"),
    // ParamIdNamePair(Synth::ParamId::EEHPF, "EEHPF"),
    // ParamIdNamePair(Synth::ParamId::EEWET, "EEWET"),
    // ParamIdNamePair(Synth::ParamId::EEDRY, "EEDRY"),

    // TODO: turn on these when Reverb is implemented
    // ParamIdNamePair(Synth::ParamId::ERRS, "ERRS"),
    // ParamIdNamePair(Synth::ParamId::ERDF, "ERDF"),
    // ParamIdNamePair(Synth::ParamId::ERDG, "ERDG"),
    // ParamIdNamePair(Synth::ParamId::ERWID, "ERWID"),
    // ParamIdNamePair(Synth::ParamId::ERHPF, "ERHPF"),
    // ParamIdNamePair(Synth::ParamId::ERWET, "ERWET"),
    // ParamIdNamePair(Synth::ParamId::ERDRY, "ERDRY"),

    ParamIdNamePair(Synth::ParamId::F1IN, "F1IN"),
    ParamIdNamePair(Synth::ParamId::F1MIN, "F1MIN"),
    ParamIdNamePair(Synth::ParamId::F1MAX, "F1MAX"),
    ParamIdNamePair(Synth::ParamId::F1AMT, "F1AMT"),
    ParamIdNamePair(Synth::ParamId::F1DST, "F1DST"),
    ParamIdNamePair(Synth::ParamId::F1RND, "F1RND"),

    ParamIdNamePair(Synth::ParamId::F2IN, "F2IN"),
    ParamIdNamePair(Synth::ParamId::F2MIN, "F2MIN"),
    ParamIdNamePair(Synth::ParamId::F2MAX, "F2MAX"),
    ParamIdNamePair(Synth::ParamId::F2AMT, "F2AMT"),
    ParamIdNamePair(Synth::ParamId::F2DST, "F2DST"),
    ParamIdNamePair(Synth::ParamId::F2RND, "F2RND"),

    ParamIdNamePair(Synth::ParamId::F3IN, "F3IN"),
    ParamIdNamePair(Synth::ParamId::F3MIN, "F3MIN"),
    ParamIdNamePair(Synth::ParamId::F3MAX, "F3MAX"),
    ParamIdNamePair(Synth::ParamId::F3AMT, "F3AMT"),
    ParamIdNamePair(Synth::ParamId::F3DST, "F3DST"),
    ParamIdNamePair(Synth::ParamId::F3RND, "F3RND"),

    ParamIdNamePair(Synth::ParamId::F4IN, "F4IN"),
    ParamIdNamePair(Synth::ParamId::F4MIN, "F4MIN"),
    ParamIdNamePair(Synth::ParamId::F4MAX, "F4MAX"),
    ParamIdNamePair(Synth::ParamId::F4AMT, "F4AMT"),
    ParamIdNamePair(Synth::ParamId::F4DST, "F4DST"),
    ParamIdNamePair(Synth::ParamId::F4RND, "F4RND"),

    ParamIdNamePair(Synth::ParamId::F5IN, "F5IN"),
    ParamIdNamePair(Synth::ParamId::F5MIN, "F5MIN"),
    ParamIdNamePair(Synth::ParamId::F5MAX, "F5MAX"),
    ParamIdNamePair(Synth::ParamId::F5AMT, "F5AMT"),
    ParamIdNamePair(Synth::ParamId::F5DST, "F5DST"),
    ParamIdNamePair(Synth::ParamId::F5RND, "F5RND"),

    ParamIdNamePair(Synth::ParamId::F6IN, "F6IN"),
    ParamIdNamePair(Synth::ParamId::F6MIN, "F6MIN"),
    ParamIdNamePair(Synth::ParamId::F6MAX, "F6MAX"),
    ParamIdNamePair(Synth::ParamId::F6AMT, "F6AMT"),
    ParamIdNamePair(Synth::ParamId::F6DST, "F6DST"),
    ParamIdNamePair(Synth::ParamId::F6RND, "F6RND"),

    ParamIdNamePair(Synth::ParamId::F7IN, "F7IN"),
    ParamIdNamePair(Synth::ParamId::F7MIN, "F7MIN"),
    ParamIdNamePair(Synth::ParamId::F7MAX, "F7MAX"),
    ParamIdNamePair(Synth::ParamId::F7AMT, "F7AMT"),
    ParamIdNamePair(Synth::ParamId::F7DST, "F7DST"),
    ParamIdNamePair(Synth::ParamId::F7RND, "F7RND"),

    ParamIdNamePair(Synth::ParamId::F8IN, "F8IN"),
    ParamIdNamePair(Synth::ParamId::F8MIN, "F8MIN"),
    ParamIdNamePair(Synth::ParamId::F8MAX, "F8MAX"),
    ParamIdNamePair(Synth::ParamId::F8AMT, "F8AMT"),
    ParamIdNamePair(Synth::ParamId::F8DST, "F8DST"),
    ParamIdNamePair(Synth::ParamId::F8RND, "F8RND"),

    ParamIdNamePair(Synth::ParamId::F9IN, "F9IN"),
    ParamIdNamePair(Synth::ParamId::F9MIN, "F9MIN"),
    ParamIdNamePair(Synth::ParamId::F9MAX, "F9MAX"),
    ParamIdNamePair(Synth::ParamId::F9AMT, "F9AMT"),
    ParamIdNamePair(Synth::ParamId::F9DST, "F9DST"),
    ParamIdNamePair(Synth::ParamId::F9RND, "F9RND"),

    ParamIdNamePair(Synth::ParamId::F10IN, "F10IN"),
    ParamIdNamePair(Synth::ParamId::F10MIN, "F10MIN"),
    ParamIdNamePair(Synth::ParamId::F10MAX, "F10MAX"),
    ParamIdNamePair(Synth::ParamId::F10AMT, "F10AMT"),
    ParamIdNamePair(Synth::ParamId::F10DST, "F10DST"),
    ParamIdNamePair(Synth::ParamId::F10RND, "F10RND"),

    ParamIdNamePair(Synth::ParamId::N1AMT, "N1AMT"),
    ParamIdNamePair(Synth::ParamId::N1INI, "N1INI"),
    ParamIdNamePair(Synth::ParamId::N1DEL, "N1DEL"),
    ParamIdNamePair(Synth::ParamId::N1ATK, "N1ATK"),
    ParamIdNamePair(Synth::ParamId::N1PK, "N1PK"),
    ParamIdNamePair(Synth::ParamId::N1HLD, "N1HLD"),
    ParamIdNamePair(Synth::ParamId::N1DEC, "N1DEC"),
    ParamIdNamePair(Synth::ParamId::N1SUS, "N1SUS"),
    ParamIdNamePair(Synth::ParamId::N1REL, "N1REL"),
    ParamIdNamePair(Synth::ParamId::N1FIN, "N1FIN"),

    ParamIdNamePair(Synth::ParamId::N2AMT, "N2AMT"),
    ParamIdNamePair(Synth::ParamId::N2INI, "N2INI"),
    ParamIdNamePair(Synth::ParamId::N2DEL, "N2DEL"),
    ParamIdNamePair(Synth::ParamId::N2ATK, "N2ATK"),
    ParamIdNamePair(Synth::ParamId::N2PK, "N2PK"),
    ParamIdNamePair(Synth::ParamId::N2HLD, "N2HLD"),
    ParamIdNamePair(Synth::ParamId::N2DEC, "N2DEC"),
    ParamIdNamePair(Synth::ParamId::N2SUS, "N2SUS"),
    ParamIdNamePair(Synth::ParamId::N2REL, "N2REL"),
    ParamIdNamePair(Synth::ParamId::N2FIN, "N2FIN"),

    ParamIdNamePair(Synth::ParamId::N3AMT, "N3AMT"),
    ParamIdNamePair(Synth::ParamId::N3INI, "N3INI"),
    ParamIdNamePair(Synth::ParamId::N3DEL, "N3DEL"),
    ParamIdNamePair(Synth::ParamId::N3ATK, "N3ATK"),
    ParamIdNamePair(Synth::ParamId::N3PK, "N3PK"),
    ParamIdNamePair(Synth::ParamId::N3HLD, "N3HLD"),
    ParamIdNamePair(Synth::ParamId::N3DEC, "N3DEC"),
    ParamIdNamePair(Synth::ParamId::N3SUS, "N3SUS"),
    ParamIdNamePair(Synth::ParamId::N3REL, "N3REL"),
    ParamIdNamePair(Synth::ParamId::N3FIN, "N3FIN"),

    ParamIdNamePair(Synth::ParamId::N4AMT, "N4AMT"),
    ParamIdNamePair(Synth::ParamId::N4INI, "N4INI"),
    ParamIdNamePair(Synth::ParamId::N4DEL, "N4DEL"),
    ParamIdNamePair(Synth::ParamId::N4ATK, "N4ATK"),
    ParamIdNamePair(Synth::ParamId::N4PK, "N4PK"),
    ParamIdNamePair(Synth::ParamId::N4HLD, "N4HLD"),
    ParamIdNamePair(Synth::ParamId::N4DEC, "N4DEC"),
    ParamIdNamePair(Synth::ParamId::N4SUS, "N4SUS"),
    ParamIdNamePair(Synth::ParamId::N4REL, "N4REL"),
    ParamIdNamePair(Synth::ParamId::N4FIN, "N4FIN"),

    ParamIdNamePair(Synth::ParamId::N5AMT, "N5AMT"),
    ParamIdNamePair(Synth::ParamId::N5INI, "N5INI"),
    ParamIdNamePair(Synth::ParamId::N5DEL, "N5DEL"),
    ParamIdNamePair(Synth::ParamId::N5ATK, "N5ATK"),
    ParamIdNamePair(Synth::ParamId::N5PK, "N5PK"),
    ParamIdNamePair(Synth::ParamId::N5HLD, "N5HLD"),
    ParamIdNamePair(Synth::ParamId::N5DEC, "N5DEC"),
    ParamIdNamePair(Synth::ParamId::N5SUS, "N5SUS"),
    ParamIdNamePair(Synth::ParamId::N5REL, "N5REL"),
    ParamIdNamePair(Synth::ParamId::N5FIN, "N5FIN"),

    ParamIdNamePair(Synth::ParamId::N6AMT, "N6AMT"),
    ParamIdNamePair(Synth::ParamId::N6INI, "N6INI"),
    ParamIdNamePair(Synth::ParamId::N6DEL, "N6DEL"),
    ParamIdNamePair(Synth::ParamId::N6ATK, "N6ATK"),
    ParamIdNamePair(Synth::ParamId::N6PK, "N6PK"),
    ParamIdNamePair(Synth::ParamId::N6HLD, "N6HLD"),
    ParamIdNamePair(Synth::ParamId::N6DEC, "N6DEC"),
    ParamIdNamePair(Synth::ParamId::N6SUS, "N6SUS"),
    ParamIdNamePair(Synth::ParamId::N6REL, "N6REL"),
    ParamIdNamePair(Synth::ParamId::N6FIN, "N6FIN"),

    // TODO: turn on these when LFOs are implemented
    // ParamIdNamePair(Synth::ParamId::L1FRQ, "L1FRQ"),
    // ParamIdNamePair(Synth::ParamId::L1AMT, "L1AMT"),
    // ParamIdNamePair(Synth::ParamId::L1MIN, "L1MIN"),
    // ParamIdNamePair(Synth::ParamId::L1MAX, "L1MAX"),
    // ParamIdNamePair(Synth::ParamId::L1DST, "L1DST"),
    // ParamIdNamePair(Synth::ParamId::L1RND, "L1RND"),

    // ParamIdNamePair(Synth::ParamId::L2FRQ, "L2FRQ"),
    // ParamIdNamePair(Synth::ParamId::L2AMT, "L2AMT"),
    // ParamIdNamePair(Synth::ParamId::L2MIN, "L2MIN"),
    // ParamIdNamePair(Synth::ParamId::L2MAX, "L2MAX"),
    // ParamIdNamePair(Synth::ParamId::L2DST, "L2DST"),
    // ParamIdNamePair(Synth::ParamId::L2RND, "L2RND"),

    // ParamIdNamePair(Synth::ParamId::L3FRQ, "L3FRQ"),
    // ParamIdNamePair(Synth::ParamId::L3AMT, "L3AMT"),
    // ParamIdNamePair(Synth::ParamId::L3MIN, "L3MIN"),
    // ParamIdNamePair(Synth::ParamId::L3MAX, "L3MAX"),
    // ParamIdNamePair(Synth::ParamId::L3DST, "L3DST"),
    // ParamIdNamePair(Synth::ParamId::L3RND, "L3RND"),

    // ParamIdNamePair(Synth::ParamId::L4FRQ, "L4FRQ"),
    // ParamIdNamePair(Synth::ParamId::L4AMT, "L4AMT"),
    // ParamIdNamePair(Synth::ParamId::L4MIN, "L4MIN"),
    // ParamIdNamePair(Synth::ParamId::L4MAX, "L4MAX"),
    // ParamIdNamePair(Synth::ParamId::L4DST, "L4DST"),
    // ParamIdNamePair(Synth::ParamId::L4RND, "L4RND"),

    // ParamIdNamePair(Synth::ParamId::L5FRQ, "L5FRQ"),
    // ParamIdNamePair(Synth::ParamId::L5AMT, "L5AMT"),
    // ParamIdNamePair(Synth::ParamId::L5MIN, "L5MIN"),
    // ParamIdNamePair(Synth::ParamId::L5MAX, "L5MAX"),
    // ParamIdNamePair(Synth::ParamId::L5DST, "L5DST"),
    // ParamIdNamePair(Synth::ParamId::L5RND, "L5RND"),

    // ParamIdNamePair(Synth::ParamId::L6FRQ, "L6FRQ"),
    // ParamIdNamePair(Synth::ParamId::L6AMT, "L6AMT"),
    // ParamIdNamePair(Synth::ParamId::L6MIN, "L6MIN"),
    // ParamIdNamePair(Synth::ParamId::L6MAX, "L6MAX"),
    // ParamIdNamePair(Synth::ParamId::L6DST, "L6DST"),
    // ParamIdNamePair(Synth::ParamId::L6RND, "L6RND"),

    // ParamIdNamePair(Synth::ParamId::L7FRQ, "L7FRQ"),
    // ParamIdNamePair(Synth::ParamId::L7AMT, "L7AMT"),
    // ParamIdNamePair(Synth::ParamId::L7MIN, "L7MIN"),
    // ParamIdNamePair(Synth::ParamId::L7MAX, "L7MAX"),
    // ParamIdNamePair(Synth::ParamId::L7DST, "L7DST"),
    // ParamIdNamePair(Synth::ParamId::L7RND, "L7RND"),

    // ParamIdNamePair(Synth::ParamId::L8FRQ, "L8FRQ"),
    // ParamIdNamePair(Synth::ParamId::L8AMT, "L8AMT"),
    // ParamIdNamePair(Synth::ParamId::L8MIN, "L8MIN"),
    // ParamIdNamePair(Synth::ParamId::L8MAX, "L8MAX"),
    // ParamIdNamePair(Synth::ParamId::L8DST, "L8DST"),
    // ParamIdNamePair(Synth::ParamId::L8RND, "L8RND"),

    // TODO: turn on this when MODE is implemented
    // ParamIdNamePair(Synth::ParamId::MODE, "MODE"),

    ParamIdNamePair(Synth::ParamId::MWAV, "MWAV"),
    ParamIdNamePair(Synth::ParamId::CWAV, "CWAV"),

    ParamIdNamePair(Synth::ParamId::MF1TYP, "MF1TYP"),
    ParamIdNamePair(Synth::ParamId::MF2TYP, "MF2TYP"),

    ParamIdNamePair(Synth::ParamId::CF1TYP, "CF1TYP"),
    ParamIdNamePair(Synth::ParamId::CF2TYP, "CF2TYP"),

    ParamIdNamePair(Synth::ParamId::EF1TYP, "EF1TYP"),
    ParamIdNamePair(Synth::ParamId::EF2TYP, "EF2TYP"),

    // TODO: turn on these when LFOs are implemented
    // ParamIdNamePair(Synth::ParamId::L1WAV, "L1WAV"),
    // ParamIdNamePair(Synth::ParamId::L2WAV, "L2WAV"),
    // ParamIdNamePair(Synth::ParamId::L3WAV, "L3WAV"),
    // ParamIdNamePair(Synth::ParamId::L4WAV, "L4WAV"),
    // ParamIdNamePair(Synth::ParamId::L5WAV, "L5WAV"),
    // ParamIdNamePair(Synth::ParamId::L6WAV, "L6WAV"),
    // ParamIdNamePair(Synth::ParamId::L7WAV, "L7WAV"),
    // ParamIdNamePair(Synth::ParamId::L8WAV, "L8WAV"),
});


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
        Synth::MessageType::ASSIGN_CONTROLLER,
        Synth::ParamId::CWAV,
        0.0,
        Synth::ControllerId::MODULATION_WHEEL
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

    assert_eq(0.42, synth.volume.get_value(), DOUBLE_DELTA);
    assert_eq(0.9, synth.modulator_params.amplitude.get_value(), DOUBLE_DELTA);
    assert_eq(
        600.0, synth.modulator_params.fine_detune.get_value(), DOUBLE_DELTA
    );
    assert_eq(SimpleOscillator::CUSTOM, synth.carrier_params.waveform.get_value());

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

    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id(""));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id(" \n"));
    assert_eq(Synth::ParamId::MAX_PARAM_ID, synth.get_param_id("NO_SUCH_PARAM"));

    for (std::vector<ParamIdNamePair>::iterator it = PARAMS.begin(); it != PARAMS.end(); ++it) {
        assert_eq(it->name, synth.get_param_name(it->param_id).c_str());
        assert_eq(it->param_id, synth.get_param_id(it->name));
    }
})
