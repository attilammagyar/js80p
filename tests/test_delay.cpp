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

#include "synth/biquad_filter.cpp"
#include "synth/delay.cpp"
#include "synth/envelope.cpp"
#include "synth/filter.cpp"
#include "synth/flexible_controller.cpp"
#include "synth/lfo.cpp"
#include "synth/math.cpp"
#include "synth/midi_controller.cpp"
#include "synth/oscillator.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"
#include "synth/wavetable.cpp"


using namespace JS80P;


constexpr Integer CHANNELS = 2;


TEST(when_delay_time_is_zero_then_copies_input_samples_unchanged, {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 2;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {0.10, 0.20, 0.30, 0.40, 0.50, 0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00, 0.20, 0.40, 0.60, 0.80, 1.00},
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, CHANNELS);
    Delay<FixedSignalProducer> delay(input);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.gain.set_value(1.0);
    delay.time.set_value(0.0);

    render_rounds< Delay<FixedSignalProducer> >(delay, output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "channel=%d",
            (int)c
        );
    }
})


TEST(repeats_input_samples_with_delay, {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 2;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {0.000, 0.000, 0.025, 0.075, 0.125, 0.175, 0.225, 0.150, 0.250, 0.050},
        {0.000, 0.000, 0.050, 0.150, 0.250, 0.350, 0.450, 0.300, 0.500, 0.100},
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, CHANNELS);
    Delay<FixedSignalProducer> delay(input);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.gain.set_value(0.5);
    delay.time.set_value(0.25);
    delay.time.schedule_value(0.71, 0.4);

    render_rounds< Delay<FixedSignalProducer> >(delay, output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "channel=%d",
            (int)c
        );
    }

    assert_eq(0.4, delay.time.get_value(), DOUBLE_DELTA);
})


TEST(block_size_may_be_larger_than_max_delay_time, {
    constexpr Integer block_size = 7;
    constexpr Integer rounds = 2;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 1.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.3},
        {0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.6},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {0.1, 0.1, 0.3, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.3, 0.1, 0.1, 0.1, 0.1},
        {0.2, 0.2, 0.6, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.6, 0.2, 0.2, 0.2, 0.2},
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, CHANNELS);
    Delay<FixedSignalProducer> delay(input);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.time.set_value(3.0);
    delay.gain.set_value(1.0);

    render_rounds< Delay<FixedSignalProducer> >(delay, output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "channel=%d",
            (int)c
        );
    }
})


TEST(feedback_signal_is_merged_into_the_delay_buffer, {
    constexpr Integer block_size = 3;
    constexpr Integer rounds = 4;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30},
        {0.20, 0.40, 0.60},
    };
    constexpr Sample feedback_samples[CHANNELS][block_size] = {
        {0.02, 0.04, 0.06},
        {0.04, 0.08, 0.12},
    };
    /* output = gain * (input + feedback) */
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {0.00, 0.00, 0.06, 0.12, 0.18, 0.12, 0.24, 0.36, 0.12, 0.24, 0.36, 0.12},
        {0.00, 0.00, 0.12, 0.24, 0.36, 0.24, 0.48, 0.72, 0.24, 0.48, 0.72, 0.24},
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    Sample const* feedback_buffer[CHANNELS] = {
        (Sample const*)&feedback_samples[0],
        (Sample const*)&feedback_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    FixedSignalProducer feedback(feedback_buffer);
    Buffer output(sample_count, CHANNELS);
    Delay<FixedSignalProducer> delay(input);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    feedback.set_sample_rate(sample_rate);
    feedback.set_block_size(block_size);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_feedback_signal_producer(&feedback);
    delay.gain.set_value(0.5);
    delay.time.set_value(0.2);
    delay.gain.schedule_value(0.5, 1.0);

    SignalProducer::produce<FixedSignalProducer>(&feedback, 12345);

    render_rounds< Delay<FixedSignalProducer> >(delay, output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "channel=%d",
            (int)c
        );
    }

    assert_eq(1.0, delay.gain.get_value(), DOUBLE_DELTA);
})
