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
