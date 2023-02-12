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

#include "synth/envelope.cpp"
#include "synth/filter.cpp"
#include "synth/math.cpp"
#include "synth/midi_controller.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"
#include "synth/wavefolder.cpp"


using namespace JS80P;


typedef Wavefolder<SumOfSines> Wavefolder_;


constexpr Frequency SAMPLE_RATE = 44100.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Integer CHANNELS = 3;
constexpr Integer BLOCK_SIZE = 1024;
constexpr Number BLOCK_LENGTH = (Number)BLOCK_SIZE / SAMPLE_RATE;
constexpr Integer ROUNDS = 20;
constexpr Integer SAMPLE_COUNT = BLOCK_SIZE * ROUNDS;


TEST(when_folding_level_is_below_the_transition_threshold_then_no_folding_happens, {
    SumOfSines input(1.0, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    Wavefolder_ folder(input);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    folder.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    folder.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);

    folder.folding.set_value(0.0);
    folder.folding.schedule_value(BLOCK_LENGTH * 2.5, 0.0);
    folder.folding.schedule_linear_ramp(
        3.0 * BLOCK_LENGTH, Constants::FOLD_TRANSITION
    );

    render_rounds<SumOfSines>(input, expected_output, ROUNDS);
    input.reset();
    render_rounds<Wavefolder_>(folder, actual_output, ROUNDS);

    assert_eq(
        Constants::FOLD_TRANSITION, folder.folding.get_value(), DOUBLE_DELTA
    );

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            SAMPLE_COUNT,
            0.05,
            "channel=%d",
            (int)c
        );
    }
})


void naive_fold(Number const folding, Buffer &buffer)
{
    for (Integer c = 0; c != CHANNELS; ++c) {
        for (Integer i = 0; i != SAMPLE_COUNT; ++i) {
            buffer.samples[c][i] *= folding;

            while (std::fabs(buffer.samples[c][i]) > 1.0) {
                buffer.samples[c][i] = (
                    (buffer.samples[c][i] < 0.0 ? -2.0 : 2.0)
                    - buffer.samples[c][i]
                );
            }
        }
    }
}


TEST(when_folding_level_is_above_the_transition_threshold_then_the_signal_is_amplified_and_folded, {
    Sample const folding = (
        1.0 + (Sample)(Constants::FOLD_MAX - Constants::FOLD_TRANSITION)
    );
    SumOfSines input(1.0, 110.0, 0.0, 0.0, 0.0, 0.0, CHANNELS);
    Wavefolder_ folder(input);
    Buffer expected_output(SAMPLE_COUNT, CHANNELS);
    Buffer actual_output(SAMPLE_COUNT, CHANNELS);

    folder.set_block_size(BLOCK_SIZE);
    input.set_block_size(BLOCK_SIZE);

    folder.set_sample_rate(SAMPLE_RATE);
    input.set_sample_rate(SAMPLE_RATE);

    folder.folding.set_value(Constants::FOLD_MAX * 0.99);
    folder.folding.schedule_value(
        BLOCK_LENGTH * 2.5, Constants::FOLD_MAX * 0.99
    );
    folder.folding.schedule_linear_ramp(3.0 * BLOCK_LENGTH, Constants::FOLD_MAX);

    render_rounds<SumOfSines>(input, expected_output, ROUNDS);
    input.reset();
    render_rounds<Wavefolder_>(folder, actual_output, ROUNDS);

    naive_fold(folding, expected_output);

    assert_eq(Constants::FOLD_MAX, folder.folding.get_value(), DOUBLE_DELTA);

    for (Integer c = 0; c != CHANNELS; ++c) {
        assert_close(
            expected_output.samples[c],
            actual_output.samples[c],
            SAMPLE_COUNT,
            0.05,
            "channel=%d",
            (int)c
        );
    }
})
