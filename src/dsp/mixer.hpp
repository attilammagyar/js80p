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

#ifndef JS80P__DSP__MIXER_HPP
#define JS80P__DSP__MIXER_HPP

#include <cstddef>
#include <vector>

#include "js80p.hpp"

#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Mixer : public SignalProducer
{
    friend class SignalProducer;

    public:
        Mixer(Integer const channels) noexcept;

        void add(InputSignalProducerClass& input) noexcept;
        void set_weight(size_t const input_index, Number const weight) noexcept;
        void set_output_buffer(Sample** output) noexcept;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

    private:
        class Input {
            public:
                Input(InputSignalProducerClass* input);
                Input(Input const& input) = default;
                Input(Input&& input) = default;

                Input& operator=(Input const& input) = default;
                Input& operator=(Input&& input) = default;

                InputSignalProducerClass* input;
                Sample const* const* buffer;
                Number weight;
        };

        static constexpr Number SILENCE_WEIGHT = 0.000001;

        template<bool has_weights>
        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        Sample** output;
        std::vector<Input> inputs;
        bool has_weights;
};

}

#endif
