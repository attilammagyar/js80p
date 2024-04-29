/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2024  Attila M. Magyar
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

#ifndef JS80P__DSP__LFO_ENVELOPE_LIST_CPP
#define JS80P__DSP__LFO_ENVELOPE_LIST_CPP

#include <cstddef>

#include "dsp/lfo_envelope_list.hpp"


namespace JS80P
{

constexpr Byte LFOEnvelopeList::EnvelopeIndex::to_byte(
        uint64_t const list,
        Byte const index
) noexcept {
    return byte_at_offset(list, index_to_offset(index));
}


constexpr Byte LFOEnvelopeList::EnvelopeIndex::byte_at_offset(
        uint64_t const list,
        Byte const offset
) noexcept {
    return (Byte)((list >> offset) & Constants::ENVELOPE_INDEX_MASK);
}


constexpr Byte LFOEnvelopeList::EnvelopeIndex::index_to_offset(
        Byte const index
) noexcept {
    JS80P_ASSERT(index < Constants::PARAM_LFO_ENVELOPE_STATES);

    return index * Constants::ENVELOPE_INDEX_BITS;
}


LFOEnvelopeList::EnvelopeIndex::EnvelopeIndex(uint64_t& list, Byte const index)
    : offset(index_to_offset(index)),
    list(list)
{
}


LFOEnvelopeList::EnvelopeIndex::operator Byte() const noexcept
{
    return byte_at_offset(list, offset);
}


LFOEnvelopeList::EnvelopeIndex& LFOEnvelopeList::EnvelopeIndex::operator=(
        Byte const envelope_index
) {
    list &= ~(Constants::ENVELOPE_INDEX_MASK << offset);
    list |= (
        (uint64_t)(envelope_index & Constants::ENVELOPE_INDEX_MASK) << offset
    );

    return *this;
}


LFOEnvelopeList::LFOEnvelopeList() : list(0)
{
    JS80P_ASSERT(
        (Integer)Constants::INVALID_ENVELOPE_INDEX <= (Integer)((1 << Constants::ENVELOPE_INDEX_BITS) - 1)
    );
    JS80P_ASSERT(
        (size_t)(Constants::ENVELOPE_INDEX_BITS * Constants::ENVELOPES) <= sizeof(list) * 8
    );

    clear();
}


void LFOEnvelopeList::clear() noexcept
{
    for (Byte index = 0; index != Constants::PARAM_LFO_ENVELOPE_STATES; ++index) {
        (*this)[index] = Constants::INVALID_ENVELOPE_INDEX;
    }
}


Byte LFOEnvelopeList::operator[](Byte const index) const noexcept
{
    return EnvelopeIndex::to_byte(list, index);
}


LFOEnvelopeList::EnvelopeIndex LFOEnvelopeList::operator[](
        Byte const index
) noexcept {
    return EnvelopeIndex(list, index);
}

}

#endif
