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
            rendered(NULL),
            block_size(synth.get_block_size()),
            next_rendered_sample_index(block_size),
            round(0)
        {
        }

        /*
        Some hosts do use variable size buffers, and we don't want delay
        feedback buffers to run out of samples when a long batch is rendered
        after a shorter one, so we split up rendering batches into equal sized
        chunks.
        */
        template<typename NumberType, Operation operation = Operation::OVERWRITE>
        void render(Integer const sample_count, NumberType** buffer)
        {
            if (JS80P_UNLIKELY(sample_count <= 0)) {
                return;
            }

            Integer const block_size = this->block_size;

            Integer next_rendered_sample_index = this->next_rendered_sample_index;
            Integer next_output_sample_index = 0;

            while (next_output_sample_index != sample_count) {
                if (next_rendered_sample_index == block_size) {
                    round = (round + 1) & ROUND_MASK;
                    rendered = synth.generate_samples(round, block_size);
                    next_rendered_sample_index = 0;
                }

                Integer const batch_size = std::min(
                    sample_count - next_output_sample_index,
                    block_size - next_rendered_sample_index
                );

                for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
                    for (Integer i = 0; i != batch_size; ++i) {
                        if constexpr (operation == Operation::OVERWRITE) {
                            buffer[c][next_output_sample_index + i] = (
                                (NumberType)rendered[c][next_rendered_sample_index + i]
                            );
                        } else {
                            buffer[c][next_output_sample_index + i] += (
                                (NumberType)rendered[c][next_rendered_sample_index + i]
                            );
                        }
                    }
                }

                next_rendered_sample_index += batch_size;
                next_output_sample_index += batch_size;
            }

            this->next_rendered_sample_index = next_rendered_sample_index;
        }

        void reset()
        {
            rendered = NULL;
            block_size = synth.get_block_size();
            next_rendered_sample_index = block_size;
        }

    private:
        static constexpr Integer ROUND_MASK = 0x7fffff;

        Synth& synth;
        Sample const* const* rendered;
        Integer block_size;
        Integer next_rendered_sample_index;
        Integer round;
};

}

#endif
