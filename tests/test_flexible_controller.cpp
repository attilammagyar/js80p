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
#include "synth/midi_controller.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"


using namespace JS80P;


TEST(flexible_controller_stores_control_change_events_adjusted_according_to_params, {
    FlexibleController flexible_controller;
    MidiController midi_controller;

    flexible_controller.input.set_midi_controller(&midi_controller);

    midi_controller.change(1.0, 0.2);
    flexible_controller.min.set_value(0.8);
    flexible_controller.max.set_value(0.3);
    flexible_controller.amount.set_value(0.5);
    flexible_controller.distortion.set_value(0.0);
    flexible_controller.randomness.set_value(0.0);
    flexible_controller.update();

    assert_eq(
        0.8 + (0.3 - 0.8) * 0.5 * 0.2,
        flexible_controller.get_value(),
        DOUBLE_DELTA
    );
})


TEST(cyclic_dependencies_are_broken_up, {
    FlexibleController flexible_controller_1("FC1");
    FlexibleController flexible_controller_2("FC2");

    flexible_controller_1.max.set_value(0.5);
    flexible_controller_2.max.set_value(0.5);

    flexible_controller_1.amount.set_flexible_controller(&flexible_controller_2);
    flexible_controller_2.amount.set_flexible_controller(&flexible_controller_1);

    flexible_controller_1.input.set_value(1.0);
    flexible_controller_2.input.set_value(1.0);

    flexible_controller_1.change(0.0, 1.0);
    flexible_controller_2.change(0.0, 1.0);

    flexible_controller_2.update();

    assert_eq(0.5, flexible_controller_1.get_value(), DOUBLE_DELTA);
    assert_eq(0.25, flexible_controller_2.get_value(), DOUBLE_DELTA);

    flexible_controller_2.update();

    assert_eq(0.125, flexible_controller_1.get_value(), DOUBLE_DELTA);
    assert_eq(0.0625, flexible_controller_2.get_value(), DOUBLE_DELTA);
})


TEST(change_index_is_updated_only_when_there_is_an_actual_change, {
    FlexibleController flexible_controller;
    Integer change_index_1;
    Integer change_index_2;
    Integer change_index_3;

    flexible_controller.input.set_value(0.2);
    flexible_controller.min.set_value(0.8);
    flexible_controller.max.set_value(0.3);
    flexible_controller.amount.set_value(0.5);
    flexible_controller.distortion.set_value(0.0);
    flexible_controller.randomness.set_value(0.0);

    change_index_1 = flexible_controller.get_change_index();
    flexible_controller.update();
    change_index_2 = flexible_controller.get_change_index();
    flexible_controller.update();
    change_index_3 = flexible_controller.get_change_index();

    assert_neq((int)change_index_1, (int)change_index_2);
    assert_eq((int)change_index_2, (int)change_index_3);
})


void assert_flexible_controller_value(
        FlexibleController& flexible_controller,
        Number const input_value,
        Number const expected_value,
        Number const tolerance = 0.01
) {
    flexible_controller.input.set_value(input_value);
    flexible_controller.update();
    assert_eq(
        expected_value,
        flexible_controller.get_value(),
        tolerance,
        "input=%f",
        input_value
    );
}


TEST(can_distort_the_value, {
    constexpr Number min = 0.1;
    constexpr Number max = 0.8;
    constexpr Number amount = 0.7;
    constexpr Number adjusted_max = (max - min) * amount;
    FlexibleController flexible_controller;

    flexible_controller.min.set_value(min);
    flexible_controller.max.set_value(max);
    flexible_controller.amount.set_value(amount);
    flexible_controller.distortion.set_value(1.0);
    flexible_controller.randomness.set_value(0.0);

    assert_flexible_controller_value(flexible_controller, 0.0, min);
    assert_flexible_controller_value(flexible_controller, 0.1, min);
    assert_flexible_controller_value(flexible_controller, 0.2, min);
    assert_flexible_controller_value(
        flexible_controller, 0.5, min + adjusted_max / 2.0
    );
    assert_flexible_controller_value(flexible_controller, 0.8, min + adjusted_max);
    assert_flexible_controller_value(flexible_controller, 0.9, min + adjusted_max);
    assert_flexible_controller_value(flexible_controller, 1.0, min + adjusted_max);
})


TEST(can_randomize_the_value, {
    constexpr Integer probes = 500;
    constexpr Number min = 0.1;
    constexpr Number max = 0.8;
    constexpr Number amount = 0.7;
    constexpr Number mean = (min + max * amount) / 2.0;
    std::vector<Number> numbers(probes);
    FlexibleController flexible_controller;
    Math::Statistics statistics;

    flexible_controller.min.set_value(min);
    flexible_controller.max.set_value(max);
    flexible_controller.amount.set_value(amount);
    flexible_controller.distortion.set_value(0.0);
    flexible_controller.randomness.set_value(1.0);

    for (Integer i = 0; i != probes; ++i) {
        flexible_controller.input.set_value((Number)i / (Number)probes);
        flexible_controller.update();
        numbers[i] = flexible_controller.get_value();
    }

    Math::compute_statistics(numbers, statistics);

    assert_statistics(
        true, min, mean, amount * max, mean, (mean - min) / 2.0, statistics, 0.025
    );
})
