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
    return ((Midi::Word)(channel & 0x0f) << 8) | (Midi::Word)note;
}


void NoteStack::decode(
    Midi::Word const word,
    Midi::Channel& channel,
    Midi::Note& note
) noexcept {
    channel = (Midi::Channel)(word >> 8) & 0x0f;
    note = get_note(word);
}


Midi::Note NoteStack::get_note(Midi::Word const word) noexcept
{
    return word & 0xff;
}


NoteStack::NoteStack() noexcept
{
    clear();
}


void NoteStack::clear() noexcept
{
    std::fill_n(next, ITEMS, INVALID_ITEM);
    std::fill_n(previous, ITEMS, INVALID_ITEM);
    std::fill_n(velocities, ITEMS, 0.0);

    head = INVALID_ITEM;
    oldest_ = INVALID_ITEM;
    lowest_ = INVALID_ITEM;
    highest_ = INVALID_ITEM;
}


bool NoteStack::is_empty() const noexcept
{
    return head == INVALID_ITEM;
}


bool NoteStack::is_top(Midi::Channel const channel, Midi::Note const note) const noexcept
{
    return head == encode(channel, note);
}


void NoteStack::top(Midi::Channel& channel, Midi::Note& note, Number& velocity) const noexcept
{
    top(channel, note);
    velocity = is_empty() ? 0.0 : velocities[head];
}


void NoteStack::top(Midi::Channel& channel, Midi::Note& note) const noexcept
{
    decode(head, channel, note);
}


void NoteStack::oldest(Midi::Channel& channel, Midi::Note& note) const noexcept
{
    decode(oldest_, channel, note);
}


void NoteStack::lowest(Midi::Channel& channel, Midi::Note& note) const noexcept
{
    decode(lowest_, channel, note);
}


void NoteStack::highest(Midi::Channel& channel, Midi::Note& note) const noexcept
{
    decode(highest_, channel, note);
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

    if (oldest_ == INVALID_ITEM) {
        oldest_ = item;
    }

    if (is_already_pushed(item)) {
        remove<false>(item);
    }

    if (head != INVALID_ITEM) {
        previous[head] = item;
    }

    next[item] = head;
    head = item;
    velocities[item] = velocity;

    if (lowest_ == INVALID_ITEM || note < get_note(lowest_)) {
        lowest_ = item;
    }

    if (highest_ == INVALID_ITEM || note > get_note(highest_)) {
        highest_ = item;
    }
}


bool NoteStack::is_invalid(
        Midi::Channel const channel,
        Midi::Note const note
) const noexcept {
    return channel > Midi::CHANNEL_MAX || note > Midi::NOTE_MAX;
}


bool NoteStack::is_already_pushed(Midi::Word const word) const noexcept
{
    return head == word || previous[word] != INVALID_ITEM;
}


void NoteStack::pop(Midi::Channel& channel, Midi::Note& note, Number& velocity) noexcept
{
    if (is_empty()) {
        decode(INVALID_ITEM, channel, note);
        velocity = 0.0;

        return;
    }

    Midi::Word const item = head;

    head = next[item];

    if (head != INVALID_ITEM) {
        previous[head] = INVALID_ITEM;
    }

    next[item] = INVALID_ITEM;

    velocity = velocities[item];
    decode(item, channel, note);

    update_extremes(item);
}


void NoteStack::update_extremes(Midi::Word const changed_item) noexcept
{
    if (is_empty()) {
        lowest_ = INVALID_ITEM;
        highest_ = INVALID_ITEM;

        return;
    }

    if (changed_item == lowest_) {
        lowest_ = INVALID_ITEM;
    }

    if (changed_item == highest_) {
        highest_ = INVALID_ITEM;
    }

    Midi::Word item = head;
    Midi::Note lowest_note = get_note(lowest_);
    Midi::Note highest_note = get_note(highest_);
    Midi::Note note;

    for (size_t i = 0; item != INVALID_ITEM && i != ITEMS; ++i) {
        note = get_note(item);

        if (lowest_ == INVALID_ITEM || note < lowest_note) {
            lowest_note = note;
            lowest_ = item;
        }

        if (highest_ == INVALID_ITEM || note > highest_note) {
            highest_note = note;
            highest_ = item;
        }

        item = next[item];
    }
}


void NoteStack::remove(Midi::Channel const channel, Midi::Note const note) noexcept
{
    if (JS80P_UNLIKELY(is_invalid(channel, note))) {
        return;
    }

    remove<true>(encode(channel, note));
}


template<bool should_update_extremes>
void NoteStack::remove(Midi::Word const word) noexcept
{
    Midi::Word const next_item = next[word];
    Midi::Word const previous_item = previous[word];

    if (word == oldest_) {
        oldest_ = previous_item;
    }

    if (next_item != INVALID_ITEM) {
        previous[next_item] = previous_item;
    }

    if (word == head) {
        head = next_item;
    } else {
        if (previous_item != INVALID_ITEM) {
            next[previous_item] = next_item;
        }

        next[word] = INVALID_ITEM;
        previous[word] = INVALID_ITEM;
    }

    if constexpr (should_update_extremes) {
        update_extremes(word);
    }
}


// void NoteStack::dump() const noexcept
// {
    // fprintf(stderr, "  top=\t%hx\n", head);
    // fprintf(stderr, "  lowest=\t%hx\n", lowest_);
    // fprintf(stderr, "  highest=\t%hx\n", highest_);

    // fprintf(stderr, "  next=\t[");

    // for (Midi::Word i = 0; i != ITEMS; ++i) {
        // if (next[i] != INVALID_ITEM) {
            // fprintf(stderr, "%hx-->%hx, ", i, next[i]);
        // }
    // }

    // fprintf(stderr, "]\n  prev=\t[");

    // for (Midi::Word i = 0; i != ITEMS; ++i) {
        // if (previous[i] != INVALID_ITEM) {
            // fprintf(stderr, "%hx-->%hx, ", i, previous[i]);
        // }
    // }

    // fprintf(stderr, "]\n\n");
// }

}

#endif
