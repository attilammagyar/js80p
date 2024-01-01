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

#ifndef JS80P__DSP__QUEUE_CPP
#define JS80P__DSP__QUEUE_CPP

#include "dsp/queue.hpp"


namespace JS80P
{

template<class Item>
Queue<Item>::Queue(SizeType const capacity) noexcept
    : next_push(0),
    next_pop(0),
    size(0)
{
    if (capacity > 0) {
        reserve(capacity);
    }
}


template<class Item>
bool Queue<Item>::is_empty() const noexcept
{
    return next_push == next_pop;
}


template<class Item>
void Queue<Item>::reserve(SizeType const capacity) noexcept
{
    items.reserve(capacity);
}


template<class Item>
void Queue<Item>::push(Item const& item) noexcept
{
    if (next_push >= size) {
        items.push_back(item);
        ++next_push;
        ++size;
    } else {
        items[next_push++] = item;
    }
}


template<class Item>
Item const& Queue<Item>::pop() noexcept
{
    Item const& item = items[next_pop++];

    reset_if_empty();

    return item;
}


template<class Item>
void Queue<Item>::reset_if_empty() noexcept
{
    if (is_empty()) {
        next_pop = 0;
        next_push = 0;
    }
}


template<class Item>
Item const& Queue<Item>::front() const noexcept
{
    return items[next_pop];
}


template<class Item>
Item const& Queue<Item>::back() const noexcept
{
    return items[next_push - 1];
}


template<class Item>
typename Queue<Item>::SizeType const Queue<Item>::length() const noexcept
{
    return next_push - next_pop;
}


template<class Item>
Item const& Queue<Item>::operator[](
        typename Queue<Item>::SizeType const index
) const noexcept {
    return items[next_pop + index];
}


template<class Item>
void Queue<Item>::drop(typename Queue<Item>::SizeType const index) noexcept
{
    next_push = next_pop + index;
    reset_if_empty();
}

}

#endif
