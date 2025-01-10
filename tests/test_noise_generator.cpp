/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025  Attila M. Magyar
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
#include <vector>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/noise_generator.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


constexpr Integer BLOCK_SIZE = 8192;


void assert_channel_statsistics(
        Sample const* const samples,
        Number const expected_min,
        Number const expected_median,
        Number const expected_max,
        Number const expected_mean,
        Number const expected_standard_deviation
) {
    std::vector<Number> samples_vector;
    Math::Statistics stats;

    samples_vector.reserve(BLOCK_SIZE);

    for (Integer i = 0; i != BLOCK_SIZE; ++i) {
        samples_vector.push_back(samples[i]);
    }

    Math::compute_statistics(samples_vector, stats);

    assert_statistics(
        true,
        expected_min,
        expected_median,
        expected_max,
        expected_mean,
        expected_standard_deviation,
        stats,
        0.1
    );
}


TEST(noise_generator_adds_white_noise_to_its_input, {
    Sample input_channel_1[BLOCK_SIZE];
    Sample input_channel_2[BLOCK_SIZE];
    Sample* input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel_1, input_channel_2
    };
    FixedSignalProducer input(input_channels);
    Math::RNG rng(123);
    FloatParamB level("L", 0.0, 1.0, 0.5);
    NoiseGenerator<FixedSignalProducer> noise_generator(
        input, level, 0.001, 22050.0, rng
    );
    Sample const* const* rendered = NULL;
    std::vector<Number> samples;

    level.set_sample_rate(22050.0);
    level.set_block_size(BLOCK_SIZE);

    noise_generator.set_sample_rate(22050.0);
    noise_generator.set_block_size(BLOCK_SIZE);

    for (Integer i = 0; i != BLOCK_SIZE; ++i) {
        input_channel_1[i] = 0.1;
        input_channel_2[i] = -0.2;
    }

    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 1, BLOCK_SIZE
    );

    assert_channel_statsistics(
        rendered[0],
        0.1 - 0.5,
        0.1,
        0.1 + 0.5,
        0.1,
        std::pow(1.0 / 12.0, 0.5)
    );
    assert_channel_statsistics(
        rendered[1],
        -0.2 - 0.5,
        -0.2,
        -0.2 + 0.5,
        -0.2,
        std::pow(1.0 / 12.0, 0.5)
    );
})
