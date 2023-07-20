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

#ifndef JS80P__DSP__MIXER_CPP
#define JS80P__DSP__MIXER_CPP

#include "dsp/mixer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Mixer<InputSignalProducerClass>::Mixer(Integer const channels) noexcept
    : SignalProducer(channels, 0),
    output(NULL)
{
}


template<class InputSignalProducerClass>
void Mixer<InputSignalProducerClass>::add(InputSignalProducerClass& input) noexcept
{
    inputs.push_back(Input(&input));
}


template<class InputSignalProducerClass>
void Mixer<InputSignalProducerClass>::set_output_buffer(Sample** output) noexcept
{
    this->output = output;
}


template<class InputSignalProducerClass>
Sample const* const* Mixer<InputSignalProducerClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    for (typename std::vector<Input>::iterator it = inputs.begin(); it != inputs.end(); ++it) {
        it->buffer = SignalProducer::produce<InputSignalProducerClass>(*it->input, round, sample_count);
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Mixer<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = get_channels();
    Sample** output = this->output != NULL ? this->output : buffer;

    render_silence(round, first_sample_index, last_sample_index, output);

    for (typename std::vector<Input>::iterator it = inputs.begin(); it != inputs.end(); ++it) {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                output[c][i] += it->buffer[c][i];
            }
        }
    }
}


template<class InputSignalProducerClass>
Mixer<InputSignalProducerClass>::Input::Input(InputSignalProducerClass* input)
    : input(input),
    buffer(NULL)
{
}


}

#endif
