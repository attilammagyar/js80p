/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025, 2026  Attila M. Magyar
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
constexpr Frequency SAMPLE_RATE = 22050.0;


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
    Sample* const input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel_1, input_channel_2
    };
    FixedSignalProducer input(input_channels);
    Math::RNG rng(123);
    FloatParamB level("L", 0.0, 1.0, 0.5);
    NoiseGenerator<FixedSignalProducer> noise_generator(
        input, level, 0.001, SAMPLE_RATE, rng
    );
    Sample const* const* rendered = NULL;

    level.set_sample_rate(SAMPLE_RATE);
    level.set_block_size(BLOCK_SIZE);

    noise_generator.set_sample_rate(SAMPLE_RATE);
    noise_generator.set_block_size(BLOCK_SIZE);

    noise_generator.start(0.0);

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


TEST(noise_level_may_be_sample_evaluated, {
    Sample input_channel_1[BLOCK_SIZE];
    Sample input_channel_2[BLOCK_SIZE];
    Sample* const input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel_1, input_channel_2
    };
    FixedSignalProducer input(input_channels);
    Math::RNG rng(123);
    FloatParamS level("L", 0.0, 1.0, 0.75);
    LFO lfo("LFO");
    NoiseGenerator<FixedSignalProducer, FloatParamS> noise_generator(
        input, level, 0.001, SAMPLE_RATE, rng
    );
    Sample const* const* rendered = NULL;

    level.set_sample_rate(SAMPLE_RATE);
    level.set_block_size(BLOCK_SIZE);

    lfo.set_sample_rate(SAMPLE_RATE);
    lfo.set_block_size(BLOCK_SIZE);

    level.set_lfo(&lfo);

    lfo.min.set_value(0.5);
    lfo.max.set_value(0.5);

    noise_generator.set_sample_rate(SAMPLE_RATE);
    noise_generator.set_block_size(BLOCK_SIZE);

    noise_generator.start(0.0);

    for (Integer i = 0; i != BLOCK_SIZE; ++i) {
        input_channel_1[i] = 0.1;
        input_channel_2[i] = -0.2;
    }

    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer, FloatParamS> >(
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


TEST(when_noise_generator_is_off_then_input_passes_through_it_unmodified, {
    constexpr Integer block_size = 10;
    constexpr Frequency sample_rate = 10.0;

    Sample input_channel_1[block_size];
    Sample input_channel_2[block_size];
    Sample* const input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel_1, input_channel_2
    };
    FixedSignalProducer input(input_channels);
    Math::RNG rng(123);
    FloatParamB level("L", 0.0, 1.0, 0.5);
    NoiseGenerator<FixedSignalProducer> noise_generator(
        input, level, 0.001, sample_rate, rng
    );
    Sample const* const* rendered = NULL;

    level.set_sample_rate(sample_rate);
    level.set_block_size(block_size);

    noise_generator.set_sample_rate(sample_rate);
    noise_generator.set_block_size(block_size);

    for (Integer i = 0; i != block_size; ++i) {
        input_channel_1[i] = 0.1;
        input_channel_2[i] = -0.2;
    }

    assert_false(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 1, block_size
    );
    assert_false(noise_generator.is_on());

    assert_eq((void const*)input_channel_1, (void const*)rendered[0], "round=1");
    assert_eq((void const*)input_channel_2, (void const*)rendered[1], "round=1");

    noise_generator.start(1.29);
    noise_generator.stop(1.69);
    noise_generator.cancel_events_at(1.60);
    noise_generator.stop(3.69);

    assert_false(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 2, block_size
    );
    assert_false(noise_generator.is_on());

    assert_eq((void const*)input_channel_1, (void const*)rendered[0], "round=2");
    assert_eq((void const*)input_channel_2, (void const*)rendered[1], "round=2");

    assert_false(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 3, block_size
    );
    assert_true(noise_generator.is_on());

    assert_eq(&input_channel_1[0], &rendered[0][0], 3, DOUBLE_DELTA, "round=3");
    assert_neq(&input_channel_1[3], &rendered[0][3], 7, DOUBLE_DELTA, "round=3");

    assert_eq(&input_channel_2[0], &rendered[1][0], 3, DOUBLE_DELTA, "round=3");
    assert_neq(&input_channel_2[3], &rendered[1][3], 7, DOUBLE_DELTA, "round=3");

    assert_true(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 4, block_size
    );
    assert_true(noise_generator.is_on());

    assert_neq(input_channel_1, rendered[0], block_size, DOUBLE_DELTA, "round=4");
    assert_neq(input_channel_2, rendered[1], block_size, DOUBLE_DELTA, "round=4");

    assert_true(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 5, block_size
    );
    assert_false(noise_generator.is_on());

    assert_neq(&input_channel_1[0], &rendered[0][0], 7, DOUBLE_DELTA, "round=5");
    assert_eq(&input_channel_1[7], &rendered[0][7], 3, DOUBLE_DELTA, "round=5");

    assert_neq(&input_channel_2[0], &rendered[1][0], 7, DOUBLE_DELTA, "round=5");
    assert_eq(&input_channel_2[7], &rendered[1][7], 3, DOUBLE_DELTA, "round=5");
})


TEST(when_noise_level_is_zero_then_handles_events_and_returns_the_input_unmodified, {
    constexpr Integer block_size = 10;
    constexpr Frequency sample_rate = 10.0;

    Sample input_channel_1[block_size];
    Sample input_channel_2[block_size];
    Sample* const input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel_1, input_channel_2
    };
    FixedSignalProducer input(input_channels);
    Math::RNG rng(123);
    FloatParamB level("L", 0.0, 1.0, 0.0);
    NoiseGenerator<FixedSignalProducer> noise_generator(
        input, level, 0.001, sample_rate, rng
    );
    Sample const* const* rendered = NULL;

    level.set_sample_rate(sample_rate);
    level.set_block_size(block_size);

    noise_generator.set_sample_rate(sample_rate);
    noise_generator.set_block_size(block_size);

    for (Integer i = 0; i != block_size; ++i) {
        input_channel_1[i] = 0.1;
        input_channel_2[i] = -0.2;
    }

    assert_false(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 1, block_size
    );
    assert_false(noise_generator.is_on());

    assert_eq((void const*)input_channel_1, (void const*)rendered[0], "round=1");
    assert_eq((void const*)input_channel_2, (void const*)rendered[1], "round=1");

    noise_generator.start(1.29);
    noise_generator.stop(1.69);
    noise_generator.cancel_events_at(1.60);
    noise_generator.stop(3.69);

    assert_false(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 2, block_size
    );
    assert_false(noise_generator.is_on());

    assert_eq((void const*)input_channel_1, (void const*)rendered[0], "round=2");
    assert_eq((void const*)input_channel_2, (void const*)rendered[1], "round=2");

    assert_false(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 3, block_size
    );
    assert_true(noise_generator.is_on());

    assert_eq(input_channel_1, rendered[0], block_size, DOUBLE_DELTA, "round=3");
    assert_eq(input_channel_2, rendered[1], block_size, DOUBLE_DELTA, "round=3");

    assert_true(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 4, block_size
    );
    assert_true(noise_generator.is_on());

    assert_eq((void const*)input_channel_1, (void const*)rendered[0], "round=4");
    assert_eq((void const*)input_channel_2, (void const*)rendered[1], "round=4");

    assert_true(noise_generator.is_on());
    rendered = SignalProducer::produce< NoiseGenerator<FixedSignalProducer> >(
        noise_generator, 5, block_size
    );
    assert_false(noise_generator.is_on());

    assert_eq(input_channel_1, rendered[0], block_size, DOUBLE_DELTA, "round=5");
    assert_eq(input_channel_2, rendered[1], block_size, DOUBLE_DELTA, "round=5");
})
