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

#ifndef JS80P__SYNTH__COMB_FILTER_HPP
#define JS80P__SYNTH__COMB_FILTER_HPP

#include "js80p.hpp"

#include "synth/biquad_filter.hpp"
#include "synth/delay.hpp"
#include "synth/filter.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
using HighShelfDelay = BiquadFilter< Delay<InputSignalProducerClass> >;


template<class InputSignalProducerClass>
class CombFilter : public Filter< HighShelfDelay<InputSignalProducerClass> >
{
    friend class SignalProducer;

    public:
        enum StereoMode {
            NORMAL = 0,
            FLIPPED = 1,
        };

        CombFilter(
            InputSignalProducerClass& input,
            StereoMode const stereo_mode,
            ToggleParam const* tempo_sync = NULL
        );

        CombFilter(
            InputSignalProducerClass& input,
            StereoMode const stereo_mode,
            FloatParam& panning_leader,
            FloatParam& delay_gain_leader,
            FloatParam& delay_time_leader,
            FloatParam& high_shelf_filter_frequency_leader,
            FloatParam& high_shelf_filter_gain_leader,
            ToggleParam const* tempo_sync = NULL
        );

        CombFilter(
            InputSignalProducerClass& input,
            StereoMode const stereo_mode,
            FloatParam& panning_leader,
            FloatParam& delay_gain_leader,
            Seconds const delay_time,
            FloatParam& high_shelf_filter_frequency_leader,
            FloatParam& high_shelf_filter_gain_leader,
            ToggleParam const* tempo_sync = NULL
        );

        virtual ~CombFilter();

        virtual void set_block_size(Integer const new_block_size) noexcept override;

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
        void initialize_instance() noexcept;

        bool const is_flipped;

        typename HighShelfDelay<InputSignalProducerClass>::TypeParam high_shelf_filter_type;
        FloatParam high_shelf_filter_q;

        Sample** stereo_gain_buffer;
        Sample const* panning_buffer;
        Sample stereo_gain_value[2];
        Sample panning_value;

    public:
        FloatParam panning;

        Delay<InputSignalProducerClass> delay;
        HighShelfDelay<InputSignalProducerClass> high_shelf_filter;
};

}

#endif
