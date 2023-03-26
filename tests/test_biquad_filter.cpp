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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "synth/biquad_filter.cpp"
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


constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 256;
constexpr Integer ROUNDS = 50;
constexpr Integer SAMPLE_COUNT = BLOCK_SIZE * ROUNDS;
constexpr Seconds ALMOST_IMMEDIATELY = 0.15 / (Seconds)SAMPLE_RATE;


TEST(basic_properties, {
    SumOfSines input(0.5, 220.0, 0.5, 440.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.set_sample_rate(SAMPLE_RATE);
    filter.set_block_size(BLOCK_SIZE);

    assert_eq((int)BLOCK_SIZE, (int)filter.get_block_size());
    assert_eq(SAMPLE_RATE, filter.get_sample_rate());

    assert_eq((int)BLOCK_SIZE, (int)filter.frequency.get_block_size());
    assert_eq(SAMPLE_RATE, filter.frequency.get_sample_rate());

    assert_eq((int)BLOCK_SIZE, (int)filter.q.get_block_size());
    assert_eq(SAMPLE_RATE, filter.q.get_sample_rate());

    assert_eq((int)BLOCK_SIZE, (int)filter.gain.get_block_size());
    assert_eq(SAMPLE_RATE, filter.gain.get_sample_rate());
})


template<class InputSignalProducerClass = SumOfSines>
void test_filter(
        BiquadFilter<InputSignalProducerClass>& filter,
        InputSignalProducerClass& input,
        SumOfSines& expected,
        Number const tolerance,
        Integer const rounds = ROUNDS,
        Integer const sample_count = SAMPLE_COUNT
) {
    Buffer expected_output(sample_count, CHANNELS);
    Buffer actual_output(sample_count, CHANNELS);

    filter.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);
    expected.set_block_size(BLOCK_SIZE);

    filter.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);
    expected.set_sample_rate(SAMPLE_RATE);

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    input.reset();
    render_rounds< BiquadFilter<InputSignalProducerClass> >(
        filter, actual_output, rounds
    );

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            sample_count,
            tolerance,
            "channel=%d",
            (int)c
        );
    }
}


void schedule_small_param_changes(
        BiquadFilter<SumOfSines>& filter,
        Number const frequency,
        Number const q,
        Number const gain
) {
    Seconds const two_blocks = (
        2.0 * filter.sample_count_to_time_offset(BLOCK_SIZE)
    );
    Seconds time_offset = (
        filter.sample_count_to_time_offset(SAMPLE_COUNT) / 3.0
    );

    filter.frequency.schedule_value(time_offset, frequency);

    time_offset += two_blocks;
    filter.q.schedule_value(time_offset, q);

    time_offset += two_blocks;
    filter.q.schedule_value(time_offset, q);
    filter.gain.schedule_value(time_offset, gain);

    time_offset += two_blocks;
    filter.frequency.schedule_value(time_offset, frequency + 0.01);
    filter.q.schedule_value(time_offset, q + 0.001);

    time_offset += two_blocks;
    filter.frequency.schedule_value(time_offset, frequency - 0.01);
    filter.gain.schedule_value(time_offset, gain + 0.01);

    time_offset += two_blocks;
    filter.q.schedule_value(time_offset, q + 0.002);
    filter.gain.schedule_value(time_offset, gain - 0.01);

    time_offset += two_blocks;
    filter.frequency.schedule_value(time_offset, frequency);
    filter.q.schedule_value(time_offset, q);
    filter.gain.schedule_value(time_offset, gain);
}


void assert_completed(
        BiquadFilter<SumOfSines>& filter,
        Number const expected_frequency,
        Number const expected_q,
        Number const expected_gain
) {
    char const* message = (
        "BiquadFilter failed to complete the timeline of its parameters"
    );

    assert_eq(
        expected_frequency, filter.frequency.get_value(), DOUBLE_DELTA, message
    );
    assert_eq(
        expected_q, filter.q.get_value(), DOUBLE_DELTA, message
    );
    assert_eq(expected_gain, filter.gain.get_value(), DOUBLE_DELTA, message);
}


TEST(when_frequency_is_at_max_value_then_low_pass_filter_is_no_op, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);
    Number const max_frequency = filter.frequency.get_max_value();

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    filter.frequency.set_value(max_frequency + 1.0);

    schedule_small_param_changes(filter, max_frequency + 1.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, max_frequency, 0.03, -6.0);
})


TEST(when_frequency_is_above_the_nyquist_frequency_then_low_pass_filter_is_no_op, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    filter.frequency.set_value(NYQUIST_FREQUENCY);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


/* JS80P doesn't let the frequency go below 0.1 Hz */
// TEST(when_frequency_is_at_minimum_then_low_pass_filter_is_silent, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // SumOfSines expected(0.0, 440.0, 0.0, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilter<SumOfSines>::TypeParam filter_type("");
    // BiquadFilter<SumOfSines> filter("", input, filter_type);

    // filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    // filter.frequency.set_value(filter.frequency.get_min_value());

    // test_filter(filter, input, expected, 0.0);
// })


TEST(low_pass_filter_attenuates_frequencies_above_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.5, 440.0, 0.0, 7040.0, 0.0, 0.0, CHANNELS, -0.0001875);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    filter.frequency.set_value(1000.0);
    filter.q.set_value(0.0);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


/* JS80P doesn't let the frequency go below 0.1 Hz */
// TEST(when_frequency_is_at_min_value_then_high_pass_filter_is_no_op, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilter<SumOfSines>::TypeParam filter_type("");
    // BiquadFilter<SumOfSines> filter("", input, filter_type);
    // Number const min_frequency = filter.frequency.get_min_value();

    // filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_PASS);
    // filter.frequency.set_value(min_frequency - 1.0);

    // schedule_small_param_changes(filter, min_frequency - 1.0, 0.03, -6.0);

    // test_filter(filter, input, input, 0.0);

    // assert_completed(filter, min_frequency, 0.03, -6.0);
// })


TEST(when_frequency_is_above_the_nyquist_frequency_then_high_pass_filter_is_silent, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.0, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_PASS);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


TEST(high_pass_filter_attenuates_frequencies_below_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_PASS);
    filter.frequency.set_value(1000.0);
    filter.q.set_value(0.03);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


TEST(when_q_is_zero_then_band_pass_is_no_op, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(0.0);

    schedule_small_param_changes(filter, 3520.0, 0.0, 0.0);

    test_filter(filter, input, expected, 0.01);

    assert_completed(filter, 3520.0, 0.0, 0.0);
})


TEST(when_frequency_is_above_the_nyquist_frequency_then_band_pass_is_silent, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.0, 3520.0, 0.0, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);
    filter.q.set_value(1.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 1.0, 0.0);

    test_filter(filter, input, expected, 0.01);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 1.0, 0.0);
})


TEST(band_pass_filter_attenuates_everything_outside_a_range_around_the_given_frequency, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.33, 3520.0, 0.0, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(5.0);

    schedule_small_param_changes(filter, 3520.0, 5.0, 0.0);

    test_filter(filter, input, expected, 0.12);

    assert_completed(filter, 3520.0, 5.0, 0.0);
})


TEST(when_q_is_zero_then_notch_filter_is_silent, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.0, 3520.0, 0.0, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::NOTCH);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(0.0);

    schedule_small_param_changes(filter, 3520.0, 0.0, 0.0);

    test_filter(filter, input, expected, 0.01);

    assert_completed(filter, 3520.0, 0.0, 0.0);
})


TEST(when_frequency_is_above_the_nyquist_frequency_then_notch_filter_is_no_op, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::NOTCH);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);
    filter.q.set_value(1.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 1.0, 0.0);

    test_filter(filter, input, expected, 0.01);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 1.0, 0.0);
})


TEST(notch_filter_attenuates_a_range_around_the_given_frequency, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.33, 440.0, 0.0, 3520.0, 0.33, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::NOTCH);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(5.0);

    schedule_small_param_changes(filter, 3520.0, 5.0, 0.0);

    test_filter(filter, input, expected, 0.05);

    assert_completed(filter, 3520.0, 5.0, 0.0);
})


TEST(when_q_is_zero_then_peaking_filter_becomes_gain, {
    SumOfSines input(0.15, 440.0, 0.15, 3520.0, 0.15, 7040.0, CHANNELS);
    SumOfSines expected(0.30, 440.0, 0.30, 3520.0, 0.30, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::PEAKING);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(0.0);
    filter.gain.set_value(6.0);

    schedule_small_param_changes(filter, 3520.0, 0.0, 6.0);

    test_filter(filter, input, expected, 0.01);

    assert_completed(filter, 3520.0, 0.0, 6.0);
})


TEST(when_frequency_is_above_the_nyquist_frequency_then_peaking_filter_is_no_op, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::PEAKING);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);
    filter.q.set_value(1.0);
    filter.gain.set_value(12.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 1.0, 0.0);

    test_filter(filter, input, expected, 0.01);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 1.0, 0.0);
})


TEST(peaking_filter_can_boost_or_attenuate_a_range_around_the_given_frequency, {
    SumOfSines input(0.25, 440.0, 0.25, 3520.0, 0.25, 7040.0, CHANNELS);
    SumOfSines expected(0.25, 440.0, 0.5, 3520.0, 0.25, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::PEAKING);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(1.0);
    filter.gain.set_value(6.0);

    schedule_small_param_changes(filter, 3520.0, 1.0, 6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 3520.0, 1.0, 6.0);
})


/* JS80P doesn't let the frequency go below 0.1 Hz */
// TEST(when_frequency_is_at_min_value_then_low_shelf_filter_is_no_op, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilter<SumOfSines>::TypeParam filter_type("");
    // BiquadFilter<SumOfSines> filter("", input, filter_type);
    // Number const min_frequency = filter.frequency.get_min_value();

    // filter.type.set_value(BiquadFilter<SumOfSines>::LOW_SHELF);
    // filter.frequency.set_value(min_frequency - 1.0);
    // filter.gain.set_value(-12.0);

    // schedule_small_param_changes(filter, min_frequency - 1.0, 0.03, -12.0);

    // test_filter(filter, input, input, 0.0);

    // assert_completed(filter, min_frequency, 0.03, -12.0);
// })


TEST(when_frequency_is_above_the_nyquist_frequency_then_low_shelf_filter_is_gain, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.25, 440.0, 0.25, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_SHELF);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.001);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


TEST(low_shelf_filter_attenuates_or_boosts_frequencies_below_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.25, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_SHELF);
    filter.frequency.set_value(2000.0);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.05);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


TEST(when_frequency_is_at_max_value_then_high_shelf_filter_is_no_op, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);
    Number const max_frequency = filter.frequency.get_max_value();

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    filter.frequency.set_value(max_frequency + 1.0);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, max_frequency + 1.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, max_frequency, 0.03, -6.0);
})


TEST(when_frequency_is_above_the_nyquist_frequency_then_high_shelf_filter_is_no_op, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    filter.frequency.set_value(NYQUIST_FREQUENCY);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


/* JS80P doesn't let the frequency go below 0.1 Hz */
// TEST(when_frequency_is_at_minimum_then_high_shelf_filter_is_gain, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // SumOfSines expected(0.25, 440.0, 0.25, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilter<SumOfSines>::TypeParam filter_type("");
    // BiquadFilter<SumOfSines> filter("", input, filter_type);

    // filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    // filter.frequency.set_value(filter.frequency.get_min_value());
    // filter.gain.set_value(-6.0);

    // test_filter(filter, input, expected, 0.001);
// })


TEST(high_shelf_filter_attenuates_or_boosts_frequencies_above_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.5, 440.0, 0.25, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    filter.frequency.set_value(2000.0);
    filter.q.set_value(0.03);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


class OtherSumOfSines : public SumOfSines
{
    public:
        OtherSumOfSines(
                Number const amplitude_1,
                Frequency const frequency_1,
                Number const amplitude_2,
                Frequency const frequency_2,
                Number const amplitude_3,
                Frequency const frequency_3,
                Integer const channels
        ) : SumOfSines(
            amplitude_1,
            frequency_1,
            amplitude_2,
            frequency_2,
            amplitude_3,
            frequency_3,
            channels
        ) {
        }
};


TEST(when_no_params_have_envelopes_then_uses_cached_coefficients, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    OtherSumOfSines input_other(
        0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS
    );
    SumOfSines expected_clones(0.0, 440.0, 0.33, 3520.0, 0.0, 7040.0, CHANNELS);
    SumOfSines expected_unique(0.33, 440.0, 0.0, 3520.0, 0.0, 7040.0, CHANNELS);
    SumOfSines expected_other(0.0, 440.0, 0.0, 3520.0, 0.33, 7040.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<OtherSumOfSines>::TypeParam filter_type_other("");
    BiquadFilter<SumOfSines> filter_clone_1(
        "", input, filter_type, BiquadFilter<SumOfSines>::Unicity::CLONED
    );
    BiquadFilter<SumOfSines> filter_clone_2(
        "", input, filter_type, BiquadFilter<SumOfSines>::Unicity::CLONED
    );
    BiquadFilter<SumOfSines> filter_unique(
        "", input, filter_type, BiquadFilter<SumOfSines>::Unicity::UNIQUE
    );
    BiquadFilter<OtherSumOfSines> filter_other(
        "",
        input_other,
        filter_type_other,
        BiquadFilter<OtherSumOfSines>::Unicity::CLONED
    );

    filter_clone_1.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter_clone_2.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter_unique.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter_other.type.set_value(BiquadFilter<OtherSumOfSines>::BAND_PASS);

    filter_clone_1.frequency.set_value(3520.0);
    filter_clone_2.frequency.set_value(3520.0);
    filter_unique.frequency.set_value(440.0);
    filter_other.frequency.set_value(7040.0);

    filter_clone_1.q.set_value(5.0);
    filter_clone_2.q.set_value(5.0);
    filter_unique.q.set_value(5.0);
    filter_other.q.set_value(5.0);

    test_filter(filter_clone_1, input, expected_clones, 0.12, 1, BLOCK_SIZE);

    filter_clone_2.frequency.set_value(15000.0);
    test_filter(filter_clone_2, input, expected_clones, 0.12, 1, BLOCK_SIZE);

    test_filter(filter_unique, input, expected_unique, 0.12, 1, BLOCK_SIZE);
    test_filter<OtherSumOfSines>(
        filter_other, input_other, expected_other, 0.12, 1, BLOCK_SIZE
    );
})


void set_up_chunk_size_independent_test(
        BiquadFilter<SumOfSines>& filter,
        BiquadFilter<SumOfSines>::Type const type,
        SumOfSines& input
) {
    constexpr Frequency sample_rate = 44100.0;

    input.set_sample_rate(sample_rate);
    input.set_block_size(16384);

    filter.type.set_value(type);
    filter.set_sample_rate(sample_rate);
    filter.frequency.set_value(3000.0);
    filter.frequency.schedule_linear_ramp(0.25, 3500.0);
    filter.q.set_value(0.0);
    filter.q.schedule_linear_ramp(0.5, 1.0);
    filter.gain.set_value(-6.0);
    filter.gain.schedule_linear_ramp(0.33, -12.0);
}


void assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::Type const type,
        char const* message
) {
    SumOfSines input_1(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines input_2(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines>::TypeParam filter_type("");
    BiquadFilter<SumOfSines> filter_1("", input_1, filter_type);
    BiquadFilter<SumOfSines> filter_2("", input_2, filter_type);

    set_up_chunk_size_independent_test(filter_1, type, input_1);
    set_up_chunk_size_independent_test(filter_2, type, input_2);

    assert_rendering_is_independent_from_chunk_size< BiquadFilter<SumOfSines> >(
        filter_1, filter_2, DOUBLE_DELTA, message
    );
}


TEST(filter_rendering_is_independent_of_chunk_size, {
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::LOW_PASS, "low-pass"
    );
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::HIGH_PASS, "high-pass"
    );
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::BAND_PASS, "band-pass"
    );
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::NOTCH, "notch"
    );
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::PEAKING, "peaking"
    );
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::LOW_SHELF, "low shelf"
    );
    assert_filter_rendering_is_independent_of_chunk_size(
        BiquadFilter<SumOfSines>::HIGH_SHELF, "high shelf"
    );
})
