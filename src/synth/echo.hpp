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

#ifndef JS80P__SYNTH__ECHO_HPP
#define JS80P__SYNTH__ECHO_HPP

#include "js80p.hpp"

#include "synth/biquad_filter.hpp"
#include "synth/comb_filter.hpp"
#include "synth/effect.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Echo : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef BiquadFilter<InputSignalProducerClass> HighPassInput;
        typedef CombFilter<HighPassInput> CombFilter1;
        typedef CombFilter< HighShelfDelay< HighPassInput > > CombFilter2;

        Echo(std::string const name, InputSignalProducerClass& input);

        FloatParam delay_time;
        FloatParam feedback;
        FloatParam damping_frequency;
        FloatParam damping_gain;
        FloatParam width;
        FloatParam high_pass_frequency;

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
        typename BiquadFilter<InputSignalProducerClass>::TypeParam high_pass_filter_type;
        FloatParam high_pass_filter_q;
        FloatParam high_pass_filter_gain;

        BiquadFilter<InputSignalProducerClass> high_pass_filter;
        CombFilter1 comb_filter_1;
        CombFilter2 comb_filter_2;

        Sample const* const* comb_filter_1_buffer;
        Sample const* const* comb_filter_2_buffer;
};

}

#endif
