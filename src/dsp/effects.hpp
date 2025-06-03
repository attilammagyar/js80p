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

#ifndef JS80P__DSP__EFFECTS_HPP
#define JS80P__DSP__EFFECTS_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/distortion.hpp"
#include "dsp/echo.hpp"
#include "dsp/filter.hpp"
#include "dsp/gain.hpp"
#include "dsp/math.hpp"
#include "dsp/param.hpp"
#include "dsp/reverb.hpp"
#include "dsp/tape.hpp"


namespace JS80P { namespace Effects
{

template<class InputSignalProducerClass>
using Volume1 = Gain<InputSignalProducerClass>;

template<class InputSignalProducerClass>
using Distortion1 = JS80P::Distortion::Distortion< Volume1<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Distortion2 = JS80P::Distortion::Distortion< Distortion1<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Filter1 = BiquadFilter< Distortion2<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Filter2 = BiquadFilter< Filter1<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Volume2 = Gain< Filter2<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Tape1 = Tape< Volume2<InputSignalProducerClass>, ToggleParam::OFF >;

template<class InputSignalProducerClass>
using Chorus = JS80P::Chorus< Tape1<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Echo = JS80P::Echo< Chorus<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Reverb = JS80P::Reverb< Echo<InputSignalProducerClass> >;

template<class InputSignalProducerClass>
using Tape2 = Tape< Reverb<InputSignalProducerClass>, ToggleParam::ON >;

template<class InputSignalProducerClass>
using Volume3 = Gain< Tape2<InputSignalProducerClass> >;


template<class InputSignalProducerClass>
class Effects : public Filter< Volume3<InputSignalProducerClass> >
{
    public:
        static constexpr Integer CHANNELS = 2;

        Effects(
            std::string const& name,
            InputSignalProducerClass& input,
            BiquadFilterSharedBuffers& echo_filter_shared_buffers,
            BiquadFilterSharedBuffers& reverb_filter_shared_buffers,
            Math::RNG& rng
        );

        FloatParamS volume_1_gain;
        FloatParamS volume_2_gain;
        FloatParamS volume_3_gain;

        Distortion::TypeParam distortion_1_type;
        Distortion::TypeParam distortion_2_type;

        ToggleParam tape_at_end;
        TapeParams tape_params;

        Volume1<InputSignalProducerClass> volume_1;
        Distortion1<InputSignalProducerClass> distortion_1;
        Distortion2<InputSignalProducerClass> distortion_2;
        BiquadFilterTypeParam filter_1_type;
        BiquadFilterTypeParam filter_2_type;
        ToggleParam filter_1_freq_log_scale;
        ToggleParam filter_1_q_log_scale;
        ToggleParam filter_2_freq_log_scale;
        ToggleParam filter_2_q_log_scale;
        Filter1<InputSignalProducerClass> filter_1;
        Filter2<InputSignalProducerClass> filter_2;
        Volume2<InputSignalProducerClass> volume_2;
        Tape1<InputSignalProducerClass> tape_1;
        Chorus<InputSignalProducerClass> chorus;
        Echo<InputSignalProducerClass> echo;
        Reverb<InputSignalProducerClass> reverb;
        Tape2<InputSignalProducerClass> tape_2;
        Volume3<InputSignalProducerClass> volume_3;
};

} }

#endif
