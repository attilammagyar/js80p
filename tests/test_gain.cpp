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

#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/gain.cpp"
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


constexpr Integer CHANNELS = 2;


TEST(multiplies_input_signals_by_the_value_of_the_gain_parameter, {
    constexpr Integer rounds = 2;
    constexpr Integer block_size = 5;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[CHANNELS][block_size] = {
        {0.01, 0.02, 0.03, 0.04, 0.05},
        {0.02, 0.04, 0.06, 0.08, 0.10},
    };
    constexpr Sample expected_output[CHANNELS][sample_count] = {
        {0.10, 0.20, 0.30, 0.40, 0.50, 0.02, 0.04, 0.06, 0.08, 0.10},
        {0.20, 0.40, 0.60, 0.80, 1.00, 0.04, 0.08, 0.12, 0.16, 0.20},
    };
    Sample const* input_buffer[CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    FloatParamS gain_param("", 0.0, 20.0, 0.12345);
    Gain<FixedSignalProducer> gain(input, gain_param);
    Buffer actual_output(sample_count, CHANNELS);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    gain_param.set_sample_rate(sample_rate);
    gain_param.set_block_size(block_size);

    gain.set_sample_rate(sample_rate);
    gain.set_block_size(block_size);

    gain_param.set_value(10.0);
    gain_param.schedule_value(0.45, 2.0);

    render_rounds< Gain<FixedSignalProducer> >(gain, actual_output, rounds);

    assert_eq(
        expected_output[0], actual_output.samples[0], sample_count, DOUBLE_DELTA
    );
    assert_eq(
        expected_output[1], actual_output.samples[1], sample_count, DOUBLE_DELTA
    );
})
