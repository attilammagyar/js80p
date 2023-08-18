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

#ifndef JS80P__DSP__REVERB_HPP
#define JS80P__DSP__REVERB_HPP

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/effect.hpp"
#include "dsp/mixer.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Reverb : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef BiquadFilter<InputSignalProducerClass> HighPassedInput;
        typedef HighShelfPannedDelay<HighPassedInput> CombFilter;

        Reverb(std::string const name, InputSignalProducerClass& input);

        virtual ~Reverb();

        FloatParamS room_size;
        FloatParamS damping_frequency;
        FloatParamS damping_gain;
        FloatParamS width;
        FloatParamS high_pass_frequency;
        ToggleParam log_scale_frequencies;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

    private:
        static constexpr Integer COMB_FILTERS = 8;

        static constexpr Seconds TUNINGS[COMB_FILTERS] = {
            1557.0 / 44100.0,
            1617.0 / 44100.0,
            1491.0 / 44100.0,
            1422.0 / 44100.0,
            1277.0 / 44100.0,
            1356.0 / 44100.0,
            1188.0 / 44100.0,
            1116.0 / 44100.0,
        };

        Mixer<CombFilter> mixer;

        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParamS high_pass_filter_q;
        FloatParamS high_pass_filter_gain;

        HighPassedInput high_pass_filter;
        CombFilter* comb_filters[COMB_FILTERS];
};

}

#endif
