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

#ifndef JS80P__RENDERER_HPP
#define JS80P__RENDERER_HPP

#include "js80p.hpp"

#include "synth.hpp"


namespace JS80P
{

class Renderer
{
    public:
        Renderer(Synth& synth)
            : synth(synth),
            round(0),
            previous_round_sample_count(0)
        {
        }

        void reset()
        {
            previous_round_sample_count = 0;
        }

/*
Some hosts do use variable size buffers, and we don't want delay feedback
buffers to run out of samples when a long batch is rendered after a shorter one.
*/
#define _JS80P_RENDER_SPLIT_BATCH(sample_count, buffer, op)                     \
{                                                                               \
    if (sample_count < 0) {                                                     \
        return;                                                                 \
    }                                                                           \
                                                                                \
    Integer buffer_pos = 0;                                                     \
    Integer remaining = sample_count;                                           \
    Integer previous_round_sample_count = this->previous_round_sample_count;    \
                                                                                \
    if (previous_round_sample_count == 0) {                                     \
        previous_round_sample_count = sample_count;                             \
    }                                                                           \
                                                                                \
    while (remaining > 0) {                                                     \
        Integer const round_size = std::min(                                    \
            previous_round_sample_count, remaining                              \
        );                                                                      \
                                                                                \
        remaining -= round_size;                                                \
                                                                                \
        Sample const* const* samples = render_next_round(round_size);           \
                                                                                \
        for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {                    \
            for (Integer i = 0; i != round_size; ++i) {                         \
                buffer[c][buffer_pos + i] op (NumberType)samples[c][i];         \
            }                                                                   \
        }                                                                       \
                                                                                \
        buffer_pos += round_size;                                               \
    }                                                                           \
                                                                                \
    this->previous_round_sample_count = sample_count;                           \
}

        template<typename NumberType>
        void add_next_round(Integer const sample_count, NumberType** buffer)
        {
            _JS80P_RENDER_SPLIT_BATCH(sample_count, buffer, +=);
        }

        template<typename NumberType>
        void write_next_round(Integer const sample_count, NumberType** buffer)
        {
            _JS80P_RENDER_SPLIT_BATCH(sample_count, buffer, =);
        }

#undef _JS80P_RENDER_SPLIT_BATCH

    private:
        static constexpr Integer ROUND_MASK = 0x7fff;

        Sample const* const* render_next_round(Integer const sample_count)
        {
            round = (round + 1) & ROUND_MASK;

            return synth.generate_samples(round, sample_count);
        }

        Synth& synth;
        Integer round;
        Integer previous_round_sample_count;
};

}

#endif
