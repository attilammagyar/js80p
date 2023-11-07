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

Midi::Word NoteStack::encode(
    Midi::Channel const channel,
    Midi::Note const note
) noexcept {
    return ((channel & 0x0f) << 8) | (note & 0xff);
}


void NoteStack::decode(
    Midi::Word const item,
    Midi::Channel& channel,
    Midi::Note& note
) noexcept {
    channel = (item >> 8) & 0x0f;
    note = item & 0xff;
}


NoteStack::NoteStack() noexcept
{
    clear();
}


void NoteStack::clear() noexcept
{
    std::fill_n(linked_list, ITEMS, INVALID_ITEM);
    std::fill_n(index, ITEMS, INVALID_ITEM);
    std::fill_n(velocities, ITEMS, 0.0);

    top_ = INVALID_ITEM;
}


bool NoteStack::is_empty() const noexcept
{
    return top_ == INVALID_ITEM;
}


bool NoteStack::is_top(Midi::Channel const channel, Midi::Note const note) const noexcept
{
    return top_ == encode(channel, note);
}


void NoteStack::top(Midi::Channel& channel, Midi::Note& note, Number& velocity) const noexcept
{
    decode(top_, channel, note);
    velocity = is_empty() ? 0.0 : velocities[top_];
}


void NoteStack::push(
        Midi::Channel const channel,
        Midi::Note const note,
        Number const velocity
) noexcept {
    if (JS80P_UNLIKELY(is_invalid(channel, note))) {
        return;
    }

    Midi::Word const item = encode(channel, note);

    if (is_already_pushed(item)) {
        remove(item);
    }

    if (top_ != INVALID_ITEM) {
        index[top_] = item;
    }

    linked_list[item] = top_;
    top_ = item;
    velocities[item] = velocity;
}


bool NoteStack::is_invalid(
        Midi::Channel const channel,
        Midi::Note const note
) const noexcept {
    return channel > Midi::CHANNEL_MAX || note > Midi::NOTE_MAX;
}


bool NoteStack::is_already_pushed(Midi::Word const item) const noexcept
{
    return top_ == item || index[item] != INVALID_ITEM;
}


void NoteStack::pop(Midi::Channel& channel, Midi::Note& note, Number& velocity) noexcept
{
    if (is_empty()) {
        decode(INVALID_ITEM, channel, note);
        velocity = 0.0;

        return;
    }

    Midi::Word const item = top_;

    top_ = linked_list[item];

    if (top_ != INVALID_ITEM) {
        index[top_] = INVALID_ITEM;
    }

    linked_list[item] = INVALID_ITEM;

    decode(item, channel, note);
    velocity = velocities[item];
}


void NoteStack::remove(Midi::Channel const channel, Midi::Note const note) noexcept
{
    if (JS80P_UNLIKELY(is_invalid(channel, note))) {
        return;
    }

    remove(encode(channel, note));
}

void NoteStack::remove(Midi::Word const item) noexcept
{
    Midi::Word const next_item = linked_list[item];
    Midi::Word const key = index[item];

    if (next_item != INVALID_ITEM) {
        index[next_item] = key;
    }

    if (item == top_) {
        top_ = next_item;
    } else {
        if (key != INVALID_ITEM) {
            linked_list[key] = next_item;
        }

        linked_list[item] = INVALID_ITEM;
        index[item] = INVALID_ITEM;
    }
}


// void NoteStack::dump() const noexcept
// {
    // Midi::Channel channel;
    // Midi::Note note;

    // fprintf(stderr, "  top:\t%hhx\n  lst:\t[", top_);

    // for (Midi::Word i = 0; i != ITEMS; ++i) {
        // if (linked_list[i] != INVALID_ITEM) {
            // decode(linked_list[i], channel, note);
            // fprintf(stderr, "%hhx:(%hhx,%hhx), ", i, channel, note);
        // }
    // }

    // fprintf(stderr, "]\n  idx:\t[");

    // for (Midi::Word i = 0; i != ITEMS; ++i) {
        // if (index[i] != INVALID_ITEM) {
            // decode(index[i], channel, note);
            // fprintf(stderr, "%hhx:(%hhx,%hhx), ", i, channel, note);
        // }
    // }

    // fprintf(stderr, "]\n\n");
// }

}

#endif
