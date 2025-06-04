/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__COMPRESSOR_CPP
#define JS80P__DSP__COMPRESSOR_CPP

#include "dsp/compressor.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Compressor<InputSignalProducerClass>::Compressor(
        std::string const& name,
        InputSignalProducerClass& input
) : SideChainCompressableEffect<InputSignalProducerClass>(
        name, input, 0, NULL
    ),
    threshold(this->side_chain_compression_threshold),
    attack_time(this->side_chain_compression_attack_time),
    release_time(this->side_chain_compression_release_time),
    ratio(this->side_chain_compression_ratio),
    mode(this->side_chain_compression_mode)
{
}


template<class InputSignalProducerClass>
Sample const* const* Compressor<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = (
        SideChainCompressableEffect<InputSignalProducerClass>::initialize_rendering(
            round, sample_count
        )
    );

    if (buffer == NULL) {
        copy_input(sample_count);
    }

    return buffer;
}


template<class InputSignalProducerClass>
void Compressor<InputSignalProducerClass>::copy_input(
        Integer const sample_count
) noexcept {
    for (Integer c = 0; c != this->channels; ++c) {
        Sample const* const in_channel = this->input_buffer[c];
        Sample* const out_channel = this->buffer[c];

        for (Integer i = 0; i != sample_count; ++i) {
            out_channel[i] = in_channel[i];
        }
    }
}

}

#endif
