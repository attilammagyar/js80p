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

#ifndef JS80P__DSP__LFO_ENVELOPE_LIST_HPP
#define JS80P__DSP__LFO_ENVELOPE_LIST_HPP

#include <cstdint>

#include "js80p.hpp"


namespace JS80P
{

class LFOEnvelopeList
{
    private:
        class EnvelopeIndex
        {
            public:
                static constexpr Byte to_byte(
                    uint64_t const list,
                    Byte const index
                ) noexcept;

                EnvelopeIndex(uint64_t& list, Byte const index);
                EnvelopeIndex(EnvelopeIndex const& envelope_index) = delete;
                EnvelopeIndex(EnvelopeIndex&& envelope_index) = delete;

                operator Byte() const noexcept;

                EnvelopeIndex& operator=(EnvelopeIndex const& envelope_index) = delete;
                EnvelopeIndex& operator=(EnvelopeIndex&& envelope_index) = delete;
                EnvelopeIndex& operator=(Byte const envelope_index);

            private:
                static constexpr Byte index_to_offset(Byte const index) noexcept;

                static constexpr Byte byte_at_offset(
                    uint64_t const list,
                    Byte const offset
                ) noexcept;

                Byte const offset;

                uint64_t& list;
        };

    public:
        LFOEnvelopeList();

        LFOEnvelopeList(
            LFOEnvelopeList const& lfo_envelope_list
        ) noexcept = default;

        LFOEnvelopeList(
            LFOEnvelopeList&& lfo_envelope_list
        ) noexcept = default;

        LFOEnvelopeList& operator=(
            LFOEnvelopeList const& lfo_envelope_list
        ) noexcept = default;

        LFOEnvelopeList& operator=(
            LFOEnvelopeList&& lfo_envelope_list
        ) noexcept = default;

        void clear() noexcept;

        Byte operator[](Byte const index) const noexcept;
        EnvelopeIndex operator[](Byte const index) noexcept;

    private:
        uint64_t list;
};

}

#endif

