/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/distortion.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


typedef Distortion::Distortion<SumOfSines> Distortion_;


constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 1024;
constexpr Number BLOCK_LENGTH = (Number)BLOCK_SIZE / SAMPLE_RATE;
constexpr Integer ROUNDS = 20;
constexpr Integer SAMPLE_COUNT = BLOCK_SIZE * ROUNDS;


TEST(while_distortion_level_is_close_to_zero_the_original_signal_is_barely_affected, {
    SumOfSines input(1.0, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    Distortion::TypeParam type("T", Distortion::TYPE_TANH_10);
    Distortion_ distortion("D", type, input);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    type.set_block_size(BLOCK_SIZE);
    distortion.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    type.set_sample_rate(SAMPLE_RATE);
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
    Distortion::TypeParam type("T", Distortion::TYPE_TANH_10);
    Distortion_ distortion("D", type, input);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    type.set_block_size(BLOCK_SIZE);
    distortion.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    type.set_sample_rate(SAMPLE_RATE);
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
            "channel=%d, original_signal_level=%f",
            (int)c,
            original_signal_level
        );
    }
}


TEST(when_distortion_level_is_high_then_the_signal_is_distorted, {
    test_distortion(1.0);
    test_distortion(3.0);
    test_distortion(10.0);
})


TEST(when_input_is_silent_then_distortion_is_no_op, {
    SumOfSines input(1e-9, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    Distortion::TypeParam type("T", Distortion::TYPE_TANH_10);
    Distortion_ distortion("D", type, input);
    Sample const* const* input_buffer;
    Sample const* const* distorted_buffer;

    type.set_block_size(BLOCK_SIZE);
    distortion.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    type.set_sample_rate(SAMPLE_RATE);
    distortion.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);

    distortion.level.set_value(1.0);

    input_buffer = SignalProducer::produce<SumOfSines>(input, 1, BLOCK_SIZE);
    distorted_buffer = SignalProducer::produce<Distortion_>(distortion, 1, BLOCK_SIZE);

    assert_eq(input_buffer, distorted_buffer);
})


TEST(delay_feedback_distortion_will_eventually_decay_completely, {
    constexpr Integer block_size = 2048;
    constexpr Integer channels = 1;
    Sample zeros[block_size];
    Sample channel[block_size];
    Sample const* const buffer[channels] = {channel};
    FixedSignalProducer input(buffer, channels);
    Distortion::TypeParam type("T", Distortion::TYPE_DELAY_FEEDBACK);
    Distortion::Distortion<FixedSignalProducer> distortion("D", type, input);
    distortion.level.set_value(1.0);
    Sample const* const* rendered = NULL;
    Sample peak;
    Sample previous_peak = 999.0;
    Integer peak_index;

    std::fill_n(zeros, block_size, 0.0);

    type.set_block_size(block_size);
    distortion.set_block_size(block_size);

    for (Integer i = 0; i != block_size; ++i) {
        constexpr Sample amplitude = Distortion::Tables::INPUT_MAX + 2.0;

        Number const x = (Number)i / (Number)block_size;

        channel[i] = amplitude * std::sin(Math::PI_DOUBLE * x);
    }

    for (Integer round = 0; round != 1000; ++round) {
        rendered = SignalProducer::produce< Distortion::Distortion<FixedSignalProducer> >(
            distortion, round
        );

        SignalProducer::find_peak(rendered, channels, block_size, peak, peak_index);

        if (round < 50) {
            assert_lt(peak, previous_peak, "round=%d", (int)round);
            assert_gt(peak, 0.0, "round=%d", (int)round);
        } else {
            assert_true(
                peak == 0.0 || peak < previous_peak,
                "round=%d,\n             peak=%.20e,\n    previous_peak=%.20e",
                (int)round,
                peak,
                previous_peak
            );
        }

        previous_peak = peak;

        for (Integer i = 0; i != block_size; ++i) {
            channel[i] = rendered[0][i] * Constants::DELAY_FEEDBACK_MAX;
        }
    }

    assert_eq(rendered[0], zeros, block_size, 0.0);
    assert_eq(0.0, peak, 0.0);
})


void assert_distortion_type_switching_is_smooth(
        Byte const type_1,
        Byte const type_2
) {
    SumOfSines input(0.70, 220.0, 0.0, 0.0, 0.0, 0.0, 1);
    Distortion::TypeParam type("T", type_1);
    Distortion_ distortion("D", type, input);
    Sample limit[BLOCK_SIZE];
    Sample const* const* output;

    type.set_block_size(BLOCK_SIZE);
    distortion.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    type.set_sample_rate(SAMPLE_RATE);
    distortion.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);

    distortion.level.set_value(1.0);

    SignalProducer::produce<Distortion_>(distortion, 1,BLOCK_SIZE);
    type.set_value(type_2);
    output = SignalProducer::produce<Distortion_>(distortion, 2, BLOCK_SIZE);

    std::fill_n(limit, BLOCK_SIZE, 1.0);
    assert_gte(
        limit, output[0], BLOCK_SIZE, "type_1=%hhu, type_2=%hhu", type_1, type_2
    );

    std::fill_n(limit, BLOCK_SIZE, -1.0);
    assert_lte(
        limit, output[0], BLOCK_SIZE, "type_1=%hhu, type_2=%hhu", type_1, type_2
    );
}


TEST(switching_distortion_types_is_smooth, {
    assert_distortion_type_switching_is_smooth(
        Distortion::TYPE_HARMONIC_13, Distortion::TYPE_HARMONIC_15
    );
    assert_distortion_type_switching_is_smooth(
        Distortion::TYPE_HARMONIC_15, Distortion::TYPE_HARMONIC_13
    );
    assert_distortion_type_switching_is_smooth(
        Distortion::TYPE_BIT_CRUSH_1, Distortion::TYPE_TANH_10
    );
    assert_distortion_type_switching_is_smooth(
        Distortion::TYPE_TANH_10, Distortion::TYPE_BIT_CRUSH_1
    );
    assert_distortion_type_switching_is_smooth(
        Distortion::TYPE_HARMONIC_135, Distortion::TYPE_HARMONIC_SQR
    );
    assert_distortion_type_switching_is_smooth(
        Distortion::TYPE_HARMONIC_SQR, Distortion::TYPE_HARMONIC_135
    );
})
