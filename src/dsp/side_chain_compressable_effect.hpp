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

#ifndef JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_HPP
#define JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/effect.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class SideChainCompressableEffect : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        SideChainCompressableEffect(
            std::string const name,
            InputSignalProducerClass& input,
            Integer const number_of_children = 0
        );

        FloatParamB side_chain_compression_threshold;
        FloatParamB side_chain_compression_attack_time;
        FloatParamB side_chain_compression_release_time;
        FloatParamB side_chain_compression_gain_reduction;

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
        enum Action {
            BYPASS = 0,
            COMPRESS = 1,
        };

        Action decide_next_action(Integer const sample_count) const noexcept;

        FloatParamS gain;

        Sample const* gain_buffer;
        Action previous_action;
        bool is_bypassing;
};

}

#endif

