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

#ifndef JS80P__NOTE_STACK_HPP
#define JS80P__NOTE_STACK_HPP

#include <vector>

#include "js80p.hpp"
#include "midi.hpp"


namespace JS80P
{

/**
 * \brief A stack (LIFO) for unique \c Midi::Note values where all operations
 *        cost O(1), including removing an element by value from the middle.
 */
class NoteStack
{
    public:
        NoteStack() noexcept;

        bool is_empty() const noexcept;
        void clear() noexcept;
        Midi::Note const top() const noexcept;
        void push(Midi::Note const note) noexcept;
        Midi::Note const pop() noexcept;
        void remove(Midi::Note const note) noexcept;

    private:
        // void dump() const noexcept;
        bool is_already_pushed(Midi::Note const note) const noexcept;

        /* linked_list[X] = Y if and only if Y is the next element after X */
        Midi::Note linked_list[Midi::NOTES];

        /* note_index[X] = Y if and only if linked_list[Y] = X */
        Midi::Note note_index[Midi::NOTES];

        Midi::Note top_;
};

}

#endif
