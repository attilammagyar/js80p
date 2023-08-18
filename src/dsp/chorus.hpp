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

#ifndef JS80P__DSP__CHORUS_HPP
#define JS80P__DSP__CHORUS_HPP

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/effect.hpp"
#include "dsp/gain.hpp"
#include "dsp/lfo.hpp"
#include "dsp/mixer.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Chorus : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef BiquadFilter<InputSignalProducerClass> HighPassedInput;
        typedef PannedDelay<HighPassedInput> CombFilter;
        typedef BiquadFilter< Mixer<CombFilter> > HighShelfFilter;

        Chorus(std::string const name, InputSignalProducerClass& input);

        FloatParamS delay_time;
        FloatParamS frequency;
        FloatParamS depth;
        FloatParamS feedback;
        FloatParamS damping_frequency;
        FloatParamS damping_gain;
        FloatParamS width;
        FloatParamS high_pass_frequency;
        ToggleParam tempo_sync;
        ToggleParam log_scale_frequencies;

        LFO lfo_1;
        LFO lfo_2;
        LFO lfo_3;

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
        static constexpr Number FEEDBACK_SCALE_INV = (
            1.0 / (Number)Constants::CHORUS_FEEDBACK_SCALE
        );

        FloatParamS biquad_filter_q;

        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParamS high_pass_filter_gain;
        HighPassedInput high_pass_filter;

        FloatParamS delay_time_1;
        FloatParamS delay_time_2;
        FloatParamS delay_time_3;

        CombFilter comb_filter_1;
        CombFilter comb_filter_2;
        CombFilter comb_filter_3;

        Mixer<CombFilter> mixer;

        typename HighShelfFilter::TypeParam high_shelf_filter_type;
        HighShelfFilter high_shelf_filter;

        Gain<HighShelfFilter> feedback_gain;

        Sample const* const* chorused;
};

}

#endif
