/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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


TEST(macro_stores_control_change_events_adjusted_according_to_params, {
    Macro macro;
    MidiController midi_controller;

    macro.input.set_midi_controller(&midi_controller);

    midi_controller.change(1.0, 0.2);
    macro.min.set_value(0.8);
    macro.max.set_value(0.3);
    macro.amount.set_value(0.5);
    macro.distortion.set_value(0.0);
    macro.randomness.set_value(0.0);
    macro.update();

    assert_eq(0.8 + (0.3 - 0.8) * 0.5 * 0.2, macro.get_value(), DOUBLE_DELTA);
})


TEST(cyclic_dependencies_are_broken_up, {
    Macro macro_1("M1");
    Macro macro_2("M2");

    macro_1.max.set_value(0.5);
    macro_2.max.set_value(0.5);

    macro_1.amount.set_macro(&macro_2);
    macro_2.amount.set_macro(&macro_1);

    macro_1.input.set_value(1.0);
    macro_2.input.set_value(1.0);

    macro_1.change(0.0, 1.0);
    macro_2.change(0.0, 1.0);

    macro_2.update();

    assert_eq(0.5, macro_1.get_value(), DOUBLE_DELTA);
    assert_eq(0.25, macro_2.get_value(), DOUBLE_DELTA);

    macro_2.update();

    assert_eq(0.125, macro_1.get_value(), DOUBLE_DELTA);
    assert_eq(0.0625, macro_2.get_value(), DOUBLE_DELTA);
})


TEST(change_index_is_updated_only_when_there_is_an_actual_change, {
    Macro macro;
    Integer change_index_1;
    Integer change_index_2;
    Integer change_index_3;

    macro.input.set_value(0.2);
    macro.min.set_value(0.8);
    macro.max.set_value(0.3);
    macro.amount.set_value(0.5);
    macro.distortion.set_value(0.0);
    macro.randomness.set_value(0.0);

    change_index_1 = macro.get_change_index();
    macro.update();
    change_index_2 = macro.get_change_index();
    macro.update();
    change_index_3 = macro.get_change_index();

    assert_neq((int)change_index_1, (int)change_index_2);
    assert_eq((int)change_index_2, (int)change_index_3);
})


Number apply_macro(Macro& macro, Number const input_value)
{
    macro.input.set_value(input_value);
    macro.update();

    return macro.get_value();
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


TEST(can_distort_the_value, {
    constexpr Number min = 0.1;
    constexpr Number max = 0.8;
    constexpr Number amount = 0.7;
    constexpr Number adjusted_max = (max - min) * amount;
    Macro macro;

    macro.min.set_value(min);
    macro.max.set_value(max);
    macro.amount.set_value(amount);
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


TEST(distortion_shape_can_be_changed, {
    Macro macro;

    macro.distortion.set_value(0.5);

    macro.distortion_shape.set_value(Macro::DIST_SHAPE_SMOOTH_SHARP);
    assert_eq(0.00, apply_macro(macro, 0.00), DOUBLE_DELTA, "smooth-sharp");
    assert_gt(0.25, apply_macro(macro, 0.25), "smooth-sharp");
    assert_gt(0.50, apply_macro(macro, 0.50), "smooth-sharp");
    assert_gt(0.75, apply_macro(macro, 0.75), "smooth-sharp");
    assert_eq(1.00, apply_macro(macro, 1.00), DOUBLE_DELTA, "smooth-sharp");

    macro.distortion_shape.set_value(Macro::DIST_SHAPE_SHARP_SMOOTH);
    assert_eq(0.00, apply_macro(macro, 0.00), DOUBLE_DELTA, "sharp-smooth");
    assert_lt(0.25, apply_macro(macro, 0.25), "sharp-smooth");
    assert_lt(0.50, apply_macro(macro, 0.50), "sharp-smooth");
    assert_lt(0.75, apply_macro(macro, 0.75), "sharp-smooth");
    assert_eq(1.00, apply_macro(macro, 1.00), DOUBLE_DELTA, "sharp-smooth");

    macro.distortion_shape.set_value(Macro::DIST_SHAPE_SHARP_SHARP);
    assert_eq(0.00, apply_macro(macro, 0.00), DOUBLE_DELTA, "sharp-sharp");
    assert_lt(0.25, apply_macro(macro, 0.25), "sharp-sharp");
    assert_eq(0.50, apply_macro(macro, 0.50), DOUBLE_DELTA, "sharp-sharp");
    assert_gt(0.75, apply_macro(macro, 0.75), "sharp-sharp");
    assert_eq(1.00, apply_macro(macro, 1.00), DOUBLE_DELTA, "sharp-sharp");
})


TEST(can_randomize_the_value, {
    constexpr Integer probes = 500;
    constexpr Number min = 0.1;
    constexpr Number max = 0.8;
    constexpr Number amount = 0.7;
    constexpr Number mean = (min + max * amount) / 2.0;
    std::vector<Number> numbers(probes);
    Macro macro;
    Math::Statistics statistics;

    macro.min.set_value(min);
    macro.max.set_value(max);
    macro.amount.set_value(amount);
    macro.distortion.set_value(0.0);
    macro.randomness.set_value(1.0);

    for (Integer i = 0; i != probes; ++i) {
        macro.input.set_value((Number)i / (Number)probes);
        macro.update();
        numbers[i] = macro.get_value();
    }

    Math::compute_statistics(numbers, statistics);

    assert_statistics(
        true, min, mean, amount * max, mean, (mean - min) / 2.0, statistics, 0.025
    );
})
