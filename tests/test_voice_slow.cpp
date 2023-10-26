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

#include <cmath>
#include <vector>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/biquad_filter.cpp"
#include "dsp/delay.cpp"
#include "dsp/distortion.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/lfo.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavefolder.cpp"
#include "dsp/wavetable.cpp"

#include "synth.cpp"
#include "voice.cpp"


using namespace JS80P;


typedef Voice<SignalProducer> SimpleVoice;

constexpr FrequencyTable FREQUENCIES{};

constexpr PerChannelFrequencyTable PER_CHANNEL_FREQUENCIES{};


TEST(inaccuracy_keeps_changing_for_each_note, {
    constexpr Integer probes = 100000;

    SimpleVoice::Params params("V");

    for (Integer i = 0; i != Synth::POLYPHONY; ++i) {
        Inaccuracy synced_inaccuracy(0.5);
        Number const inaccuracy = Synth::calculate_inaccuracy_seed(i);

        SimpleVoice voice(
            FREQUENCIES,
            PER_CHANNEL_FREQUENCIES,
            synced_inaccuracy,
            inaccuracy,
            params
        );
        std::vector<Number> inaccuracies(probes);
        Math::Statistics statistics;

        voice.set_block_size(10);
        voice.set_sample_rate(1000.0);

        for (Integer j = 0; j != probes; ++j) {
            voice.note_on(0.001, j, Midi::NOTE_A_3, 0, 1.0, 0);
            voice.note_off(0.002, j, Midi::NOTE_A_3, 1.0);
            SignalProducer::produce<SimpleVoice>(voice, j);

            inaccuracies[j] = voice.get_inaccuracy();
        }

        Math::compute_statistics(inaccuracies, statistics);

        assert_statistics(true, 0.1, 0.55, 1.0, 0.55, 0.225, statistics, 0.02);
    }
})
