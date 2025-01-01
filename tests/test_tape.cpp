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

#include "dsp/biquad_filter.cpp"
#include "dsp/delay.cpp"
#include "dsp/distortion.cpp"
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
#include "dsp/tape.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


constexpr Integer BLOCK_SIZE = 10;


TEST(when_bypass_toggle_value_is_matched_then_tape_is_engaged_otherwise_bypassed, {
    Sample input_channel[BLOCK_SIZE] = {
        0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9,
    };
    Sample distorted[BLOCK_SIZE] = {
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    };
    Sample* input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel, input_channel
    };
    FixedSignalProducer input(input_channels);
    ToggleParam toggle("B", ToggleParam::OFF);
    TapeParams params("T", toggle);
    Tape<FixedSignalProducer, ToggleParam::OFF> tape_off("F", params, input);
    Tape<FixedSignalProducer, ToggleParam::ON> tape_on("N", params, input);
    Sample const* const* rendered = NULL;

    toggle.set_value(ToggleParam::ON);
    params.stop_start.set_value(0.0);
    params.wnf_amp.set_value(0.001);
    params.distortion_level.set_value(1.0);
    params.distortion_type.set_value(Distortion::TYPE_TANH_10);
    params.color.set_value(0.8);
    params.hiss_level.set_value(0.001);

    rendered = SignalProducer::produce< Tape<FixedSignalProducer, ToggleParam::OFF> >(
        tape_off, 1, BLOCK_SIZE
    );
    assert_eq(
        input_channel,
        rendered[0],
        DOUBLE_DELTA,
        "tape=toggle-off, toggle=on, channel=0"
    );
    assert_eq(
        input_channel,
        rendered[1],
        DOUBLE_DELTA,
        "tape=toggle-off, toggle=on, channel=1"
    );

    rendered = SignalProducer::produce< Tape<FixedSignalProducer, ToggleParam::ON> >(
        tape_on, 1, BLOCK_SIZE
    );
    assert_eq(
        distorted,
        rendered[0],
        DOUBLE_DELTA,
        "tape=toggle-on, toggle=on, channel=0"
    );
    assert_eq(
        distorted,
        rendered[1],
        DOUBLE_DELTA,
        "tape=toggle-on, toggle=on, channel=1"
    );

    toggle.set_value(ToggleParam::OFF);

    rendered = SignalProducer::produce< Tape<FixedSignalProducer, ToggleParam::OFF> >(
        tape_off, 2, BLOCK_SIZE
    );
    assert_eq(
        distorted,
        rendered[0],
        DOUBLE_DELTA,
        "tape=toggle-off, toggle=off, channel=0"
    );
    assert_eq(
        distorted,
        rendered[1],
        DOUBLE_DELTA,
        "tape=toggle-off, toggle=off, channel=1"
    );

    rendered = SignalProducer::produce< Tape<FixedSignalProducer, ToggleParam::ON> >(
        tape_on, 2, BLOCK_SIZE
    );
    assert_eq(
        input_channel,
        rendered[0],
        DOUBLE_DELTA,
        "tape=toggle-on, toggle=off, channel=0"
    );
    assert_eq(
        input_channel,
        rendered[1],
        DOUBLE_DELTA,
        "tape=toggle-on, toggle=off, channel=1"
    );
})
