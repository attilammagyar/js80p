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

#include <cstddef>

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

        void start_lfos(Seconds const time_offset) noexcept;
        void stop_lfos(Seconds const time_offset) noexcept;

        void skip_round_for_lfos(
            Integer const round,
            Integer const sample_count
        ) noexcept;

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

        /*
        The delay_time parameter controls the maximum of the centered LFOs
        which control the actual delay time of the delay lines, but for the
        chorus effect, we want to control the midpoint of the oscillation
        instead of the maximum.  Thus, the actual delay time range needs to be
        twice as large as the delay time range that we present to the user.
        */
        static constexpr Number DELAY_TIME_MAX = Constants::CHORUS_DELAY_TIME_MAX * 2.0;
        static constexpr Number DELAY_TIME_DEFAULT = Constants::CHORUS_DELAY_TIME_DEFAULT * 2.0;

        static constexpr size_t VOICES = 3;

        FloatParamS biquad_filter_q;
        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParamS high_pass_filter_gain;
        HighPassedInput high_pass_filter;
        LFO lfos[VOICES];
        FloatParamS delay_times[VOICES];
        CombFilter comb_filters[VOICES];
        Mixer<CombFilter> mixer;
        typename HighShelfFilter::TypeParam high_shelf_filter_type;
        HighShelfFilter high_shelf_filter;
        Gain<HighShelfFilter> feedback_gain;
        Sample const* const* chorused;
};

}

#endif
