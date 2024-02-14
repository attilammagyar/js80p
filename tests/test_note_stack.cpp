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
#include "utils.hpp"

#include "note_stack.cpp"


using namespace JS80P;


#define assert_empty(note_stack)                                                    \
    do {                                                                            \
        Number velocity = -1.0;                                                     \
        Midi::Channel channel = Midi::INVALID_CHANNEL;                              \
        Midi::Note note = 0;                                                        \
                                                                                    \
        assert_true(note_stack.is_empty());                                         \
                                                                                    \
        note_stack.top(channel, note, velocity);                                    \
        assert_eq(Midi::INVALID_NOTE, note);                                        \
        assert_eq(0, channel);                                                      \
        assert_eq(0.0, velocity, DOUBLE_DELTA);                                     \
                                                                                    \
        channel = Midi::INVALID_CHANNEL;                                            \
        note = 0;                                                                   \
        velocity = -1.0;                                                            \
        note_stack.pop(channel, note, velocity);                                    \
        assert_eq(Midi::INVALID_NOTE, note);                                        \
        assert_eq(0, channel);                                                      \
        assert_eq(0.0, velocity, DOUBLE_DELTA);                                     \
    } while (false)

#define assert_top(expected_channel, expected_note, expected_velocity, note_stack)  \
    do {                                                                            \
        Number velocity = -1.0;                                                     \
        Midi::Channel channel = Midi::INVALID_CHANNEL;                              \
        Midi::Note note = Midi::INVALID_NOTE;                                       \
                                                                                    \
        assert_false(note_stack.is_empty());                                        \
        assert_true(note_stack.is_top(expected_channel, expected_note));            \
                                                                                    \
        note_stack.top(channel, note, velocity);                                    \
        assert_eq(expected_channel, channel);                                       \
        assert_eq(expected_note, note);                                             \
        assert_eq(expected_velocity, velocity, DOUBLE_DELTA);                       \
    } while (false)

#define assert_pop(                                                                 \
        expected_popped_channel,                                                    \
        expected_popped_note,                                                       \
        expected_popped_velocity,                                                   \
        expected_top_channel_after_pop,                                             \
        expected_top_note_after_pop,                                                \
        expected_top_velocity_after_pop,                                            \
        note_stack                                                                  \
)                                                                                   \
    do {                                                                            \
        Number velocity = -1.0;                                                     \
        Midi::Channel channel = Midi::INVALID_CHANNEL;                              \
        Midi::Note note = Midi::INVALID_NOTE;                                       \
                                                                                    \
        assert_true(                                                                \
            note_stack.is_top(                                                      \
                expected_popped_channel, expected_popped_note                       \
            )                                                                       \
        );                                                                          \
        note_stack.top(channel, note, velocity);                                    \
        assert_eq(expected_popped_channel, channel);                                \
        assert_eq(expected_popped_note, note);                                      \
        assert_eq(expected_popped_velocity, velocity, DOUBLE_DELTA);                \
                                                                                    \
        channel = Midi::INVALID_CHANNEL;                                            \
        note = Midi::INVALID_NOTE;                                                  \
        velocity = -1.0;                                                            \
        note_stack.pop(channel, note, velocity);                                    \
        assert_eq(expected_popped_channel, channel);                                \
        assert_eq(expected_popped_note, note);                                      \
        assert_eq(expected_popped_velocity, velocity, DOUBLE_DELTA);                \
                                                                                    \
        channel = Midi::INVALID_CHANNEL;                                            \
        note = Midi::INVALID_NOTE;                                                  \
        velocity = -1.0;                                                            \
        note_stack.top(channel, note, velocity);                                    \
        assert_true(                                                                \
            note_stack.is_top(                                                      \
                expected_top_channel_after_pop, expected_top_note_after_pop         \
            )                                                                       \
        );                                                                          \
        assert_eq(expected_top_channel_after_pop, channel);                         \
        assert_eq(expected_top_note_after_pop, note);                               \
        assert_eq(expected_top_velocity_after_pop, velocity, DOUBLE_DELTA);         \
    } while (false)


TEST(note_stack_is_created_empty, {
    NoteStack note_stack;

    assert_empty(note_stack);
})


TEST(when_a_note_is_pushed_on_the_stack_then_stack_is_no_longer_empty_and_the_note_is_on_the_top, {
    NoteStack note_stack;

    note_stack.push(15, Midi::NOTE_A_3, 0.5);

    assert_pop(15, Midi::NOTE_A_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(pushing_an_invalid_note_is_no_op, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_MAX + 1, 0.5);

    assert_empty(note_stack);
})


TEST(pushing_an_invalid_channel_is_no_op, {
    NoteStack note_stack;

    note_stack.push(Midi::INVALID_CHANNEL, Midi::NOTE_A_3, 0.5);

    assert_empty(note_stack);
})


TEST(note_stack_is_a_lifo_container, {
    NoteStack note_stack;

    note_stack.push(1, Midi::NOTE_A_3, 0.2);
    note_stack.push(2, Midi::NOTE_B_3, 0.3);
    note_stack.push(3, Midi::NOTE_C_3, 0.4);
    note_stack.push(4, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.6);

    assert_pop(5, Midi::NOTE_E_3, 0.6, 4, Midi::NOTE_D_3, 0.5, note_stack);
    assert_pop(4, Midi::NOTE_D_3, 0.5, 3, Midi::NOTE_C_3, 0.4, note_stack);
    assert_pop(3, Midi::NOTE_C_3, 0.4, 2, Midi::NOTE_B_3, 0.3, note_stack);
    assert_pop(2, Midi::NOTE_B_3, 0.3, 1, Midi::NOTE_A_3, 0.2, note_stack);
    assert_pop(1, Midi::NOTE_A_3, 0.2, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(when_a_note_stack_is_cleared_then_it_will_become_empty, {
    NoteStack note_stack;

    note_stack.push(1, Midi::NOTE_A_3, 0.5);
    note_stack.push(2, Midi::NOTE_B_3, 0.5);
    note_stack.push(3, Midi::NOTE_C_3, 0.5);
    note_stack.push(4, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);
    note_stack.clear();

    assert_empty(note_stack);
})


TEST(removing_from_empty_stack_is_no_op, {
    NoteStack note_stack;

    note_stack.remove(5, Midi::NOTE_A_3);

    assert_empty(note_stack);
})


TEST(removing_an_invalid_note_is_no_op, {
    NoteStack note_stack;

    note_stack.remove(5, Midi::NOTE_MAX + 1);

    assert_empty(note_stack);
})


TEST(removing_an_invalid_channel_is_no_op, {
    NoteStack note_stack;

    note_stack.remove(Midi::INVALID_CHANNEL, Midi::NOTE_A_3);

    assert_empty(note_stack);
})


TEST(removing_note_which_is_not_in_the_stack_is_no_op, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);

    note_stack.remove(5, Midi::NOTE_E_3);
    note_stack.remove(1, Midi::NOTE_A_3);

    assert_pop(5, Midi::NOTE_C_3, 0.5, 5, Midi::NOTE_B_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_B_3, 0.5, 5, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_A_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(top_note_can_be_removed, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_E_3);

    assert_pop(5, Midi::NOTE_D_3, 0.5, 5, Midi::NOTE_C_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_C_3, 0.5, 5, Midi::NOTE_B_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_B_3, 0.5, 5, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_A_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(stack_can_be_emptied_by_removing_notes_from_the_top, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_E_3);
    assert_top(5, Midi::NOTE_D_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_D_3);
    assert_top(5, Midi::NOTE_C_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_C_3);
    assert_top(5, Midi::NOTE_B_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_B_3);
    assert_top(5, Midi::NOTE_A_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_A_3);
    assert_empty(note_stack);
})


TEST(first_note_can_be_removed, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_A_3);

    assert_pop(5, Midi::NOTE_E_3, 0.5, 5, Midi::NOTE_D_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_D_3, 0.5, 5, Midi::NOTE_C_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_C_3, 0.5, 5, Midi::NOTE_B_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_B_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(note_stack_can_be_emptied_by_removing_notes_from_the_bottom, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_A_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_B_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_C_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_D_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_E_3);
    assert_empty(note_stack);
})


TEST(note_can_be_removed_from_the_middle, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_C_3);

    assert_pop(5, Midi::NOTE_E_3, 0.5, 5, Midi::NOTE_D_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_D_3, 0.5, 5, Midi::NOTE_B_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_B_3, 0.5, 5, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_A_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(all_notes_can_be_removed_from_the_middle, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_C_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_B_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_D_3);

    assert_pop(5, Midi::NOTE_E_3, 0.5, 5, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_A_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(all_notes_can_be_removed_starting_from_the_middle, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_D_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);

    note_stack.remove(5, Midi::NOTE_C_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_B_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_D_3);
    assert_top(5, Midi::NOTE_E_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_E_3);
    assert_top(5, Midi::NOTE_A_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_A_3);
    assert_empty(note_stack);
})


TEST(removing_note_which_is_already_removed_is_no_op, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_B_3, 0.5);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);

    note_stack.remove(5, Midi::NOTE_B_3);
    note_stack.remove(5, Midi::NOTE_B_3);
    assert_top(5, Midi::NOTE_C_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_C_3);
    note_stack.remove(5, Midi::NOTE_C_3);
    assert_top(5, Midi::NOTE_A_3, 0.5, note_stack);

    note_stack.remove(5, Midi::NOTE_A_3);
    note_stack.remove(5, Midi::NOTE_A_3);
    assert_empty(note_stack);
})


TEST(when_a_note_is_pushed_multiple_times_then_only_the_last_instance_remains, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_D_3, 0.1);
    note_stack.push(5, Midi::NOTE_A_3, 0.1);
    note_stack.push(5, Midi::NOTE_D_3, 0.2);
    note_stack.push(5, Midi::NOTE_E_3, 0.2);
    note_stack.push(5, Midi::NOTE_B_3, 0.1);
    note_stack.push(5, Midi::NOTE_E_3, 0.3);
    note_stack.push(5, Midi::NOTE_E_3, 0.4);
    note_stack.push(5, Midi::NOTE_C_3, 0.1);
    note_stack.push(5, Midi::NOTE_E_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.6);
    note_stack.push(5, Midi::NOTE_D_3, 0.3);
    note_stack.push(5, Midi::NOTE_E_3, 0.7);
    note_stack.push(5, Midi::NOTE_E_3, 0.8);

    assert_pop(5, Midi::NOTE_E_3, 0.8, 5, Midi::NOTE_D_3, 0.3, note_stack);
    assert_pop(5, Midi::NOTE_D_3, 0.3, 5, Midi::NOTE_C_3, 0.1, note_stack);
    assert_pop(5, Midi::NOTE_C_3, 0.1, 5, Midi::NOTE_B_3, 0.1, note_stack);
    assert_pop(5, Midi::NOTE_B_3, 0.1, 5, Midi::NOTE_A_3, 0.1, note_stack);
    assert_pop(5, Midi::NOTE_A_3, 0.1, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(same_note_on_different_channel_is_considered_different, {
    NoteStack note_stack;

    note_stack.push(1, Midi::NOTE_A_3, 0.5);
    note_stack.push(2, Midi::NOTE_A_3, 0.5);
    note_stack.push(3, Midi::NOTE_A_3, 0.5);
    note_stack.push(4, Midi::NOTE_A_3, 0.5);
    note_stack.push(5, Midi::NOTE_A_3, 0.5);

    assert_pop(5, Midi::NOTE_A_3, 0.5, 4, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(4, Midi::NOTE_A_3, 0.5, 3, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(3, Midi::NOTE_A_3, 0.5, 2, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(2, Midi::NOTE_A_3, 0.5, 1, Midi::NOTE_A_3, 0.5, note_stack);
    assert_pop(1, Midi::NOTE_A_3, 0.5, 0, Midi::INVALID_NOTE, 0.0, note_stack);
    assert_empty(note_stack);
})


TEST(stays_consistent_after_many_operations, {
    NoteStack note_stack;

    note_stack.push(5, Midi::NOTE_C_3, 0.1);
    note_stack.push(5, Midi::NOTE_B_3, 0.1);
    note_stack.push(5, Midi::NOTE_A_3, 0.1);
    note_stack.push(5, Midi::NOTE_B_3, 0.2);

    note_stack.remove(5, Midi::NOTE_C_3);

    assert_pop(5, Midi::NOTE_B_3, 0.2, 5, Midi::NOTE_A_3, 0.1, note_stack);

    note_stack.push(5, Midi::NOTE_D_3, 0.1);
    note_stack.push(5, Midi::NOTE_E_3, 0.1);
    assert_pop(5, Midi::NOTE_E_3, 0.1, 5, Midi::NOTE_D_3, 0.1, note_stack);

    note_stack.remove(5, Midi::NOTE_D_3);
    assert_top(5, Midi::NOTE_A_3, 0.1, note_stack);

    note_stack.push(5, Midi::NOTE_C_3, 0.2);
    note_stack.push(5, Midi::NOTE_B_3, 0.3);
    note_stack.push(5, Midi::NOTE_C_3, 0.3);
    note_stack.push(5, Midi::NOTE_D_3, 0.2);
    assert_pop(5, Midi::NOTE_D_3, 0.2, 5, Midi::NOTE_C_3, 0.3, note_stack);
    assert_pop(5, Midi::NOTE_C_3, 0.3, 5, Midi::NOTE_B_3, 0.3, note_stack);

    note_stack.remove(5, Midi::NOTE_B_3);
    assert_top(5, Midi::NOTE_A_3, 0.1, note_stack);

    note_stack.push(5, Midi::NOTE_C_3, 0.4);
    note_stack.push(5, Midi::NOTE_B_3, 0.4);
    note_stack.push(5, Midi::NOTE_D_3, 0.3);
    note_stack.push(5, Midi::NOTE_C_3, 0.5);
    note_stack.push(5, Midi::NOTE_E_3, 0.2);
    note_stack.push(5, Midi::NOTE_D_3, 0.4);
    note_stack.push(5, Midi::NOTE_E_3, 0.3);

    assert_pop(5, Midi::NOTE_E_3, 0.3, 5, Midi::NOTE_D_3, 0.4, note_stack);
    assert_pop(5, Midi::NOTE_D_3, 0.4, 5, Midi::NOTE_C_3, 0.5, note_stack);
    assert_pop(5, Midi::NOTE_C_3, 0.5, 5, Midi::NOTE_B_3, 0.4, note_stack);
    assert_pop(5, Midi::NOTE_B_3, 0.4, 5, Midi::NOTE_A_3, 0.1, note_stack);
    assert_pop(5, Midi::NOTE_A_3, 0.1, 0, Midi::INVALID_NOTE, 0.0, note_stack);

    assert_empty(note_stack);
})
