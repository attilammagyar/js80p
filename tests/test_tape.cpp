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
#include <array>
#include <cmath>
#include <cstddef>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/biquad_filter.cpp"
#include "dsp/delay.cpp"
#include "dsp/distortion.cpp"
#include "dsp/effect.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/noise_generator.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/tape.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


constexpr Integer BLOCK_SIZE = 10;

constexpr TapeParams::State TS_INIT = TapeParams::State::TAPE_STATE_INIT;
constexpr TapeParams::State TS_NORMAL = TapeParams::State::TAPE_STATE_NORMAL;
constexpr TapeParams::State TS_STOPPING = TapeParams::State::TAPE_STATE_STOPPING;
constexpr TapeParams::State TS_STOPPED = TapeParams::State::TAPE_STATE_STOPPED;
constexpr TapeParams::State TS_STARTABLE = TapeParams::State::TAPE_STATE_STARTABLE;
constexpr TapeParams::State TS_STARTING = TapeParams::State::TAPE_STATE_STARTING;
constexpr TapeParams::State TS_STARTED = TapeParams::State::TAPE_STATE_STARTED;
constexpr TapeParams::State TS_FF_STARTABLE = TapeParams::State::TAPE_STATE_FF_STARTABLE;
constexpr TapeParams::State TS_FF_STARTING = TapeParams::State::TAPE_STATE_FF_STARTING;
constexpr TapeParams::State TS_FF_STARTED = TapeParams::State::TAPE_STATE_FF_STARTED;

typedef bool TapeStopTestStepSoundExpectation;
typedef bool TapeStopTestStepParamRampingExpectation;

constexpr TapeStopTestStepSoundExpectation EXPECT_SILENCE = true;
constexpr TapeStopTestStepSoundExpectation EXPECT_SOUND = false;

constexpr TapeStopTestStepParamRampingExpectation EXPECT_RAMPING = true;
constexpr TapeStopTestStepParamRampingExpectation EXPECT_CONST = false;


Math::RNG rng(123);


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
    Tape<FixedSignalProducer, ToggleParam::OFF> tape_off("F", params, input, rng);
    Tape<FixedSignalProducer, ToggleParam::ON> tape_on("N", params, input, rng);
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


class TapeStopTestStep
{
    public:
        TapeStopTestStep(
                Number const stop_start_value,
                Integer const samples_to_render,
                TapeStopTestStepSoundExpectation const output_expectation,
                TapeStopTestStepParamRampingExpectation const volume_param_expectation,
                TapeStopTestStepParamRampingExpectation const delay_time_param_expectation,
                TapeParams::State const expected_state
        ) : stop_start_value(stop_start_value),
            samples_to_render(samples_to_render),
            output_expectation(output_expectation),
            volume_param_expectation(volume_param_expectation),
            delay_time_param_expectation(delay_time_param_expectation),
            expected_state(expected_state)
        {
        }

        Number const stop_start_value;
        Integer const samples_to_render;
        TapeStopTestStepSoundExpectation const output_expectation;
        TapeStopTestStepParamRampingExpectation const volume_param_expectation;
        TapeStopTestStepParamRampingExpectation const delay_time_param_expectation;
        TapeParams::State const expected_state;
};


template<size_t step_count>
void test_tape_stop(std::array<TapeStopTestStep, step_count> const& steps)
{
    constexpr Frequency sample_rate = 1.0;

    Sample input_channel[BLOCK_SIZE] = {
        0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
    };
    Sample silence[BLOCK_SIZE] = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    };
    Sample* input_channels[FixedSignalProducer::CHANNELS] = {
        input_channel, input_channel
    };
    FixedSignalProducer input(input_channels);
    ToggleParam toggle_param("toggle", ToggleParam::ON);
    TapeParams tape_params("tape_params", toggle_param);
    Tape<FixedSignalProducer, ToggleParam::ON> tape(
        "tape", tape_params, input, rng
    );
    Sample const* const* rendered = NULL;
    size_t i = 0;
    SignalProducer* sp = tape_params.get_signal_producer(i++);

    while (sp != NULL) {
        sp->set_sample_rate(sample_rate);
        sp->set_block_size(BLOCK_SIZE);
        sp = tape_params.get_signal_producer(i++);
    }

    input.set_sample_rate(sample_rate);
    input.set_block_size(BLOCK_SIZE);

    toggle_param.set_sample_rate(sample_rate);
    toggle_param.set_block_size(BLOCK_SIZE);

    tape.set_sample_rate(sample_rate);
    tape.set_block_size(BLOCK_SIZE);

    tape_params.distortion_level.set_value(0.001);

    for (i = 0; i != step_count; ++i) {
        TapeStopTestStep const& step = steps[i];

        tape_params.stop_start.set_value(step.stop_start_value);

        rendered = SignalProducer::produce< Tape<FixedSignalProducer, ToggleParam::ON> >(
            tape, (Integer)i, step.samples_to_render
        );

        if (step.output_expectation == EXPECT_SILENCE) {
            for (Integer c = 0; c != tape.get_channels(); ++c) {
                assert_eq(
                    silence,
                    rendered[c],
                    step.samples_to_render,
                    DOUBLE_DELTA,
                    "step=%u, channel=%d",
                    (unsigned int)i,
                    (int)c
                );
            }
        } else {
            for (Integer c = 0; c != tape.get_channels(); ++c) {
                assert_lt(
                    silence,
                    rendered[c],
                    step.samples_to_render,
                    "step=%u, channel=%d",
                    (unsigned int)i,
                    (int)c
                );
            }
        }

        if (step.volume_param_expectation == EXPECT_RAMPING) {
            assert_true(tape_params.volume.is_ramping(), "step=%u", (unsigned int)i);
        } else {
            assert_false(tape_params.volume.is_ramping(), "step=%u", (unsigned int)i);
        }

        if (step.delay_time_param_expectation == EXPECT_RAMPING) {
            assert_true(tape_params.delay_time_lfo.min.is_ramping(), "step=%u", (unsigned int)i);
            assert_true(tape_params.delay_time_lfo.max.is_ramping(), "step=%u", (unsigned int)i);
        } else {
            assert_false(tape_params.delay_time_lfo.min.is_ramping(), "step=%u", (unsigned int)i);
            assert_false(tape_params.delay_time_lfo.max.is_ramping(), "step=%u", (unsigned int)i);
        }

        assert_eq(step.expected_state, tape_params.state, "step=%u", (unsigned int)i);
    }
}


TEST(stopping_time_must_be_set_to_zero_before_first_use, {
    test_tape_stop<5>(
        {
            TapeStopTestStep(1.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_INIT),
            TapeStopTestStep(2.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_INIT),
            TapeStopTestStep(3.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_INIT),
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(5.1, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
        }
    );
})


TEST(increasing_the_stop_time_during_stopping_schedules_new_stop, {
    test_tape_stop<6>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(6.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(9.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(9.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(9.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(9.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
        }
    );
})


TEST(decreasing_the_stop_time_during_stopping_schedules_fast_forward_start, {
    test_tape_stop<5>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(6.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(5.0, 1, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_FF_STARTING),
            TapeStopTestStep(3.0, 1, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(2.0, 1, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTED),
        }
    );
})


TEST(stopped_tape_remains_stopped_while_stop_time_is_decreasing, {
    test_tape_stop<6>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(4.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(3.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(2.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
        }
    );
})


TEST(stopped_tape_can_be_fast_forwarded_by_setting_zero_stop_start_time_then_turning_it_up, {
    test_tape_stop<8>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(4.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(0.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTABLE),
            TapeStopTestStep(0.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTABLE),
            TapeStopTestStep(5.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(5.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_FF_STARTING),
        }
    );
})


TEST(stopped_tape_can_be_quickly_started_by_increasing_the_stop_start_time, {
    test_tape_stop<8>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(4.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(5.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STARTABLE),
            TapeStopTestStep(5.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STARTING),
            TapeStopTestStep(5.0, 1, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_STARTING),
            TapeStopTestStep(5.0, 1, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_STARTED),
        }
    );
})


TEST(stopped_and_quickly_started_tape_waits_for_a_zero_stop_time_before_returning_to_normal_operation, {
    test_tape_stop<12>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(4.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(5.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STARTABLE),
            TapeStopTestStep(5.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STARTING),
            TapeStopTestStep(5.0, 1, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_STARTING),
            TapeStopTestStep(5.0, 1, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_STARTED),
            TapeStopTestStep(6.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_STARTED),
            TapeStopTestStep(3.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_STARTED),
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
        }
    );
})


TEST(fast_forwarding_tape_schedules_new_fast_forwarding_when_stop_start_time_is_changed, {
    test_tape_stop<11>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(4.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(0.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTABLE),
            TapeStopTestStep(0.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTABLE),
            TapeStopTestStep(2.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(5.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_FF_STARTING),
            TapeStopTestStep(5.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(3.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(3.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTED),
        }
    );
})


TEST(fast_forwarded_tape_waits_for_zero_start_stop_time_before_returning_to_normal_operation, {
    test_tape_stop<12>(
        {
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
            TapeStopTestStep(4.0, 5, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPING),
            TapeStopTestStep(4.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_STOPPED),
            TapeStopTestStep(0.0, 5, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTABLE),
            TapeStopTestStep(2.0, 1, EXPECT_SILENCE, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(3.0, 3, EXPECT_SOUND, EXPECT_RAMPING, EXPECT_RAMPING, TS_FF_STARTING),
            TapeStopTestStep(3.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTING),
            TapeStopTestStep(3.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTED),
            TapeStopTestStep(2.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTED),
            TapeStopTestStep(5.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_FF_STARTED),
            TapeStopTestStep(0.0, 5, EXPECT_SOUND, EXPECT_CONST, EXPECT_CONST, TS_NORMAL),
        }
    );
})
