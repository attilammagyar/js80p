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
#include "dsp/flexible_controller.cpp"
#include "dsp/lfo.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


TEST(param_stores_basic_properties, {
    constexpr Integer block_size = 8;
    constexpr Sample expected_samples[] = {
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5
    };
    Param<double> param("param", -1.0, 1.0, 0.25);
    Sample const* const* rendered_samples;

    param.set_block_size(block_size);
    param.set_sample_rate(1.0);

    assert_eq("param", param.get_name());
    assert_eq(-1.0, param.get_min_value(), DOUBLE_DELTA);
    assert_eq(1.0, param.get_max_value(), DOUBLE_DELTA);
    assert_eq(0.25, param.get_default_value(), DOUBLE_DELTA);
    assert_eq(0.25, param.get_value(), DOUBLE_DELTA);

    Integer const change_index_before = param.get_change_index();
    param.set_value(0.5);
    Integer const change_index_after = param.get_change_index();

    rendered_samples = SignalProducer::produce< Param<double> >(param, 1);

    assert_eq(0.5, param.get_value(), DOUBLE_DELTA);
    assert_eq(0.25, param.get_default_value(), DOUBLE_DELTA);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_neq((int)change_index_before, (int)change_index_after);
})


TEST(param_clamps_float_value_to_be_between_min_and_max, {
    Param<double> param("param", -1.0, 1.0, 0.0);

    assert_eq(0.0, param.get_default_value(), DOUBLE_DELTA);
    assert_eq(0.0, param.get_value(), DOUBLE_DELTA);

    param.set_value(2.0);
    assert_eq(1.0, param.get_value());

    param.set_value(-2.0);
    assert_eq(-1.0, param.get_value());
})


TEST(param_clamps_ratio_value_to_be_between_min_and_max, {
    Param<int> param("param", -100, 100, 0);

    assert_eq(0, param.get_default_value());
    assert_eq(0, param.get_value());
    assert_eq(0.5, param.get_ratio(), DOUBLE_DELTA);
    assert_eq(0.5, param.get_default_ratio(), DOUBLE_DELTA);

    param.set_value(50);
    assert_eq(0.75, param.get_ratio(), DOUBLE_DELTA);

    param.set_ratio(0.25);
    assert_eq(-50, param.get_value());

    param.set_ratio(2.0);
    assert_eq(100, param.get_value());
    assert_eq(1.0, param.get_ratio(), DOUBLE_DELTA);

    param.set_ratio(-2.0);
    assert_eq(-100, param.get_value());
    assert_eq(0.0, param.get_ratio(), DOUBLE_DELTA);
})


TEST(param_can_convert_between_value_and_ratio, {
    for (int max = 1; max != 1000; ++max) {
        Param<int> int_param("int_param", 0, max, 0);
        int i;

        assert_eq(0, int_param.ratio_to_value(0.0));

        for (i = 0; i != max + 1; ++i) {
            assert_eq(
                i,
                int_param.ratio_to_value(int_param.value_to_ratio(i)),
                "max=%d",
                max
            );
        }

        assert_eq(
            max, int_param.ratio_to_value(int_param.value_to_ratio(max + 1))
        );
    }

    Param<double> double_param("double_param", -10.0, 10.0, 0.0);

    assert_eq(-10.0, double_param.ratio_to_value(0.0), DOUBLE_DELTA);
    assert_eq(-5.0, double_param.ratio_to_value(0.25), DOUBLE_DELTA);
    assert_eq(0.0, double_param.ratio_to_value(0.5), DOUBLE_DELTA);
    assert_eq(5.0, double_param.ratio_to_value(0.75), DOUBLE_DELTA);
    assert_eq(10.0, double_param.ratio_to_value(1.0), DOUBLE_DELTA);

    assert_eq(0.0, double_param.value_to_ratio(-10.0), DOUBLE_DELTA);
    assert_eq(0.25, double_param.value_to_ratio(-5.0), DOUBLE_DELTA);
    assert_eq(0.5, double_param.value_to_ratio(0.0), DOUBLE_DELTA);
    assert_eq(0.75, double_param.value_to_ratio(5.0), DOUBLE_DELTA);
    assert_eq(1.0, double_param.value_to_ratio(10.0), DOUBLE_DELTA);
})


class IntParam : public Param<int>
{
    public:
        IntParam(
            std::string const name,
            int const min_value,
            int const max_value,
            int const default_value
        ) noexcept : Param<int>(name, min_value, max_value, default_value)
        {
        }

        int ratio_to_value(Number const ratio) const noexcept
        {
            return Param<int>::ratio_to_value(ratio);
        }
};


TEST(param_clamps_integer_value_to_be_between_min_and_max, {
    IntParam param("param", -10, 10, 0);

    assert_eq(0, param.get_default_value());
    assert_eq(0, param.get_value());

    param.set_value(20);
    assert_eq(10, param.get_value());

    param.set_value(-20);
    assert_eq(-10, param.get_value());

    assert_eq(-10, param.ratio_to_value(-1.0));
    assert_eq(-10, param.ratio_to_value(0.0));
    assert_eq(-5, param.ratio_to_value(0.25));
    assert_eq(0, param.ratio_to_value(0.5));
    assert_eq(5, param.ratio_to_value(0.75));
    assert_eq(10, param.ratio_to_value(1.0));
    assert_eq(10, param.ratio_to_value(1.1));
    assert_eq(10, param.ratio_to_value(2.0));
})


TEST(when_a_midi_controller_is_assigned_to_a_param_then_the_params_value_follows_the_changes_of_the_midi_controller, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[] = {
        2, 2, 2, 2, 2,
    };
    Param<int> param("int", -10, 10, 0);
    MidiController midi_controller;
    Sample const* const* rendered_samples;
    Integer change_index_1;
    Integer change_index_2;

    midi_controller.change(0.0, 0.6);
    midi_controller.clear();

    param.set_block_size(block_size);
    param.set_sample_rate(1.0);
    param.set_midi_controller(&midi_controller);

    assert_eq((void*)&midi_controller, (void*)param.get_midi_controller());
    assert_eq(2, param.get_value());
    assert_eq(0.6, param.get_ratio());

    rendered_samples = SignalProducer::produce< Param<int> >(param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);

    change_index_1 = param.get_change_index();
    midi_controller.change(0.0, 0.2514);
    change_index_2 = param.get_change_index();

    assert_eq(-5, param.get_value());
    assert_eq(0.2514, param.get_ratio());

    assert_neq((int)change_index_1, (int)change_index_2);

    midi_controller.change(0.0, 0.35);
    param.set_midi_controller(NULL);
    assert_eq(-3, param.get_value());
})


void assert_float_param_does_not_change_during_rendering(
        FloatParam& float_param,
        Integer const round,
        Integer const chunk_size,
        Sample const** rendered_samples
) {
    Integer const change_index_before = float_param.get_change_index();
    *rendered_samples = (
        FloatParam::produce<FloatParam>(float_param, round, chunk_size)[0]
    );
    Integer const change_index_after = float_param.get_change_index();

    assert_eq((int)change_index_before, (int)change_index_after);
}


void assert_float_param_changes_during_rendering(
        FloatParam& float_param,
        Integer const round,
        Integer const chunk_size,
        Sample const** rendered_samples
) {
    Integer const change_index_before = float_param.get_change_index();
    *rendered_samples = (
        FloatParam::produce<FloatParam>(float_param, round, chunk_size)[0]
    );
    Integer const change_index_after = float_param.get_change_index();

    assert_neq((int)change_index_before, (int)change_index_after);
}


TEST(float_param_can_schedule_and_clamp_values, {
    constexpr Integer block_size = 5;
    Sample const* rendered_samples;
    constexpr Sample expected_samples[] = {0.5, 0.5, 1.0, 1.0, 1.0};
    FloatParam float_param("float", -1.0, 1.0, 0.0);

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);
    float_param.set_value(0.5);
    float_param.schedule_value(2.0, 1.1);

    assert_true(float_param.is_constant_until(2));
    assert_false(float_param.is_constant_until(block_size));
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    assert_float_param_changes_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_false(float_param.is_constant_in_next_round(1, block_size));
    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(float_param_can_tell_if_it_is_constant_through_the_next_round, {
    constexpr Integer block_size = 3;
    constexpr Sample expected_samples[3][block_size] = {
        {0.5, 0.5, 0.5},
        {0.5, 0.5, 1.0},
        {1.0, 1.0, 1.0},
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);
    float_param.set_value(0.5);
    float_param.schedule_value(5.0, 1.0);

    assert_true(float_param.is_constant_in_next_round(1, block_size));
    assert_float_param_does_not_change_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_true(float_param.is_constant_in_next_round(1, block_size));
    assert_eq(expected_samples[0], rendered_samples, block_size, DOUBLE_DELTA);

    assert_false(float_param.is_constant_in_next_round(2, block_size));
    assert_float_param_changes_during_rendering(
        float_param, 2, block_size, &rendered_samples
    );
    assert_false(float_param.is_constant_in_next_round(2, block_size));
    assert_eq(expected_samples[1], rendered_samples, block_size, DOUBLE_DELTA);

    assert_true(float_param.is_constant_in_next_round(3, block_size));
    assert_float_param_does_not_change_during_rendering(
        float_param, 3, block_size, &rendered_samples
    );
    assert_true(float_param.is_constant_in_next_round(3, block_size));
    assert_eq(expected_samples[2], rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(float_param_can_schedule_and_clamp_values_between_samples, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[] = {0.5, 0.5, 0.5, 1.0, 1.0};
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(3.0);
    float_param.set_value(0.5);
    float_param.schedule_value(0.75, 1.1);

    assert_true(float_param.is_constant_in_next_round(1, 3));
    assert_true(float_param.is_constant_until(3));
    assert_false(float_param.is_constant_until(4));

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(float_param_can_cancel_scheduled_value, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[] = {0.5, 0.5, 0.5, 0.5, 0.5};
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(0.5);
    float_param.schedule_value(2.0, 1.1);
    float_param.cancel_events_at(1.0);

    assert_float_param_does_not_change_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(float_param_can_cancel_scheduled_value_between_samples, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[] = {0.5, 0.5, 0.5, 0.5, 0.5};
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_sample_rate(2.0);
    float_param.set_value(0.5);
    float_param.schedule_value(1.0, 1.1);
    float_param.cancel_events_at(0.9);

    assert_float_param_does_not_change_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(float_param_can_round_set_and_scheduled_values, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[] = {10.0, 10.0, 10.0, 20.0, 20.0};
    FloatParam float_param("float", 0.0, 100.0, 0.0, 10.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(3.0);
    float_param.set_block_size(block_size);

    float_param.set_value(42.0);
    assert_eq(40.0, float_param.get_value(), DOUBLE_DELTA);

    float_param.set_ratio(0.12);
    assert_eq(10.0, float_param.get_value(), DOUBLE_DELTA);

    float_param.schedule_value(0.9, 19.0);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(float_param_can_schedule_linear_ramping_clamped_to_max_value, {
    constexpr Integer block_size = 20;
    constexpr Sample expected_samples[] = {
        -0.1, -0.1, -0.1, -0.1, 0.0,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.6, 0.7, 0.8, 0.9, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(-0.1);
    float_param.schedule_value(4.0, 0.0);
    float_param.schedule_linear_ramp(15.0, 1.5);

    assert_true(float_param.is_constant_until(4));
    assert_false(float_param.is_constant_until(5));
    assert_float_param_changes_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(float_param_can_schedule_linear_ramping_clamped_to_min_value, {
    constexpr Integer block_size = 20;
    constexpr Sample expected_samples[] = {
        0.1, 0.1, 0.1, 0.1, 0.0,
        -0.1, -0.2, -0.3, -0.4, -0.5,
        -0.6, -0.7, -0.8, -0.9, -1.0,
        -1.0, -1.0, -1.0, -1.0, -1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(0.1);
    float_param.schedule_value(4.0, 0.0);
    float_param.schedule_linear_ramp(15.0, -1.5);

    assert_float_param_changes_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(when_float_param_linear_ramping_is_canceled_then_last_calculated_value_is_held, {
    constexpr Integer block_size = 20;
    constexpr Sample expected_samples[] = {
        -0.1, -0.1, -0.1, -0.1, -0.1,
        0.0, 0.1, 0.2, 0.3, 0.4,
        0.5, 0.5, 0.5, 0.5, 0.5,
        0.5, 0.5, 0.5, 0.5, 0.5,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(-0.1);
    float_param.schedule_value(5.0, 0.0);
    float_param.schedule_linear_ramp(15.0, 1.5);
    float_param.cancel_events_at(10.0);

    assert_float_param_changes_during_rendering(
        float_param, 1, block_size, &rendered_samples
    );
    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(resetting_cancels_a_linear_ramping, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[] = {0.3, 0.3, 0.3, 0.3, 0.3};
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_block_size(block_size);
    float_param.set_value(-0.1);
    float_param.schedule_value(2.0, 0.0);
    float_param.schedule_linear_ramp(10.0, 1.0);

    FloatParam::produce<FloatParam>(float_param, 1);
    float_param.reset();
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 2)[0];

    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


TEST(float_param_linear_ramps_may_stretch_over_several_rendering_rounds, {
    constexpr Integer block_size = 3;
    constexpr Integer rounds = 7;
    constexpr Integer sample_count = block_size * rounds;
    constexpr Sample expected_samples[sample_count] = {
        -0.1, -0.1, -0.1,
        -0.1, 0.0, 0.1,
        0.2, 0.3, 0.4,
        0.5, 0.5, 0.5,
        0.5, 0.5, 0.5,
        0.5, 0.5, 0.5,
        0.5, 0.5, 0.5,
    };
    Integer next_sample_index = 0;
    Buffer rendered(sample_count);
    FloatParam float_param("float", -1.0, 1.0, 0.0);

    float_param.set_sample_rate(1.0);
    float_param.set_value(-0.1);
    float_param.schedule_value(4.0, 0.0);
    float_param.schedule_linear_ramp(15.0, 1.5);

    for (Integer round = 0; round != rounds; ++round) {
        assert_eq(
            round < 1 || round >= 4,
            float_param.is_constant_until(block_size),
            "round=%lld", (long long int)round
        );

        Sample const* const* const block = FloatParam::produce<FloatParam>(
            float_param, round, block_size
        );

        for (Integer i = 0; i != block_size; ++i) {
            rendered.samples[0][next_sample_index++] = block[0][i];
        }

        if (round == 1) {
            float_param.cancel_events_at(3.0);
        }
    }

    assert_eq(expected_samples, rendered.samples[0], sample_count, DOUBLE_DELTA);
})


TEST(when_float_param_linear_ramp_is_canceled_between_samples_then_in_between_sample_value_is_held, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[] = {
        -0.1, -0.5, 0.5, 0.75, 0.75,
        0.75, 0.75, 0.75, 0.75, 0.75,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(-0.1);
    float_param.schedule_value(0.5, -1.0);
    float_param.schedule_linear_ramp(2.0, 1.0);
    float_param.cancel_events_at(2.25);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(when_float_param_linear_ramp_is_canceled_between_samples_then_in_between_sample_value_is_clamped_and_held, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[] = {
        -0.1, -0.5, 0.5, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(-0.1);
    float_param.schedule_value(0.5, -1.0);
    float_param.schedule_linear_ramp(10.0, 9.0);
    float_param.cancel_events_at(2.75);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(float_param_zero_length_linear_ramp_is_equivalent_to_setting_value, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[block_size] = {-1.0, -1.0, 1.0, 1.0, 1.0};
    FloatParam float_param("float", -1.0, 1.0, -1.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(-1.0);
    float_param.schedule_value(2.0, 0.0);
    float_param.schedule_linear_ramp(0.0, 1.0);

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(float_param_negative_length_linear_ramp_is_equivalent_to_setting_value, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[block_size] = {-1.0, -1.0, 1.0, 1.0, 1.0};
    FloatParam float_param("float", -1.0, 1.0, -1.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(-1.0);
    float_param.schedule_value(2.0, 0.0);
    float_param.schedule_linear_ramp(-1.0, 1.0);

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(float_param_rendering_is_independent_of_chunk_size, {
    constexpr Frequency sample_rate = 22050.0;
    FloatParam param_1("", -1.0, 1.0, 0.0);
    FloatParam param_2("", -1.0, 1.0, 0.0);

    param_1.set_sample_rate(sample_rate);
    param_2.set_sample_rate(sample_rate);

    param_1.set_value(-1.0);
    param_2.set_value(-1.0);

    param_1.schedule_linear_ramp(1.0, 0.2);
    param_2.schedule_linear_ramp(1.0, 0.2);

    assert_rendering_is_independent_from_chunk_size<FloatParam>(param_1, param_2);
})


TEST(float_param_linear_ramps_may_follow_each_other, {
    constexpr Integer block_size = 15;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.1, 0.2, 0.1, 0.0,
        0.2, 0.4, 0.6, 0.8, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(10.0);
    float_param.set_value(0.0);
    float_param.schedule_linear_ramp(0.2, 0.2);
    float_param.schedule_linear_ramp(0.2, 0.0);
    float_param.schedule_linear_ramp(0.5, 1.0);

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(float_param_linear_ramp_may_start_and_end_between_samples, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[] = {
        -0.1, -0.1, 0.25, 0.75, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(2.0);
    float_param.set_value(-0.1);
    float_param.schedule_value(0.75, 0.0);
    float_param.schedule_linear_ramp(1.5, 1.5);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(when_float_param_linear_ramp_goes_out_of_bounds_between_samples_then_it_is_clamped, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[] = {
        0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(5.0);
    float_param.set_value(0.0);
    float_param.schedule_value(0.99, 1.0);
    float_param.schedule_linear_ramp(0.02, 99999.0);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(a_float_params_clock_can_be_advanced_without_rendering, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[] = {
        -1.0, 0.125, 0.375, 0.625, 0.875,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(2.0);
    float_param.set_value(-1.0);
    float_param.schedule_value(15.25, 0.0);
    float_param.schedule_linear_ramp(2.0, 1.0);

    float_param.skip_round(0, block_size);
    float_param.skip_round(1, block_size);
    float_param.skip_round(2, block_size);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 3, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(skipping_the_same_round_multiple_times_advances_the_clock_only_once, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[] = {
        -1.0, 0.125, 0.375, 0.625, 0.875,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(2.0);
    float_param.set_value(-1.0);
    float_param.schedule_value(15.25, 0.0);
    float_param.schedule_linear_ramp(2.0, 1.0);

    float_param.skip_round(0, block_size);
    float_param.skip_round(1, block_size);
    float_param.skip_round(1, block_size);
    float_param.skip_round(1, block_size);
    float_param.skip_round(2, block_size);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 3, block_size);

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(a_skipped_round_may_be_shorter_than_the_block_size, {
    constexpr Integer block_size = 10;
    constexpr Integer short_round_length = 8;
    constexpr Sample expected_samples[] = {
        -1.0, -1.0, -1.0, -1.0, -1.0,
        -1.0, -1.0, 0.125,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(2.0);
    float_param.set_value(-1.0);
    float_param.schedule_value(15.25, 0.0);
    float_param.schedule_linear_ramp(2.0, 1.0);

    float_param.skip_round(0, short_round_length);
    float_param.skip_round(1, short_round_length);
    float_param.skip_round(1, short_round_length);
    float_param.skip_round(1, short_round_length);
    float_param.skip_round(2, short_round_length);
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 3, short_round_length);

    assert_eq(expected_samples, rendered_samples[0], short_round_length, DOUBLE_DELTA);
})


TEST(float_param_can_automatically_skip_constant_rounds, {
    constexpr Integer block_size = 10;
    constexpr Integer short_round_length = 6;
    constexpr Sample expected_samples[] = {
        /* -1.0, -1.0, -1.0, -1.0, -1.0, */
        -1.0, 0.125, 0.375, 0.625,
        0.875, 1.0,
        /* 1.0, 1.0, 1.0, 1.0, 1.0, */
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* first_round;
    Sample const* second_round_1;
    Sample const* second_round_2;
    Sample const* third_round_1;
    Sample const* third_round_2;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(2.0);
    float_param.set_value(-1.0);
    float_param.schedule_value(6.25, 0.0);
    float_param.schedule_linear_ramp(2.0, 1.0);

    first_round = FloatParam::produce_if_not_constant(
        float_param, 0, short_round_length
    );
    second_round_1 = FloatParam::produce_if_not_constant(
        float_param, 1, short_round_length
    );
    second_round_2 = FloatParam::produce_if_not_constant<FloatParam>(
        float_param, 1, short_round_length
    );
    third_round_1 = FloatParam::produce_if_not_constant<FloatParam>(
        float_param, 2, short_round_length
    );
    third_round_2 = FloatParam::produce_if_not_constant<FloatParam>(
        float_param, 2, short_round_length
    );

    assert_eq(NULL, first_round);
    assert_eq(NULL, second_round_1);
    assert_eq(NULL, second_round_2);
    assert_eq(expected_samples, third_round_1, short_round_length, DOUBLE_DELTA);
    assert_eq(expected_samples, third_round_2, short_round_length, DOUBLE_DELTA);
})


TEST(auto_skipping_a_follower_float_param_advances_the_clock_of_the_leader, {
    constexpr Integer block_size = 10;
    constexpr Integer short_round_length = 6;
    constexpr Sample expected_samples[] = {
        /* -1.0, -1.0, -1.0, -1.0, -1.0, */
        -1.0, 0.125, 0.375, 0.625,
        0.875, 1.0,
        /* 1.0, 1.0, 1.0, 1.0, 1.0, */
    };
    FloatParam leader("float", -1.0, 1.0, 0.0);
    FloatParam follower(leader);
    Sample const* first_round;
    Sample const* second_round_1;
    Sample const* second_round_2;
    Sample const* third_round_1;
    Sample const* third_round_2;

    leader.set_block_size(block_size);
    leader.set_sample_rate(2.0);
    leader.set_value(-1.0);
    leader.schedule_value(6.25, 0.0);
    leader.schedule_linear_ramp(2.0, 1.0);

    follower.set_block_size(block_size);
    follower.set_sample_rate(2.0);

    first_round = FloatParam::produce_if_not_constant(
        follower, 0, short_round_length
    );
    second_round_1 = FloatParam::produce_if_not_constant(
        follower, 1, short_round_length
    );
    second_round_2 = FloatParam::produce_if_not_constant<FloatParam>(
        follower, 1, short_round_length
    );
    third_round_1 = FloatParam::produce_if_not_constant<FloatParam>(
        follower, 2, short_round_length
    );
    third_round_2 = FloatParam::produce_if_not_constant<FloatParam>(
        follower, 2, short_round_length
    );

    assert_eq(NULL, first_round);
    assert_eq(NULL, second_round_1);
    assert_eq(NULL, second_round_2);
    assert_eq(expected_samples, third_round_1, short_round_length, DOUBLE_DELTA);
    assert_eq(expected_samples, third_round_2, short_round_length, DOUBLE_DELTA);
})


template<class FloatParamClass>
void test_follower_signal()
{
    constexpr Integer block_size = 10;
    FloatParam leader("float", -1.0, 1.0, 0.0);
    FloatParamClass follower(leader);
    Sample const* const* leader_samples;
    Sample const* const* follower_samples;
    Integer const follower_change_index_before = follower.get_change_index();
    Integer const leader_change_index_before = leader.get_change_index();

    leader_samples = FloatParam::produce<FloatParam>(leader, 1, block_size);
    follower_samples = FloatParam::produce<FloatParamClass>(follower, 1, block_size);

    Integer const follower_change_index_after = follower.get_change_index();
    Integer const leader_change_index_after = leader.get_change_index();

    assert_eq((void*)leader_samples, (void*)follower_samples);
    assert_eq((int)leader_change_index_before, (int)follower_change_index_before);
    assert_eq((int)leader_change_index_after, (int)follower_change_index_after);
    assert_eq((int)leader_change_index_before, (int)leader_change_index_after);
}


TEST(when_a_float_param_is_following_another_then_it_does_not_render_its_own_signal, {
    test_follower_signal<FloatParam>();
    test_follower_signal< ModulatableFloatParam<SignalProducer> >();
})


template<class FloatParamClass>
void test_follower_value()
{
    FloatParam leader("float", -1.0, 1.0, 0.0);
    FloatParamClass follower(leader);

    leader.set_value(0.5);

    assert_eq(0.5, leader.get_value(), DOUBLE_DELTA);
    assert_eq(0.5, follower.get_value(), DOUBLE_DELTA);
}


TEST(when_a_float_param_is_following_another_then_it_has_the_same_value, {
    test_follower_value<FloatParam>();
    test_follower_value< ModulatableFloatParam<SignalProducer> >();
})


template<class FloatParamClass>
void test_follower_constantness()
{
    constexpr Integer block_size = 5;
    FloatParam leader("float", -1.0, 1.0, 0.0);
    FloatParamClass follower(leader);

    leader.set_block_size(block_size);
    leader.set_sample_rate(10.0);

    follower.set_block_size(block_size);
    follower.set_sample_rate(10.0);

    leader.schedule_value(0.5, 1.0);

    assert_true(follower.is_constant_until(5));
    assert_false(follower.is_constant_until(6));

    assert_eq(0.0, follower.get_value());

    assert_true(follower.is_constant_in_next_round(1, block_size), "next_round=1");
    FloatParam::produce<FloatParam>(leader, 1);
    assert_true(follower.is_constant_in_next_round(1, block_size), "next_round=1");

    Integer const follower_change_index_before = follower.get_change_index();
    Integer const leader_change_index_before = leader.get_change_index();

    assert_false(follower.is_constant_in_next_round(2, block_size), "next_round=2");
    FloatParam::produce<FloatParam>(leader, 2);
    assert_false(follower.is_constant_in_next_round(2, block_size), "next_round=2");

    Integer const follower_change_index_after = follower.get_change_index();
    Integer const leader_change_index_after = leader.get_change_index();

    assert_true(follower.is_constant_in_next_round(3, block_size), "next_round=3");
    assert_eq(1.0, follower.get_value(), DOUBLE_DELTA);

    assert_eq((int)leader_change_index_before, (int)follower_change_index_before);
    assert_eq((int)leader_change_index_after, (int)follower_change_index_after);
    assert_neq((int)leader_change_index_before, (int)leader_change_index_after);
}


TEST(when_a_float_param_is_following_another_then_it_is_constant_if_the_leader_is_constant, {
    test_follower_constantness<FloatParam>();
    test_follower_constantness< ModulatableFloatParam<SignalProducer> >();
})


TEST(when_a_float_param_does_not_have_an_envelope_then_applying_envelope_is_no_op, {
    constexpr Integer block_size = 10;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = block_size * rounds;
    constexpr Sample expected_samples[sample_count] = {
        1.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -1.0, 1.0, 0.0);
    Sample const* const* rendered_samples;

    float_param.set_sample_rate(1.0);
    float_param.set_value(1.0);
    float_param.start_envelope(3.0);
    assert_eq(0.0, float_param.end_envelope(6.0), DOUBLE_DELTA);
    assert_eq(NULL, float_param.get_envelope());

    assert_true(float_param.is_constant_until(block_size));

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples[0], sample_count, DOUBLE_DELTA);
})


TEST(when_a_float_param_does_have_an_envelope_then_dahds_can_be_applied, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.0, 1.0, 2.0, 3.0,
        3.0, 2.0, 1.0, 1.0, 1.0,
    };
    FloatParam float_param("float", -5.0, 5.0, 0.0);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);
    float_param.set_envelope(&envelope);

    assert_eq((void*)&envelope, (void*)float_param.get_envelope());

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(0.0);
    envelope.final_value.set_value(0.0);

    float_param.start_envelope(0.3);

    assert_true(float_param.is_constant_until(1));
    assert_false(float_param.is_constant_until(2));

    assert_false(float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(a_float_param_envelope_may_be_released_before_dahds_is_completed, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.0, 1.0, 2.0, 3.0,
        1.5, 0.0, 0.0, 0.0, 0.0,
    };
    FloatParam float_param("float", -5.0, 5.0, 0.0);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);
    float_param.set_envelope(&envelope);

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(2.0);
    envelope.final_value.set_value(0.625);

    float_param.start_envelope(0.3);
    assert_eq(2.0, float_param.end_envelope(4.0), DOUBLE_DELTA);

    assert_true(float_param.is_constant_until(1));
    assert_false(float_param.is_constant_until(2));
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(a_float_param_envelope_may_be_released_immediately, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        5.0, 0.0, -1.0, -2.0, -2.0,
        -2.0, -2.0, -2.0, -2.0, -2.0,
    };
    FloatParam float_param("float", -5.0, 5.0, 0.0);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);

    float_param.set_value(5.0);
    float_param.set_envelope(&envelope);

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.5);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(2.0);
    envelope.final_value.set_value(0.375);

    float_param.start_envelope(1.0);
    assert_eq(2.0, float_param.end_envelope(1.0), DOUBLE_DELTA);

    assert_true(float_param.is_constant_until(1));
    assert_false(float_param.is_constant_until(2));
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(envelope_release_params_are_saved_when_the_envelope_is_started, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.0, 1.0, 2.0, 3.0,
        1.5, 0.0, 0.0, 0.0, 0.0,
    };
    FloatParam float_param("float", -5.0, 5.0, 0.0);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);
    float_param.set_envelope(&envelope);

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(2.0);
    envelope.final_value.set_value(0.625);

    float_param.start_envelope(0.3);

    envelope.release_time.set_value(0.123);
    envelope.amount.set_value(1.0);
    envelope.final_value.set_value(1.0);

    assert_eq(2.0, float_param.end_envelope(4.0), DOUBLE_DELTA);

    assert_true(float_param.is_constant_until(1));
    assert_false(float_param.is_constant_until(2));
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(cancelling_an_envelope_releases_it_in_a_given_amount_of_time, {
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.0, 1.0, 2.0, 3.0,
        1.5, 0.0, 0.0, 0.0, 0.0,
    };
    FloatParam float_param("float", -5.0, 5.0, 0.0);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);

    envelope.dynamic.set_value(ToggleParam::ON);
    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(0.1);
    envelope.final_value.set_value(0.625);

    float_param.set_envelope(&envelope);

    envelope.release_time.set_value(0.01);

    float_param.start_envelope(0.3);
    float_param.cancel_envelope(4.0, 2.0);

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
})


template<class FloatParamClass>
void test_follower_envelope()
{
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.0, 1.0, 2.0, 3.0,
        1.5, 0.0, 0.0, 0.0, 0.0,
    };
    FloatParam leader("follow", -5.0, 5.0, 0.0);
    FloatParamClass follower(leader);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    leader.set_block_size(block_size);
    leader.set_sample_rate(1.0);
    leader.set_envelope(&envelope);
    leader.set_value(0.2);

    follower.set_block_size(block_size);
    follower.set_sample_rate(1.0);

    assert_eq((void*)&envelope, (void*)follower.get_envelope());

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(2.0);
    envelope.final_value.set_value(0.625);

    follower.start_envelope(0.3);

    envelope.release_time.set_value(0.123);

    assert_eq(2.0, follower.end_envelope(4.0), DOUBLE_DELTA);

    assert_true(follower.is_constant_until(1));
    assert_false(follower.is_constant_until(2));

    rendered_samples = FloatParam::produce<FloatParamClass>(follower, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
}


TEST(follower_float_param_follows_the_leaders_envelope, {
    test_follower_envelope<FloatParam>();
    test_follower_envelope< ModulatableFloatParam<SignalProducer> >();
})


template<class FloatParamClass>
void test_follower_envelope_cancellation()
{
    constexpr Integer block_size = 10;
    constexpr Sample expected_samples[block_size] = {
        0.0, 0.0, 1.0, 2.0, 3.0,
        1.5, 0.0, 0.0, 0.0, 0.0,
    };
    FloatParam leader("follow", -5.0, 5.0, 0.0);
    FloatParamClass follower(leader);
    Envelope envelope("env");
    Sample const* const* rendered_samples;

    leader.set_block_size(block_size);
    leader.set_sample_rate(1.0);
    leader.set_envelope(&envelope);
    leader.set_value(0.2);

    follower.set_block_size(block_size);
    follower.set_sample_rate(1.0);

    assert_eq((void*)&envelope, (void*)follower.get_envelope());

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(0.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(1.0);
    envelope.decay_time.set_value(2.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(6.0);
    envelope.final_value.set_value(0.625);

    follower.start_envelope(0.3);

    envelope.release_time.set_value(0.123);

    follower.end_envelope(4.0);
    follower.cancel_envelope(4.001, 1.999);
    assert_eq(1.999, follower.end_envelope(5.0), DOUBLE_DELTA);

    assert_true(follower.is_constant_until(1));
    assert_false(follower.is_constant_until(2));

    rendered_samples = FloatParam::produce<FloatParamClass>(follower, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, 0.001);
}


TEST(canceling_follower_float_param_envelope_releases_it_in_the_given_amount_of_time, {
    test_follower_envelope_cancellation<FloatParam>();
    test_follower_envelope_cancellation< ModulatableFloatParam<SignalProducer> >();
})


template<class FloatParamClass>
void test_follower_dynamic_envelope()
{
    constexpr Integer block_size = 10;
    constexpr Sample expected_dahd_samples[block_size] = {
        0.0, 1.0, 2.0, 3.0, 3.0,
        3.0, 2.5, 2.0, 1.5, 1.0,
    };
    constexpr Sample expected_r_samples[block_size] = {
        1.0, 1.0, 1.0, 1.0, 1.0,
        0.5, 0.0, 0.0, 0.0, 0.0,
    };
    FloatParam leader("follow", -5.0, 5.0, 0.0);
    FloatParamClass follower(leader);
    Envelope envelope("env");
    Sample const* rendered_samples;

    leader.set_block_size(block_size);
    leader.set_sample_rate(1.0);
    leader.set_envelope(&envelope);
    leader.set_value(0.2);

    follower.set_block_size(block_size);
    follower.set_sample_rate(1.0);

    assert_eq((void*)&envelope, (void*)follower.get_envelope());

    envelope.dynamic.set_value(ToggleParam::ON);
    envelope.amount.set_value(0.1);
    envelope.initial_value.set_value(0.1);
    envelope.delay_time.set_value(5.7);
    envelope.attack_time.set_value(0.1);
    envelope.peak_value.set_value(0.1);
    envelope.hold_time.set_value(0.1);
    envelope.decay_time.set_value(0.1);
    envelope.sustain_value.set_value(0.1);
    envelope.release_time.set_value(0.123);
    envelope.final_value.set_value(0.625);

    follower.start_envelope(0.3);

    envelope.release_time.set_value(2.0);

    assert_eq(2.0, follower.end_envelope(29.0), DOUBLE_DELTA);

    FloatParam::produce<FloatParamClass>(follower, 1, 6);

    envelope.amount.set_value(0.8);
    envelope.initial_value.set_value(0.625);
    envelope.delay_time.set_value(5.7);
    envelope.attack_time.set_value(3.0);
    envelope.peak_value.set_value(1.0);
    envelope.hold_time.set_value(2.0);
    envelope.decay_time.set_value(4.0);
    envelope.sustain_value.set_value(0.75);
    envelope.release_time.set_value(6.0);

    assert_false(follower.is_constant_until(2));

    rendered_samples = FloatParam::produce_if_not_constant<FloatParamClass>(
        follower, 2, block_size
    );
    assert_eq(expected_dahd_samples, rendered_samples, block_size, DOUBLE_DELTA);

    assert_true(follower.is_constant_until(block_size));
    rendered_samples = FloatParam::produce_if_not_constant<FloatParamClass>(
        follower, 3, block_size
    );
    assert_eq(NULL, rendered_samples, block_size, DOUBLE_DELTA);

    rendered_samples = FloatParam::produce_if_not_constant<FloatParamClass>(
        follower, 4, block_size
    );
    assert_eq(expected_r_samples, rendered_samples, block_size, DOUBLE_DELTA);
}


TEST(when_the_envelope_is_dynamic_then_the_param_reacts_to_its_changes_during_dahds, {
    test_follower_dynamic_envelope<FloatParam>();
    test_follower_dynamic_envelope< ModulatableFloatParam<SignalProducer> >();
})


TEST(when_a_midi_controller_is_assigned_to_a_float_param_then_float_param_value_follows_the_changes_of_the_midi_controller, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[block_size] = {
        3.0, 3.0, -2.5, -2.5, -2.5,
    };
    FloatParam float_param("float", -5.0, 5.0, 3.0, 0.5);
    MidiController midi_controller;
    Integer change_index_1;
    Integer change_index_2;
    Sample const* const* rendered_samples;

    midi_controller.change(0.0, 0.8);
    midi_controller.clear();

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(1.0);
    float_param.set_midi_controller(&midi_controller);

    assert_eq((void*)&midi_controller, (void*)float_param.get_midi_controller());

    change_index_1 = float_param.get_change_index();
    midi_controller.change(1.5, 0.2514);
    change_index_2 = float_param.get_change_index();

    /* Non-sample-exact param usage. */
    assert_eq(-2.5, float_param.get_value(), DOUBLE_DELTA);
    assert_eq(0.2514, float_param.get_ratio(), DOUBLE_DELTA);

    assert_neq((int)change_index_1, (int)change_index_2);

    assert_true(float_param.is_constant_until(2));
    assert_false(float_param.is_constant_until(3));

    /* Sample-exact param usage. */
    assert_false(float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, 0.01);

    midi_controller.change(0.0, 0.35);
    float_param.set_midi_controller(NULL);
    assert_eq(-1.5, float_param.get_value(), DOUBLE_DELTA);
})


TEST(float_param_follows_midi_controller_changes_gradually, {
    constexpr Integer block_size = 2000;
    constexpr Frequency sample_rate = 3000.0;
    constexpr Seconds big_change_duration = FloatParam::MIDI_CTL_BIG_CHANGE_DURATION;
    constexpr Seconds small_change_duration = FloatParam::MIDI_CTL_SMALL_CHANGE_DURATION;

    FloatParam reference_float_param("reference", 0.0, 10.0, 0.0);
    FloatParam float_param("float", 0.0, 10.0, 0.0);
    MidiController midi_controller;
    Sample const* expected_samples;
    Sample const* rendered_samples;

    midi_controller.change(0.0, 0.0);
    midi_controller.clear();

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(sample_rate);
    float_param.set_midi_controller(&midi_controller);

    midi_controller.change(0.01, 0.001);
    midi_controller.change(0.02, 0.005);
    midi_controller.change(0.03, 0.010);
    midi_controller.change(0.31, 0.025);
    midi_controller.change(0.31, 0.325);
    midi_controller.change(0.31, 0.325);
    midi_controller.change(0.31, 0.325);
    midi_controller.change(0.32, 0.325);
    midi_controller.change(0.33, 0.325);
    midi_controller.change(0.33, 0.325);
    midi_controller.change(0.33, 0.330);
    midi_controller.change(0.41, 0.960);

    reference_float_param.set_block_size(block_size);
    reference_float_param.set_sample_rate(sample_rate);

    reference_float_param.set_value(0.0);

    reference_float_param.schedule_linear_ramp(small_change_duration, 0.1);
    reference_float_param.schedule_linear_ramp(0.3, 3.3);
    reference_float_param.schedule_linear_ramp(big_change_duration * 0.63, 9.6);

    expected_samples = FloatParam::produce_if_not_constant(reference_float_param, 1, block_size);
    rendered_samples = FloatParam::produce_if_not_constant(float_param, 1, block_size);

    assert_eq(expected_samples, rendered_samples, block_size, DOUBLE_DELTA);
})


template<class FloatParamClass>
void test_follower_midi_controller()
{
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[block_size] = {
        3.0, 3.0, -2.5, -2.5, -2.5,
    };
    FloatParam leader("float", -5.0, 5.0, 3.0, 0.5);
    FloatParamClass follower(leader);
    MidiController midi_controller;
    Integer change_index_1;
    Integer change_index_2;
    Sample const* const* leader_samples;
    Sample const* const* follower_samples;

    midi_controller.change(0.0, 0.8);
    midi_controller.clear();

    leader.set_block_size(block_size);
    leader.set_sample_rate(1.0);
    leader.set_midi_controller(&midi_controller);

    follower.set_block_size(block_size);
    follower.set_sample_rate(1.0);

    change_index_1 = follower.get_change_index();
    midi_controller.change(1.5, 0.2514);
    change_index_2 = follower.get_change_index();
    assert_eq(-2.5, follower.get_value(), DOUBLE_DELTA);
    assert_eq(0.2514, follower.get_ratio(), DOUBLE_DELTA);

    assert_neq((int)change_index_1, (int)change_index_2);

    assert_true(follower.is_constant_until(2));
    assert_false(follower.is_constant_until(3));

    assert_false(follower.is_constant_in_next_round(1, block_size));
    leader_samples = FloatParam::produce<FloatParam>(leader, 1, block_size);
    follower_samples = FloatParam::produce<FloatParamClass>(follower, 1, block_size);
    assert_false(follower.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, follower_samples[0], block_size, 0.01);
    assert_eq((void*)leader_samples, (void*)follower_samples);

    midi_controller.change(0.0, 0.35);
    leader.set_midi_controller(NULL);
    assert_eq(-1.5, follower.get_value(), DOUBLE_DELTA);
}


TEST(when_a_midi_controller_is_assigned_to_the_leader_of_a_float_param_then_the_follower_value_follows_the_changes_of_the_midi_controller, {
    test_follower_midi_controller<FloatParam>();
    test_follower_midi_controller< ModulatableFloatParam<SignalProducer> >();
})


TEST(when_a_flexible_controller_is_assigned_to_a_float_param_then_float_param_value_follows_the_changes_of_the_flexible_controller, {
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[block_size] = {
        3.0, 3.0, 3.0, 3.0, 3.0,
    };
    constexpr Frequency sample_rate = 1.0;
    FloatParam float_param("float", 0.0, 10.0, 9.0, 1.0);
    FlexibleController flexible_controller;
    Integer change_index;
    Sample const* const* rendered_samples;

    float_param.set_block_size(block_size);
    flexible_controller.input.set_block_size(block_size);
    flexible_controller.amount.set_block_size(block_size);
    flexible_controller.min.set_block_size(block_size);
    flexible_controller.max.set_block_size(block_size);
    flexible_controller.distortion.set_block_size(block_size);
    flexible_controller.randomness.set_block_size(block_size);

    float_param.set_sample_rate(sample_rate);
    flexible_controller.input.set_sample_rate(sample_rate);
    flexible_controller.amount.set_sample_rate(sample_rate);
    flexible_controller.min.set_sample_rate(sample_rate);
    flexible_controller.max.set_sample_rate(sample_rate);
    flexible_controller.distortion.set_sample_rate(sample_rate);
    flexible_controller.randomness.set_sample_rate(sample_rate);

    flexible_controller.input.set_value(0.2);
    flexible_controller.amount.set_value(0.5);

    float_param.set_flexible_controller(&flexible_controller);

    assert_eq(
        (void*)&flexible_controller, (void*)float_param.get_flexible_controller()
    );
    assert_eq(1.0, float_param.get_value());

    flexible_controller.input.set_value(0.64);

    assert_false(float_param.is_constant_in_next_round(1, block_size));

    change_index = float_param.get_change_index();
    assert_eq(3.0, float_param.get_value(), DOUBLE_DELTA);
    assert_eq(0.32, float_param.get_ratio(), DOUBLE_DELTA);

    rendered_samples = FloatParam::produce<FloatParam>(float_param, 1, block_size);
    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);

    assert_eq((int)change_index, (int)float_param.get_change_index());

    flexible_controller.input.set_value(0.4);
    assert_neq((int)change_index, (int)float_param.get_change_index());

    float_param.set_flexible_controller(NULL);
    assert_eq(2.0, float_param.get_value(), DOUBLE_DELTA);
})


TEST(when_an_lfo_is_assigned_to_a_float_param_then_float_param_value_follows_the_changes_of_the_lfo, {
    constexpr Integer block_size = 1024;
    constexpr Frequency sample_rate = 11025.0;
    constexpr Frequency frequency = 20.0;
    FloatParam float_param("float", -3.0, 7.0, 2.0);
    FloatParam fast_float_param("fast-float", 0.0, 1.0, 1.0);
    LFO lfo("lfo");
    Sample const* rendered_samples;
    Sample const* lfo_buffer;
    SumOfSines expected(5.0, frequency, 0.0, 0.0, 0.0, 0.0, 1, 0.0, 2.0);
    Buffer expected_output(block_size, 1);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    lfo.set_block_size(block_size);
    lfo.set_sample_rate(sample_rate);
    lfo.frequency.set_value(20.0);
    lfo.waveform.set_value(LFO::Oscillator_::SINE);
    lfo.start(0.0);

    float_param.set_block_size(block_size);
    float_param.set_sample_rate(sample_rate);
    float_param.set_lfo(&lfo);

    fast_float_param.set_block_size(block_size);
    fast_float_param.set_sample_rate(sample_rate);
    fast_float_param.set_lfo(&lfo);

    Integer const fast_float_param_change_index = fast_float_param.get_change_index();

    assert_eq((void*)&lfo, (void*)float_param.get_lfo());
    assert_false(float_param.is_constant_in_next_round(1, block_size));

    render_rounds<SumOfSines>(expected, expected_output, 1);
    rendered_samples = FloatParam::produce_if_not_constant(
        float_param, 1, block_size
    );
    lfo_buffer = SignalProducer::produce<LFO>(lfo, 1, block_size)[0];

    assert_eq(expected_output.samples[0], rendered_samples, block_size, 0.001);

    rendered_samples = FloatParam::produce_if_not_constant(
        fast_float_param, 1, block_size
    );
    assert_eq((void*)lfo_buffer, (void*)rendered_samples);
    assert_eq(lfo_buffer[block_size - 1], fast_float_param.get_value());
    assert_neq((int)fast_float_param_change_index, (int)fast_float_param.get_change_index());
})


template<class FloatParamClass>
void test_follower_lfo()
{
    constexpr Integer block_size = 1024;
    constexpr Frequency sample_rate = 11025.0;
    constexpr Frequency frequency = 20.0;
    FloatParam leader("leader", -3.0, 7.0, 2.0);
    FloatParamClass follower(leader);
    LFO lfo("lfo");
    Sample const* rendered_samples;
    SumOfSines expected(5.0, frequency, 0.0, 0.0, 0.0, 0.0, 1, 0.0, 2.0);
    Buffer expected_output(block_size, 1);

    expected.set_block_size(block_size);
    expected.set_sample_rate(sample_rate);

    lfo.set_block_size(block_size);
    lfo.set_sample_rate(sample_rate);
    lfo.frequency.set_value(20.0);
    lfo.waveform.set_value(LFO::Oscillator_::SINE);
    lfo.start(0.0);

    leader.set_block_size(block_size);
    leader.set_sample_rate(sample_rate);
    leader.set_lfo(&lfo);

    follower.set_block_size(block_size);
    follower.set_sample_rate(sample_rate);
    follower.set_lfo(&lfo);

    assert_eq((void*)&lfo, (void*)follower.get_lfo());
    assert_false(follower.is_constant_in_next_round(1, block_size));

    render_rounds<SumOfSines>(expected, expected_output, 1);
    rendered_samples = FloatParam::produce_if_not_constant<FloatParamClass>(
        follower, 1, block_size
    );

    assert_eq(expected_output.samples[0], rendered_samples, block_size, 0.001);
}


TEST(when_an_lfo_is_assigned_to_the_leader_of_a_float_param_then_the_follower_value_follows_the_changes_of_the_lfo, {
    test_follower_lfo<FloatParam>();
    test_follower_lfo< ModulatableFloatParam<SignalProducer> >();
})


template<class FloatParamClass>
void test_follower_flexible_controller()
{
    constexpr Integer block_size = 5;
    constexpr Sample expected_samples[block_size] = {
        3.0, 3.0, 3.0, 3.0, 3.0,
    };
    constexpr Frequency sample_rate = 1.0;
    FloatParam leader("leader", 0.0, 10.0, 9.0, 1.0);
    FloatParamClass follower(leader);
    FlexibleController flexible_controller;
    Integer change_index;
    Sample const* const* leader_samples;
    Sample const* const* follower_samples;

    leader.set_block_size(block_size);
    follower.set_block_size(block_size);
    flexible_controller.input.set_block_size(block_size);
    flexible_controller.amount.set_block_size(block_size);
    flexible_controller.min.set_block_size(block_size);
    flexible_controller.max.set_block_size(block_size);
    flexible_controller.distortion.set_block_size(block_size);
    flexible_controller.randomness.set_block_size(block_size);

    leader.set_sample_rate(sample_rate);
    follower.set_sample_rate(sample_rate);
    flexible_controller.input.set_sample_rate(sample_rate);
    flexible_controller.amount.set_sample_rate(sample_rate);
    flexible_controller.min.set_sample_rate(sample_rate);
    flexible_controller.max.set_sample_rate(sample_rate);
    flexible_controller.distortion.set_sample_rate(sample_rate);
    flexible_controller.randomness.set_sample_rate(sample_rate);

    flexible_controller.input.set_value(0.2);
    flexible_controller.amount.set_value(0.5);

    leader.set_flexible_controller(&flexible_controller);

    assert_eq(1.0, follower.get_value());

    flexible_controller.input.set_value(0.64);

    assert_false(follower.is_constant_in_next_round(1, block_size));

    change_index = follower.get_change_index();
    assert_eq(3.0, follower.get_value(), DOUBLE_DELTA);
    assert_eq(0.32, follower.get_ratio(), DOUBLE_DELTA);

    follower_samples = FloatParam::produce<FloatParamClass>(follower, 1, block_size);
    leader_samples = FloatParam::produce<FloatParam>(leader, 1, block_size);
    assert_eq(expected_samples, follower_samples[0], block_size, DOUBLE_DELTA);
    assert_eq((void*)leader_samples, (void*)follower_samples);

    assert_eq((int)change_index, (int)follower.get_change_index());

    flexible_controller.input.set_value(0.4);
    assert_neq((int)change_index, (int)follower.get_change_index());

    leader.set_flexible_controller(NULL);
    assert_eq(2.0, follower.get_value(), DOUBLE_DELTA);
}


TEST(when_a_flexible_controller_is_assigned_to_the_leader_of_a_float_param_then_the_follower_value_follows_the_changes_of_the_flexible_controller, {
    test_follower_flexible_controller<FloatParam>();
    test_follower_flexible_controller< ModulatableFloatParam<SignalProducer> >();
})


TEST(a_float_param_may_use_logarithmic_scale, {
    constexpr Number min = Constants::BIQUAD_FILTER_FREQUENCY_MIN;
    constexpr Number max = Constants::BIQUAD_FILTER_FREQUENCY_MAX;
    constexpr Number log2_min = std::log2(min);
    constexpr Number log2_max = std::log2(max);
    constexpr Integer block_size = 15;
    constexpr Frequency sample_rate = 14.0;
    constexpr Sample expected_samples_log[] = {
        0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 12.0, 12.0,
    };
    ToggleParam log_scale("log", ToggleParam::OFF);
    FloatParam leader(
        "freq",
        min,
        max,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        &log_scale,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_SCALE
    );
    FloatParam follower(leader);
    Envelope envelope("env");
    Sample const* rendered_samples;
    Sample rendered_samples_log[block_size];

    leader.set_sample_rate(sample_rate);
    leader.set_value(Constants::BIQUAD_FILTER_FREQUENCY_MIN);
    leader.set_envelope(&envelope);

    follower.set_sample_rate(sample_rate);
    follower.set_value(Constants::BIQUAD_FILTER_FREQUENCY_MIN);

    envelope.amount.set_value(1.0);
    envelope.initial_value.set_value(0.0);
    envelope.delay_time.set_value(0.0);
    envelope.attack_time.set_value(1.0);
    envelope.peak_value.set_value(std::log2(16384.0) / (log2_max - log2_min));

    assert_eq(min, follower.ratio_to_value(0.0), DOUBLE_DELTA);
    assert_eq((min + max) / 2.0, follower.ratio_to_value(0.5), DOUBLE_DELTA);
    assert_eq(0.5, follower.value_to_ratio((min + max) / 2.0), DOUBLE_DELTA);
    assert_eq(max, follower.ratio_to_value(1.0), DOUBLE_DELTA);

    log_scale.set_value(ToggleParam::ON);

    leader.set_ratio(0.3);
    assert_eq(0.3, leader.get_ratio(), 0.001);

    assert_eq(min, follower.ratio_to_value(0.0), DOUBLE_DELTA);
    assert_eq(
        (log2_min + log2_max) / 2.0,
        std::log2(follower.ratio_to_value(0.5)),
        0.02
    );
    assert_eq(
        0.5,
        follower.value_to_ratio(std::pow(2.0, (log2_min + log2_max) / 2.0)),
        DOUBLE_DELTA
    );
    assert_eq(max, follower.ratio_to_value(1.0), DOUBLE_DELTA);

    follower.start_envelope(0.0);
    follower.cancel_events_at(12.0 / sample_rate);

    assert_float_param_changes_during_rendering(
        follower, 1, block_size, &rendered_samples
    );

    for (Integer i = 0; i != block_size; ++i) {
        rendered_samples_log[i] = std::log2(rendered_samples[i]);
    }

    assert_eq(expected_samples_log, rendered_samples_log, block_size, 0.027);
});


class Modulator : public SignalProducer
{
    friend class SignalProducer;

    public:
        static constexpr Number VALUE = 2.0;

        Modulator() noexcept : SignalProducer(1, 0), render_called(0)
        {
        }

        int render_called;

    protected:
        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) noexcept {
            ++render_called;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = VALUE;
            }
        }
};


TEST(when_no_modulator_is_set_then_modulated_float_param_is_constant, {
    constexpr Integer block_size = 3;
    constexpr Frequency sample_rate = 1.0;
    constexpr Sample expected_samples[][block_size] = {
        {6.0, 6.0, 6.0},
        {6.0, 3.0, 3.0},
    };

    FloatParam modulation_level_leader("MOD", 0.0, 1.0, 1.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        NULL, modulation_level_leader, "", 0.0, 10.0, 0.0
    );
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);

    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(6.0);
    modulation_level_leader.set_value(0.5);
    modulation_level_leader.schedule_linear_ramp(2.0, 1.0);

    assert_true(modulatable_float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_true(modulatable_float_param.is_constant_in_next_round(1, block_size));
    assert_eq(expected_samples[0], rendered_samples[0], block_size, DOUBLE_DELTA);

    modulatable_float_param.schedule_value(1.0, 3.0);
    assert_false(modulatable_float_param.is_constant_in_next_round(2, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 2
    );
    assert_eq(expected_samples[1], rendered_samples[0], block_size, DOUBLE_DELTA);

})


TEST(when_modulation_level_is_zero_then_modulated_float_param_is_constant_and_does_not_invoke_modulator, {
    constexpr Integer block_size = 3;
    constexpr Frequency sample_rate = 1.0;
    constexpr Sample expected_samples[] = {1.0, 1.0, 1.0};

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 1.0, 0.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 10.0, 0.0
    );
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);

    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(1.0);
    modulation_level_leader.set_value(0.0);

    assert_true(modulatable_float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_true(modulatable_float_param.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(0, modulator.render_called);
})


TEST(when_modulation_level_is_zero_but_the_modulatable_float_param_is_scheduled_then_modulated_float_param_is_not_constant, {
    constexpr Integer block_size = 3;
    constexpr Frequency sample_rate = 1.0;
    constexpr Sample expected_samples[] = {1.0, 2.0, 2.0};

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 1.0, 0.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 10.0, 0.0
    );
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);

    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(1.0);
    modulatable_float_param.schedule_value(1.0, 2.0);
    modulation_level_leader.set_value(0.0);

    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(0, modulator.render_called);
})


TEST(when_modulation_level_is_positive_then_modulated_float_param_is_not_constant_and_does_invoke_modulator, {
    constexpr Integer block_size = 3;
    constexpr Frequency sample_rate = 1.0;
    constexpr Number modulation_level_value = 3.0;
    constexpr Number param_value = 1.0;
    constexpr Sample expected_samples[] = {
        (Sample)(param_value + Modulator::VALUE * modulation_level_value),
        (Sample)(param_value + Modulator::VALUE * modulation_level_value),
        (Sample)(param_value + Modulator::VALUE * modulation_level_value),
    };

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 3.0, 3.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 10.0, 0.0
    );
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);

    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(param_value);
    modulation_level_leader.set_value(modulation_level_value);

    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(1, modulator.render_called);
})


TEST(when_modulation_level_is_changing_then_modulated_float_param_is_not_constant_and_does_invoke_modulator, {
    constexpr Integer block_size = 5;
    constexpr Frequency sample_rate = 1.0;
    constexpr Number modulation_level_value = 3.0;
    constexpr Number param_value = 1.0;
    constexpr Sample expected_samples[] = {
        param_value,
        param_value,
        (Sample)(param_value + Modulator::VALUE * modulation_level_value),
        (Sample)(param_value + Modulator::VALUE * modulation_level_value),
        (Sample)(param_value + Modulator::VALUE * modulation_level_value),
    };

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 3.0, 0.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 10.0, 0.0
    );
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);

    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(param_value);
    modulation_level_leader.set_value(0.0);
    modulation_level_leader.schedule_value(2.0, modulation_level_value);

    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(1, modulator.render_called);
})


TEST(modulation_level_may_be_automated_with_envelope, {
    constexpr Integer block_size = 5;
    constexpr Frequency sample_rate = 1.0;
    constexpr Number param_value = 1.0;
    constexpr Sample expected_samples[] = {
        param_value,
        param_value,
        (Sample)(param_value + Modulator::VALUE * 1.0),
        (Sample)(param_value + Modulator::VALUE * 2.0),
        (Sample)(param_value + Modulator::VALUE * 3.0),
    };
    Envelope envelope("ENV");

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 3.0, 0.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 10.0, 0.0
    );
    Sample const* const* rendered_samples;

    envelope.amount.set_block_size(block_size);
    envelope.initial_value.set_block_size(block_size);
    envelope.delay_time.set_block_size(block_size);
    envelope.attack_time.set_block_size(block_size);
    envelope.peak_value.set_block_size(block_size);
    envelope.hold_time.set_block_size(block_size);
    envelope.decay_time.set_block_size(block_size);
    envelope.sustain_value.set_block_size(block_size);
    envelope.release_time.set_block_size(block_size);
    envelope.final_value.set_block_size(block_size);

    envelope.amount.set_sample_rate(sample_rate);
    envelope.initial_value.set_sample_rate(sample_rate);
    envelope.delay_time.set_sample_rate(sample_rate);
    envelope.attack_time.set_sample_rate(sample_rate);
    envelope.peak_value.set_sample_rate(sample_rate);
    envelope.hold_time.set_sample_rate(sample_rate);
    envelope.decay_time.set_sample_rate(sample_rate);
    envelope.sustain_value.set_sample_rate(sample_rate);
    envelope.release_time.set_sample_rate(sample_rate);
    envelope.final_value.set_sample_rate(sample_rate);

    envelope.attack_time.set_value(3.0);
    envelope.hold_time.set_value(12.0);
    envelope.release_time.set_value(3.0);

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);
    modulation_level_leader.set_envelope(&envelope);

    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(param_value);
    modulatable_float_param.start_envelope(6.0);
    assert_eq(3.0, modulatable_float_param.end_envelope(12.0), DOUBLE_DELTA);

    assert_eq(
        NULL,
        (void*)FloatParam::produce_if_not_constant< ModulatableFloatParam<Modulator> >(
            modulatable_float_param, 1, block_size
        )
    );

    assert_false(modulatable_float_param.is_constant_in_next_round(2, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 2
    );
    assert_false(modulatable_float_param.is_constant_in_next_round(2, block_size));

    assert_eq(expected_samples, rendered_samples[0], block_size, DOUBLE_DELTA);
    assert_eq(1, modulator.render_called);
})


TEST(modulated_values_are_not_clamped, {
    constexpr Integer block_size = 5;
    constexpr Frequency sample_rate = 1.0;
    constexpr Number modulation_level_value = 3.0;
    constexpr Sample expected_samples[][block_size] = {
        {1.0, 1.0, 7.0, 7.0, 7.0},
        {7.0, 7.0, 7.0, 7.0, 7.0},
    };

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 3.0, 0.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 2.0, 0.0
    );
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);
    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_value(1.0);
    modulation_level_leader.set_value(0.0);
    modulation_level_leader.schedule_value(2.0, modulation_level_value);

    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_eq(expected_samples[0], rendered_samples[0], block_size, DOUBLE_DELTA);

    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 2
    );
    assert_eq(expected_samples[1], rendered_samples[0], block_size, DOUBLE_DELTA);
})


TEST(modulated_param_might_have_a_midi_controller_assigned, {
    constexpr Integer block_size = 5;
    constexpr Frequency sample_rate = 1.0;
    constexpr Number modulation_level_value = 3.0;
    constexpr Sample expected_samples[][block_size] = {
        {0.25, 1.0, 7.0, 7.0, 7.0},
        {7.0, 7.0, 7.0, 7.0, 7.0},
    };

    Modulator modulator;
    FloatParam modulation_level_leader("MOD", 0.0, 3.0, 0.0);
    ModulatableFloatParam<Modulator> modulatable_float_param(
        &modulator, modulation_level_leader, "", 0.0, 2.0, 0.0
    );
    MidiController midi_controller;
    Sample const* const* rendered_samples;

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);
    modulatable_float_param.set_block_size(block_size);
    modulatable_float_param.set_sample_rate(sample_rate);

    modulatable_float_param.set_midi_controller(&midi_controller);
    modulatable_float_param.set_value(0.25);
    modulation_level_leader.set_value(0.0);
    modulation_level_leader.schedule_value(2.0, modulation_level_value);
    midi_controller.change(0.1, 0.5);

    assert_false(modulatable_float_param.is_constant_in_next_round(1, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 1
    );
    assert_eq(expected_samples[0], rendered_samples[0], block_size, DOUBLE_DELTA);

    assert_false(modulatable_float_param.is_constant_in_next_round(2, block_size));
    rendered_samples = FloatParam::produce< ModulatableFloatParam<Modulator> >(
        modulatable_float_param, 2
    );
    assert_eq(expected_samples[1], rendered_samples[0], block_size, DOUBLE_DELTA);
})


void set_up_chunk_size_independent_test(
        ModulatableFloatParam<Modulator> param,
        Modulator& modulator,
        FloatParam& modulation_level_leader,
        Integer const block_size,
        Frequency const sample_rate
) {
    modulator.set_block_size(block_size);
    modulator.set_sample_rate(sample_rate);

    modulation_level_leader.set_block_size(block_size);
    modulation_level_leader.set_sample_rate(sample_rate);
    modulation_level_leader.set_value(0.0);
    modulation_level_leader.schedule_linear_ramp(0.25, 1.0);

    param.set_sample_rate(sample_rate);
    param.set_value(0.5);
}


TEST(modulatable_param_rendering_is_independent_of_chunk_size, {
    constexpr Integer block_size = 5000;
    constexpr Frequency sample_rate = 22050.0;
    Modulator modulator_1;
    Modulator modulator_2;
    FloatParam modulation_level_1("MOD", 0.0, 1.0, 0.0);
    FloatParam modulation_level_2("MOD", 0.0, 1.0, 0.0);
    ModulatableFloatParam<Modulator> param_1(
        &modulator_1, modulation_level_1, "", 0.0, 1.0, 0.0
    );
    ModulatableFloatParam<Modulator> param_2(
        &modulator_2, modulation_level_2, "", 0.0, 1.0, 0.0
    );

    set_up_chunk_size_independent_test(
        param_1, modulator_1, modulation_level_1, block_size, sample_rate
    );
    set_up_chunk_size_independent_test(
        param_2, modulator_2, modulation_level_2, block_size, sample_rate
    );

    assert_rendering_is_independent_from_chunk_size< ModulatableFloatParam<Modulator> >(
        param_1, param_2
    );
})
