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

#include <cstring>
#include <vector>

#include "js80p.hpp"

#include "dsp/envelope.cpp"
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


constexpr Frequency SAMPLE_RATE = 11025.0;
constexpr Integer BLOCK_SIZE = 2048;
constexpr Integer CHANNELS = 1;

constexpr Byte OFF = ToggleParam::OFF;
constexpr Byte ON = ToggleParam::ON;


void test_lfo(
        Byte const tempo_sync,
        Number const bpm,
        Frequency const frequency,
        Frequency const expected_frequency
) {
    constexpr Integer rounds = 20;
    constexpr Integer sample_count = BLOCK_SIZE * rounds;
    constexpr Number phase = 0.3333;
    constexpr Number min = 0.1;
    constexpr Number max = 0.7;
    constexpr Number amount = 0.75 * 0.5;
    constexpr Number range = max - min;
    constexpr Sample expected_sample_offset = min + amount * range;

    Seconds const phase_seconds = phase / expected_frequency;

    LFO lfo("L1");
    SumOfSines expected(
        amount * range,
        expected_frequency,
        0.0,
        0.0,
        0.0,
        0.0,
        1,
        phase_seconds,
        expected_sample_offset
    );
    Buffer expected_output(sample_count, CHANNELS);
    Buffer actual_output(sample_count, CHANNELS);

    expected.set_block_size(BLOCK_SIZE);
    expected.set_sample_rate(SAMPLE_RATE);

    lfo.set_block_size(BLOCK_SIZE);
    lfo.set_sample_rate(SAMPLE_RATE);
    lfo.set_bpm(bpm);
    lfo.waveform.set_value(LFO::Oscillator_::SINE);
    lfo.phase.set_value(phase - 0.000001);
    lfo.phase.schedule_value(0.001, phase);
    lfo.frequency.set_value(frequency - 0.000001);
    lfo.frequency.schedule_value(0.2, frequency);
    lfo.min.set_value(min - 0.000001);
    lfo.min.schedule_value(0.4, min);
    lfo.max.set_value(max - 0.000001);
    lfo.max.schedule_value(0.6, max);
    lfo.amount.set_value(amount - 0.000001);
    lfo.amount.schedule_value(0.8, amount);
    lfo.tempo_sync.set_value(tempo_sync);
    lfo.center.set_value(OFF);
    lfo.start(0.0);

    assert_false(lfo.is_on());

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    render_rounds<LFO>(lfo, actual_output, rounds);

    assert_true(lfo.is_on());

    assert_eq(
        expected_output.samples[0],
        actual_output.samples[0],
        sample_count,
        0.001,
        "tempo_sync=%s",
        tempo_sync ? "ON" : "OFF"
    );
}


TEST(lfo_oscillates_between_min_and_max_times_amount, {
    test_lfo(OFF, 180.0, 20.0, 20.0);
    test_lfo(ON, 180.0, 20.0, 60.0);
})


TEST(when_lfo_is_centered_then_it_oscillates_around_the_center_point_between_min_and_max, {
    constexpr Integer rounds = 20;
    constexpr Integer sample_count = BLOCK_SIZE * rounds;
    constexpr Number min = 0.1;
    constexpr Number max = 0.5;
    constexpr Number amount = 0.25;
    constexpr Frequency frequency = 30.0;

    LFO lfo("L1");
    FloatParamS param("F", -3.0, 7.0, 0.0);
    SumOfSines expected(1.0, frequency, 0.0, 0.0, 0.0, 0.0, 1);
    Buffer expected_output(sample_count, CHANNELS);
    Buffer actual_output(sample_count, CHANNELS);

    expected.set_block_size(BLOCK_SIZE);
    expected.set_sample_rate(SAMPLE_RATE);

    param.set_block_size(BLOCK_SIZE);
    param.set_sample_rate(SAMPLE_RATE);
    param.set_lfo(&lfo);

    lfo.set_block_size(BLOCK_SIZE);
    lfo.set_sample_rate(SAMPLE_RATE);
    lfo.waveform.set_value(LFO::Oscillator_::SINE);
    lfo.frequency.set_value(frequency - 0.000001);
    lfo.frequency.schedule_value(0.2, frequency);
    lfo.min.set_value(min - 0.000001);
    lfo.min.schedule_value(0.4, min);
    lfo.max.set_value(max - 0.000001);
    lfo.max.schedule_value(0.6, max);
    lfo.amount.set_value(amount - 0.000001);
    lfo.amount.schedule_value(0.8, amount);
    lfo.center.set_value(ON);
    lfo.start(0.0);

    assert_false(lfo.is_on());

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    render_rounds<FloatParamS>(param, actual_output, rounds);

    assert_true(lfo.is_on());

    assert_eq(
        expected_output.samples[0], actual_output.samples[0], sample_count, 0.001
    );
})


TEST(lfo_performance, {
    /*
    Usage: time ./build/dev-linux-x86_64-avx/test_lfo lfo_performance ON|OFF number-of-samples
    */
    LFO lfo("L1");

    if (TEST_ARGV.size() < 3) {
        return;
    }

    Integer const rounds = atoi(TEST_ARGV.back().c_str());

    assert_gt((int)rounds, 0, "Number of rounds to render must be positive");

    TEST_ARGV.pop_back();

    std::string const center = TEST_ARGV.back();

    if (center == "ON") {
        lfo.center.set_value(ToggleParam::ON);
    } else if (center == "OFF") {
        lfo.center.set_value(ToggleParam::OFF);
    } else {
        assert_true(
            false,
            "Unknown setting for LFO::center: \"%s\" - must be \"ON\" or \"OFF\"\n",
            TEST_ARGV[2]
        );
    }

    lfo.set_block_size(BLOCK_SIZE);
    lfo.set_sample_rate(SAMPLE_RATE);
    lfo.amount.set_value(0.99);
    lfo.amount.schedule_linear_ramp(5.0, 1.0);

    Number const total_sample_count = (Number)(BLOCK_SIZE * rounds);

    Integer number_of_rendered_samples = 0;
    Number sum = 0.0;

    for (Integer round = 0; round != rounds; ++round) {
        Sample const* const* const rendered_samples = (
            SignalProducer::produce<LFO>(lfo, round)
        );
        number_of_rendered_samples += BLOCK_SIZE;

        for (Integer i = 0; i != BLOCK_SIZE; ++i) {
            sum += rendered_samples[0][i];
        }
    }

    assert_lt(-100000.0, sum / total_sample_count);
})


void test_lfo_modifier_statistics(
        Number const distortion,
        Number const randomness,
        Byte const centered,
        Number const tolerance
) {
    LFO lfo("L1");
    std::vector<Number> numbers;
    Sample const* const* rendered_samples;
    Math::Statistics stats;
    char message[128];

    snprintf(
        message,
        128,
        "distortion=%f, randomness=%f, centered=%s",
        distortion,
        randomness,
        centered == ToggleParam::ON ? "ON" : "OFF"
    );

    lfo.set_block_size(BLOCK_SIZE);
    lfo.set_sample_rate(SAMPLE_RATE);
    lfo.waveform.set_value(LFO::Oscillator_::TRIANGLE);
    lfo.min.set_value(0.25);
    lfo.max.set_value(0.75);
    lfo.distortion.set_value(distortion);
    lfo.distortion.set_value(randomness);
    lfo.frequency.set_value(30.0);
    lfo.center.set_value(centered);
    lfo.start(0.0);

    rendered_samples = SignalProducer::produce<LFO>(lfo, 1);

    numbers.reserve(BLOCK_SIZE);

    for (Integer i = 0; i != BLOCK_SIZE; ++i) {
        numbers.push_back(rendered_samples[0][i]);
    }

    Math::compute_statistics(numbers, stats);

    assert_statistics(true, 0.25, 0.5, 0.75, 0.5, 0.125, stats, tolerance, message);
    assert_gte(0.75, stats.max);
    assert_lte(0.25, stats.min);
}


TEST(distortion_and_randomness_respect_min_and_max_values, {
    test_lfo_modifier_statistics(0.0, 0.0, ToggleParam::OFF, 0.02);
    test_lfo_modifier_statistics(1.0, 0.0, ToggleParam::OFF, 0.02);
    test_lfo_modifier_statistics(0.0, 1.0, ToggleParam::OFF, 0.14);
    test_lfo_modifier_statistics(1.0, 1.0, ToggleParam::OFF, 0.14);
    test_lfo_modifier_statistics(0.0, 0.0, ToggleParam::ON, 0.02);
    test_lfo_modifier_statistics(1.0, 0.0, ToggleParam::ON, 0.02);
    test_lfo_modifier_statistics(0.0, 1.0, ToggleParam::ON, 0.14);
    test_lfo_modifier_statistics(1.0, 1.0, ToggleParam::ON, 0.14);
})


TEST(can_tell_if_an_envelope_is_set_even_when_there_is_a_dependency_cycle_between_lfos, {
    LFO lfo_1("L1");
    LFO lfo_2("L2");
    LFO lfo_3("L3");
    LFOEnvelopeList envelope_list;

    lfo_1.randomness.set_lfo(&lfo_2);
    lfo_2.randomness.set_lfo(&lfo_3);
    lfo_3.randomness.set_lfo(&lfo_1);

    assert_false(lfo_1.has_envelope());
    assert_false(lfo_2.has_envelope());
    assert_false(lfo_3.has_envelope());

    lfo_1.amount_envelope.set_value(3);
    lfo_2.amount_envelope.set_value(5);
    lfo_3.amount_envelope.set_value(9);

    assert_true(lfo_1.has_envelope());
    assert_true(lfo_2.has_envelope());
    assert_true(lfo_3.has_envelope());

    lfo_1.collect_envelopes(envelope_list);

    assert_eq(3, envelope_list[0]);
    assert_eq(5, envelope_list[1]);
    assert_eq(9, envelope_list[2]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, envelope_list[3]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, envelope_list[4]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, envelope_list[5]);
})


void test_inverted_min_max_lfo(
        Byte const centering,
        Number const min,
        Number const max,
        Number const amount,
        Number const exp_phase,
        Number const exp_min,
        Number const exp_max
) {
    LFO expected("E");
    LFO lfo("L");
    Sample const* const* expected_samples;
    Sample const* const* rendered_samples;

    expected.set_block_size(BLOCK_SIZE);
    lfo.set_block_size(BLOCK_SIZE);

    expected.set_sample_rate(SAMPLE_RATE);
    lfo.set_sample_rate(SAMPLE_RATE);

    expected.phase.set_ratio(exp_phase);
    expected.min.set_ratio(exp_min);
    expected.max.set_ratio(exp_max);
    expected.frequency.set_value(20.0);

    lfo.min.set_ratio(min);
    lfo.max.set_ratio(max);
    lfo.amount.set_ratio(amount);
    lfo.center.set_ratio(centering);
    lfo.frequency.set_value(20.0);

    expected.start(0.0);
    lfo.start(0.0);

    expected_samples = SignalProducer::produce<LFO>(expected, 1);
    rendered_samples = SignalProducer::produce<LFO>(lfo, 1);

    assert_eq(expected_samples[0], rendered_samples[0], BLOCK_SIZE, DOUBLE_DELTA);
}


TEST(min_and_max_values_may_be_inverted, {
    test_inverted_min_max_lfo(ToggleParam::ON, 0.7, 0.2, 0.6, 0.5, 0.3, 0.6);
    test_inverted_min_max_lfo(ToggleParam::OFF, 0.7, 0.2, 0.6, 0.5, 0.4, 0.7);
})


TEST(when_a_round_is_skipped_then_params_are_still_processed, {
    constexpr Seconds duration = (Number)(BLOCK_SIZE - 1) / SAMPLE_RATE;

    LFO lfo("L");

    lfo.set_block_size(BLOCK_SIZE);
    lfo.set_sample_rate(SAMPLE_RATE);
    lfo.start(0.0);

    lfo.frequency.schedule_linear_ramp(duration, 0.7);
    lfo.phase.schedule_linear_ramp(duration, 0.6);
    lfo.min.schedule_linear_ramp(duration, 0.5);
    lfo.max.schedule_linear_ramp(duration, 0.4);
    lfo.amount.schedule_linear_ramp(duration, 0.3);
    lfo.distortion.schedule_linear_ramp(duration, 0.2);
    lfo.randomness.schedule_linear_ramp(duration, 0.1);

    lfo.skip_round(1, BLOCK_SIZE);

    assert_eq(0.7, lfo.frequency.get_value(), DOUBLE_DELTA);
    assert_eq(0.6, lfo.phase.get_value(), DOUBLE_DELTA);
    assert_eq(0.5, lfo.min.get_value(), DOUBLE_DELTA);
    assert_eq(0.4, lfo.max.get_value(), DOUBLE_DELTA);
    assert_eq(0.3, lfo.amount.get_value(), DOUBLE_DELTA);
    assert_eq(0.2, lfo.distortion.get_value(), DOUBLE_DELTA);
    assert_eq(0.1, lfo.randomness.get_value(), DOUBLE_DELTA);
})
