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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "synth/distortion.cpp"
#include "synth/envelope.cpp"
#include "synth/filter.cpp"
#include "synth/math.cpp"
#include "synth/midi_controller.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"


using namespace JS80P;


typedef Distortion<SumOfSines> Distortion_;


constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 1024;
constexpr Number BLOCK_LENGTH = (Number)BLOCK_SIZE / SAMPLE_RATE;
constexpr Integer ROUNDS = 20;
constexpr Integer SAMPLE_COUNT = BLOCK_SIZE * ROUNDS;


TEST(while_distortion_level_is_close_to_zero_the_original_signal_is_barely_affected, {
    SumOfSines input(1.0, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    Distortion_ distortion("D", 10.0, input);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    distortion.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    distortion.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);

    distortion.level.set_value(0.0);
    distortion.level.schedule_value(BLOCK_LENGTH * 2.5, 0.0);
    distortion.level.schedule_linear_ramp(3.0 * BLOCK_LENGTH, 0.01);

    render_rounds<SumOfSines>(input, expected_output, ROUNDS);
    input.reset();
    render_rounds<Distortion_>(distortion, actual_output, ROUNDS);

    assert_eq(0.01, distortion.level.get_value(), DOUBLE_DELTA);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            SAMPLE_COUNT,
            0.05,
            "channel=%d",
            (int)c
        );
    }
})


void naive_distort(Number const level, Buffer &buffer)
{
    for (Integer c = 0; c != CHANNELS; ++c) {
        for (Integer i = 0; i != SAMPLE_COUNT; ++i) {
            buffer.samples[c][i] *= level;
            buffer.samples[c][i] = std::min(
                1.0, std::max(-1.0, buffer.samples[c][i])
            );
        }
    }
}


void test_distortion(Number const original_signal_level)
{
    SumOfSines input(original_signal_level, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    Distortion_ distortion("D", 10.0, input);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    distortion.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    distortion.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);

    distortion.level.set_value(0.99);
    distortion.level.schedule_value(BLOCK_LENGTH * 2.5, 0.99);
    distortion.level.schedule_linear_ramp(3.0 * BLOCK_LENGTH, 1.0);

    render_rounds<SumOfSines>(input, expected_output, ROUNDS);
    input.reset();
    render_rounds<Distortion_>(distortion, actual_output, ROUNDS);

    naive_distort(10.0, expected_output);

    assert_eq(1.0, distortion.level.get_value(), DOUBLE_DELTA);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            SAMPLE_COUNT,
            0.06,
            "channel=%d",
            (int)c
        );
    }
}


TEST(when_distortion_level_is_high_then_the_signal_is_distorted, {
    test_distortion(1.0);
    test_distortion(3.0);
    test_distortion(10.0);
})
