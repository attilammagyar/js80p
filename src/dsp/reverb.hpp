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

#include <cstddef>

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/mixer.hpp"
#include "dsp/param.hpp"
#include "dsp/side_chain_compressable_effect.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Reverb : public SideChainCompressableEffect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef Byte Type;

        typedef BiquadFilter<InputSignalProducerClass> HighPassedInput;
        typedef HighShelfPannedDelay<HighPassedInput> CombFilter;

        static constexpr Type FREEVERB = 0;

        class TypeParam : public Param<Type, ParamEvaluation::BLOCK>
        {
            public:
                TypeParam(std::string const name) noexcept;
        };

        Reverb(std::string const name, InputSignalProducerClass& input);

        virtual ~Reverb();

        virtual void reset() noexcept override;

        TypeParam type;
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
        class CombFilterTuning
        {
            public:
                constexpr CombFilterTuning(
                        Seconds const delay_time,
                        Number const weight,
                        Number const panning_scale
                ) : delay_time(delay_time),
                    weight(weight),
                    panning_scale(panning_scale) {}

                Seconds const delay_time;
                Number const weight;
                Number const panning_scale;
        };

        static constexpr size_t COMB_FILTERS = 10;

        static constexpr Seconds DELAY_TIME_MAX = 0.3;

        static constexpr CombFilterTuning TUNINGS[][COMB_FILTERS] = {
            /*
            Tunings from Freeverb:
            https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
            */
            {
                {1557.0 / 44100.0, 1.00,  1.00},
                {1617.0 / 44100.0, 1.00, -1.00},
                {1491.0 / 44100.0, 1.00,  1.00},
                {1422.0 / 44100.0, 1.00, -1.00},
                {1277.0 / 44100.0, 1.00,  1.00},
                {1356.0 / 44100.0, 1.00, -1.00},
                {1188.0 / 44100.0, 1.00,  1.00},
                {1116.0 / 44100.0, 1.00, -1.00},
                {0.0,              0.00,  1.00},
                {0.0,              0.00, -1.00},
            },
        };

        Mixer<CombFilter> mixer;

        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParamS high_pass_filter_q;
        FloatParamS high_pass_filter_gain;

        HighPassedInput high_pass_filter;
        CombFilter* comb_filters[COMB_FILTERS];
        Type previous_type;
};

}

#endif
