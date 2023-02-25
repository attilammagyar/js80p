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

#include "synth/midi_controller.cpp"


using namespace JS80P;


TEST(midi_controller_stores_midi_control_change_events, {
    MidiController midi_controller;
    Integer change_index_1;
    Integer change_index_2;
    Integer change_index_3;

    change_index_1 = midi_controller.get_change_index();
    midi_controller.change(1.0, 0.2);
    change_index_2 = midi_controller.get_change_index();
    assert_eq(0.2, midi_controller.get_value());

    midi_controller.change(1.5, 0.5);
    change_index_3 = midi_controller.get_change_index();
    assert_eq(0.5, midi_controller.get_value());

    midi_controller.change(2.0, 0.8);
    assert_eq(0.8, midi_controller.get_value());

    assert_neq((int)change_index_1, (int)change_index_2);
    assert_neq((int)change_index_2, (int)change_index_3);
    assert_neq((int)change_index_3, (int)change_index_1);

    assert_eq(3, (int)midi_controller.events.length());
    assert_eq(1.0, midi_controller.events[0].time_offset, DOUBLE_DELTA);
    assert_eq(0.2, midi_controller.events[0].number_param_1, DOUBLE_DELTA);
    assert_eq(1.5, midi_controller.events[1].time_offset, DOUBLE_DELTA);
    assert_eq(0.5, midi_controller.events[1].number_param_1, DOUBLE_DELTA);
    assert_eq(2.0, midi_controller.events[2].time_offset, DOUBLE_DELTA);
    assert_eq(0.8, midi_controller.events[2].number_param_1, DOUBLE_DELTA);

    midi_controller.clear();
    assert_eq(0, (int)midi_controller.events.length());
})
