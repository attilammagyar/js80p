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
 * \brief A stack (LIFO) for unique \c Midi::Channel and \c Midi::Note pairs
 *        where all operations cost O(1), including removing an element by value
 *        from the middle.
 */
class NoteStack
{
    public:
        NoteStack() noexcept;

        void clear() noexcept;
        bool is_empty() const noexcept;
        bool is_top(Midi::Channel const channel, Midi::Note const note) const noexcept;

        void top(Midi::Channel& channel, Midi::Note& note, Number& velocity) const noexcept;

        void push(
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void pop(Midi::Channel& channel, Midi::Note& note, Number& velocity) noexcept;

        void remove(Midi::Channel const channel, Midi::Note const note) noexcept;

    private:
        static constexpr Midi::Word INVALID_ITEM = Midi::INVALID_NOTE;

        static constexpr size_t ITEMS = Midi::CHANNELS << 8;

        static Midi::Word encode(
            Midi::Channel const channel,
            Midi::Note const note
        ) noexcept;

        static void decode(
            Midi::Word const word,
            Midi::Channel& channel,
            Midi::Note& note
        ) noexcept;

        // void dump() const noexcept;

        bool is_invalid(Midi::Channel const channel, Midi::Note const note) const noexcept;

        void remove(Midi::Word const word) noexcept;

        bool is_already_pushed(Midi::Word const word) const noexcept;

        Number velocities[ITEMS];

        /*
        Since we have a small, finite number of possible elements, and they are
        unique, we can represent the LIFO container as a pair of arrays which
        contain respectively the next and previous pointers of a finite sized
        doubly linked list, and we can use the values themselves as indices
        within the arrays. This way we can both add, remove, and look up
        elements at any position of the container in constant time.

        In other words:

            next[X] = Y if and only if Y is the next element after X
            previous[Y] = X if and only if next[X] = Y
        */
        Midi::Word next[ITEMS];
        Midi::Word previous[ITEMS];

        Midi::Word head;
};

}

#endif
