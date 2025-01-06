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

template<class InputSignalProducerClass, CompressionCurve curve>
Compressor<InputSignalProducerClass, curve>::Compressor(
        std::string const& name,
        InputSignalProducerClass& input,
        SignalProducer* const buffer_owner,
        Number const makeup_gain
) : SideChainCompressableEffect<InputSignalProducerClass, curve>(
        name,
        input,
        0,
        buffer_owner,
        makeup_gain
    ),
    threshold(this->side_chain_compression_threshold),
    attack_time(this->side_chain_compression_attack_time),
    release_time(this->side_chain_compression_release_time),
    ratio(this->side_chain_compression_ratio),
    mode(this->side_chain_compression_mode)
{
}

}

#endif

