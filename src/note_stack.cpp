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

#ifndef JS80P__NOTE_STACK_CPP
#define JS80P__NOTE_STACK_CPP

#include <algorithm>

#include "note_stack.hpp"


namespace JS80P
{

NoteStack::NoteStack() noexcept
{
    clear();
}


void NoteStack::clear() noexcept
{
    std::fill_n(linked_list, Midi::NOTES, Midi::INVALID_NOTE);
    std::fill_n(note_index, Midi::NOTES, Midi::INVALID_NOTE);

    top_ = Midi::INVALID_NOTE;
}


bool NoteStack::is_empty() const noexcept
{
    return top_ == Midi::INVALID_NOTE;
}


Midi::Note const NoteStack::top() const noexcept
{
    return top_;
}


void NoteStack::push(Midi::Note const note) noexcept
{
    if (note > Midi::NOTE_MAX) {
        return;
    }

    if (is_already_pushed(note)) {
        remove(note);
    }

    if (top_ != Midi::INVALID_NOTE) {
        note_index[top_] = note;
    }

    linked_list[note] = top_;
    top_ = note;
}


bool NoteStack::is_already_pushed(Midi::Note const note) const noexcept
{
    return top_ == note || note_index[note] != Midi::INVALID_NOTE;
}


Midi::Note const NoteStack::pop() noexcept
{
    if (is_empty()) {
        return Midi::INVALID_NOTE;
    }

    Midi::Note const note = top_;

    top_ = linked_list[note];

    if (top_ != Midi::INVALID_NOTE) {
        note_index[top_] = Midi::INVALID_NOTE;
    }

    linked_list[note] = Midi::INVALID_NOTE;

    return note;
}


void NoteStack::remove(Midi::Note const note) noexcept
{
    if (note > Midi::NOTE_MAX) {
        return;
    }

    Midi::Note const next_note = linked_list[note];

    if (next_note != Midi::INVALID_NOTE) {
        note_index[next_note] = note_index[note];
    }

    if (note == top_) {
        top_ = next_note;
    } else {
        Midi::Note const index = note_index[note];

        if (index != Midi::INVALID_NOTE) {
            linked_list[index] = next_note;
        }

        linked_list[note] = Midi::INVALID_NOTE;
        note_index[note] = Midi::INVALID_NOTE;
    }
}


// void NoteStack::dump() const noexcept
// {
    // fprintf(stderr, "  top:\t%hhx\n  lst:\t[", top_);

    // for (Midi::Note n = 0; n != Midi::NOTES; ++n) {
        // if (linked_list[n] != Midi::INVALID_NOTE) {
            // fprintf(stderr, "%hhx:%hhx, ", n, linked_list[n]);
        // }
    // }

    // fprintf(stderr, "]\n  idx:\t[");

    // for (Midi::Note n = 0; n != Midi::NOTES; ++n) {
        // if (note_index[n] != Midi::INVALID_NOTE) {
            // fprintf(stderr, "%hhx:%hhx, ", n, note_index[n]);
        // }
    // }

    // fprintf(stderr, "]\n\n");
// }

}

#endif
