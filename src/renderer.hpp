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
        enum Operation
        {
            ADD = 0,
            OVERWRITE = 1,
        };

        Renderer(Synth& synth)
            : synth(synth),
            round(0),
            previous_round_sample_count(0)
        {
        }

        /*
        Some hosts do use variable size buffers, and we don't want delay
        feedback buffers to run out of samples when a long batch is rendered
        after a shorter one, so we split up rendering batches into chunks that
        are smaller than the previously rendered batch.
        */
        template<typename NumberType, Operation operation = Operation::OVERWRITE>
        void render(Integer const sample_count, NumberType** buffer)
        {
            if (UNLIKELY(sample_count <= 0)) {
                return;
            }

            Integer const previous_round_sample_count = (
                LIKELY(this->previous_round_sample_count != 0)
                    ? this->previous_round_sample_count
                    : sample_count
            );

            Integer buffer_pos = 0;
            Integer remaining = sample_count;

            while (remaining > 0) {
                round = (round + 1) & ROUND_MASK;

                Integer const round_size = std::min(
                    previous_round_sample_count, remaining
                );

                remaining -= round_size;

                Sample const* const* samples = synth.generate_samples(
                    round, round_size
                );

                if constexpr (operation == Operation::OVERWRITE) {
                    for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
                        for (Integer i = 0; i != round_size; ++i) {
                            buffer[c][buffer_pos + i] = (NumberType)samples[c][i];
                        }
                    }
                } else {
                    for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
                        for (Integer i = 0; i != round_size; ++i) {
                            buffer[c][buffer_pos + i] += (NumberType)samples[c][i];
                        }
                    }
                }

                buffer_pos += round_size;
            }

            this->previous_round_sample_count = sample_count;
        }

        void reset()
        {
            previous_round_sample_count = 0;
        }

    private:
        static constexpr Integer ROUND_MASK = 0x7fffff;

        Synth& synth;
        Integer round;
        Integer previous_round_sample_count;
};

}

#endif
