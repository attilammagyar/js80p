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

#include "note_stack.cpp"


using namespace JS80P;


#define assert_empty(note_stack)                            \
    do {                                                    \
        assert_true(note_stack.is_empty());                 \
        assert_eq(Midi::INVALID_NOTE, note_stack.top());    \
        assert_eq(Midi::INVALID_NOTE, note_stack.pop());    \
    } while (false)

#define assert_top(expected_note, note_stack)               \
    do {                                                    \
        assert_false(note_stack.is_empty());                \
        assert_eq(expected_note, note_stack.top());         \
    } while (false)

#define assert_pop(expected_popped_note, expected_top_note_after_pop, note_stack)   \
    do {                                                                            \
        assert_top(expected_popped_note, note_stack);                               \
        assert_eq(expected_popped_note, note_stack.pop());                          \
        assert_eq(expected_top_note_after_pop, note_stack.top());                   \
    } while (false)


TEST(note_stack_is_created_empty, {
    NoteStack note_stack;

    assert_empty(note_stack);
})


TEST(when_a_note_is_pushed_on_the_stack_then_stack_is_no_longer_empty_and_the_note_is_on_the_top, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);

    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(pushing_an_invalid_note_is_no_op, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_MAX + 1);

    assert_empty(note_stack);
})


TEST(note_stack_is_a_lifo_container, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    assert_pop(Midi::NOTE_E_3, Midi::NOTE_D_3, note_stack);
    assert_pop(Midi::NOTE_D_3, Midi::NOTE_C_3, note_stack);
    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(when_a_note_stack_is_cleared_then_it_will_become_empty, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.clear();

    assert_empty(note_stack);
})


TEST(removing_from_empty_stack_is_no_op, {
    NoteStack note_stack;

    note_stack.remove(Midi::NOTE_A_3);

    assert_empty(note_stack);
})


TEST(removing_an_invalid_note_is_no_op, {
    NoteStack note_stack;

    note_stack.remove(Midi::NOTE_MAX + 1);

    assert_empty(note_stack);
})


TEST(removing_note_which_is_not_in_the_stack_is_no_op, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);

    note_stack.remove(Midi::NOTE_E_3);

    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(top_note_can_be_removed, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_E_3);

    assert_pop(Midi::NOTE_D_3, Midi::NOTE_C_3, note_stack);
    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(stack_can_be_emptied_by_removing_notes_from_the_top, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_E_3);
    assert_top(Midi::NOTE_D_3, note_stack);

    note_stack.remove(Midi::NOTE_D_3);
    assert_top(Midi::NOTE_C_3, note_stack);

    note_stack.remove(Midi::NOTE_C_3);
    assert_top(Midi::NOTE_B_3, note_stack);

    note_stack.remove(Midi::NOTE_B_3);
    assert_top(Midi::NOTE_A_3, note_stack);

    note_stack.remove(Midi::NOTE_A_3);
    assert_empty(note_stack);
})


TEST(first_note_can_be_removed, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_A_3);

    assert_pop(Midi::NOTE_E_3, Midi::NOTE_D_3, note_stack);
    assert_pop(Midi::NOTE_D_3, Midi::NOTE_C_3, note_stack);
    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(note_stack_can_be_emptied_by_removing_notes_from_the_bottom, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_A_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_B_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_C_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_D_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_E_3);
    assert_empty(note_stack);
})


TEST(note_can_be_removed_from_the_middle, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_C_3);

    assert_pop(Midi::NOTE_E_3, Midi::NOTE_D_3, note_stack);
    assert_pop(Midi::NOTE_D_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(all_notes_can_be_removed_from_the_middle, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_C_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_B_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_D_3);

    assert_pop(Midi::NOTE_E_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(all_notes_can_be_removed_starting_from_the_middle, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    note_stack.remove(Midi::NOTE_C_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_B_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_D_3);
    assert_top(Midi::NOTE_E_3, note_stack);

    note_stack.remove(Midi::NOTE_E_3);
    assert_top(Midi::NOTE_A_3, note_stack);

    note_stack.remove(Midi::NOTE_A_3);
    assert_empty(note_stack);
})


TEST(removing_note_which_is_already_removed_is_no_op, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);

    note_stack.remove(Midi::NOTE_B_3);
    note_stack.remove(Midi::NOTE_B_3);
    assert_top(Midi::NOTE_C_3, note_stack);

    note_stack.remove(Midi::NOTE_C_3);
    note_stack.remove(Midi::NOTE_C_3);
    assert_top(Midi::NOTE_A_3, note_stack);

    note_stack.remove(Midi::NOTE_A_3);
    note_stack.remove(Midi::NOTE_A_3);
    assert_empty(note_stack);
})


TEST(when_a_note_is_pushed_multiple_times_then_only_the_last_instance_remains, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    assert_pop(Midi::NOTE_E_3, Midi::NOTE_D_3, note_stack);
    assert_pop(Midi::NOTE_D_3, Midi::NOTE_C_3, note_stack);
    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);
    assert_empty(note_stack);
})


TEST(stays_consistent_after_many_operations, {
    NoteStack note_stack;

    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_A_3);
    note_stack.push(Midi::NOTE_B_3);

    note_stack.remove(Midi::NOTE_C_3);

    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);

    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);
    assert_pop(Midi::NOTE_E_3, Midi::NOTE_D_3, note_stack);

    note_stack.remove(Midi::NOTE_D_3);
    assert_top(Midi::NOTE_A_3, note_stack);

    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_D_3);
    assert_pop(Midi::NOTE_D_3, Midi::NOTE_C_3, note_stack);
    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);

    note_stack.remove(Midi::NOTE_B_3);
    assert_top(Midi::NOTE_A_3, note_stack);

    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_B_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_C_3);
    note_stack.push(Midi::NOTE_E_3);
    note_stack.push(Midi::NOTE_D_3);
    note_stack.push(Midi::NOTE_E_3);

    assert_pop(Midi::NOTE_E_3, Midi::NOTE_D_3, note_stack);
    assert_pop(Midi::NOTE_D_3, Midi::NOTE_C_3, note_stack);
    assert_pop(Midi::NOTE_C_3, Midi::NOTE_B_3, note_stack);
    assert_pop(Midi::NOTE_B_3, Midi::NOTE_A_3, note_stack);
    assert_pop(Midi::NOTE_A_3, Midi::INVALID_NOTE, note_stack);

    assert_empty(note_stack);
})
