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

#include "dsp/biquad_filter.cpp"
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


constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 256;
constexpr Integer ROUNDS = 50;
constexpr Integer SAMPLE_COUNT = BLOCK_SIZE * ROUNDS;
constexpr Seconds ALMOST_IMMEDIATELY = 0.15 / (Seconds)SAMPLE_RATE;


TEST(basic_properties, {
    SumOfSines input(0.5, 220.0, 0.5, 440.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
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


template<
    class InputSignalProducerClass = SumOfSines,
    BiquadFilterFixedType fixed_type = BiquadFilterFixedType::BFFT_CUSTOMIZABLE
>
void test_filter(
        BiquadFilter<InputSignalProducerClass, fixed_type>& filter,
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
    render_rounds< BiquadFilter<InputSignalProducerClass, fixed_type> >(
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


template<BiquadFilterFixedType fixed_type = BiquadFilterFixedType::BFFT_CUSTOMIZABLE>
void schedule_small_param_changes(
        BiquadFilter<SumOfSines, fixed_type>& filter,
        Number const frequency,
        Number const q,
        Number const gain
) {
    Seconds const two_blocks = (
        2.0 * filter.sample_count_to_relative_time_offset(BLOCK_SIZE)
    );
    Seconds time_offset = (
        filter.sample_count_to_relative_time_offset(SAMPLE_COUNT) / 3.0
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


template<BiquadFilterFixedType fixed_type = BiquadFilterFixedType::BFFT_CUSTOMIZABLE>
void assert_completed(
        BiquadFilter<SumOfSines, fixed_type>& filter,
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


void test_silent_input_is_no_op(Byte const type)
{
    constexpr Number low_amplitude = 1e-9;

    SumOfSines input(low_amplitude, 440.0, low_amplitude, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(type);

    schedule_small_param_changes(filter, 1000.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, 1000.0, 0.03, -6.0);
}


TEST(when_input_is_silent_then_biquad_filter_is_no_op, {
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::LOW_PASS);
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::HIGH_PASS);
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::BAND_PASS);
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::NOTCH);
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::PEAKING);
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::LOW_SHELF);
    test_silent_input_is_no_op(BiquadFilter<SumOfSines>::HIGH_SHELF);
})


TEST(when_frequency_is_at_max_value_then_low_pass_filter_is_no_op, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    filter.frequency.set_value(NYQUIST_FREQUENCY);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


/* JS80P doesn't let the frequency go below 1.0 Hz */
// TEST(when_frequency_is_at_minimum_then_low_pass_filter_is_silent, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // SumOfSines expected(0.0, 440.0, 0.0, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilterTypeParam filter_type("");
    // BiquadFilter<SumOfSines> filter("", input, filter_type);

    // filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    // filter.frequency.set_value(filter.frequency.get_min_value());

    // test_filter(filter, input, expected, 0.0);
// })


TEST(low_pass_filter_attenuates_frequencies_above_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.5, 440.0, 0.0, 7040.0, 0.0, 0.0, CHANNELS, -0.0001875);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    filter.frequency.set_value(1000.0);
    filter.q.set_value(0.0);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


/* JS80P doesn't let the frequency go below 1.0 Hz */
// TEST(when_frequency_is_at_min_value_then_high_pass_filter_is_no_op, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_PASS);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


TEST(when_buffer_is_external_and_frequency_is_above_the_nyquist_frequency_then_high_pass_filter_is_silent, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.0, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type, NULL, 0.0, NULL, NULL, &input);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_PASS);
    filter.frequency.set_value(NYQUIST_FREQUENCY + 1.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


TEST(high_pass_filter_attenuates_frequencies_below_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_PASS);
    filter.frequency.set_value(1000.0);
    filter.q.set_value(0.03);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


TEST(fixed_type_high_pass_filter_attenuates_frequencies_below_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.0, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines, BiquadFilterFixedType::BFFT_HIGH_PASS> filter(
        "", input
    );

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_PASS);
    filter.frequency.set_value(1000.0);
    filter.q.set_value(0.03);

    schedule_small_param_changes<BiquadFilterFixedType::BFFT_HIGH_PASS>(
        filter, 2000.0, 0.03, -6.0
    );

    test_filter<SumOfSines, BiquadFilterFixedType::BFFT_HIGH_PASS>(
        filter, input, expected, 0.1
    );

    assert_completed<BiquadFilterFixedType::BFFT_HIGH_PASS>(
        filter, 2000.0, 0.03, -6.0
    );
})


TEST(when_q_is_zero_then_band_pass_is_no_op, {
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::PEAKING);
    filter.frequency.set_value(3520.0);
    filter.q.set_value(1.0);
    filter.gain.set_value(6.0);

    schedule_small_param_changes(filter, 3520.0, 1.0, 6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 3520.0, 1.0, 6.0);
})


/* JS80P doesn't let the frequency go below 1.0 Hz */
// TEST(when_frequency_is_at_min_value_then_low_shelf_filter_is_no_op, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
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
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    filter.frequency.set_value(NYQUIST_FREQUENCY);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);

    test_filter(filter, input, input, 0.0);

    assert_completed(filter, NYQUIST_FREQUENCY + 1.0, 0.03, -6.0);
})


/* JS80P doesn't let the frequency go below 1.0 Hz */
// TEST(when_frequency_is_at_minimum_then_high_shelf_filter_is_gain, {
    // SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    // SumOfSines expected(0.25, 440.0, 0.25, 7040.0, 0.0, 0.0, CHANNELS);
    // BiquadFilterTypeParam filter_type("");
    // BiquadFilter<SumOfSines> filter("", input, filter_type);

    // filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    // filter.frequency.set_value(filter.frequency.get_min_value());
    // filter.gain.set_value(-6.0);

    // test_filter(filter, input, expected, 0.001);
// })


TEST(high_shelf_filter_attenuates_or_boosts_frequencies_above_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.5, 440.0, 0.25, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter("", input, filter_type);

    filter.type.set_value(BiquadFilter<SumOfSines>::HIGH_SHELF);
    filter.frequency.set_value(2000.0);
    filter.q.set_value(0.03);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes(filter, 2000.0, 0.03, -6.0);

    test_filter(filter, input, expected, 0.1);

    assert_completed(filter, 2000.0, 0.03, -6.0);
})


TEST(fixed_type_high_shelf_filter_attenuates_or_boosts_frequencies_above_the_given_frequency, {
    SumOfSines input(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected(0.5, 440.0, 0.25, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilter<SumOfSines, BiquadFilterFixedType::BFFT_HIGH_SHELF> filter(
        "", input
    );

    filter.type.set_value(BiquadFilter<SumOfSines>::LOW_SHELF);
    filter.frequency.set_value(2000.0);
    filter.q.set_value(0.03);
    filter.gain.set_value(-6.0);

    schedule_small_param_changes<BiquadFilterFixedType::BFFT_HIGH_SHELF>(
        filter, 2000.0, 0.03, -6.0
    );

    test_filter<SumOfSines, BiquadFilterFixedType::BFFT_HIGH_SHELF>(
        filter, input, expected, 0.1
    );

    assert_completed<BiquadFilterFixedType::BFFT_HIGH_SHELF>(
        filter, 2000.0, 0.03, -6.0
    );
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


TEST(when_no_params_are_polyphonic_then_uses_cached_coefficients, {
    BiquadFilterSharedBuffers shared_buffers;
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected_clones(0.0, 440.0, 0.33, 3520.0, 0.0, 7040.0, CHANNELS);
    SumOfSines expected_unique(0.33, 440.0, 0.0, 3520.0, 0.0, 7040.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<SumOfSines> filter_clone_1(
        "", input, filter_type, &shared_buffers
    );
    BiquadFilter<SumOfSines> filter_clone_2(
        "", input, filter_type, &shared_buffers
    );
    BiquadFilter<SumOfSines> filter_unique(
        "", input, filter_type, NULL
    );

    shared_buffers.b0_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.b1_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.b2_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.a1_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.a2_buffer = new Sample[BLOCK_SIZE];

    filter_clone_1.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter_clone_2.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    filter_unique.type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);

    filter_clone_1.frequency.set_value(3520.0);
    filter_clone_2.frequency.set_value(3520.0);
    filter_unique.frequency.set_value(440.0);

    filter_clone_1.q.set_value(5.0);
    filter_clone_2.q.set_value(5.0);
    filter_unique.q.set_value(5.0);

    test_filter(filter_clone_1, input, expected_clones, 0.12, 1, BLOCK_SIZE);

    filter_clone_2.frequency.set_value(15000.0);
    test_filter(filter_clone_2, input, expected_clones, 0.12, 1, BLOCK_SIZE);

    test_filter(filter_unique, input, expected_unique, 0.12, 1, BLOCK_SIZE);

    delete[] shared_buffers.b0_buffer;
    delete[] shared_buffers.b1_buffer;
    delete[] shared_buffers.b2_buffer;
    delete[] shared_buffers.a1_buffer;
    delete[] shared_buffers.a2_buffer;
})


TEST(when_params_are_polyphonic_then_does_not_use_cached_coefficients, {
    /* Compensate for the headroom of the bandwidth-limited LFO square wave. */
    constexpr Number headroom = 1.1;

    BiquadFilterSharedBuffers shared_buffers;
    SumOfSines input(0.33, 440.0, 0.33, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected_1(0.0, 440.0, 0.0, 3520.0, 0.33, 7040.0, CHANNELS);
    SumOfSines expected_2(0.33, 440.0, 0.0, 3520.0, 0.0, 7040.0, CHANNELS);
    Envelope envelope("ENV");
    Envelope* envelopes[Constants::ENVELOPES] = {
        &envelope, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL,
    };
    BiquadFilterTypeParam filter_type("TYP");
    FloatParamS frequency(
        "FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        envelopes
    );
    FloatParamS q(
        "Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT,
        0.0,
        envelopes
    );
    FloatParamS gain(
        "G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        Constants::BIQUAD_FILTER_GAIN_DEFAULT,
        0.0,
        envelopes
    );
    LFO lfo("LFO", true);
    BiquadFilter<SumOfSines> filter_1(
        input, filter_type, frequency, q, gain, &shared_buffers
    );
    BiquadFilter<SumOfSines> filter_2(
        input, filter_type, frequency, q, gain, &shared_buffers
    );

    lfo.waveform.set_value(LFO::Oscillator_::SQUARE);
    lfo.frequency.set_value(Constants::LFO_FREQUENCY_MIN);
    lfo.phase.set_value(0.1);
    lfo.min.set_value(0.0);
    lfo.max.set_value(1.0);
    lfo.amount.set_value(1.0);
    lfo.amount_envelope.set_value(0.0);
    lfo.start(0.0);

    envelope.initial_value.set_value(1.0);
    envelope.peak_value.set_value(1.0);
    envelope.sustain_value.set_value(1.0);
    envelope.final_value.set_value(1.0);

    filter_type.set_value(BiquadFilter<SumOfSines>::BAND_PASS);
    frequency.set_lfo(&lfo);
    q.set_value(5.0);

    shared_buffers.b0_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.b1_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.b2_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.a1_buffer = new Sample[BLOCK_SIZE];
    shared_buffers.a2_buffer = new Sample[BLOCK_SIZE];

    envelope.amount.set_value(filter_1.frequency.value_to_ratio(7040.0) * headroom);
    filter_1.frequency.start_envelope(0.0, 0.0, 0.0);

    envelope.amount.set_value(filter_2.frequency.value_to_ratio(440.0) * headroom);
    filter_2.frequency.start_envelope(0.0, 0.0, 0.0);

    test_filter(filter_1, input, expected_1, 0.11, 1, BLOCK_SIZE);
    test_filter(filter_2, input, expected_2, 0.11, 1, BLOCK_SIZE);

    delete[] shared_buffers.b0_buffer;
    delete[] shared_buffers.b1_buffer;
    delete[] shared_buffers.b2_buffer;
    delete[] shared_buffers.a1_buffer;
    delete[] shared_buffers.a2_buffer;
})


void test_fast_path_continuity(
        Integer const block_size,
        Integer const batch_size,
        BiquadFilterSharedBuffers* const shared_buffers,
        Byte const type,
        Number const q,
        Sample const silent_round_input_sample
) {
    constexpr Number tolerance = 0.0065;

    Sample input_channel[block_size];
    Sample* input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel, input_channel
    };
    Sample const* const* rendered = NULL;
    FixedSignalProducer input(input_channels);
    BiquadFilterTypeParam filter_type("");
    BiquadFilter<FixedSignalProducer> filter_1("", input, filter_type, shared_buffers);
    BiquadFilter<FixedSignalProducer> filter_2("", input, filter_type, shared_buffers);

    input.set_sample_rate(22050.0);
    input.set_block_size(block_size);
    filter_type.set_sample_rate(22050.0);
    filter_type.set_block_size(block_size);
    filter_1.set_sample_rate(22050.0);
    filter_1.set_block_size(block_size);
    filter_2.set_sample_rate(22050.0);
    filter_2.set_block_size(block_size);

    filter_type.set_value(type);

    filter_1.frequency.set_value(5000.0);
    filter_1.q.set_value(1.0);

    filter_2.frequency.set_value(5000.0);
    filter_2.q.set_value(1.0);

    std::fill_n(input_channel, block_size, 0.9);
    input_channel[0] = 0.3;
    input_channel[1] = 0.6;
    SignalProducer::produce< BiquadFilter<FixedSignalProducer> >(
        filter_1, 1, batch_size
    );
    rendered = SignalProducer::produce< BiquadFilter<FixedSignalProducer> >(
        filter_2, 1, batch_size
    );

    /*
    Too small batch will differ due to filter latency. We don't assert, but let
    valgrind find memory handling errors.
    */
    if (batch_size > 8) {
        assert_close(
            input_channel,
            rendered[0],
            batch_size,
            tolerance,
            "shared_buffers=%p, type=%d",
            (void*)shared_buffers,
            (int)type
        );
        assert_close(
            input_channel,
            rendered[1],
            batch_size,
            tolerance,
            "shared_buffers=%p, type=%d",
            (void*)shared_buffers,
            (int)type
        );
    }

    filter_1.q.set_value(q);
    filter_2.q.set_value(q);

    std::fill_n(input_channel, block_size, silent_round_input_sample);
    SignalProducer::produce< BiquadFilter<FixedSignalProducer> >(
        filter_1, 2, batch_size
    );
    rendered = SignalProducer::produce< BiquadFilter<FixedSignalProducer> >(
        filter_2, 2, batch_size
    );

    /*
    Too small batch will differ due to filter latency. We don't assert, but let
    valgrind find memory handling errors.
    */
    if (batch_size > 8) {
        std::fill_n(input_channel, block_size, 0.0);
        assert_close(
            input_channel,
            rendered[0],
            batch_size,
            tolerance,
            "shared_buffers=%p, type=%d",
            (void*)shared_buffers,
            (int)type
        );
        assert_close(
            input_channel,
            rendered[1],
            batch_size,
            tolerance,
            "shared_buffers=%p, type=%d",
            (void*)shared_buffers,
            (int)type
        );
    }

    filter_1.q.set_value(1.0);
    filter_2.q.set_value(1.0);

    std::fill_n(input_channel, block_size, -0.9);
    input_channel[0] = -0.3;
    input_channel[1] = -0.6;
    SignalProducer::produce< BiquadFilter<FixedSignalProducer> >(
        filter_1, 3, batch_size
    );
    rendered = SignalProducer::produce< BiquadFilter<FixedSignalProducer> >(
        filter_2, 3, batch_size
    );

    /*
    Too small batch will differ due to filter latency. We don't assert, but let
    valgrind find memory handling errors.
    */
    if (batch_size > 8) {
        assert_close(
            input_channel,
            rendered[0],
            batch_size,
            tolerance,
            "shared_buffers=%p, type=%d",
            (void*)shared_buffers,
            (int)type
        );
        assert_close(
            input_channel,
            rendered[1],
            batch_size,
            tolerance,
            "shared_buffers=%p, type=%d",
            (void*)shared_buffers,
            (int)type
        );
    }
}


TEST(silent_input_fast_path_keeps_continuity, {
    constexpr Integer block_size = 128;

    BiquadFilterSharedBuffers shared_buffers;

    shared_buffers.b0_buffer = new Sample[block_size];
    shared_buffers.b1_buffer = new Sample[block_size];
    shared_buffers.b2_buffer = new Sample[block_size];
    shared_buffers.a1_buffer = new Sample[block_size];
    shared_buffers.a2_buffer = new Sample[block_size];

    test_fast_path_continuity(
        block_size, 0, NULL, BiquadFilter<FixedSignalProducer>::LOW_PASS, 1.0, 0.0
    );
    test_fast_path_continuity(
        block_size, 0, &shared_buffers, BiquadFilter<FixedSignalProducer>::LOW_PASS, 1.0, 0.0
    );

    test_fast_path_continuity(
        block_size, 0, NULL, BiquadFilter<FixedSignalProducer>::NOTCH, 0.0, 0.9
    );
    test_fast_path_continuity(
        block_size, 0, &shared_buffers, BiquadFilter<FixedSignalProducer>::NOTCH, 0.0, 0.9
    );

    test_fast_path_continuity(
        block_size, 1, NULL, BiquadFilter<FixedSignalProducer>::LOW_PASS, 1.0, 0.0
    );
    test_fast_path_continuity(
        block_size, 1, &shared_buffers, BiquadFilter<FixedSignalProducer>::LOW_PASS, 1.0, 0.0
    );

    test_fast_path_continuity(
        block_size, 1, NULL, BiquadFilter<FixedSignalProducer>::NOTCH, 0.0, 0.9
    );
    test_fast_path_continuity(
        block_size, 1, &shared_buffers, BiquadFilter<FixedSignalProducer>::NOTCH, 0.0, 0.9
    );

    test_fast_path_continuity(
        block_size, block_size, NULL, BiquadFilter<FixedSignalProducer>::LOW_PASS, 1.0, 0.0
    );
    test_fast_path_continuity(
        block_size, block_size, &shared_buffers, BiquadFilter<FixedSignalProducer>::LOW_PASS, 1.0, 0.0
    );

    test_fast_path_continuity(
        block_size, block_size, NULL, BiquadFilter<FixedSignalProducer>::NOTCH, 0.0, 0.9
    );
    test_fast_path_continuity(
        block_size, block_size, &shared_buffers, BiquadFilter<FixedSignalProducer>::NOTCH, 0.0, 0.9
    );

    delete[] shared_buffers.b0_buffer;
    delete[] shared_buffers.b1_buffer;
    delete[] shared_buffers.b2_buffer;
    delete[] shared_buffers.a1_buffer;
    delete[] shared_buffers.a2_buffer;
})


void set_up_chunk_size_independent_test(
        BiquadFilter<SumOfSines>& filter,
        Byte const type,
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
        Byte const type,
        char const* message
) {
    SumOfSines input_1(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    SumOfSines input_2(0.5, 440.0, 0.5, 7040.0, 0.0, 0.0, CHANNELS);
    BiquadFilterTypeParam filter_type("");
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
