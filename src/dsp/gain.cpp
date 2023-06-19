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

#ifndef JS80P__DSP__GAIN_CPP
#define JS80P__DSP__GAIN_CPP

#include "dsp/gain.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Gain<InputSignalProducerClass>::Gain(
        InputSignalProducerClass& input,
        FloatParam& gain
) noexcept
    : Filter<InputSignalProducerClass>(input),
    gain_buffer(NULL),
    gain(gain)
{
}


template<class InputSignalProducerClass>
Sample const* const* Gain<InputSignalProducerClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    Sample const* const* const input_buffer = (
        Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count)
    );
    gain_buffer = FloatParam::produce_if_not_constant(gain, round, sample_count);

    if (gain_buffer == NULL && std::fabs(1.0 - gain.get_value()) < 0.000001) {
        return input_buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Gain<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->get_channels();
    Sample const* const* const input_buffer = this->input_buffer;

    if (gain_buffer == NULL) {
        Number const gain_value = gain.get_value();

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = gain_value * input_buffer[c][i];
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = gain_buffer[i] * input_buffer[c][i];
            }
        }
    }
}

}

#endif
