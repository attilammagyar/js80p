/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#include <algorithm>

#include "js80p.hpp"

#include "synth.hpp"


namespace JS80P
{

class Renderer
{
    public:
        enum Operation {
            ADD = 0,
            OVERWRITE = 1,
        };

        explicit Renderer(Synth& synth) noexcept
            : block_size(synth.get_block_size()),
            channels(synth.get_channels()),
            synth(synth),
            rendered(NULL),
            next_synth_sample_index(block_size),
            round(0)
        {
            input = new Sample*[channels];

            for (Integer c = 0; c != channels; ++c) {
                input[c] = new Sample[block_size];

                std::fill_n(input[c], block_size, 0.0);
            }
        }

        ~Renderer()
        {
            for (Integer c = 0; c != channels; ++c) {
                delete[] input[c];

                input[c] = NULL;
            }

            delete[] input;

            input = NULL;
        }

        Integer get_latency_samples() const noexcept
        {
            return block_size;
        }

        /*
        Some hosts do use variable size buffers, and we don't want delay
        feedback buffers to run out of samples when a long batch is rendered
        after a shorter one, so we split up rendering batches into equal sized
        chunks.
        */
        template<typename NumberType, Operation operation = Operation::OVERWRITE>
        void render(
                Integer const sample_count,
                NumberType const* const* const in_samples,
                NumberType** out_samples
        ) noexcept {
            if (JS80P_UNLIKELY(sample_count <= 0)) {
                return;
            }

            Integer const block_size = this->block_size;

            Integer next_synth_sample_index = this->next_synth_sample_index;
            Integer next_host_sample_index = 0;

            while (next_host_sample_index != sample_count) {
                if (next_synth_sample_index == block_size) {
                    next_synth_sample_index = 0;
                    round = (round + 1) & ROUND_MASK;
                    rendered = synth.generate_samples(round, block_size, input);
                }

                Integer const batch_size = std::min(
                    sample_count - next_host_sample_index,
                    block_size - next_synth_sample_index
                );

                if (JS80P_LIKELY(input != NULL)) {
                    if (JS80P_LIKELY(in_samples != NULL)) {
                        for (Integer c = 0; c != Synth::IN_CHANNELS; ++c) {
                            NumberType const* const src_channel = in_samples[c];
                            Sample* const dst_channel = input[c];

                            for (Integer i = 0; i != batch_size; ++i) {
                                dst_channel[next_synth_sample_index + i] = (
                                    (Sample)src_channel[next_host_sample_index + i]
                                );
                            }
                        }
                    } else {
                        for (Integer c = 0; c != Synth::IN_CHANNELS; ++c) {
                            Sample* const dst_channel = input[c];

                            for (Integer i = 0; i != batch_size; ++i) {
                                dst_channel[next_synth_sample_index + i] = 0.0;
                            }
                        }
                    }
                }

                for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
                    Sample const* const src_channel = rendered[c];
                    NumberType* const dst_channel = out_samples[c];

                    for (Integer i = 0; i != batch_size; ++i) {
                        if constexpr (operation == Operation::OVERWRITE) {
                            dst_channel[next_host_sample_index + i] = (
                                (NumberType)src_channel[next_synth_sample_index + i]
                            );
                        } else {
                            dst_channel[next_host_sample_index + i] += (
                                (NumberType)src_channel[next_synth_sample_index + i]
                            );
                        }
                    }
                }

                next_synth_sample_index += batch_size;
                next_host_sample_index += batch_size;
            }

            this->next_synth_sample_index = next_synth_sample_index;
        }

        void reset() noexcept
        {
            rendered = NULL;
            next_synth_sample_index = block_size;

            for (Integer c = 0; c != channels; ++c) {
                std::fill_n(input[c], block_size, 0.0);
            }
        }

    private:
        static constexpr Integer ROUND_MASK = 0x7fffff;

        Integer const block_size;
        Integer const channels;

        Synth& synth;
        Sample const* const* rendered;
        Sample** input;
        Integer next_synth_sample_index;
        Integer round;
};

}

#endif
