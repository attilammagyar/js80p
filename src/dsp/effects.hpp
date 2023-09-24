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

#ifndef JS80P__DSP__EFFECTS_HPP
#define JS80P__DSP__EFFECTS_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/distortion.hpp"
#include "dsp/echo.hpp"
#include "dsp/filter.hpp"
#include "dsp/gain.hpp"
#include "dsp/param.hpp"
#include "dsp/reverb.hpp"


namespace JS80P { namespace Effects
{

template<class InputSignalProducerClass>
using Overdrive = JS80P::Distortion<InputSignalProducerClass>;

template<class InputSignalProducerClass>
using Distortion = JS80P::Distortion< Overdrive<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Filter1 = BiquadFilter< Distortion<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Filter2 = BiquadFilter< Filter1<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using GainEffect = Gain< Filter2<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Chorus = JS80P::Chorus< GainEffect<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Echo = JS80P::Echo< Chorus<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Reverb = JS80P::Reverb< Echo<InputSignalProducerClass> >;


template<class InputSignalProducerClass>
class Effects : public Filter< Reverb<InputSignalProducerClass> >
{
    public:
        Effects(std::string const name, InputSignalProducerClass& input);

        FloatParamS gain_param;

        Overdrive<InputSignalProducerClass> overdrive;
        Distortion<InputSignalProducerClass> distortion;
        typename Filter1<InputSignalProducerClass>::TypeParam filter_1_type;
        typename Filter2<InputSignalProducerClass>::TypeParam filter_2_type;
        ToggleParam filter_1_log_scale;
        ToggleParam filter_2_log_scale;
        Filter1<InputSignalProducerClass> filter_1;
        Filter2<InputSignalProducerClass> filter_2;
        GainEffect<InputSignalProducerClass> gain;
        Chorus<InputSignalProducerClass> chorus;
        Echo<InputSignalProducerClass> echo;
        Reverb<InputSignalProducerClass> reverb;
};

} }

#endif
