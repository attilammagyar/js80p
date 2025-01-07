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

#ifndef JS80P__DSP__COMPRESSOR_HPP
#define JS80P__DSP__COMPRESSOR_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/side_chain_compressable_effect.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Compressor : public SideChainCompressableEffect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        Compressor(
            std::string const& name,
            InputSignalProducerClass& input,
            SignalProducer* const buffer_owner = NULL
        );

        FloatParamB& threshold;
        FloatParamB& attack_time;
        FloatParamB& release_time;
        FloatParamB& ratio;
        CompressionModeParam& mode;
};

}

#endif

