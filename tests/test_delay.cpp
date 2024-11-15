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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/biquad_filter.cpp"
#include "dsp/delay.cpp"
#include "dsp/distortion.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/gain.cpp"
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


void test_basic_delay(
        Number const time_scale,
        Number const bpm,
        Byte const tempo_sync_state
) {
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
    ToggleParam tempo_sync("SYN", tempo_sync_state);
    Delay<FixedSignalProducer> delay(input, &tempo_sync);

    tempo_sync.set_sample_rate(sample_rate);
    tempo_sync.set_block_size(block_size);
    tempo_sync.set_bpm(bpm);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);
    input.set_bpm(bpm);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_bpm(bpm);
    delay.gain.set_value(0.5);
    delay.time.set_value(0.25 * time_scale);
    delay.time.schedule_value(0.71, 0.4 * time_scale);

    render_rounds< Delay<FixedSignalProducer> >(delay, output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "unexpected delay; channel=%d, bpm=%f, tempo_sync=%s",
            (int)c,
            bpm,
            tempo_sync.get_value() == ToggleParam::ON ? "ON" : "OFF"
        );
    }

    assert_eq(0.4 * time_scale, delay.time.get_value(), DOUBLE_DELTA);
}


TEST(repeats_input_samples_with_delay, {
    test_basic_delay(1.0, 120.0, ToggleParam::OFF);
    test_basic_delay(2.0, 120.0, ToggleParam::ON);
})


void test_delay_with_time_scale_param(
        Number const time_scale,
        Number const bpm,
        Byte const tempo_sync_state
) {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 5;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {
            0.000, 0.000, 0.025, 0.075, 0.125,
            0.175, 0.225, 0.150, 0.075, 0.125,
            0.175, 0.225, 0.150, 0.075, 0.125,
            0.175, 0.225, 0.150, 0.075, 0.125,
            0.175, 0.225, 0.150, 0.075, 0.125,
        },
        {
            0.000, 0.000, 0.050, 0.150, 0.250,
            0.350, 0.450, 0.300, 0.150, 0.250,
            0.350, 0.450, 0.300, 0.150, 0.250,
            0.350, 0.450, 0.300, 0.150, 0.250,
            0.350, 0.450, 0.300, 0.150, 0.250,
        },
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, CHANNELS);
    ToggleParam tempo_sync("SYN", tempo_sync_state);
    FloatParamS delay_time_scale("SCL", 0.0, 2.0, 2.0);
    Delay<FixedSignalProducer, DelayCapabilities::DC_SCALABLE> delay(
        input, &tempo_sync
    );

    delay.set_time_scale_param(delay_time_scale);

    delay_time_scale.set_sample_rate(sample_rate);
    delay_time_scale.set_block_size(block_size);
    delay_time_scale.set_bpm(bpm);

    tempo_sync.set_sample_rate(sample_rate);
    tempo_sync.set_block_size(block_size);
    tempo_sync.set_bpm(bpm);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);
    input.set_bpm(bpm);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_bpm(bpm);

    delay.gain.set_value(0.5);
    delay.time.set_value(0.125 * time_scale);
    delay.time.schedule_value(0.2, 0.5 * time_scale);
    delay.time.schedule_value(1.4, 0.499999 * time_scale);

    delay_time_scale.schedule_value(0.2, 0.5);
    delay_time_scale.schedule_value(0.8, 0.500001);

    render_rounds< Delay<FixedSignalProducer, DelayCapabilities::DC_SCALABLE> >(
        delay, output, rounds
    );

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "unexpected delay; channel=%d, bpm=%f, tempo_sync=%s",
            (int)c,
            bpm,
            tempo_sync.get_value() == ToggleParam::ON ? "ON" : "OFF"
        );
    }
}


TEST(delay_time_may_be_scaled_by_float_param, {
    test_delay_with_time_scale_param(1.0, 120.0, ToggleParam::OFF);
    test_delay_with_time_scale_param(2.0, 120.0, ToggleParam::ON);
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


void test_delay_with_feedback(
        Number const time_scale,
        Number const bpm,
        Byte const tempo_sync_state
) {
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
    ToggleParam tempo_sync("SYN", tempo_sync_state);
    Delay<FixedSignalProducer> delay(input, &tempo_sync);

    tempo_sync.set_sample_rate(sample_rate);
    tempo_sync.set_block_size(block_size);
    tempo_sync.set_bpm(bpm);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);
    input.set_bpm(bpm);

    feedback.set_sample_rate(sample_rate);
    feedback.set_block_size(block_size);
    feedback.set_bpm(bpm);

    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_bpm(bpm);
    delay.set_feedback_signal_producer(feedback);
    delay.gain.set_value(0.5);
    delay.time.set_value(0.2 * time_scale);
    delay.gain.schedule_value(0.7, 1.0);

    SignalProducer::produce<FixedSignalProducer>(feedback, 12345);

    render_rounds< Delay<FixedSignalProducer> >(delay, output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "unexpected delay with feedback; channel=%d, bpm=%f, tempo_sync=%s",
            (int)c,
            bpm,
            tempo_sync.get_value() == ToggleParam::ON ? "ON" : "OFF"
        );
    }

    assert_eq(1.0, delay.gain.get_value(), DOUBLE_DELTA);
}


TEST(feedback_signal_is_merged_into_the_delay_buffer, {
    test_delay_with_feedback(1.0, 120.0, ToggleParam::OFF);
    test_delay_with_feedback(2.0, 120.0, ToggleParam::ON);
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
    delay.set_feedback_signal_producer(feedback);
    delay.gain.set_value(1.0);
    delay.time.set_value(0.2);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 1, 3), 3);
    SignalProducer::produce<FixedSignalProducer>(feedback, 1, 3);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 2, 1), 1);
    SignalProducer::produce<FixedSignalProducer>(feedback, 2, 1);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 3, 3), 3);
    SignalProducer::produce<FixedSignalProducer>(feedback, 3, 3);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 4, 2), 2);
    SignalProducer::produce<FixedSignalProducer>(feedback, 4, 2);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 5, 1), 1);
    SignalProducer::produce<FixedSignalProducer>(feedback, 5, 1);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 6, 2), 2);
    SignalProducer::produce<FixedSignalProducer>(feedback, 6, 2);

    output.append(SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 7, 3), 3);
    SignalProducer::produce<FixedSignalProducer>(feedback, 7, 3);

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
    delay.set_feedback_signal_producer(delay);
    delay.gain.set_value(1.0);
    delay.time.set_value(0.2);

    SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 1);
    SignalProducer::produce< Delay<FixedSignalProducer> >(delay, 2);
    delay.reset();
    rendered_samples = SignalProducer::produce< Delay<FixedSignalProducer> >(
        delay, 3
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


TEST(when_tempo_sync_is_on_then_delay_time_is_measured_in_beats_instead_of_seconds, {
    test_basic_delay(1.0, 120.0, ToggleParam::OFF);
    test_delay_with_feedback(1.0, 180.0, ToggleParam::OFF);
    test_delay_with_feedback(1.0, 30.0, ToggleParam::OFF);

    test_basic_delay(2.0, 120.0, ToggleParam::ON);
    test_delay_with_feedback(3.0, 180.0, ToggleParam::ON);
    test_delay_with_feedback(0.5, 30.0, ToggleParam::ON);
})


TEST(when_tempo_sync_is_on_but_tempo_is_too_slow_then_the_minimum_tempo_is_used, {
    constexpr Number time_scale = (
        Delay<FixedSignalProducer>::BPM_MIN / Math::SECONDS_IN_ONE_MINUTE
    );

    test_basic_delay(1.0, 0.1, ToggleParam::OFF);
    test_delay_with_feedback(1.0, 0.1, ToggleParam::OFF);

    test_basic_delay(time_scale, 0.1, ToggleParam::ON);
    test_delay_with_feedback(time_scale, 0.1, ToggleParam::ON);
})


TEST(identical_delays_may_share_delay_buffer, {
    constexpr Integer block_size = 3;
    constexpr Integer rounds = 2;
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
    constexpr Sample expected_output_1[CHANNELS][sample_count] = {
        {0.00, 0.00, 0.025, 0.05, 0.075, 0.03},
        {0.00, 0.00, 0.050, 0.10, 0.150, 0.06},
    };
    constexpr Sample expected_output_2[CHANNELS][sample_count] = {
        {0.00, 0.00, 0.05, 0.10, 0.15, 0.06},
        {0.00, 0.00, 0.10, 0.20, 0.30, 0.12},
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
    Buffer output_1(sample_count, CHANNELS);
    Buffer output_2(sample_count, CHANNELS);
    ToggleParam tempo_sync("SYN", ToggleParam::OFF);
    Delay<FixedSignalProducer> delay_1(input, &tempo_sync);
    Delay<FixedSignalProducer> delay_2(input, &tempo_sync);

    tempo_sync.set_sample_rate(sample_rate);
    tempo_sync.set_block_size(block_size);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    feedback.set_sample_rate(sample_rate);
    feedback.set_block_size(block_size);

    delay_2.use_shared_delay_buffer(delay_1);

    delay_1.set_sample_rate(sample_rate);
    delay_1.set_block_size(block_size);
    delay_1.set_feedback_signal_producer(feedback);
    delay_1.gain.set_value(0.25);
    delay_1.time.set_value(0.2);
    delay_1.gain.schedule_value(0.7, 0.5);

    delay_2.set_sample_rate(sample_rate);
    delay_2.set_block_size(block_size);
    delay_2.gain.set_value(0.5);
    delay_2.time.set_value(0.2);
    delay_2.gain.schedule_value(0.7, 1.0);

    delay_2.reset();

    SignalProducer::produce<FixedSignalProducer>(feedback, 12345);

    render_rounds< Delay<FixedSignalProducer> >(delay_1, output_1, rounds);
    render_rounds< Delay<FixedSignalProducer> >(delay_2, output_2, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output_1[c],
            output_1.samples[c],
            sample_count,
            0.001,
            "unexpected delay with feedback; channel=%d",
            (int)c
        );
        assert_eq(
            expected_output_2[c],
            output_2.samples[c],
            sample_count,
            0.001,
            "unexpected delay with feedback; channel=%d",
            (int)c
        );
    }
})


TEST(delay_may_be_reversed, {
    constexpr Number bpm = 120.0;
    constexpr Number gain = 0.5;
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 3;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Number envelope = (
        Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE>::TEST_REVERSE_ENVELOPE
    );

    /*
    Let S denote the index of the last written sample within the delay buffer
    when we start observing a normal forward delay. If T denotes the delay time
    expressed as sample count (let's assume it's constant), then the I-th
    output sample of the delay will be the (S+I-T)-th sample from the delay
    buffer. During a period of T samples (let's truncate it to integers and
    let's assume that it's constant), a delay and a reversed delay should
    produce the following samples:

        Index   Output sample (forward)     Output sample (reversed)
        0       B[S + 0 - T]                B[S + T - T]
        1       B[S + 1 - T]                B[S + (T - 1) - T]
        2       B[S + 2 - T]                B[S + (T - 2) - T]
        ...     ...                         ...
        T - 2   B[S + (T - 2) - T]          B[S + 2 - T]
        T - 1   B[S + (T - 1) - T]          B[S + 1 - T]
        T       B[S + T - T]                B[S + 0 - T]

    Once the reversed delay hits the (S + 0 - T)-th sample, it should restart
    from S.

    If the delay time (expressed in samples) is not an integer, then the
    repeated section is supposed to be longer by a fraction of a sample. Let T
    denote the integer part of the delay time (measured in samples), and let
    the fraction be denoted by F. When we notice that we went further back in
    the buffer than (T + F), e.g. by D samples (D will equal to (1.0 - F) on the
    first restart), then the next output sample should come exactly from D
    samples before S:

        Buffer:       2      1      0
               |---+--|------|------|------|------|------|------|------|------|
                  -F -T             S
        Output:                     0      1      2  0      1      2  0      1
    */
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {
            envelope * gain * 0.1,
            envelope * gain * 0.0,
            envelope * gain * 0.0,
            envelope * gain * 0.0,
            envelope * gain * (0.7 * 0.1 + 0.3 * 0.0),

            envelope * gain * 0.0,
            envelope * gain * 0.0,
            envelope * gain * (0.4 * 0.1 + 0.6 * 0.5),
            envelope * gain * (0.4 * 0.5 + 0.6 * 0.4),
            envelope * gain * (0.4 * 0.4 + 0.6 * 0.3),

            envelope * gain * (0.1 * 0.1 + 0.9 * 0.5),
            envelope * gain * (0.1 * 0.5 + 0.9 * 0.4),
            envelope * gain * (0.1 * 0.4 + 0.9 * 0.3),
            envelope * gain * (0.1 * 0.3 + 0.9 * 0.2),
            envelope * gain * (0.8 * 0.1 + 0.2 * 0.5),
        },
        {
            envelope * gain * 0.2,
            envelope * gain * 0.0,
            envelope * gain * 0.0,
            envelope * gain * 0.0,
            envelope * gain * (0.7 * 0.2 + 0.3 * 0.0),

            envelope * gain * 0.0,
            envelope * gain * 0.0,
            envelope * gain * (0.4 * 0.2 + 0.6 * 1.0),
            envelope * gain * (0.4 * 1.0 + 0.6 * 0.8),
            envelope * gain * (0.4 * 0.8 + 0.6 * 0.6),

            envelope * gain * (0.1 * 0.2 + 0.9 * 1.0),
            envelope * gain * (0.1 * 1.0 + 0.9 * 0.8),
            envelope * gain * (0.1 * 0.8 + 0.9 * 0.6),
            envelope * gain * (0.1 * 0.6 + 0.9 * 0.4),
            envelope * gain * (0.8 * 0.2 + 0.2 * 1.0),
        }
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, CHANNELS);
    ToggleParam tempo_sync("SYN", ToggleParam::ON);
    ToggleParam reverse("REV", ToggleParam::ON);
    Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE> delay(
        input, &tempo_sync
    );

    delay.begin_reverse_delay_test();

    tempo_sync.set_sample_rate(sample_rate);
    tempo_sync.set_block_size(block_size);
    tempo_sync.set_bpm(bpm);

    reverse.set_sample_rate(sample_rate);
    reverse.set_block_size(block_size);
    reverse.set_bpm(bpm);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);
    input.set_bpm(bpm);

    delay.set_reverse_toggle_param(reverse);
    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_bpm(bpm);
    delay.gain.set_value(gain);
    delay.time.set_value(0.66);
    delay.time.schedule_value(0.71, 0.65999);

    render_rounds< Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE> >(
        delay, output, rounds
    );

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "unexpected delay; channel=%d, bpm=%f, tempo_sync=%s",
            (int)c,
            bpm,
            tempo_sync.get_value() == ToggleParam::ON ? "ON" : "OFF"
        );
    }

    delay.end_reverse_delay_test();
})


TEST(reverse_delay_time_change_affects_the_speed_of_the_remaining_part_of_the_delay_segment, {
    constexpr Number bpm = 120.0;
    constexpr Integer block_size = 10;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Number envelope = (
        Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE>::TEST_REVERSE_ENVELOPE
    );
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
        {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {
            envelope * 0.00,
            envelope * 1.00,
            envelope * 0.90,
            envelope * 0.80,
            envelope * 0.70,
            envelope * 0.60,
            envelope * 0.50,
            envelope * 0.45,
            envelope * 0.40,
            envelope * 0.35,
        },
        {
            envelope * 0.00,
            envelope * 1.00,
            envelope * 0.90,
            envelope * 0.80,
            envelope * 0.70,
            envelope * 0.60,
            envelope * 0.50,
            envelope * 0.45,
            envelope * 0.40,
            envelope * 0.35,
        }
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, CHANNELS);
    ToggleParam tempo_sync("SYN", ToggleParam::OFF);
    ToggleParam reverse("REV", ToggleParam::ON);
    Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE> delay(
        input, &tempo_sync
    );

    delay.begin_reverse_delay_test();

    tempo_sync.set_sample_rate(sample_rate);
    tempo_sync.set_block_size(block_size);
    tempo_sync.set_bpm(bpm);

    reverse.set_sample_rate(sample_rate);
    reverse.set_block_size(block_size);
    reverse.set_bpm(bpm);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);
    input.set_bpm(bpm);

    delay.set_reverse_toggle_param(reverse);
    delay.set_sample_rate(sample_rate);
    delay.set_block_size(block_size);
    delay.set_bpm(bpm);
    delay.gain.set_value(1.0);
    delay.time.set_value(1.0);

    SignalProducer::produce< Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE> >(
        delay, 123
    );

    delay.time.schedule_value(0.51, 2.0);
    render_rounds< Delay<FixedSignalProducer, DelayCapabilities::DC_REVERSIBLE> >(
        delay, output, rounds
    );

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.001,
            "unexpected delay; channel=%d, bpm=%f, tempo_sync=%s",
            (int)c,
            bpm,
            tempo_sync.get_value() == ToggleParam::ON ? "ON" : "OFF"
        );
    }

    delay.end_reverse_delay_test();
})


template<class PannedDelayClass>
void test_panned_delay(
        PannedDelayClass& panned_delay,
        FixedSignalProducer& input,
        char const* class_name,
        Integer const block_size,
        Integer const rounds,
        Number const panning_scale,
        Sample const* const* input_buffer,
        Sample const* const* expected_output
) {
    constexpr Frequency sample_rate = 10.0;

    Integer const sample_count = rounds * block_size;

    Buffer output(sample_count, FixedSignalProducer::CHANNELS);

    input.set_fixed_samples(input_buffer);
    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    panned_delay.reset();

    panned_delay.set_sample_rate(sample_rate);
    panned_delay.set_block_size(block_size);
    panned_delay.delay.gain.set_value(0.75);
    panned_delay.delay.time.set_value(0.2);
    panned_delay.panning.set_value(0.0);
    panned_delay.panning.schedule_value(0.45, -1.0);
    panned_delay.set_panning_scale(panning_scale);

    assert_eq((int)input.get_channels(), (int)panned_delay.get_channels());

    render_rounds<PannedDelayClass>(panned_delay, output, rounds);

    for (Integer c = 0; c != FixedSignalProducer::CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            0.000015,
            "class=%s, panning_scale=%f, channel=%d",
            class_name,
            panning_scale,
            (int)c
        );
    }

    assert_eq(-1.0, panned_delay.panning.get_value(), DOUBLE_DELTA);
}


template<class PannedDelayClass>
void test_panned_delay(
        PannedDelayClass& panned_delay,
        FixedSignalProducer& input,
        char const* class_name
) {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 2;
    constexpr Integer sample_count = rounds * block_size;

    constexpr Sample input_samples[FixedSignalProducer::CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };

    constexpr Sample expected_output_full_panning_samples[FixedSignalProducer::CHANNELS][sample_count] = {
        {0.000, 0.000, 0.075, 0.150, 0.225, 0.000, 0.000, 0.000, 0.000, 0.000},
        {0.000, 0.000, 0.150, 0.300, 0.450, 0.900, 1.125, 0.225, 0.450, 0.675},
    };

    constexpr Sample expected_output_opposite_panning_samples[FixedSignalProducer::CHANNELS][sample_count] = {
        {0.000, 0.000, 0.075, 0.150, 0.225, 0.900, 1.125, 0.225, 0.450, 0.675},
        {0.000, 0.000, 0.150, 0.300, 0.450, 0.000, 0.000, 0.000, 0.000, 0.000},
    };

    constexpr Sample expected_output_no_panning_samples[FixedSignalProducer::CHANNELS][sample_count] = {
        {0.000, 0.000, 0.075, 0.150, 0.225, 0.300, 0.375, 0.075, 0.150, 0.225},
        {0.000, 0.000, 0.150, 0.300, 0.450, 0.600, 0.750, 0.150, 0.300, 0.450},
    };

    Sample const* input_buffer[FixedSignalProducer::CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };

    Sample const* expected_output_full_panning[] = {
        (Sample const*)&expected_output_full_panning_samples[0],
        (Sample const*)&expected_output_full_panning_samples[1],
    };

    Sample const* expected_output_opposite_panning[] = {
        (Sample const*)&expected_output_opposite_panning_samples[0],
        (Sample const*)&expected_output_opposite_panning_samples[1],
    };

    Sample const* expected_output_no_panning[] = {
        (Sample const*)&expected_output_no_panning_samples[0],
        (Sample const*)&expected_output_no_panning_samples[1],
    };

    test_panned_delay<PannedDelayClass>(
        panned_delay, input, class_name, block_size, rounds, 1.0, input_buffer, expected_output_full_panning
    );
    test_panned_delay<PannedDelayClass>(
        panned_delay, input, class_name, block_size, rounds, -1.0, input_buffer, expected_output_opposite_panning
    );
    test_panned_delay<PannedDelayClass>(
        panned_delay, input, class_name, block_size, rounds, 0.000001, input_buffer, expected_output_no_panning
    );
}


TEST(output_may_be_panned, {
    FloatParamS distortion("DST", 0.0, 1.0, 0.00001);
    FixedSignalProducer input(NULL);
    PannedDelay<FixedSignalProducer> panned_delay(
        input, PannedDelayStereoMode::FLIPPED
    );
    Distortion::TypeParam distortion_type("DSTTYP", Distortion::TYPE_DELAY_FEEDBACK);
    DistortedHighShelfPannedDelay<FixedSignalProducer> distorted_delay(
        input, PannedDelayStereoMode::FLIPPED, distortion, distortion_type
    );


    test_panned_delay< PannedDelay<FixedSignalProducer> >(
        panned_delay, input, "PannedDelay"
    );
    test_panned_delay< DistortedHighShelfPannedDelay<FixedSignalProducer> >(
        distorted_delay, input, "DistortedHighShelfPannedDelay"
    );
})
