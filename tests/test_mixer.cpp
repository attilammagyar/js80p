/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#include "dsp/mixer.cpp"
#include "dsp/signal_producer.cpp"


using namespace JS80P;


constexpr Integer CHANNELS = 2;


TEST(renders_and_sums_positive_weight_input_signals, {
    constexpr Integer block_size = 5;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples_1[CHANNELS][block_size] = {
        {0.20, 0.40, 0.60, 0.80, 1.00},
        {0.40, 0.80, 1.20, 1.60, 2.00},
    };
    constexpr Sample input_samples_2[CHANNELS][block_size] = {
        {0.01, 0.02, 0.03, 0.04, 0.05},
        {0.02, 0.04, 0.06, 0.08, 0.10},
    };
    constexpr Sample input_samples_3[CHANNELS][block_size] = {
        {9.09, 9.09, 9.09, 9.09, 9.09},
        {9.90, 9.90, 9.90, 9.90, 9.90},
    };
    constexpr Sample expected_output[CHANNELS][block_size] = {
        {0.11, 0.22, 0.33, 0.44, 0.55},
        {0.22, 0.44, 0.66, 0.88, 1.10},
    };
    Sample const* const input_buffer_1[CHANNELS] = {
        (Sample const*)&input_samples_1[0],
        (Sample const*)&input_samples_1[1]
    };
    Sample const* const input_buffer_2[CHANNELS] = {
        (Sample const*)&input_samples_2[0],
        (Sample const*)&input_samples_2[1]
    };
    Sample const* const input_buffer_3[CHANNELS] = {
        (Sample const*)&input_samples_3[0],
        (Sample const*)&input_samples_3[1]
    };
    FixedSignalProducer input_1(input_buffer_1);
    FixedSignalProducer input_2(input_buffer_2);
    FixedSignalProducer input_3(input_buffer_3);
    Mixer<FixedSignalProducer> mixer(CHANNELS);
    Sample const* const* rendered;

    mixer.add(input_1);
    mixer.add(input_2);
    mixer.add(input_3);

    mixer.set_weight(0, 0.5);
    mixer.set_weight(2, -0.1);
    mixer.set_weight(9, 9.99);

    input_1.set_sample_rate(sample_rate);
    input_1.set_block_size(block_size);

    input_2.set_sample_rate(sample_rate);
    input_2.set_block_size(block_size);

    input_3.set_sample_rate(sample_rate);
    input_3.set_block_size(block_size);

    mixer.set_sample_rate(sample_rate);
    mixer.set_block_size(block_size);

    rendered = SignalProducer::produce< Mixer<FixedSignalProducer> >(mixer, 1);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            rendered[c],
            block_size,
            DOUBLE_DELTA,
            "channel=%d",
            (int)c
        );
    }

    assert_neq(1, (int)input_3.get_cached_round());
})
