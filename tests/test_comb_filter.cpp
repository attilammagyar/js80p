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
#include "synth/comb_filter.cpp"
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


template<class CombFilterClass>
void test_comb_filter_panning()
{
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 2;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Frequency sample_rate = 10.0;
    constexpr Sample input_samples[FixedSignalProducer::CHANNELS][block_size] = {
        {0.10, 0.20, 0.30, 0.40, 0.50},
        {0.20, 0.40, 0.60, 0.80, 1.00},
    };
    constexpr Sample expected_output[FixedSignalProducer::CHANNELS][sample_count] = {
        {0.000, 0.000, 0.075, 0.150, 0.225, 0.000, 0.000, 0.000, 0.000, 0.000},
        {0.000, 0.000, 0.150, 0.300, 0.450, 0.900, 1.125, 0.225, 0.450, 0.675},
    };
    Sample const* input_buffer[FixedSignalProducer::CHANNELS] = {
        (Sample const*)&input_samples[0],
        (Sample const*)&input_samples[1]
    };
    FixedSignalProducer input(input_buffer);
    Buffer output(sample_count, FixedSignalProducer::CHANNELS);
    CombFilterClass comb_filter(input, CombFilterStereoMode::FLIPPED);

    input.set_sample_rate(sample_rate);
    input.set_block_size(block_size);

    comb_filter.set_sample_rate(sample_rate);
    comb_filter.set_block_size(block_size);
    comb_filter.delay.gain.set_value(0.75);
    comb_filter.delay.time.set_value(0.2);
    comb_filter.panning.set_value(0.0);
    comb_filter.panning.schedule_value(0.45, -1.0);

    assert_eq((int)input.get_channels(), (int)comb_filter.get_channels());

    render_rounds<CombFilterClass>(comb_filter, output, rounds);

    for (Integer c = 0; c != FixedSignalProducer::CHANNELS; ++c) {
        assert_eq(
            expected_output[c],
            output.samples[c],
            sample_count,
            DOUBLE_DELTA,
            "channel=%d",
            (int)c
        );
    }

    assert_eq(-1.0, comb_filter.panning.get_value(), DOUBLE_DELTA);
}


TEST(output_may_be_panned, {
    test_comb_filter_panning< HighShelfPannedCombFilter<FixedSignalProducer> >();
    test_comb_filter_panning< PannedCombFilter<FixedSignalProducer> >();
})
