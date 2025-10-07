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

#include <vector>

#include "js80p.hpp"
#include "midi.hpp"

#include "dsp/envelope.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


TEST(macros_adjust_control_change_events, {
    Macro macro;
    MidiController midi_controller;

    macro.input.set_midi_controller(&midi_controller);

    midi_controller.change(PARAM_DEFAULT_MPE_CHANNEL, 1.0, 0.2);
    macro.min.set_value(0.8);
    macro.max.set_value(0.3);
    macro.scale.set_value(0.5);
    macro.distortion.set_value(0.0);
    macro.randomness.set_value(0.0);
    macro.update(PARAM_DEFAULT_MPE_CHANNEL);

    assert_eq(
        0.8 + (0.3 - 0.8) * 0.5 * 0.2,
        macro.get_value(PARAM_DEFAULT_MPE_CHANNEL),
        DOUBLE_DELTA
    );
})


TEST(circular_dependencies_between_macros_are_broken_up, {
    Macro macro_1("M1");
    Macro macro_2("M2");

    macro_1.max.set_value(0.5);
    macro_2.max.set_value(0.5);

    macro_1.scale.set_macro(&macro_2);
    macro_2.scale.set_macro(&macro_1);

    macro_1.input.set_value(1.0);
    macro_2.input.set_value(1.0);

    macro_1.change(PARAM_DEFAULT_MPE_CHANNEL, 0.0, 1.0);
    macro_2.change(PARAM_DEFAULT_MPE_CHANNEL, 0.0, 1.0);

    macro_2.update(PARAM_DEFAULT_MPE_CHANNEL);

    assert_eq(0.5, macro_1.get_value(PARAM_DEFAULT_MPE_CHANNEL), DOUBLE_DELTA);
    assert_eq(0.25, macro_2.get_value(PARAM_DEFAULT_MPE_CHANNEL), DOUBLE_DELTA);

    macro_2.update(PARAM_DEFAULT_MPE_CHANNEL);

    assert_eq(0.125, macro_1.get_value(PARAM_DEFAULT_MPE_CHANNEL), DOUBLE_DELTA);
    assert_eq(0.0625, macro_2.get_value(PARAM_DEFAULT_MPE_CHANNEL), DOUBLE_DELTA);
})


TEST(macro_change_index_is_updated_only_when_there_is_an_actual_change, {
    Macro macro;
    Integer change_index_1;
    Integer change_index_2;
    Integer change_index_3;

    macro.input.set_value(0.2);
    macro.min.set_value(0.8);
    macro.max.set_value(0.3);
    macro.scale.set_value(0.5);
    macro.distortion.set_value(0.0);
    macro.randomness.set_value(0.0);

    change_index_1 = macro.get_change_index(PARAM_DEFAULT_MPE_CHANNEL);
    macro.update(PARAM_DEFAULT_MPE_CHANNEL);
    change_index_2 = macro.get_change_index(PARAM_DEFAULT_MPE_CHANNEL);
    macro.update(PARAM_DEFAULT_MPE_CHANNEL);
    change_index_3 = macro.get_change_index(PARAM_DEFAULT_MPE_CHANNEL);

    assert_neq((int)change_index_1, (int)change_index_2);
    assert_eq((int)change_index_2, (int)change_index_3);
})


Number apply_macro(Macro& macro, Number const input_value)
{
    macro.input.set_value(input_value);
    macro.update(PARAM_DEFAULT_MPE_CHANNEL);

    return macro.get_value(PARAM_DEFAULT_MPE_CHANNEL);
}


void assert_macro_value(
        Macro& macro,
        Number const input_value,
        Number const expected_value,
        Number const tolerance = 0.01
) {
    Number const value = apply_macro(macro, input_value);

    assert_eq(expected_value, value, tolerance, "input=%f", input_value);
}


TEST(macro_value_can_be_distorted, {
    constexpr Number min = 0.1;
    constexpr Number max = 0.8;
    constexpr Number scale = 0.7;
    constexpr Number adjusted_max = (max - min) * scale;
    Macro macro;

    macro.min.set_value(min);
    macro.max.set_value(max);
    macro.scale.set_value(scale);
    macro.distortion.set_value(1.0);
    macro.randomness.set_value(0.0);

    assert_macro_value(macro, 0.0, min);
    assert_macro_value(macro, 0.1, min);
    assert_macro_value(macro, 0.2, min);
    assert_macro_value(macro, 0.5, min + adjusted_max / 2.0);
    assert_macro_value(macro, 0.8, min + adjusted_max);
    assert_macro_value(macro, 0.9, min + adjusted_max);
    assert_macro_value(macro, 1.0, min + adjusted_max);
})


TEST(macro_distortion_curve_can_be_changed, {
    Macro macro;

    macro.distortion.set_value(0.5);

    macro.distortion_curve.set_value(Macro::DIST_CURVE_SMOOTH_SHARP);
    assert_eq(0.00, apply_macro(macro, 0.00), DOUBLE_DELTA, "smooth-sharp");
    assert_gt(0.25, apply_macro(macro, 0.25), "smooth-sharp");
    assert_gt(0.50, apply_macro(macro, 0.50), "smooth-sharp");
    assert_gt(0.75, apply_macro(macro, 0.75), "smooth-sharp");
    assert_eq(1.00, apply_macro(macro, 1.00), DOUBLE_DELTA, "smooth-sharp");

    macro.distortion_curve.set_value(Macro::DIST_CURVE_SHARP_SMOOTH);
    assert_eq(0.00, apply_macro(macro, 0.00), DOUBLE_DELTA, "sharp-smooth");
    assert_lt(0.25, apply_macro(macro, 0.25), "sharp-smooth");
    assert_lt(0.50, apply_macro(macro, 0.50), "sharp-smooth");
    assert_lt(0.75, apply_macro(macro, 0.75), "sharp-smooth");
    assert_eq(1.00, apply_macro(macro, 1.00), DOUBLE_DELTA, "sharp-smooth");

    macro.distortion_curve.set_value(Macro::DIST_CURVE_SHARP_SHARP);
    assert_eq(0.00, apply_macro(macro, 0.00), DOUBLE_DELTA, "sharp-sharp");
    assert_lt(0.25, apply_macro(macro, 0.25), "sharp-sharp");
    assert_eq(0.50, apply_macro(macro, 0.50), DOUBLE_DELTA, "sharp-sharp");
    assert_gt(0.75, apply_macro(macro, 0.75), "sharp-sharp");
    assert_eq(1.00, apply_macro(macro, 1.00), DOUBLE_DELTA, "sharp-sharp");
})


TEST(macro_value_can_be_randomized, {
    constexpr Integer probes = 500;
    constexpr Number min = 0.1;
    constexpr Number max = 0.8;
    constexpr Number scale = 0.7;
    constexpr Number mean = (min + max * scale) / 2.0;

    std::vector<Number> numbers(probes);
    Macro macro;
    Math::Statistics statistics;

    macro.min.set_value(min);
    macro.max.set_value(max);
    macro.scale.set_value(scale);
    macro.distortion.set_value(0.0);
    macro.randomness.set_value(1.0);

    for (Integer i = 0; i != probes; ++i) {
        macro.input.set_value((Number)i / (Number)probes);
        macro.update(PARAM_DEFAULT_MPE_CHANNEL);
        numbers[i] = macro.get_value(PARAM_DEFAULT_MPE_CHANNEL);
    }

    Math::compute_statistics(numbers, statistics);

    assert_statistics(
        true, min, mean, scale * max, mean, (mean - min) / 2.0, statistics, 0.025
    );
})


TEST(macro_value_midpoint_can_be_shifted, {
    Macro macro;

    macro.midpoint.set_value(0.7);

    assert_macro_value(macro, 0.00, 0.00);
    assert_macro_value(macro, 0.25, 0.35);
    assert_macro_value(macro, 0.50, 0.70);
    assert_macro_value(macro, 0.75, 0.85);
    assert_macro_value(macro, 1.00, 1.00);

    macro.distortion.set_value(1.0);

    assert_macro_value(macro, 0.00, 0.00);
    assert_macro_value(macro, 0.25, 0.00);
    assert_macro_value(macro, 0.50, 0.99);
    assert_macro_value(macro, 0.75, 0.99);
    assert_macro_value(macro, 1.00, 1.00);

    macro.min.set_value(0.1);
    macro.max.set_value(0.8);
    macro.distortion.set_value(0.0);

    assert_macro_value(macro, 0.00, 0.10);
    assert_macro_value(macro, 0.25, 0.10 + 0.35 * (0.80 - 0.10));
    assert_macro_value(macro, 0.50, 0.10 + 0.70 * (0.80 - 0.10));
    assert_macro_value(macro, 0.75, 0.10 + 0.85 * (0.80 - 0.10));
    assert_macro_value(macro, 1.00, 0.80);
})


TEST(macro_can_modify_midi_controller_channels_independently_from_each_other, {
    constexpr Midi::Channel channel_1 = 1;
    constexpr Midi::Channel channel_2 = 2;

    Macro macro_1;
    Macro macro_2;
    MidiController midi_controller;

    macro_1.input.set_midi_controller(&midi_controller);
    macro_1.min.set_value(1.0);
    macro_1.max.set_value(0.0);

    macro_2.input.set_macro(&macro_1);
    macro_2.min.set_value(0.8);
    macro_2.max.set_value(0.3);
    macro_2.scale.set_value(0.5);
    macro_2.distortion.set_value(0.0);
    macro_2.randomness.set_value(0.0);

    midi_controller.change(channel_1, 1.0, 0.2);
    midi_controller.change(channel_2, 1.0, 0.7);
    macro_2.update(channel_1);
    macro_2.update(channel_2);

    assert_eq(
        0.8 + (0.3 - 0.8) * 0.5 * (1.0 - 0.2),
        macro_2.get_value(channel_1),
        DOUBLE_DELTA
    );
    assert_eq(
        0.8 + (0.3 - 0.8) * 0.5 * (1.0 - 0.7),
        macro_2.get_value(channel_2),
        DOUBLE_DELTA
    );
})
