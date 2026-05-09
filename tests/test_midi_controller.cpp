/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
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
#include "midi.hpp"

#include "dsp/midi_controller.cpp"


using namespace JS80P;


TEST(midi_controller_stores_midi_control_change_events, {
    constexpr Midi::Channel channel = 0;

    MidiController midi_ctl;
    Integer change_index_1;
    Integer change_index_2;
    Integer change_index_3;

    change_index_1 = midi_ctl.get_change_index(channel);
    midi_ctl.change(channel, 1.0, 0.2);
    change_index_2 = midi_ctl.get_change_index(channel);
    assert_eq(0.2, midi_ctl.get_value(channel), DOUBLE_DELTA);

    midi_ctl.change(channel, 1.5, 0.5);
    change_index_3 = midi_ctl.get_change_index(channel);
    assert_eq(0.5, midi_ctl.get_value(channel), DOUBLE_DELTA);

    midi_ctl.change(channel, 2.0, 0.8);
    assert_eq(0.8, midi_ctl.get_value(channel), DOUBLE_DELTA);

    assert_neq((int)change_index_1, (int)change_index_2);
    assert_neq((int)change_index_2, (int)change_index_3);
    assert_neq((int)change_index_3, (int)change_index_1);

    assert_eq(3, (int)midi_ctl.event_queues[channel].length());
    assert_eq(1.0, midi_ctl.event_queues[channel][0].time_offset, DOUBLE_DELTA);
    assert_eq(
        0.2, midi_ctl.event_queues[channel][0].number_param_1, DOUBLE_DELTA
    );
    assert_eq(1.5, midi_ctl.event_queues[channel][1].time_offset, DOUBLE_DELTA);
    assert_eq(
        0.5, midi_ctl.event_queues[channel][1].number_param_1, DOUBLE_DELTA
    );
    assert_eq(2.0, midi_ctl.event_queues[channel][2].time_offset, DOUBLE_DELTA);
    assert_eq(
        0.8, midi_ctl.event_queues[channel][2].number_param_1, DOUBLE_DELTA
    );

    midi_ctl.clear();
    assert_eq(0, (int)midi_ctl.event_queues[channel].length());
})


TEST(channels_are_independent_from_each_other, {
    constexpr Midi::Channel channel_1 = 1;
    constexpr Midi::Channel channel_2 = 5;
    constexpr Midi::Channel channel_3 = 10;

    MidiController midi_ctl;
    Integer change_index_ch1_1;
    Integer change_index_ch1_2;
    Integer change_index_ch1_3;
    Integer change_index_ch2_1;
    Integer change_index_ch2_2;
    Integer change_index_ch2_3;

    change_index_ch1_1 = midi_ctl.get_change_index(channel_1);
    change_index_ch2_1 = midi_ctl.get_change_index(channel_2);
    midi_ctl.change(channel_1, 1.0, 0.2);
    change_index_ch1_2 = midi_ctl.get_change_index(channel_1);
    change_index_ch2_2 = midi_ctl.get_change_index(channel_2);
    assert_eq(0.2, midi_ctl.get_value(channel_1), DOUBLE_DELTA);

    midi_ctl.change(channel_2, 1.5, 0.3);
    change_index_ch1_3 = midi_ctl.get_change_index(channel_1);
    change_index_ch2_3 = midi_ctl.get_change_index(channel_2);
    assert_eq(0.2, midi_ctl.get_value(channel_1), DOUBLE_DELTA);
    assert_eq(0.3, midi_ctl.get_value(channel_2), DOUBLE_DELTA);

    assert_neq((int)change_index_ch1_1, (int)change_index_ch1_2);
    assert_eq((int)change_index_ch2_1, (int)change_index_ch2_2);
    assert_eq((int)change_index_ch1_2, (int)change_index_ch1_3);
    assert_neq((int)change_index_ch2_2, (int)change_index_ch2_3);
    assert_neq((int)change_index_ch1_3, (int)change_index_ch1_1);
    assert_neq((int)change_index_ch2_3, (int)change_index_ch2_1);

    assert_eq(1, (int)midi_ctl.event_queues[channel_1].length());
    assert_eq(1, (int)midi_ctl.event_queues[channel_2].length());
    assert_eq(0, (int)midi_ctl.event_queues[channel_3].length());

    assert_eq(
        1.0, midi_ctl.event_queues[channel_1][0].time_offset, DOUBLE_DELTA
    );
    assert_eq(
        0.2, midi_ctl.event_queues[channel_1][0].number_param_1, DOUBLE_DELTA
    );
    assert_eq(
        1.5, midi_ctl.event_queues[channel_2][0].time_offset, DOUBLE_DELTA
    );
    assert_eq(
        0.3, midi_ctl.event_queues[channel_2][0].number_param_1, DOUBLE_DELTA
    );

    midi_ctl.clear();
    assert_eq(0, (int)midi_ctl.event_queues[channel_1].length());
    assert_eq(0, (int)midi_ctl.event_queues[channel_2].length());
    assert_eq(0, (int)midi_ctl.event_queues[channel_3].length());
})


TEST(can_change_all_channels_at_once, {
    constexpr Midi::Channel channel_1 = 1;
    constexpr Midi::Channel channel_2 = 5;

    MidiController midi_ctl;
    Integer change_index_ch1_1;
    Integer change_index_ch1_2;
    Integer change_index_ch1_3;
    Integer change_index_ch2_1;
    Integer change_index_ch2_2;
    Integer change_index_ch2_3;

    change_index_ch1_1 = midi_ctl.get_change_index(channel_1);
    change_index_ch2_1 = midi_ctl.get_change_index(channel_2);
    midi_ctl.change(channel_1, 1.0, 0.2);
    change_index_ch1_2 = midi_ctl.get_change_index(channel_1);
    change_index_ch2_2 = midi_ctl.get_change_index(channel_2);
    assert_eq(0.2, midi_ctl.get_value(channel_1), DOUBLE_DELTA);

    midi_ctl.change_all_channels(1.5, 0.3);
    change_index_ch1_3 = midi_ctl.get_change_index(channel_1);
    change_index_ch2_3 = midi_ctl.get_change_index(channel_2);
    assert_eq(0.3, midi_ctl.get_value(channel_1), DOUBLE_DELTA);
    assert_eq(0.3, midi_ctl.get_value(channel_2), DOUBLE_DELTA);

    assert_neq((int)change_index_ch1_1, (int)change_index_ch1_2);
    assert_eq((int)change_index_ch2_1, (int)change_index_ch2_2);
    assert_neq((int)change_index_ch1_2, (int)change_index_ch1_3);
    assert_neq((int)change_index_ch2_2, (int)change_index_ch2_3);
    assert_neq((int)change_index_ch1_3, (int)change_index_ch1_1);
    assert_neq((int)change_index_ch2_3, (int)change_index_ch2_1);

    assert_eq(2, (int)midi_ctl.event_queues[channel_1].length());
    assert_eq(1, (int)midi_ctl.event_queues[channel_2].length());

    assert_eq(
        1.0, midi_ctl.event_queues[channel_1][0].time_offset, DOUBLE_DELTA
    );
    assert_eq(
        0.2, midi_ctl.event_queues[channel_1][0].number_param_1, DOUBLE_DELTA
    );
    assert_eq(
        1.5, midi_ctl.event_queues[channel_1][1].time_offset, DOUBLE_DELTA
    );
    assert_eq(
        0.3, midi_ctl.event_queues[channel_1][1].number_param_1, DOUBLE_DELTA
    );
    assert_eq(
        1.5, midi_ctl.event_queues[channel_2][0].time_offset, DOUBLE_DELTA
    );
    assert_eq(
        0.3, midi_ctl.event_queues[channel_2][0].number_param_1, DOUBLE_DELTA
    );

    midi_ctl.clear();
    assert_eq(0, (int)midi_ctl.event_queues[channel_1].length());
    assert_eq(0, (int)midi_ctl.event_queues[channel_2].length());
})


TEST(keeps_track_of_assignments, {
    MidiController midi_ctl;

    assert_false(midi_ctl.is_assigned());

    midi_ctl.assigned();
    assert_true(midi_ctl.is_assigned());

    midi_ctl.assigned();
    assert_true(midi_ctl.is_assigned());

    midi_ctl.released();
    assert_true(midi_ctl.is_assigned());

    midi_ctl.released();
    assert_false(midi_ctl.is_assigned());

    midi_ctl.released();
    assert_false(midi_ctl.is_assigned());

    midi_ctl.assigned();
    assert_true(midi_ctl.is_assigned());
})
