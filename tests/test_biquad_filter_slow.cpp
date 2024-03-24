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
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 256;


typedef Number SumOfSinesAmplitudes[3];
typedef Frequency SumOfSinesFrequencies[3];


void test_inaccurate_filter_with_random(
        Number const random,
        BiquadFilter<SumOfSines>::Type const filter_type,
        Frequency const frequency,
        Number const q,
        Number const gain,
        SumOfSinesAmplitudes const& input_amplitudes,
        SumOfSinesFrequencies const& input_frequencies,
        SumOfSinesAmplitudes const& expected_amplitudes,
        SumOfSinesFrequencies const& expected_frequencies,
        Number const phase_offset,
        Number const tolerance
) {
    constexpr Integer rounds = 20;
    constexpr Integer sample_count = BLOCK_SIZE * rounds;

    SumOfSines input(
        input_amplitudes[0], input_frequencies[0],
        input_amplitudes[1], input_frequencies[1],
        input_amplitudes[2], input_frequencies[2],
        CHANNELS
    );
    SumOfSines expected(
        expected_amplitudes[0], expected_frequencies[0],
        expected_amplitudes[1], expected_frequencies[1],
        expected_amplitudes[2], expected_frequencies[2],
        CHANNELS,
        phase_offset
    );
    BiquadFilter<SumOfSines>::TypeParam filter_type_param("");
    FloatParamB inaccuracy("IA", 0.0, 1.0, 0.2);
    BiquadFilter<SumOfSines> filter(
        "", input, filter_type_param, NULL, 1.0, &inaccuracy, &inaccuracy
    );

    Buffer expected_output(sample_count, CHANNELS);
    Buffer actual_output(sample_count, CHANNELS);

    filter.set_block_size(BLOCK_SIZE);
    inaccuracy.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);
    expected.set_block_size(BLOCK_SIZE);

    filter.set_sample_rate(SAMPLE_RATE);
    inaccuracy.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);
    expected.set_sample_rate(SAMPLE_RATE);

    filter.type.set_value(filter_type);
    filter.frequency.set_value(frequency);
    filter.q.set_value(q);
    filter.gain.set_value(gain);

    filter.update_inaccuracy(random, random);

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    input.reset();
    render_rounds< BiquadFilter<SumOfSines> >(filter, actual_output, rounds);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            sample_count,
            tolerance,
            "channel=%d, filter_type=%d, random=%f, frequency=%f, q=%f, gain=%f",
            (int)c,
            (int)filter_type,
            random,
            frequency,
            q,
            gain
        );
    }
}


void test_inaccurate_filter(
        BiquadFilter<SumOfSines>::Type const filter_type,
        Frequency const frequency,
        Number const q,
        Number const gain,
        SumOfSinesAmplitudes const& input_amplitudes,
        SumOfSinesFrequencies const& input_frequencies,
        SumOfSinesAmplitudes const& expected_amplitudes,
        SumOfSinesFrequencies const& expected_frequencies,
        Number const phase_offset,
        Number const tolerance
) {
    for (Number random = 0.0; random < 1.0; random += 0.00099) {
        test_inaccurate_filter_with_random(
            random,
            filter_type,
            frequency,
            q,
            gain,
            input_amplitudes,
            input_frequencies,
            expected_amplitudes,
            expected_frequencies,
            phase_offset,
            tolerance
        );
    }
}


TEST(filter_inaccuracy, {
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::LOW_PASS,
        2500.0,
        1.0,
        0.0,
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        {   0.50,    0.00,    0.00},
        { 500.00,    0.00, 5000.00},
        -0.0000968,
        0.122
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::HIGH_PASS,
        2500.0,
        1.0,
        0.0,
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        {   0.00,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        -0.0001875,
        0.12
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::BAND_PASS,
        2500.0,
        5.0,
        0.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.00,    0.33,    0.00},
        { 500.00, 2500.00, 5000.00},
        0.002006,
        0.177
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::NOTCH,
        2500.0,
        1.0,
        0.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.33,    0.00,    0.33},
        { 500.00, 2500.00, 5000.00},
        0.0019945,
        0.161
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::PEAKING,
        2500.0,
        5.0,
        6.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.33,    0.66,    0.33},
        { 500.00, 2500.00, 5000.00},
        0.00000494331,
        0.201
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::LOW_SHELF,
        1000.0,
        1.0,
        Constants::BIQUAD_FILTER_GAIN_MIN,
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        {   0.00,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        -0.000165,
        0.061
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::HIGH_SHELF,
        3000.0,
        1.0,
        Constants::BIQUAD_FILTER_GAIN_MIN,
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        {   0.33,    0.00,    0.00},
        { 500.00,    0.00, 5000.00},
        -0.00027838,
        0.06
    );

    Frequency const freq_min = Constants::BIQUAD_FILTER_FREQUENCY_MIN + 0.1;
    Number const q_min = Constants::BIQUAD_FILTER_Q_MIN + 0.001;

    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::LOW_PASS,
        freq_min,
        q_min,
        0.0,
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        {   0.00,    0.00,    0.00},
        { 500.00,    0.00, 5000.00},
        -0.0000968,
        0.01
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::HIGH_PASS,
        freq_min,
        q_min,
        0.0,
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        0.0000039257,
        0.05
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::BAND_PASS,
        freq_min,
        q_min,
        0.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.33,    0.15,    0.05},
        { 500.00, 2500.00, 5000.00},
        -0.0000653515,
        0.06
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::NOTCH,
        freq_min,
        q_min,
        0.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.07,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        0.0020172,
        0.1
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::PEAKING,
        freq_min,
        q_min,
        6.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.66,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        -0.0000086621,
        0.11
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::LOW_SHELF,
        freq_min,
        q_min,
        Constants::BIQUAD_FILTER_GAIN_MIN,
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        0.0000073356,
        0.05
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::HIGH_SHELF,
        freq_min,
        q_min,
        Constants::BIQUAD_FILTER_GAIN_MIN,
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        {   0.00,    0.00,    0.00},
        { 500.00,    0.00, 5000.00},
        0.0000073356,
        0.05
    );

    Frequency const freq_max = SAMPLE_RATE * 0.999 / 2.0;
    Number const q_max = Constants::BIQUAD_FILTER_Q_MAX * 0.999;

    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::LOW_PASS,
        freq_max,
        q_max,
        0.0,
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        0.0000075084,
        0.08

    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::HIGH_PASS,
        freq_max,
        q_max,
        0.0,
        {   0.50,    0.00,    0.50},
        { 500.00,    0.00, 5000.00},
        {   0.00,    0.00,    0.00},
        { 500.00,    0.00, 5000.00},
        0.0,
        0.01
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::BAND_PASS,
        freq_max,
        q_max,
        0.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.00,    0.00,    0.00},
        { 500.00, 2500.00, 5000.00},
        0.0,
        0.01
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::NOTCH,
        freq_max,
        q_max,
        0.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        0.00000267574,
        0.02
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::PEAKING,
        freq_max,
        q_max,
        6.0,
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        {   0.33,    0.33,    0.33},
        { 500.00, 2500.00, 5000.00},
        0.0,
        0.01
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::LOW_SHELF,
        freq_max,
        q_max,
        Constants::BIQUAD_FILTER_GAIN_MIN,
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        {   0.00,    0.00,    0.00},
        { 500.00,    0.00, 5000.00},
        0.0,
        0.01
    );
    test_inaccurate_filter(
        BiquadFilter<SumOfSines>::HIGH_SHELF,
        freq_max,
        q_max,
        Constants::BIQUAD_FILTER_GAIN_MIN,
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        {   0.33,    0.00,    0.33},
        { 500.00,    0.00, 5000.00},
        0.0,
        0.087
    );
})
