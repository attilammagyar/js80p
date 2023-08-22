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

#ifndef JS80P__SPSCQUEUE_CPP
#define JS80P__SPSCQUEUE_CPP

#include <utility>

#include "spscqueue.hpp"


namespace JS80P
{

template<class ItemClass>
SPSCQueue<ItemClass>::SPSCQueue(SizeType const capacity) noexcept
    : capacity(capacity + 1),
    next_push(0),
    next_pop(0)
{
    items.reserve(capacity);

    for (SizeType i = 0; i != capacity; ++i) {
        items.push_back(ItemClass());
    }
}


template<class ItemClass>
bool SPSCQueue<ItemClass>::is_empty() const noexcept
{
    return length() == 0;
}


template<class ItemClass>
typename SPSCQueue<ItemClass>::SizeType SPSCQueue<ItemClass>::length() const noexcept
{
    size_t const next_pop = this->next_pop.load();
    size_t const next_push = this->next_push.load();

    if (next_push < next_pop) {
        return capacity + next_push - next_pop;
    } else {
        return next_push - next_pop;
    }
}


template<class ItemClass>
bool SPSCQueue<ItemClass>::is_lock_free() const noexcept
{
    return next_push.is_lock_free() && next_pop.is_lock_free();
}


template<class ItemClass>
bool SPSCQueue<ItemClass>::push(ItemClass const& item) noexcept
{
    size_t const old_next_push = next_push.load();
    size_t const next_pop = this->next_pop.load();
    size_t const new_next_push = advance(old_next_push);

    if (next_pop == new_next_push) {
        return false;
    }

    items[old_next_push] = item;
    next_push.store(new_next_push);

    return true;
}


template<class ItemClass>
typename SPSCQueue<ItemClass>::SizeType SPSCQueue<ItemClass>::advance(
        SizeType const index
) const noexcept {
    SizeType const new_index = index + 1;

    return new_index >= capacity ? new_index - capacity : new_index;
}


template<class ItemClass>
bool SPSCQueue<ItemClass>::pop(ItemClass& item) noexcept
{
    size_t const next_pop = this->next_pop.load();
    size_t const next_push = this->next_push.load();

    if (next_push == next_pop) {
        return false;
    }

    item = std::move(items[next_pop]);

    this->next_pop.store(advance(next_pop));

    return true;
}

}

#endif
