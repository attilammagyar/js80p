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

#ifndef JS80P__DSP__LFO_ENVELOPE_MAPPING_CPP
#define JS80P__DSP__LFO_ENVELOPE_MAPPING_CPP

#include "dsp/lfo_envelope_mapping.hpp"


namespace JS80P
{

constexpr Byte LFOEnvelopeMapping::EnvelopeIndex::to_byte(
        Integer const mapping,
        Byte const offset
) noexcept {
    return (Byte)((mapping >> offset) & Constants::ENVELOPE_INDEX_MASK);
}


LFOEnvelopeMapping::EnvelopeIndex::EnvelopeIndex(
        Integer& mapping,
        Byte const lfo_index
) : offset(lfo_index * Constants::ENVELOPE_INDEX_BITS),
    mapping(mapping)
{
}


LFOEnvelopeMapping::EnvelopeIndex::operator Byte() const noexcept
{
    return to_byte(mapping, offset);
}


LFOEnvelopeMapping::EnvelopeIndex& LFOEnvelopeMapping::EnvelopeIndex::operator=(
        Byte const envelope_index
) {
    mapping &= ~(Constants::ENVELOPE_INDEX_MASK << offset);
    mapping |= (
        (Integer)(envelope_index & Constants::ENVELOPE_INDEX_MASK) << offset
    );

    return *this;
}


LFOEnvelopeMapping::LFOEnvelopeMapping() : mapping(0)
{
    clear();
}


void LFOEnvelopeMapping::clear() noexcept
{
    for (Byte lfo_index = 0; lfo_index != Constants::LFOS; ++lfo_index) {
        (*this)[lfo_index] = Constants::INVALID_ENVELOPE_INDEX;
    }
}


Byte LFOEnvelopeMapping::operator[](Byte const lfo_index) const noexcept
{
    Byte const offset = lfo_index * Constants::ENVELOPE_INDEX_BITS;

    return EnvelopeIndex::to_byte(mapping, offset);
}


LFOEnvelopeMapping::EnvelopeIndex LFOEnvelopeMapping::operator[](
        Byte const lfo_index
) noexcept {
    return EnvelopeIndex(mapping, lfo_index);
}

}

#endif
