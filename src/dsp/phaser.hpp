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

#ifndef JS80P__DSP__PHASER_HPP
#define JS80P__DSP__PHASER_HPP

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/effect.hpp"
#include "dsp/lfo.hpp"
#include "dsp/mixer.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Phaser : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef BiquadFilter<InputSignalProducerClass> HighPassedInput;
        typedef BiquadFilter<HighPassedInput> PhaserFilter;
        typedef BiquadFilter< Mixer<PhaserFilter> > HighShelfFilter;

        Phaser(std::string const name, InputSignalProducerClass& input);

        ToggleParam tempo_sync;
        ToggleParam phaser_filter_log_scale;
        FloatParam lfo_frequency;
        FloatParam phaser_filter_frequency;
        FloatParam phaser_filter_q;
        FloatParam depth;
        FloatParam damping_frequency;
        FloatParam damping_gain;
        FloatParam phase_difference;
        FloatParam high_pass_frequency;

        LFO lfo_1;
        LFO lfo_2;

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
        FloatParam biquad_filter_q;

        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParam high_pass_filter_gain;
        HighPassedInput high_pass_filter;

        typename PhaserFilter::TypeParam phaser_filter_type;

        PhaserFilter phaser_filter_1;
        PhaserFilter phaser_filter_2;

        Mixer<PhaserFilter> mixer;

        typename HighShelfFilter::TypeParam high_shelf_filter_type;
        HighShelfFilter high_shelf_filter;

        Sample const* const* phaser_filtered;
};

}

#endif
