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

#include <vector>

#include "js80p.hpp"

#include "synth/envelope.cpp"
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


constexpr Frequency SAMPLE_RATE = 11025.0;
constexpr Integer BLOCK_SIZE = 2048;
constexpr Integer CHANNELS = 1;

constexpr Toggle OFF = ToggleParam::OFF;
constexpr Toggle ON = ToggleParam::ON;


void test_lfo(
        Toggle const tempo_sync,
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
    FloatParam param("F", -3.0, 7.0, 0.0);
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
    render_rounds<FloatParam>(param, actual_output, rounds);

    assert_true(lfo.is_on());

    assert_eq(
        expected_output.samples[0], actual_output.samples[0], sample_count, 0.001
    );
})
