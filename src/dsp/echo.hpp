/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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

#ifndef JS80P__DSP__ECHO_HPP
#define JS80P__DSP__ECHO_HPP

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/distortion.hpp"
#include "dsp/gain.hpp"
#include "dsp/param.hpp"
#include "dsp/side_chain_compressable_effect.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Echo : public SideChainCompressableEffect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef Gain<InputSignalProducerClass> BoostedInput;
        typedef BiquadFilter<BoostedInput, BiquadFilterFixedType::BFFT_HIGH_PASS> HighPassedInput;

        typedef DistortedHighShelfStereoPannedDelay<
            HighPassedInput, DelayCapabilities::DC_REVERSIBLE
        > CombFilter1;

        typedef DistortedHighShelfStereoPannedDelay<
            DistortedHighShelfDelay<HighPassedInput, DelayCapabilities::DC_REVERSIBLE>,
            DelayCapabilities::DC_REVERSIBLE
        > CombFilter2;

        Echo(
            std::string const& name,
            InputSignalProducerClass& input,
            BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers
        );

        FloatParamS delay_time;
        FloatParamS input_volume;
        FloatParamS feedback;
        FloatParamS damping_frequency;
        FloatParamS damping_gain;
        FloatParamS width;
        FloatParamS high_pass_frequency;
        FloatParamS high_pass_q;
        FloatParamS distortion_level;
        ToggleParam tempo_sync;
        ToggleParam log_scale_frequencies;
        ToggleParam log_scale_high_pass_q;
        ToggleParam reversed_1;
        ToggleParam reversed_2;

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
        Distortion::TypeParam distortion_type;
        FloatParamS high_pass_filter_gain;

        BoostedInput gain;
        HighPassedInput high_pass_filter;
        CombFilter1 comb_filter_1;
        CombFilter2 comb_filter_2;

        Sample const* const* comb_filter_1_buffer;
        Sample const* const* comb_filter_2_buffer;
};

}

#endif
