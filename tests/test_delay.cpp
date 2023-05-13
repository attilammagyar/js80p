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
        {0.0, 0.0, 0.0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.3, 0.1, 0.1, 0.1, 0.1},
        {0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.6, 0.2, 0.2, 0.2, 0.2},
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
        {0.00, 0.00, 0.05, 0.10, 0.15, 0.06, 0.12, 0.36, 0.12, 0.24, 0.36, 0.12},
        {0.00, 0.00, 0.10, 0.20, 0.30, 0.12, 0.24, 0.72, 0.24, 0.48, 0.72, 0.24},
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
    delay.gain.schedule_value(0.7, 1.0);

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


TEST(feedback_signal_merging_is_independent_of_rendered_sample_count, {
    constexpr Integer block_size = 5;
    constexpr Integer sample_count = 15;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.99, 0.99},
        {0.20, 0.40, 0.60, 0.99, 0.99},
    };
    constexpr Sample feedback_samples[CHANNELS][block_size] = {
        {0.01, 0.02, 0.03, 0.099, 0.099},
        {0.02, 0.04, 0.06, 0.099, 0.099},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {0.00, 0.00, 0.10, 0.20, 0.30, 0.11, 0.12, 0.23, 0.31, 0.11, 0.22, 0.13, 0.11, 0.22, 0.11},
        {0.00, 0.00, 0.20, 0.40, 0.60, 0.22, 0.24, 0.46, 0.62, 0.22, 0.44, 0.26, 0.22, 0.44, 0.22},
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
    Buffer output(sample_count, 2);
    Delay<FixedSignalProducer> delay(input);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    feedback.set_sample_rate(sample_rate);
    feedback.set_block_size(block_size);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_feedback_signal_producer(&feedback);
    delay.gain.set_value(1.0);
    delay.time.set_value(0.2);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 1, 3), 3);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 1, 3);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 2, 1), 1);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 2, 1);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 3, 3), 3);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 3, 3);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 4, 2), 2);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 4, 2);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 5, 1), 1);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 5, 1);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 6, 2), 2);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 6, 2);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 7, 3), 3);
    SignalProducer::produce<FixedSignalProducer>(&feedback, 7, 3);

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


TEST(reset_clears_the_delay_buffer, {
    constexpr Integer block_size = 5;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };
    constexpr Sample expected_output[CHANNELS][block_size] = {
        {0.0, 0.0, 0.10, 0.20, 0.30},
        {0.0, 0.0, 0.20, 0.40, 0.60},
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Delay<FixedSignalProducer> delay(input);
    Sample const* const* rendered_samples;

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_feedback_signal_producer(&delay);
    delay.gain.set_value(1.0);
    delay.time.set_value(0.2);

    SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 1);
    SignalProducer::produce< Delay<FixedSignalProducer> >(&delay, 2);
    delay.reset();
    rendered_samples = SignalProducer::produce< Delay<FixedSignalProducer> >(
        &delay, 3
    );

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            rendered_samples[c],
            block_size,
            0.001,
            "channel=%d",
            (int)c
        );
    }
})
