/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025  Attila M. Magyar
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

#include "dsp/compressor.cpp"
#include "dsp/effect.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/peak_tracker.cpp"
#include "dsp/queue.cpp"
#include "dsp/side_chain_compressable_effect.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 1024;
constexpr Number BLOCK_LENGTH = (Number)BLOCK_SIZE / SAMPLE_RATE;
constexpr Integer ROUNDS = 20;
constexpr Integer SAMPLE_COUNT = BLOCK_SIZE * ROUNDS;

constexpr CompressionCurve CC_LINEAR = CompressionCurve::COMPRESSION_CURVE_LINEAR;
constexpr CompressionCurve CC_SMOOTH = CompressionCurve::COMPRESSION_CURVE_SMOOTH;

constexpr CompressionMode CM_COMP = CompressionMode::COMPRESSION_MODE_COMPRESSOR;
constexpr CompressionMode CM_EXPAND = CompressionMode::COMPRESSION_MODE_EXPANDER;


template<CompressionCurve curve>
void test_compressor(
        CompressionMode const mode,
        Number const input_level,
        Number const threshold,
        Number const ratio,
        Number const makeup_gain,
        Number const wet,
        Number const dry,
        Number const expected_output_level
) {
    typedef Compressor<SumOfSines, curve> Compressor_;

    SumOfSines input(input_level, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    SumOfSines expected_output_generator(
        expected_output_level, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS
    );
    Compressor_ compressor("C", input, NULL, makeup_gain);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    compressor.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);
    expected_output_generator.set_block_size(BLOCK_SIZE);

    compressor.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);
    expected_output_generator.set_sample_rate(SAMPLE_RATE);

    compressor.mode.set_value((Byte)mode);
    compressor.threshold.set_value(threshold);
    compressor.attack_time.set_value(0.001);
    compressor.release_time.set_value(0.001);
    compressor.ratio.set_value(ratio);
    compressor.dry.set_value(dry);
    compressor.wet.set_value(wet);

    render_rounds<Compressor_>(compressor, actual_output, ROUNDS);
    render_rounds<SumOfSines>(expected_output_generator, expected_output, ROUNDS);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            SAMPLE_COUNT,
            0.02,
            (
                "curve=%d, mode=%d, input_level=%f, threshold=%f, ratio=%f,"
                " makeup_gain=%f, wet=%f, dry=%f, expected_output_level=%f,"
                " channel=%d"
            ),
            (int)curve,
            (int)mode,
            input_level,
            threshold,
            ratio,
            makeup_gain,
            wet,
            dry,
            expected_output_level,
            (int)c
        );
    }
}


template<CompressionCurve curve>
void test_compressor()
{
    /*
    Rule of thumb: subtracting 6 dB is the same as multiplying by 0.5, and
    adding 6 dB is the same as multiplying by 2.
    */

    test_compressor<curve>(CM_COMP, 1.00,  -6.0,   1.0, 1.0, 1.00, 0.00, 1.00);
    test_compressor<curve>(CM_COMP, 1.00,  -6.0,   1.0, 1.0, 0.99, 0.01, 1.00);
    test_compressor<curve>(CM_COMP, 1.00,  -6.0,   1.0, 1.0, 0.00, 1.00, 1.00);

    test_compressor<curve>(CM_COMP, 0.50,  -6.0, 120.0, 1.0, 1.00, 0.00, 0.50);
    test_compressor<curve>(CM_COMP, 0.50,  -6.0, 120.0, 1.0, 0.99, 0.01, 0.50);
    test_compressor<curve>(CM_COMP, 0.50,  -6.0, 120.0, 1.0, 0.00, 1.00, 0.50);

    test_compressor<curve>(CM_COMP, 1.00,  -6.0, 120.0, 1.0, 1.00, 0.00, 0.50);
    test_compressor<curve>(CM_COMP, 1.00,  -6.0, 120.0, 1.0, 0.99, 0.01, 0.50);

    test_compressor<curve>(CM_COMP, 1.00, -18.0,   3.0, 1.0, 1.00, 0.00, 0.25);
    test_compressor<curve>(CM_COMP, 1.00, -18.0,   3.0, 1.0, 0.99, 0.01, 0.25);

    test_compressor<curve>(CM_COMP, 1.00, -18.0,   3.0, 2.0, 1.00, 0.00, 0.50);
    test_compressor<curve>(CM_COMP, 1.00, -18.0,   3.0, 2.0, 0.99, 0.01, 0.50);

    test_compressor<curve>(CM_COMP, 0.30,  -6.0, 120.0, 1.0, 1.00, 0.00, 0.30);
    test_compressor<curve>(CM_COMP, 0.30,  -6.0, 120.0, 1.0, 0.99, 0.01, 0.30);

    test_compressor<curve>(CM_COMP, 0.00,  -6.0, 120.0, 1.0, 1.00, 0.00, 0.00);
    test_compressor<curve>(CM_COMP, 0.00,  -6.0, 120.0, 1.0, 0.99, 0.01, 0.00);
}


TEST(when_compressor_mode_is_selected_then_signals_above_the_threshold_are_compressed, {
    test_compressor<CC_LINEAR>();
    test_compressor<CC_SMOOTH>();
})


template<CompressionCurve curve>
void test_expand()
{
    /*
    Rule of thumb: subtracting 6 dB is the same as multiplying by 0.5, and
    adding 6 dB is the same as multiplying by 2.
    */

    test_compressor<curve>(CM_EXPAND, 1.00, -6.0,   1.0, 1.0, 1.00, 0.00, 1.00);
    test_compressor<curve>(CM_EXPAND, 1.00, -6.0,   1.0, 1.0, 0.99, 0.01, 1.00);
    test_compressor<curve>(CM_EXPAND, 1.00, -6.0,   1.0, 1.0, 0.00, 1.00, 1.00);

    test_compressor<curve>(CM_EXPAND, 0.50, -6.1, 120.0, 1.0, 1.00, 0.00, 0.50);
    test_compressor<curve>(CM_EXPAND, 0.50, -6.1, 120.0, 1.0, 0.99, 0.01, 0.50);
    test_compressor<curve>(CM_EXPAND, 0.50, -6.1, 120.0, 1.0, 0.00, 1.00, 0.50);

    test_compressor<curve>(CM_EXPAND, 1.00, -6.0, 120.0, 1.0, 1.00, 0.00, 1.00);
    test_compressor<curve>(CM_EXPAND, 1.00, -6.0, 120.0, 1.0, 0.99, 0.01, 1.00);

    test_compressor<curve>(CM_EXPAND, 0.30, -6.0, 120.0, 1.0, 1.00, 0.00, 0.00);
    test_compressor<curve>(CM_EXPAND, 0.30, -6.0, 120.0, 1.0, 0.99, 0.01, 0.00);

    test_compressor<curve>(CM_EXPAND, 0.50, -3.0,   3.0, 1.0, 1.00, 0.00, 0.25);
    test_compressor<curve>(CM_EXPAND, 0.50, -3.0,   3.0, 1.0, 0.99, 0.01, 0.25);

    /* Expansion does not require make-up gain though. */
    test_compressor<curve>(CM_EXPAND, 0.50, -3.0,   3.0, 2.0, 1.00, 0.00, 0.50);
    test_compressor<curve>(CM_EXPAND, 0.50, -3.0,   3.0, 2.0, 0.99, 0.01, 0.50);

    test_compressor<curve>(CM_EXPAND, 0.00, -6.0, 120.0, 1.0, 1.00, 0.00, 0.00);
    test_compressor<curve>(CM_EXPAND, 0.00, -6.0, 120.0, 1.0, 0.99, 0.01, 0.00);
}


TEST(when_expand_mode_is_selected_then_signals_below_the_threshold_are_compressed, {
    test_expand<CC_LINEAR>();
    test_expand<CC_SMOOTH>();
})
