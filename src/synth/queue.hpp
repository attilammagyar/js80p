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

#ifndef JS80P__SYNTH__QUEUE_HPP
#define JS80P__SYNTH__QUEUE_HPP

#include <vector>


namespace JS80P
{

/**
 * \brief A FIFO container for \c SignalProducer events, which can drop
 *        all items after a given index, and all operations run in constant
 *        time.
 */
template<class Item>
class Queue
{
    public:
        typedef typename std::vector<Item>::size_type SizeType;

        // One shouldn't (re)allocate memory in the audio thread - using a
        // dynamically growing std::vector here is cheating, but it should
        // settle after a while.
        static constexpr SizeType RESERVED = 32;

        Queue();

        bool is_empty() const;
        void push(Item const& item);
        Item const& pop();
        Item const& front() const;
        Item const& back() const;
        SizeType const length() const;
        Item const& operator[](SizeType const index) const;
        void drop(SizeType const index);

    protected:
        std::vector<Item> items;

    private:
        void reset_if_empty();

        SizeType next_push;
        SizeType next_pop;
        SizeType size;
};

}

#endif
